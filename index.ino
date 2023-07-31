// Constant
const byte INT_PIN {2};
const byte ENA_PIN {11};
const byte RS_PIN {12};
const byte SSR_PIN {13};
// Device states
volatile byte mode {0};
volatile bool isUserPressedFinish {false};
bool isRunning {false};
bool isFinishedInitProgram {false};
// LCD Instruction

// Global vars
int normPrevTemp {0};
int isNormTempOverheat {false};
int normTempLimit {40};

class Timer
{
    unsigned int value;
    unsigned long currCount;
    unsigned long prevCount;
    
  public:
    Timer() : value{0}, currCount{0}, prevCount{0} {}
    Timer(unsigned int val)
      : currCount{0} { setCounter(val); }
    ~Timer() = default;
  
    bool checkFinished(){
      currCount = millis();
      return currCount >= (prevCount + value);
    }
    void setCounter(unsigned long val){
      value = val;
      prevCount = millis();
    }
};

Timer normModeTim;
Timer autoModeTim;

void writeLCD(char val, bool isWrite = false){
  char temp {val};
  for(byte port {3}; port <= 10; port++){
    temp = val & 0x1;
    val = val >> 1;
    digitalWrite(port, temp == 1 ? HIGH : LOW);
  }
  
  if(isWrite) digitalWrite(RS_PIN, HIGH); // write data to DDRAM
  else digitalWrite(RS_PIN, LOW);         // else write instructions

  // High-to-low enable LCD data fetch sequence
  setEnablePin();         
  delayMicroseconds(37);
  clearEnablePin();
  // Delay before next write
  delay(5);
}

void writeStringLCD(char *str){
  for(size_t i {0}; str[i] != '\0'; i++)
  {
    writeLCD(str[i], true);
  }
}

void writeIntLCD(int num){
  int count {0};
  int temp {num};
  
  while(temp != 0){
    temp /= 10;
    count++;
  }
  
  moveCursorRightLCD(count - 1);  // move (count - 1) line
  int i {2};
  while(num != 0){
    writeLCD(convertIntDigitToChar(num % 10), true);  // write num backward
    num /= 10;
    if(num == 0) break;
    moveCursorLeftLCD(2); // move cursor to correct backward num
  }
  moveCursorRightLCD(count - 1);  // move to the next line after corrected num display
}

void moveCursorRightLCD(int line){
  for(byte i {0}; i < line; i++){
    writeLCD(0x14);
  }
}

void moveCursorLeftLCD(int line){
  for(byte i {0}; i < line; i++){
    writeLCD(0x10);
  }
}

void clearEnablePin(){
  digitalWrite(ENA_PIN, LOW);
}
void setEnablePin(){
  digitalWrite(ENA_PIN, HIGH);
}

void initLCD(){
  delay(50);      // delay before start init sequence
  writeLCD(0x01); // display clear
  writeLCD(0x38); // function set - 8 bits - 2 lines
  writeLCD(0x0E); // set entire display on, cursor on, blinking of cursor off
  writeLCD(0x06); // entry mode - set cursor move direction
}

void initProgram(){
  moveCursorRightLCD(4);
  writeStringLCD("Loading");
  writeLCD(0xC0);           // set cursor to second line
  for(byte i {0}; i < 8; i++){
    writeLCD('#', true);
    delay(600 + i * 50);
  }
  
  writeLCD(0x80);           // set cursor to first line
  writeStringLCD("Fetching Command");
  writeLCD(0xC7);
  for(byte i {0}; i < 8; i++){
    writeLCD('#', true);
    delay(500 + i * 30);
  }
  
  writeLCD(0x01);           // clear display
  for(byte i {0}; i < 32; i++){
    if(i < 16) writeLCD('>', true);
    else {
      if(i == 16) {
        writeLCD(0xC0);
        moveCursorRightLCD(15);
      }
      writeLCD('<', true);
      moveCursorLeftLCD(2);
    }
    delay(40);
  }
  
  writeLCD(0x01);           // clear display
  moveCursorRightLCD(1);
  writeStringLCD("Access Granted");
  writeLCD(0xC0);           // set cursor to second line
  writeStringLCD("Welcome back Phu");
  delay(4000);

  // set state flag
  isFinishedInitProgram = true;
  
  writeLCD(0x01);           // clear previous display
  writeStringLCD("Select a mode:");
  writeLCD(0xC0);
  writeStringLCD("Auto - Norm");
  // Waiting for user - mode selection (auto/normal)
  Timer initTim{10000};
  while(!initTim.checkFinished()){
    if(mode == 1) break;
  }
  
  writeLCD(0x01);
  moveCursorRightLCD(3);
  writeStringLCD("Temp:");
  writeLCD(0x8C);           // set cursor to address 0x0B - first line - 8 is prefix for "set DDRAM" instruction
  writeLCD(0xDF, true);     // special character * in kanji
  writeLCD('C', true);

  if(mode == 1){
    writeLCD(0xC0);         // set cursor to second line
    moveCursorRightLCD(3);
    writeStringLCD("Mode:Auto");
  } else {
    writeLCD(0xC0);         // set cursor to second line
    writeStringLCD("Mode:Norm");
    moveCursorRightLCD(1);
    writeStringLCD("Mx:");
  }
}

void setup() {
  // OUTPUT to inteface with LCD
  for(byte i {3}; i <= 12; i++){
    pinMode(i, OUTPUT);
  }
  // OUTPUT to drive SSR
  pinMode(SSR_PIN, OUTPUT);
  digitalWrite(SSR_PIN, LOW);
  // INPUT - interrupt
  pinMode(INT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(INT_PIN), setAutoMode, FALLING);
  // ADC - 1.1 Vref
  analogReference(INTERNAL);
  // Testing - will be deleted
  Serial.begin(9600);
  // Init LCD instructions
  initLCD();
  // Init program - text placement
  initProgram();
  // Set program operating state flags to true
  isRunning = true;
  // Turn on TIM based on mode selection
  if(mode == 1){
    autoModeTim.setCounter(1000);
  } else {
    normModeTim.setCounter(15000);
  }
}

void loop() {
  if(mode == 1) doAutoMode();
  else doNormalMode();
}

void doAutoMode(){
  writeLCD(0x88);
  writeIntLCD((getSensorVoltage() * 1024.0 / 1.1));
  if(((getSensorVoltage() * 1024.0 / 1.1, 4) / 10) < 10) writeLCD(' ', true);
  delay(200);
}

void doNormalMode(){
  int temperature{};
  int tempLimit{};
  
  while(!isUserPressedFinish){
    delay(200);
    temperature = getTemperature();
    tempLimit = getTempLimit();

    if(temperature <= tempLimit) digitalWrite(SSR_PIN, HIGH);
    else digitalWrite(SSR_PIN, LOW);
    
    writeLCD(0x88);
    writeIntLCD(temperature);
    if((temperature / 10) < 10) writeLCD(' ', true);
    
    writeLCD(0xC0);
    moveCursorRightLCD(13);
    writeIntLCD(tempLimit);
    if((tempLimit / 10) < 10) writeLCD(' ', true);
  }

  writeLCD(0x01);           // clear display
  writeStringLCD("Program closed");
  writeLCD(0xC0);
  writeStringLCD("See you again!");
  // turn off SSR
  digitalWrite(SSR_PIN, LOW);
  while(1);
}

char convertIntDigitToChar(int digit){
  byte count {0};
  int temp {digit};
  while(temp != 0){
    temp /= 10;
    count++;
  }
  if(count > 1) return 'X';

  return static_cast<char>(digit + 48);
}

float getSensorVoltage(){
//  float amplifiedSensorVoltage = (analogRead(A0)) * 1.1 / 1024.0;
//  return (amplifiedSensorVoltage * 1000.0) / (4700.0 + 1000.0); 
  return (analogRead(A0)) * 1.1 / 1024.0;
}

float getTemperature(){
//  float sensorVoltage = getSensorVoltage();
//  float sensorResistance = (sensorVoltage * 4700.0) / (5.0 - sensorVoltage);
//  return static_cast<int>((sensorResistance - 100.0) / 0.4); 
  float sensorResistance = (getSensorVoltage() * 1000.0) / (5.0 - getSensorVoltage());
  return static_cast<int>((sensorResistance - 100.0) / 0.4);
}

int getTempLimit(){
  return static_cast<int>(analogRead(A1) * 255.0 / 1024.0);
}

void setAutoMode() {
  if(isRunning){
    isUserPressedFinish = true;
    return;
  }

  if(!isFinishedInitProgram) return;
  
  mode = 1;
}

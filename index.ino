// Constant
const byte INT_PIN {2};
const byte ENA_PIN {11};
const byte RS_PIN {12};
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
  for(int port {3}; port <= 10; port++){
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
  for(int i {0}; i < line; i++){
    writeLCD(0x14);
  }
}

void moveCursorLeftLCD(int line){
  for(int i {0}; i < line; i++){
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
  for(int i {0}; i < 8; i++){
    writeLCD('#', true);
    delay(750 + i * 50);
  }
  
  writeLCD(0x80);           // set cursor to first line
  writeStringLCD("Fetching Command");
  writeLCD(0xC7);
  for(int i {0}; i < 8; i++){
    writeLCD('#', true);
    delay(500 + i * 30);
  }
  
  writeLCD(0x01);           // clear display
  for(int i {0}; i < 32; i++){
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
  writeLCD(0x8B);           // set cursor to address 0x0B - first line - 8 is prefix for "set DDRAM" instruction
  writeLCD(0xDF, true);     // special character * in kanji
  writeLCD('C', true);

  if(mode == 1){
    writeLCD(0xC0);         // set cursor to second line
    moveCursorRightLCD(3);
    writeStringLCD("Mode:Auto");
  } else {
    writeLCD(0xC0);         // set cursor to second line
    moveCursorRightLCD(3);
    writeStringLCD("Mode:Norm");
  }
}

void setup() {
  // OUTPUT to inteface with LCD
  for(byte i {3}; i <= 12; i++){
    pinMode(i, OUTPUT);
  }
  // INPUT to drive SSR
  pinMode(A0, INPUT);
  // OUTPUT to switch SSR
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  analogWrite(A1, 255);
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
  
}

void doNormalMode(){ 
  if(normModeTim.checkFinished() && (getTemperature() <= normTempLimit)){
    normModeTim.setCounter(9900);
    if(normTempLimit - getTemperature() <= 5) normTempLimit += 2;
    if(normTempLimit >= 180) isNormTempOverheat = true;
    else if((getTemperature() < 50) && isNormTempOverheat) isNormTempOverheat = false; 
    delay(100);
    Serial.println(normTempLimit);
  }
  
  if(getTemperature() > normTempLimit) analogWrite(A1, 0);
  else analogWrite(A1, 255);
  
  writeLCD(0x88);
  writeIntLCD(getTemperature());
  if((getTemperature() / 10) < 10) writeLCD(' ', true);

  delay(200);
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
  return (analogRead(A0)) * 1.1 / 1024.0;
}

float getTemperature(){
  float sensorResistance = (getSensorVoltage() * 10000.0) / (5.0 - getSensorVoltage());
  return static_cast<int>((sensorResistance - 100.0) / 0.4);
}

void setAutoMode() {
  if(isRunning){
    isUserPressedFinish = true;
    return;
  }

  if(!isFinishedInitProgram) return;
  
  mode = 1;
}

<h1>Custom Hot Plate</h1>
In this project, my goal was to design a reflow hot plate to help me in future projects involving soldering of SMD components.
As I was researching on how a reflow hot plate (or oven) works, I found some useful insights such as a temperature sensor to monitor
the changing temperature of the hot plate, a way to see the current temperature, something that can heats up over time 
and I needed a method to let me control the temperature.

<br />
<br />
<br />

My choice for temperature sensor is a PT-100 RTD (Resistance Temperature Detector) which have high accuracy and can measure very high temperature.
With that I can move on to the next component which is a LCD 16x2 (HD47780 Driver) to display the temperature measured by the sensor.
The heart of the project is the heating element, at first I was thinking of disassemble an electric iron but I went for thermoelectric materials instead.
If you apply electric current to these materials it will create a temperature difference. Lastly, to control the temperature I used an Arduino Nano as the brain for 
this project and a solid-state relay to turn on/off the hot plate.

<br />
<br />

  > - During the reflow process of the PCB, the setting of the reflow temperature curve will greatly affect the strength of the solder joints. It is not only necessary to ensure that each solder joint meets the quality requirements, but also to ensure that the components and boards are not damaged.

With that being said, to get a nice reflow temperature curve, I followed this graph that have the same type of alloy as mine.

![image](https://github.com/KizEvo/hot-plate-project/assets/104358167/65d0f4e8-c827-4455-921f-d02e5d285ebd)

**Note :** After some testings and calibrations, the process in the graph and the actual device process may not closely matched. 

<h2>Preview</h2> 
<h6>T: Time elapsed (s)
<br/>
Temp: Temperature (*C)</h6>

  > - Plate heating up process
  
![heating_up_board](https://github.com/KizEvo/hot-plate-project/assets/104358167/542fc5d1-82be-4744-89ef-393bf982d8c3)

  > - Reflow zone

![reflow_zone](https://github.com/KizEvo/hot-plate-project/assets/104358167/6462c83f-aafc-4c5a-9d65-52a9e7ade8a1)

  > - Result

![result_board](https://github.com/KizEvo/hot-plate-project/assets/104358167/a14e044f-eea8-42f3-8864-1cca10ef47a3)


<br />

<h2>Firmware</h2>

- Arduino Framework

<h2>Hardware</h2>

- Arduino Nano (Atmega328p)

- LCD 16x2 - HD47750 driver

- Solid State Relay - 40 DC

- PT100 - RTD Temperature Sensor 

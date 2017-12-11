# DIY Aquarium Controller

## Features
* Controller based on ATMEL microcontroller ATMEGA8A
   * All settings (except date/time) stored in EEPROM
* Remote controlling with Bluetooth
   * Based on HC-05 module
   * RFCOMM connection type
   * All possible settings is accessible
* Available [special app for controlling aquarium settings](https://github.com/baranovskiykonstantin/aquarium-control) from Linux or Android
* LED lighting
   * 12V DC output
   * 400mA max.
   * Adjustable brightness
   * Adjustable gradual turning on/off (0-30 min)
   * Adjustable on/off time
* Thermostat
   * 220V AC output
   * 3A max.
   * Adjustable min. and max. temperature
* Water temperature measurement
   * DS18B20 sensor
   * Accuracy 0.5 °C
   * Error detecting
* On-board RTC
   * Based on DS1302
   * Adjustable date/time
   * Adjustable daily time correction
* 4-digits LED display
   * Two modes of displaying: current time and temperature of the water
   * Modes is switches by capacitive sensor

## Wiring diagram
![wiring-diagram](https://raw.github.com/baranovskiykonstantin/aquarium/master/images/wiring-diagram.png)

1. Capacitive sensor (aluminium scotch 10x50mm)
2. Mains plug
3. PCB of aquarium controller
4. PCB of power supply
5. Bluetooth module HC-05
6. LEDs (4 x 1W = 12V 300mA)
7. Waterproof DS18B20
8. Heater

## HC-05 configuration
* Indicating mode changing
   * By default, LED in idle -- blinking, in connecting -- lights on
   * Disconnect LED2(31) output from PCB's pad and connect to this pad LED1(32) output
   * Now, LED in idle -- lights off, in connecting -- lights on
* Restore default settings
   * Enter to AT-command mode
   * Send command `AT+ORGL`
* Set device name
   * Enter to AT-command mode
   * Send command `AT+NAME=aquarium`
* Reduce power consumption (from ~40mA to ~3mA in idle!)
   * Enter to AT-command mode
   * Send command `AT+IPSCAN=1024,1,1024,1`

## Communicating with controller
After every command (except `status` command) the controller sends response `OK` if operation was success or `ERROR` on fail.
Commands must ends with `\n` or `\r\n`. If value of parameter is bigger than allowed will be used max possible value.

### Command `status`
Get information about current state of aquarium.

Format:

`status`

Answer:

`Date: 01.01.2017 Friday`<br>
`Time: 13:29:59 (-3 sec at 12:00:00)`<br>
`Temp: 22`<br>
`Heat: OFF auto (20-22)`<br>
`Light: ON manual (10:00:00-20:00:00) 50% 10min`<br>
`Display: time`

Meaning:

Line 1: current date and day of week;<br>
Line 2: current time and info about daily time correction;<br>
Line 3: temperature of the water (°C);<br>
Line 4: thermostat status:<br>
* `ON` - heater is on, `OFF` - heater is off
* `auto` - automatic mode, `manual` - manual mode
* value in the bracket indicates the temperature range supported in the automatic mode <br>

Line 5: lighting status:<br>
* `ON` - light is on, `OFF` - light is off
* `auto` - automatic mode, `manual` - manual mode
* value in the bracket indicates the period of time during which the light is switched on in automatic mode<br>
* at the end, the brightness level in percentage and light rising time in minutes

Line 6: display mode (`time` - current time shows, `temp` - temperature of the water shows)

### Command `date`
Set date.

Format:

`date DD.MM.YY W`

Parameters:<br>
* `DD` - day of month (01-31)
* `MM` - month (01-12)
* `YY` - year (00-99)
* `W` - day of week (1 - Monday ... 7 - Sunday)

Answer:

`OK` or `ERROR`

### Command `time`
Set time and/or time correction

Format 1:

`time HH:MM:SS`

Parameters:<br>
* `HH` - hours (00-23)
* `MM` - minutes (00-59)
* `SS` - seconds (00-59)

Format 2:

`time +CC`<br>
`time -CC`

Parameters:<br>
* `+` or `-` - increase or decrease time correction value
* `CC` - time correction in seconds (00-59)

Format 3:

`time HH:MM:SS +CC`<br>
`time HH:MM:SS -CC`

Parameters:<br>
* `HH` - hours (00-23)
* `MM` - minutes (00-59)
* `SS` - seconds (00-59)
* `+` or `-` - increase or decrease time correction value
* `CC` - time correction in seconds (00-59)

Answer:

`OK` or `ERROR`

### Command `heat`
Heater setup.

Format 1:

`heat LL-HH`

Parameters:<br>
* `LL` - minimal temperature (00-99)
* `HH` - maximal temperature (00-99)

Format 2:

`heat on`<br>
`heat off`<br>
`heat auto`<br>

Parameters:<br>
* `on` - switch to manual mode and turn on heater
* `off` - switch to manual mode and turn off heater
* `auto` - switch to automatic mode

Answer:

`OK` or `ERROR`

### Command `light`
Lighting setup.

Format 1:

`light H1:M1:S1-H2:M2:S2`

Parameters:<br>
* `H1:M1:S1` - time of turn on light (00:00:00-23:59:59)
* `H2:M2:S2` - time of turn off light (00:00:00-23:59:59)

Format 2:

`light on`<br>
`light off`<br>
`light auto`<br>

Parameters:<br>
* `on` - switch to manual mode and turn on light
* `off` - switch to manual mode and turn off light
* `auto` - switch to automatic mode

Format 3:

`light level XXX`

Parameters:<br>
* `XXX` - brightness level in percentage (000-100)

Format: 4:

`light rise YY`

Parameters:<br>
* `YY` - time of the light rising in minutes (00-30)

NOTE: Specified time actual for brightness 100%. This means, that for the
light rising time in 10 minutes with brightness 50% the real rise time will be
5 minutes.

Format: 5:

`light H1:M1:S1-H2:M2:S2 XXX YY`

Parameters:<br>
* `H1:M1:S1` - time of turn on light (00:00:00-23:59:59)
* `H2:M2:S2` - time of turn off light (00:00:00-23:59:59)
* `XXX` - brightness level (000-100)
* `YY` - light rising time (00-30)

Answer:

`OK` or `ERROR`

### Command `display`
Display setup.

Format:

`display time`<br>
`display temp`

Parameters:<br>
* `time` - to show current time
* `temp` - to show current temperature of the water

Answer:

`OK` or `ERROR`

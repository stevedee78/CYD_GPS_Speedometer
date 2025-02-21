# CYD_GPS_Speedometer
This project allows a ‘Cheap Yellow Display’ to be used as a GPS speedometer.

Hardware required:
```
1x ESP32-2432S028R v3 (ST7789 SPI, MODE 3)：A type with two USB ports 
1x ATGM336H-5N, GNSS Module support BDS (BeiDou Navigation Satellite System)
```
Description:

The display shows: satellites, date, time, direction of travel (north, south, etc.), speed, topspeed, avg, trip odometer and a button to reset the kilometres you have driven.


![PictureScreen](https://github.com/stevedee78/CYD_GPS_Speedometer/blob/main/Bilder/Version2.jpg)

Wiring:
```
  CYD2USB             -  GPS Modul

IO16 (RGB LED)        -    TX
IO17 (RGB LED)        -    RX
%V close to USB port  -    VCC
GND (SD Shield)       -    GND 

```

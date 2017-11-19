# airQualityBox
Air Quality Measurement sketch for Arduino Mega with Adafruit TFT touch screen and DSM501A dust sensor

## Building the Box
To build the box, I partially followed this instructions:
https://projetsdiy.fr/calculer-indice-qualite-air-iaq-ciqa-iqa-dsm501-arduino-esp8266/

The changes to that design I made where:
- I used an Arduino Mega instead of the ESP8266 used in that example
- I added a TFT screen from Adafruit and all the logic to present the data recovered using the proposed sketch

Once the dust sensor (http://www.samyoungsnc.com/products/3-1%20Specification%20DSM501.pdf) is connected to GND, 5v and digital inputs 30 and 40, after solder close the pins 11, 12 & 13 of your TFT screen as explained here https://learn.adafruit.com/adafruit-2-8-tft-touch-shield-v2/connecting (to allow it working with Arduino Mega) and plugged it into the arduino Mega, then you just need to power it with a powerbank and place the dust sensor in vertical position and let it work. It's important to keep the sensor out from any artificial air current (like a fan). The dust sensor will heat inside to create a small air current to make the dust go through the optical sensor. The sketch will keep a record of the last measurements, splitting the results with the following distribution:

- Upper side of the screen - Average
Average for the last X measurements. Colors will vary depending on the average AQI obtained

- Lower side of the screen - Last instant measure
  - Left small window representing the values for PM25 (concentration and AQI) and background color according to the AQI

  - Right small windows representing the values for PM10 (concentration and AQI) and background color according to the AQI
  
  - Global AQI measurement. For the rest of the lower part of the screen, the color will be the global AQI (this is, the worse value between PM25 and PM10), and a message accordingly (excellent, bon, moyen, mauvais, dangereux)

If the screen is touched, it toggles between screen on and screen off, to save battery. 

Enjoy, measure and improve this sketch! and please, if you do so, let me know!

## Roadmap
- Screen reacts to touch events switching on an off the screen by manually polling if the screen is touched. I would need to change it into the use of a touch interrupt to remove unnecessary pollings. 
- Assess and calibrate the measurements to be sure they're accurate.
- Use free communication digital pins to introduce a radio (Lora, bluetooh, WiFi...whatever) to send the results to an external storage system

## Well known issues
- Currently the main issue is that I'm not sure about the accurate of the results. I've blindly followed the algorithm used in this link (https://projetsdiy.fr/calculer-indice-qualite-air-iaq-ciqa-iqa-dsm501-arduino-esp8266/) and looks reasonable by comparing with the sensor specifications (http://www.samyoungsnc.com/products/3-1%20Specification%20DSM501.pdf) but still need some time to properly assess it. The reason why I think it's not accurate is that modifying the time between measurements from 30 seconds to 1h makes the measurement increase by an order of magnitude...and that shouldn't vary so much as the algorithm should be taking into account also the measurement time and normalize the output.

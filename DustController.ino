/* Connect the DSM501 sensor as follow 
 * https://www.elektronik.ropla.eu/pdf/stock/smy/dsm501.pdf
 * 1 green vert - Not used
 * 2 yellow jaune - Vout2 - 1 microns (PM1.0)
 * 3 white blanc - Vcc
 * 4 red rouge - Vout1 - 2.5 microns (PM2.5)
 * 5 black noir - GND
*/
void dustSensorInitialConfig(){
  pinMode(DUST_SENSOR_DIGITAL_PIN_PM10,INPUT);
  pinMode(DUST_SENSOR_DIGITAL_PIN_PM25,INPUT);
}

void warmUp(){
  // wait for DSM501 to warm up (typically 60s)
  for (int i = 1; i <= SENSOR_WARMMUP_TIME; i++)
  {
    delay(1000); // 1s
    //Serial
    Serial.print(i);
    Serial.println(" s (wait 60s for DSM501 to warm up)");

    //TFT
    if (i == MAX_LINES) {
      tft.fillScreen(ILI9341_BLACK);
      tft.setCursor(0, 0);
    }
    tft.print(i);
    tft.println(" s (wait 60s for DSM501 to warm up)");
  }
}

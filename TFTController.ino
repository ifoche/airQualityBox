

void tftInitialConfig(){
  // diagnosis
  tft.begin();
  diagnosis();

  // prepare for writing the first text
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);  
  tft.setTextSize(1);
  w = tft.width();
  h = tft.height();

  // control backlight with TS_BACKLIGHT pin
  pinMode(TS_BACKLIGHT, OUTPUT);
  screenOn();
}

void diagnosis(){
// read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 
}

void screenOn(){
  screenBacklight = true;
  digitalWrite(TS_BACKLIGHT, HIGH);
}

void screenOff(){
  screenBacklight = false;
  digitalWrite(TS_BACKLIGHT, LOW);
}

void toggleScreen(){
  (screenBacklight) ? nextScreen() : screenOn();
  if (screenBacklight) drawDisplay();
}

void nextScreen(){
  currentScreen++;
  if(currentScreen == MAX_SCREENS) {
    currentScreen = 0;
    screenOff();
  } 
  
  if (!screenBacklight) Serial.println("Screen is off");
  Serial.print("Toggl screen. Switching to screen number: ");
  Serial.println(currentScreen);
}

void tftDrawBackground() {
  x1 = 0;
  x2 = w;
  y1 = y2 = h/2;
  tft.fillScreen(BACKGROUND_COLOR);
  
  for (int i=0; i<CENTRAL_LINEWIDTH; i++){
    tft.drawLine(x1, y1-CENTRAL_LINEWIDTH/2+i, x2, y2-CENTRAL_LINEWIDTH/2+i, FOREGROUND_COLOR);
  }

  /* Upper side */
  tftUpperBackground();

  /* Down side */
  tftLowerBackground();
}

void tftUpperBackground() {
  tftUpperBackground(UPPER_BG_COLOR);
}

void tftUpperBackground(int color) {
  tft.fillRect(0, 0, w, h/2-CENTRAL_LINEWIDTH/2, color);
}

void tftLowerBackground() {
  tftLowerBackground(LOWER_BG_COLOR);
}

void tftLowerBackground(int color) {
  tft.fillRect(0, h/2+CENTRAL_LINEWIDTH/2, w, h/2-CENTRAL_LINEWIDTH/2, color);
}

void tftLowerUpLeftCorner(int color) {
  tft.fillRoundRect(CENTRAL_LINEWIDTH, h/2+CENTRAL_LINEWIDTH/2+CENTRAL_LINEWIDTH, w/3, h/3, CENTRAL_LINEWIDTH, color);
  tft.drawRoundRect(CENTRAL_LINEWIDTH, h/2+CENTRAL_LINEWIDTH/2+CENTRAL_LINEWIDTH, w/3, h/3, CENTRAL_LINEWIDTH, FOREGROUND_COLOR);
}

void tftLowerUpRightCorner(int color){
  tft.fillRoundRect(w-w/3-CENTRAL_LINEWIDTH, h/2+CENTRAL_LINEWIDTH/2+CENTRAL_LINEWIDTH, w/3, h/3, CENTRAL_LINEWIDTH, color);
  tft.drawRoundRect(w-w/3-CENTRAL_LINEWIDTH, h/2+CENTRAL_LINEWIDTH/2+CENTRAL_LINEWIDTH, w/3, h/3, CENTRAL_LINEWIDTH, FOREGROUND_COLOR);
}

void tftLowerMessage(String message) {
  tft.setCursor(w/7, h/2+h/3+CENTRAL_LINEWIDTH*5);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK);
  tft.println(message);
}

void tftLowerUpLeftTitle(String title) {
  tft.setCursor(CENTRAL_LINEWIDTH*2, h/2+CENTRAL_LINEWIDTH/2+CENTRAL_LINEWIDTH*2);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK);
  tft.println(title);
}

void tftLowerUpRightTitle(String title) {
  tft.setCursor(w-w/3, h/2+CENTRAL_LINEWIDTH/2+CENTRAL_LINEWIDTH*2);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK);
  tft.println(title);
}

void tftLowerUpLeftMessageCursor(int line){
  tft.setCursor(CENTRAL_LINEWIDTH*2, h/2+CENTRAL_LINEWIDTH/2+CENTRAL_LINEWIDTH*6+CENTRAL_LINEWIDTH*2*line);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_BLACK);
}

void tftLowerUpRightMessageCursor(int line){
  tft.setCursor(w-w/3, h/2+CENTRAL_LINEWIDTH/2+CENTRAL_LINEWIDTH*6+CENTRAL_LINEWIDTH*2*line);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_BLACK);
}

void tftUpperMessageCursor(int line, int column, int size) {
  tft.setCursor(w/16*column, CENTRAL_LINEWIDTH*4*line);
  tft.setTextSize(size);
  tft.setTextColor(ILI9341_BLACK);
}

void drawDisplay(){
  switch(currentScreen){
    case SCREEN_MEASUREMENTS:
      drawMeasurementsScreen();
      break;
    case SCREEN_CHART:
      drawChartScreen();
      break;
  }
}

void drawMeasurementsScreen(){
  // draw global AQI level
  drawAQILevel(false, "GLOBAL", AQI.AQI);
  // draw PM25 AQI level
  drawAQILevel(false, "PM25", AQI.AqiPM25);
  // draw PM10 AQI level
  drawAQILevel(false, "PM10", AQI.AqiPM10);
  // draw last BUFFER_SIZE average global AQI level
  drawAQILevel(true, "AVERAGE", AQIAvg);

  // write global AQI message
  writeAQILevelMessage(AQI.AqiString + " ("  + AQI.AQI + ")");

  // write titles
  tftLowerUpLeftTitle("PM25:");
  tftLowerUpRightTitle("PM10:");

  // write PM25 values
  writePM25Values();

  // write PM10 values
  writePM10Values();

  // write average AQI message
  writeAvgAQIMessage();
}

void writePM25Values(){
  tftLowerUpLeftMessageCursor(0);
  tft.print("Conc: ");
  tft.println(AQI.concentrationPM25);
  tftLowerUpLeftMessageCursor(1);
  tft.print("AQI: ");
  tft.println(AQI.AqiPM25);
  tftLowerUpLeftMessageCursor(2);
  tft.print("Max: ");
  tft.println(maxPM25);
  tftLowerUpLeftMessageCursor(3);
  tft.print("Min: ");
  tft.println(minPM25);
}

void writePM10Values(){
  tftLowerUpRightMessageCursor(0);
  tft.print("Conc: ");
  tft.println(AQI.concentrationPM10);
  tftLowerUpRightMessageCursor(1);
  tft.print("AQI: ");
  tft.println(AQI.AqiPM10);
  tftLowerUpRightMessageCursor(2);
  tft.print("Max: ");
  tft.println(maxPM10);
  tftLowerUpRightMessageCursor(3);
  tft.print("Min: ");
  tft.println(minPM10);
}

void writeAQILevelMessage(String message){
  tftLowerMessage(message);
}

void writeAvgAQIMessage(){
  tftUpperMessageCursor(1, 4, 1);
  tft.print("Avg for the last "); 
  tft.println((measurementNumber<BUFFER_SIZE) ? measurementNumber+1 : BUFFER_SIZE);
  tftUpperMessageCursor(2, 4, 1);
  tft.print("AQI measurements: "); 
  tftUpperMessageCursor(3, 7, 2);
  tft.println(AQIAvg);
  Serial.print("Avg for the last: "); Serial.print(measurementNumber); Serial.print(" AQI measurements: "); Serial.println(AQIAvg);
}

void drawAQILevel(boolean average, String partSize, int level){
  switch(level){
    case 1:
    drawBackground(average, partSize, EXCELLENT_COLOR);
  break;
    case 2:
    drawBackground(average, partSize, EXCELLENT_COLOR);
  break;
    case 3:
    drawBackground(average, partSize, GOOD_COLOR);
  break;
    case 4:
    drawBackground(average, partSize, GOOD_COLOR);
  break;
    case 5:
    drawBackground(average, partSize, ACCEPTABLE_COLOR);
  break;
    case 6:
    drawBackground(average, partSize, MODERATE_COLOR);
  break;
    case 7:
    drawBackground(average, partSize, MODERATE_COLOR);
  break;
    case 8:
    drawBackground(average, partSize, HEAVY_COLOR);
  break;
    case 9:
    drawBackground(average, partSize, HEAVY_COLOR);
  break;
    case 10:
    drawBackground(average, partSize, HAZARDOUS_COLOR);
  break;
  }  
}

void drawBackground(boolean average, String partSize, int color){
  if (!average){
    if (partSize == "GLOBAL") {
      tftLowerBackground(color);
    } else if (partSize == "PM25") {
      tftLowerUpLeftCorner(color);
    } else if (partSize == "PM10") {
      tftLowerUpRightCorner(color);
    }
  } else{
    tftUpperBackground(color);  
  }
}

// CHART functions
void drawChartScreen(){
  drawChartBackground();
  drawChartAxis();
}

void drawChartBackground(){
  tft.fillScreen(ILI9341_LIGHTGREY);
}

void drawChartAxis(){
  drawChartXAxis();

  drawChartYAxis();
}

void drawChartXAxis(){
  x1 = CENTRAL_LINEWIDTH;
  x2 = w-CENTRAL_LINEWIDTH;
  y1 = y2 = h-CENTRAL_LINEWIDTH;
  tft.drawLine(x1, y1, x2, y2, FOREGROUND_COLOR);
}

void drawChartYAxis(){
  x1 = x2= CENTRAL_LINEWIDTH;
  y1 = CENTRAL_LINEWIDTH;
  y2 = h-CENTRAL_LINEWIDTH;
  tft.drawLine(x1, y1, x2, y2, FOREGROUND_COLOR);  
}


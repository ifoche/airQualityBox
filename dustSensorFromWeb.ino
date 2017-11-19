#include "PinChangeInt.h"
#include "SimpleTimer.h"
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Adafruit_STMPE610.h"

/* Connect the DSM501 sensor as follow 
 * https://www.elektronik.ropla.eu/pdf/stock/smy/dsm501.pdf
 * 1 green vert - Not used
 * 2 yellow jaune - Vout2 - 1 microns (PM1.0)
 * 3 white blanc - Vcc
 * 4 red rouge - Vout1 - 2.5 microns (PM2.5)
 * 5 black noir - GND
*/
#define SENSOR_WARMMUP_TIME           1

#define DUST_SENSOR_DIGITAL_PIN_PM10  30        // DSM501 Pin 2 of DSM501 (jaune / Yellow)
#define DUST_SENSOR_DIGITAL_PIN_PM25  40        // DSM501 Pin 4 (rouge / red) 

#define COUNTRY                       0         // 0. France, 1. Europe, 2. USA/China
#define FRANCE                        0
#define EUROPE                        1
#define USA_CHINA                     2

#define EXCELLENT                     "Excellent"
#define GOOD                          "Bon"
#define ACCEPTABLE                    "Moyen"
#define MODERATE                      "Mediocre"
#define HEAVY                         "Mauvais"
#define SEVERE                        "Tres mauvais"
#define HAZARDOUS                     "Dangereux"

#define PM25_SENSOR                   0
#define PM10_SENSOR                   1

/* Screen TFT from Adafruit */
// For the Adafruit shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10
#define TS_INTERRUPTION 7
#define TS_BACKLIGHT 3

// The STMPE610 uses hardware SPI on the shield, and #8
#define STMPE_CS 8

#define MAX_LINES                     40
#define CENTRAL_LINEWIDTH             6
#define BACKGROUND_COLOR              ILI9341_WHITE
#define FOREGROUND_COLOR              ILI9341_DARKGREY
#define UPPER_BG_COLOR                ILI9341_NAVY
#define LOWER_BG_COLOR                ILI9341_LIGHTGREY

#define EXCELLENT_COLOR               ILI9341_DARKGREEN
#define GOOD_COLOR                    ILI9341_GREEN
#define ACCEPTABLE_COLOR              ILI9341_GREENYELLOW
#define MODERATE_COLOR                ILI9341_YELLOW
#define HEAVY_COLOR                   ILI9341_ORANGE
#define SEVERE_COLOR                  ILI9341_RED
#define HAZARDOUS_COLOR               ILI9341_RED

// With 30s measurements, 120 gives 1h buffer
#define BUFFER_SIZE                   60

// Needed for interrupts
#define NO_PORTB_PINCHANGES // to indicate that port b will not be used for pin change interrupts
#define NO_PORTJ_PINCHANGES // to indicate that port c will not be used for pin change interrupts
#define NO_PORTK_PINCHANGES // to indicate that port d will not be used for pin change interrupts

unsigned long duration;
unsigned long starttime;
unsigned long endtime;
unsigned long lowpulseoccupancy = 0;
float         ratio = 0;
unsigned long SLEEP_TIME    = 2 * 1000;       // Sleep time between reads (in milliseconds)
unsigned long sampletime_ms = 1L * 30L * 1000L;  // Durée de mesure - sample time (ms)

struct structAQI{
  // variable enregistreur - recorder variables
  unsigned long durationPM10;
  unsigned long lowpulseoccupancyPM10 = 0;
  unsigned long durationPM25;
  unsigned long lowpulseoccupancyPM25 = 0;
  unsigned long starttime;
  unsigned long endtime;
  // Sensor AQI data
  float         concentrationPM25 = 0;
  float         concentrationPM10  = 0;
  int           AqiPM10            = -1;
  int           AqiPM25            = -1;
  // Indicateurs AQI - AQI display
  int           AQI                = 0;
  String        AqiString          = "";
  int           AqiColor           = 0;
};
struct structAQI AQI;
struct structAQI lastAQIs[BUFFER_SIZE];
unsigned long measurementNumber = 0;
int AQIAvg = 0;


SimpleTimer timer;

Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
int           x1, y1, x2, y2, w, h;
unsigned long   screenLine = 0;
boolean screenBacklight = true;

void updateAQILevel(){
  AQI.AQI = (AQI.AqiPM10 > AQI.AqiPM25) ? AQI.AqiPM10 : AQI.AqiPM25;
}

void updateAQI() {
  // Actualise les mesures - update measurements
  
  AQI.endtime = millis();
  
  float ratio = (AQI.lowpulseoccupancyPM10) / (sampletime_ms * 10.0);
  float concentration = 1.1 * pow( ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62; // using spec sheet curve
  if ( sampletime_ms < 3600000 ) { concentration = concentration * ( sampletime_ms / 3600000.0 ); }
  AQI.lowpulseoccupancyPM10 = 0;
  AQI.concentrationPM10 = concentration;
  
  ratio = (AQI.lowpulseoccupancyPM25) / (sampletime_ms * 10.0);
  concentration = 1.1 * pow( ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62;
  if ( sampletime_ms < 3600000 ) { concentration = concentration * ( sampletime_ms / 3600000.0 ); }
  AQI.lowpulseoccupancyPM25 = 0;
  AQI.concentrationPM25 = concentration;

  Serial.print("Concentrations => PM2.5: "); Serial.print(AQI.concentrationPM25); Serial.print(" | PM10: "); Serial.println(AQI.concentrationPM10);
  
  AQI.starttime = millis();
      
  // Actualise l'AQI de chaque capteur - update AQI for each sensor 
  if ( COUNTRY == FRANCE ) {
    // France
    AQI.AqiPM25 = getATMO( PM25_SENSOR, AQI.concentrationPM25 );
    AQI.AqiPM10 = getATMO( PM10_SENSOR, AQI.concentrationPM10 );
  } else if ( COUNTRY == EUROPE ) {
    // Europe
    AQI.AqiPM25 = getACQI( PM25_SENSOR, AQI.concentrationPM25 );
    AQI.AqiPM10 = getACQI( PM10_SENSOR, AQI.concentrationPM10 );
  } else {
    // USA / China
    AQI.AqiPM25 = getAQI( PM25_SENSOR, AQI.concentrationPM25 );
    AQI.AqiPM10 = getAQI( PM10_SENSOR, AQI.concentrationPM10 );
  }

  // Actualise l'indice AQI - update AQI index
  updateAQILevel();

  lastAQIs[measurementNumber%BUFFER_SIZE] = AQI;
  int elements = (measurementNumber>BUFFER_SIZE) ? BUFFER_SIZE : measurementNumber;
  int average = 0;
  for (int i=0; i<elements; i++){
      average+=lastAQIs[i].AQI;
  }
  if (elements != 0) AQIAvg = average/elements;
  else AQIAvg = AQI.AQI;
  
  updateAQIDisplay();
  
  Serial.print("AQIs => PM25: "); Serial.print(AQI.AqiPM25); Serial.print(" | PM10: "); Serial.println(AQI.AqiPM10);
  Serial.print(" | AQI: "); Serial.println(AQI.AQI); Serial.print(" | Message: "); Serial.println(AQI.AqiString);

  //blink(AQI.AQI);
  measurementNumber++;
}

void blink(){
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(200);
}

void blink(int times) {
  for (int i=0; i<times; i++){
    blink();  
  }
}

void setup() {
  Serial.begin(9600);
  dustSensorInitialConfig();
  ledInitialConfig();
  tftInitialConfig();

  // warm up for the dust sensor (1 minute)
  warmUp();
  
  Serial.println("Ready!");
  tft.println("Ready!");

  tftDrawBackground();
  
  AQI.starttime = millis();
  timer.setInterval(sampletime_ms, updateAQI);

  //pinMode(TS_INTERRUPTION, INPUT_PULLUP);
  //attachInterrupt(TS_INTERRUPTION, toggleScreen, RISING);
  //PCintPort::attachInterrupt(TS_INTERRUPTION, toggleScreen, CHANGE); // attach a PinChange Interrupt to our pin on the rising edge
  // (RISING, FALLING and CHANGE all work with this library)
  if (!ts.begin()) Serial.println("Failure on touch screen initialization");
}

void loop() {
  AQI.lowpulseoccupancyPM10 += pulseIn(DUST_SENSOR_DIGITAL_PIN_PM10, LOW);
  AQI.lowpulseoccupancyPM25 += pulseIn(DUST_SENSOR_DIGITAL_PIN_PM25, LOW);

  // polling methodfor touch screen
  if (ts.touched()) toggleScreen();
  
  timer.run(); 
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

void dustSensorInitialConfig(){
  pinMode(DUST_SENSOR_DIGITAL_PIN_PM10,INPUT);
  pinMode(DUST_SENSOR_DIGITAL_PIN_PM25,INPUT);
}

void ledInitialConfig(){
  pinMode(LED_BUILTIN, OUTPUT);  
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

/*
 * Calcul l'indice de qualité de l'air français ATMO
 * Calculate French ATMO AQI indicator
 */
int getATMO( int sensor, float density ){
  if ( sensor == PM25_SENSOR ) { //PM2,5
    if ( density <= 11 ) {
      return 1; 
    } else if ( density > 11 && density <= 24 ) {
      return 2;
    } else if ( density > 24 && density <= 36 ) {
      return 3;
    } else if ( density > 36 && density <= 41 ) {
      return 4;
    } else if ( density > 41 && density <= 47 ) {
      return 5;
    } else if ( density > 47 && density <= 53 ) {
      return 6;
    } else if ( density > 53 && density <= 58 ) {
      return 7;
    } else if ( density > 58 && density <= 64 ) {
      return 8;
    } else if ( density > 64 && density <= 69 ) {
      return 9;
    } else {
      return 10;
    }
  } else { // sensor == PM10_SENSOR 
    if ( density <= 6 ) {
      return 1; 
    } else if ( density > 6 && density <= 13 ) {
      return 2;
    } else if ( density > 13 && density <= 20 ) {
      return 3;
    } else if ( density > 20 && density <= 27 ) {
      return 4;
    } else if ( density > 27 && density <= 34 ) {
      return 5;
    } else if ( density > 34 && density <= 41 ) {
      return 6;
    } else if ( density > 41 && density <= 49 ) {
      return 7;
    } else if ( density > 49 && density <= 64 ) {
      return 8;
    } else if ( density > 64 && density <= 79 ) {
      return 9;
    } else {
      return 10;
    }  
  }
}

void updateAQIDisplay(){
  /*
   * 1 EXCELLENT                    
   * 2 GOOD                         
   * 3 ACCEPTABLE               
   * 4 MODERATE            
   * 5 HEAVY               
   * 6 SEVERE
   * 7 HAZARDOUS
   */
  if ( COUNTRY == 0 ) {
    // Système ATMO français - French ATMO AQI system 
    switch ( AQI.AQI) {
      case 10: 
        AQI.AqiString = SEVERE;
        AQI.AqiColor = SEVERE_COLOR;
        break;
      case 9:
        AQI.AqiString = HEAVY;
        AQI.AqiColor = HEAVY_COLOR;
        break;
      case 8:
        AQI.AqiString = HEAVY;
        AQI.AqiColor = HEAVY_COLOR;
        break;  
      case 7:
        AQI.AqiString = MODERATE;
        AQI.AqiColor = MODERATE_COLOR;
        break;
      case 6:
        AQI.AqiString = MODERATE;
        AQI.AqiColor = MODERATE_COLOR;
        break;   
      case 5:
        AQI.AqiString = ACCEPTABLE;
        AQI.AqiColor = ACCEPTABLE_COLOR;
        break;
      case 4:
        AQI.AqiString = GOOD;
        AQI.AqiColor = GOOD_COLOR;
        break;
      case 3:
        AQI.AqiString = GOOD;
        AQI.AqiColor = GOOD_COLOR;
        break;
      case 2:
        AQI.AqiString = EXCELLENT;
        AQI.AqiColor = EXCELLENT_COLOR;
        break;
      case 1:
        AQI.AqiString = EXCELLENT;
        AQI.AqiColor = EXCELLENT_COLOR;
        break;           
      }
  } else if ( COUNTRY == 1 ) {
    // European CAQI
    switch ( AQI.AQI) {
      case 25: 
        AQI.AqiString = GOOD;
        AQI.AqiColor = GOOD_COLOR;
        break;
      case 50:
        AQI.AqiString = ACCEPTABLE;
        AQI.AqiColor = ACCEPTABLE_COLOR;
        break;
      case 75:
        AQI.AqiString = MODERATE;
        AQI.AqiColor = MODERATE_COLOR;
        break;
      case 100:
        AQI.AqiString = HEAVY;
        AQI.AqiColor = HEAVY_COLOR;
        break;         
      default:
        AQI.AqiString = SEVERE;
        AQI.AqiColor = SEVERE_COLOR;
      }  
  } else if ( COUNTRY == 2 ) {
    // USA / CN
    if ( AQI.AQI <= 50 ) {
        AQI.AqiString = GOOD;
        AQI.AqiColor = GOOD_COLOR;
    } else if ( AQI.AQI > 50 && AQI.AQI <= 100 ) {
        AQI.AqiString = ACCEPTABLE;
        AQI.AqiColor = ACCEPTABLE_COLOR;
    } else if ( AQI.AQI > 100 && AQI.AQI <= 150 ) {
        AQI.AqiString = MODERATE;
        AQI.AqiColor = MODERATE_COLOR;
    } else if ( AQI.AQI > 150 && AQI.AQI <= 200 ) {
        AQI.AqiString = HEAVY;
        AQI.AqiColor = HEAVY_COLOR;
    } else if ( AQI.AQI > 200 && AQI.AQI <= 300 ) {  
        AQI.AqiString = SEVERE;
        AQI.AqiColor = SEVERE_COLOR;
    } else {    
       AQI.AqiString = HAZARDOUS;
       AQI.AqiColor = HAZARDOUS_COLOR;
    }  
  }

  drawDisplay();
}
/*
 * CAQI Européen - European CAQI level 
 * source : http://www.airqualitynow.eu/about_indices_definition.php
 */
 
int getACQI( int sensor, float density ){  
  if ( sensor == 0 ) {  //PM2,5
    if ( density == 0 ) {
      return 0; 
    } else if ( density <= 15 ) {
      return 25 ;
    } else if ( density > 15 && density <= 30 ) {
      return 50;
    } else if ( density > 30 && density <= 55 ) {
      return 75;
    } else if ( density > 55 && density <= 110 ) {
      return 100;
    } else {
      return 150;
    }
  } else {              //PM10
    if ( density == 0 ) {
      return 0; 
    } else if ( density <= 25 ) {
      return 25 ;
    } else if ( density > 25 && density <= 50 ) {
      return 50;
    } else if ( density > 50 && density <= 90 ) {
      return 75;
    } else if ( density > 90 && density <= 180 ) {
      return 100;
    } else {
      return 150;
    }
  }
}

/*
 * AQI formula: https://en.wikipedia.org/wiki/Air_Quality_Index#United_States
 * Arduino code https://gist.github.com/nfjinjing/8d63012c18feea3ed04e
 * On line AQI calculator https://www.airnow.gov/index.cfm?action=resources.conc_aqi_calc
 */
float calcAQI(float I_high, float I_low, float C_high, float C_low, float C) {
  return (I_high - I_low) * (C - C_low) / (C_high - C_low) + I_low;
}

int getAQI(int sensor, float density) {
  int d10 = (int)(density * 10);
  if ( sensor == 0 ) {
    if (d10 <= 0) {
      return 0;
    }
    else if(d10 <= 120) {
      return calcAQI(50, 0, 120, 0, d10);
    }
    else if (d10 <= 354) {
      return calcAQI(100, 51, 354, 121, d10);
    }
    else if (d10 <= 554) {
      return calcAQI(150, 101, 554, 355, d10);
    }
    else if (d10 <= 1504) {
      return calcAQI(200, 151, 1504, 555, d10);
    }
    else if (d10 <= 2504) {
      return calcAQI(300, 201, 2504, 1505, d10);
    }
    else if (d10 <= 3504) {
      return calcAQI(400, 301, 3504, 2505, d10);
    }
    else if (d10 <= 5004) {
      return calcAQI(500, 401, 5004, 3505, d10);
    }
    else if (d10 <= 10000) {
      return calcAQI(1000, 501, 10000, 5005, d10);
    }
    else {
      return 1001;
    }
  } else {
    if (d10 <= 0) {
      return 0;
    }
    else if(d10 <= 540) {
      return calcAQI(50, 0, 540, 0, d10);
    }
    else if (d10 <= 1540) {
      return calcAQI(100, 51, 1540, 541, d10);
    }
    else if (d10 <= 2540) {
      return calcAQI(150, 101, 2540, 1541, d10);
    }
    else if (d10 <= 3550) {
      return calcAQI(200, 151, 3550, 2541, d10);
    }
    else if (d10 <= 4250) {
      return calcAQI(300, 201, 4250, 3551, d10);
    }
    else if (d10 <= 5050) {
      return calcAQI(400, 301, 5050, 4251, d10);
    }
    else if (d10 <= 6050) {
      return calcAQI(500, 401, 6050, 5051, d10);
    }
    else {
      return 1001;
    }
  }   
}

void drawDisplay(){
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
}

void writePM10Values(){
  tftLowerUpRightMessageCursor(0);
  tft.print("Conc: ");
  tft.println(AQI.concentrationPM10);
  tftLowerUpRightMessageCursor(1);
  tft.print("AQI: ");
  tft.println(AQI.AqiPM10);
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

void screenOn(){
  screenBacklight = true;
  digitalWrite(TS_BACKLIGHT, HIGH);
}

void screenOff(){
  screenBacklight = false;
  digitalWrite(TS_BACKLIGHT, LOW);
}

void toggleScreen(){
  (screenBacklight) ? screenOff() : screenOn();
  Serial.print("Toggl screen. Screen is now: ");
  Serial.println(screenBacklight);
}

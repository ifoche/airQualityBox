#include "PinChangeInt.h"
#include "SimpleTimer.h"
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Adafruit_STMPE610.h"
#include "airQualityBox.h"


unsigned long duration;
unsigned long starttime;
unsigned long endtime;
unsigned long lowpulseoccupancy = 0;
float         ratio = 0;
unsigned long sampletime_ms = SAMPLE_TIME;  // Durée de mesure - sample time (ms)

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
int maxPM25 = 0;
int maxPM10 = 0;
int minPM25 = 10;
int minPM10 = 10;

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

  updateStatistics();
  
  updateAQIDisplay();
  
  printSerialAQI();

  //blink(AQI.AQI);
  
  measurementNumber++;
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

void updateStatistics(){
  lastAQIs[measurementNumber%BUFFER_SIZE] = AQI;
  int elements = (measurementNumber>BUFFER_SIZE) ? BUFFER_SIZE : measurementNumber;
  int average = 0;
  for (int i=0; i<elements; i++){
      average+=lastAQIs[i].AQI;
  }
  if (elements != 0) AQIAvg = average/elements;
  else AQIAvg = AQI.AQI;
  if (AQI.AqiPM25 < minPM25) minPM25 = AQI.AqiPM25;
  if (AQI.AqiPM25 > maxPM25) maxPM25 = AQI.AqiPM25;
  if (AQI.AqiPM10 < minPM10) minPM10 = AQI.AqiPM10;
  if (AQI.AqiPM10 > maxPM10) maxPM10 = AQI.AqiPM10;
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

void printSerialAQI(){
  Serial.print("AQIs => PM25: "); Serial.print(AQI.AqiPM25); Serial.print(" | PM10: "); Serial.println(AQI.AqiPM10);
  Serial.print(" | AQI: "); Serial.println(AQI.AQI); Serial.print(" | Message: "); Serial.println(AQI.AqiString);
}




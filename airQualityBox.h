#define SENSOR_WARMMUP_TIME           60
#define SAMPLE_TIME                   15L * 60L * 1000L
#define BUFFER_SIZE                   60

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
#define MAX_SCREENS                   2
#define SCREEN_MEASUREMENTS           0
#define SCREEN_CHART                  1
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

// Needed for interrupts
#define NO_PORTB_PINCHANGES // to indicate that port b will not be used for pin change interrupts
#define NO_PORTJ_PINCHANGES // to indicate that port c will not be used for pin change interrupts
#define NO_PORTK_PINCHANGES // to indicate that port d will not be used for pin change interrupts

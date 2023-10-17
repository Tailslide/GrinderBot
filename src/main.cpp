/**************************************************************************
 Arduino Scale
 **************************************************************************/
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Chrono.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/DejaVu_Sans_Mono_14.h>
#include <HX711_ADC.h>
#include "loadcell.h"

//HX711 pins:

//const int HX711_dout = 11; //mcu > HX711 dout pin
//const int HX711_sck = 12; //mcu > HX711 sck pin
const int HX711_dout = 6; //mcu > HX711 dout pin
const int HX711_sck = 7; //mcu > HX711 sck pin
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
unsigned long t = 0;

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
//#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
//#define SCREEN_ADDRESS 0x6B ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Chrono timer(Chrono::SECONDS);

void UpdateStatus(Chrono fortimer, float weight)
{
  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setFont(&FreeMono9pt7b);
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 12);     // Start at top-left corner
  char weightStr[64];
  sprintf(weightStr, "Wt. %.1f", weight);

  char timeStr[16];  // Buffer to hold the time string
  int seconds = millis() / 1000;  // Get the elapsed seconds since the Arduino started

  int minutes = seconds / 60;
  int remainingSeconds = seconds % 60;

  sprintf(timeStr, "Time %02d:%02d", minutes, remainingSeconds);  // Format the time string

  //display.println(F("Wt.  01.23g"));
  display.println(weightStr);
  display.print(timeStr);
  //display.print(F("Time 00:00"));
}

void SetupDisplay()
{
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  //display.display();
  //delay(2000);
  timer.restart();
  UpdateStatus(timer,0.0f);
  display.display();
}

void setup() {
  Serial.begin(9600);


  //Serial.begin(57600); 
  delay(10);
  Serial.println();
  Serial.println("Starting...");

  SetupLoadCell(LoadCell);
  SetupDisplay();

}

void loop() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 200; //increase value to slow down serial print activity
  
  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData();
      //Serial.print("Load_cell output val: ");
      //Serial.println(i);
      newDataReady = 0;
      t = millis();
      UpdateStatus(timer,i);
      display.display();
   }
  }

  // receive command from serial terminal
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay(); //tare
    else if (inByte == 'r') calibrate(LoadCell); //calibrate
    else if (inByte == 'c') changeSavedCalFactor(LoadCell); //edit calibration value manually
  }

  // check if last tare operation is complete
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }
  //if (timer.hasPassed(1))
  //{
//  UpdateStatus(timer);
  //display.display();
//  }
  //delay(10);
}

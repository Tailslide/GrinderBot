/**************************************************************************
 Arduino Scale
 **************************************************************************/
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Chrono.h>
#include "loadcell.h"
#include "display.h"

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Chrono timer(Chrono::SECONDS);

void setup() {
  Serial.begin(9600);
  //Serial.begin(57600); 
  delay(10);
  Serial.println();
  Serial.println("Starting...");

  SetupLoadCell(LoadCell);
  SetupDisplay(display);
  timer.restart();
  DisplayWeight(display,timer,0.0f);
}

unsigned long t = 0;
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
      DisplayWeight(display, timer,i);
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

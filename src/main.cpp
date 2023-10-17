/**************************************************************************
 Arduino Scale
 **************************************************************************/
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Chrono.h>
#include "loadcell.h"
#include "display.h"
#include "FlashStore.h"

HX711_ADC LoadCell(HX711_dout, HX711_sck);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Chrono timer(Chrono::SECONDS);

const int button1Pin = 0;  // Pin number to read from
const int button2Pin = 0;  // Pin number to read from
const int button3Pin = 0;  // Pin number to read from
const int button4Pin = 0;  // Pin number to read from

// Define a function to handle displaying strings to the Serial Monitor
void ScreenDisplay(const char* message) {
  DisplayMessage(display,message);
}

void setup() {
  Serial.begin(9600);  //Serial.begin(57600); 
  delay(10);
  Serial.println();
  Serial.println("Starting...");
  pinMode(button1Pin, INPUT);  // Set the pin as INPUT
  pinMode(button2Pin, INPUT);  // Set the pin as INPUT
  pinMode(button3Pin, INPUT);  // Set the pin as INPUT
  pinMode(button4Pin, INPUT);  // Set the pin as INPUT
  SetupDisplay(display);
  String message;
  SetupLoadCell(LoadCell, message);
  if (message != "") DisplayMessage(display,message,3000);
  timer.restart();
  //if (! settings.valid) calibrate(LoadCell, ScreenDisplay);
  DisplayWeight(display,timer,0.0f);
}

unsigned long t = 0;
void loop() {

    int pinValue = digitalRead(button1Pin);  // Read the value of the pin (HIGH or LOW)
  
  if(pinValue == HIGH) {
    Serial.println("Pin is HIGH");
  } else {
    Serial.println("Pin is LOW");
  }
 delay(1000);  // Delay for 1 second before reading again
}
//   static boolean newDataReady = 0;
//   const int serialPrintInterval = 200; //increase value to slow down serial print activity
  
//   // check for new data/start next conversion:
//   if (LoadCell.update()) newDataReady = true;

//   // get smoothed value from the dataset:
//   if (newDataReady) {
//     if (millis() > t + serialPrintInterval) {
//       float i = LoadCell.getData();
//       //Serial.print("Load_cell output val: ");
//       //Serial.println(i);
//       newDataReady = 0;
//       t = millis();
//       DisplayWeight(display, timer,i);
//       display.display();
//    }
//   }

//   // receive command from serial terminal
//   if (Serial.available() > 0) {
//     char inByte = Serial.read();
//     if (inByte == 't') LoadCell.tareNoDelay(); //tare
//     else if (inByte == 'r') calibrate(LoadCell, ScreenDisplay); //calibrate
//     else if (inByte == 'c') changeSavedCalFactor(LoadCell); //edit calibration value manually
//   }

//   // check if last tare operation is complete
//   if (LoadCell.getTareStatus() == true) {
//     Serial.println("Tare complete");
//   }
//   //if (timer.hasPassed(1))
//   //{
// //  UpdateStatus(timer);
//   //display.display();
// //  }
//   //delay(10);
// }

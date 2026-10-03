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
#define _PWM_LOGLEVEL_       1

#include "SAMD_PWM.h"
#include <Servo.h>

// Not OK for Nano_33_IoT (0, 1, 7, 8, 13, 14, 15 )
// OK for Nano_33_IoT (2, 3, 4, 5, 6, 9, 10, 11, 12, 16, 17)
// TCC OK => pin 4, 5, 6, 8, 9, 10, 11, 16/A2, 17/A3
// TC OK  => pin 12
// For ITSYBITSY_M4
// 16-bit Higher accuracy, Lower Frequency, PWM Pin OK: TCCx: 0-2, 4, 5, 7, 9-13
//  8-bit Lower  accuracy, Hi Frequency,    PWM Pin OK: TCx: 18-20, 24-25

//#define pinToUse       11

HX711_ADC LoadCell(HX711_dout, HX711_sck);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Chrono timer(Chrono::SECONDS);
//creates pwm instance
SAMD_PWM* PWM_Instance;
Servo myservo;  // create servo object to control a servo
int pos = 0;    // variable to store the servo position

float frequency = 0.0f;

float dutyCycle = 0.00f;

uint8_t channel = 0;
const int button1Pin = 0;  // Pin number to read from
const int button2Pin = 1;  // Pin number to read from
const int button3Pin = 2;  // Pin number to read from
const int button4Pin = 3;  // Pin number to read from
const int servoPin = 4;
const int buzzerPin = 5;
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
  pinMode(buzzerPin, OUTPUT);
  SetupDisplay(display);
  String message;
  SetupLoadCell(LoadCell, message);
  if (message != "") DisplayMessage(display,message,3000);
  timer.restart();
  //if (! settings.valid) calibrate(LoadCell, ScreenDisplay);
  DisplayWeight(display,timer,0.0f);
  Serial.print(F("\nStarting PWM_Basic on "));
  Serial.println(BOARD_NAME);
  Serial.println(SAMD_PWM_VERSION);

  //assigns PWM frequency of 1.0 KHz and a duty cycle of 0%
  PWM_Instance = new SAMD_PWM(buzzerPin, frequency, dutyCycle);
  myservo.attach(servoPin,0,45);
  myservo.write(0);  
  Serial.println("Started");
}
void soundoff()
{
  // PWM_Instance->setPWM_DCPercentage_manual(buzzerPin, 0.0f);
  frequency = 0.0f;
  dutyCycle = 0.0f;
  PWM_Instance->setPWM(buzzerPin, frequency, dutyCycle);  // freq, duty cycle
}
bool playingSound()
{
  return  (dutyCycle > 0.0f);
}
void beep()
{
  frequency = 3000.0f;
  dutyCycle = 50.0f;
  PWM_Instance->setPWM(buzzerPin, frequency, dutyCycle);  // freq, duty cycle
}

unsigned long tScale = 0;
unsigned long tBeep = 0;
bool debouncing = false;

bool played = false;
void loop() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 150; //increase value to slow down serial print activity
  const int beepLen = 50; //increase value to slow down serial print activity

  int pinValue = digitalRead(button1Pin);  // Read the value of the pin (HIGH or LOW)
  
  if(pinValue == HIGH && !playingSound() && !debouncing) {
    Serial.println("Pin is HIGH");
    tBeep = millis();
    debouncing = true;
    beep();
  } else {
    if ((millis() > tBeep + beepLen) && playingSound())
    {
      Serial.println("Sound off");
      soundoff();

      for (pos = 0; pos <= 45; pos += 1) { // goes from 0 degrees to 180 degrees
        // in steps of 1 degree
        myservo.write(pos);              // tell servo to go to position in variable 'pos'
        delay(15);                       // waits 15ms for the servo to reach the position
      }
      delay(5000);
      pos = 0;
      myservo.write(pos);
      // for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
      //   myservo.write(pos);              // tell servo to go to position in variable 'pos'
      //   delay(15);                       // waits 15ms for the servo to reach the position
      // }


    }
    //Serial.println("Pin is LOW");
    //soundoff();
  }
  if (pinValue == LOW)
  {
    debouncing = false;
  }

   //delay(100);  // Delay for 1 second before reading again
   //Serial.println("got there");

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > tScale + serialPrintInterval) {
      float i = LoadCell.getData();
      //Serial.print("Load_cell output val: ");
      //Serial.println(i);
      newDataReady = 0;
      tScale = millis();
      DisplayWeight(display, timer,i);
      display.display();
   }
  }

  // receive command from serial terminal
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay(); //tare
    else if (inByte == 'r') calibrate(LoadCell, ScreenDisplay); //calibrate
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
  delay(10);
}

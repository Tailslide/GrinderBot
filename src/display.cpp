#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "display.h"


void SetupDisplay(Adafruit_SSD1306& display)
{
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.display();
}

void DisplayMessage(Adafruit_SSD1306& display, String message, int pause)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setFont(&FreeMono9pt7b);
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setTextWrap(true);
    display.setCursor(0, 12);     // Start at top-left corner
      //char weightStr[1024];
    display.print(message.c_str());
    display.display();
    if (pause != 0) delay(pause);
}

void DisplayWeight(Adafruit_SSD1306& display, Chrono fortimer, float weight)
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
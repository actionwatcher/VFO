#include <Arduino.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include "si5351.h"


#include "pinout.h"
#include "Rotary.h"

#define I2C_ADDRESS 0x3C // Or 0x3D depending on your display

SSD1306AsciiWire oled;
Si5351 si5351;
volatile auto currentFreq = 700000000ULL; // 14 MHz
volatile auto prevFreq = currentFreq; // 14 MHz
Rotary rotary(DT, CLK);
String currentDir = "";

void setup() {
  // Set encoder pins as inputs
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);

  // Setup Serial Monitor
  Serial.begin(9600);

  // enable interrupts on the CLK and DT pins
  PCICR |= (1 << PCIE2);    // Enable PCINT2 interrupt
  PCMSK2 |= (1 << PCINT18); // Enable interrupt for pin D3 (DT)
  PCMSK2 |= (1 << PCINT19); // Enable interrupt for pin D2 (CLK)
  // set port D direction to input
  DDRD &= ~((1 << DDD2) | (1 << DDD3));

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  Wire.begin();
  oled.begin(&Adafruit128x32, I2C_ADDRESS); // Or &Adafruit128x32
  oled.setFont(Adafruit5x7); // Set font
  oled.clear();
  oled.set2X(); 
  oled.setCursor(0, 0);
  oled.print("Dir: ");
  oled.setCursor(0, 2);
  oled.print("Freq: ");

    bool success = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  
  if (success) {
    // Set frequency in 0.01 Hz units. 
    // For 14 MHz, use 1400000000ULL (ULL denotes unsigned long long)
    si5351.set_freq(currentFreq, SI5351_CLK0);
  }
  }

ISR(PCINT2_vect) {
  auto result = rotary.process();
  if (result) {
    if (result == DIR_CW) {

      currentFreq += 100ULL;
      currentDir = "CW ";
    } else {
      currentFreq -= 100ULL;
      currentDir = "CCW";
    }
  }
}

void loop() {
  //Do some useful stuff here
  if (prevFreq != currentFreq) {
    prevFreq = currentFreq;
    si5351.set_freq(currentFreq, SI5351_CLK0);
    // Simulate some processing delay
    Serial.print("Direction: ");
    Serial.print(currentDir);
    Serial.print(" | Counter: ");
    unsigned long displayFreq = currentFreq;
    Serial.println(displayFreq);
    auto w = 11;
    oled.setCursor(6*w, 0);
    oled.clearToEOL();
    oled.setCursor(6*w, 0);
    oled.print(currentDir);
    oled.setCursor(6*w, 2);
    oled.clearToEOL();
    oled.setCursor(6*w, 2);
    oled.print(displayFreq);
  }
}

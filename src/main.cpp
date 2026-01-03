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
volatile auto currentFreq = 700000000ULL; // Starting frequency: 7000 kHz
auto prevFreq = currentFreq+1; // init to different value to force update on start
auto deltaFreq = 100ULL; // Frequency step: 1 Hz
volatile uint8_t key_state = 1;
uint8_t prevKeyState = 0; // init to different value to force update on start
Rotary rotary(DT, CLK);
String currentDir = "";

void setup() {
  // Set encoder pins as inputs
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(KEY, INPUT_PULLUP);

  // Setup Serial Monitor
  Serial.begin(9600);

  // enable interrupts on the CLK and DT pins
  PCICR |= (1 << PCIE2);    // Enable PCINT2 interrupt
  PCMSK2 |= (1 << PCINT18); // Enable interrupt for pin D3 (DT)
  PCMSK2 |= (1 << PCINT19); // Enable interrupt for pin D2 (CLK)
  PCMSK2 |= (1 << PCINT20); // Enable interrupt for pin D4 (KEY)
  // set port D direction to input
  DDRD &= ~((1 << DDD2) | (1 << DDD3) | (1 << DDD4));

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
  
  if (!success) {
    Serial.println("Si5351 Init failed");
    while (1);
  }
  si5351.set_freq(currentFreq, SI5351_CLK0);
  }

ISR(PCINT2_vect) {
  uint8_t val = PIND;
  key_state = val & (1 << KEY);
  auto result = rotary.process(val);
  if (result) {
    if (result == DIR_CW) {

      currentFreq += deltaFreq;
      currentDir = "CW ";
    } else {
      currentFreq -= deltaFreq;
      currentDir = "CCW";
    }
  }
}

void loop() {
  if (prevFreq != currentFreq || key_state != prevKeyState) {
    prevFreq = currentFreq;
    prevKeyState = key_state;
    si5351.output_enable(SI5351_CLK0, key_state? 0 : 1 ); // Enable output
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
    oled.print(key_state ? "OFF " : "ON ");
    oled.setCursor(6*w, 2);
    oled.clearToEOL();
    oled.setCursor(6*w, 2);
    oled.print(displayFreq);
  }
}

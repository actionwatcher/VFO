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
Rotary rotary(DT, CLK, 250);  // 250 PPR encoder
uint8_t displayPrecision = 3; // number of digits after second dot

// TX/RX control
// Timer1 configuration for delay calculation
constexpr uint32_t F_CPU_HZ = 16000000UL;  // Arduino Uno clock frequency
constexpr uint8_t TIMER1_PRESCALER = 8;     // Timer1 prescaler value
constexpr uint32_t TIMER1_FREQ = F_CPU_HZ / TIMER1_PRESCALER;  // 2MHz
constexpr uint16_t TX_DELAY_MS = 50;       // Desired TX delay in milliseconds
// Calculate timer ticks for desired delay: (TIMER1_FREQ / 1000) * TX_DELAY_MS
// For 50ms at 2MHz: (2000000 / 1000) * 50 = 100,000 ticks
// Timer1 is 16-bit (max 65535), so we need overflow handling
constexpr uint32_t TX_DELAY_TICKS_FULL = (TIMER1_FREQ / 1000UL) * TX_DELAY_MS;
constexpr uint16_t TX_DELAY_TICKS = (uint16_t)(TX_DELAY_TICKS_FULL & 0xFFFF);  // Wrap to 16-bit
volatile uint16_t keyPressTime = 0;  // Timer1 value when key was pressed
volatile bool txDelayActive = false;  // Flag indicating we're in delay period

// Format frequency with dot separators
// decimals: number of digits to show after the second dot (0 = hide them)
String formatFrequency(unsigned long freq, uint8_t decimals = displayPrecision) {
  String result = "";
  String freqStr = String(freq);
  int len = freqStr.length();

  // If decimals < 3, truncate the number
  if (decimals < 3 && len > 3) {
    int truncateDigits = 3 - decimals;
    len -= truncateDigits;
    freqStr = freqStr.substring(0, len);
  }

  for (int i = 0; i < len; i++) {
    result += freqStr[i];
    int remaining = len - i - 1;
    if (remaining > 0 && remaining % 3 == 0) {
      result += '.';
    }
  }
  return result;
}

void setup() {
  // Set encoder pins as inputs
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(KEY, INPUT_PULLUP);

  // Set TRANSMIT pin as output
  pinMode(TRANSMIT_PIN, OUTPUT);
  digitalWrite(TRANSMIT_PIN, LOW);  // Start in RX mode

  // Setup Serial Monitor
  Serial.begin(9600);

  // Initialize Timer1 for velocity tracking
  // Setup Timer1 as a free-running counter at 2MHz (prescaler 8 on 16MHz Arduino)
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B = (1 << CS11);  // Prescaler 8: 16MHz / 8 = 2MHz (0.5µs per tick)
  TCNT1 = 0;

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

  // Use variable speed processing - pass current timer value
  int16_t speedMultiplier = rotary.processWithSpeed(val, TCNT1);
  if (speedMultiplier != 0) {
    // speedMultiplier is already signed (positive for CW, negative for CCW)
    currentFreq += (int64_t)deltaFreq * speedMultiplier;
  }
}

void loop() {
  constexpr int w = 11;

  if (key_state != prevKeyState) {
    prevKeyState = key_state;

    if (prevKeyState) {
      // Key released - going to RX mode
      si5351.output_enable(SI5351_CLK0, 0);  // Stop signal generation
      digitalWrite(TRANSMIT_PIN, LOW);        // TRANSMIT pin LOW
      txDelayActive = false;
      oled.setCursor(0, 0);
      oled.print("RX  ");
    } else {
      // Key pressed - start TX delay
      keyPressTime = TCNT1;
      txDelayActive = true;
      oled.setCursor(0, 0);
      oled.print("  TX");
    }
  }

  // Handle TX delay using Timer1
  // Unsigned arithmetic handles timer overflow automatically
  if (txDelayActive && ((uint16_t)(TCNT1 - keyPressTime) >= TX_DELAY_TICKS)) {
    txDelayActive = false;
    digitalWrite(TRANSMIT_PIN, HIGH);       // TRANSMIT pin HIGH
    si5351.output_enable(SI5351_CLK0, 1);   // Start signal generation
  }
    
  if (prevFreq != currentFreq) {
    prevFreq = currentFreq;
    si5351.set_freq(currentFreq, SI5351_CLK0);
    unsigned long displayFreq = currentFreq/100;
    oled.setCursor(w, 2);
    oled.print(formatFrequency(displayFreq));
  }
}

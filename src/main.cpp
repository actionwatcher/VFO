#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include "si5351.h"


#include "pinout.h"
#include "Rotary.h"

#define I2C_ADDRESS 0x3C // Or 0x3D depending on your display

// Band structure
struct Band {
    const char* name;
    const uint64_t defaultFreq;  // VFO frequency (before multiplication)
    uint64_t lastFreq;           // Runtime value: default or loaded from EEPROM
    const uint8_t mult;          // Output frequency multiplier
};

// Band definitions (excluding WARC bands: 30m, 17m, 12m)
Band bands[] = {
    { "80m",  350000000ULL,  350000000ULL, 1 },   // 3.5 MHz, no mult
    { "40m",  700000000ULL,  700000000ULL, 1 },   // 7.0 MHz, no mult
    { "20m",  700000000ULL,  700000000ULL, 2 },   // 14.0 MHz, VFO at 7MHz
    { "15m", 1050000000ULL, 1050000000ULL, 2 },   // 21.0 MHz, VFO at 10.5MHz
    { "10m", 1400000000ULL, 1400000000ULL, 2 },   // 28.0 MHz, VFO at 14MHz
};
constexpr uint8_t NUM_BANDS = sizeof(bands) / sizeof(bands[0]);

// Button press types
enum ButtonPress {
    BTN_NONE,
    BTN_BAND_UP,
    BTN_BAND_DOWN,
    BTN_TUNE
};

SSD1306AsciiWire oled;
Si5351 si5351;
uint8_t currentBand = 1;  // Start on 40m
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

// Button handling
// 50ms debounce at 2MHz = 100,000 ticks, but Timer1 is 16-bit
// Use shorter debounce that fits: 20ms = 40,000 ticks
constexpr uint16_t DEBOUNCE_MS = 20;
constexpr uint16_t DEBOUNCE_TICKS = (uint16_t)((TIMER1_FREQ / 1000UL) * DEBOUNCE_MS);
uint8_t prevButton = BTN_NONE;
uint16_t buttonChangeTime = 0;
bool tuneActive = false;

// EEPROM functions
// EEPROM memory map:
//   0-39: Band frequencies (8 bytes each, 5 bands)
//   40:   Current band index (1 byte)
constexpr uint16_t EEPROM_BAND_SIZE = 8;
constexpr uint16_t EEPROM_CURRENT_BAND_ADDR = NUM_BANDS * EEPROM_BAND_SIZE;

void saveBandToEEPROM(uint8_t bandIdx) {
  uint16_t addr = bandIdx * EEPROM_BAND_SIZE;
  uint64_t freq = bands[bandIdx].lastFreq;
  EEPROM.put(addr, freq);
}

void saveCurrentBandToEEPROM() {
  EEPROM.update(EEPROM_CURRENT_BAND_ADDR, currentBand);
}

void loadBandFromEEPROM(uint8_t bandIdx) {
  uint16_t addr = bandIdx * EEPROM_BAND_SIZE;
  uint64_t freq;
  EEPROM.get(addr, freq);

  // Validate frequency (simple range check: 1MHz to 30MHz)
  if (freq >= 100000000ULL && freq <= 3000000000ULL) {
    bands[bandIdx].lastFreq = freq;
  }
  // If invalid, keep default value
}

void loadCurrentBandFromEEPROM() {
  uint8_t band = EEPROM.read(EEPROM_CURRENT_BAND_ADDR);
  if (band < NUM_BANDS) {
    currentBand = band;
  }
  // If invalid, keep default (40m)
}

void loadAllBandsFromEEPROM() {
  for (uint8_t i = 0; i < NUM_BANDS; i++) {
    loadBandFromEEPROM(i);
  }
  loadCurrentBandFromEEPROM();
}

// Button reading with voltage divider
// Expected ADC values: BTN1~92, BTN2~205, BTN3~390, NONE~1023
ButtonPress readButton() {
  int val = analogRead(BUTTON_PIN);
  if (val < 150) return BTN_BAND_UP;      // ~92 ± margin
  if (val < 300) return BTN_BAND_DOWN;    // ~205 ± margin
  if (val < 500) return BTN_TUNE;         // ~390 ± margin
  return BTN_NONE;
}

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

  // Load band frequencies from EEPROM
  loadAllBandsFromEEPROM();

  // Set initial frequency from current band
  currentFreq = bands[currentBand].lastFreq;

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

  // Handle button presses with debouncing
  ButtonPress btn = readButton();
  if (btn != prevButton) {
    uint16_t now = TCNT1;
    // Check if enough time has passed since last change (debouncing)
    if ((uint16_t)(now - buttonChangeTime) >= DEBOUNCE_TICKS) {
      buttonChangeTime = now;

      if (btn == BTN_BAND_UP && prevButton == BTN_NONE) {
        // Save current band's frequency to EEPROM
        bands[currentBand].lastFreq = currentFreq;
        saveBandToEEPROM(currentBand);

        // Move to next band (wrap around)
        currentBand = (currentBand + 1) % NUM_BANDS;
        saveCurrentBandToEEPROM();
        currentFreq = bands[currentBand].lastFreq;
        prevFreq = currentFreq + 1;  // Force display update

      } else if (btn == BTN_BAND_DOWN && prevButton == BTN_NONE) {
        // Save current band's frequency to EEPROM
        bands[currentBand].lastFreq = currentFreq;
        saveBandToEEPROM(currentBand);

        // Move to previous band (wrap around)
        currentBand = (currentBand == 0) ? (NUM_BANDS - 1) : (currentBand - 1);
        saveCurrentBandToEEPROM();
        currentFreq = bands[currentBand].lastFreq;
        prevFreq = currentFreq + 1;  // Force display update

      } else if (btn == BTN_TUNE) {
        // TUNE pressed - enable signal immediately (no TRANSMIT pin, no delay)
        si5351.output_enable(SI5351_CLK0, 1);
        tuneActive = true;
      } else if (tuneActive && btn == BTN_NONE) {
        // TUNE released - disable signal and save frequency
        si5351.output_enable(SI5351_CLK0, 0);
        tuneActive = false;
        bands[currentBand].lastFreq = currentFreq;
        saveBandToEEPROM(currentBand);
      }

      prevButton = btn;
    }
  }

  // Handle KEY (morse key) press for TX
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
    bands[currentBand].lastFreq = currentFreq;  // Update band's last frequency
    si5351.set_freq(currentFreq, SI5351_CLK0);
    unsigned long displayFreq = currentFreq/100 * bands[currentBand].mult;
    oled.setCursor(w, 2);
    oled.print(formatFrequency(displayFreq));
    // Display band name
    oled.setCursor(0, 0);
    oled.print(bands[currentBand].name);
    oled.print("  ");  // Clear any leftover characters
  }
}

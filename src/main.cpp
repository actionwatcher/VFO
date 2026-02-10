#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include "si5351.h"

#include "pinout.h"
#include "Rotary.h"
#include "task.h"
#include "VFOState.h"

#define I2C_ADDRESS 0x3C // Or 0x3D depending on your display

// Band definitions (excluding WARC bands: 30m, 17m, 12m)
// deltaFreq = 100 (1Hz output step) / mult
Band bands[] = {
    { "80m",  350000000ULL,  350000000ULL, 1, 100 },   // 3.5 MHz, no mult, 1Hz step
    { "40m",  700000000ULL,  700000000ULL, 1, 100 },   // 7.0 MHz, no mult, 1Hz step
    { "20m",  700000000ULL,  700000000ULL, 2,  50 },   // 14.0 MHz, VFO at 7MHz, 0.5Hz VFO step = 1Hz output
    { "15m", 1050000000ULL, 1050000000ULL, 2,  50 },   // 21.0 MHz, VFO at 10.5MHz, 0.5Hz VFO step
    { "10m", 1400000000ULL, 1400000000ULL, 2,  50 },   // 28.0 MHz, VFO at 14MHz, 0.5Hz VFO step
};
const uint8_t NUM_BANDS = sizeof(bands) / sizeof(bands[0]);

// Global state
VFOState state = {
    .currentBand = 1,                      // Start on 40m
    .currentFreq = 700000000ULL,           // 7.0 MHz
    .prevFreq = 700000000ULL + 1,          // Different to force initial update
    .keyState = 1,
    .prevKeyState = 0,                     // Different to force initial update
    .prevButton = BTN_NONE,
    .buttonChangeTime = 0,
    .tuneActive = false,
    .keyPressTime = 0,
    .txDelayActive = false,
    .lastShift = 0
};

SSD1306AsciiWire oled;
Si5351 si5351;
Rotary rotary(DT, CLK, 250);  // 250 PPR encoder
uint8_t displayPrecision = 3;

// Timer1 configuration
constexpr uint32_t F_CPU_HZ = 16000000UL;
constexpr uint16_t TIMER1_PRESCALER = 256;
constexpr uint32_t TIMER1_FREQ = F_CPU_HZ / TIMER1_PRESCALER;  // 62.5kHz
constexpr uint16_t TX_DELAY_MS = 50;
constexpr uint32_t TX_DELAY_TICKS_FULL = (TIMER1_FREQ / 1000UL) * TX_DELAY_MS;
const uint16_t TX_DELAY_TICKS = (uint16_t)(TX_DELAY_TICKS_FULL & 0xFFFF);
constexpr uint16_t DEBOUNCE_MS = 20;
const uint16_t DEBOUNCE_TICKS = (uint16_t)((TIMER1_FREQ / 1000UL) * DEBOUNCE_MS);

// EEPROM functions
constexpr uint16_t EEPROM_BAND_SIZE = 8;
constexpr uint16_t EEPROM_CURRENT_BAND_ADDR = 5 * EEPROM_BAND_SIZE;  // NUM_BANDS * 8

void saveBandToEEPROM(uint8_t bandIdx) {
    uint16_t addr = bandIdx * EEPROM_BAND_SIZE;
    uint64_t freq = bands[bandIdx].lastFreq;
    EEPROM.put(addr, freq);
}

void saveCurrentBandToEEPROM() {
    EEPROM.update(EEPROM_CURRENT_BAND_ADDR, state.currentBand);
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
        state.currentBand = band;
    }
    // If invalid, keep default (40m)
}

void loadAllBandsFromEEPROM() {
    for (uint8_t i = 0; i < NUM_BANDS; i++) {
        loadBandFromEEPROM(i);
    }
    loadCurrentBandFromEEPROM();
}

// Task function declarations
extern task_state_t buttonTask();
extern task_state_t bandTask();
extern task_state_t tuneTask();
extern task_state_t keyTask();
extern task_state_t displayTask();

void setup() {
    // Set encoder pins as inputs
    pinMode(CLK, INPUT);
    pinMode(DT, INPUT);
    pinMode(KEY, INPUT_PULLUP);

    // Set TRANSMIT pin as output
    pinMode(PTT_PIN, OUTPUT);
    digitalWrite(PTT_PIN, LOW);  // Start in RX mode

    // Setup Serial Monitor
    Serial.begin(9600);

    // Initialize Timer1 for velocity tracking
    TCCR1A = 0;
    TCCR1B = 0;
    TCCR1B = (1 << CS12);  // Prescaler 256: 16MHz / 256 = 62.5kHz
    TCNT1 = 0;

    // Enable interrupts on the CLK, DT, and KEY pins
    PCICR |= (1 << PCIE2);
    PCMSK2 |= (1 << PCINT18); // D3 (DT)
    PCMSK2 |= (1 << PCINT19); // D2 (CLK)
    PCMSK2 |= (1 << PCINT20); // D4 (KEY)
    DDRD &= ~((1 << DDD2) | (1 << DDD3) | (1 << DDD4));

    // Load band frequencies from EEPROM
    loadAllBandsFromEEPROM();
    state.currentFreq = bands[state.currentBand].lastFreq;

    Wire.begin();
    oled.begin(&Adafruit128x64, I2C_ADDRESS);
    oled.setFont(Adafruit5x7);
    oled.clear();
    oled.set2X();

    bool success = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
    if (!success) {
        Serial.println("Si5351 Init failed");
        while (1);
    }
    si5351.set_freq(state.currentFreq, SI5351_CLK0);

    // Register tasks
    addTask(buttonTask);
    addTask(bandTask);
    addTask(tuneTask);
    addTask(keyTask);
    addTask(displayTask);
}

ISR(PCINT2_vect) {
    uint8_t val = PIND;
    state.keyState = val & (1 << KEY);

    RotaryResult r = rotary.processWithSpeed(val, TCNT1);
    if (r.direction == DIR_CW) {
        state.currentFreq += bands[state.currentBand].deltaFreq << r.shift;
        state.lastShift = r.shift;
    } else if (r.direction == DIR_CCW) {
        state.currentFreq -= bands[state.currentBand].deltaFreq << r.shift;
        state.lastShift = r.shift;
    }
}

void loop() {
    runTasks();
}

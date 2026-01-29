#pragma once

#include <Arduino.h>

// Button press types
enum ButtonPress {
    BTN_NONE,
    BTN_BAND_UP,
    BTN_BAND_DOWN,
    BTN_TUNE
};

// Band structure
struct Band {
    const char* name;
    const uint64_t defaultFreq;  // VFO frequency (before multiplication)
    uint64_t lastFreq;           // Runtime value: default or loaded from EEPROM
    const uint8_t mult;          // Output frequency multiplier
    const uint64_t deltaFreq;    // VFO frequency step (output step / mult)
};

// Shared state structure
struct VFOState {
    uint8_t currentBand;
    volatile uint64_t currentFreq;
    uint64_t prevFreq;
    volatile uint8_t keyState;
    uint8_t prevKeyState;
    ButtonPress prevButton;
    uint16_t buttonChangeTime;
    bool tuneActive;
    volatile uint16_t keyPressTime;
    volatile bool txDelayActive;
};

extern VFOState state;
extern Band bands[];
extern const uint8_t NUM_BANDS;

// Timer constants
extern const uint16_t DEBOUNCE_TICKS;
extern const uint16_t TX_DELAY_TICKS;

// EEPROM functions
void saveBandToEEPROM(uint8_t bandIdx);
void saveCurrentBandToEEPROM();
void loadBandFromEEPROM(uint8_t bandIdx);
void loadCurrentBandFromEEPROM();
void loadAllBandsFromEEPROM();


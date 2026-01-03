/*
 * Rotary encoder library for Arduino.
 */

#ifndef rotary_h
#define rotary_h

#include "Arduino.h"

// Enable this to emit codes twice per step.
//#define HALF_STEP

// Enable weak pullups
//#define ENABLE_PULLUPS

// Values returned by 'process'
// No complete step yet.
#define DIR_NONE 0x0
// Clockwise step.
#define DIR_CW 0x10
// Anti-clockwise step.
#define DIR_CCW 0x20

class Rotary
{
public:
    Rotary(char, char);
    // Process pin(s)
    uint8_t process(const uint8_t val);

    // Process with velocity-based multiplier
    // timerValue: current timer counter value (e.g., TCNT1)
    // Returns: signed multiplier (positive for CW, negative for CCW, 0 for no movement)
    int16_t processWithSpeed(const uint8_t val, uint16_t timerValue);

    // Get current speed multiplier (for debugging)
    uint16_t getMultiplier() const { return currentMultiplier; }

private:
    // State machine state
    uint8_t state;
    uint8_t pin1;
    uint8_t pin2;

    // Velocity tracking using Timer1
    uint16_t lastTimerValue;
    uint16_t stepInterval;
    uint16_t currentMultiplier;

    // Tunable acceleration thresholds (in Timer1 ticks at 2MHz: 0.5µs per tick)
    // These values are for 16MHz Arduino
    static constexpr uint16_t THRESHOLD_SLOW = 8000;    // ~4ms - slow turning
    static constexpr uint16_t THRESHOLD_MEDIUM = 4000;  // ~2ms - medium speed
    static constexpr uint16_t THRESHOLD_FAST = 2000;    // ~1ms - fast turning
    static constexpr uint16_t THRESHOLD_VERY_FAST = 1000; // ~0.5ms - very fast

    // Speed multipliers
    static constexpr uint16_t MULT_SLOW = 1;
    static constexpr uint16_t MULT_MEDIUM = 5;
    static constexpr uint16_t MULT_FAST = 20;
    static constexpr uint16_t MULT_VERY_FAST = 100;

    // Calculate multiplier based on step interval
    uint16_t calculateMultiplier();
};

#endif

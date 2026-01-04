/*
 * Rotary encoder library for Arduino.
 */

#ifndef rotary_h
#define rotary_h

#include "Arduino.h"

#define DIR_NONE 0x0
// Clockwise step.
#define DIR_CW 0x10
// Anti-clockwise step.
#define DIR_CCW 0x20

class Rotary
{
public:
    Rotary(char pin1, char pin2, uint16_t ppr = 250);
    // Process pin(s)
    uint8_t process(const uint8_t val);

    // Process with velocity-based multiplier
    // timerValue: current timer counter value (e.g., TCNT1)
    // Returns: signed multiplier (positive for CW, negative for CCW, 0 for no movement)
    int16_t processWithSpeed(const uint8_t val, uint16_t timerValue);

    // Get current speed multiplier (for debugging)
    uint16_t getMultiplier() const { return _currentMultiplier; }

private:
    // State machine state
    uint8_t _state;
    const uint8_t _pin1;
    const uint8_t _pin2;

    // Velocity tracking
    uint16_t _lastTimerValue;
    uint16_t _currentMultiplier;

    // Encoder pulses per revolution (PPR)
    uint16_t _pulsesPerRev;

    // Base thresholds for 20 PPR encoder (in Timer1 ticks at 2MHz: 0.5µs per tick)
    // These get scaled based on actual encoder PPR
    static constexpr uint16_t BASE_THRESHOLD_SLOW = 8000;      // ~4ms - slow turning
    static constexpr uint16_t BASE_THRESHOLD_MEDIUM = 4000;    // ~2ms - medium speed
    static constexpr uint16_t BASE_THRESHOLD_FAST = 2000;      // ~1ms - fast turning
    static constexpr uint16_t BASE_THRESHOLD_VERY_FAST = 1000; // ~0.5ms - very fast
    static constexpr uint16_t BASE_PPR = 250;  // Reference encoder: 250 pulses/rev

    // Scaled thresholds (calculated in constructor based on PPR)
    const uint16_t _thresholdSlow;
    const uint16_t _thresholdMedium;
    const uint16_t _thresholdFast;
    const uint16_t _thresholdVeryFast;

    // Speed multipliers
    static constexpr uint16_t MULT_SLOW = 1;
    static constexpr uint16_t MULT_MEDIUM = 5;
    static constexpr uint16_t MULT_FAST = 20;
    static constexpr uint16_t MULT_VERY_FAST = 100;

    // Calculate multiplier based on step interval
    uint16_t _calculateMultiplier(uint16_t stepInterval);
};

#endif

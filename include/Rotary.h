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

// Result from processWithSpeed
struct RotaryResult {
    uint8_t direction;  // DIR_NONE, DIR_CW, or DIR_CCW
    uint8_t shift;      // Shift amount for << operator
};

class Rotary
{
public:
    Rotary(char pin1, char pin2, uint16_t ppr = 250);
    // Process pin(s)
    uint8_t process(const uint8_t val);

    // Process with velocity-based shift amount
    // timerValue: current timer counter value (e.g., TCNT1)
    // Returns: RotaryResult with direction and shift
    RotaryResult processWithSpeed(const uint8_t val, uint16_t timerValue);

private:
    // State machine state
    uint8_t _state;
    const uint8_t _pin1;
    const uint8_t _pin2;

    // Velocity tracking
    uint16_t _lastTimerValue;
    uint8_t _currentShift;

    // Encoder pulses per revolution (PPR)
    uint16_t _pulsesPerRev;

    // Base thresholds for 250 PPR encoder (in Timer1 ticks at 62.5kHz: 16µs per tick)
    // These get scaled based on actual encoder PPR
    static constexpr uint16_t BASE_THRESHOLD_MEDIUM = 800;    // ~32ms - medium speed
    static constexpr uint16_t BASE_THRESHOLD_FAST = 450;      // ~20ms - fast turning
    static constexpr uint16_t BASE_THRESHOLD_VERY_FAST = 200; // ~16ms - very fast
    static constexpr uint16_t BASE_PPR = 250;  // Reference encoder: 250 pulses/rev

    // Scaled thresholds (calculated in constructor based on PPR)
    const uint16_t _thresholdMedium;
    const uint16_t _thresholdFast;
    const uint16_t _thresholdVeryFast;

    // Speed shift amounts (powers of 2: 1, 2, 4, 8, 16)
    static constexpr uint8_t SHIFT_SLOW = 0;       // 1x
    static constexpr uint8_t SHIFT_MEDIUM = 1;     // 2x
    static constexpr uint8_t SHIFT_FAST = 3;       // 4x
    static constexpr uint8_t SHIFT_VERY_FAST = 6;  // 16x

    // Calculate shift amount based on step interval
    uint8_t _calculateShift(uint16_t stepInterval);
};

#endif

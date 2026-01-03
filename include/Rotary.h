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
    uint8_t process();
private:
    uint8_t state;
    uint8_t pin1;
    uint8_t pin2;
};

#endif

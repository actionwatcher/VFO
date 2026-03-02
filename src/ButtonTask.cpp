#include "task.h"
#include "VFOState.h"
#include "pinout.h"

static ButtonPress readButton() {
    // Voltage divider: 10kΩ pull-up to 5V, each button connects resistor to GND:
    // BTN_BAND_UP:   1kΩ  → ADC ~93
    // BTN_BAND_DOWN: 2.2kΩ → ADC ~184
    // BTN_TUNE:      4.7kΩ → ADC ~327
    // BTN_RESERVED_1: 10kΩ → ADC ~512
    // BTN_RESERVED_2: 22kΩ → ADC ~704
    // BTN_RESERVED_3: 47kΩ → ADC ~844
    // None:          open  → ADC ~1023
    int val = analogRead(BUTTON_PIN);
    if (val < 138) return BTN_BAND_UP;
    if (val < 255) return BTN_BAND_DOWN;
    if (val < 420) return BTN_TUNE;
    if (val < 608) return BTN_RESERVED_1;
    if (val < 774) return BTN_RESERVED_2;
    if (val < 933) return BTN_RESERVED_3;
    return BTN_NONE;
}

task_state_t buttonTask() {
    ButtonPress btn = readButton();
    if (btn != state.prevButton) {
        uint16_t now = TCNT1;
        // Check if enough time has passed since last change (debouncing)
        if ((uint16_t)(now - state.buttonChangeTime) >= DEBOUNCE_TICKS) {
            state.buttonChangeTime = now;
            state.prevButton = btn;
        }
    }
    return task_state_t::kYeilding;
}

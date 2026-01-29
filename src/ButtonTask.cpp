#include "task.h"
#include "VFOState.h"
#include "pinout.h"

static ButtonPress readButton() {
    int val = analogRead(BUTTON_PIN);
    if (val < 150) return BTN_BAND_UP;
    if (val < 300) return BTN_BAND_DOWN;
    if (val < 500) return BTN_TUNE;
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

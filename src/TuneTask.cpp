#include "task.h"
#include "VFOState.h"
#include "si5351.h"

extern Si5351 si5351;

static ButtonPress lastHandledButton = BTN_NONE;

task_state_t tuneTask() {
    // Only process on button change (edge detection)
    if (state.prevButton == lastHandledButton) {
        return task_state_t::kYeilding;
    }

    ButtonPress btn = state.prevButton;

    if (btn == BTN_TUNE) {
        // TUNE pressed - enable signal immediately (no TRANSMIT pin, no delay)
        si5351.output_enable(SI5351_CLK0, 1);
        state.tuneActive = true;
    } else if (state.tuneActive && btn == BTN_NONE) {
        // TUNE released - disable signal and save frequency
        si5351.output_enable(SI5351_CLK0, 0);
        state.tuneActive = false;
        bands[state.currentBand].lastFreq = state.currentFreq;
        saveBandToEEPROM(state.currentBand);
    }

    lastHandledButton = btn;
    return task_state_t::kYeilding;
}

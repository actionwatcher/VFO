#include "task.h"
#include "VFOState.h"
#include "DDS.h"

static ButtonPress lastHandledButton = BTN_NONE;

task_state_t tuneTask() {
    // Only process on button change (edge detection)
    if (state.prevButton == lastHandledButton) {
        return task_state_t::kYeilding;
    }

    ButtonPress btn = state.prevButton;

    if (btn == BTN_TUNE) {
        // TUNE pressed - enable signal immediately (no TRANSMIT pin, no delay)
        dds.enable(true);
        state.tuneActive = true;
    } else if (state.tuneActive && btn == BTN_NONE) {
        // TUNE released - disable signal and save frequency
        dds.enable(false);
        state.tuneActive = false;
        bands[state.currentBand].lastFreq = state.currentFreq;
        saveBandToEEPROM(state.currentBand);
    }

    lastHandledButton = btn;
    return task_state_t::kYeilding;
}

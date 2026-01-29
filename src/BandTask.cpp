#include "task.h"
#include "VFOState.h"

static ButtonPress lastHandledButton = BTN_NONE;

task_state_t bandTask() {
    // Only process on button change (edge detection)
    if (state.prevButton == lastHandledButton) {
        return task_state_t::kYeilding;
    }

    ButtonPress btn = state.prevButton;

    if (btn == BTN_BAND_UP && lastHandledButton == BTN_NONE) {
        // Save current band's frequency to EEPROM
        bands[state.currentBand].lastFreq = state.currentFreq;
        saveBandToEEPROM(state.currentBand);

        // Move to next band (wrap around)
        state.currentBand = (state.currentBand + 1) % NUM_BANDS;
        saveCurrentBandToEEPROM();
        state.currentFreq = bands[state.currentBand].lastFreq;
        state.prevFreq = state.currentFreq + 1;  // Force display update

    } else if (btn == BTN_BAND_DOWN && lastHandledButton == BTN_NONE) {
        // Save current band's frequency to EEPROM
        bands[state.currentBand].lastFreq = state.currentFreq;
        saveBandToEEPROM(state.currentBand);

        // Move to previous band (wrap around)
        state.currentBand = (state.currentBand == 0) ? (NUM_BANDS - 1) : (state.currentBand - 1);
        saveCurrentBandToEEPROM();
        state.currentFreq = bands[state.currentBand].lastFreq;
        state.prevFreq = state.currentFreq + 1;  // Force display update
    }

    lastHandledButton = btn;
    return task_state_t::kYeilding;
}

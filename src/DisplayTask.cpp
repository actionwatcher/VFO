#include "task.h"
#include "VFOState.h"
#include "si5351.h"
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

extern Si5351 si5351;
extern SSD1306AsciiWire oled;
extern uint8_t displayPrecision;

// Format frequency with dot separators
static String formatFrequency(unsigned long freq, uint8_t decimals) {
    String result = "";
    String freqStr = String(freq);
    int len = freqStr.length();

    // If decimals < 3, truncate the number
    if (decimals < 3 && len > 3) {
        int truncateDigits = 3 - decimals;
        len -= truncateDigits;
        freqStr = freqStr.substring(0, len);
    }

    for (int i = 0; i < len; i++) {
        result += freqStr[i];
        int remaining = len - i - 1;
        if (remaining > 0 && remaining % 3 == 0) {
            result += '.';
        }
    }
    return result;
}

task_state_t displayTask() {
    constexpr int w = 11;

    if (state.prevFreq != state.currentFreq) {
        state.prevFreq = state.currentFreq;
        bands[state.currentBand].lastFreq = state.currentFreq;  // Update band's last frequency
        si5351.set_freq(state.currentFreq, SI5351_CLK0);
        unsigned long displayFreq = state.currentFreq / 100 * bands[state.currentBand].mult;
        oled.setCursor(w, 2);
        oled.print(formatFrequency(displayFreq, displayPrecision));
        // Display band name
        oled.setCursor(0, 0);
        oled.print(bands[state.currentBand].name);
        oled.print("  ");  // Clear any leftover characters
    }

    return task_state_t::kYeilding;
}

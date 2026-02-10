#include "task.h"
#include "VFOState.h"
#include "pinout.h"
#include "si5351.h"
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

extern Si5351 si5351;
extern SSD1306AsciiWire oled;

task_state_t keyTask() {
    // Handle KEY (morse key) press for TX
    if (state.keyState != state.prevKeyState) {
        state.prevKeyState = state.keyState;

        if (state.prevKeyState) {
            // Key released - going to RX mode
            si5351.output_enable(SI5351_CLK0, 0);  // Stop signal generation
            digitalWrite(PTT_PIN, LOW);        // TRANSMIT pin LOW
            state.txDelayActive = false;
            oled.setCursor(0, 0);
            oled.print("RX  ");
        } else {
            // Key pressed - start TX delay
            state.keyPressTime = TCNT1;
            state.txDelayActive = true;
            oled.setCursor(0, 0);
            oled.print("  TX");
        }
    }

    // Handle TX delay using Timer1
    // Unsigned arithmetic handles timer overflow automatically
    if (state.txDelayActive && ((uint16_t)(TCNT1 - state.keyPressTime) >= TX_DELAY_TICKS)) {
        state.txDelayActive = false;
        digitalWrite(PTT_PIN, HIGH);       // TRANSMIT pin HIGH
        si5351.output_enable(SI5351_CLK0, 1);   // Start signal generation
    }

    return task_state_t::kYeilding;
}

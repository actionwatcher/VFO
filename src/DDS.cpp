#include "DDS.h"
#include "pinout.h"

#if defined(DDS_SI5351)
#include "si5351.h"
static Si5351 si5351;
#endif

DDSController dds;

bool DDSController::init() {
#if defined(DDS_SI5351)
    return si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
#elif defined(DDS_AD9850)
    pinMode(AD9850_W_CLK, OUTPUT);
    pinMode(AD9850_FQ_UD, OUTPUT);
    pinMode(AD9850_DATA, OUTPUT);
    pinMode(AD9850_RESET, OUTPUT);

    digitalWrite(AD9850_W_CLK, LOW);
    digitalWrite(AD9850_FQ_UD, LOW);
    digitalWrite(AD9850_DATA, LOW);
    digitalWrite(AD9850_RESET, LOW);

    // Pulse RESET to initialize serial mode
    digitalWrite(AD9850_RESET, HIGH);
    delay(1);
    digitalWrite(AD9850_RESET, LOW);
    
    // Set serial mode: W_CLK high then low, then FQ_UD high then low
    digitalWrite(AD9850_W_CLK, HIGH);
    delayMicroseconds(1);
    digitalWrite(AD9850_W_CLK, LOW);
    delayMicroseconds(1);
    digitalWrite(AD9850_FQ_UD, HIGH);
    delayMicroseconds(1);
    digitalWrite(AD9850_FQ_UD, LOW);
    return true;
#else
    return false;
#endif
}

void DDSController::set_freq(uint64_t freq_hz) {
    if (_freq != freq_hz) {
        _freq = freq_hz;
        update_hardware();
    }
}

void DDSController::enable(bool en) {
    if (_enabled != en) {
        _enabled = en;
        update_hardware();
    }
}

void DDSController::update_hardware() {
#if defined(DDS_SI5351)
    if (_enabled && _freq > 0) {
        si5351.set_freq(_freq, SI5351_CLK0);
        si5351.output_enable(SI5351_CLK0, 1);
    } else {
        si5351.output_enable(SI5351_CLK0, 0);
    }
#elif defined(DDS_AD9850)
    uint32_t ftw = 0;
    bool should_power_down = !_enabled || (_freq == 0);
    
    if (!should_power_down) {
        // Calculate 32-bit frequency tuning word: FTW = (freq_hz * 2^32) / 125000000
        // Use 64-bit integer arithmetic to prevent overflow:
        ftw = (uint32_t)(((uint64_t)_freq * 4294967296ULL) / 125000000ULL);
    }
    
    // Shift out 32 bits of FTW (LSB first)
    for (int i = 0; i < 32; i++) {
        digitalWrite(AD9850_DATA, (ftw >> i) & 1);
        digitalWrite(AD9850_W_CLK, HIGH);
        delayMicroseconds(1);
        digitalWrite(AD9850_W_CLK, LOW);
        delayMicroseconds(1);
    }
    
    // Shift out 8 bits of Control / Phase word (LSB first)
    // D32, D33 = 0
    // D34 = Power Down
    // D35..D39 = 0
    uint8_t ctrl_phase = 0;
    if (should_power_down) {
        ctrl_phase |= (1 << 2); // Bit 2 corresponds to D34 (Power Down)
    }
    
    for (int i = 0; i < 8; i++) {
        digitalWrite(AD9850_DATA, (ctrl_phase >> i) & 1);
        digitalWrite(AD9850_W_CLK, HIGH);
        delayMicroseconds(1);
        digitalWrite(AD9850_W_CLK, LOW);
        delayMicroseconds(1);
    }
    
    // Pulse FQ_UD to load the new 40-bit control word into the DDS core
    digitalWrite(AD9850_FQ_UD, HIGH);
    delayMicroseconds(1);
    digitalWrite(AD9850_FQ_UD, LOW);
#endif
}

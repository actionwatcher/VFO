#pragma once
#include <Arduino.h>
#include "platform.h"

class DDSController {
public:
    bool init();
    void set_freq(uint64_t freq_hz);
    void enable(bool en);

private:
    bool _enabled = false;
    uint64_t _freq = 0;
    void update_hardware();
};

extern DDSController dds;

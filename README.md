This repo about simulating vintage VFO for tube transmitters. I plan to include support for receivers VFOs as well.

## Features
* Tunable frequency in assumption of multiplication in tube trx
* Variable speed tuning (faster rotation larger steps)
* Frequency and band displayed on OLED display
* Straight key support for VFO manipulation

## Pin Assignment

| Pin | Function |
|-----|----------|
| D2  | Rotary encoder CLK |
| D3  | Rotary encoder DT |
| D4  | CW key (INPUT_PULLUP) |
| D5  | PTT output (HIGH = TX) |
| A0  | Button array (analog voltage divider) |

## Button Circuit

6 buttons on a single analog pin via voltage divider. Pull-up resistor 10kΩ to 5V, each button connects a resistor to GND:

```
5V ── 10kΩ ──┬── A0
             │
       BTN1 ─┤── 1kΩ   ── GND
       BTN2 ─┤── 2.2kΩ ── GND
       BTN3 ─┤── 4.7kΩ ── GND
       BTN4 ─┤── 10kΩ  ── GND
       BTN5 ─┤── 22kΩ  ── GND
       BTN6 ─┘── 47kΩ  ── GND
```

| Button | Resistor | ADC  | Function    |
|--------|----------|------|-------------|
| BTN1   | 1kΩ      | ~93  | BAND UP     |
| BTN2   | 2.2kΩ    | ~184 | BAND DOWN   |
| BTN3   | 4.7kΩ    | ~327 | TUNE        |
| BTN4   | 10kΩ     | ~512 | Reserved    |
| BTN5   | 22kΩ     | ~704 | Reserved    |
| BTN6   | 47kΩ     | ~844 | Reserved    |

## Variable Speed Tuning

Encoder rotation speed is measured using Timer1 (prescaler 256, 62.5kHz). Faster rotation produces larger frequency steps:

| Speed     | Step multiplier |
|-----------|----------------|
| Slow      | 1x             |
| Medium    | 2x             |
| Fast      | 8x             |
| Very fast | 64x            |

The current multiplier is shown in the top-right corner of the OLED display.

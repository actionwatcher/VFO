// Rotary Encoder Inputs
#define CLK 2       //Rotary encoder CLK pin
#define DT 3        //Rotary encoder DT pin
#define KEY 4       //CW Key pin (with internal pull-up)

// Transmit control output
#define PTT_PIN 5

// Button input (analog voltage divider)
#define BUTTON_PIN A0

// AD9850 Pins
constexpr uint8_t AD9850_W_CLK = 6;
constexpr uint8_t AD9850_FQ_UD = 7;
constexpr uint8_t AD9850_DATA = 8;
constexpr uint8_t AD9850_RESET = 9;

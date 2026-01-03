# Variable Speed Rotary Encoder Implementation

This branch implements velocity-based acceleration for the rotary encoder, allowing fast frequency changes while maintaining precise control.

## How It Works

The implementation uses **Timer1** as a hardware counter to measure the time between encoder steps:
- **Slow rotation** → Small time intervals between steps → Multiplier = 1
- **Fast rotation** → Large time intervals between steps → Multiplier up to 100

### Key Features

1. **Hardware Timer-Based** - Uses Timer1 for accurate, ISR-safe timing
2. **No millis()/micros()** - Avoids interrupt conflicts
3. **Automatic Overflow Handling** - Unsigned arithmetic handles timer wraparound
4. **Tunable Thresholds** - Easy to adjust acceleration curves

## Technical Details

### Design Principles

1. **No Timer Ownership**: The Rotary class does not own the timer. The caller is responsible for:
   - Setting up the timer hardware
   - Reading the timer value
   - Passing the timer value to `processWithSpeed()`

2. **PPR-Aware Thresholds**: Thresholds automatically scale based on encoder PPR (Pulses Per Revolution):
   - Higher PPR encoders (e.g., 250) have more pulses, so time between pulses is shorter
   - Thresholds are scaled proportionally to maintain consistent feel across encoders
   - No manual tuning needed when changing encoders

This separation of concerns makes the class more flexible and reusable.

### Timer Configuration (Recommended)
- **Timer**: Timer1 (16-bit) - but any timer/counter can be used
- **Prescaler**: 8 (on 16MHz Arduino)
- **Frequency**: 2 MHz (0.5µs per tick)
- **Overflow**: Every ~32.768ms

### Encoder PPR Support

The class automatically scales thresholds based on encoder type:

| Encoder Type | PPR | Scaled Thresholds | Notes |
|-------------|-----|-------------------|-------|
| Low res     | 16-24 | Longer intervals | Cheap encoders, low precision |
| Medium res  | 100 | Medium intervals | Common mechanical encoders |
| High res    | 250 | Shorter intervals | Better feel, recommended |
| Very high   | 600 | Very short intervals | Optical encoders |

**Example**: For a 250 PPR encoder vs 20 PPR reference:
- Base threshold: 8000 ticks (4ms)
- Scaled threshold: 8000 × (20/250) = 640 ticks (0.32ms)
- This maintains the same "feel" regardless of encoder resolution

### Speed Thresholds (Base values for 20 PPR encoder)

| Rotation Speed | Time Between Steps | Timer Ticks (20 PPR) | Timer Ticks (250 PPR) | Multiplier |
|---------------|-------------------|---------------------|----------------------|------------|
| Very Slow     | > 4ms             | > 8000      | > 640               | 1×         |
| Medium        | 2-4ms             | 4000-8000   | 320-640             | 5×         |
| Fast          | 1-2ms             | 2000-4000   | 160-320             | 20×        |
| Very Fast     | 0.5-1ms           | 1000-2000   | 80-160              | 20×        |
| Ultra Fast    | < 0.5ms           | < 1000      | < 80                | 100×       |

### Tuning the Acceleration

**Option 1: Change PPR** (Recommended)
Specify your encoder's PPR in the constructor:
```cpp
Rotary rotary(DT, CLK, 250);  // 250 pulses per revolution
```

**Option 2: Adjust Base Thresholds**
Edit constants in [Rotary.h](include/Rotary.h) to change base thresholds:
```cpp
// Base thresholds for 20 PPR encoder
static constexpr uint16_t BASE_THRESHOLD_SLOW = 8000;      // ~4ms
static constexpr uint16_t BASE_THRESHOLD_MEDIUM = 4000;    // ~2ms
static constexpr uint16_t BASE_THRESHOLD_FAST = 2000;      // ~1ms
static constexpr uint16_t BASE_THRESHOLD_VERY_FAST = 1000; // ~0.5ms
static constexpr uint16_t BASE_PPR = 20;  // Reference PPR

// Multipliers (same for all encoder types)
static constexpr uint16_t MULT_SLOW = 1;
static constexpr uint16_t MULT_MEDIUM = 5;
static constexpr uint16_t MULT_FAST = 20;
static constexpr uint16_t MULT_VERY_FAST = 100;
```

## Usage

### Initialization

Create Rotary object with your encoder's PPR and set up timer in `setup()`:

```cpp
// Create rotary encoder object with 250 PPR encoder
Rotary rotary(DT, CLK, 250);

void setup() {
    // ... other setup code ...

    // Initialize Timer1 for velocity tracking
    // Setup Timer1 as a free-running counter at 2MHz
    TCCR1A = 0;
    TCCR1B = 0;
    TCCR1B = (1 << CS11);  // Prescaler 8: 16MHz / 8 = 2MHz
    TCNT1 = 0;

    // ... rest of setup ...
}
```

**Common PPR values:**
- `Rotary rotary(DT, CLK)` - Default 20 PPR
- `Rotary rotary(DT, CLK, 24)` - EC11 encoder (24 PPR)
- `Rotary rotary(DT, CLK, 100)` - Common mechanical
- `Rotary rotary(DT, CLK, 250)` - High resolution (recommended)
- `Rotary rotary(DT, CLK, 600)` - Optical encoder

### In ISR

Pass the timer value to `processWithSpeed()`:

```cpp
ISR(PCINT2_vect) {
    uint8_t val = PIND;

    // Read timer and pass to processWithSpeed
    int16_t speedMultiplier = rotary.processWithSpeed(val, TCNT1);
    if (speedMultiplier != 0) {
        // Positive = CW, Negative = CCW
        currentFreq += (int64_t)deltaFreq * speedMultiplier;
    }
}
```

**Note**: You can use any timer/counter source. Just pass the current counter value to `processWithSpeed()`. The class uses unsigned arithmetic, so timer overflow is handled automatically.

### Debugging

Get current multiplier for debugging:

```cpp
uint16_t mult = rotary.getMultiplier();
Serial.println(mult);
```

## Benefits

✅ **Fast navigation** - Quickly jump across bands
✅ **Precise tuning** - Fine control when needed
✅ **ISR-safe** - No timing function calls in interrupts
✅ **Low overhead** - Single register read per step
✅ **Natural feel** - Speed-dependent acceleration

## Compatibility

- **Arduino Uno/Nano**: ✅ Fully supported (uses Timer1)
- **Arduino Mega**: ✅ Supported (Timer1 available)
- **Other AVR boards**: ✅ Should work if Timer1 is available

## Comparison with Original

| Feature | Original | Variable Speed |
|---------|----------|----------------|
| Step size | Fixed (100 Hz) | 100 Hz to 10 kHz |
| Band change | ~700 steps | ~7-70 steps |
| Fine tuning | ✅ Good | ✅ Excellent |
| Fast tuning | ❌ Slow | ✅ Very fast |
| ISR overhead | Minimal | Minimal + 1 register read |

## Future Improvements

- [ ] Add momentum/smoothing for even smoother acceleration
- [ ] Make thresholds configurable at runtime
- [ ] Add exponential acceleration curve option
- [ ] Add display indicator for current multiplier

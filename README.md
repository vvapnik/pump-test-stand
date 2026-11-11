# 12V Pump Test Stand

A standalone ATmega328-based stand for characterizing a 12V pump: PWM control,
current measurement with an ACS712, and pump state detection from current
(air / water / approximate pressure in arbitrary units). PWM duty and the
maximum current threshold are set with a rotary encoder and a start/stop
button; readings are shown on an I2C LCD 1602.

No crystal is needed for a first check on a breadboard - use the
`atmega328_internal_rc` environment (internal 8 MHz RC oscillator). The
16 MHz crystal is only needed for the production `atmega328_external_xtal`
environment (see "Build and flash" below).

A CH32V003F4P6 port lives in [ch32v003/](ch32v003/), with a pin check
firmware in [ch32v003-pin-check/](ch32v003-pin-check/).

## Wiring

Power: ATmega328 and LCD from 5V, pump from 12V (separate supply).
**A common GND between the 5V and 12V circuits is mandatory.**

| Signal | ATmega328 pin | Note |
|---|---|---|
| PWM to MOSFET gate | D9 | via a ~220Ω gate resistor |
| ACS712 OUT | A0 | via an RC filter R=1 kΩ, C=10 µF on OUT before A0 |
| LCD SDA / SCL | A4 / A5 | I2C, address 0x27 (see `pins.h` if yours is 0x3F) |
| Encoder CLK | D2 (INT0) | |
| Encoder DT | D4 | |
| Encoder SW (click) | D5 | INPUT_PULLUP |
| Start/Stop button | D6 | INPUT_PULLUP |
| 16 MHz crystal | XTAL1/XTAL2 | + 2×22 pF to GND - **only for `atmega328_external_xtal`**, not needed on the breadboard (`atmega328_internal_rc`) |

Power stage:
- MOSFET - logic-level N-channel (e.g. IRLZ44N/IRLB8721).
  Gate ← D9 via a ~220Ω resistor; plus a 10 kΩ pull-down Gate→GND (so the
  pump does not turn on while the MCU is unprogrammed/resetting). Source →
  common GND. Drain → pump negative.
- Pump positive → +12V **through the ACS712** (the sensor must see all of the
  pump current).
- Flyback diode 1N5819/1N4007 across the pump: cathode to +12V, anode to the
  MOSFET drain - protects the switch from the inductive spike.

Why the ACS712 needs an RC filter: with low-side PWM the current through the
sensor is chopped (it flows only while the MOSFET is on). The filter averages
the PWM ripple on the ACS712 analog output before the ADC; the firmware also
divides the measured average current by the duty cycle to get a comparable
estimate of the "effective" motor current regardless of the current duty.

## Controls

- **Encoder rotation** - changes the active parameter (PWM duty or max
  current threshold), in steps of 5% / 50 mA.
- **Encoder click** - switches which parameter the encoder adjusts. The
  active parameter is marked with `*` on screen (`D*` - duty, `M*` - current).
- **Start/Stop button** - turns the pump on/off. After the overcurrent
  protection trips, the first press only clears the latch; the second press
  starts the pump again.
- Settings (duty, current threshold) are stored in EEPROM.

## Display (16×2)

```
Line1: H2OI: 412mA  72     <state><current>[pressure, H2O only]
Line2: D*050%M 0800mA      <duty><* active><current limit>
```

States: `STOP` (pump off / no current), `AIR ` (pumping air), `H2O `
(pumping water, pressure 0-100 units on the right), `OVER` (max current
protection tripped).

The same data is written to Serial (9600 baud) 5 times per second - handy in
Serial Monitor/Plotter during calibration.

## Threshold calibration (required!)

The thresholds in [src/pump_state.h](src/pump_state.h) are rough starting
values. Every pump has its own current curve, so tune them empirically for
your pump:

1. Assemble the stand and run the pump **without water** (dry / air) at a
   medium duty. Note the typical `measuredMa`/`effectiveMa` in Serial - this
   is the upper bound of the idle mode → `CURRENT_AIR_MAX_MA`.
2. Put the inlet in water with the outlet open (minimum pressure). Note the
   current - this is the lower bound of "pumping water" →
   `CURRENT_WATER_MIN_MA` (usually equal to `CURRENT_AIR_MAX_MA` or slightly
   higher).
3. Gradually close the outlet and watch the current rise, without exceeding
   the configured `MaxCurrent`. The current near full closure is the top of
   the pressure scale (it corresponds to the `MaxCurrent` threshold you set
   with the encoder on the real stand for your pump).
4. Put the values into `CURRENT_IDLE_MAX_MA` / `CURRENT_AIR_MAX_MA` /
   `CURRENT_WATER_MIN_MA` in [src/pump_state.h](src/pump_state.h) and
   reflash.

## Build and flash

The project has two environments (see `platformio.ini`) with the same code:

- **`atmega328_internal_rc`** - no crystal, internal 8 MHz RC oscillator.
  For a first check on a breadboard - no need to solder the crystal and its
  22 pF caps, an ISP programmer is enough.
- **`atmega328_external_xtal`** - production build with an external 16 MHz
  crystal (see the pinout above). Switch to it once the breadboard circuit is
  confirmed and you move to permanent assembly.

Switching environments changes the fuses (clock source), and fuses are
flashed separately from the program - write them once via ISP when you
switch:

```bash
pio run -e atmega328_internal_rc -t fuses    # once when choosing the environment
pio run -e atmega328_internal_rc -t upload   # upload the program
pio device monitor                           # serial log for calibration (9600 baud)
```

Same for production with `-e atmega328_external_xtal` (`-t fuses` first,
once when first flashing the chip with the crystal, then regular
`-t upload`).

Both environments flash via an ISP programmer (`usbasp`) by default. If you
use the Optiboot bootloader and a USB-UART adapter instead of ISP for
production, change `upload_protocol`/`upload_speed` to `arduino`/`57600` in
`[env:atmega328_external_xtal]` (the fuses must then already be written,
e.g. when the bootloader was flashed).

## License

MIT - see [LICENSE](LICENSE).

Development is assisted by [Claude Code](https://claude.com/claude-code) (Anthropic).

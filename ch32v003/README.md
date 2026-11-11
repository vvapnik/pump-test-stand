# 12V Pump Test Stand - CH32V003F4P6 version

A port of the main firmware (see `../platformio.ini` for the ATmega328
version) to the CH32V003F4P6 (TSSOP-20). For the pinout and SWIO/NRST safety
notes see [../ch32v003-pin-check/README.md](../ch32v003-pin-check/README.md).

## Differences from the ATmega328 version

- **Settings are not persisted across power cycles.** Duty/MaxCurrent are
  not in EEPROM (there is none as such) but plain constants at startup.
  Flash persistence is not implemented - it can be added later via
  `ch32v00x_flash.h` if needed.
- **The encoder and buttons are polled on a timer** (every 10 ms), not
  interrupt-driven - simpler, and reliable enough for turning the encoder by
  hand.
- **No serial log** - on the CH32V003 that needs separate UART1 setup
  (default TX=PD5, RX=PD6), not wired up yet.
- **Integer arithmetic everywhere** (no float) - the CH32V003 (RV32EC core)
  has no hardware FPU, and software float emulation is costly on 16 KB of
  flash (~3-4 KB extra). Current and effective current are computed in whole
  milliamps/millivolts.

## Encoder controls

A "select → edit → commit" scheme (one encoder button, no long press):

1. **Select.** By default the cursor (`>` on screen) sits on one of three
   parameters - Duty → MaxCurrent → RunTime in a loop - and turning just
   moves the cursor; values do not change.
2. **Click** - enter edit mode for the selected parameter; the marker
   changes to `*`.
3. **Turning** in edit mode changes the value (steps of 5% / 50 mA / 5 s).
   RunTime=0 means "timed auto-stop disabled" (`OFF` on screen).
4. **Click again** - commits the value and returns to select mode (marker
   back to `>`); keep turning to move to another parameter.

Line 2 always shows Duty, and the second slot shows MaxCurrent **or**
RunTime - whichever was selected last (a third value does not fit in 16
characters).

The Start/Stop button is independent of this logic and works as before. On
start, overcurrent protection is disabled for the first ~0.5 s
(`STARTUP_GRACE_MS` in `main.c`) - otherwise the motor's inrush current
during spin-up would immediately trip as an overload. If RunTime is set, the
pump stops by itself when it runs out (a normal stop, no latch, same as the
button).

## I2C LCD

The LCD1602 driver (`lcd1602.c`) is hand-written on top of a minimal I2C1
master (`i2c_master.c`) using SPL registers directly - there is no Arduino
`Wire`/`LiquidCrystal_I2C` on this MCU. If the module does not respond at
`0x27` (see `lcd1602.h`), try `0x3F`.

## Build and flash via Arduino Uno (Ardulink)

Programmer wiring (the Uno acts as an SWIO bridge running the Ardulink
firmware - https://gitlab.com/BlueSyncLine/arduino-ch32v003-swio; if the Uno
does not have it yet, flash it first with the regular Arduino IDE/CLI):

| Uno | → | CH32V003F4P6 |
|---|---|---|
| Digital pin 8 (PB0) | via a ~1 kΩ resistor | pin 18 (PD1/SWIO) |
| GND | direct | GND (shared with the stand's 5V circuit) |

The chip is powered from the stand's own 5V (same as the LCD), **not** from
the Uno; Ardulink's power-control pin (Uno pin 9) is not used here.

```bash
pio run                                    # build -> .pio/build/ch32v003f4p6_evt_r0/firmware.bin

ls /dev/cu.*                               # find the Uno port (usually /dev/cu.usbmodemXXXX)

minichlink -C ardulink -c /dev/cu.XXXX \
  -w .pio/build/ch32v003f4p6_evt_r0/firmware.bin flash -b
```

`pio run -t upload` is configured too (`upload_protocol = minichlink`), but
PlatformIO does not pass `-C ardulink -c <port>` automatically - calling
`minichlink` manually as above is more reliable.

Current flash usage: ~46% (7.5 KB of 16 KB) - there is room for a serial log
and/or settings persistence if needed.

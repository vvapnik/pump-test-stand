# CH32V003F4P6 pin check before building the stand

Judging by the "SSOP-20" marking in the listing, this is almost certainly a
**CH32V003F4P6** in a **TSSOP-20** package (sellers mix up SSOP/TSSOP/"SOP-20"
all the time; what matters is that the pinout and pin numbers are the same -
only the lead pitch differs, not the pin functions). Check the marking on the
chip itself under a magnifier if it is readable - it should be something like
`CH32V003F4P6`.

## Pin table for this project

| Phys. pin | Name | Role in the stand |
|---|---|---|
| 9 | VDD | +5V |
| 7 | VSS | GND |
| **4** | **PD7** | **NRST - do not touch** |
| **18** | **PD1** | **SWIO - do not touch (except for flashing/debugging)** |
| 5 | PA1 | ADC ← ACS712 OUT |
| 1 | PD4 | Encoder SW |
| 2 | PD5 | Encoder DT |
| 3 | PD6 | Encoder CLK |
| 11 | PC1 | I2C1 SDA → LCD (fixed pin, no remap) |
| 12 | PC2 | I2C1 SCL → LCD (fixed pin, no remap) |
| 14 | PC4 | PWM → MOSFET gate |
| 19 | PD2 | Start/Stop button |

Free pins (unused, spare): PA2(6), PD3(20), PD0(8), PC0(10), PC3(13),
PC5(15), PC6(16), PC7(17). The encoder is now entirely on PD4/PD5/PD6
(pins 1-3) - moved from the original PC0/PC3/PD0, physically easier to route
in one corner next to NRST. PD5/PD6 are also the default UART1 TX/RX pins -
if a serial log is needed later, the USART can be remapped to other pins via
AFIO.

**Why pins 4 and 18 are the most dangerous.** SWIO (pin 18, PD1) is the only
wire the programmer uses to talk to the chip. If this pin is mistakenly
configured as a regular GPIO and something is connected to it (e.g. bridged
to a neighbouring lead while soldering), you can lose contact with the
programmer - recoverable by shorting PD1 to GND at power-up and reconnecting
the debugger, but better not to touch it at all. NRST (pin 4, PD7) is the
hardware reset; the procedure below does not drive it as a GPIO either.

## Check procedure

1. **Unpowered.** With a multimeter in continuity mode, from each pad on the
   adapter to the corresponding package lead (the one actually soldered),
   make sure that: a) there are no shorts between neighbouring leads
   (especially important at 0.65 mm pitch) and b) above all between
   VDD(9)/VSS(7) - a power/ground short before first power-up is what can
   really burn the chip. Also check that VDD and VSS are not shorted to
   anything other than their own nets.
2. **Apply power (5V), no firmware yet.** Measure VDD(9) against VSS(7) - it
   should be ~5V. If it is much lower, there is a parasitic load/short
   somewhere; find it before flashing.
3. **Flash this firmware** (`pio run -t upload`, see `platformio.ini` -
   switch `upload_protocol` to your option: WCH-LinkE or
   Arduino+Ardulink/minichlink). If flashing succeeds without complaining
   about the SWIO link, pins 18/9/7/4 (SWIO/VDD/VSS/NRST) are already
   confirmed working by the successful flash itself.
4. **Check the other 8 pins one by one.** The firmware toggles all used pins
   (5,10,11,12,13,14,8,19) in sync, ~2 times per second, 0V↔5V. With a
   multimeter (DC voltage) or an LED with a ~1 kΩ resistor to GND, touch each
   physical pin in turn and make sure you see 0↔5V toggling on that exact pin
   (not a neighbour - at 0.65 mm pitch the probe slips easily).
5. If a pin does **not** toggle, or toggles out of phase with the others, it
   is most likely a bad/missing joint right there; resolder that pin and
   repeat step 4 before building the rest of the stand.

## Next

Once all 8 working pins are confirmed (plus power/SWIO/NRST), move on to the
full stand firmware on the CH32V003 (PWM on TIM1, ADC, I2C to the LCD,
encoder/button handling) in the main project folder.

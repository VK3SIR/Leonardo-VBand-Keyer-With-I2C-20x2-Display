# I2C Level Shifter — Do You Need One?

## Short answer

**5V Pro Micro (the standard blue board) — no level shifter needed.** The
ATmega32U4 runs at 5V and so does the PCF8574 I2C backpack on virtually every
20×4 LCD module sold. Pin 2 (SDA) and Pin 3 (SCL) connect directly with no
components in between.

**3.3V / 8 MHz Pro Micro — a bidirectional level shifter is required.** The
I2C bus is open-drain, which means either side can pull the line low, so a
simple resistor divider won't work — you need true bidirectional translation.
The standard solution is a BSS138 N-channel MOSFET circuit, one transistor per
signal line (SDA and SCL), with 10 kΩ pullups on both the 3.3V and 5V sides.

---

## 5V Pro Micro — direct connection

```
Pro Micro (5V)          PCF8574 LCD Backpack (5V)
──────────────          ─────────────────────────
VCC (5V)  ────────────  VCC
GND       - - - - - -   GND
Pin 2 SDA ════════════  SDA
Pin 3 SCL ════════════  SCL
```

Same voltage on both sides — no level converter required.

---

## 3.3V Pro Micro — BSS138 bidirectional level shifter

```
Pro Micro (3.3V)      Level Shifter Module         PCF8574 LCD Backpack (5V)
────────────────      ──────────────────────────   ─────────────────────────
VCC (3.3V) ────────── LV    HV ──────────────────  VCC  (5V from RAW pin)
GND        - - - - -  GND  GND  - - - - - - - - -  GND
Pin 2 SDA  ══════════ LV1  HV1 ════════════════════ SDA
Pin 3 SCL  ══════════ LV2  HV2 ════════════════════ SCL
RAW (5V)   ────────── HV supply
```

### Why a simple resistor divider won't work

I2C is an open-drain bus — both the controller and the peripheral can pull
either line LOW. A resistor divider only works in one direction (high-to-low),
so it cannot translate signals that originate on the 5V (high-voltage) side.
A MOSFET-based bidirectional shifter handles current flowing either way.

### BSS138 MOSFET channel — how it works

Each signal line (SDA, SCL) gets its own BSS138 N-channel MOSFET:

```
3.3V ──┤10kΩ├──┬── Source          Drain ──┤10kΩ├── 5V
                │                   │
               LV signal          HV signal
                │                   │
                └─── Gate ──────────┘
                     (common)
```

- When the LV side pulls the signal LOW, the gate voltage drops, turning the
  MOSFET on and pulling the HV side LOW as well.
- When the HV side pulls the signal LOW, the body diode conducts, pulling the
  LV side LOW and turning the MOSFET on to complete the path.
- When neither side drives, both pullup resistors hold their respective lines
  HIGH at their own voltage level.

---

## Components for the 3.3V build

| Component | Detail |
|-----------|--------|
| BSS138 level shifter module | SparkFun BOB-12009 or HiLetgo equivalent (~$2). Includes MOSFETs and pullup resistors — no extra parts needed. |
| 10 kΩ pullup resistors × 4 | Only needed if building from bare BSS138 chips. Two to 3.3V on the LV side; two to 5V on the HV side. |
| 5V source for HV rail | Use the **RAW** pin on the 3.3V Pro Micro. It carries unregulated USB 5V directly and can drive the LCD VCC and HV rail simultaneously. |

> **Tip:** A pre-built BSS138 breakout board is by far the easiest option. Four
> wires in on the LV side, four wires out on the HV side — done.

---

## Wiring summary

| Wire | Colour convention | Notes |
|------|-------------------|-------|
| Power | Orange / red | 3.3V to LV; 5V (RAW) to HV and LCD VCC |
| GND | Black / dashed | Common ground across all three boards |
| SDA | Blue | LV1 → HV1 through the shifter |
| SCL | Green | LV2 → HV2 through the shifter |

---

## Summary

| Pro Micro variant | Level shifter needed? |
|-------------------|-----------------------|
| 5V / 16 MHz (standard blue board) | **No** — connect directly |
| 3.3V / 8 MHz | **Yes** — BSS138 bidirectional module |

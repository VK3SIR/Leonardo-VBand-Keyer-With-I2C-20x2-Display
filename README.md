# VBand HID Paddle Keyer + Morse Decoder

A firmware for the **SparkFun Pro Micro** (ATmega32U4) that turns any standard iambic morse paddle into a plug-and-play USB HID interface for [VBand](https://vband.in) and other browser-based CW trainers — while simultaneously decoding everything you send to a **20×4 I2C character LCD** in real time.

No drivers. No custom software on the PC. Just plug in, focus your browser tab, and key away.

---

## Features

- **USB HID keyboard interface** — the Pro Micro appears to the operating system as a standard keyboard. Each dit and dah presses and releases a configurable key (default: Left Ctrl / Right Ctrl), exactly as VBand's hardware paddle interface expects.
- **Iambic keyer** — full Mode A / Mode B support with correct squeeze behaviour and inter-element spacing.
- **Real-time LCD decoder** — decoded characters scroll across a 20×4 display with auto line-wrap and scroll. Prosigns are shown in `<angle brackets>` so they are unambiguous at a glance.
- **Adaptive or fixed speed** — run at a fixed WPM you set, or let the firmware learn your sending speed automatically from your own dits.
- **Sidetone** — optional 700 Hz piezo tone on pin 9, toggled on or off via serial command.
- **Fully configurable over serial** — speed, mode, dit/dah key mappings, sidetone and HID output are all set with simple `CFG:` text commands and saved to EEPROM so settings survive power-off.
- **HID safety switch** — suppress all keyboard output with `CFG:HID=OFF` while wiring or testing, without losing decoder or sidetone functionality.

---

## Supported Morse Characters

### Letters & Digits
Full ITU A–Z and 0–9.

### Punctuation
| Symbol | Name | Pattern |
|--------|------|---------|
| `.` | Full stop | `.-.-.-` |
| `,` | Comma | `--..--` |
| `?` | Question mark | `..--..` |
| `'` | Apostrophe | `.----.` |
| `!` | Exclamation | `-.-.--` |
| `/` | Fraction bar | `-..-.` |
| `(` | Open parenthesis | `-.--. ` |
| `)` | Close parenthesis | `-.--.-` |
| `&` | Ampersand | `.-...` |
| `:` | Colon | `---...` |
| `;` | Semicolon | `-.-.-.` |
| `=` | Equals / double dash | `-...-` |
| `+` | Plus | `.-.-.` |
| `-` | Hyphen | `-....-` |
| `_` | Underscore | `..--.-` |
| `"` | Quotation mark | `.-..-.` |
| `$` | Dollar sign | `...-..-` |
| `@` | At sign | `.--.- .` |

### Amateur Radio Prosigns
Prosigns are decoded and displayed as `<XX>` to distinguish them from individual letters that share the same pattern. Where a prosign has an identical pattern to a punctuation character above (AR = `+`, BT = `=`, KN = `(`), the punctuation symbol is shown.

| Display | Common name | Pattern |
|---------|-------------|---------|
| `<CT>` | Start of transmission (KA) | `-.-.-` |
| `<SK>` | End of contact (VA) | `...-.-` |
| `<AR>` | End of message / Over | `.-.-.` |
| `<BK>` | Break | `-...-.-` |
| `<KN>` | Go ahead (specific station) | `-.--.` |
| `<AS>` | Wait / Stand by | `.-...` |
| `<SN>` | Understood (VE) | `...-. ` |
| `<CL>` | Closing station | `-.-..-` |
| `<HH>` | Error / correction | `........` |
| `<SOS>` | Distress | `...---...` |
| `<AA>` | New line | `.-.-` |
| `<AL>` | All after | `.-.-..` |
| `<DO>` | Change to wideband | `-..---` |
| `<NJ>` | Shift to Wabun code | `--.---` |

---

## Hardware

### Parts List

| Component | Notes |
|-----------|-------|
| SparkFun Pro Micro (5V/16MHz) | Elite-C or any ATmega32U4 Pro Micro clone also works |
| 20×4 I2C character LCD | With PCF8574 I2C backpack (very common, ~$5) |
| Iambic morse paddle | Any standard dual-lever paddle |
| Piezo buzzer (passive) | Optional sidetone, ~$1 |
| Short lengths of hookup wire | |

### Wiring

```
Pro Micro Pin   Function          Connect to
─────────────   ────────────────  ──────────────────────────────
Pin 15          Paddle DIT        Dit lever contact; other lever contact → GND
Pin 14          Paddle DAH        Dah lever contact; other lever contact → GND
Pin 9  (PWM)    Sidetone          Passive piezo buzzer (+); buzzer (−) → GND
Pin 2  (SDA)    I2C LCD data      LCD SDA
Pin 3  (SCL)    I2C LCD clock     LCD SCL
VCC  (5V)       LCD power         LCD VCC
GND             Common ground     LCD GND, paddle GND, buzzer GND
```

Paddle inputs use the ATmega32U4's internal pull-up resistors — **no external resistors are needed**. Connect one contact of each paddle lever to the pin, the other contact to any GND pin.

> **3.3V Pro Micro users:** select the 3.3V/8MHz processor in the IDE. Most PCF8574-based LCD modules operate correctly at 3.3V.

---

## Software Setup

### Arduino IDE Board Selection

**Option A — SparkFun board definition (recommended)**

1. Open *File → Preferences* and add this URL to Additional Boards Manager URLs:
   ```
   https://raw.githubusercontent.com/sparkfun/Arduino_Boards/master/IDE_Support/package_sparkfun_index.json
   ```
2. Open *Tools → Board → Boards Manager*, search **SparkFun**, install *SparkFun AVR Boards*.
3. Select *Tools → Board → SparkFun AVR Boards → SparkFun Pro Micro*.
4. Select *Tools → Processor → ATmega32U4 (5V, 16 MHz)* (or 3.3V/8MHz as appropriate).

**Option B — Arduino Leonardo definition**

Select *Tools → Board → Arduino AVR Boards → Arduino Leonardo*. The Leonardo and Pro Micro share the same ATmega32U4 silicon; all HID, I2C and EEPROM features are identical. Upload works correctly as long as you have the right COM port selected.

### Required Libraries

Install via *Tools → Manage Libraries*:

| Library | Search term |
|---------|-------------|
| LiquidCrystal I2C | `LiquidCrystal I2C` (by Frank de Brabander) |
| Wire | Built in — no install needed |
| Keyboard | Built in — no install needed |
| EEPROM | Built in — no install needed |

### Upload

1. Open `vband_paddle_keyer_promicro.ino` in the Arduino IDE.
2. Select the correct board and port.
3. Click **Upload**.

> **If the COM port disappears after a bad upload:** double-tap the RST pin to GND rapidly to enter the 8-second bootloader window, then immediately click Upload in the IDE.

---

## Using with VBand

1. Flash the sketch and plug the Pro Micro into your PC via USB.
2. Open [https://vband.in](https://vband.in) in your browser.
3. Click into the VBand practice area so the browser tab has keyboard focus.
4. Start keying — VBand will receive Left Ctrl (dit) and Right Ctrl (dah) key events from the Pro Micro just as if you had pressed them on a keyboard.

The LCD will simultaneously display the characters you are sending, scrolling upward as lines fill.

> **Tip:** make sure no other application intercepts Ctrl key combinations while you are using VBand. If your OS or browser steals a Ctrl chord, reassign the dit/dah keys to something neutral (e.g. `CFG:DITKEY=F13`) — see configuration below.

---

## Serial Configuration

Connect a serial terminal (Arduino Serial Monitor, PuTTY, screen, etc.) at **9600 baud** to the Pro Micro's USB COM port. Send any of the following commands, terminated with a newline. Commands are **case insensitive**.

### Speed

| Command | Effect |
|---------|--------|
| `CFG:WPM=20` | Set fixed speed (5–60 WPM) |
| `CFG:WPM=AUTO` | Enable adaptive speed — the keyer learns from your actual dit length |

In AUTO mode the keyer tracks a smoothed moving average of your most recent dits. It adjusts continuously, so you can speed up or slow down naturally and the timing follows you.

### Keyer Mode

| Command | Effect |
|---------|--------|
| `CFG:MODE=B` | Iambic Mode B (default) — memory latches the opposite paddle squeeze during an element |
| `CFG:MODE=A` | Iambic Mode A — no memory; current paddle state is read at end of each element |

### Key Mappings

| Command | Effect |
|---------|--------|
| `CFG:DITKEY=LCTRL` | Set the HID key sent for dit (default: Left Ctrl) |
| `CFG:DAHKEY=RCTRL` | Set the HID key sent for dah (default: Right Ctrl) |
| `CFG:KEYS` | Print all valid key names |

**Valid key names:**

```
LCTRL  RCTRL  LSHIFT  RSHIFT  LALT  RALT  LGUI  RGUI  SPACE
A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
0 1 2 3 4 5 6 7 8 9
```

### Other Settings

| Command | Effect |
|---------|--------|
| `CFG:TONE=ON` | Enable 700 Hz sidetone on pin 9 |
| `CFG:TONE=OFF` | Disable sidetone |
| `CFG:HID=ON` | Enable HID keyboard output (default) |
| `CFG:HID=OFF` | Disable HID keyboard output — keyer and LCD still work |
| `CFG:STATUS` | Print all current settings |
| `CFG:SAVE` | Save current settings to EEPROM (persists across power cycles) |
| `CFG:HELP` | Print command reference |

### Example Session

```
CFG:WPM=AUTO
OK: speed = AUTO

CFG:MODE=B
OK: keyer mode = B

CFG:DITKEY=LCTRL
OK: DITKEY = LCTRL

CFG:DAHKEY=RCTRL
OK: DAHKEY = RCTRL

CFG:TONE=ON
OK: sidetone ON

CFG:SAVE
OK: configuration saved

CFG:STATUS
---- Status ----
Speed mode : AUTO
Current WPM: 18.4
Keyer mode : B
Sidetone   : ON
HID output : ON
Dit key    : LCTRL
Dah key    : RCTRL
-----------------
```

---

## LCD Display

On power-up the display shows a splash screen with current speed, key assignments and HID status for two seconds, then clears ready for decoding.

Decoded characters appear left-to-right across all four rows. Word spaces insert a space character. When the bottom row fills, all rows scroll up one line and the new row is written at the bottom. Prosigns appear in `<angle brackets>` (e.g. `<SK>`, `<CT>`, `<SOS>`) so they are immediately distinguishable from letter sequences.

If the LCD address `0x27` does not work, try `0x3F` — change `LCD_I2C_ADDR` near the top of the sketch.

---

## ⚠️ Safety Note

Once HID output is enabled, the Pro Micro sends **real keyboard events** to the host PC — any window with focus will receive them, not just VBand. Avoid keying while a text editor, terminal or other keyboard-sensitive application has focus, or disable HID output first with `CFG:HID=OFF`.

---

## Customisation

All user-adjustable settings are `#define` constants near the top of the sketch:

| Constant | Default | Purpose |
|----------|---------|---------|
| `PIN_DIT` | `15` | Paddle dit input pin |
| `PIN_DAH` | `14` | Paddle dah input pin |
| `PIN_SIDETONE` | `9` | Sidetone piezo pin |
| `LCD_I2C_ADDR` | `0x27` | I2C address of LCD backpack |
| `LCD_COLS` | `20` | LCD column count |
| `LCD_ROWS` | `4` | LCD row count |
| `SIDETONE_FREQ_HZ` | `700` | Sidetone pitch in Hz |
| `MIN_DIT_MS` | `20` | Fastest speed (~60 WPM) |
| `MAX_DIT_MS` | `240` | Slowest speed (~5 WPM) |
| `DEFAULT_DIT_KEY` | `KEY_LEFT_CTRL` | Power-on dit key |
| `DEFAULT_DAH_KEY` | `KEY_RIGHT_CTRL` | Power-on dah key |

---

## Licence

Released under the [MIT Licence](LICENSE). Use freely in personal and amateur radio projects. 73 de OM.

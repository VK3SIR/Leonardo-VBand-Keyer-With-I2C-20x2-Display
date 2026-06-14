/*
 * ===========================================================================
 *  VBand HID Paddle Keyer + 20x4 LCD Morse Decoder
 *  for SparkFun Pro Micro (ATmega32U4, 5V/16MHz or 3.3V/8MHz)
 *  *** Also compatible with Elite-C and any ATmega32U4 Pro Micro clone ***
 * ===========================================================================
 *
 *  WHAT THIS DOES
 *  ---------------------------------------------------------------------
 *  1. Reads a standard iambic morse paddle (dit / dah levers).
 *  2. Runs an iambic keyer (Mode A or Mode B) to turn paddle squeezes
 *     into proper dits/dahs with correct spacing.
 *  3. Acts as a USB HID keyboard (the Pro Micro's ATmega32U4 has native
 *     USB) and presses/releases a configurable key for each dit and dah:
 *        - DEFAULT: Left Ctrl  = dit
 *        - DEFAULT: Right Ctrl = dah
 *     This matches how VBand (and similar browser-based CW trainers)
 *     expect a hardware paddle to behave - the browser tab just needs
 *     to be focused and it will "hear" the key-down/key-up events as
 *     paddle presses.
 *  4. Independently decodes the same paddle input into characters and
 *     scrolls them across a 20x4 I2C character LCD.
 *  5. Sending speed (WPM) can be:
 *        - FIXED  : set a constant WPM via serial command
 *        - AUTO   : the keyer continuously learns your sending speed
 *                    from the length of the dits you actually send
 *     The dit/dah keys, keyer mode, sidetone and speed are all
 *     configurable over USB-serial and stored in EEPROM.
 *
 *  BOARD SELECTION IN ARDUINO IDE
 *  ---------------------------------------------------------------------
 *   Tools -> Board -> SparkFun AVR Boards -> SparkFun Pro Micro
 *   Tools -> Processor:
 *     - "ATmega32U4 (5V, 16 MHz)"  for the standard 5V blue Pro Micro
 *     - "ATmega32U4 (3.3V, 8 MHz)" for the 3.3V variant
 *   (Install SparkFun boards via Board Manager URL:
 *    https://raw.githubusercontent.com/sparkfun/Arduino_Boards/master/IDE_Support/package_sparkfun_index.json)
 *
 *   Alternatively use: Tools -> Board -> Arduino AVR -> Arduino Leonardo
 *   The Leonardo and Pro Micro share the same ATmega32U4 chip; all HID
 *   and I2C features work identically. The Leonardo board definition
 *   uploads correctly to the Pro Micro if you use the right COM port.
 *
 *  PRO MICRO PINOUT & WIRING
 *  ---------------------------------------------------------------------
 *  The Pro Micro has two rows of pins along its edges. Pin numbers
 *  below refer to the DIGITAL pin numbers silkscreened on the board.
 *
 *  +---------------------------+
 *  |  FUNCTION   | PRO MICRO   |
 *  +-------------+-------------+
 *  |  Paddle DIT | Pin 15      |  -> other paddle contact to GND
 *  |  Paddle DAH | Pin 14      |  -> other paddle contact to GND
 *  |  Sidetone   | Pin 9  (PWM)|  -> piezo buzzer -> GND
 *  |  LCD VCC    | VCC (5V)    |
 *  |  LCD GND    | GND         |
 *  |  LCD SDA    | Pin 2 (SDA) |  I2C data
 *  |  LCD SCL    | Pin 3 (SCL) |  I2C clock
 *  +-------------+-------------+
 *
 *  Pin 2 (SDA) and Pin 3 (SCL) are the hardware I2C pins on the
 *  Pro Micro - these are the same physical ATmega32U4 pins (PD1/PD0)
 *  that the Wire library uses automatically.
 *
 *  Pin 9 is one of the PWM-capable pins that supports tone() on the
 *  Pro Micro (Timer 1). Pins 5, 6, 9, 10 all support tone().
 *
 *  Paddle inputs use INPUT_PULLUP - no external resistors needed.
 *  Connect one paddle contact to the pin, the other contact to GND.
 *
 *  NOTE ON 3.3V PRO MICRO:
 *  If using the 3.3V/8MHz variant, the LCD backlight and I2C pullups
 *  must be compatible with 3.3V logic. Most common 20x4 I2C LCD
 *  modules with a PCF8574 backpack run fine at 3.3V.
 *
 *  LIBRARIES REQUIRED
 *  ---------------------------------------------------------------------
 *   - Wire        (built in)
 *   - Keyboard    (built in, part of Arduino's HID core for 32U4 boards)
 *   - LiquidCrystal_I2C  (by Frank de Brabander / johnrickman -
 *     install via Library Manager, search "LiquidCrystal I2C")
 *
 *  *** IMPORTANT SAFETY NOTE ***
 *  ---------------------------------------------------------------------
 *  Once HID output is enabled, this board sends real Ctrl key presses
 *  to whatever computer it's plugged into and whatever window/app has
 *  focus - not just a browser running VBand. While testing/wiring this
 *  up, you may want to disable HID output with:
 *
 *      CFG:HID=OFF
 *
 *  ...so you can use the serial monitor safely. The keyer and LCD
 *  decoder keep working normally either way; only the actual key
 *  presses sent to the PC are suppressed. Re-enable with CFG:HID=ON.
 *
 *  *** PRO MICRO BOOTLOADER / UPLOAD NOTE ***
 *  ---------------------------------------------------------------------
 *  The Pro Micro does not have a physical reset button exposed in the
 *  same way as the Leonardo. If the sketch gets into a bad state and
 *  the COM port disappears, double-tap the RST pin to GND quickly to
 *  enter the 8-second bootloader window, then immediately click Upload
 *  in the IDE.
 *
 *  SERIAL CONFIGURATION COMMANDS
 *  ---------------------------------------------------------------------
 *  Open a serial terminal at 9600 baud and send one of these lines
 *  (terminated with newline). Commands are case insensitive.
 *
 *    CFG:WPM=20        -> set fixed sending speed to 20 WPM
 *    CFG:WPM=AUTO      -> enable adaptive ("dynamic") speed tracking
 *    CFG:MODE=A        -> iambic keyer Mode A
 *    CFG:MODE=B        -> iambic keyer Mode B (default)
 *    CFG:TONE=ON       -> enable sidetone on pin 9
 *    CFG:TONE=OFF      -> disable sidetone
 *    CFG:HID=ON        -> enable sending key presses to the PC (default)
 *    CFG:HID=OFF       -> disable sending key presses (keyer/LCD still work)
 *    CFG:DITKEY=<name> -> set the key sent for "dit" (default LCTRL)
 *    CFG:DAHKEY=<name> -> set the key sent for "dah" (default RCTRL)
 *    CFG:KEYS          -> list valid key names for DITKEY/DAHKEY
 *    CFG:STATUS        -> print current configuration
 *    CFG:SAVE          -> save current configuration to EEPROM
 *    CFG:HELP          -> print this command list
 *
 *  Valid key names (see CFG:KEYS): LCTRL, RCTRL, LSHIFT, RSHIFT, LALT,
 *  RALT, LGUI, RGUI, SPACE, and the letters A-Z / digits 0-9.
 *
 * ===========================================================================
 */

#include <Wire.h>
#include <Keyboard.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// ---------------------------------------------------------------------
// User-adjustable constants
// ---------------------------------------------------------------------
// Pro Micro pin assignments (see wiring table in header comment above)
#define PIN_DIT        15   // Paddle dit  -> Pin 15 -> GND
#define PIN_DAH        14   // Paddle dah  -> Pin 14 -> GND
#define PIN_SIDETONE   9    // Piezo buzzer -> Pin 9  -> GND (PWM/Timer1)

// I2C LCD is on hardware I2C pins 2 (SDA) and 3 (SCL) - wired directly,
// no pin define needed; Wire uses them automatically on the Pro Micro.
#define LCD_I2C_ADDR   0x27     // common addresses: 0x27 or 0x3F
#define LCD_COLS       20
#define LCD_ROWS       4

#define SERIAL_BAUD    9600

#define SIDETONE_FREQ_HZ 700

// Speed limits for adaptive ("dynamic") mode, expressed as dit length
// in milliseconds. 1200 / WPM = dit length in ms.
#define MIN_DIT_MS     20.0f    // ~60 WPM
#define MAX_DIT_MS     240.0f   // ~5 WPM

// Default HID keys for the VBand interface
#define DEFAULT_DIT_KEY  KEY_LEFT_CTRL
#define DEFAULT_DAH_KEY  KEY_RIGHT_CTRL

// ---------------------------------------------------------------------
// EEPROM layout
// ---------------------------------------------------------------------
#define EE_MAGIC_ADDR     0
#define EE_MAGIC_VALUE    0x43
#define EE_AUTOSPEED_ADDR 1   // 1 byte: 0 = fixed, 1 = auto
#define EE_DITMS_ADDR     2   // float (4 bytes)
#define EE_MODE_ADDR      6   // 1 byte: 0 = Mode A, 1 = Mode B
#define EE_TONE_ADDR      7   // 1 byte: 0 = off, 1 = on
#define EE_HID_ADDR       8   // 1 byte: 0 = off, 1 = on
#define EE_DITKEY_ADDR    9   // 1 byte: HID keycode for dit
#define EE_DAHKEY_ADDR    10  // 1 byte: HID keycode for dah

// ---------------------------------------------------------------------
// Globals: configuration
// ---------------------------------------------------------------------
enum KeyerMode { MODE_A = 0, MODE_B = 1 };

bool      autoSpeed   = false;   // true = dynamic/adaptive speed
float     currentDitMs = 60.0f;  // 1200/20 = 60ms -> 20 WPM default
KeyerMode keyerMode    = MODE_B;
bool      sidetoneOn   = true;
bool      hidEnabled   = true;

uint8_t   ditKeyCode = DEFAULT_DIT_KEY;
uint8_t   dahKeyCode = DEFAULT_DAH_KEY;

// ---------------------------------------------------------------------
// LCD
// ---------------------------------------------------------------------
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
char  lcdBuf[LCD_ROWS][LCD_COLS + 1];
int   curRow = 0;
int   curCol = 0;

// ---------------------------------------------------------------------
// Morse code table
// ---------------------------------------------------------------------
// 'display' is the string printed on the LCD.
// For single characters it is one char + NUL.
// For prosigns it is the conventional abbreviation wrapped in < >
// e.g. "<AR>" so the operator can see it was a prosign, not letters.
// ---------------------------------------------------------------------
struct MorseEntry {
  const char* display; // text shown on LCD / serial
  const char* code;    // dit/dah pattern
};

const MorseEntry MORSE_TABLE[] = {

  // --- Letters ---
  {"A", ".-"},    {"B", "-..."},  {"C", "-.-."},  {"D", "-.."},
  {"E", "."},     {"F", "..-."},  {"G", "--."},   {"H", "...."},
  {"I", ".."},    {"J", ".---"},  {"K", "-.-"},   {"L", ".-.."},
  {"M", "--"},    {"N", "-."},    {"O", "---"},   {"P", ".--."},
  {"Q", "--.-"},  {"R", ".-."},   {"S", "..."},   {"T", "-"},
  {"U", "..-"},   {"V", "...-"},  {"W", ".--"},   {"X", "-..-"},
  {"Y", "-.--"},  {"Z", "--.."},

  // --- Digits ---
  {"0", "-----"}, {"1", ".----"}, {"2", "..---"}, {"3", "...--"},
  {"4", "....-"}, {"5", "....."}, {"6", "-...."}, {"7", "--..."},
  {"8", "---.."}, {"9", "----."},

  // --- Punctuation ---
  // ITU / internationally agreed punctuation marks
  {".",  ".-.-.-"},   // Full stop (period)
  {",",  "--..--"},   // Comma
  {"?",  "..--.."},   // Question mark
  {"'",  ".----."},   // Apostrophe
  {"!",  "-.-.--"},   // Exclamation mark  (KW)
  {"/",  "-..-."},    // Fraction bar / forward slash
  {"(",  "-.--."},    // Open parenthesis  (KN open)
  {")",  "-.--.-"},   // Close parenthesis
  {"&",  ".-..."},    // Ampersand / Wait (AS)
  {":",  "---..."},   // Colon
  {";",  "-.-.-."},   // Semicolon
  {"=",  "-...-"},    // Double dash / equals (BT)
  {"+",  ".-.-."},    // Plus sign  (AR without the prosign flag)
  {"-",  "-....-"},   // Hyphen / minus
  {"_",  "..--.-"},   // Underscore
  {"\"", ".-..-."},   // Quotation mark
  {"$",  "...-..-"},  // Dollar sign
  {"@",  ".--.-."},   // At sign  (AC)

  // --- Amateur Radio Prosigns ---
  // Displayed as <XX> on the LCD to distinguish them from individual
  // letters with identical patterns (e.g. AR vs A then R).
  // The keyer sends them as a single unbroken run (no inter-element
  // space inside) which is exactly what iambic mode produces naturally
  // when the paddle input is timed correctly.
  //
  // NOTE: some prosigns share a pattern with a punctuation mark above
  // (AR = "+", BT = "=", KN = "(").  Those are listed under
  // punctuation above and are NOT duplicated here to keep the table
  // unambiguous for the decoder.  The prosign equivalents that have NO
  // single-character counterpart are listed below.
  //
  {"<SK>",  "...-.-"},   // End of contact  (VA / SK)
  {"<SN>",  "...-."},    // Understood  (SN / VE - also written <VE>)
  {"<SOS>", "...---..."}, // Distress signal
  {"<HH>",  "........"}, // Error / correction  (8 dots)
  {"<AS>",  ".-..."},    // Wait / Stand by  (same as & above;
                          // decoder will match & first - operators
                          // should be aware both map to the same pattern)
  {"<CT>",  "-.-.-"},    // Start of transmission  (CT / KA)
  {"<CL>",  "-.-..-"},   // Closing station
  {"<DO>",  "-..---"},   // Change to wideband working (DO)
  {"<AA>",  ".-.-"},     // New line (used in Morse telegraphy)
  {"<AL>",  ".-.-.."},   // All after (used with repeats)
  {"<AR>",  ".-.-."},    // End of message / over  (same as + above;
                          // decoder will match + first)
  {"<BK>",  "-...-.-"},  // Break  (BK)
  {"<KN>",  "-.--."},    // Go ahead, specific station only (same as ()
  {"<NJ>",  "--.---"},   // Shift to Wabun code
};
const int MORSE_TABLE_LEN = sizeof(MORSE_TABLE) / sizeof(MorseEntry);

// ---------------------------------------------------------------------
// HID key name <-> keycode table (for DITKEY/DAHKEY configuration)
// ---------------------------------------------------------------------
struct KeyNameEntry {
  const char* name;
  uint8_t     code;
};

const KeyNameEntry KEY_NAME_TABLE[] = {
  {"LCTRL",  KEY_LEFT_CTRL},
  {"RCTRL",  KEY_RIGHT_CTRL},
  {"LSHIFT", KEY_LEFT_SHIFT},
  {"RSHIFT", KEY_RIGHT_SHIFT},
  {"LALT",   KEY_LEFT_ALT},
  {"RALT",   KEY_RIGHT_ALT},
  {"LGUI",   KEY_LEFT_GUI},
  {"RGUI",   KEY_RIGHT_GUI},
  {"SPACE",  ' '},
  {"A", 'a'}, {"B", 'b'}, {"C", 'c'}, {"D", 'd'}, {"E", 'e'},
  {"F", 'f'}, {"G", 'g'}, {"H", 'h'}, {"I", 'i'}, {"J", 'j'},
  {"K", 'k'}, {"L", 'l'}, {"M", 'm'}, {"N", 'n'}, {"O", 'o'},
  {"P", 'p'}, {"Q", 'q'}, {"R", 'r'}, {"S", 's'}, {"T", 't'},
  {"U", 'u'}, {"V", 'v'}, {"W", 'w'}, {"X", 'x'}, {"Y", 'y'},
  {"Z", 'z'},
  {"0", '0'}, {"1", '1'}, {"2", '2'}, {"3", '3'}, {"4", '4'},
  {"5", '5'}, {"6", '6'}, {"7", '7'}, {"8", '8'}, {"9", '9'}
};
const int KEY_NAME_TABLE_LEN = sizeof(KEY_NAME_TABLE) / sizeof(KeyNameEntry);

// Returns true and fills 'code' if 'name' is a recognised key name.
bool lookupKeyCode(const String &name, uint8_t &code) {
  for (int i = 0; i < KEY_NAME_TABLE_LEN; i++) {
    if (name.equals(KEY_NAME_TABLE[i].name)) {
      code = KEY_NAME_TABLE[i].code;
      return true;
    }
  }
  return false;
}

// Returns a human-readable name for a keycode (for CFG:STATUS).
String keyCodeToName(uint8_t code) {
  for (int i = 0; i < KEY_NAME_TABLE_LEN; i++) {
    if (KEY_NAME_TABLE[i].code == code) {
      return String(KEY_NAME_TABLE[i].name);
    }
  }
  return String("0x") + String(code, HEX);
}

// =======================================================================
// Iambic keyer state machine
// =======================================================================
enum KeyerState { KEYER_IDLE, KEYER_SENDING, KEYER_SPACING };

KeyerState     keyerState   = KEYER_IDLE;
bool           sendingDit   = false;  // true if current/last element is a dit
unsigned long  elementStart = 0;
unsigned long  elementDuration = 0;

bool ditMemory = false;
bool dahMemory = false;

bool    keyIsDown   = false;
uint8_t activeHidKey = 0; // which HID key is currently "pressed"

// ---------------------------------------------------------------------
// Decoder state
// ---------------------------------------------------------------------
char symbolBuffer[16];
int  symbolLen = 0;

unsigned long keyDownTime = 0;
unsigned long lastKeyUpTime = 0;
bool          haveLastKeyUp = false;

// Adaptive speed smoothing
float adaptiveDitMs = 60.0f;

// ---------------------------------------------------------------------
// Serial config command buffer
// ---------------------------------------------------------------------
String cmdBuffer = "";

// =======================================================================
// Setup
// =======================================================================
void setup() {
  pinMode(PIN_DIT, INPUT_PULLUP);
  pinMode(PIN_DAH, INPUT_PULLUP);
  pinMode(PIN_SIDETONE, OUTPUT);
  digitalWrite(PIN_SIDETONE, LOW);

  Serial.begin(SERIAL_BAUD);

  Keyboard.begin();

  Wire.begin();
  lcd.init();
  lcd.backlight();
  clearDisplay();

  loadConfig();

  adaptiveDitMs = currentDitMs;

  showSplash();
}

// =======================================================================
// Main loop
// =======================================================================
void loop() {
  handleSerialConfig();
  updateKeyer();
  checkDecoderTimeouts();
}

// =======================================================================
// ----------------------- KEYER (paddle -> CW) -------------------------
// =======================================================================

bool ditPressed() { return digitalRead(PIN_DIT) == LOW; }
bool dahPressed() { return digitalRead(PIN_DAH) == LOW; }

void updateKeyer() {
  unsigned long now = millis();
  unsigned long ditMs = (unsigned long)currentDitMs;
  unsigned long dahMs = ditMs * 3UL;
  unsigned long spaceMs = ditMs; // inter-element space = 1 dit

  switch (keyerState) {

    case KEYER_IDLE: {
      if (ditPressed()) {
        startElement(true, now);
      } else if (dahPressed()) {
        startElement(false, now);
      }
      break;
    }

    case KEYER_SENDING: {
      // capture paddle squeeze memory while the current element is keyed
      if (sendingDit && dahPressed())  dahMemory = true;
      if (!sendingDit && ditPressed()) ditMemory = true;

      unsigned long dur = sendingDit ? ditMs : dahMs;
      if (now - elementStart >= dur) {
        endElement(now);
        keyerState = KEYER_SPACING;
        elementStart = now;
        elementDuration = spaceMs;
      }
      break;
    }

    case KEYER_SPACING: {
      if (now - elementStart >= elementDuration) {
        bool nextIsDit;
        bool haveNext = decideNextElement(nextIsDit);
        if (haveNext) {
          startElement(nextIsDit, now);
        } else {
          keyerState = KEYER_IDLE;
          ditMemory = false;
          dahMemory = false;
        }
      }
      break;
    }
  }
}

// Decide what to send next based on current paddle state + memory.
// Returns true if there is a next element, and sets nextIsDit accordingly.
bool decideNextElement(bool &nextIsDit) {
  bool dit = ditPressed();
  bool dah = dahPressed();

  if (keyerMode == MODE_B) {
    if (sendingDit) {
      // last element was a dit
      if (dahMemory || dah) { dahMemory = false; nextIsDit = false; return true; }
      if (dit)              { nextIsDit = true;  return true; }
    } else {
      // last element was a dah
      if (ditMemory || dit) { ditMemory = false; nextIsDit = true;  return true; }
      if (dah)              { nextIsDit = false; return true; }
    }
  } else { // MODE_A - no memory, just look at current paddle state
    if (sendingDit) {
      if (dah) { nextIsDit = false; return true; }
      if (dit) { nextIsDit = true;  return true; }
    } else {
      if (dit) { nextIsDit = true;  return true; }
      if (dah) { nextIsDit = false; return true; }
    }
  }
  return false;
}

void startElement(bool isDit, unsigned long now) {
  sendingDit = isDit;
  elementStart = now;
  keyerState = KEYER_SENDING;
  keyDown(now, isDit);
}

void endElement(unsigned long now) {
  keyUp(now);
}

// =======================================================================
// ------------------ KEY EVENTS: VBand (HID) + decoder ------------------
// =======================================================================

void keyDown(unsigned long now, bool isDit) {
  if (keyIsDown) return;
  keyIsDown = true;

  // VBand HID output: press the configured key for dit or dah
  activeHidKey = isDit ? ditKeyCode : dahKeyCode;
  if (hidEnabled) {
    Keyboard.press(activeHidKey);
  }

  // Sidetone
  if (sidetoneOn) tone(PIN_SIDETONE, SIDETONE_FREQ_HZ);

  // ---- Decoder: handle the gap since the previous key-up ----
  keyDownTime = now;
  if (haveLastKeyUp) {
    unsigned long gap = now - lastKeyUpTime;
    handleGap(gap);
  }
}

void keyUp(unsigned long now) {
  if (!keyIsDown) return;
  keyIsDown = false;

  // VBand HID output: release the key that was pressed
  if (hidEnabled && activeHidKey != 0) {
    Keyboard.release(activeHidKey);
  }
  activeHidKey = 0;

  // Sidetone
  if (sidetoneOn) noTone(PIN_SIDETONE);

  // ---- Decoder: classify the element we just sent ----
  unsigned long dur = now - keyDownTime;
  classifyElement(dur);

  lastKeyUpTime = now;
  haveLastKeyUp = true;
}

// =======================================================================
// ------------------------------ DECODER --------------------------------
// =======================================================================

// Classify a completed element (key-down duration) as dit or dah and
// append it to the current symbol buffer. Also feeds the adaptive
// speed estimator.
void classifyElement(unsigned long durationMs) {
  float ref = currentDitMs;

  bool isDit = (float)durationMs < (2.0f * ref);

  if (symbolLen < (int)sizeof(symbolBuffer) - 1) {
    symbolBuffer[symbolLen++] = isDit ? '.' : '-';
    symbolBuffer[symbolLen] = '\0';
  }

  // Adaptive speed: only learn from elements that look like dits,
  // since dahs are defined relative to the dit length anyway.
  if (isDit) {
    adaptiveDitMs = adaptiveDitMs * 0.8f + (float)durationMs * 0.2f;
    if (adaptiveDitMs < MIN_DIT_MS) adaptiveDitMs = MIN_DIT_MS;
    if (adaptiveDitMs > MAX_DIT_MS) adaptiveDitMs = MAX_DIT_MS;

    if (autoSpeed) {
      currentDitMs = adaptiveDitMs;
    }
  }
}

// Handle the silence gap that occurred BEFORE a new key-down (or that
// has elapsed with the key still up - see checkDecoderTimeouts()).
// Decides whether the pending symbol buffer represents a finished
// letter, and/or whether a word space should be inserted.
void handleGap(unsigned long gapMs) {
  float ref = currentDitMs;

  // Roughly: <1.5 dits  -> still inside the same letter (ignore)
  //          1.5-5 dits -> letter space  -> flush current letter
  //          >5 dits    -> word space    -> flush letter + add space
  if (gapMs < (unsigned long)(1.5f * ref)) {
    return; // normal inter-element space, nothing to do
  }

  flushLetter();

  if (gapMs >= (unsigned long)(5.0f * ref)) {
    appendToDisplay(' ');
  }
}

// Called from loop() so that the last letter / word-space of a
// transmission is displayed even if the operator stops sending and
// never presses the paddle again.
void checkDecoderTimeouts() {
  if (keyIsDown) return;          // currently sending an element
  if (!haveLastKeyUp) return;     // nothing sent yet
  if (symbolLen == 0) return;     // nothing pending to flush

  unsigned long gap = millis() - lastKeyUpTime;
  float ref = currentDitMs;

  if (gap >= (unsigned long)(1.5f * ref)) {
    flushLetter();
  }
  if (gap >= (unsigned long)(5.0f * ref)) {
    appendToDisplay(' ');
    haveLastKeyUp = false; // avoid repeatedly inserting spaces
  }
}

// Look up symbolBuffer in the morse table and print the matching
// display string (or "?" if unrecognised), then clear the buffer.
void flushLetter() {
  if (symbolLen == 0) return;

  const char* decoded = "?";
  for (int i = 0; i < MORSE_TABLE_LEN; i++) {
    if (strcmp(symbolBuffer, MORSE_TABLE[i].code) == 0) {
      decoded = MORSE_TABLE[i].display;
      break;
    }
  }

  appendStringToDisplay(decoded);

  symbolLen = 0;
  symbolBuffer[0] = '\0';
}

// Print each character of a string to the LCD (handles multi-char
// prosign labels like "<SK>" transparently).
void appendStringToDisplay(const char* s) {
  while (*s) {
    appendToDisplay(*s++);
  }
}

// =======================================================================
// ------------------------------- LCD ------------------------------------
// =======================================================================

void clearDisplay() {
  lcd.clear();
  for (int r = 0; r < LCD_ROWS; r++) {
    for (int c = 0; c < LCD_COLS; c++) lcdBuf[r][c] = ' ';
    lcdBuf[r][LCD_COLS] = '\0';
  }
  curRow = 0;
  curCol = 0;
  lcd.setCursor(0, 0);
}

void appendToDisplay(char c) {
  // Collapse repeated leading spaces at the start of the very first line
  if (c == ' ' && curRow == 0 && curCol == 0) return;

  lcdBuf[curRow][curCol] = c;
  lcd.setCursor(curCol, curRow);
  lcd.print(c);

  curCol++;
  if (curCol >= LCD_COLS) {
    curCol = 0;
    curRow++;
    if (curRow >= LCD_ROWS) {
      scrollUp();
      curRow = LCD_ROWS - 1;
    }
    lcd.setCursor(curCol, curRow);
  }
}

void scrollUp() {
  for (int r = 0; r < LCD_ROWS - 1; r++) {
    memcpy(lcdBuf[r], lcdBuf[r + 1], LCD_COLS + 1);
  }
  for (int c = 0; c < LCD_COLS; c++) lcdBuf[LCD_ROWS - 1][c] = ' ';
  lcdBuf[LCD_ROWS - 1][LCD_COLS] = '\0';

  lcd.clear();
  for (int r = 0; r < LCD_ROWS; r++) {
    lcd.setCursor(0, r);
    lcd.print(lcdBuf[r]);
  }
}

void showSplash() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("VBand HID Keyer"));
  lcd.setCursor(0, 1);
  if (autoSpeed) {
    lcd.print(F("Speed: AUTO"));
  } else {
    lcd.print(F("Speed: "));
    lcd.print(wpmFromDitMs(currentDitMs), 1);
    lcd.print(F(" WPM"));
  }
  lcd.setCursor(0, 2);
  lcd.print(F("Dit="));
  lcd.print(keyCodeToName(ditKeyCode));
  lcd.print(F(" Dah="));
  lcd.print(keyCodeToName(dahKeyCode));
  lcd.setCursor(0, 3);
  lcd.print(hidEnabled ? F("HID: ON  Ready...") : F("HID: OFF Ready..."));

  delay(2000);
  clearDisplay();
}

// =======================================================================
// --------------------------- CONFIGURATION ------------------------------
// =======================================================================

float wpmFromDitMs(float ditMs) {
  return 1200.0f / ditMs;
}

float ditMsFromWpm(float wpm) {
  return 1200.0f / wpm;
}

void loadConfig() {
  uint8_t magic = EEPROM.read(EE_MAGIC_ADDR);
  if (magic != EE_MAGIC_VALUE) {
    // No valid config saved yet - use defaults already set above.
    return;
  }

  uint8_t autoByte = EEPROM.read(EE_AUTOSPEED_ADDR);
  autoSpeed = (autoByte == 1);

  float ditMs;
  EEPROM.get(EE_DITMS_ADDR, ditMs);
  if (ditMs >= MIN_DIT_MS && ditMs <= MAX_DIT_MS) {
    currentDitMs = ditMs;
  }

  uint8_t modeByte = EEPROM.read(EE_MODE_ADDR);
  keyerMode = (modeByte == 0) ? MODE_A : MODE_B;

  uint8_t toneByte = EEPROM.read(EE_TONE_ADDR);
  sidetoneOn = (toneByte != 0);

  uint8_t hidByte = EEPROM.read(EE_HID_ADDR);
  hidEnabled = (hidByte != 0);

  uint8_t storedDit = EEPROM.read(EE_DITKEY_ADDR);
  uint8_t storedDah = EEPROM.read(EE_DAHKEY_ADDR);
  if (storedDit != 0xFF) ditKeyCode = storedDit;
  if (storedDah != 0xFF) dahKeyCode = storedDah;
}

void saveConfig() {
  EEPROM.write(EE_MAGIC_ADDR, EE_MAGIC_VALUE);
  EEPROM.write(EE_AUTOSPEED_ADDR, autoSpeed ? 1 : 0);
  EEPROM.put(EE_DITMS_ADDR, currentDitMs);
  EEPROM.write(EE_MODE_ADDR, keyerMode == MODE_A ? 0 : 1);
  EEPROM.write(EE_TONE_ADDR, sidetoneOn ? 1 : 0);
  EEPROM.write(EE_HID_ADDR, hidEnabled ? 1 : 0);
  EEPROM.write(EE_DITKEY_ADDR, ditKeyCode);
  EEPROM.write(EE_DAHKEY_ADDR, dahKeyCode);
}

// Read characters from Serial, build up lines, and act on any line
// that starts with "CFG:". All other bytes are ignored by this parser.
void handleSerialConfig() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    if (c == '\r') continue;

    if (c == '\n') {
      if (cmdBuffer.length() > 0) {
        processCommandLine(cmdBuffer);
      }
      cmdBuffer = "";
    } else {
      cmdBuffer += c;
      if (cmdBuffer.length() > 40) {
        cmdBuffer = ""; // avoid runaway buffer on noise
      }
    }
  }
}

void processCommandLine(String line) {
  line.trim();
  String upper = line;
  upper.toUpperCase();

  if (!upper.startsWith("CFG:")) {
    return; // not a configuration command - ignore
  }

  String body = upper.substring(4); // after "CFG:"

  if (body.startsWith("WPM=")) {
    String val = body.substring(4);
    if (val == "AUTO") {
      autoSpeed = true;
      Serial.println(F("OK: speed = AUTO"));
    } else {
      float wpm = val.toFloat();
      if (wpm >= wpmFromDitMs(MAX_DIT_MS) && wpm <= wpmFromDitMs(MIN_DIT_MS) && wpm > 0) {
        autoSpeed = false;
        currentDitMs = ditMsFromWpm(wpm);
        adaptiveDitMs = currentDitMs;
        Serial.print(F("OK: speed = "));
        Serial.print(wpm, 1);
        Serial.println(F(" WPM (fixed)"));
      } else {
        Serial.println(F("ERR: WPM out of range (try 5-60)"));
      }
    }
  }
  else if (body.startsWith("MODE=")) {
    String val = body.substring(5);
    if (val == "A") {
      keyerMode = MODE_A;
      Serial.println(F("OK: keyer mode = A"));
    } else if (val == "B") {
      keyerMode = MODE_B;
      Serial.println(F("OK: keyer mode = B"));
    } else {
      Serial.println(F("ERR: MODE must be A or B"));
    }
  }
  else if (body.startsWith("TONE=")) {
    String val = body.substring(5);
    if (val == "ON") {
      sidetoneOn = true;
      Serial.println(F("OK: sidetone ON"));
    } else if (val == "OFF") {
      sidetoneOn = false;
      noTone(PIN_SIDETONE);
      Serial.println(F("OK: sidetone OFF"));
    } else {
      Serial.println(F("ERR: TONE must be ON or OFF"));
    }
  }
  else if (body.startsWith("HID=")) {
    String val = body.substring(4);
    if (val == "ON") {
      hidEnabled = true;
      Serial.println(F("OK: HID output ON"));
    } else if (val == "OFF") {
      // make sure we don't leave a key stuck down
      if (keyIsDown && activeHidKey != 0) {
        Keyboard.release(activeHidKey);
      }
      hidEnabled = false;
      Serial.println(F("OK: HID output OFF"));
    } else {
      Serial.println(F("ERR: HID must be ON or OFF"));
    }
  }
  else if (body.startsWith("DITKEY=")) {
    String val = body.substring(7);
    uint8_t code;
    if (lookupKeyCode(val, code)) {
      ditKeyCode = code;
      Serial.print(F("OK: DITKEY = "));
      Serial.println(val);
    } else {
      Serial.println(F("ERR: unknown key name (try CFG:KEYS)"));
    }
  }
  else if (body.startsWith("DAHKEY=")) {
    String val = body.substring(7);
    uint8_t code;
    if (lookupKeyCode(val, code)) {
      dahKeyCode = code;
      Serial.print(F("OK: DAHKEY = "));
      Serial.println(val);
    } else {
      Serial.println(F("ERR: unknown key name (try CFG:KEYS)"));
    }
  }
  else if (body == "KEYS") {
    printKeyNames();
  }
  else if (body == "STATUS") {
    printStatus();
  }
  else if (body == "SAVE") {
    saveConfig();
    Serial.println(F("OK: configuration saved"));
  }
  else if (body == "HELP") {
    printHelp();
  }
  else {
    Serial.println(F("ERR: unknown command (try CFG:HELP)"));
  }
}

void printStatus() {
  Serial.println(F("---- Status ----"));
  Serial.print(F("Speed mode : "));
  Serial.println(autoSpeed ? F("AUTO") : F("FIXED"));
  Serial.print(F("Current WPM: "));
  Serial.println(wpmFromDitMs(currentDitMs), 1);
  Serial.print(F("Keyer mode : "));
  Serial.println(keyerMode == MODE_A ? F("A") : F("B"));
  Serial.print(F("Sidetone   : "));
  Serial.println(sidetoneOn ? F("ON") : F("OFF"));
  Serial.print(F("HID output : "));
  Serial.println(hidEnabled ? F("ON") : F("OFF"));
  Serial.print(F("Dit key    : "));
  Serial.println(keyCodeToName(ditKeyCode));
  Serial.print(F("Dah key    : "));
  Serial.println(keyCodeToName(dahKeyCode));
  Serial.println(F("-----------------"));
}

void printKeyNames() {
  Serial.println(F("Valid key names:"));
  Serial.println(F("  LCTRL RCTRL LSHIFT RSHIFT LALT RALT LGUI RGUI SPACE"));
  Serial.println(F("  A-Z  0-9"));
}

void printHelp() {
  Serial.println(F("CFG:WPM=<5-60>   set fixed speed"));
  Serial.println(F("CFG:WPM=AUTO     enable adaptive speed"));
  Serial.println(F("CFG:MODE=A|B     iambic keyer mode"));
  Serial.println(F("CFG:TONE=ON|OFF  sidetone (pin 9)"));
  Serial.println(F("CFG:HID=ON|OFF   enable/disable HID key output"));
  Serial.println(F("CFG:DITKEY=<key> set HID key sent for dit"));
  Serial.println(F("CFG:DAHKEY=<key> set HID key sent for dah"));
  Serial.println(F("CFG:KEYS         list valid key names"));
  Serial.println(F("CFG:STATUS       show current settings"));
  Serial.println(F("CFG:SAVE         save settings to EEPROM"));
  Serial.println(F("CFG:HELP         this help text"));
}

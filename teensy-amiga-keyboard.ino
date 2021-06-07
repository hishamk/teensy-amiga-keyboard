/*
   Copyright (c) 2021 Hisham Khalifa <hisham@binarybakery.com>.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, version 3.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

/*   _
  _ // Teensy 2.0-based
  \X/  AMIGA Keyboard Converter

   Allows modern computers to use Amiga keyboards as regular USB keyboards.

   The following sources for an Arduino Leonardo version were consulted during the development of this code:
   https://forum.arduino.cc/t/amiga-500-1000-2000-keyboard-interface/136052/81
   https://forum.arduino.cc/t/amiga-500-1000-2000-keyboard-interface/136052/96
   https://github.com/BrainSlugs83/Amiga500-Keyboard/blob/main/AmigaKeyboard.ino

   And of course, the Amiga Hardware Reference Manual and an oscilloscope for sanity checks.

   Tested with Teensy 2.0 and US Amiga 3000 keyboard model KKQ-E94YC.

   Note that not only is the data line active low, so is the clock line.

   I have not included joystick support since it's not something I'm going to use.

*/

#define KB_CLK PIN_F6 // Clock PIN_F6 = 17
#define KB_DATA PIN_F7  // Serial data PIN_F7 = 16
#define HANDSHAKE_PULSE_LENGTH 100 // microseconds
#define MACOS_CTRL_RELEASE_DELAY 100 // milliseconds

int nClock;
int pClock;
int bitCursor;
uint8_t dataBits;
byte keyCode = -1;
bool isUp;
bool handShakeRequired;

// US Amiga 3000 Keyboard mapping
static const unsigned int keyLUT[104] = {
  KEY_TILDE, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,
  KEY_MINUS, KEY_EQUAL, KEY_BACKSLASH, 0, KEYPAD_0, KEY_Q, KEY_W, KEY_E, KEY_R,
  KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P, KEY_LEFT_BRACE, KEY_RIGHT_BRACE, 0,
  KEYPAD_1, KEYPAD_2, KEYPAD_3, KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J,
  KEY_K, KEY_L, KEY_SEMICOLON, KEY_QUOTE, 0,  0, KEYPAD_4, KEYPAD_5, KEYPAD_6,
  0, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_PERIOD,
  KEY_SLASH, 0, KEYPAD_PERIOD, KEYPAD_7, KEYPAD_8, KEYPAD_9, KEY_SPACE, KEY_BACKSPACE,
  KEY_TAB, KEYPAD_ENTER, KEY_RETURN, KEY_ESC, KEY_DELETE, 0, 0, 0, KEYPAD_MINUS, 0,
  KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_LEFT, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
  KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_LEFT_BRACE, KEY_RIGHT_BRACE, KEYPAD_SLASH, KEYPAD_ASTERIX, KEYPAD_PLUS,
  KEY_SYSTEM_WAKE_UP, KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT, KEY_CAPS_LOCK, KEY_LEFT_CTRL, // Help key set to KEY_SYSTEM_WAKE_UP
  KEY_RIGHT_ALT, KEY_LEFT_ALT, KEY_RIGHT_GUI, KEY_LEFT_GUI
};

void setup() {
//  Serial.begin(9600);
  pinMode(KB_CLK, INPUT);
  pinMode(KB_DATA, INPUT);
}

void loop() {
  static unsigned int hostKey;

  keyCode = readBits();

  if (handShakeRequired) {
    if (keyCode > -1) {
//      for (int i = 7; i >= 0; i--) {
//        Serial.print((char)('0' + ((keyCode >> i) & 1)));
//      }
//      Serial.println();
//      Serial.println(keyCode, BIN);
//      Serial.println(keyCode, HEX);
//      Serial.println();
      hostKey = keyLUT[keyCode];

      if (!isUp) {
        if (hostKey == KEY_CAPS_LOCK) {
          Keyboard.press(hostKey);
          // For macOS, CAPS LOCK press needs to linger a bit longer to register.
          // Should not delay excessively otherwise keyboard will not get handshake quick enough.
          delay(MACOS_CTRL_RELEASE_DELAY);
          Keyboard.release(hostKey);
        } else {
          Keyboard.press(hostKey);
        }
      } else {
        if (hostKey == KEY_CAPS_LOCK) {
          Keyboard.press(hostKey);
          delay(MACOS_CTRL_RELEASE_DELAY);
          Keyboard.release(hostKey);
        } else {
          Keyboard.release(hostKey);
        }
      }
    }

    dataBits = 0b00000000;
    handShakeRequired = false;
    handShake(); // hand shake should happen within 143us of the last clock transmission.
  }
}

byte readBits() {
  static const byte bitOrder[] = {6, 5, 4, 3, 2, 1, 0};

  if (bitCursor < 7 && clockPulledToLow()) {
    dataBits |= (getData() << bitOrder[bitCursor]);
    bitCursor++;
  } else if (bitCursor == 7 && clockPulledToLow()) {
    isUp = getData();
    if (isUp) {
//      Serial.println("Key UP");
    } else {
//      Serial.println("Key DOWN");
    }
    handShakeRequired = true;
    bitCursor = 0;
    return dataBits;
  }

  return -1;
}

byte handShake() {
  pinMode(KB_DATA, OUTPUT);
  digitalWrite(KB_DATA, LOW);
  delayMicroseconds(HANDSHAKE_PULSE_LENGTH);
  digitalWrite(KB_DATA, HIGH);
  pinMode(KB_DATA, INPUT);
}

bool clockPulledToLow() {
  pClock = nClock;
  nClock = digitalRead(KB_CLK);

  if (nClock == 0 && pClock == 1) {
    pClock = nClock;
    return true;
  }

  pClock = nClock;
  return false;
}

bool clockPulledToHigh() {
  pClock = nClock;
  nClock = digitalRead(KB_CLK);

  if (nClock == 1 && pClock == 0) {
    pClock = nClock;
    return true;
  }

  pClock = nClock;
  return false;
}

int getData() {
  return digitalRead(KB_DATA) ? 0 : 1; // active low 0 = on, 1 = off.
}

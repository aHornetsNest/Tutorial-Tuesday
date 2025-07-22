#define DCSBIOS_IRQ_SERIAL

#include <DcsBios.h>
#include "AHN_Gauge.h"
#include "AHN_gaugeOperation.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>

/*
  ====================================================================================
  F/A-18C Hornet Standby Altimeter and Pressure Setting Drum Display
  ====================================================================================

  This section manages the rolling drum-style digit displays for the standby altimeter and pressure setting in the F/A-18C Standby Altimeter Gauge.

  Key Parts:
  - **Structs:**
    - Drum struct holds data for each rolling drum: scaled value, previous digit, current digit

  - **Drum Variables:**
    - drum0, drum1, drum2 handle the pressure setting display (3 drums).
    - drum10k, drum1k handle the 10,000 and 1,000 ft altimeter drums.

  - **Multiplexer:**
    - tcaSelect() switches between connected OLED displays using an I2C multiplexer.

  - **Update Functions:**
    - updateDrum() converts a raw DCS BIOS value to a 0–10 range, determines the digit to show, the next digit, the scroll offset, and roll direction.
    - updateDrum2() does similar for a limited range drum (pressure set value drum 2) with specific min and max scaling.
    - updateDisplays() takes the data and projects it to the OLED screens. Contains a refresh rate limiting function to allow more processor to drive smooth stepper motors.

  ====================================================================================
*/

// ----- Constants -----
constexpr uint8_t TCAADDR = 0x70;
constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 32;
constexpr int OLED_RESET = -1;
constexpr int digitHeightPx = 20;
constexpr int digitHeightAlt = 20;
constexpr int yCenterPx = (SCREEN_HEIGHT - digitHeightPx) / 2;
constexpr int yCenterAlt = (SCREEN_HEIGHT - digitHeightPx) / 2;

// ----- Drum Struct Definitions -----
struct Drum {
  float scaledValue = 0.0;
  int currentDigit = 0;
  int lastDigit = -1;
};

// ----- Drum Creation -----
Drum drum2, drum1, drum0, drum10k, drum1k;

// ----- Multiplexer Select -----
void tcaSelect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}

// ----- Displays -----
Adafruit_SSD1306 display0(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


void displaySetup(Adafruit_SSD1306& display, uint8_t muxChannel, uint8_t rotation) {
  tcaSelect(muxChannel);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(rotation);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
}

// ----- Drum Update -----

bool display0NeedsUpdate = true;
bool display1NeedsUpdate = true;

void updateDrum(Drum& drum, unsigned int newValue, bool isPress = false) {
  drum.lastDigit = drum.currentDigit;
  if (isPress) {
    constexpr unsigned int rawMin = 26214;
    constexpr unsigned int rawMax = 65535;
    constexpr int digitMin = 28;
    constexpr int digitMax = 31;
    newValue = constrain(newValue, rawMin, rawMax);
    drum.scaledValue = digitMin + (float)(newValue - rawMin) * (digitMax - digitMin) / (rawMax - rawMin);
  } else {
    drum.scaledValue = (newValue / 65535.0f) * 10.0f;
  }
  drum.currentDigit = (int)floor(drum.scaledValue + 0.0001f);  // + 0.0001f corrects for rounding error causing use of floor error.
}

// ----- Draw Functions -----
void drawDrum(Adafruit_SSD1306& display, int x, int y, int value) {
  display.setCursor(x, y);
  display.print(value);
}

void drawPressSet2(Adafruit_SSD1306& display, int x, int y, int value) {
  int tens = value / 10;
  int ones = value % 10;
  display.setCursor(x, y);
  display.print(tens);
  display.setCursor(x + 14, y);
  display.print(ones);
}

void drawStripedPattern(Adafruit_SSD1306& display, int x, int y) {
  constexpr int patternWidth = 20;
  constexpr int patternHeight = 30;
  constexpr int stripeThickness = 4;
  constexpr int stripeSpacing = 4;
  int yCenter = y - patternHeight / 2;

  for (int offset = -patternHeight; offset < patternWidth; offset += stripeThickness + stripeSpacing) {
    for (int thicknessOffset = 0; thicknessOffset < stripeThickness; thicknessOffset++) {
      display.drawLine(x + offset + thicknessOffset, yCenter, x + offset + thicknessOffset + patternHeight, yCenter + patternHeight, SSD1306_WHITE);
    }
  }
}


// ----- DCS BIOS Callbacks -----
void onStbyPressSet0Change(unsigned int newValue) {
  int oldDigit = drum0.currentDigit;
  updateDrum(drum0, newValue);
  if (drum0.currentDigit != oldDigit) {
    display0NeedsUpdate = true;
  }
}
void onStbyPressSet1Change(unsigned int newValue) {
  int oldDigit = drum1.currentDigit;
  updateDrum(drum1, newValue);
  if (drum1.currentDigit != oldDigit) {
    display0NeedsUpdate = true;
  }
}
void onStbyPressSet2Change(unsigned int newValue) {
  int oldDigit = drum2.currentDigit;
  updateDrum(drum2, newValue, true);
  if (drum2.currentDigit != oldDigit) {
    display0NeedsUpdate = true;
  }
}

void onStbyAlt1000FtCntChange(unsigned int newValue) {
  int oldDigit = drum1k.currentDigit;
  updateDrum(drum1k, newValue);
  if (drum1k.currentDigit != oldDigit) {
    display1NeedsUpdate = true;
  }
}
void onStbyAlt10000FtCntChange(unsigned int newValue) {
  int oldDigit = drum10k.currentDigit;
  updateDrum(drum10k, newValue);
  if (drum10k.currentDigit != oldDigit) {
    display1NeedsUpdate = true;
  }
}


// ----- DCS BIOS Buffers -----
DcsBios::IntegerBuffer stbyPressSet0Buffer(FA_18C_hornet_STBY_PRESS_SET_0, onStbyPressSet0Change);
DcsBios::IntegerBuffer stbyPressSet1Buffer(FA_18C_hornet_STBY_PRESS_SET_1, onStbyPressSet1Change);
DcsBios::IntegerBuffer stbyPressSet2Buffer(FA_18C_hornet_STBY_PRESS_SET_2, onStbyPressSet2Change);
DcsBios::IntegerBuffer stbyAlt10000FtCntBuffer(FA_18C_hornet_STBY_ALT_10000_FT_CNT, onStbyAlt10000FtCntChange);
DcsBios::IntegerBuffer stbyAlt1000FtCntBuffer(FA_18C_hornet_STBY_ALT_1000_FT_CNT, onStbyAlt1000FtCntChange);

// ----- Display Update -----
unsigned long lastDisplayUpdate = 0;

void updateDisplays() {
  const unsigned long interval = 100;
  static unsigned long lastUpdate0 = 0;
  static unsigned long lastUpdate1 = 0;
  unsigned long now = millis();

  // Display 0
  if (display0NeedsUpdate && (now - lastUpdate0 >= interval)) {
    tcaSelect(0);
    display0.clearDisplay();
    drawPressSet2(display0, 3, yCenterPx + 4, drum2.currentDigit);
    drawDrum(display0, 32, yCenterPx + 4, drum1.currentDigit);
    drawDrum(display0, 48, yCenterPx + 4, drum0.currentDigit);
    display0.display();

    drum0.lastDigit = drum0.currentDigit;
    drum1.lastDigit = drum1.currentDigit;
    drum2.lastDigit = drum2.currentDigit;
    display0NeedsUpdate = false;

    lastUpdate0 = now;
  }

  // Display 1
  if (display1NeedsUpdate && (now - lastUpdate1 >= interval)) {
    tcaSelect(1);
    display1.clearDisplay();

    if (drum10k.currentDigit == 0) {
      drawStripedPattern(display1, 30, yCenterAlt + 18);
    } else {
      drawDrum(display1, 30, yCenterAlt + 8, drum10k.currentDigit);
    }

    display1.fillRect(50, 0, 60, 60, SSD1306_BLACK);
    drawDrum(display1, 70, yCenterAlt + 8, drum1k.currentDigit);
    display1.display();

    // Update last displayed digits always if an update happened
    drum10k.lastDigit = drum10k.currentDigit;
    drum1k.lastDigit = drum1k.currentDigit;

    display1NeedsUpdate = false;
    // Move this outside so it runs regardless of display1NeedsUpdate
    lastUpdate1 = now;
  }
}

/*
  ====================================================================================
  COCKPIT BACKLIGHTING CONTROL
  ====================================================================================

  This section manages the cockpit backlighting for the cockpit panels using NeoPixel 1938 LEDs.
  It handles two color modes: NVIS Green (for night vision compatibility)
  plus a Test Color mode (Red) for system testing / Calibration.

  Key Parts:
  - pixelCount and backlightIn define how many LEDs and which pin controls them.
  - Variables like brightness, nvisOn, and currentMode track the current lighting settings.
  - Color values are stored in RGB byte arrays for Green, White, and Test Color.
  - A lightMode enum allows switching between normal and test modes.

  DCS-BIOS Callbacks:
  - onCockkpitLightModeSwChange() listens to the DCS cockpit light mode switch to enable or disable NVIS mode.
  - onInstPnlDimmerChange() listens to the panel dimmer value and scales it to LED brightness (0–255).

  Functions:
  - getCurrentColor() returns the appropriate RGB color value based on the current mode and NVIS status.
  - backlightOperation() updates the LEDs' brightness and color only if there’s a change, then sends the update to the LEDs.

    ====================================================================================
*/

// ----- Backlight Config -----
#define pixelCount 6
#define backlightIn 11

// ----- Backlight Variables & Previous States -----
int brightness = 0;
bool nvisOn = false;

byte blNil[3] = { 0, 0, 0 };        // Nil Backlight
byte blGreen[3] = { 255, 0, 0 };    // NVIS Green -- GRB Ordering due Neo Pixel 1938 requirement
byte testColor[3] = { 0, 255, 0 };  // Red Test Colour -- GRB Ordering due Neo Pixel 1938 requirement

uint32_t lastColor = 0;
int lastBrightness = -1;

// ----- Lighting Mode Enum -----
enum lightMode {
  MODE_NORMAL,
  MODE_TEST
};

lightMode currentMode = MODE_NORMAL;

// ----- Neopixel 1938 Setup -----
Adafruit_NeoPixel pixels(pixelCount, backlightIn, NEO_GRB + NEO_KHZ800);

// ----- DCS BIOS Callbacks -----
void onCockkpitLightModeSwChange(unsigned int newValue) {
  nvisOn = (newValue != 0);
}
DcsBios::IntegerBuffer cockkpitLightModeSwBuffer(FA_18C_hornet_COCKKPIT_LIGHT_MODE_SW, onCockkpitLightModeSwChange);

void onInstPnlDimmerChange(unsigned int newValue) {
  brightness = map(newValue, 0, 65535, 0, 255);
}
DcsBios::IntegerBuffer instPnlDimmerBuffer(FA_18C_hornet_INST_PNL_DIMMER, onInstPnlDimmerChange);

// ----- Backlighting Colour -----
uint32_t getCurrentColor() {
  if (currentMode == MODE_TEST) {
    // Return test color (e.g., rotating value or set by test function)
    return pixels.Color(testColor[0], testColor[1], testColor[2]);
  } else {
    // Normal operation color
    if (nvisOn) {
      return pixels.Color(blGreen[0], blGreen[1], blGreen[2]);
    } else {
      return pixels.Color(blNil[0], blNil[1], blNil[2]);
    }
  }
}

// ----- Update Backlighting -----
void backlightOperation() {
  uint32_t color = getCurrentColor();

  if (brightness != lastBrightness || color != lastColor) {
    pixels.setBrightness(brightness);
    for (int i = 0; i < pixelCount; i++) {
      pixels.setPixelColor(i, color);
    }
    pixels.show();
    lastBrightness = brightness;
    lastColor = color;
  }
}

/*
  ====================================================================================
  STANDBY INSTRUMENT GAUGE AND ALTIMETER PRESSURE CONTROL MODULE
  ====================================================================================

  This section initializes and manages the standby analog gauges and controls for:
    - Vertical Velocity Indicator (VVI)
    - Standby Airspeed Indicator (ASI)
    - Standby Altimeter (ALT)

  Key Functions:
    - IMPORTANT! Coil1-4 for each motor, are to be entered as they appear on the OH electrical diagrams. 
    - "BKA-30" and "VID-29" for the motor type will re-order the pins, try "BKA-30" first, then try 
      "VID-29" if incorrect operation is detected.
    - Create Gauge instances for each instrument, specifying stepper motor parameters.
    - Handle zeroing, limits, and DCS-BIOS data callbacks.
    - ASI Step 0 is 0kts - this is the reference point for the CW angle to the homePosition & initialisedPosition
    - ALT Step 0 = 0ft - this is the reference point for the CW angle to the homePosition & initialisedPosition
    - VVI Step 0 = -6000ft - this is the reference point for the CW angle to the homePosition & initialisedPosition

  Notes:
    - Ensure zero switches and coil wiring match assigned Arduino pins.
    - Extendable for additional standby instruments using this pattern.

  ====================================================================================
*/

// ----- Gauge Instance Definitions --------------------------------------------------
// Create gauge instances — (name, stepsPerRevolution, homePosition, intialisedPosition, maxTravelAngle, minTravelAngle, zeroPin, opticalZeroingCapability, Coil1, Coil2, Coil3, Coil4, Motor Type)

// Uncomment Below if using AHN VVI Gauge Face
Gauge vvi("VVI", 720, 135.0, 170.0, 340.0, 0.0, A2, true, 34, 36, 32, 30, "BKA-30");  // home: -500fpm, init: 0fpm, max: +6000fpm, min: -6000fpm

// Uncomment Below if using OH VVI Gauge Face
// Gauge vvi("VVI", 720, 145.0, 170.0, 340.0, 0.0, A2, true, 34, 36, 32, 30, "BKA-30");  // home: -500fpm, init: 0fpm, max: +6000fpm, min: -6000fpm

Gauge asi("ASI", 720, 225.0, 0.0, 345.0, 0.0, A0, true, 50, 52, 48, 46, "BKA-30");  // home: 250 KIAS, init: 0, max: 850, min: 0 KIAS

Gauge alt("ALT", 720, 36.0, 0.0, 360.0, 0.0, A1, true, 42, 44, 38, 40, "BKA-30");  // home: 100ft, init: 0ft

int climbRate = 0;

// ----- DCS-BIOS Data Callbacks -----------------------------------------------------

void onVsiChange(unsigned int newValue) {
  updateTargetPosition(&vvi, newValue);
  climbRate = map(newValue, 0, 65535, -6000, +6000);
}
DcsBios::IntegerBuffer vsiBuffer(FA_18C_hornet_VSI, onVsiChange);

void onStbyAsiAirspeedChange(unsigned int newValue) {
  updateTargetPosition(&asi, newValue);
}
DcsBios::IntegerBuffer stbyAsiAirspeedBuffer(
  FA_18C_hornet_STBY_ASI_AIRSPEED, onStbyAsiAirspeedChange);

void onStbyAlt100FtPtrChange(unsigned int newValue) {
  updateTargetPosition(&alt, newValue);
}
DcsBios::IntegerBuffer stbyAlt100FtPtrBuffer(
  FA_18C_hornet_STBY_ALT_100_FT_PTR, onStbyAlt100FtPtrChange);

/*
  ====================================================================================
  STANDBY INSTRUMENT PILOT CONTROLS
  ====================================================================================

  Description:
    - Manages pilot controls for standby instruments.
    - Rotary encoder implementation for standby altimeter pressure setting and SAI attitude reference.
    - Potentiometer implementation for RWR brightness.

  ====================================================================================
*/

// ----- Control Pins ----------------------------------------------------------------

int sariCageSw = 22;
int sariRotSwA = 26;
int sariRotSwB = 24;
int sariTest = 28;

int rwrBrt = A3;

int altRotSwA = 12;
int altRotSwB = 13;

// ----- DCS-BIOS Control Callbacks --------------------------------------------------

DcsBios::Switch2Pos saiCage("SAI_CAGE", sariCageSw);
DcsBios::RotaryEncoder saiSet("SAI_SET", "-1600", "+1600", sariRotSwA, sariRotSwB);
DcsBios::Potentiometer rwrDmrCtrl("RWR_DMR_CTRL", rwrBrt);
DcsBios::RotaryEncoder stbyPressAlt("STBY_PRESS_ALT", "-600", "+600", altRotSwA, altRotSwB);

// ----- Test Mode and Zeroing Control -----------------------------------------------

unsigned long pressStartSariTest = 0;
unsigned long pressStartSariCage = 0;

bool testInProgress = false;
bool zeroingInProgress = false;
unsigned long testModeStartTime = 0;

bool testButtonHold(int pin, unsigned long holdTime, unsigned long& pressStart) {
  int state = digitalRead(pin);

  if (state == LOW) {
    if (pressStart == 0) pressStart = millis();

    if (millis() - pressStart >= holdTime) {
      pressStart = millis() + 10000;  // Prevent immediate retrigger for 10s or until release
      return true;
    }
  } else {
    pressStart = 0;
  }
  return false;
}

void stepperZero() {
  brightness = 255;
  backlightOperation();

  asi.zero();
  alt.zero();
  vvi.zero();
}


void setup() {
  // Initialize DCS Bios
  DcsBios::setup();

  // I2C initialize 
  Wire.begin();

  // Initialize steppers
  asi.initializeStepper();
  alt.initializeStepper();
  vvi.initializeStepper();

  // Zero Stepper Motors
  vvi.zero();
  asi.zero();
  alt.zero();

  

  // Test Buttons
  pinMode(sariCageSw, INPUT_PULLUP);
  pinMode(sariTest, INPUT_PULLUP);

  // Display Initialize
  displaySetup(display0, 0, 2);
  displaySetup(display1, 1, 0);

  pixels.begin();
  pixels.clear();
  pixels.show();
}

void loop() {
  // Run DCS Bios
  DcsBios::loop();
  updateGauge(&asi);
  updateGauge(&vvi);
  updateGauge(&alt);
  updateDisplays();

  // Check for long press to trigger test
  if (testButtonHold(sariTest, 3000, pressStartSariTest)) {
    if (currentMode != MODE_TEST) {
      currentMode = MODE_TEST;
      stepperZero();
      testModeStartTime = millis();
    }
  }

  // Stay in test mode for a short time, then return to normal
  if (currentMode == MODE_TEST) {
    if (millis() - testModeStartTime >= 1000) {  // 1 second for demo
      currentMode = MODE_NORMAL;
    }
  }

  backlightOperation();
}

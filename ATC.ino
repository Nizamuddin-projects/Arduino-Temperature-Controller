/*
  Arduino-Based Temperature Controller
  ------------------------------------
  Arduino Nano + DHT11 + I2C 16x2 LCD
  + 2 potentiometers + 2 push buttons
  + Red/White/Blue status LEDs + Power LED

  Reconstructed from the supplied project report.
  The report describes the project behavior and pin connections,
  but does not contain the original source code.
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// -------------------- Pin Definitions --------------------
#define DHT_PIN       2
#define DHT_TYPE      DHT11

#define SETPOINT_POT  A0
#define HYST_POT      A1

#define MODE_BUTTON    3
#define UNIT_BUTTON    4

#define POWER_LED      5
#define HEATING_LED    6   // Red
#define STABLE_LED     7   // White
#define COOLING_LED    8   // Blue

// -------------------- LCD / Sensor --------------------
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHT_PIN, DHT_TYPE);

// -------------------- Temperature Parameters --------------------
const float MIN_SETPOINT = 20.0;
const float MAX_SETPOINT = 40.0;

const float MIN_HYST = 0.5;
const float MAX_HYST = 5.0;

// Calibration offset; change after comparing with a trusted thermometer.
const float TEMP_OFFSET = 0.0;

// -------------------- Timing --------------------
const unsigned long SENSOR_INTERVAL = 2000UL;
const unsigned long LCD_INTERVAL = 250UL;
const unsigned long BUTTON_DEBOUNCE = 50UL;
const unsigned long POWER_BLINK_TIME = 100UL;
const unsigned long ERROR_DISPLAY_TIME = 1000UL;

// -------------------- Global Variables --------------------
float currentTempC = 25.0;
float currentHumidity = 0.0;

float setpoint = 30.0;
float hysteresis = 1.0;

bool paramMode = false;

// 0 = Celsius, 1 = Fahrenheit, 2 = Kelvin
byte unitIndex = 0;

unsigned long lastSensorRead = 0;
unsigned long lastLCDUpdate = 0;

unsigned long powerBlinkUntil = 0;
unsigned long sensorErrorMessageUntil = 0;

byte sensorErrorCount = 0;
bool haveValidReading = false;

// -------------------- Button State --------------------
bool lastModeButtonState = HIGH;
bool lastUnitButtonState = HIGH;

// ------------------------------------------------------------
// Convert Celsius to the selected temperature unit
// ------------------------------------------------------------
float convertTemperature(float tempC) {
  if (unitIndex == 1) {
    return (tempC * 9.0 / 5.0) + 32.0;   // Fahrenheit
  }

  if (unitIndex == 2) {
    return tempC + 273.15;               // Kelvin
  }

  return tempC;                          // Celsius
}

// ------------------------------------------------------------
// Return selected unit symbol
// ------------------------------------------------------------
char getUnitChar() {
  if (unitIndex == 1) return 'F';
  if (unitIndex == 2) return 'K';
  return 'C';
}

// ------------------------------------------------------------
// Blink power LED for button/error feedback
// ------------------------------------------------------------
void blinkPowerLED() {
  powerBlinkUntil = millis() + POWER_BLINK_TIME;
  digitalWrite(POWER_LED, HIGH);
}

// ------------------------------------------------------------
// Update the power LED without blocking the program
// ------------------------------------------------------------
void updatePowerLED() {
  if (millis() >= powerBlinkUntil) {
    digitalWrite(POWER_LED, LOW);
  }
}

// ------------------------------------------------------------
// Update red / white / blue control LEDs
// ------------------------------------------------------------
void updateControlLEDs() {
  if (!haveValidReading) {
    digitalWrite(HEATING_LED, LOW);
    digitalWrite(STABLE_LED, LOW);
    digitalWrite(COOLING_LED, LOW);
    return;
  }

  float lowerLimit = setpoint - hysteresis;
  float upperLimit = setpoint + hysteresis;

  if (currentTempC < lowerLimit) {
    // Heating required
    digitalWrite(HEATING_LED, HIGH);
    digitalWrite(STABLE_LED, LOW);
    digitalWrite(COOLING_LED, LOW);
  }
  else if (currentTempC > upperLimit) {
    // Cooling required
    digitalWrite(HEATING_LED, LOW);
    digitalWrite(STABLE_LED, LOW);
    digitalWrite(COOLING_LED, HIGH);
  }
  else {
    // Temperature is stable
    digitalWrite(HEATING_LED, LOW);
    digitalWrite(STABLE_LED, HIGH);
    digitalWrite(COOLING_LED, LOW);
  }
}

// ------------------------------------------------------------
// Read potentiometers and map them to project ranges
// ------------------------------------------------------------
void readParameters() {
  if (!paramMode) return;

  int setpointRaw = analogRead(SETPOINT_POT);
  int hystRaw = analogRead(HYST_POT);

  // Multiply by 100 to preserve one decimal place, as described
  // in the project report.
  long setpointScaled = map(setpointRaw, 0, 1023,
                             (long)(MIN_SETPOINT * 100),
                             (long)(MAX_SETPOINT * 100));

  long hystScaled = map(hystRaw, 0, 1023,
                        (long)(MIN_HYST * 100),
                        (long)(MAX_HYST * 100));

  setpoint = setpointScaled / 100.0;
  hysteresis = hystScaled / 100.0;
}

// ------------------------------------------------------------
// Check a button using simple 50 ms debouncing and wait-release
// ------------------------------------------------------------
bool buttonPressed(uint8_t pin, bool &lastState) {
  bool currentState = digitalRead(pin);

  if (lastState == HIGH && currentState == LOW) {
    delay(BUTTON_DEBOUNCE);

    if (digitalRead(pin) == LOW) {
      // Wait until the user releases the button.
      while (digitalRead(pin) == LOW) {
        updatePowerLED();
      }

      lastState = HIGH;
      return true;
    }
  }

  lastState = currentState;
  return false;
}

// ------------------------------------------------------------
// Handle Mode button
// ------------------------------------------------------------
void handleModeButton() {
  if (buttonPressed(MODE_BUTTON, lastModeButtonState)) {
    paramMode = !paramMode;
    blinkPowerLED();
    lcd.clear();
  }
}

// ------------------------------------------------------------
// Handle Unit button
// Unit changes only in Display Mode.
// ------------------------------------------------------------
void handleUnitButton() {
  if (buttonPressed(UNIT_BUTTON, lastUnitButtonState)) {
    if (!paramMode) {
      unitIndex++;
      if (unitIndex > 2) {
        unitIndex = 0;
      }
      lcd.clear();
    }

    blinkPowerLED();
  }
}

// ------------------------------------------------------------
// Read DHT11 every 2 seconds using millis()
// ------------------------------------------------------------
void readSensor() {
  unsigned long now = millis();

  if (now - lastSensorRead < SENSOR_INTERVAL) {
    return;
  }

  lastSensorRead = now;

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    if (sensorErrorCount < 255) {
      sensorErrorCount++;
    }

    if (sensorErrorCount >= 3) {
      sensorErrorMessageUntil = millis() + ERROR_DISPLAY_TIME;
      blinkPowerLED();
    }

    return;
  }

  // Valid reading
  currentTempC = temperature + TEMP_OFFSET;
  currentHumidity = humidity;

  sensorErrorCount = 0;
  haveValidReading = true;
}

// ------------------------------------------------------------
// Display current temperature and humidity
// ------------------------------------------------------------
void showDisplayMode() {
  float displayTemp = convertTemperature(currentTempC);

  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(displayTemp, 1);
  lcd.print((char)223);
  lcd.print(getUnitChar());
  lcd.print("   ");

  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  lcd.print(currentHumidity, 1);
  lcd.print("%   ");
}

// ------------------------------------------------------------
// Display setpoint and hysteresis
// ------------------------------------------------------------
void showParameterMode() {
  lcd.setCursor(0, 0);
  lcd.print("Set: ");
  lcd.print(setpoint, 1);
  lcd.print((char)223);
  lcd.print("C      ");

  lcd.setCursor(0, 1);
  lcd.print("Hyst: ");
  lcd.print(hysteresis, 1);
  lcd.print((char)223);
  lcd.print("C     ");
}

// ------------------------------------------------------------
// Show sensor error for one second after 3 consecutive failures
// ------------------------------------------------------------
void showSensorError() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sensor Error!");
  lcd.setCursor(0, 1);
  lcd.print("Using last value");
}

// ------------------------------------------------------------
// Refresh LCD according to current mode
// ------------------------------------------------------------
void updateLCD() {
  unsigned long now = millis();

  if (now - lastLCDUpdate < LCD_INTERVAL) {
    return;
  }

  lastLCDUpdate = now;

  if (sensorErrorMessageUntil > now) {
    showSensorError();
    return;
  }

  if (paramMode) {
    showParameterMode();
  } else {
    showDisplayMode();
  }
}

// ------------------------------------------------------------
// Arduino setup
// ------------------------------------------------------------
void setup() {
  pinMode(MODE_BUTTON, INPUT_PULLUP);
  pinMode(UNIT_BUTTON, INPUT_PULLUP);

  pinMode(POWER_LED, OUTPUT);
  pinMode(HEATING_LED, OUTPUT);
  pinMode(STABLE_LED, OUTPUT);
  pinMode(COOLING_LED, OUTPUT);

  digitalWrite(POWER_LED, LOW);
  digitalWrite(HEATING_LED, LOW);
  digitalWrite(STABLE_LED, LOW);
  digitalWrite(COOLING_LED, LOW);

  Wire.begin();

  lcd.init();
  lcd.backlight();

  dht.begin();

  // Start parameters from the physical knob positions.
  int setpointRaw = analogRead(SETPOINT_POT);
  int hystRaw = analogRead(HYST_POT);

  setpoint = map(setpointRaw, 0, 1023,
                 (long)(MIN_SETPOINT * 100),
                 (long)(MAX_SETPOINT * 100)) / 100.0;

  hysteresis = map(hystRaw, 0, 1023,
                   (long)(MIN_HYST * 100),
                   (long)(MAX_HYST * 100)) / 100.0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp Controller");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(1000);
  lcd.clear();

  // Allow the first sensor reading immediately.
  lastSensorRead = millis() - SENSOR_INTERVAL;
}

// ------------------------------------------------------------
// Main loop
// ------------------------------------------------------------
void loop() {
  handleModeButton();
  handleUnitButton();

  readParameters();
  readSensor();

  updateControlLEDs();
  updatePowerLED();
  updateLCD();
}

# Arduino-Based Temperature Controller

An Arduino Nano based temperature and humidity monitoring and control system using a DHT11 sensor, I2C 16×2 LCD, potentiometers, push buttons, and LED indicators.

## 📌 Project Overview

This project provides real-time temperature and humidity monitoring with adjustable temperature control logic.

The system uses an Arduino Nano to read temperature and humidity from a DHT11 sensor and displays the readings on an I2C 16×2 LCD.

Two potentiometers allow the user to adjust:

- Target temperature (Setpoint)
- Hysteresis (Deadband)

Two push buttons provide user control for:

- Switching between Display Mode and Parameter Mode
- Changing the temperature unit between Celsius, Fahrenheit, and Kelvin

Three LEDs provide visual status indication:

- 🔴 Red LED → Heating required
- ⚪ White LED → Temperature stable
- 🔵 Blue LED → Cooling required

A power LED provides feedback during button presses and sensor errors.

---

## ✨ Features

- Real-time temperature and humidity monitoring
- Arduino Nano based system
- DHT11 temperature and humidity sensor
- I2C 16×2 LCD display
- Adjustable temperature setpoint
- Adjustable hysteresis
- Celsius, Fahrenheit, and Kelvin display
- Display Mode and Parameter Mode
- Heating, Stable, and Cooling LED indication
- Sensor error handling
- Non-blocking sensor reading using `millis()`
- Simple button debouncing
- Temperature calibration offset
- Linear potentiometer scaling using `map()`

---

## 🧰 Components Required

| Component | Quantity | Specifications / Notes |
|---|---:|---|
| Arduino Nano | 1 | ATmega328P, 5V logic |
| DHT11 Sensor | 1 | Temperature 0–50°C, Humidity 20–90% |
| I2C LCD 16×2 | 1 | Address 0x27 or 0x3F |
| 10kΩ Potentiometers | 2 | Setpoint and hysteresis |
| 4-pin Push Buttons | 2 | Momentary, normally open |
| LEDs | 4 | Heating, Stable, Cooling and Power |
| 330Ω Resistors | 4 | Current limiting for LEDs |
| Breadboard & Jumper Wires | — | For prototyping |

---

## 🔌 Pin Connections

| Arduino Nano Pin | Connected To |
|---|---|
| D2 | DHT11 Data |
| A0 | Setpoint Potentiometer Wiper |
| A1 | Hysteresis Potentiometer Wiper |
| D3 | Mode Button |
| D4 | Unit Button |
| D5 | Power LED |
| D6 | Red LED – Heating |
| D7 | White LED – Stable |
| D8 | Blue LED – Cooling |
| A4 (SDA) | LCD SDA |
| A5 (SCL) | LCD SCL |
| GND | Common Ground |

The potentiometer outer terminals are connected to 5V and GND.

---

## ⚙️ Working Principle

### 1. Display Mode

In Display Mode, the LCD displays:

- Current temperature
- Humidity
- Selected temperature unit

The Unit button cycles through:

```text
Celsius → Fahrenheit → Kelvin

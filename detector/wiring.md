# Detector Wiring Reference
Source: detector.ino
Board: ESP32-WROOM-32 38-pin

---

## NRF24L01 — VSPI

| NRF24L01 Pin | ESP32 GPIO | Notes |
|---|---|---|
| VCC | 3.3V | **Not 5V** |
| GND | GND | |
| CE | 22 | |
| CSN | 21 | |
| SCK | 18 | |
| MOSI | 23 | |
| MISO | 19 | |

Decoupling: 100µF electrolytic across VCC/GND on module (if available)

---

## LEDs — Progressive Bar Display

Each LED wired: **ESP32 GPIO → resistor → LED (+/anode) → LED (-/cathode) → GND**

| LED Color | GPIO | Resistor | Level | Meaning |
|---|---|---|---|---|
| Green | 25 | 100Ω | 1 | Clean — no interference |
| Yellow | 26 | 220Ω | 2 | Trace interference |
| Orange | 27 | 220Ω | 3 | Moderate interference |
| Red | 32 | 220Ω | 4 | Strong interference |
| Blue | 33 | 100Ω | 5 | Heavy interference |
| Clear | 4 | 100Ω | 6 | Maximum — full jamming |

**Polarity:**
- Long leg = anode (+) → connects toward GPIO via resistor
- Short leg = cathode (−) → connects to GND

---

## Detection Thresholds (adjustable in detector.ino)

| LEDs Lit | Channels Hot | Label |
|---|---|---|
| 1 (green) | ≥ 5% | Clean |
| 2 (+ yellow) | ≥ 15% | Trace |
| 3 (+ orange) | ≥ 30% | Moderate |
| 4 (+ red) | ≥ 50% | Strong |
| 5 (+ blue) | ≥ 70% | Heavy |
| 6 (+ clear) | ≥ 85% | Jamming |

Thresholds are starting points — calibrate against live transmitter after first run.

---

## Notes

- Detector uses one NRF24L01 module only (VSPI)
- PA level set to RF24_PA_MIN — receiving only, no transmission
- LED startup sequence on boot confirms all 6 LEDs are working
- If NRF24L01 not found, all LEDs flash rapidly as error indicator
- Serial Monitor at 115200 baud for live channel readings

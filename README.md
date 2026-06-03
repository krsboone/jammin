# jammin
ESP32 Signal testing

## cap_stress_test
Used to ensure that wiring of capacitors is correct, and test for brown outs
* Hammers both NRF24L01 modules simultaneously at PA_MAX
* Reports reset reason on boot — ESP_RST_BROWNOUT means caps aren't doing enough

## nrf24_test
Tests both HSPI and VSPI modules against BlueJammer expected pin mapping

## detector
NRF24L01 Interference Detector
* Scans all 125 2.4GHz channels, measures noise floor
* Progressive 6-LED bar display shows interference level

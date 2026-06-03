// NRF24L01 Interference Detector
// Scans all 125 2.4GHz channels, measures noise floor
// Progressive 6-LED bar display shows interference level
// Open Serial Monitor at 115200 baud for live readings

#include <SPI.h>
#include <RF24.h>

// ── NRF24L01 — VSPI ──────────────────────────────────────────────
#define CE_PIN    22
#define CSN_PIN   21
#define SCK_PIN   18
#define MOSI_PIN  23
#define MISO_PIN  19

// ── LED pins — in order of severity ─────────────────────────────
#define LED_GREEN   25    // Level 1 — clean
#define LED_YELLOW  26    // Level 2 — trace
#define LED_ORANGE  27    // Level 3 — moderate
#define LED_RED     32    // Level 4 — strong
#define LED_BLUE    33    // Level 5 — heavy
#define LED_CLEAR    4    // Level 6 — maximum / full saturation

// ── Detection thresholds (% of 125 channels above -64dBm) ────────
// Adjust these after calibrating against live transmitter
#define THRESH_1  0.05    //  5% — green lights
#define THRESH_2  0.15    // 15% — + yellow
#define THRESH_3  0.30    // 30% — + orange
#define THRESH_4  0.50    // 50% — + red
#define THRESH_5  0.70    // 70% — + blue
#define THRESH_6  0.85    // 85% — all LEDs (full jamming)

// ── Config ───────────────────────────────────────────────────────
#define NUM_CHANNELS      125
#define CHANNEL_DWELL_US  200   // µs dwell per channel (min ~170µs for RPD settle)
#define REPORT_INTERVAL   500   // ms between serial reports

SPIClass vspi_bus(VSPI);
RF24 radio(CE_PIN, CSN_PIN);

const int   LED_PINS[]   = { LED_GREEN, LED_YELLOW, LED_ORANGE, LED_RED, LED_BLUE, LED_CLEAR };
const float THRESHOLDS[] = { THRESH_1,  THRESH_2,   THRESH_3,   THRESH_4, THRESH_5, THRESH_6 };

// ── Setup ────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Init LED pins
  for (int i = 0; i < 6; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  // LED startup sequence — visual confirmation all LEDs working
  Serial.println("\n=== NRF24L01 Interference Detector ===");
  Serial.println("LED test...");
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED_PINS[i], HIGH);
    delay(200);
  }
  delay(300);
  for (int i = 5; i >= 0; i--) {
    digitalWrite(LED_PINS[i], LOW);
    delay(200);
  }

  // Init radio
  vspi_bus.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CSN_PIN);
  if (!radio.begin(&vspi_bus)) {
    Serial.println("ERROR: NRF24L01 not found — check wiring!");
    // Rapid flash all LEDs as error indicator
    while (1) {
      for (int i = 0; i < 6; i++) digitalWrite(LED_PINS[i], HIGH);
      delay(150);
      for (int i = 0; i < 6; i++) digitalWrite(LED_PINS[i], LOW);
      delay(150);
    }
  }

  radio.setAutoAck(false);
  radio.setPALevel(RF24_PA_MIN);  // RX only — no need for PA power
  radio.startListening();

  Serial.println("Radio initialized.");
  Serial.println("Scanning 125 channels...\n");
  Serial.println("Channels  | Bar                  | LEDs");
  Serial.println("----------+----------------------+------");
}

// ── Scan all 125 channels, return hot channel count ─────────────
int scanChannels() {
  int hot = 0;
  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    radio.setChannel(ch);
    delayMicroseconds(CHANNEL_DWELL_US);
    if (radio.testRPD()) hot++;
  }
  return hot;
}

// ── Update LEDs based on interference ratio ───────────────────────
int updateLEDs(float ratio) {
  int ledsOn = 0;
  for (int i = 0; i < 6; i++) {
    if (ratio >= THRESHOLDS[i]) ledsOn = i + 1;
  }
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED_PINS[i], i < ledsOn ? HIGH : LOW);
  }
  return ledsOn;
}

// ── Main loop ────────────────────────────────────────────────────
uint32_t lastReport = 0;

void loop() {
  int   hotChannels = scanChannels();
  float ratio       = (float)hotChannels / NUM_CHANNELS;
  int   ledsOn      = updateLEDs(ratio);

  if (millis() - lastReport >= REPORT_INTERVAL) {
    lastReport = millis();

    // ASCII bar — 25 chars wide
    int bars = (int)(ratio * 25);
    Serial.printf("%3d/125 (%5.1f%%) |", hotChannels, ratio * 100.0);
    for (int i = 0; i < 25; i++) Serial.print(i < bars ? '#' : ' ');
    Serial.printf("| %d/6 LEDs", ledsOn);

    if      (ledsOn == 0) Serial.println("  CLEAN");
    else if (ledsOn <= 2) Serial.println("  trace");
    else if (ledsOn <= 3) Serial.println("  moderate");
    else if (ledsOn <= 4) Serial.println("  STRONG");
    else if (ledsOn <= 5) Serial.println("  HEAVY");
    else                  Serial.println("  *** JAMMING ***");
  }
}

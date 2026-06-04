// NRF24L01 Interference Detector
// Scans all 125 2.4GHz channels, measures noise floor
// Progressive 6-LED bar display shows interference level
// Open Serial Monitor at 115200 baud for live readings
//
// v2 — thresholds calibrated from log-1.log data:
//       jammer OFF: 0–5%, jammer ON: 5–10%
//       dwell increased to 500µs, rolling average over 5 scans

#include <SPI.h>
#include <RF24.h>

// ── NRF24L01 — VSPI ──────────────────────────────────────────────
#define CE_PIN    22
#define CSN_PIN   21
#define SCK_PIN   18
#define MOSI_PIN  23
#define MISO_PIN  19

// ── LED pins — in order of severity ─────────────────────────────
#define LED_GREEN   25    // Level 1 — minimal / clean
#define LED_YELLOW  26    // Level 2 — trace
#define LED_ORANGE  27    // Level 3 — moderate (jammer reliably triggers this)
#define LED_RED     32    // Level 4 — strong
#define LED_BLUE    33    // Level 5 — heavy
#define LED_CLEAR    4    // Level 6 — maximum

// ── Detection thresholds — calibrated from log-1.log ─────────────
// Jammer OFF baseline: 0–4%   Jammer ON: 5–10%
#define THRESH_1  0.02    //  2% — green (ambient noise floor)
#define THRESH_2  0.04    //  4% — + yellow (rising above baseline)
#define THRESH_3  0.06    //  6% — + orange (jammer reliably here)
#define THRESH_4  0.10    // 10% — + red
#define THRESH_5  0.20    // 20% — + blue  (closer range / stronger signal)
#define THRESH_6  0.35    // 35% — all LEDs

// ── Config ───────────────────────────────────────────────────────
#define NUM_CHANNELS      125
#define CHANNEL_DWELL_US  500   // µs dwell per channel (increased from 200)
#define AVG_WINDOW        5     // rolling average over N scans
#define REPORT_INTERVAL   500   // ms between serial reports

SPIClass vspi_bus(VSPI);
RF24 radio(CE_PIN, CSN_PIN);

const int   LED_PINS[]   = { LED_GREEN, LED_YELLOW, LED_ORANGE, LED_RED, LED_BLUE, LED_CLEAR };
const float THRESHOLDS[] = { THRESH_1,  THRESH_2,   THRESH_3,   THRESH_4, THRESH_5, THRESH_6 };

// Rolling average buffer
float avgBuffer[AVG_WINDOW] = {0};
int   avgIndex = 0;

// ── Setup ────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);

  for (int i = 0; i < 6; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  Serial.println("\n=== NRF24L01 Interference Detector v2 ===");
  Serial.println("LED test...");
  for (int i = 0; i < 6; i++) { digitalWrite(LED_PINS[i], HIGH); delay(200); }
  delay(300);
  for (int i = 5; i >= 0; i--) { digitalWrite(LED_PINS[i], LOW); delay(200); }

  vspi_bus.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CSN_PIN);
  if (!radio.begin(&vspi_bus)) {
    Serial.println("ERROR: NRF24L01 not found — check wiring!");
    while (1) {
      for (int i = 0; i < 6; i++) digitalWrite(LED_PINS[i], HIGH);
      delay(150);
      for (int i = 0; i < 6; i++) digitalWrite(LED_PINS[i], LOW);
      delay(150);
    }
  }

  radio.setAutoAck(false);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();

  Serial.println("Radio initialized. Scanning 125 channels...\n");
  Serial.println("Raw ch   | Avg     | Bar                      | LEDs");
  Serial.println("---------+---------+--------------------------+------");
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

// ── Rolling average ───────────────────────────────────────────────
float updateAverage(float newValue) {
  avgBuffer[avgIndex] = newValue;
  avgIndex = (avgIndex + 1) % AVG_WINDOW;
  float sum = 0;
  for (int i = 0; i < AVG_WINDOW; i++) sum += avgBuffer[i];
  return sum / AVG_WINDOW;
}

// ── Update LEDs based on averaged ratio ──────────────────────────
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
  float rawRatio    = (float)hotChannels / NUM_CHANNELS;
  float avgRatio    = updateAverage(rawRatio);
  int   ledsOn      = updateLEDs(avgRatio);

  if (millis() - lastReport >= REPORT_INTERVAL) {
    lastReport = millis();

    int bars = (int)(avgRatio * 25);
    Serial.printf("%3d/125 (%4.1f%%) | avg %4.1f%% |", hotChannels, rawRatio * 100.0, avgRatio * 100.0);
    for (int i = 0; i < 25; i++) Serial.print(i < bars ? '#' : ' ');
    Serial.printf("| %d/6 LEDs", ledsOn);

    if      (ledsOn == 0) Serial.println("  CLEAN");
    else if (ledsOn <= 2) Serial.println("  trace");
    else if (ledsOn == 3) Serial.println("  JAMMING DETECTED");
    else if (ledsOn == 4) Serial.println("  STRONG");
    else if (ledsOn == 5) Serial.println("  HEAVY");
    else                  Serial.println("  MAXIMUM");
  }
}

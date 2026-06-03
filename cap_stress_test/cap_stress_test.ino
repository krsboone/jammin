// Capacitor Stress Test
// Hammers both NRF24L01 modules simultaneously at PA_MAX
// Reports reset reason on boot — ESP_RST_BROWNOUT means caps aren't doing enough
// Open Serial Monitor at 115200 baud

#include <SPI.h>
#include <RF24.h>
#include "esp_system.h"

// HSPI — Module 1
#define HSPI_CE   16
#define HSPI_CSN  15
#define HSPI_SCK  14
#define HSPI_MOSI 13
#define HSPI_MISO 12

// VSPI — Module 2
#define VSPI_CE   22
#define VSPI_CSN  21
#define VSPI_SCK  18
#define VSPI_MOSI 23
#define VSPI_MISO 19

SPIClass hspi_bus(HSPI);
SPIClass vspi_bus(VSPI);

RF24 radio1(HSPI_CE, HSPI_CSN);
RF24 radio2(VSPI_CE, VSPI_CSN);

uint32_t sent1 = 0, sent2 = 0;
uint32_t fail1 = 0, fail2 = 0;

void reportResetReason() {
  Serial.println("\n=== Reset Reason ===");
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:   Serial.println("Power-on reset — normal startup"); break;
    case ESP_RST_BROWNOUT:  Serial.println("*** BROWNOUT RESET — voltage dropped, caps may be insufficient ***"); break;
    case ESP_RST_SW:        Serial.println("Software reset"); break;
    case ESP_RST_PANIC:     Serial.println("Panic/exception reset"); break;
    case ESP_RST_INT_WDT:   Serial.println("Watchdog reset (interrupt)"); break;
    case ESP_RST_TASK_WDT:  Serial.println("Watchdog reset (task)"); break;
    case ESP_RST_WDT:       Serial.println("Watchdog reset (other)"); break;
    case ESP_RST_DEEPSLEEP: Serial.println("Deep sleep wakeup"); break;
    default:                Serial.println("Unknown reset reason"); break;
  }
  Serial.println();
}

bool initModule(RF24 &radio, SPIClass &bus, uint8_t sck, uint8_t miso, uint8_t mosi, const char *label) {
  bus.begin(sck, miso, mosi, -1);
  if (!radio.begin(&bus)) {
    Serial.printf("%s: NOT FOUND — check wiring\n", label);
    return false;
  }
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_2MBPS);
  radio.setAutoAck(false);       // don't wait for ACK — no receiver present
  radio.stopListening();
  Serial.printf("%s: CONNECTED at PA_MAX ✓\n", label);
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("=== Capacitor Stress Test ===");
  reportResetReason();

  bool ok1 = initModule(radio1, hspi_bus, HSPI_SCK, HSPI_MISO, HSPI_MOSI, "Module 1 (HSPI)");
  bool ok2 = initModule(radio2, vspi_bus, VSPI_SCK, VSPI_MISO, VSPI_MOSI, "Module 2 (VSPI)");

  if (!ok1 && !ok2) {
    Serial.println("\nNo modules found — cannot run stress test.");
    return;
  }

  Serial.println("\nStress test running — both modules transmitting at PA_MAX");
  Serial.println("Watch for BROWNOUT RESET on next boot if caps are insufficient");
  Serial.println("Will report stats every 10 seconds. Run for at least 2 minutes.\n");
}

uint32_t lastReport = 0;
uint8_t payload[32] = {0};

void loop() {
  // Hammer both modules alternately as fast as possible
  if (radio1.isChipConnected()) {
    radio1.openWritingPipe(0xABCDABCD01LL);
    if (radio1.write(payload, 32)) sent1++; else fail1++;
  }

  if (radio2.isChipConnected()) {
    radio2.openWritingPipe(0xABCDABCD02LL);
    if (radio2.write(payload, 32)) sent2++; else fail2++;
  }

  // Report every 10 seconds
  if (millis() - lastReport >= 10000) {
    lastReport = millis();
    uint32_t total1 = sent1 + fail1;
    uint32_t total2 = sent2 + fail2;
    Serial.printf("[%lus] Module 1: %u sent, %u failed (%.1f%% success) | Module 2: %u sent, %u failed (%.1f%% success)\n",
      millis() / 1000,
      sent1, fail1, total1 ? (100.0 * sent1 / total1) : 0.0,
      sent2, fail2, total2 ? (100.0 * sent2 / total2) : 0.0
    );
  }
}

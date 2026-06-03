// NRF24L01 Wiring Diagnostic
// Tests both HSPI and VSPI modules against BlueJammer expected pin mapping
// Open Serial Monitor at 115200 baud after flashing

#include <SPI.h>
#include <RF24.h>

// HSPI — BlueJammer Module 1
#define HSPI_CE   16
#define HSPI_CSN  15
#define HSPI_SCK  14
#define HSPI_MOSI 13
#define HSPI_MISO 12

// VSPI — BlueJammer Module 2
#define VSPI_CE   22
#define VSPI_CSN  21
#define VSPI_SCK  18
#define VSPI_MOSI 23
#define VSPI_MISO 19

SPIClass hspi_bus(HSPI);
SPIClass vspi_bus(VSPI);

RF24 radio1(HSPI_CE, HSPI_CSN);
RF24 radio2(VSPI_CE, VSPI_CSN);

void testModule(RF24 &radio, SPIClass &bus, const char *label) {
  Serial.println("----------------------------------------");
  Serial.println(label);
  Serial.println("----------------------------------------");
  if (radio.begin(&bus)) {
    Serial.println("STATUS: CONNECTED ✓");
    Serial.println();
    radio.printDetails();
  } else {
    Serial.println("STATUS: NOT FOUND ✗");
    Serial.println("Check wiring — module not responding on this SPI bus.");
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n=== NRF24L01 Wiring Diagnostic ===");
  Serial.println("BlueJammer expected pin mapping\n");

  hspi_bus.begin(HSPI_SCK, HSPI_MISO, HSPI_MOSI, HSPI_CSN);
  testModule(radio1, hspi_bus, "Module 1 — HSPI (CE=16, CSN=15, SCK=14, MOSI=13, MISO=12)");

  vspi_bus.begin(VSPI_SCK, VSPI_MISO, VSPI_MOSI, VSPI_CSN);
  testModule(radio2, vspi_bus, "Module 2 — VSPI (CE=22, CSN=21, SCK=18, MOSI=23, MISO=19)");

  Serial.println("=== Done ===");
}

void loop() {}

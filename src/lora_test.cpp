#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

// Standalone RFM96W test, using the HUZZAH32 Feather's labeled SPI pins.
constexpr int radioReset = 27;
constexpr int radioCs = 33;
constexpr int radioMosi = 18;
constexpr int radioMiso = 19;
constexpr int radioSck = 5;
constexpr int radioDio0 = 12;
uint32_t packetNumber = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("RFM96W standalone test: 433 MHz");
  Serial.println("RST=27 CS=33 MOSI=18 MISO=19 SCK=5 G0=12");
  SPI.begin(radioSck, radioMiso, radioMosi, radioCs);
  LoRa.setPins(radioCs, radioReset, radioDio0);
  if (!LoRa.begin(433E6)) {
    Serial.println("FAIL: radio initialization. Check power, ground, SPI and reset wiring.");
    while (true) delay(1000);
  }
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();
  LoRa.setTxPower(5);
  Serial.println("PASS: radio initialized over SPI. This does not verify the RF link or G0 wire.");
  Serial.println("Listening. Attach a 433 MHz antenna before typing t to transmit one packet.");
  Serial.println("Use a second matching radio to verify reception; TX completion is not an acknowledgement.");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    Serial.printf("RX %d bytes: ", packetSize);
    while (LoRa.available()) Serial.write(LoRa.read());
    Serial.printf(" | RSSI=%d dBm SNR=%.1f dB\n", LoRa.packetRssi(), LoRa.packetSnr());
  }

  if (Serial.available()) {
    char command = Serial.read();
    if (command == 't' || command == 'T') {
      if (!LoRa.beginPacket()) {
        Serial.println("FAIL: could not start TX.");
        return;
      }
      LoRa.print("RFM96W test #");
      LoRa.print(++packetNumber);
      if (LoRa.endPacket()) {
        Serial.printf("TX complete: RFM96W test #%lu (delivery unconfirmed)\n",
                      static_cast<unsigned long>(packetNumber));
      } else {
        Serial.println("FAIL: TX did not complete.");
      }
    }
  }
  delay(1);
}

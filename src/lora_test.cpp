#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

// Standalone RFM96W test, using the HUZZAH32 Feather's labeled SPI pins.
constexpr int radioReset = 27;
constexpr int radioCs = 33;
constexpr int radioMosi = 18;
constexpr int radioMiso = 19;
constexpr int radioSck = 5;
constexpr int radioDio0 = 15; // G0 wiring in the CAD reference.
uint32_t packetNumber = 0;
uint8_t message[255];
size_t messageLength = 0;
bool messageOverflow = false;

void sendMessage() {
  if (messageOverflow) {
    Serial.println("FAIL: message exceeds 255 bytes; nothing sent.");
    return;
  }
  if (messageLength == 0) return;
  if (!LoRa.beginPacket()) {
    Serial.println("FAIL: could not start TX.");
    return;
  }
  if (messageLength == 1 && (message[0] == 't' || message[0] == 'T')) {
    LoRa.print("RFM96W test #");
    LoRa.print(++packetNumber);
  } else {
    LoRa.write(message, messageLength);
  }
  Serial.println(LoRa.endPacket()
      ? "TX complete (delivery unconfirmed)."
      : "FAIL: TX did not complete.");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("RFM96W standalone test: 433 MHz");
  Serial.println("RST=27 CS=33 MOSI=18 MISO=19 SCK=5 G0=15");
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
  Serial.println("Listening. Attach a 433 MHz antenna before transmitting.");
  Serial.println("Type a message and press Enter to send (255 bytes max). t + Enter sends a numbered test.");
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
    int input = Serial.read();
    if (input == '\r' || input == '\n') {
      // CRLF is safe: the second terminator sees an empty buffer.
      if (messageLength > 0 || messageOverflow) {
        Serial.println();
        sendMessage();
        messageLength = 0;
        messageOverflow = false;
      }
    } else if (input == 8 || input == 127) {
      if (!messageOverflow && messageLength > 0) {
        --messageLength;
        Serial.print("\b \b");
      }
    } else if (input >= 32) {
      if (messageLength < sizeof(message) && !messageOverflow) {
        message[messageLength++] = static_cast<uint8_t>(input);
        Serial.write(input);
      } else {
        // Discard the entire oversized line rather than sending truncated text.
        messageOverflow = true;
      }
    }
  }
  delay(1);
}

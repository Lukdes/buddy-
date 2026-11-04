#include "Link.h"
#include <Wire.h>
#include <MAX30105.h>
#include <spo2_algorithm.h>
#include "Vibration.h"

uint32_t lastEstimate = 0;

MAX30105 sensor;
const uint8_t destination[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
uint32_t lastSend = 0, sequence = 0;
uint32_t samplePairsSinceReport = 0;
uint32_t irBuffer[100], redBuffer[100];
uint16_t sampleCount = 0;
uint32_t latestIr = 0, lastSample = 0;
int32_t oxygen = 0, heartRate = 0;
int8_t oxygenValid = 0, heartValid = 0;
bool finger = false;

void resetReadings() {
  sampleCount = 0;
  oxygenValid = heartValid = 0;
  oxygen = heartRate = 0;
  vibration::update(millis(), 0, false, 0);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  vibration::begin();
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  startRadio();
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, destination, 6);
  peer.channel = radioChannel;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("Could not add broadcast peer.");
    while (true) delay(1000);
  }
  Wire.begin(23, 22); // Original HUZZAH32 Feather SDA, SCL.
  if (!sensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("Sensor not detected. Check VIN, GND, SDA and SCL; press RESET.");
    while (true) delay(1000);
  }
  sensor.setup(60, 4, 2, 100, 411, 4096); // 25 averaged samples/sec for SpO2 algorithm.
  lastSend = millis();
  Serial.println("Sender ready. Place a fingertip gently over the sensor.");
}

void loop() {
  sensor.check();
  while (sensor.available()) {
    // Read both channels from the same queued sample before advancing.
    uint32_t red = sensor.getFIFORed();
    latestIr = sensor.getFIFOIR();
    sensor.nextSample();
    ++samplePairsSinceReport; // Include pairs acquired without finger contact.
    lastSample = millis();
    finger = latestIr >= 50000; // Contact threshold; may need tuning.
    if (!finger) {
      resetReadings();
      continue;
    }
    irBuffer[sampleCount] = latestIr;
    redBuffer[sampleCount] = red;
    ++sampleCount;
    if (sampleCount == 100) {
      maxim_heart_rate_and_oxygen_saturation(irBuffer, 100, redBuffer,
          &oxygen, &oxygenValid, &heartRate, &heartValid);
      oxygenValid = oxygenValid && oxygen > 0 && oxygen <= 100;
      heartValid = heartValid && heartRate >= 30 && heartRate <= 220;
      lastEstimate = millis();
      // Retain 3 seconds; collect 1 second before the next calculation.
      memmove(irBuffer, irBuffer + 25, 75 * sizeof(uint32_t));
      memmove(redBuffer, redBuffer + 25, 75 * sizeof(uint32_t));
      sampleCount = 75;
    }
  }
  uint32_t now = millis();
  if (now - lastSample > 2000) {
    finger = false;
    resetReadings();
  }
  vibration::update(now, oxygen, finger && oxygenValid, lastEstimate);
  if (now - lastSend >= 1000) {
    uint32_t reportElapsed = now - lastSend;
    uint32_t reportPairs = samplePairsSinceReport;
    float samplePairsPerSecond = reportPairs * 1000.0f / reportElapsed;
    samplePairsSinceReport = 0;
    lastSend = now;
    HeartPacket packet = {};
    packet.magic = packetMagic;
    packet.sequence = ++sequence;
    packet.infrared = latestIr;
    packet.bpm = heartValid ? heartRate : 0;
    packet.status = !finger ? 0 : (heartValid ? 2 : 1);
    packet.spo2 = oxygenValid ? oxygen : 0;
    packet.spo2Valid = oxygenValid ? 1 : 0;
    esp_err_t result = esp_now_send(destination,
        reinterpret_cast<const uint8_t *>(&packet), sizeof(packet));
    Serial.printf("Packet %lu | IR=%lu | ",
        (unsigned long)packet.sequence, (unsigned long)packet.infrared);
    if (!finger) Serial.print("No finger | ");
    else if (heartValid) Serial.printf("Estimated BPM=%ld | ", (long)heartRate);
    else Serial.print("BPM calculating/unavailable | ");
    if (oxygenValid) Serial.printf("Estimated SpO2=%ld%% | ", (long)oxygen);
    else Serial.print("SpO2 calculating/unavailable | ");
    Serial.printf("Vibration=%s | Alert below=%.1f%% | ",
        vibration::isActive() ? "ON" : "OFF", vibration::threshold());
    Serial.printf("Sample pairs/s=%.1f (expected 25; %lu pairs/%lu ms) | ",
        samplePairsPerSecond, (unsigned long)reportPairs,
        (unsigned long)reportElapsed);
    Serial.println(result == ESP_OK ? "queued" : "send error");
  }
  delay(1);
}

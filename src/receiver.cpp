#include "Link.h"
#include "Vibration.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <math.h>

struct ReceivedReading {
  HeartPacket packet;
  uint32_t receivedAt;
};
QueueHandle_t incoming;
float latestOxygen = 0;
bool oxygenValid = false;
uint32_t lastReceived = 0;
bool waiting = true;

// Arduino ESP32 2.x callback signature (platform pinned in platformio.ini).
void receivePacket(const uint8_t *mac, const uint8_t *data, int length) {
  if (length != sizeof(HeartPacket)) return;
  HeartPacket packet;
  memcpy(&packet, data, sizeof(packet));
  if (packet.magic != packetMagic || packet.status > 2 ||
      !isfinite(packet.bpm) || packet.spo2Valid > 1 ||
      !isfinite(packet.spo2) ||
      (packet.spo2Valid && (packet.status == 0 || packet.spo2 <= 0 || packet.spo2 > 100)) ||
      (packet.status == 2 && (packet.bpm < 30 || packet.bpm > 220))) return;
  ReceivedReading reading = {packet, millis()};
  xQueueOverwrite(incoming, &reading);
}

void setup() {
  vibration::begin();
  Serial.begin(115200);
  delay(1000);
  incoming = xQueueCreate(1, sizeof(ReceivedReading));
  if (!incoming) {
    Serial.println("Could not allocate receive queue.");
    while (true) delay(1000);
  }
  startRadio();
  if (esp_now_register_recv_cb(receivePacket) != ESP_OK) {
    Serial.println("Could not register receiver.");
    while (true) delay(1000);
  }
  Serial.println("Receiver ready. Waiting for heart-rate sender...");
}

void loop() {
  ReceivedReading reading;
  if (xQueueReceive(incoming, &reading, pdMS_TO_TICKS(10)) == pdTRUE) {
    const HeartPacket &packet = reading.packet;
    lastReceived = reading.receivedAt;
    latestOxygen = packet.spo2;
    oxygenValid = packet.status != 0 && packet.spo2Valid;
    vibration::update(millis(), latestOxygen, oxygenValid, lastReceived);
    waiting = false;
    Serial.printf("Received #%lu | IR=%lu | ",
        (unsigned long)packet.sequence, (unsigned long)packet.infrared);
    if (packet.status == 0) Serial.print("No finger on sensor");
    else if (packet.status == 1) Serial.print("BPM calculating/unavailable");
    else Serial.printf("Estimated BPM=%.1f", packet.bpm);
    if (packet.spo2Valid) Serial.printf(" | Estimated SpO2=%.0f%%", packet.spo2);
    else Serial.print(" | SpO2 calculating/unavailable");
    Serial.printf(" | Vibration=%s | Alert below=%.1f%%\n",
        vibration::isActive() ? "ON" : "OFF", vibration::threshold());
  }
  // Continue timing the pulse and expiring data even when packets stop.
  vibration::update(millis(), latestOxygen, oxygenValid, lastReceived);
  if (!waiting && millis() - lastReceived > 5000) {
    waiting = true;
    Serial.println("No packets for 5 seconds. Check sender power and distance.");
  }
}

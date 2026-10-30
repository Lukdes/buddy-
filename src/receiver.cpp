#include "Link.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <math.h>

QueueHandle_t incoming;
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
  xQueueOverwrite(incoming, &packet);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  incoming = xQueueCreate(1, sizeof(HeartPacket));
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
  HeartPacket packet;
  if (xQueueReceive(incoming, &packet, pdMS_TO_TICKS(100)) == pdTRUE) {
    lastReceived = millis();
    waiting = false;
    Serial.printf("Received #%lu | IR=%lu | ",
        (unsigned long)packet.sequence, (unsigned long)packet.infrared);
    if (packet.status == 0) Serial.print("No finger on sensor");
    else if (packet.status == 1) Serial.print("BPM calculating/unavailable");
    else Serial.printf("Estimated BPM=%.1f", packet.bpm);
    if (packet.spo2Valid) Serial.printf(" | Estimated SpO2=%.0f%%\n", packet.spo2);
    else Serial.println(" | SpO2 calculating/unavailable");
  }
  if (!waiting && millis() - lastReceived > 5000) {
    waiting = true;
    Serial.println("No packets for 5 seconds. Check sender power and distance.");
  }
}

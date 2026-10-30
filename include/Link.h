#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

constexpr uint8_t radioChannel = 1;
constexpr uint32_t packetMagic = 0x48525432;
struct HeartPacket {
  uint32_t magic;
  uint32_t sequence;
  uint32_t infrared;
  float bpm;
  uint8_t status; // 0: no finger, 1: finding pulse, 2: pulse available
  uint8_t spo2Valid;
  uint8_t reserved[2];
  float spo2;
};
static_assert(sizeof(HeartPacket) == 24, "Unexpected packet layout");

inline void startRadio() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_wifi_set_channel(radioChannel, WIFI_SECOND_CHAN_NONE) != ESP_OK ||
      esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed. Press RESET.");
    while (true) delay(1000);
  }
  Serial.print("ESP-NOW ready. MAC: ");
  Serial.println(WiFi.macAddress());
}

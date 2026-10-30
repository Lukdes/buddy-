#include <Arduino.h>
#include <Wire.h>
#include <MAX30105.h>
#include <heartRate.h>

MAX30105 sensor; // Also supports MAX30102.
uint32_t lastBeat = 0;
uint32_t lastPrint = 0;
float bpm = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(27, OUTPUT);
  digitalWrite(27, LOW);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  Wire.begin(23, 22); // Original HUZZAH32 Feather: SDA=23, SCL=22.

  if (!sensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("Sensor not detected. Check VIN, GND, SDA and SCL, then press RESET.");
    while (true) delay(1000);
  }
  sensor.setup(0x1F, 4, 2, 400, 411, 4096); // Red and infrared LEDs.
  Serial.println("Sensor detected. Place your fingertip gently on the sensor and keep still.");
}

void loop() {
  long ir = sensor.getIR();
  bool beat = checkForBeat(ir);
  uint32_t now = millis();

  if (ir < 50000) {
    lastBeat = 0;
    bpm = 0;
  } else if (beat) {
    if (lastBeat != 0) {
      float reading = 60000.0f / (now - lastBeat);
      if (reading >= 30 && reading <= 220) bpm = reading;
    }
    lastBeat = now;
  }
  if (lastBeat != 0 && now - lastBeat > 3000) {
    lastBeat = 0;
    bpm = 0;
  }

  if (now - lastPrint >= 500) {
    lastPrint = now;
    Serial.print("IR=");
    Serial.print(ir);
    if (ir < 50000) {
      Serial.println(" | Place finger on sensor");
    } else if (bpm == 0) {
      Serial.println(" | Finding pulse... keep still");
    } else {
      Serial.print(" | Estimated BPM=");
      Serial.println(bpm, 1);
    }
  }
}

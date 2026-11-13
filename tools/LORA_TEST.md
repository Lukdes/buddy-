# Standalone RFM96W test

`src/lora_test.cpp` is isolated in the `lora_test` PlatformIO environment.
It targets the Adafruit HUZZAH32 ESP32 Feather shown in the wiring photo,
using its labeled SPI pins (`featheresp32` board configuration).
The existing default environment and sensor/receiver firmwares are unchanged.

Power off before wiring. Use these GPIO numbers, not physical pin positions:

| RFM96W | ESP32 |
| --- | --- |
| VIN | 3V3 |
| GND | GND |
| RST | GPIO27 |
| CS | GPIO33 |
| MOSI | MOSI / GPIO18 |
| MISO | MISO / GPIO19 |
| SCK | SCK / GPIO5 |
| G0 / DIO0 | GPIO12 |

Leave EN and G1–G5 disconnected. Attach a 433 MHz antenna before transmitting.
GPIO12 is a boot strapping pin: if the ESP32 fails to boot with the radio
attached, power off and disconnect G0 to troubleshoot. This test polls the
radio over SPI, so success does not verify the G0 connection.
Disconnect any vibration module from GPIO27 before using that pin for radio reset.

Build without uploading:

```sh
pio run -e lora_test
```

When ready to replace the firmware on the physically identified test board,
close other serial monitors/recorders and run these commands using its actual port:

```sh
pio run -e lora_test -t upload --upload-port <test-board-port>
pio device monitor -p <test-board-port> -b 115200
```

Use your installed PlatformIO executable if `pio` is not on PATH. Reset the
board after opening the monitor to see startup output. `PASS` means the radio
initialized over SPI; failure suggests checking wiring and power.

The test listens by default. Type `t` to send one numbered packet at 5 dBm;
there is no automatic repeat. TX completion alone does not prove reception.
For an over-the-air test, use a second ESP32/RFM96W running this sketch with
its own matching wiring. Send from one board and look for `RX` plus the matching
packet number on the other, then test the reverse direction. Both use 433 MHz,
SF7, 125 kHz bandwidth, coding rate 4/5, sync word 0x12 and CRC enabled.

Library API: <https://github.com/sandeepmistry/arduino-LoRa/blob/master/API.md>
Radio guide: <https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts>

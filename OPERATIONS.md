# Operations and agent handoff

Last updated: 2026-11-17. Read `AGENTS.md` for project rules and
`tools/README.md` for full commands. Check live Git status and remotes before
working; this document is a handoff snapshot, not a substitute for inspection.

## Current baseline

- Standalone `src/lora_test.cpp` / `lora_test` environment checks the RFM96W
  at 433 MHz with HUZZAH32 labeled SPI pins (MOSI18/MISO19/SCK5),
  RST27, CS33 and G0 on GPIO12. It listens
  and sends typed serial messages on Enter (maximum 255 bytes); `t` + Enter
  sends a numbered test packet. Oversized lines are rejected without transmission.
  See `tools/LORA_TEST.md`.
  PlatformIO build passed; no upload or hardware test was performed. SPI
  initialization does not verify the RF link or G0 wiring. Existing firmware
  entry points, packet contract, and default build environment are unchanged.

- Vibration control now runs only on the observer receiver. Move the module IN
  to receiver GPIO27, VCC to the supported receiver supply, and GND to common
  receiver ground while powered off. Upload both firmwares to remove the old
  sender behavior and enable receiver alerts. Sender upload succeeded on a replacement
  board on 2026-11-17; receiver alert behavior still needs hardware verification.
- The receiver default threshold is now 98% for testing: valid SpO2 below 98%
  keeps the alarm ON continuously. At or above 98% turns it OFF. No re-arm or
  cooldown is required. Invalid/no-contact input turns it OFF.
  Receiver packet age over two seconds cancels output; packet arrival time is not
  sensor acquisition time. The five-second link-loss message remains serial-only.
- Previous motor bench testing was on the sender setup. Receiver motor wiring,
  live threshold transitions, and packet-loss behavior need hardware verification.
- Acquisition and optional logging are the current focus; prediction is deferred.
- The sender uses matched red/IR FIFO reads and prints its processed sample-pair
  rate on USB. Expected rate is approximately 25 pairs per second. The receiver
  does not receive this diagnostic through the existing packet format.
- The FIFO fix built successfully and user-run hardware checks showed improved
  resting BPM agreement. This is not clinical validation or a validation of SpO2
  accuracy. Repeated reference comparisons remain outstanding.
- No 20-second settling stage or additional smoothing has been implemented.
  The temporary I2C scanner was removed and is not part of this baseline.

## Sender upload on 2026-11-17

- Built and uploaded the existing sender firmware successfully to a replacement
  USB-connected ESP32. The uploader verified the written flash and reset the board.
- No firmware source change was made. A brief serial check returned no output;
  sensor acquisition and wireless packet delivery remain unverified. With the
  optical sensor absent, this firmware stops during sensor initialization.

## Board identification and upload workflow

The sender is the ESP32 physically wired to the sensor, often powered by a
battery. The receiver is the separate ESP32 normally connected to the computer.
Uploading sender firmware to the receiver causes it to search for an absent
wired sensor. That error alone does not establish a fault in the remote sensor.

1. Stop the serial monitor or recorder normally.
2. Connect the intended board by USB. If ports are ambiguous, connect one board
   at a time and run `make ports`; never infer role from an old port name.
3. Use one of the explicit upload commands:

   ```sh
   make upload-sender PORT=<sender-port>
   make upload-receiver PORT=<receiver-port>
   ```

4. Confirm upload success, then view or record from the chosen port. Sender output
   says `Sender ready` and `Packet`; receiver output says `Receiver ready` and
   `Received`. Verify the sample-rate field directly on the sender when testing
   sampling changes.

These commands build and upload the current checkout. An explicit port is
required, but the commands cannot detect the physical board role. They do not
update a battery-powered sender through its receiver. `PIO` can override the
PlatformIO executable; otherwise the Makefile selects its usual home installation
when present and falls back to PATH. View/record commands do not flash firmware.

## Verification and limitations

- Local main integration (2026-11-17): all seven recorder tests and synthetic
  alarm policy tests passed. All five PlatformIO environments built successfully.
  The existing MAX3010x I2C_BUFFER_LENGTH warning remains. No hardware was flashed
  during integration; receiver alarm and LoRa RF validation remain pending.
- Continuous-alarm update (2026-11-05): receiver and vibration_test builds and
  updated host policy tests passed. Tests cover sustained low output beyond five
  seconds, contact return without re-arming, recovery, and invalid inputs. The
  standalone bench test explicitly invalidates its input after five seconds so
  it remains bounded. No upload or hardware validation of this update was done.
- Receiver-only move: sender and receiver builds passed; host policy tests passed
  with a compatible SDK. Existing library I2C_BUFFER_LENGTH warning remains.
  No upload was performed; receiver-side motor and dropout tests remain pending.
- Pre-push verification on 2026-11-04: sender, receiver, and vibration_test builds
  passed, as did host policy tests with a compatible SDK. No hardware was flashed.
- External-data analysis is local under ignored `data/external/`; no prediction
  model or revised alert policy from that analysis has been integrated.
- Vibration policy: sender build passed; synthetic policy tests passed using an
  installed compatible macOS SDK override. The default host SDK initially failed
  at link time due to an unsupported architecture in its system-library stubs.
  Tests cover threshold boundaries, duration, re-arming, invalid input, threshold
  updates, and timer rollover. The `vibration_test` firmware produced labelled
  synthetic ON/OFF messages and the user confirmed physical vibration and stop.
  Normal sender output also showed expected sampling and motor OFF for unavailable
  and above-threshold estimates. Physiological accuracy and vibration interference
  with optical sensing remain unvalidated.
- Upload shortcuts were checked with a simulated uploader: both environments,
  rejection of a missing port before invocation, arguments with spaces, and
  propagation of upload failure passed. No board was flashed by these checks.
- On 2026-11-03, all seven recording transport/file-integrity tests passed.
- Firmware source and packet format are unchanged by the upload-shortcut work.
  The sender upload shortcut has now been used successfully on hardware.
- Receiver recovery: the separate receiver was found running sender firmware.
  After verifying its identity, receiver firmware was uploaded successfully and
  consecutive wireless packets arrived from the breadboard sender. This verified
  communication with no finger present, not a live threshold-triggered motor pulse.
- Preserve original serial bytes and unavailable readings. A stable estimate and
  an expected processing rate alone do not prove physiological accuracy.

## Repository and local data

Local `main` now includes Luke's `origin/main` through `4266ee7` (standalone
LoRa tests) and the pending continuous receiver alarm changes. `origin` points
to Luke's repository and `fork` to the personal fork. Local `main` still tracks
`fork/main`; check the intended destination before any future authorized push.
No push was performed during this integration. `data-logging` remains at
`169009c`. Local recordings and machine configuration remain ignored.

Recordings live under ignored `data/raw_records/`. Local troubleshooting notes
live under ignored `data/knowledge_base/`; they may be absent on another computer.
Do not publish their contents or force-add data. The tracked handoff contains
only the operational workflow; local files need separate backups.

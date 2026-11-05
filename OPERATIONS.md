# Operations and agent handoff

Last updated: 2026-11-04. Read `AGENTS.md` for project rules and
`tools/README.md` for full commands. Check live Git status and remotes before
working; this document is a handoff snapshot, not a substitute for inspection.

## Current baseline

- Vibration control now runs only on the observer receiver. Move the module IN
  to receiver GPIO27, VCC to the supported receiver supply, and GND to common
  receiver ground while powered off. Upload both firmwares to remove the old
  sender behavior and enable receiver alerts. Neither has been flashed for this change.
- The unchanged policy triggers one five-second pulse for valid SpO2 below 97%,
  with valid recovery needed to re-arm. Invalid/no-contact input cancels it.
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

At this handoff, `origin` points to `Lukdes/buddy-`, and `data-logging` tracks
`origin/data-logging`. A separate `fork` remote points to the personal fork.
The receiver-only change targets Luke's `main` by explicit user request; local
`data-logging` remains the working branch. Main already merged the earlier work
through PR #2. Updating main does not flash either board: move the motor to the
receiver and upload both updated firmwares before hardware validation.
Confirm `git remote -v` and `git status -sb` before any authorized push; do not
assume the remote names from older change-log entries remain current.

Recordings live under ignored `data/raw_records/`. Local troubleshooting notes
live under ignored `data/knowledge_base/`; they may be absent on another computer.
Do not publish their contents or force-add data. The tracked handoff contains
only the operational workflow; local files need separate backups.

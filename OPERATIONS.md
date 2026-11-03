# Operations and agent handoff

Last updated: 2026-11-03. Read `AGENTS.md` for project rules and
`tools/README.md` for full commands. Check live Git status and remotes before
working; this document is a handoff snapshot, not a substitute for inspection.

## Current baseline

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

- Upload shortcuts were checked with a simulated uploader: both environments,
  rejection of a missing port before invocation, arguments with spaces, and
  propagation of upload failure passed. No board was flashed by these checks.
- On 2026-11-03, all seven recording transport/file-integrity tests passed.
- Firmware source and packet format are unchanged by the upload-shortcut work.
  Actual hardware use of the shortcuts remains to be confirmed.
- Preserve original serial bytes and unavailable readings. A stable estimate and
  an expected processing rate alone do not prove physiological accuracy.

## Repository and local data

At this handoff, `origin` points to `Lukdes/buddy-`, and `data-logging` tracks
`origin/data-logging`. A separate `fork` remote points to the personal fork.
Confirm `git remote -v` and `git status -sb` before any authorized push; do not
assume the remote names from older change-log entries remain current.

Recordings live under ignored `data/raw_records/`. Local troubleshooting notes
live under ignored `data/knowledge_base/`; they may be absent on another computer.
Do not publish their contents or force-add data. The tracked handoff contains
only the operational workflow; local files need separate backups.

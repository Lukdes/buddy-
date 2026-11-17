# Change log

## 2026-11-17 — Local main integration and continuous receiver alarm

- Bring local main forward through Luke's main at 4266ee7, including the
  standalone RFM96W test and typed-message support. Preserve the pending receiver,
  bench-test, and documentation changes while resolving the handoff date conflict.
- Keep receiver vibration active while fresh valid SpO2 is below 98%; cancel on
  recovery, invalid/no-contact input, or packets older than two seconds. Remove
  the pulse latch and re-arm requirement. The reusable policy defaults to 100%;
  receiver initialization explicitly sets 98%. Keep the standalone synthetic
  bench test bounded to five seconds.
- Print receiver SpO2 with nine significant digits and identify continuous-v2
  at startup. Update usage and handoff documentation and record the successful
  existing-sender upload to a replacement board.
- Verification: all seven recorder tests and synthetic alarm policy tests passed.
  PlatformIO builds passed for sender, receiver, vibration_test, heartrate, and
  lora_test. The existing MAX3010x I2C_BUFFER_LENGTH warning remains. Whitespace
  and ignore checks passed; data and machine configuration remain excluded.
- No hardware was flashed during Git integration. Earlier sender flash succeeded,
  but sensor acquisition and packet delivery were not verified. Receiver alarm
  transitions and the standalone LoRa RF link still require hardware testing.
  This integration is committed locally; no push was performed.

## 2026-11-12 — Typed LoRa messages

- Extend the standalone radio test to send a typed word or sentence on Enter,
  with a 255-byte limit, input echo and backspace editing. Accept CR, LF and
  CRLF, ignore empty lines and reject oversized lines in full without sending.
  The numbered `t`/`T` shortcut now also requires Enter.
- Update radio usage instructions and the operational handoff. Existing sensor
  firmware, radio settings and ESP-NOW packet contract remain unchanged.
- Verification: `pio run -e lora_test` passed; Git whitespace and ignored-data
  checks passed. No firmware was uploaded by the agent. Typed-message reception,
  input editing and overflow handling still require hardware verification.

## 2026-11-12 — Standalone RFM96W radio test

- Add an isolated `lora_test` PlatformIO environment and firmware for testing
  two 433 MHz RFM96W radios without changing the sensor sender, receiver,
  heartrate entry points, ESP-NOW packet contract, or default environment.
- Match the HUZZAH32 Feather's labeled SPI pins: MOSI18, MISO19 and SCK5,
  with RST27, CS33 and G0 on GPIO12. Listen for packets and transmit one
  numbered message per serial `t` command at 115200 baud.
- Pin LoRa library version 0.8.0 and document wiring, building, uploading,
  two-board testing, antenna use, GPIO12 boot considerations and GPIO27 conflict
  with the separate vibration module. Update the operational handoff.
- Verification: `pio run -e lora_test` passed using the Feather configuration;
  Git whitespace and ignore checks passed. Data, build artifacts and local
  configuration remain excluded. No agent upload or hardware test was performed;
  initialization, RF reception and G0 wiring still need hardware verification.

## 2026-11-04 — Receiver-only observer vibration

- Move the vibration controller from sender to receiver so the observer receives
  the alert. Sender GPIO27 stays low; sensor acquisition and packet layout remain
  unchanged. Receiver output now includes motor state and threshold.
- Timestamp accepted packets in the receive callback and service pulse timing
  between packets. Stop output after more than two seconds without a fresh packet;
  retain the five-second serial connection-loss warning. No separate link-loss
  vibration pattern is implemented. Arrival age is not sensor acquisition age.
- Preserve the below-97% trigger, bounded five-second pulse, invalid/contact-loss
  cancellation, and recovery re-arming policy. Update Make help, wiring/upload
  instructions, agent architecture notes, and operational handoff.
- Verification: sender and receiver builds and host policy tests passed during
  this change; Git whitespace and ignored-data checks passed. The existing
  library I2C_BUFFER_LENGTH warning remains. No hardware was flashed; receiver
  wiring, motor operation, and packet-loss behavior still need hardware testing.
- Integrate with Luke's main, which already includes the earlier data-logging
  and vibration work through PR #2. Recordings and external analysis stay ignored.

## 2026-11-04 — Sensor-driven vibration alerts

- Add a separate vibration controller to the sender with a configurable default
  threshold of strictly below 97% and one nonblocking five-second pulse. Valid
  recovery re-arms the policy; invalid, stale, or lost-contact input stops it.
- Expose a threshold setter for future integration and print motor state on sender
  USB output. Preserve sensor settings, estimation, and the wireless packet format.
- Add standalone synthetic motor-test firmware and host policy tests. Clarify
  Make help and the two-board upload/view workflow, including receiver recovery.
- Verification for this push: sender, receiver, and vibration_test builds passed;
  host policy tests passed with a compatible SDK selected. Tests cover threshold
  boundaries, pulse duration, recovery, invalid input, and timer rollover.
- Earlier bench testing confirmed physical motor activation and stop; receiver
  recovery confirmed wireless packet reception. No hardware was flashed for this
  push. The separated controller still needs hardware verification; physiological
  accuracy and vibration interference remain unvalidated.
- External-data exploratory analysis remains local under ignored data/. It does
  not change the deployed algorithm. Recordings, datasets, and analysis outputs
  are excluded; Git ignore and whitespace checks passed.

## 2026-11-03 — Explicit firmware upload shortcuts and agent handoff

- Add `make upload-sender PORT=...` and `make upload-receiver PORT=...` to build
  and upload the selected firmware. Require an explicit port to avoid automatic
  selection of the other board, with a configurable PlatformIO executable.
- Document how to identify the sender and receiver, close monitors, upload, and
  resume viewing/recording. The commands cannot verify physical board role.
- Add `OPERATIONS.md` with the current baseline, operational workflow, verification
  limits, and repository context. Link it from the agent instructions and record
  the new upload procedure there.
- Validation: simulated uploader checks passed for both targets, missing-port
  rejection, quoted paths/arguments, and failure propagation. All seven recording
  tests passed; help output and Git whitespace checks passed. No hardware was
  flashed by these checks; firmware sources and the packet format are unchanged.
- Recordings and the detailed local troubleshooting knowledge base remain ignored
  and excluded from this push. Hardware use of the new shortcuts and repeated
  physiological accuracy checks remain pending.

## 2026-11-01 — Correct sender sample pairing

- Read red and infrared from the same queued FIFO sample before advancing,
  avoiding separate fresh-data waits that could skip samples and distort the
  timing assumed by the BPM algorithm.
- Add a sender USB diagnostic reporting processed sample pairs per second and
  the measured interval, with an expected rate of 25 pairs per second.
- Document how to upload to the sensor-connected sender and capture the
  diagnostic. Receiver firmware, packet format, and sensor settings are unchanged.
- Verification: the updated sender built successfully with PlatformIO; the build
  reported a library/framework `I2C_BUFFER_LENGTH` redefinition warning. User-run
  hardware testing confirmed the diagnostic appeared and reported approximately
  the expected sampling rate. `git diff --check` passed.
- Repeated accuracy validation remains pending. The throughput diagnostic does
  not establish signal quality, lossless sampling, or SpO2 accuracy. Personal
  recordings and detailed troubleshooting notes remain local under ignored data/.

## 2026-11-01 — Integrate data logging into main

- Fast-forward `main` to include the data-logging workflow documented below.
  Firmware sources, headers, and PlatformIO configuration retain their original
  versions; the experimental sampling changes and I2C scanner are not included.
- Configure the local checkout to use the personal fork as `origin` and Luke's
  repository as `upstream`, so normal pushes target the fork.
- Verification: all seven `make test` tests passed; `git diff --check` passed;
  no files under `data/` or `.vscode/` are tracked. Local recordings were preserved.
- Sensor communication and physiological accuracy still require hardware
  validation; this integration introduces no sensor-estimation fix.

## 2026-11-01 — Data logging

### Changes and purpose

- Add a separate laptop serial viewer and optional recorder without modifying
  sensor firmware. Viewing saves no files; recording displays and preserves the
  exact received bytes in unique `data/raw_records/` directories.
- Save metadata with UTC session times, source label, notes, byte count, SHA-256,
  and completion status. Retain partial captures after disconnects and identify
  empty recordings explicitly.
- Add uv dependency configuration and lockfile, Make shortcuts for setup, viewing,
  recording, port listing, and tests, plus usage documentation.
- Ignore all local data, VS Code settings, virtual environments, and Python
  caches. Remove previously tracked VS Code configuration from Git while keeping
  the local file.
- Add repository agent instructions covering coding habits, firmware boundaries,
  recording conventions, setup on other computers, and the pre-push change log
  procedure.

### Verification

- `make test`: all seven synthetic transport and file-integrity tests passed.
  Coverage includes view-only behavior, exact byte preservation, metadata/hash,
  disconnects, busy ports, empty captures, unique names, and path traversal.
- `make help`: command descriptions display successfully.
- `git diff --check`: passed. Ignore rules match local recordings, VS Code
  configuration, and the virtual environment; no data files are tracked.
- Confirmed no changes to firmware sources, headers, or PlatformIO configuration.

### Limitations and remaining work

- These tests do not validate physiological accuracy. Heart-rate disagreement
  with the reference device and SpO2 accuracy still require investigation.
- The logger captures existing serial output; raw optical waveforms and sensor
  acquisition timestamps are unavailable unless the firmware prints them.
- Hardware validation was not performed for this push. Prediction development
  remains deferred while acquisition reliability is investigated.

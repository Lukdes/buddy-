# Change log

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

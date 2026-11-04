# Buddy Minus agent instructions

These instructions apply throughout this repository. Read this file before
working, then inspect `git status` and the relevant source/configuration. Preserve
existing user changes. Explicit user instructions take precedence over this file.
Read `OPERATIONS.md` for the current handoff and `CHANGELOG.md` for change history.

## Purpose and current scope

Buddy- is the main project for sensor acquisition, live viewing, and optional
recording of heart rate and SpO2 during experiments. Reliable acquisition and
recording come first; prediction algorithm development is currently deferred.
Buddy Plus is a separate analysis project. Inspect its current files when needed;
do not assume it has the same setup or copy its algorithms into this project
without discussing the change.

Recorded estimates are experimental and not clinically validated. Saving an
estimate does not establish its accuracy, and a displayed SpO2 of 100% does not
prove correct sensor operation. Do not design validation around deliberately
inducing dangerous desaturation or blackout.

## Architecture and firmware boundaries

- `platformio.ini` defines separate `sender`, `receiver`, and `heartrate` firmware
  environments. Each has its own entry point; they are not called together by the
  laptop recorder.
- `src/sender.cpp` reads the optical sensor, estimates BPM/SpO2, and sends ESP-NOW
  packets. `src/receiver.cpp` receives packets and prints their readings.
  `src/heartrate.cpp` is an alternative standalone BPM firmware.
- `include/Link.h` defines the shared packet contract. Keep sender and receiver
  compatible when an approved change affects that contract.
- `tools/record.py` is the separate laptop serial viewer/recorder. See
  `tools/README.md` for its interface and recording guarantees.

Discuss proposed firmware, sensor-setting, estimation, and packet-format changes
with the user before implementing them. In particular, do not change the three
existing firmware entry points as a side effect of recording work. Routine
documentation and laptop-tool changes within the requested scope can proceed.
Do not flash hardware or interrupt an active recording without user authorization.

## Coding and update habits

- Keep changes small and focused. Follow the surrounding style and avoid unrelated
  refactoring. Use clear names and comments that explain non-obvious decisions.
- Preserve missing/invalid readings and errors; do not silently replace them with
  plausible values, smooth the original capture, or claim unverified fixes.
- Update `tools/README.md` when commands, output formats, or setup steps change.
  Update this file when an agreed project rule or workflow changes.
- Update `OPERATIONS.md` when handing off an operational change, including what
  was verified and what still needs hardware testing. Keep personal experiment
  details in ignored local data rather than the tracked handoff.
- Keep experimental notes with local recordings. This file records durable rules,
  not a running diary of individual experiments.
- Report what changed, what was checked, and what still needs hardware validation.
  Do not commit or push unless requested.

## Before every GitHub push

1. Review all commits and changes included in the proposed push.
2. Update the root `CHANGELOG.md` before committing the final documentation for
   that push; create it if it does not exist. Add a dated entry describing what
   changed, why, the checks actually performed and their results, and any known
   limitations or remaining work. Summarize the full work included in the push,
   not just the last edit. Keep previous entries as history.
3. Update relevant usage/setup documentation and this file if the workflow or
   agreed rules changed. Keep recordings, personal details, and machine-specific
   configuration out of the change log.
4. Check the final diff and Git status, confirm ignored data is excluded, and
   include the change log update in the commits being pushed. If more changes are
   made afterward, refresh the entry before pushing.
5. Push only when the user has requested or authorized it. Afterward, report the
   actual outcome; do not describe an unattempted or failed push as completed.

## Recording and data handling

- Viewing is optional and must save no recordings. Recording must remain a
  separate explicit action that also displays the current serial output.
- Save recordings in unique directories under `data/raw_records/`, containing
  `serial.txt` and `metadata.json`. Never overwrite an existing capture.
- Preserve the received bytes in `serial.txt`, including partial lines and errors.
  Keep metadata separate; laptop timestamps are not sensor acquisition timestamps.
- `SOURCE`/`--source` is a metadata label for firmware already running, not a
  firmware selector. The recorder cannot recover raw red/IR samples that firmware
  does not print.
- Only one process can own a serial port at a time. Close the other monitor before
  connecting; do not kill an unknown process or an ongoing capture automatically.
- Preserve partial captures after disconnects and clearly record their status.
  Keep original captures unchanged when producing later derived data.
- All of `data/` stays out of Git, including personal recordings and downloaded
  datasets. Use only publicly available external datasets under the current scope.
  Do not contact dataset authors on the user's behalf without explicit permission.
- Do not force-add ignored data, credentials, local environments, or editor state.
  Ignoring files is not a backup; arrange local-data backups separately.

## Setup on another computer

Keep this `AGENTS.md`, source, documentation, `Makefile`, `pyproject.toml`, and
`uv.lock` in Git so the workflow travels with the repository. `.vscode/`, `.pio/`,
`.venv/`, Python caches, and `data/` are local and ignored.

Install uv and a compatible Python (currently Python >=3.10). From the repo root:

```sh
make setup
make ports
make view
make record NAME=contact_test_001 SOURCE=sender DURATION=60
make test
```

Use the actual port on that computer when specifying `PORT`; never hardcode a
personal filesystem path or device identifier into shared code. Dependencies are
declared in `pyproject.toml` and locked in `uv.lock`; update them together when
intentionally changing dependencies. If Make is unavailable, use the direct uv
commands in `tools/README.md` (for setup: `uv sync --locked`).

PlatformIO is needed for firmware builds/uploads, not for the uv recording app.
After an approved firmware change, build the affected environment with
`pio run -e <environment>`. Building is separate from uploading.
For an authorized upload, use `make upload-sender PORT=<sender-port>` or
`make upload-receiver PORT=<receiver-port>`. Identify the physically correct board
before selecting its port; the target selects firmware but does not detect board
role. Close active monitors first. `PIO=/path/to/pio` overrides the executable.

## Verification

For recording-tool changes, run `make test` and any relevant focused checks.
Transport/file-integrity tests do not validate physiological accuracy. Only claim
a hardware test when it was actually performed. Documentation/ignore-only changes
need consistency and Git-ignore checks, not additional implementation tests.
Before handing off, review the diff and status to ensure local data and machine
configuration are excluded and unrelated user changes are preserved.

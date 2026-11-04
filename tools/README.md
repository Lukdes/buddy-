# Optional serial viewer and text recorder

This laptop app listens to the output already printed by Buddy's firmware. It
does not upload firmware, call the C++ programs, change sensor settings, or parse
and smooth readings. Firmware is built and uploaded separately from this app.

Close the PlatformIO Serial Monitor first with Ctrl+C. Only one application can
own a serial port at a time. Run these commands from the Buddy Minus folder.

## Shortcuts with uv

With `uv` installed, these commands automatically prepare the Python environment:

```sh
make view
make record
make record NAME=contact_test_001 SOURCE=sender
make record NAME=contact_test_002 DURATION=60 NOTES="Finger contact experiment"
make ports
```

Both modes display the board's existing output. Only `make record` saves files.
Use `PORT=/dev/cu.usbserial-02E4CA10` if automatic port selection is unsuitable.
`SOURCE` is a metadata label, not a firmware selector. Press Ctrl+C to finish.
`make` alone lists the commands. `make setup` explicitly installs the locked
dependencies; `make test` runs the existing transport and file-integrity checks.
The environment lives in the ignored `.venv/`; dependency versions are recorded
in `pyproject.toml` and `uv.lock`.

You can also run the app directly through uv:

```sh
uv run python tools/record.py
uv run python tools/record.py --record contact_test_001 --source sender
```

## Firmware upload shortcuts

Close the serial monitor or recorder before uploading. Connect the intended
board by USB, then run `make ports` to identify its port. With similar boards,
connect one at a time to establish which port belongs to which board.

```sh
make upload-sender PORT=/dev/cu.YOUR_SENDER_PORT
make upload-receiver PORT=/dev/cu.YOUR_RECEIVER_PORT
```

Replace the example ports with the actual ports from `make ports`. Each command
builds the selected firmware and uploads it in one step. The sender is the board
physically wired to the sensor; the receiver is the separate board receiving
wireless packets. Uploading to the battery-powered sender requires connecting
its USB cable temporarily. These commands cannot update it through the receiver.

Uploads require an explicit `PORT`. This prevents automatic port selection but
cannot verify the board's physical role: selecting the wrong port still uploads
the wrong firmware. `SOURCE` is only a recording label and does not affect uploads.
Viewing and recording remain separate commands and never trigger an upload.

The Makefile uses PlatformIO's usual `~/.platformio/penv/bin/pio` installation
when present, otherwise `pio` from PATH. Override the executable if needed:

```sh
make upload-sender PORT=/dev/cu.YOUR_SENDER_PORT PIO=/path/to/pio
```

After a successful upload, run `make view PORT=...` or `make record PORT=...`
using that board's port. No source edits or GitHub download are performed by
these commands; they upload the firmware built from the current local checkout.

For the normal two-board arrangement, keep sender firmware on the sensor/motor
board and receiver firmware on the separate USB-connected board. Run
`make view PORT=<receiver-port>` or `make record SOURCE=receiver PORT=<receiver-port>`
to read wireless measurements through the receiver. The motor is controlled
locally by the sender even when its USB cable is disconnected; it still needs
power. Do not upload sender firmware to the receiver to enable vibration. A
`Sensor not detected` message from the receiver indicates the wrong firmware is
running there. The receiver does not display the motor state in the current
packet format, so absence of a vibration field does not mean it is disabled.

## Direct commands using PlatformIO's Python

List devices:

```sh
~/.platformio/penv/bin/python tools/record.py --list-ports
```

View only, without saving:

```sh
~/.platformio/penv/bin/python tools/record.py --port /dev/cu.usbserial-02E4CA10
```

View and record:

```sh
~/.platformio/penv/bin/python tools/record.py --port /dev/cu.usbserial-02E4CA10 --record contact_test_001 --source sender
```

Use your device's actual port. `--source` is an optional user-supplied label for
the firmware already running; it does not select or flash firmware. It can be
`sender`, `receiver` or `heartrate`. Sender and receiver print BPM/SpO₂; heartrate
prints BPM only. The recorder saves whatever that board prints, including
unavailable readings, startup messages and errors. It cannot recover raw red/IR
waveforms or measurements that the firmware does not print.

Press Ctrl+C to stop. Add `--duration 60` to stop automatically after one minute.
You can omit `--port` when exactly one USB serial device is connected. The default
baud rate is 115200. Bare `--record` uses the name `capture`. Each recording gets a
unique folder and never overwrites another recording:

```text
data/raw_records/contact_test_001_<UTC timestamp>_<unique suffix>/
  serial.txt
  metadata.json
```

`serial.txt` contains exactly the bytes received from USB, with no timestamps or
headers inserted. It is a text log for the firmware's current ASCII output.
Metadata separately records laptop start/stop UTC, port, baud, source label,
notes, byte count, SHA-256 and completion status. Start/stop times are not optical
acquisition timestamps. Readings retain the sensor/estimator limitations of the
existing firmware; recording does not improve their accuracy.

Text is flushed to disk after every received chunk. On normal stop the file is
synced and metadata finalized. Disconnects leave an `incomplete` capture and do
not reconnect silently. A capture with no received bytes is marked `empty`. If
the process or laptop crashes, metadata may remain `recording`; treat that as an
unfinished capture. No guarantee is made against hardware power-loss data loss.

The app sends no serial commands and does not deliberately pulse reset lines.
Opening a serial port can nevertheless reset some boards. Recording begins after
the app connects, so earlier terminal output is not included.

All recordings are in the ignored `data/` folder. Keep a separate backup. In
view-only mode no recording folder or files are created. You may use PlatformIO's
ordinary monitor instead whenever recording is unnecessary.

## Five-second vibration alert (sender only)

The sender now uses a nonblocking five-second pulse for a fresh, valid SpO2
estimate **strictly below 97%**. Exactly 97% does not trigger it. A continuous low
reading produces only one pulse, not repeated vibration. After the pulse ends,
a valid reading at or above the threshold re-arms the alert for another low
episode. Recovery during a pulse does not shorten the five seconds. Invalid,
stale (no new estimate for more than two seconds), or lost-contact data stops
the pulse early and does not re-arm it. These conditions are not interpreted as
zero oxygen or as recovery. A restart resets the alert state.

`src/vibration.cpp` receives the sender's live SpO2 estimate, validity, and
calculation timestamp through `vibration::update(...)` on every loop. It runs
inside the sender firmware, not as a separate terminal program or firmware
environment. It consumes real sensor estimates; no synthetic values are injected
into this path. Upload with `make upload-sender PORT=<sender-port>`.

In `src/vibration.cpp`, `initialSpO2Threshold` controls the default threshold,
`vibrationPin` is set to GPIO 27, and `vibrationActiveHigh` is set to true. These
settings passed a bench test on the current module: GPIO 27 HIGH drove vibration
and LOW stopped it, confirmed by the user. The user reports VCC wired to 3V and
shared ground; full electrical specifications have not been independently verified.
Recheck wiring when changing hardware. No motor power should come directly from
a GPIO.

For later on-device algorithm integration, call
`vibration::setThreshold(percent)` from the sender loop context (include
`Vibration.h`). It rejects
nonfinite values and values outside (0, 100], leaving the previous setting intact.
It changes the trigger/recovery threshold without forcing a new pulse or clearing
the consumed-alert state. A host-to-device command protocol is not implemented.
The reusable policy lives in `include/SpO2Alert.h`; pulse duration is `pulseMs`.

Sender USB output records the motor state and threshold. The wireless packet and
receiver are unchanged, so this extra diagnostic is not visible on the receiver.
The alert does not modify the underlying SpO2 estimate or its validity flag.

Run `make test-alert` with a host C++ compiler installed to test synthetic readings
and timing without touching hardware. If macOS reports an SDK/linker architecture
mismatch, select an installed SDK compatible with the compiler through `SDKROOT`
for that command; this is a host-toolchain issue, not a firmware upload failure.
Tests include threshold boundaries, sustained
low readings, recovery/re-triggering, invalid inputs, threshold changes, and
millisecond timer rollover. Do not induce desaturation to test an alert. Physical
motor ON/OFF operation passed; precise physical timing, startup behavior under
different power conditions, and vibration interference with optical sensing need
further verification. This is an experimental alert, not a validated
safety alarm. Upload only the sender using the explicit port workflow above.

For a standalone motor bench test, the `vibration_test` environment uses labelled
synthetic 96% input, waits ten seconds after boot, and requests one five-second
pulse on GPIO 27. It reads no sensor and sends no radio packets. Each reset
(including one caused by opening a monitor) starts a new test. It replaces the
sender firmware until the normal sender is uploaded again:

```sh
pio run -e vibration_test -t upload --upload-port <sender-port>
make view PORT=<sender-port>
# After the test, stop the monitor and restore sensor-driven operation:
make upload-sender PORT=<sender-port>
```

## Sender sampling check

The sender reads matched red/IR samples from the FIFO and reports its processed
sample-pair rate in USB output, for example:

```text
Sample pairs/s=25.0 (expected 25; 25 pairs/1000 ms)
```

The rate includes pairs collected without finger contact. It measures processing
throughput over the reporting interval, not signal quality or individual sample
timing, and does not prove that no samples were lost. Small interval fluctuations
can occur; a persistently incorrect rate needs investigation because the BPM
algorithm assumes 25 sample pairs per second.

To upload this change, connect USB to the board physically wired to the sensor
and use `make upload-sender PORT=<sender-port>`. Leave the separate receiver
running receiver firmware. After upload, record from the sender's USB port:

```sh
make record NAME=sampling_check SOURCE=sender DURATION=60
```

Use `PORT=<sender-port>` when multiple USB devices are connected. `SOURCE` only
labels the capture. The sample-rate diagnostic is not transmitted to the receiver;
it requires the sender's USB connection. Compare resting BPM with the reference
device during the capture. Physiological accuracy still requires validation.

The Python bundled with PlatformIO already includes pyserial. The uv environment
gets pyserial from `pyproject.toml`. Run the transport/persistence tests with
`make test`, or with PlatformIO's Python:

```sh
~/.platformio/penv/bin/python -m unittest discover -s test -p 'test_*.py' -v
```

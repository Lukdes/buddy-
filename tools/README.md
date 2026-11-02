# Optional serial viewer and text recorder

This laptop app listens to the output already printed by Buddy's firmware. It
does not upload firmware, call the C++ programs, change sensor settings, or parse
and smooth readings. `heartrate.cpp`, `sender.cpp` and `receiver.cpp` are untouched.

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

The Python bundled with PlatformIO already includes pyserial. The uv environment
gets pyserial from `pyproject.toml`. Run the transport/persistence tests with
`make test`, or with PlatformIO's Python:

```sh
~/.platformio/penv/bin/python -m unittest discover -s test -p 'test_*.py' -v
```

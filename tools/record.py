#!/usr/bin/env python3
"""View existing Buddy serial output; optionally save the exact stream to text."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import re
import sys
import time
import uuid

DEFAULT_ROOT = Path(__file__).resolve().parents[1] / "data/raw_records"


def utc_now():
    return datetime.now(timezone.utc).isoformat(timespec="milliseconds")


class TextCapture:
    def __init__(self, root, name, port, baud, source, notes):
        if not re.fullmatch(r"[A-Za-z0-9_-]{1,80}", name):
            raise ValueError("recording name must use 1–80 letters, digits, underscores or hyphens")
        stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S_%fZ")
        self.directory = Path(root) / f"{name}_{stamp}_{uuid.uuid4().hex[:8]}"
        self.directory.mkdir(parents=True, exist_ok=False)
        self.path = self.directory / "serial.txt"
        self.file = self.path.open("xb")
        self.digest = hashlib.sha256()
        self.last_metadata_update = time.monotonic()
        self.metadata = {
            "format_version": 1,
            "recording_name": name,
            "started_utc": utc_now(),
            "status": "recording",
            "port": port,
            "baud": baud,
            "source_label": source,
            "source_label_note": "User-supplied label; firmware is not detected or changed.",
            "notes": notes,
            "file": "serial.txt",
            "bytes_recorded": 0,
            "timing_note": "Start/stop times are laptop UTC. Original output has no added per-line or optical-acquisition timestamps.",
        }
        try:
            self.save_metadata()
        except OSError:
            self.file.close()
            raise

    def save_metadata(self):
        temporary = self.directory / "metadata.json.tmp"
        temporary.write_text(json.dumps(self.metadata, indent=2) + "\n", encoding="utf-8")
        temporary.replace(self.directory / "metadata.json")

    def write(self, data):
        self.file.write(data)
        self.file.flush()
        self.digest.update(data)
        self.metadata["bytes_recorded"] += len(data)
        if time.monotonic() - self.last_metadata_update >= 1:
            self.save_metadata()
            self.last_metadata_update = time.monotonic()

    def close(self, status, reason):
        try:
            self.file.flush()
            os.fsync(self.file.fileno())
        finally:
            self.file.close()
        self.metadata.update(
            stopped_utc=utc_now(), status=status, stop_reason=reason,
            sha256=self.digest.hexdigest(),
        )
        self.save_metadata()


def show(data, output):
    # A real terminal receives the original bytes. Text-only output sinks get a
    # readable replacement for invalid UTF-8; recording bytes are never decoded.
    binary = getattr(output, "buffer", None)
    if binary is not None:
        binary.write(data)
        binary.flush()
    else:
        output.write(data.decode("utf-8", errors="replace"))
        output.flush()


def run_session(serial_factory, port, *, baud=115200, record_name=None,
                source_label="unspecified", notes="", duration=None,
                output_root=DEFAULT_ROOT, output=None, messages=None):
    output = sys.stdout if output is None else output
    messages = sys.stderr if messages is None else messages
    connection = None
    capture = None
    status, reason, code = "completed", "duration_reached", 0
    try:
        options = dict(port=None, baudrate=baud, timeout=0.2)
        if os.name == "posix":
            options["exclusive"] = True
        connection = serial_factory(**options)
        # No commands are sent to the board and no deliberate reset pulse occurs.
        connection.dtr = False
        connection.rts = False
        connection.port = port
        connection.open()
        if record_name is not None:
            capture = TextCapture(output_root, record_name, port, baud, source_label, notes)
            print(f"Recording to {capture.path}\nPress Ctrl+C to stop and save.", file=messages, flush=True)
        else:
            print(f"Viewing {port} at {baud} baud. Nothing is being saved. Ctrl+C to stop.", file=messages, flush=True)
        started = time.monotonic()
        while duration is None or time.monotonic() - started < duration:
            data = connection.read(min(4096, connection.in_waiting or 1))
            if data:
                if capture is not None:
                    capture.write(data)
                show(data, output)
    except KeyboardInterrupt:
        reason = "user_stopped"
    except Exception as error:
        status, reason, code = "incomplete", f"{type(error).__name__}: {error}", 2
        print(f"Serial session error: {error}\nClose other serial monitors and check the USB connection/port.", file=messages)
    finally:
        if connection is not None:
            try:
                connection.close()
            except Exception as error:
                status, reason, code = "incomplete", f"Serial close failed: {error}", 2
                print(reason, file=messages)
        if capture is not None:
            if status == "completed" and capture.metadata["bytes_recorded"] == 0:
                status = "empty"
            try:
                capture.close(status, reason)
                print(f"\nSaved {capture.metadata['bytes_recorded']} bytes. Status: {status}.\n{capture.path}", file=messages)
            except OSError as error:
                print(f"Could not finalize recording: {error}. Inspect {capture.directory} before using it.", file=messages)
                code = 2
    return code


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", help="Serial port; auto-selected only when exactly one USB serial device is connected")
    parser.add_argument("--list-ports", action="store_true")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--record", nargs="?", const="capture", metavar="NAME",
                        help="Save text plus metadata; optional recording name. Omit for view-only mode.")
    parser.add_argument("--source", choices=("sender", "receiver", "heartrate", "unspecified"),
                        default="unspecified", help="Label the firmware already running; does not upload or select firmware")
    parser.add_argument("--notes", default="")
    parser.add_argument("--duration", type=float, help="Stop after this many seconds; otherwise use Ctrl+C")
    args = parser.parse_args(argv)
    if args.baud <= 0:
        parser.error("baud must be positive")
    if args.duration is not None and (not math.isfinite(args.duration) or args.duration <= 0):
        parser.error("duration must be positive and finite")
    if args.record is not None and not re.fullmatch(r"[A-Za-z0-9_-]{1,80}", args.record):
        parser.error("recording name must use 1–80 letters, digits, underscores or hyphens")
    try:
        import serial
        from serial.tools import list_ports
    except ImportError:
        print("pyserial is missing. Use 'make view' / 'make record', or run with ~/.platformio/penv/bin/python.", file=sys.stderr)
        return 2
    ports = list(list_ports.comports())
    if args.list_ports:
        for port in ports:
            print(f"{port.device}\t{port.description}")
        return 0
    port = args.port
    if not port:
        candidates = [p.device for p in ports if p.vid is not None]
        if len(candidates) != 1:
            parser.error("specify --port; use --list-ports to see available devices")
        port = candidates[0]
    return run_session(serial.Serial, port, baud=args.baud, record_name=args.record,
                       source_label=args.source, notes=args.notes, duration=args.duration)


if __name__ == "__main__":
    raise SystemExit(main())

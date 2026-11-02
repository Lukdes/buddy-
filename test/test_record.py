"""Transport and file-integrity checks using synthetic serial bytes only."""
import csv
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import tempfile
import unittest

path = Path(__file__).resolve().parents[1] / "tools/record.py"
spec = importlib.util.spec_from_file_location("buddy_record", path)
record = importlib.util.module_from_spec(spec)
spec.loader.exec_module(record)


class FakeSerial:
    def __init__(self, chunks, open_error=None):
        self.chunks = iter(chunks)
        self.open_error = open_error
        self.closed = False
        self.writes = []

    @property
    def in_waiting(self):
        return 4096

    def open(self):
        if self.open_error:
            raise self.open_error

    def read(self, size):
        try:
            value = next(self.chunks)
        except StopIteration:
            raise KeyboardInterrupt
        if isinstance(value, BaseException):
            raise value
        return value

    def close(self):
        self.closed = True


class RecorderTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name) / "raw_records"
        self.console_bytes = io.BytesIO()
        self.console = io.TextIOWrapper(self.console_bytes, encoding="utf-8")
        self.messages = io.StringIO()

    def tearDown(self):
        self.console.close()
        self.temp.cleanup()

    def run_capture(self, chunks, name=None, **kwargs):
        self.serial = FakeSerial(chunks, **kwargs)
        return record.run_session(lambda **options: self.serial, "synthetic-port",
                                  record_name=name, source_label="sender", notes="Synthetic test only",
                                  output_root=self.root, output=self.console, messages=self.messages)

    def capture_files(self):
        directory = next(self.root.iterdir())
        return directory, (directory / "serial.txt").read_bytes(), json.loads((directory / "metadata.json").read_text())

    def test_view_only_displays_output_without_creating_recording_files(self):
        original = b"IR=12345 | Place finger on sensor\r\n"
        self.assertEqual(self.run_capture([original]), 0)
        self.assertFalse(self.root.exists())
        self.assertEqual(self.console_bytes.getvalue(), original)
        self.assertTrue(self.serial.closed)

    def test_recording_preserves_exact_partial_lines_bytes_and_metadata(self):
        chunks = [b"Packet 1 | IR=123", b"45 | BPM calculating/unavailable\r\n",
                  b"binary-byte:\xff\npartial-without-newline"]
        expected = b"".join(chunks)
        self.assertEqual(self.run_capture(chunks, "contact_001"), 0)
        _, actual, metadata = self.capture_files()
        self.assertEqual(actual, expected)
        self.assertEqual(self.console_bytes.getvalue(), expected)
        self.assertEqual(metadata["bytes_recorded"], len(expected))
        self.assertEqual(metadata["sha256"], hashlib.sha256(expected).hexdigest())
        self.assertEqual(metadata["status"], "completed")
        self.assertEqual(metadata["stop_reason"], "user_stopped")
        self.assertEqual(metadata["port"], "synthetic-port")
        self.assertEqual(metadata["source_label"], "sender")
        self.assertTrue(metadata["started_utc"].endswith("+00:00"))
        self.assertIn("stopped_utc", metadata)

    def test_disconnect_retains_received_data_and_marks_incomplete(self):
        expected = b"Estimated BPM=72 | Estimated SpO2=98%\n"
        self.assertEqual(self.run_capture([expected, OSError("synthetic unplug")], "disconnect"), 2)
        _, actual, metadata = self.capture_files()
        self.assertEqual(actual, expected)
        self.assertEqual(metadata["status"], "incomplete")
        self.assertIn("synthetic unplug", metadata["stop_reason"])
        self.assertTrue(self.serial.closed)

    def test_busy_port_creates_no_recording(self):
        self.assertEqual(self.run_capture([], "busy", open_error=OSError("port locked")), 2)
        self.assertFalse(self.root.exists())
        self.assertIn("Close other serial monitors", self.messages.getvalue())

    def test_empty_capture_is_explicit_and_not_a_successful_measurement(self):
        self.assertEqual(self.run_capture([], "empty"), 0)
        _, actual, metadata = self.capture_files()
        self.assertEqual(actual, b"")
        self.assertEqual(metadata["status"], "empty")
        self.assertEqual(metadata["bytes_recorded"], 0)

    def test_same_name_never_overwrites_an_earlier_recording(self):
        self.run_capture([b"first\n"], "same")
        self.run_capture([b"second\n"], "same")
        files = sorted(p / "serial.txt" for p in self.root.iterdir())
        self.assertEqual(len(files), 2)
        self.assertEqual({p.read_bytes() for p in files}, {b"first\n", b"second\n"})

    def test_path_traversal_name_is_rejected_before_creating_files(self):
        with self.assertRaises(ValueError):
            record.TextCapture(self.root, "../outside", "synthetic-port", 115200, "sender", "")
        self.assertFalse(self.root.exists())


if __name__ == "__main__":
    unittest.main()

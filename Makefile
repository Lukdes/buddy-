.DEFAULT_GOAL := help
.PHONY: help setup view record ports test test-alert upload-sender upload-receiver

UV ?= uv
export PIO ?= $(if $(wildcard $(HOME)/.platformio/penv/bin/pio),$(HOME)/.platformio/penv/bin/pio,pio)
export NAME ?= capture
export PORT ?=
export SOURCE ?= unspecified
export NOTES ?=
export DURATION ?=

# Pass values through environment variables so names/notes remain single shell
# arguments. An empty port lets the app auto-select one USB serial device.
SERIAL_ARGS = --port "$$PORT" --source "$$SOURCE" --notes "$$NOTES" $(if $(strip $(DURATION)),--duration "$$DURATION")

help:
	@printf '%s\n' \
	  'make view                         Display only; does not enable vibration or save files' \
	  'make record                       Display and save a new text recording' \
	  'make record NAME=contact_test_001  Choose a recording name' \
	  'make ports                        List serial ports' \
	  'make test                         Run transport/file-integrity tests' \
	  'make test-alert                   Simulate vibration logic without hardware' \
	  'make setup                        Install the locked Python dependencies' \
	  'make upload-sender PORT=...       Enable automatic vibration by uploading sender firmware' \
	  'make upload-receiver PORT=...     Build and flash the separate receiver board' \
	  '' \
	  'Vibration: connect USB to the sensor-connected breadboard board, then upload-sender.' \
	  'After a successful upload, valid SpO2 below the configured threshold (default 97%)' \
	  'triggers one five-second pulse. Recovery re-arms it; invalid/stale data stops it early.' \
	  'It runs on the board even when make view is closed. No separate enable command is needed.' \
	  'make test-alert only simulates the logic; it does not activate the physical motor.' \
	  '' \
	  'Optional: PORT=/dev/cu.usbserial-... SOURCE=sender DURATION=60 NOTES="..."' \
	  'Close other serial monitors first. Press Ctrl+C to stop.'

setup:
	@$(UV) sync --locked

view:
	@$(UV) run python tools/record.py $(SERIAL_ARGS)

record:
	@$(UV) run python tools/record.py $(SERIAL_ARGS) --record "$$NAME"

ports:
	@$(UV) run python tools/record.py --list-ports

test:
	@$(UV) run python -m unittest discover -s test -p 'test_*.py' -v

test-alert:
	@alert_test=$$(mktemp "$${TMPDIR:-/tmp}/buddy-alert.XXXXXX") || exit 1; \
	trap 'rm -f "$$alert_test"' EXIT HUP INT TERM; \
	$(CXX) -std=c++11 -Wall -Wextra -Werror -Iinclude test/test_spo2_alert.cpp -o "$$alert_test" && \
	"$$alert_test" && printf '%s\n' 'SpO2 vibration policy tests passed (simulated readings; no hardware).'

# Require a port instead of letting PlatformIO choose the other board silently.
# The target selects firmware; PORT must identify the physically correct board.
# upload-sender includes src/vibration.cpp: after successful upload the board
# automatically drives the motor for valid low SpO2. view/record only read output.
upload-sender upload-receiver:
	@if [ -z "$$PORT" ]; then \
	  printf '%s\n' 'PORT is required for uploads. Run make ports, identify the board, then retry with PORT=...'; \
	  exit 2; \
	fi
	@printf 'Uploading %s firmware to %s. Close serial monitors first.\n' '$(patsubst upload-%,%,$@)' "$$PORT"
	@"$$PIO" run -e $(patsubst upload-%,%,$@) -t upload --upload-port "$$PORT"

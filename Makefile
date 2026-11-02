.DEFAULT_GOAL := help
.PHONY: help setup view record ports test

UV ?= uv
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
	  'make view                         Display only; save nothing' \
	  'make record                       Display and save a new text recording' \
	  'make record NAME=contact_test_001  Choose a recording name' \
	  'make ports                        List serial ports' \
	  'make test                         Run transport/file-integrity tests' \
	  'make setup                        Install the locked Python dependencies' \
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

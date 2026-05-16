# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

DIY ESP32-based cistern water-level monitor. PlatformIO project targeting `esp32dev` with the Arduino framework (see [platformio.ini](platformio.ini)). At present the firmware in [src/main.cpp](src/main.cpp) is only the PlatformIO scaffold — there is no real application logic yet, so most "architecture" decisions are still open.

## Commands

Build, upload, and monitor go through PlatformIO (the VS Code extension or `pio` CLI):

- Build: `pio run`
- Upload to a connected ESP32: `pio run -t upload`
- Serial monitor: `pio device monitor`
- Clean: `pio run -t clean`
- Run unit tests (PlatformIO's Unity-based runner, native + on-device): `pio test`
- Run a single test environment: `pio test -e esp32dev -f <test_name>`

There is no separate lint step configured.

## Layout conventions (PlatformIO)

- [src/](src/) — application sources; entry point is [src/main.cpp](src/main.cpp) with the standard Arduino `setup()` / `loop()`.
- [include/](include/) — project-wide headers.
- [lib/](lib/) — private libraries; each subdirectory under `lib/` is treated as its own library by PlatformIO's Library Dependency Finder. Prefer adding new modules here (e.g. `lib/sensor/`, `lib/wifi/`) rather than piling files into `src/`.
- [test/](test/) — PlatformIO/Unity tests.
- [platformio.ini](platformio.ini) — single source of truth for board, framework, build flags, and `lib_deps`. Add libraries via `lib_deps` rather than vendoring.

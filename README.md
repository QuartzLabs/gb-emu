# gb-emu

A Game Boy emulator written in C++.

## Current status

This is the first project step.

Implemented so far:
- ROM loading
- Basic cartridge header parsing
- Minimal memory bus
- Minimal CPU bootstrap
- Basic instruction stepping for early testing

## Build

Requirements:
- CMake 3.20+
- A C++20 compiler

Build example:

```bash
cmake -S . -B build
cmake --build build
```

## Run

Current builds use a ROM path as a command line argument:

```bash
gb-emu path/to/rom.gb
```

## Notes

This project is currently in an early bootstrap stage.
The current executable is mainly intended for testing cartridge loading and CPU stepping.

## License

MIT
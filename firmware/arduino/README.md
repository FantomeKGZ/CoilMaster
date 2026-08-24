# CoilMaster Arduino Uno firmware

## Current build entry point

- `src/main.cpp`

During the safe migration period the build still compiles shared implementation files from the repository-level `Core/` and `Arduino/` directories. They will be moved only after every intermediate commit passes the Arduino Uno CI build.

## Target structure

```text
firmware/arduino/
├── src/          # application entry point and target-specific sources
├── include/      # public target configuration and interfaces
├── lib/          # private modules/drivers
└── test/         # host and embedded tests
```

## Migration rules

1. Keep the build green after each move.
2. Never move all modules in one commit.
3. Move Core modules before hardware adapters.
4. Update include paths and PlatformIO filters in the same commit.
5. Remove legacy files only after the new copies compile successfully.
6. Do not enable the real SSR in simulation builds.


## Temporary USB reset-loop diagnostic

The normal `uno` environment remains the production image with verbose diagnostics disabled. For the bounded hardware investigation use:

```text
pio run -e uno_diagnostic -t upload
pio device monitor -b 115200
```

In VS Code, select the PlatformIO environment `uno_diagnostic`, upload it to the Uno, then open Serial Monitor at `115200`. Capture from the first `CM_BOOT reset_flags=` line through the next restart. The image keeps SSR fail-safe OFF during boot and does not add automatic START or reboot resume.

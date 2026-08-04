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

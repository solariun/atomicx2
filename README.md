Initial

Arduino test

for nano:
arduino-cli compile --upload  --fqbn arduino:avr:nano --programmer usbasp simple


## Run on the Nano emulator

[nanoemu](https://github.com/solariun/nanoemu) is linked as a git submodule in `nanoemu/`. After cloning, run `git submodule update --init` (the Makefile also does this on first use).

```bash
make nano_bin                 # compile arduino/simple with arduino-cli into arduino/bin (.hex .bin .elf)
make nanoemu                  # compile and run it in the emulator's TUI
make nanoemu_headless SECS=3  # no UI: 3 simulated seconds, serial output to stdout
make nanoemu SKETCH=other     # use arduino/other instead of arduino/simple
```

On Apple Silicon without Rosetta, install the native compiler from [arduino-avr-arm64](https://github.com/solariun/arduino-avr-arm64) so arduino-cli can build.

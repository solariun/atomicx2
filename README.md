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
make nanoemu_serial           # no UI, real time: serial on stdout, what you type goes to the sketch; Ctrl-C stops
make nanoemu SKETCH=other     # use arduino/other instead of arduino/simple
make nanoemu BREAK=simple.ino:56   # stop at a line (or a function, or 0xADDR)
```

The emulator runs the `.elf`, so it shows the source line being executed, the current function and the values of the globals used on that line.

On Apple Silicon without Rosetta, install the native compiler from [arduino-avr-arm64](https://github.com/solariun/arduino-avr-arm64) so arduino-cli can build.

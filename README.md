# TinyMOD - Amiga MOD File Player

TinyMOD is a high-fidelity Amiga MOD (Protracker format) file player that authentically recreates the sound characteristics of original Amiga hardware through software emulation of the Paula audio chip.

## Features

- **Authentic Paula Chip Emulation**: Faithfully replicates the Amiga Paula chip sound by operating at the original master clock rate (3.5 MHz) and downsampling to standard output rates
- **Protracker Support**: Full support for MOD file format with all standard effects
- **High-Quality Resampling**: Uses windowed-sinc FIR filtering for excellent audio quality
- **4-Channel Audio**: Stereo output with proper channel mixing and panning
- **Effect Processing**: Complete MOD effect support including:
  - Vibrato and tremolo
  - Pitch slides and portamento
  - Volume slides
  - Arpeggio
  - Pattern looping and breaking
  - And more...

## Building

### Requirements

- GCC/G++ compiler
- PortAudio library (libportaudio.a)
- ALSA development libraries (for Linux)
- Make

### Compilation

```bash
make
```

The Makefile will:
- Compile the modular source files
- Link with PortAudio and system libraries
- Create the `tinymod` executable

### Cleanup

```bash
make clean
```

## Usage

### Playing a MOD File

```bash
./tinymod music.mod
```

### Getting Help

```bash
./tinymod --help
```

### About

```bash
./tinymod --about
```

## Architecture

The codebase is organized into modular components:

### Core Modules

- **`src/types.h`**: Type definitions, memory utilities, and mathematical functions
- **`src/config.h`**: Centralized configuration constants
- **`src/paula.h`/`src/paula.cpp`**: Amiga Paula chip emulator
- **`src/modplayer.h`/`src/modplayer.cpp`**: MOD file parser and playback engine
- **`src/main.cpp`**: Command-line interface and audio system integration

### Design Principles

- **Modular Design**: Each component has a single responsibility
- **Well-Documented**: Extensive comments explaining algorithms and MOD format details
- **Portable**: Platform-independent code with PortAudio for audio I/O
- **Authentic**: Preserves original Paula chip behavior and MOD effect processing

## Technical Details

### Paula Chip Emulation

The Paula emulator processes audio at the original Amiga clock rate (3,740,000 Hz) and applies:
- PWM (Pulse Width Modulation) for sample playback
- Ring buffer for sample storage
- Windowed-sinc FIR filter for high-quality resampling

### MOD Format Support

Supports the following MOD file variants:
- Standard MOD (4 channels, 16-32 samples)
- M.K. format (32 samples)
- FLT4 (Startrekker, 32 samples)
- M!K! (extended patterns, 32 samples)

### Effect Processing

All major Protracker effects are implemented:
- 0x: Arpeggio
- 1x: Slide up
- 2x: Slide down
- 3x: Tone portamento
- 4x: Vibrato
- 5x: Tone portamento + volume slide
- 6x: Vibrato + volume slide
- 7x: Tremolo
- 9x: Sample offset
- Ax: Volume slide
- Bx: Position jump
- Cx: Set volume
- Dx: Pattern break
- Ex: Extended effects
- Fx: Set speed/BPM

## Authors

- **Tammo "kb" Hinrichs** - Original Paula emulator implementation (2007)
- **Jason Bou-samra** - Code refactoring, modularization, and PortAudio integration (2024)

## License

This software is released into the **public domain**. Use, distribute, and modify freely without restriction.

## Notes

- High-quality Paula emulation requires significant CPU resources due to the 3.5 MHz -> 48 KHz resampling ratio
- Playback duration is configurable via NUM_SECONDS constant in config.h
- Audio output can be customized through PortAudio configuration

## References

- Protracker MOD Format Documentation
- Amiga Hardware Specifications
- Paula Chip Audio Hardware Documentation

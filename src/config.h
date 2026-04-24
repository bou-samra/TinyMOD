// =================== Configuration Constants ===================
// All configuration defines for TinyMOD in one central location

#ifndef CONFIG_H
#define CONFIG_H

// === Audio Configuration ===
#define NUM_SECONDS         (1000)       // Duration to play (in seconds)
#define SAMPLE_RATE         (96000)      // Playback sample rate (Hz)
#define FRAMES_PER_BUFFER   (0x10000)    // Audio buffer size (65536 frames)

// === Paula Chip Emulation ===
// These constants define the Amiga Paula chip parameters
const int PAULARATE = 3740000;             // Paula chip master clock (~3.546895MHz DAC base clock)
const int OUTRATE = 48000;                 // Output/playback rate (48KHz)
const int OUTFPS = 50;                     // Frames per second (50Hz - PAL)

// === Paula Ring Buffer ===
const int PAULA_RBSIZE = 4096;             // Paula ring buffer (circular buffer) size
const int PAULA_FIR_WIDTH = 512;           // Finite Impulse Response (FIR) filter width

// === MOD Format Constants ===
const int MOD_CHANNELS = 4;                // Standard MOD files have 4 channels
const int MOD_SAMPLES = 32;                // Maximum 32 samples per MOD file
const int MOD_PATTERNS = 128;              // Maximum 128 patterns per MOD file
const int MOD_PATTERN_ROWS = 64;           // 64 rows per pattern
const int MOD_PATTERN_SIZE = 1024;         // 1024 bytes per pattern

// === Utility Macros ===
#define cls()               printf("\033[H\033[J")  // ANSI escape codes to clear screen

#endif // CONFIG_H

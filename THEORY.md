`TinyMOD` is a sophisticated Protracker module player that prioritizes "accuracy" over performance. Unlike modern players that use simple linear interpolation to play samples, this code emulates the physical behavior of the Amiga’s **Paula** custom chip.

Here is a detailed breakdown of the principles of operation, divided into three layers: the **Paula Emulation**, the **ModPlayer Logic**, and the **Resampling/Filtering**.

---

### 1. The Paula Emulation Layer (The "Hardware")
The core of this code is the `Paula` class. In a real Amiga, the Paula chip didn't have a fixed sample rate. Instead, it changed the speed at which it read from memory to change the pitch.

#### The Pulse-Width Modulation (PWM) Emulation
Inside `Voice::Render`, you see this logic:
```cpp
if (PWMCnt < Volume) buffer[i] += Cur.F32;
PWMCnt = (PWMCnt + 1) & 0x3f;
```
This is a very clever way to emulate the Amiga's volume control. Instead of just multiplying the sample by a volume fraction (0.0 to 1.0), the code uses a high-speed counter (`PWMCnt`). If the counter is below the `Volume` value (0–64), the signal is "on." This mimics how some hardware digital-to-analog converters (DACs) handle amplitude.

#### The Timing (Paula Rate)
The code operates internally at `PAULARATE` (approx. 3.74 MHz). This represents the master clock of the Amiga. 
* **`DivCnt`**: This acts as a countdown timer. When it hits zero, the next 8-bit sample is fetched from memory. 
* **`Period`**: This is the value from the MOD file that determines pitch. A smaller period means the counter hits zero faster, resulting in a higher pitch.

---

### 2. The Resampling & FIR Filtering (The "Magic")
Because the internal "Paula" speed is 3.7 MHz and your sound card likely runs at 48 kHz, the code must downsample the audio. If you simply picked every 78th sample, the audio would sound terrible due to **aliasing** (high-frequency noise folding back into the audible range).

#### The Sinc Filter
To solve this, `kb` implemented a **Finite Impulse Response (FIR) Filter** using a **Windowed Sinc** function.


* **`FIRMem`**: In the `Paula` constructor, it pre-calculates a Sinc table windowed with a Hamming window.
* **`RingBuf`**: The player renders the raw, "noisy" 3.7 MHz audio into a large ring buffer.
* **The Filter Loop**: In `Paula::Render`, the code slides the FIR window over the ring buffer. It performs a convolution (multiplying sample values by the Sinc weights). This acts as a near-perfect low-pass filter, removing all the high-frequency garbage and leaving only a clean, "Amiga-accurate" sound.

---

### 3. The ModPlayer Layer (The "Sequencer")
The `ModPlayer` class is the "brain" that reads the Protracker data (notes and effects) and tells the `Paula` class what to do.

#### Period and Vibrato Tables
* **`BasePTable`**: Contains the Amiga "Periods" for notes (C-1 to B-5).
* **`PTable`**: A pre-computed table for all 16 "Finetune" settings. This prevents the code from having to do complex math during playback.
* **`VibTable`**: Pre-computed Sine, Sawtooth, and Square waves for the Vibrato and Tremolo effects.

#### The Tick System
Music in a MOD file is not measured in milliseconds but in **Ticks**.
* **Speed**: Defines how many ticks are in a "Row" (usually 6).
* **Tick 0**: This is where "trigger" events happen (starting a new note, changing a sample).
* **Ticks 1+**: This is where "continuous" effects happen (pitch slides, vibrato, volume slides).

#### Effect Handling
The `Tick()` function is a massive switch statement that handles the Protracker command set:
* **Effect 3**: Tone Portamento (sliding from one note to another).
* **Effect 9**: Sample Offset (starting a sample from the middle).
* **Effect E (14)**: The "Special" sub-commands like Pattern Loop or Retrigger.

---

### 4. Data Flow Summary

1.  **Initialization**: `ModPlayer` parses the MOD file, prepares sample loops, and pre-calculates the Period and Vibrato tables. `Paula` calculates the FIR filter coefficients.
2.  **The Request**: The Sound API (DirectSound/PortAudio) asks for a buffer of audio (e.g., 480 words).
3.  **The Sequencing**: `ModPlayer::Render` checks if it's time for a new **Tick**. If so, it calls `Tick()`, updating the `Volume` and `Period` for the 4 hardware voices.
4.  **Emulation**: `Paula::Render` sees how many output samples are needed. It calculates the "step" (how far to move in the 3.7 MHz buffer for every 48 kHz output sample).
5.  **Filtering**: The FIR filter convolutes the raw high-speed data into the output buffer, applying the `MasterSeparation` (Stereo pan) and `MasterVolume`.
6.  **Output**: The resulting floating-point data is sent to the speakers.

### Why is this code unique?
Most players use **Linear Interpolation**. It’s fast but "blurs" the sound. This code emulates the **Zero-Order Hold** (the "stepping" nature of the DAC) and then uses a high-quality brick-wall filter to clean it up. This results in the "crisp" yet "warm" sound characteristic of real Amiga hardware.

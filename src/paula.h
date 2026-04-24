// =================== Paula Chip Emulator ===================
// Emulates the Amiga Paula chip audio hardware
// Faithfully recreates the sound of the Amiga by resampling
// at the Paula master clock (3.5 MHz) and downsampling to output rate

#ifndef PAULA_H
#define PAULA_H

#include "types.h"
#include "config.h"

// =================== Paula Class ===================
// Represents the Amiga Paula audio chip emulator
class Paula
{
public:
    // === FIR Filter Configuration ===
    static const sInt FIR_WIDTH = 512;     // Finite Impulse Response filter width
    sF32 FIRMem[2 * FIR_WIDTH + 1];       // FIR filter coefficients (1025 taps)

    // =================== Voice Structure ===================
    // Represents a single audio channel (Paula has 4 voices)
    struct Voice
    {
    private:
        sInt Pos;                          // Current sample position in waveform
        sInt PWMCnt, DivCnt;               // PWM counter and period divider
        sIntFlt Cur;                       // Current sample value (float/int union)

    public:
        sS8 *Sample;                       // Pointer to sample data
        sInt SampleLen;                    // Total sample length in words
        sInt LoopLen;                      // Loop length in words
        sInt Period;                       // Audio period (sample playback rate)
        sInt Volume;                       // Volume (0-64)

        // Voice constructor: initialize all values to default/zero
        Voice()
            : Period(65535), Volume(0), Sample(0), Pos(0), PWMCnt(0), DivCnt(0), LoopLen(1)
        {
            Cur.F32 = 0;
        }

        // Render voice samples into output buffer
        // Uses PWM (Pulse Width Modulation) to convert sample data
        void Render(sF32 *buffer, sInt samples);

        // Trigger voice: start playing a sample
        // smp: pointer to sample data
        // sl: sample length in words
        // ll: loop length in words
        // offs: offset into sample (default 0)
        void Trigger(sS8 *smp, sInt sl, sInt ll, sInt offs = 0);
    };

    Voice V[4];                            // Array of 4 voices (Paula has 4 audio channels)

    // =================== Ring Buffer ===================
    // Circular buffer stores audio samples at Paula rate before resampling
    static const sInt RBSIZE = 4096;       // Ring buffer size in samples
    sF32 RingBuf[2 * RBSIZE];             // Stereo ring buffer (left + right channels)
    sInt WritePos;                         // Current write position in ring buffer
    sInt ReadPos;                          // Current read position in ring buffer
    sF32 ReadFrac;                         // Fractional position for interpolation

    // Generate audio fragments at Paula rate (3.74 MHz)
    // This is where the actual Paula emulation happens
    void CalcFrag(sF32 *out, sInt samples);

    // Calculate and fill ring buffer with new Paula-rate samples
    void Calc();

    // =================== Output Rendering ===================
    // Master volume control (0.0 = silent, 1.0 = full volume)
    sF32 MasterVolume;

    // Stereo separation control (0.0 = mono, 1.0 = full stereo)
    sF32 MasterSeparation;

    // Resample from Paula rate to output rate and apply FIR filter
    // Uses windowed-sinc FIR filtering for high-quality resampling
    void Render(sF32 *outbuf, sInt samples);

    // Paula constructor: initialize FIR filter and ring buffer
    Paula();
};

#endif // PAULA_H

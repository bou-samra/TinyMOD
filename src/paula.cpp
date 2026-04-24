// =================== Paula Chip Emulator Implementation ===================
// Implementation of the Amiga Paula chip audio hardware emulator

#include "paula.h"
#include <cstring>

// =================== Voice::Render ===================
// Render voice samples into output buffer using PWM
void Paula::Voice::Render(sF32 *buffer, sInt samples)
{
    if (!Sample)
        return;  // No sample data, nothing to render

    sU8 *smp = (sU8 *)Sample;
    for (sInt i = 0; i < samples; i++)
    {
        if (!DivCnt)
        {
            // Load next sample: convert from 8-bit unsigned to 32-bit float
            // XOR with 0x80 converts unsigned to signed format
            // Shift left 15 bits and OR with mantissa to create float representation
            Cur.U32 = ((smp[Pos] ^ 0x80) << 15) | 0x40000000;
            Cur.F32 -= 3.0f;  // Normalize to proper range

            // Advance to next sample, handle looping
            if (++Pos == SampleLen)
                Pos -= LoopLen;  // Jump back to loop start

            DivCnt = Period;  // Reset period counter
        }

        // PWM (Pulse Width Modulation) output
        // Only output if PWM counter is below volume level
        if (PWMCnt < Volume)
            buffer[i] += Cur.F32;

        PWMCnt = (PWMCnt + 1) & 0x3f;  // 6-bit PWM counter (0-63)
        DivCnt--;  // Decrement period counter
    }
}

// =================== Voice::Trigger ===================
// Trigger a voice to start playing a sample
void Paula::Voice::Trigger(sS8 *smp, sInt sl, sInt ll, sInt offs)
{
    Sample = smp;                          // Set sample pointer
    SampleLen = sl;                        // Set sample length
    LoopLen = ll;                          // Set loop length
    Pos = sMin(offs, SampleLen - 1);       // Set start position (clamped)
}

// =================== Paula::CalcFrag ===================
// Generate audio fragments at Paula rate
// This function renders all 4 voice channels into the output buffer
void Paula::CalcFrag(sF32 *out, sInt samples)
{
    // Zero out output buffer (stereo: 2 channels)
    sZeroMem(out, sizeof(sF32) * samples);
    sZeroMem(out + RBSIZE, sizeof(sF32) * samples);

    // Render each of the 4 Paula voices
    for (sInt i = 0; i < 4; i++)
    {
        // Paula has stereo hardwired:
        // Voices 0,3 go to left channel
        // Voices 1,2 go to right channel
        if (i == 1 || i == 2)
            V[i].Render(out + RBSIZE, samples);  // Right channel
        else
            V[i].Render(out, samples);            // Left channel
    }
}

// =================== Paula::Calc ===================
// Fill ring buffer with new samples at Paula rate
void Paula::Calc()
{
    // Calculate number of samples needed
    sInt RealReadPos = ReadPos - FIR_WIDTH - 1;
    sInt samples = (RealReadPos - WritePos) & (RBSIZE - 1);

    // Generate samples in two chunks if wrapping around ring buffer
    sInt todo = sMin(samples, RBSIZE - WritePos);
    CalcFrag(RingBuf + WritePos, todo);

    if (todo < samples)
    {
        WritePos = 0;
        todo = samples - todo;
        CalcFrag(RingBuf, todo);
    }

    WritePos += todo;
}

// =================== Paula::Render ===================
// Resample from Paula rate (3.74 MHz) to output rate (48 KHz)
// Uses windowed-sinc FIR filtering for high-quality resampling
void Paula::Render(sF32 *outbuf, sInt samples)
{
    // Calculate resampling ratio
    const sF32 step = sF32(PAULARATE) / sF32(OUTRATE);  // ~77.92

    // Calculate stereo panning coefficients
    // Maintains constant power panning: vol_L^2 + vol_R^2 = constant
    const sF32 pan = 0.5f + 0.5f * MasterSeparation;
    const sF32 vm0 = MasterVolume * sFSqrt(pan);
    const sF32 vm1 = MasterVolume * sFSqrt(1 - pan);

    // Generate output samples
    for (sInt s = 0; s < samples; s++)
    {
        // Check if we need to generate more Paula-rate samples
        sInt ReadEnd = ReadPos + FIR_WIDTH + 1;
        if (WritePos < ReadPos)
            ReadEnd -= RBSIZE;
        if (ReadEnd > WritePos)
            Calc();  // Generate more Paula samples

        // FIR filter: convolution with filter coefficients
        sF32 outl0 = 0, outl1 = 0;         // Left channel (two taps for interpolation)
        sF32 outr0 = 0, outr1 = 0;         // Right channel

        // Calculate offset into ring buffer
        sInt offs = (ReadPos - FIR_WIDTH - 1) & (RBSIZE - 1);

        // Load first sample pair
        sF32 vl = RingBuf[offs];
        sF32 vr = RingBuf[offs + RBSIZE];

        // Convolve with FIR filter coefficients
        for (sInt i = 1; i < 2 * FIR_WIDTH - 1; i++)
        {
            sF32 w = FIRMem[i];             // FIR coefficient
            outl0 += vl * w;                // Accumulate left channel tap 0
            outr0 += vr * w;                // Accumulate right channel tap 0

            // Advance to next sample
            offs = (offs + 1) & (RBSIZE - 1);
            vl = RingBuf[offs];
            vr = RingBuf[offs + RBSIZE];

            outl1 += vl * w;                // Accumulate left channel tap 1
            outr1 += vr * w;                // Accumulate right channel tap 1
        }

        // Linear interpolation between two filter taps
        sF32 outl = sLerp(outl0, outl1, ReadFrac);
        sF32 outr = sLerp(outr0, outr1, ReadFrac);

        // Apply panning and output (constant power stereo mixing)
        *outbuf++ = vm0 * outl + vm1 * outr;  // Output sample (mixed)
        *outbuf++ = vm1 * outl + vm0 * outr;  // Swapped for stereo separation

        // Advance read position with fractional interpolation
        ReadFrac += step;
        sInt rfi = sInt(ReadFrac);
        ReadPos = (ReadPos + rfi) & (RBSIZE - 1);
        ReadFrac -= rfi;  // Keep only fractional part
    }
}

// =================== Paula::Constructor ===================
// Initialize Paula emulator and build FIR filter
Paula::Paula()
{
    // Build windowed-sinc FIR filter for low-pass resampling
    sF32 *FIRTable = FIRMem + FIR_WIDTH;   // Point to center of FIR array

    // Calculate filter coefficients
    sF32 yscale = sF32(OUTRATE) / sF32(PAULARATE);     // Output/Paula rate ratio
    sF32 xscale = sFPi * yscale;                        // Frequency scaling

    // Generate windowed-sinc filter taps
    // Windowed sinc: sinc(x) * hamming_window(x)
    for (sInt i = -FIR_WIDTH; i <= FIR_WIDTH; i++)
    {
        sF32 sinc = sFSinc(sF32(i) * xscale);
        sF32 hamming = sFHamming(sF32(i) / sF32(FIR_WIDTH - 1));
        FIRTable[i] = yscale * sinc * hamming;
    }

    // Initialize ring buffer
    sZeroMem(RingBuf, sizeof(RingBuf));
    ReadPos = 0;
    ReadFrac = 0;
    WritePos = FIR_WIDTH;

    // Initialize master volume and panning
    MasterVolume = 0.66f;                  // Default to 66% volume
    MasterSeparation = 0.5f;               // Default to 50:50 stereo separation
}

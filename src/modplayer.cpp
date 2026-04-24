// =================== MOD File Player Implementation ===================
// Implementation of MOD file parsing and playback

#include "modplayer.h"
#include <cstring>
#include <cstdlib>

// =================== Static Data Initialization ===================
// Base period table for MOD format
// Period values define the playback rate (frequency) of samples
// Lower period = higher frequency = higher pitch
sInt ModPlayer::BasePTable[61] = {
    0,  // Dummy entry
    // C-0 to B-0 (Octave 0)
    1712, 1616, 1525, 1440, 1357, 1281, 1209, 1141, 1077, 1017, 961, 907,
    // C-1 to B-1 (Octave 1)
    856,  808,  762,  720,  678,  640,  604,  570,  538,  508, 480, 453,
    // C-2 to B-2 (Octave 2)
    428,  404,  381,  360,  339,  320,  302,  285,  269,  254, 240, 226,
    // C-3 to B-3 (Octave 3)
    214,  202,  190,  180,  170,  160,  151,  143,  135,  127, 120, 113,
    // C-4 to B-4 (Octave 4)
    107,  101,   95,   90,   85,   80,   76,   71,   67,   64,  60,  57,
};

// Period and vibrato tables (filled in by constructor)
sInt ModPlayer::PTable[16][60];
sInt ModPlayer::VibTable[3][15][64];

// =================== Sample::Prepare ===================
void ModPlayer::Sample::Prepare()
{
    // MOD files store multi-byte values in big-endian format
    // Convert to native byte order (usually little-endian on modern systems)
    sSwapEndian(Length);                   // 16-bit value: swap bytes
    sSwapEndian(LoopStart);                // 16-bit value: swap bytes
    sSwapEndian(LoopLen);                  // 16-bit value: swap bytes

    // Clamp finetune to valid range (-8 to +7)
    Finetune &= 0x0f;                      // Keep only lower 4 bits
    if (Finetune >= 8)
        Finetune -= 16;                    // Convert from unsigned to signed
}

// =================== Pattern Constructor ===================
ModPlayer::Pattern::Pattern()
{
    // Zero out all event data
    sZeroMem(this, sizeof(Pattern));
}

// =================== Pattern::Load ===================
// Parse pattern data from MOD file
// Each note event is 4 bytes: (sample/period_hi, period_lo, effect, parameter)
void ModPlayer::Pattern::Load(sU8 *ptr)
{
    for (sInt row = 0; row < 64; row++)
    {
        for (sInt ch = 0; ch < 4; ch++)
        {
            Event &e = Events[row][ch];

            // Parse sample number (upper 4 bits of byte 0 + upper 4 bits of byte 2)
            e.Sample = (ptr[0] & 0xf0) | (ptr[2] >> 4);

            // Parse effect type (lower 4 bits of byte 2)
            e.FX = ptr[2] & 0x0f;

            // Parse effect parameter (byte 3)
            e.FXParm = ptr[3];

            // Parse note/period (bytes 0,1)
            // Convert period value to note number using period table
            e.Note = 0;
            sInt period = (sInt(ptr[0] & 0x0f) << 8) | ptr[1];

            if (period)
            {
                // Find closest matching note in period table
                sInt bestd = sAbs(period - BasePTable[0]);
                for (sInt i = 1; i <= 60; i++)
                {
                    sInt d = sAbs(period - BasePTable[i]);
                    if (d < bestd)
                    {
                        bestd = d;
                        e.Note = i;
                    }
                }
            }

            ptr += 4;  // Move to next note event
        }
    }
}

// =================== Chan Constructor ===================
ModPlayer::Chan::Chan()
{
    sZeroMem(this, sizeof(Chan));
}

// =================== Chan::GetPeriod ===================
// Calculate Paula period from note number and finetune
sInt ModPlayer::Chan::GetPeriod(sInt offs, sInt fineoffs)
{
    // Apply finetune offset
    sInt ft = FineTune + fineoffs;

    // Normalize finetune to -8 to +7 range
    while (ft > 7)
    {
        offs++;      // Increase octave
        ft -= 16;
    }
    while (ft < -8)
    {
        offs--;      // Decrease octave
        ft += 16;
    }

    // Look up period from table using note + octave offset
    return Note ? (PTable[ft & 0x0f][sClamp(Note + offs - 1, 0, 59)]) : 0;
}

// =================== Chan::SetPeriod ===================
void ModPlayer::Chan::SetPeriod(sInt offs, sInt fineoffs)
{
    if (Note)
        Period = GetPeriod(offs, fineoffs);
}

// =================== ModPlayer::CalcTickRate ===================
// Calculate samples per tick based on BPM
// Formula: samples = (125 * SAMPLE_RATE) / (BPM * OUTFPS)
void ModPlayer::CalcTickRate(sInt bpm)
{
    TickRate = (125 * OUTRATE) / (bpm * OUTFPS);
}

// =================== ModPlayer::TrigNote ===================
// Trigger a note: start playing a sample on a channel
void ModPlayer::TrigNote(sInt ch, const Pattern::Event &e)
{
    Chan &c = Chans[ch];
    Paula::Voice &v = P->V[ch];
    const Sample &s = Samples[c.Sample];
    sInt offset = 0;

    // Effect 9: Sample offset
    if (e.FX == 9)
        offset = c.FXBuf[9] << 8;

    // Trigger note unless it's a portamento effect (3 or 5)
    if (e.FX != 3 && e.FX != 5)
    {
        c.SetPeriod();

        // Handle looping vs. one-shot samples
        if (s.LoopLen > 1)
            // Looping sample
            v.Trigger(SData[c.Sample], 2 * (s.LoopStart + s.LoopLen), 2 * s.LoopLen, offset);
        else
            // One-shot sample
            v.Trigger(SData[c.Sample], v.SampleLen = 2 * s.Length, 1, offset);

        // Reset vibrato/tremolo position unless set to "don't retrigger"
        if (!c.VibRetr)
            c.VibPos = 0;
        if (!c.TremRetr)
            c.TremPos = 0;
    }
}

// =================== ModPlayer::Reset ===================
// Reset playback to beginning of song
void ModPlayer::Reset()
{
    CalcTickRate(125);  // Default BPM = 125
    Speed = 6;          // Default speed = 6 ticks per row
    TRCounter = 0;
    CurTick = 0;
    CurRow = 0;
    CurPos = 0;
    Delay = 0;
}

// =================== ModPlayer::Tick ===================
// Process one tick of MOD playback
// This is called SPEED times per row
// Handles note triggers, effect processing, and timing
void ModPlayer::Tick()
{
    const Pattern &p = Patterns[PatternList[CurPos]];
    const Pattern::Event *re = p.Events[CurRow];

    // Process each of the 4 channels
    for (sInt ch = 0; ch < 4; ch++)
    {
        const Pattern::Event &e = re[ch];
        Paula::Voice &v = P->V[ch];
        Chan &c = Chans[ch];
        const sInt fxpl = e.FXParm & 0x0f;  // Low nibble of effect parameter
        sInt TremVol = 0;                   // Tremolo volume change

        if (!CurTick)  // First tick of row: trigger new notes and set up effects
        {
            // Set sample if specified
            if (e.Sample)
            {
                c.Sample = e.Sample;
                c.FineTune = Samples[c.Sample].Finetune;
                c.Volume = Samples[c.Sample].Volume;
            }

            // Store effect parameter in buffer
            if (e.FXParm)
                c.FXBuf[e.FX] = e.FXParm;

            // Trigger note (unless it's a portamento effect)
            if (e.Note && (e.FX != 14 || ((e.FXParm >> 4) != 13)))
            {
                c.Note = e.Note;
                TrigNote(ch, e);
            }

            // Handle various effects on first tick
            switch (e.FX)
            {
            case 4:  // Vibrato
            case 6:  // Vibrato + volume slide
                if (c.FXBuf[4] & 0x0f)
                    c.VibAmpl = c.FXBuf[4] & 0x0f;  // Low nibble = amplitude
                if (c.FXBuf[4] & 0xf0)
                    c.VibSpeed = c.FXBuf[4] >> 4;  // High nibble = speed
                c.SetPeriod(0, VibTable[c.VibWave][(c.VibAmpl) - 1][c.VibPos]);
                break;

            case 7:  // Tremolo (volume modulation)
                if (c.FXBuf[7] & 0x0f)
                    c.TremAmpl = c.FXBuf[7] & 0x0f;
                if (c.FXBuf[7] & 0xf0)
                    c.TremSpeed = c.FXBuf[7] >> 4;
                TremVol = VibTable[c.TremWave][(c.TremAmpl) - 1][c.TremPos];
                break;

            case 12:  // Set volume
                c.Volume = sClamp(e.FXParm, 0, 64);
                break;

            case 14:  // Special effects (Exx)
                if (fxpl)
                    c.FXBuf14[e.FXParm >> 4] = fxpl;

                switch (e.FXParm >> 4)
                {
                case 1:  // Fine slide up
                    c.Period = sMax(113, c.Period - c.FXBuf14[1]);
                    break;
                case 2:  // Fine slide down
                    c.Period = sMin(856, c.Period + c.FXBuf14[2]);
                    break;
                case 4:  // Set vibrato waveform
                    c.VibWave = fxpl & 3;
                    if (c.VibWave == 3)
                        c.VibWave = 0;
                    c.VibRetr = fxpl & 4;
                    break;
                case 5:  // Set finetune
                    c.FineTune = fxpl;
                    if (c.FineTune >= 8)
                        c.FineTune -= 16;
                    break;
                case 7:  // Set tremolo waveform
                    c.TremWave = fxpl & 3;
                    if (c.TremWave == 3)
                        c.TremWave = 0;
                    c.TremRetr = fxpl & 4;
                    break;
                case 9:  // Retrigger note
                    if (c.FXBuf14[9] && !e.Note)
                        TrigNote(ch, e);
                    c.RetrigCount = 0;
                    break;
                case 10:  // Fine volume slide up
                    c.Volume = sMin(c.Volume + c.FXBuf14[10], 64);
                    break;
                case 11:  // Fine volume slide down
                    c.Volume = sMax(c.Volume - c.FXBuf14[11], 0);
                    break;
                case 14:  // Pattern delay
                    Delay = c.FXBuf14[14];
                    break;
                }
                break;

            case 15:  // Set speed/BPM
                if (e.FXParm)
                    if (e.FXParm <= 32)
                        Speed = e.FXParm;  // Set ticks per row
                    else
                        CalcTickRate(e.FXParm);  // Set BPM
                break;
            }
        }
        else  // Subsequent ticks: apply continuous effects
        {
            switch (e.FX)
            {
            case 0:  // Arpeggio: cycle between note and two pitch variations
                if (e.FXParm)
                {
                    sInt no = 0;
                    switch (CurTick % 3)
                    {
                    case 1:
                        no = e.FXParm >> 4;  // First variation
                        break;
                    case 2:
                        no = e.FXParm & 0x0f;  // Second variation
                        break;
                    }
                    c.SetPeriod(no);
                }
                break;

            case 1:  // Slide up
                c.Period = sMax(113, c.Period - c.FXBuf[1]);
                break;

            case 2:  // Slide down
                c.Period = sMin(856, c.Period + c.FXBuf[2]);
                break;

            case 3:  // Tone portamento (slide to note)
            case 5:  // Tone portamento + volume slide
                if (e.FX == 5)
                {
                    // Volume slide
                    if (c.FXBuf[5] & 0xf0)
                        c.Volume = sMin(c.Volume + (c.FXBuf[5] >> 4), 0x40);
                    else
                        c.Volume = sMax(c.Volume - (c.FXBuf[5] & 0x0f), 0);
                }
                // Portamento
                {
                    sInt np = c.GetPeriod();
                    if (c.Period > np)
                        c.Period = sMax(c.Period - c.FXBuf[3], np);
                    else if (c.Period < np)
                        c.Period = sMin(c.Period + c.FXBuf[3], np);
                }
                break;

            case 4:  // Vibrato
            case 6:  // Vibrato + volume slide
                if (e.FX == 6)
                {
                    // Volume slide
                    if (c.FXBuf[6] & 0xf0)
                        c.Volume = sMin(c.Volume + (c.FXBuf[6] >> 4), 0x40);
                    else
                        c.Volume = sMax(c.Volume - (c.FXBuf[6] & 0x0f), 0);
                }
                // Vibrato
                c.SetPeriod(0, VibTable[c.VibWave][c.VibAmpl - 1][c.VibPos]);
                c.VibPos = (c.VibPos + c.VibSpeed) & 0x3f;
                break;

            case 7:  // Tremolo
                TremVol = VibTable[c.TremWave][c.TremAmpl - 1][c.TremPos];
                c.TremPos = (c.TremPos + c.TremSpeed) & 0x3f;
                break;

            case 10:  // Volume slide
                if (c.FXBuf[10] & 0xf0)
                    c.Volume = sMin(c.Volume + (c.FXBuf[10] >> 4), 0x40);
                else
                    c.Volume = sMax(c.Volume - (c.FXBuf[10] & 0x0f), 0);
                break;

            case 11:  // Position jump
                if (CurTick == Speed - 1)
                {
                    CurRow = -1;
                    CurPos = e.FXParm;
                }
                break;

            case 13:  // Pattern break
                if (CurTick == Speed - 1)
                {
                    CurPos++;
                    CurRow = (10 * (e.FXParm >> 4) + (e.FXParm & 0x0f)) - 1;
                }
                break;

            case 14:  // Special effects (Exx continued)
                switch (e.FXParm >> 4)
                {
                case 6:  // Pattern loop
                    if (!fxpl)
                        c.LoopStart = CurRow;  // Set loop start
                    else if (CurTick == Speed - 1)
                    {
                        if (c.LoopCount < fxpl)
                        {
                            CurRow = c.LoopStart - 1;
                            c.LoopCount++;
                        }
                        else
                            c.LoopCount = 0;
                    }
                    break;

                case 9:  // Retrigger note
                    if (++c.RetrigCount == c.FXBuf14[9])
                    {
                        c.RetrigCount = 0;
                        TrigNote(ch, e);
                    }
                    break;

                case 12:  // Cut note
                    if (CurTick == c.FXBuf14[12])
                        c.Volume = 0;
                    break;

                case 13:  // Delay note
                    if (CurTick == c.FXBuf14[13])
                        TrigNote(ch, e);
                    break;
                }
                break;
            }
        }

        // Apply tremolo to final volume and update Paula voice
        v.Volume = sClamp(c.Volume + TremVol, 0, 64);
        v.Period = c.Period;
    }

    // Advance tick counter and handle row/position advancement
    CurTick++;
    if (CurTick >= Speed * (Delay + 1))
    {
        CurTick = 0;
        CurRow++;
        Delay = 0;
    }

    // Advance to next pattern after 64 rows
    if (CurRow >= 64)
    {
        CurRow = 0;
        CurPos++;
    }

    // Loop back to beginning when reaching end of song
    if (CurPos >= PositionCount)
        CurPos = 0;
}

// =================== ModPlayer Constructor ===================
// Load and parse MOD file
ModPlayer::ModPlayer(Paula *p, sU8 *moddata) : P(p)
{
    // Build period table for all finetune values (-8 to +7)
    // This adjusts the base periods by fractional semitones
    for (sInt ft = 0; ft < 16; ft++)
    {
        // Convert finetune index to signed value
        sInt rft = -((ft >= 8) ? ft - 16 : ft);

        // Calculate frequency multiplier for this finetune
        sF32 fac = sFPow(2.0f, sF32(rft) / (12.0f * 16.0f));

        // Generate period table for this finetune
        for (sInt i = 0; i < 60; i++)
            PTable[ft][i] = sInt(sF32(BasePTable[i]) * fac + 0.5f);
    }

    // Build vibrato/tremolo waveform tables
    // Three waveforms: sine, ramp, square
    for (sInt ampl = 0; ampl < 15; ampl++)
    {
        sF32 scale = ampl + 1.5f;  // Amplitude scaling
        sF32 shift = 0;            // DC offset

        for (sInt x = 0; x < 64; x++)
        {
            // Waveform 0: Sine
            VibTable[0][ampl][x] = sInt(scale * sFSin(x * sFPi / 32.0f) + shift);
            // Waveform 1: Ramp down
            VibTable[1][ampl][x] = sInt(scale * ((63 - x) / 31.5f - 1.0f) + shift);
            // Waveform 2: Square
            VibTable[2][ampl][x] = sInt(scale * ((x < 32) ? 1 : -1) + shift);
        }
    }

    // === Parse MOD File ===
    // Extract song name (first 20 bytes)
    memcpy(Name, moddata, 20);
    Name[20] = 0;  // Null terminate
    moddata += 20;

    // Initialize sample array
    SampleCount = 32;  // Default to 32 samples
    ChannelCount = 4;  // MOD format always has 4 channels
    Samples = (Sample *)(moddata - sizeof(Sample));
    moddata += 15 * sizeof(Sample);  // Skip first 15 sample headers

    // Check MOD format tag (determines sample count)
    sU32 &tag = *(sU32 *)(moddata + 130 + 16 * sizeof(Sample));
    switch (tag)
    {
    case '.K.M':  // M.K. (Michael Kleps) - standard 4-channel MOD
    case '4TLF':  // FLT4 (Startrekker 4 channel)
    case '!K!M':  // M!K! (more than 100 patterns)
        SampleCount = 32;  // These formats use 32 samples
        break;
    }

    // Skip extra sample headers if needed
    if (SampleCount > 16)
        moddata += (SampleCount - 16) * sizeof(Sample);

    // Prepare all samples (convert from big-endian format)
    for (sInt i = 1; i < SampleCount; i++)
        Samples[i].Prepare();

    // Load song structure
    PositionCount = *moddata;  // Number of patterns in sequence
    moddata += 2;              // Skip unused byte
    memcpy(PatternList, moddata, 128);  // Load pattern order list
    moddata += 128;

    // Skip format tag if present
    if (SampleCount > 15)
        moddata += 4;

    // Find highest pattern number used
    PatternCount = 0;
    for (sInt i = 0; i < 128; i++)
        PatternCount = sMax(PatternCount, PatternList[i] + 1);

    // Load all patterns
    for (sInt i = 0; i < PatternCount; i++)
    {
        Patterns[i].Load(moddata);
        moddata += 1024;  // Each pattern is 1024 bytes
    }

    // Load sample data
    sZeroMem(SData, sizeof(SData));
    for (sInt i = 1; i < SampleCount; i++)
    {
        SData[i] = (sS8 *)moddata;
        moddata += 2 * Samples[i].Length;  // Samples are stored as words (2 bytes)
    }

    // Initialize playback state
    Reset();
}

// =================== ModPlayer::Render ===================
// Generate audio samples for playback
sU32 ModPlayer::Render(sF32 *buf, sU32 len)
{
    while (len)
    {
        // Calculate how many samples to generate before next tick
        sInt todo = sMin<sInt>(len, TRCounter);

        if (todo)
        {
            // Render Paula audio
            P->Render(buf, todo);
            buf += 2 * todo;           // Stereo: 2 samples per frame
            len -= todo;
            TRCounter -= todo;
        }
        else
        {
            // Time for next MOD tick
            Tick();
            TRCounter = TickRate;  // Reset counter for next tick
        }
    }
    return 1;
}

// =================== ModPlayer::RenderProxy ===================
// Static wrapper function for use as C-style callback
sU32 ModPlayer::RenderProxy(void *parm, sF32 *buf, sU32 len)
{
    return ((ModPlayer *)parm)->Render(buf, len);
}

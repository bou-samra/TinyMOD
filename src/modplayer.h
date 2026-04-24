// =================== MOD File Player ===================
// Plays Amiga MOD (Protracker) format music files
// Handles all MOD format parsing, effect processing, and timing

#ifndef MODPLAYER_H
#define MODPLAYER_H

#include "types.h"
#include "config.h"
#include "paula.h"

// =================== ModPlayer Class ===================
// Represents a MOD file player with playback control and effect processing
class ModPlayer
{
private:
    // === Paula Reference ===
    Paula *P;                              // Pointer to Paula emulator instance

    // === Period & Frequency Tables ===
    // These tables convert MOD note values to Paula periods
    static sInt BasePTable[5 * 12 + 1];    // Base period table (5 octaves x 12 semitones + extra)
    static sInt PTable[16][60];            // Period table for each finetune (-8 to +7)
    static sInt VibTable[3][15][64];       // Vibrato/tremolo lookup tables

    // === Playback State ===
    sInt Speed;                            // Ticks per row (default 6)
    sInt TickRate;                         // Number of samples per tick
    sInt TRCounter;                        // Tick rate counter (samples remaining)

    sInt CurTick;                          // Current tick within row (0 to Speed-1)
    sInt CurRow;                           // Current pattern row (0-63)
    sInt CurPos;                           // Current song position (pattern index)
    sInt Delay;                            // Pattern delay in ticks

    // === Sample Storage ===
    sS8 *SData[32];                        // Pointers to sample data
    sInt SampleCount;                      // Number of samples in file
    sInt ChannelCount;                     // Number of channels (always 4 for standard MOD)

    // === Song Structure ===
    sU8 PatternList[128];                  // List of which patterns to play in which order
    sInt PositionCount;                    // Number of positions in song
    sInt PatternCount;                     // Number of unique patterns

    // =================== Sample Structure ===================
    // Represents a single instrument/sample in MOD format
    struct Sample
    {
        char Name[22];                     // Sample name (22 bytes in MOD format)
        sU16 Length;                       // Sample length in words (1 word = 2 bytes)
        sS8 Finetune;                      // Finetune value (-8 to +7)
        sU8 Volume;                        // Default volume (0-64)
        sU16 LoopStart;                    // Loop start position in words
        sU16 LoopLen;                      // Loop length in words

        // Convert sample data from big-endian MOD format to native format
        // MOD files store multi-byte values in big-endian format
        void Prepare();
    } *Samples;                            // Pointer to sample array

    // =================== Pattern Structure ===================
    // Represents a 64-row pattern with 4 channels of note data
    struct Pattern
    {
        // Single note event (one channel, one row)
        struct Event
        {
            sInt Sample;                   // Sample number (0-31)
            sInt Note;                     // Note number (0-60)
            sInt FX;                       // Effect type (0-15)
            sInt FXParm;                   // Effect parameter value
        } Events[64][4];                   // 64 rows x 4 channels

        // Zero out pattern data
        Pattern();

        // Parse pattern data from MOD file format
        void Load(sU8 *ptr);
    } Patterns[128];                       // Array of patterns

    // =================== Channel State Structure ===================
    // Maintains playback state for a single audio channel
    struct Chan
    {
        sInt Note;                         // Current note number
        sInt Period;                       // Current period (Paula playback rate)
        sInt Sample;                       // Current sample number
        sInt FineTune;                     // Current finetune value
        sInt Volume;                       // Current volume (0-64)
        sInt FXBuf[16];                    // Effect command values (command 0-15)
        sInt FXBuf14[16];                  // Effect parameters for command 14 (special)
        sInt LoopStart;                    // Pattern loop start row
        sInt LoopCount;                    // Pattern loop counter
        sInt RetrigCount;                  // Retrigger counter (for effect 9)
        sInt VibWave;                      // Vibrato waveform (0-3)
        sInt VibRetr;                      // Vibrato retrigger flag
        sInt VibPos;                       // Vibrato position (0-63)
        sInt VibAmpl;                      // Vibrato amplitude (1-15)
        sInt VibSpeed;                     // Vibrato speed
        sInt TremWave;                     // Tremolo waveform (0-3)
        sInt TremRetr;                     // Tremolo retrigger flag
        sInt TremPos;                      // Tremolo position (0-63)
        sInt TremAmpl;                     // Tremolo amplitude (1-15)
        sInt TremSpeed;                    // Tremolo speed

        // Initialize channel state to all zeros
        Chan();

        // Calculate Paula period from note and finetune values
        sInt GetPeriod(sInt offs = 0, sInt fineoffs = 0);

        // Set Paula period
        void SetPeriod(sInt offs = 0, sInt fineoffs = 0);
    } Chans[4];                            // Array of 4 channels

    // =================== Playback Control ===================
    // Calculate number of samples per tick based on BPM
    // Higher BPM = faster playback
    void CalcTickRate(sInt bpm);

    // Trigger a note on a channel (start playing sample)
    void TrigNote(sInt ch, const Pattern::Event &e);

    // Reset playback state to beginning of song
    void Reset();

    // Process one "tick" of MOD playback
    // Updates effects, advances notes, handles timing
    void Tick();

public:
    // Song name from MOD file
    char Name[21];

    // ModPlayer constructor: load and initialize MOD file
    // p: pointer to Paula emulator
    // moddata: pointer to MOD file data in memory
    ModPlayer(Paula *p, sU8 *moddata);

    // =================== Audio Rendering ===================
    // Render audio samples into buffer
    // Called repeatedly by audio system to generate sound
    // buf: output buffer for stereo samples (float, interleaved L/R)
    // len: number of samples to generate
    // Returns: number of samples generated
    sU32 Render(sF32 *buf, sU32 len);

    // Static callback function for audio systems
    // Allows this to be used as a C-style callback
    static sU32 __stdcall RenderProxy(void *parm, sF32 *buf, sU32 len);
};

#endif // MODPLAYER_H

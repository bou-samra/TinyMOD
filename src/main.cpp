// =================== TinyMOD Player - Main Entry Point ===================
// TinyMOD: An Amiga MOD file player with authentic Paula chip emulation
// Supports playback of Protracker MOD files with full effect support
//
// Authors:
//   Tammo "kb" Hinrichs - Original Paula emulator (2007)
//   Jason Bou-samra - Refactoring and PortAudio integration (2024)
//
// This program is released into the public domain.
// Use, distribute, modify as you wish.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <portaudio.h>

#include "types.h"
#include "config.h"
#include "paula.h"
#include "modplayer.h"

// =================== Audio Configuration Constants ===================
const int SAMPLE_RATE_INTERNAL = 96000;    // Internal Paula emulation rate
const int SAMPLE_RATE_OUTPUT = 48000;      // Output audio sample rate

// =================== Utility Functions ===================

// Load MOD file from disk into memory
// Returns pointer to allocated memory containing file data
// Sets file_size to the number of bytes read
sU8 *load_mod_file(const char *filename, size_t &file_size)
{
    // Open file for reading in binary mode
    FILE *fh = fopen(filename, "rb");
    if (!fh)
    {
        perror("fopen");
        return NULL;
    }

    // Get file size using stat()
    struct stat sb;
    if (stat(filename, &sb) == -1)
    {
        perror("stat");
        fclose(fh);
        return NULL;
    }

    file_size = sb.st_size;

    // Allocate memory for file (4MB max)
    if (file_size > 4 * 1024 * 1024)
    {
        fprintf(stderr, "Error: MOD file too large (max 4MB)\n");
        fclose(fh);
        return NULL;
    }

    sU8 *mod = (sU8 *)malloc(file_size);
    if (!mod)
    {
        perror("malloc");
        fclose(fh);
        return NULL;
    }

    // Read file into memory
    if (fread(mod, file_size, 1, fh) != 1)
    {
        perror("fread");
        free(mod);
        fclose(fh);
        return NULL;
    }

    fclose(fh);
    return mod;
}

// PortAudio error handler
// Prints error information and terminates program
void handle_pa_error(PaError err)
{
    if (err == paNoError)
        return;

    fprintf(stderr, "PortAudio Error: %s\n", Pa_GetErrorText(err));

    // Print additional host API error information if available
    if (err == paUnanticipatedHostError)
    {
        const PaHostErrorInfo *hostErrorInfo = Pa_GetLastHostErrorInfo();
        fprintf(stderr, "Host API Error: #%ld\n", hostErrorInfo->errorCode);
        fprintf(stderr, "Host API: %d\n", hostErrorInfo->hostApiType);
        fprintf(stderr, "Details: %s\n", hostErrorInfo->errorText);
    }

    Pa_Terminate();
    exit(1);
}

// Print usage information
void print_usage(const char *program_name)
{
    printf("Usage: %s [<mod file>|OPTION]\n\n", program_name);
    printf("OPTIONS:\n");
    printf("  --about             Display about message\n");
    printf("  --help              Display this help message\n");
}

// Print about information
void print_about()
{
    printf("TinyMOD - Amiga MOD File Player\n\n");
    printf("An Amiga MOD file player that replicates the authentic sound\n");
    printf("characteristics of an Amiga via Paula chip emulation.\n\n");
    printf("Authors:\n");
    printf("  Tammo \"kb\" Hinrichs - Paula emulator (2007)\n");
    printf("  Jason Bou-samra - Refactoring and integration (2024)\n\n");
    printf("Released into the public domain.\n");
}

// =================== Main Program ===================
int main(int argc, const char **argv)
{
    // === Check Command Line Arguments ===
    if (argc != 2)
    {
        print_usage(argv[0]);
        return 1;
    }

    const char *filename = argv[1];

    // Handle --about option
    if (!strcmp(filename, "--about"))
    {
        print_about();
        return 0;
    }

    // Handle --help option
    if (!strcmp(filename, "--help"))
    {
        print_usage(argv[0]);
        return 0;
    }

    // === Load MOD File ===
    printf("Loading MOD file: %s\n", filename);
    size_t mod_size = 0;
    sU8 *mod_data = load_mod_file(filename, mod_size);

    if (!mod_data)
    {
        fprintf(stderr, "Error: Failed to load MOD file\n");
        return 1;
    }

    printf("Loaded %zu bytes\n", mod_size);

    // === Initialize PortAudio ===
    printf("Initializing PortAudio...\n");
    PaError err = Pa_Initialize();
    if (err != paNoError)
        handle_pa_error(err);

    // === Configure Output Stream ===
    PaStreamParameters outputParameters;
    outputParameters.device = Pa_GetDefaultOutputDevice();

    if (outputParameters.device == paNoDevice)
    {
        fprintf(stderr, "Error: No default output device found\n");
        Pa_Terminate();
        free(mod_data);
        return 1;
    }

    outputParameters.channelCount = 2;                 // Stereo output
    outputParameters.sampleFormat = paFloat32;         // 32-bit float samples
    outputParameters.suggestedLatency =
        Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    // === Open Audio Stream ===
    PaStream *stream;
    err = Pa_OpenStream(
        &stream,
        NULL,                              // No input
        &outputParameters,
        SAMPLE_RATE_OUTPUT,               // Output sample rate
        FRAMES_PER_BUFFER,                // Frames per buffer
        paClipOff,                        // Don't clip output
        NULL,                             // No callback
        NULL);                            // No user data

    if (err != paNoError)
        handle_pa_error(err);

    // === Start Audio Stream ===
    err = Pa_StartStream(stream);
    if (err != paNoError)
        handle_pa_error(err);

    // === Initialize MOD Player and Paula Emulator ===
    Paula paula;                          // Create Paula emulator instance
    ModPlayer player(&paula, mod_data);   // Create MOD player with MOD file

    // === Display Playback Information ===
    cls();  // Clear screen
    printf("TinyMOD - Amiga MOD File Player\n");
    printf("================================\n\n");
    printf("Currently playing: %s\n", player.Name);
    printf("Duration: %d seconds\n", NUM_SECONDS);
    printf("Sample rate: %d Hz (Paula: %d Hz)\n", SAMPLE_RATE_OUTPUT, SAMPLE_RATE_INTERNAL);
    printf("\nPress Ctrl+C to stop\n\n");

    // === Calculate Playback Parameters ===
    sInt nwrite = FRAMES_PER_BUFFER / 2;  // Samples per buffer
    sInt buffer_count = (NUM_SECONDS * SAMPLE_RATE_OUTPUT) / FRAMES_PER_BUFFER;

    // === Allocate Audio Buffers ===
    sF32 *mixbuffer = (sF32 *)malloc(nwrite * 2 * sizeof(sF32));
    if (!mixbuffer)
    {
        fprintf(stderr, "Error: Failed to allocate audio buffer\n");
        Pa_CloseStream(stream);
        Pa_Terminate();
        free(mod_data);
        return 1;
    }

    // === Main Playback Loop ===
    printf("Playing...\n");
    for (int i = 0; i < buffer_count; i++)
    {
        // Render MOD file audio
        player.RenderProxy(&player, mixbuffer, nwrite);

        // Write audio to stream
        err = Pa_WriteStream(stream, mixbuffer, nwrite);
        if (err != paNoError)
        {
            fprintf(stderr, "Warning: Write error - %s\n", Pa_GetErrorText(err));
        }

        // Print progress
        if ((i + 1) % 10 == 0)
        {
            printf(".");
            fflush(stdout);
        }
    }

    printf("\n\n");

    // === Shutdown Audio ===
    err = Pa_StopStream(stream);
    if (err != paNoError)
        handle_pa_error(err);

    // Allow stream to finish draining
    Pa_Sleep(1000);

    err = Pa_CloseStream(stream);
    if (err != paNoError)
        handle_pa_error(err);

    Pa_Terminate();

    // === Cleanup ===
    free(mixbuffer);
    free(mod_data);

    printf("Playback complete. Goodbye!\n");
    return 0;
}

/*

  TinyMOD
  Written by Tammo "kb" Hinrichs in 2007
  This source code is hereby placed into the public domain. Use, distribute,
  modify, misappropriate and generally abuse it as you wish. Giving credits
  would be nice of course.

  This player includes an Amiga Paula chip "emulation" that faithfully recreates
  how it sounds when a sample is resampled using a master clock of 3.5 MHz. Yes,
  rendering at this rate and downsampling to the usual 48KHz takes quite a bit
  of CPU. Feel free to replace this part with some conventional mixing routines
  if authenticity isn't your goal and you really need Protracker MOD support
  for any other reason...

  The code should be pretty portable, all OS/platform dependent stuff is
  at the top. Code for testing is at the bottom.

  You'll need some kind of sound output that calls back the player providing
  it a stereo interleaved single float buffer to write into (0dB=1.0).

  Changelog:

  2024-03-09: (Jason Bou-Samra)
  * changes to main() routine to make executable from Linux command line interface
  * modularised paula and modplayer classes into seperate files
  * added sound output using port audio
  * sprinkled comments throughout source code, and general source formatting
  * created makefile

  2007-12-07: (Tammo "kb" Hinrichs)
  * fixed 40x and 4x0 vibrato effects (jogeir - tiny tunes)
  * fixed pattern loop (olof gustafsson - pinball illusions)
  * fixed fine volslide down (olof gustafsson - pinball illusions)
  * included some external header files
  * cleanups

  2007-12-06: (Tammo "kb" Hinrichs)
  * first "release". Note to self: Don't post stuff on pouet.net when drunk.

*/

//  TO COMPILE: g++ -o tinymod `pkg-config --libs alsa` tinymod.cpp -lm -L . -l:libportaudio.a

// =================== system dependent stuff starts here =====================
// Multicharacter literal
#pragma GCC diagnostic ignored "-Wmultichar"
#pragma intrinsic(memset, sqrt, sin, cos, atan, powf)	// memory set, square root, SINe, COSine, Arc TAngent, power of


// ### defines
#define __stdcall
#define __cdecl

#define NUM_SECONDS         (1000)
#define SAMPLE_RATE         (96000)
#define FRAMES_PER_BUFFER   (0x10000)

#define cls()               printf("\033[H\033[J")  // control chars to clear screen

// ### includes
#include <inttypes.h>								// fixed size integer types (part of c standard library)
#include <math.h>									// basic mathematical operations (part of c standard library) - mostly floating point
#include <stdio.h>									// standard I/O library
#include <string.h>									// string handling (part of c standard library)
#include <unistd.h>									// (unix standard)
#include <fcntl.h>									// (file control options) - used by open()
#include <sys/stat.h>								// needed by stat function (get file status)
#include <cstdint>									// defines set of integeral types
#include <cstdio>									// defines set of C standard I/O types
#include "portaudio.h"								// port audio
#include "types.h"                                  // various type definitions
#include "paula.h"                                  // Amiga "Paula" sound hardware emulator
#include "modplayer.h"                              // MOD play routine

// ## declerations
void error(PaError err1);							// declare port audio error handling function

// ***********************************************
// ** handle Command Line Interface (CLI) stuff **
// ***********************************************
int __cdecl main (int argc, const char **argv)		// main start
{
	// freopen("warning.log", "w", stderr);			// surpress console messages
    const char* filename = argv[1];					// only one argument and command

  if (argc != 2){									// if less than 2 arguments passed on command line, display usage
      printf("Usage: tinymod [<mod name>|OPTION]\n\
tinymod --help for help\n");
      return 1;
    }												// display usage, then exit

if(!strcmp(filename, "--about")) {
    printf("TinyMOD\n\n\
An Amiga MOD file player that tries to replicate the authentic sound\n\
charateristics of an Amiga via software emulation of the Paula chip.\n\n\
Authors:\n\
[Tammo \"kb\" Hinrichs in 2007]\n\
[Jason Bou-samra in 2024]\n\
"); return 0;
    }												// display about, then exit

if(!strcmp (filename,  "--help")) {
    printf("Usage: tinymod [<mod name>|OPTION]\n\n\
OPTIONS\n\
--about            displays about message\n\
--help             displays this help message\n"); return 0;
    }												// display help, then exit

// **********************
// **  load MOD file   **
// **********************
    static sU8 mod[4 * 1024 * 1024];				// 4MB unsigned character array to hold MOD file

    FILE* fh = fopen(filename, "rb");
    if (!fh) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    struct stat sb;
    if (stat(filename, &sb) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    fread(mod, sb.st_size, 1, fh);
    fclose(fh);

// **********************
// ** port audio setup **
// **********************
    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;

    float buffer[FRAMES_PER_BUFFER][2];				// stereo output buffer
    int left_phase = 0;
    int right_phase = 0;
    int left_inc = 1;
    int right_inc = 1;								// higher pitch so we can distinguish left and right.
    int i, j, k;									// for indexing
    int bufferCount;
    static const int BUFFERLEN = 0x10000;			// buffer length
    sInt nwrite = 0x10000;							// number of samples
    sF32 mixbuffer[BUFFERLEN];						// sample buffer

    err = Pa_Initialize();							// initialize
    if( err != paNoError ) error(err);

    outputParameters.device = Pa_GetDefaultOutputDevice();	// default output device
    if (outputParameters.device == paNoDevice) {
      fprintf(stderr,"Error: No default output device.\n");
      error(err);
    }
    outputParameters.channelCount = 2;				// stereo output
    outputParameters.sampleFormat = paFloat32;		// 32 bit floating point output
    outputParameters.suggestedLatency = 0.050;		// Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
              &stream,
              NULL,									// no input
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,							// we won't output out of range samples so don't bother clipping them
              NULL,									// no callback, use blocking API
              NULL );								// no callback, so no callback userData
    if( err != paNoError ) error(err);

        err = Pa_StartStream( stream );
        if( err != paNoError ) error(err);

// *********************
// ** play setup/loop **
// *********************
    Paula P;										// instantiate paula
    ModPlayer player (&P, mod);						// instantiate paramaterised constructior ModPlayer
    cls();											// clear screen
    printf("TinyMOD\n");
    printf ("currently playing: %s\n", player.Name);	// display details of MOD being played
    printf("Playing for %d seconds.\n", NUM_SECONDS );
    printf("^C to stop\n");
	// printf("PortAudio: SR = %d, BufSize = %d\n", SAMPLE_RATE, FRAMES_PER_BUFFER);

        bufferCount = ((NUM_SECONDS * SAMPLE_RATE) / FRAMES_PER_BUFFER);	// determine buffer loads

        for( i=0; i < bufferCount; i++ )			// countdown buffers
        {
        player.RenderProxy(&player, mixbuffer, nwrite/2);	// MOD player
            for( j=0; j < FRAMES_PER_BUFFER; j++ )	// copy samples to buffer
            {
                buffer[j][0] = mixbuffer[left_phase];	// left
                buffer[j][1] = mixbuffer[right_phase];	// right
                left_phase += left_inc;
                right_phase += right_inc;
                if( left_phase >= FRAMES_PER_BUFFER ) left_phase -= FRAMES_PER_BUFFER;
                if( right_phase >= FRAMES_PER_BUFFER ) right_phase -= FRAMES_PER_BUFFER;
            }

			// left_phase = right_phase = 0;

            err = Pa_WriteStream( stream, buffer, FRAMES_PER_BUFFER );	// transfer buffer to stream
            if( err != paNoError ) error(err);
        }											// keep going

// *************************
// ** port audio shutdown **
// *************************
        err = Pa_StopStream( stream );
        if( err != paNoError ) error(err);

		// ++left_inc;
		// ++right_inc;

        Pa_Sleep( 1000 );							// mainly for test purposes so we can hear sound

    err = Pa_CloseStream( stream );
    if( err != paNoError ) error(err);

    Pa_Terminate();
    printf("Bye bye!.\n");

    return err;										// that's all folks, all done
}

// **********************
// **  hidden message  **
// **********************
char message[] = { "toto, all the way\n" };
const char* text = "09/03/2024 jason bou-samra, 07/12/2007 Tammo \"kb\" Hinrichs\n";

// *******************************
// **  error handling routine   **
// *******************************
void error(PaError err1)
{
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err1 );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err1 ) );
    // Print more information about the error.
    if( err1 == paUnanticipatedHostError )
    {
        const PaHostErrorInfo *hostErrorInfo = Pa_GetLastHostErrorInfo();
        fprintf( stderr, "Host API error = #%ld, hostApiType = %d\n", hostErrorInfo->errorCode, hostErrorInfo->hostApiType );
        fprintf( stderr, "Host API error = %s\n", hostErrorInfo->errorText );
    }
    Pa_Terminate();
    exit(1);
}

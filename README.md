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

  2024-03-09:
  * modifications by Jason Bou-Samra
  * changes to main() routine to make executable from Linux command line interface
  * modularised paula and modplayer classes into seperate files
  * added sound output using port audio
  * sprinkled comments throughout source code, and general source formatting
  * created makefile

  2007-12-07:
  * fixed 40x and 4x0 vibrato effects (jogeir - tiny tunes)
  * fixed pattern loop (olof gustafsson - pinball illusions)
  * fixed fine volslide down (olof gustafsson - pinball illusions)
  * included some external header files
  * cleanups

  2007-12-06: first "release". Note to self: Don't post stuff on pouet.net when drunk.

  TO COMPILE: g++ -o tinymod `pkg-config --libs alsa` tinymod.cpp -lm -L . -l:libportaudio.a

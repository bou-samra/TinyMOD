# kb's blog

random and not so random stuff/  
**TinyMOD**  
*Posted on 2011/10/23*

At some point in 2007 somebody told me how the Amiga’s sound chip, Paula, was able to modify a sound’s volume digitally without using multiplication – dedicated circuits for that would have been prohibitively expensive for a home computer in 1984: It simply has a 6-bit counter per voice that’s incremented every cycle and if its value is above the set volume, the voice is silenced for that cycle. So effectively it’s PWM with a pulse frequency of about 50Khz.

“But wait, shouldn’t that color the sound, ring modulation artifacts and such?” I thought. The answer is of course a resounding no (also all artifacts introduced by the PWM are outside the audible range) but that didn’t stop me from trying to emulate a Paula voice at the full 3.5MHz and then filtering it down to find out how it sounds.

Yeah well, and while we’re at it, let’s see if we can hack up a simple .MOD player too without using anything a 68000 didn’t have to offer (multiplications and such). Because coding for a couple of hours and the only being able to play a single waveform is boring.

Another few hours later there was one additional never-to-be-published toy project on my HD that way able to play a few MOD files that I liked, and that was about to be abandoned… if it hadn’t been for a thread on pouet.net where somebody was asking for a module player source. And I just came home from a party and was ever so slightly inebriated, so I just pasted the source code there. A discussion spawned, I cleaned up the code a bit and fixed some replay errors, and so here it is, released into the public domain for everyone to enjoy or laugh at:

Just add sound output. Have fun :)

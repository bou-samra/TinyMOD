// ================================ Define ModPlayer Class (MOD Player) ===============================================

class ModPlayer						// ModPlayer class
{
private:
  Paula *P;						// create instance of Paula

  static sInt BasePTable[5 * 12 + 1];			// base period table (5 octaves x 12 semitones + extra 1)
  static sInt PTable[16][60];				// period table (16 period tables x 60 semitones (5 x 12)
  static sInt VibTable[3][15][64];			// vibrato table (vib waveform, vib amplitude, vib position)

  // ** tick related
  sInt Speed;						// speed
  sInt TickRate;					// tick rate
  sInt TRCounter;					// tick rate counter

  sInt CurTick;						// current tick
  sInt CurRow;						// current row
  sInt CurPos;						// current song position
  sInt Delay;						// delay?

  // ** sample related
  sS8 *SData[32];					// sample data
  sInt SampleCount;					// sample count
  sInt ChannelCount;					// channel count

  // ** pattern related
  sU8 PatternList[128];					// pattern list
  sInt PositionCount;					// position count
  sInt PatternCount;					// pattern count

// ################################################ SAMPLE STRUCTURE ################################################
private:
struct Sample
  {
    char Name[22];					// sample name
    sU16 Length;					// sample length
    sS8 Finetune;					// sample finetune
    sU8 Volume;						// sample volume
    sU16 LoopStart;					// sample loop start position
    sU16 LoopLen;					// sample loop length

// ************ PREPARE ************
    void Prepare ()					// take care on endianness issues
    {
      sSwapEndian (Length);				// swap high & low bytes
      sSwapEndian (LoopStart);				// swap high & low bytes
      sSwapEndian (LoopLen);				// swap high & low bytes
      Finetune &= 0x0f;
      if (Finetune >= 8) Finetune -= 16;		// sample finetune is between -8 & 7
    }
  } *Samples;						// Sample structure end
// Sample *Samples;

// ################################################ PATTERN STRUCTURE ################################################
 private:
  struct Pattern					// pattern structure
  {
    struct Event
    {
      sInt Sample;					// sample
      sInt Note;					// note
      sInt FX;						// effect
      sInt FXParm;					// effect paramater
    } Events[64][4];

// ************ PATTERN CONSTRUCTOR ************
    Pattern ()
    {
    	sZeroMem (this, sizeof (Pattern));		// pattern constructor
    }

// ************ LOAD ************
    void Load (sU8 *ptr)				// Load start
    {
      for (sInt row = 0; row < 64; row++)
        for (sInt ch = 0; ch < 4; ch++)
          {
            Event &e = Events[row][ch];
            e.Sample = (ptr[0] & 0xf0) | (ptr[2] >> 4);	// sample
            e.FX = ptr[2] & 0x0f;			// effect
            e.FXParm = ptr[3];				// effect paramater

            e.Note = 0;					// note
            sInt period = (sInt (ptr[0] & 0x0f) << 8) | ptr[1];
            sInt bestd = sAbs (period - BasePTable[0]);
            if (period)
              for (sInt i = 1; i <= 60; i++)
                {
                  sInt d = sAbs (period - BasePTable[i]);
                  if (d < bestd)
                    {
                      bestd = d;
                      e.Note = i;
                    }
                }					// period

            ptr += 4;
          }
    }							// Load end
  } Patterns[128];					// Pattern structure end
// Pattern Patterns[128];				// patterns

// ################################################ CHANNEL STRUCTURE ################################################
private:
struct Chan						// Channel Structure start
  {
    sInt Note;						// note
    sInt Period;					// period
    sInt Sample;					// sample
    sInt FineTune;					// fine tune
    sInt Volume;					// volume
    sInt FXBuf[16];					// effects buffer
    sInt FXBuf14[16];					// effects buffer (command 14 - extend)
    sInt LoopStart;					// loop start
    sInt LoopCount;					// loop count
    sInt RetrigCount;					// retrigger count
    sInt VibWave;					// vibrato waveform
    sInt VibRetr;					// vibrato retrigger
    sInt VibPos;					// vibrato position
    sInt VibAmpl;					// vibrato amplitude
    sInt VibSpeed;					// vibrato speed
    sInt TremWave;					// tremolo waveform
    sInt TremRetr;					// tremolo retrigger
    sInt TremPos;					// tremolo position
    sInt TremAmpl;					// tremolo amplitude
    sInt TremSpeed;					// tremolo speed

// ************ CONSTRUCTOR ************
    Chan () { sZeroMem (this, sizeof (Chan)); }		// channel constructor

// ************ GET PERIOD ************
    sInt GetPeriod (sInt offs = 0, sInt fineoffs = 0)	// Get Period
    {
      sInt ft = FineTune + fineoffs;			// fintune offset
      while (ft > 7)
        {
          offs++;					// offset
          ft -= 16;
        }
      while (ft < -8)
        {
          offs--;
          ft += 16;
        }
      return Note ? (PTable[ft & 0x0f][sClamp (Note + offs - 1, 0, 59)]) : 0;
    }

// ************ SET PERIOD ************
    void SetPeriod (sInt offs = 0, sInt fineoffs = 0)	// Set Period
    {
      if (Note)
        Period = GetPeriod (offs, fineoffs);
    }
  } Chans[4];						// Channel Structure end

// ************ CALCULATE TICK RATE ************
private:
void CalcTickRate (sInt bpm)				// calculate tick rate start
  {
    TickRate = (125 * OUTRATE) / (bpm * OUTFPS);
  }							// calculate tick rate end

// ************ TRIGGER NOTE ************
private:
void TrigNote (sInt ch, const Pattern::Event &e)	// trigger note (channel, event)
  {
    Chan &c = Chans[ch];				// channel
    Paula::Voice &v = P->V[ch];				// Paula::Voice &v = P->[ch]; P is instance of Paula class,
							// v is voice, ch is channel -  reference (alias)
    const Sample &s = Samples[c.Sample];
    sInt offset = 0;

    if (e.FX == 9)
      offset = c.FXBuf[9] << 8;
    if (e.FX != 3 && e.FX != 5)
      {
        c.SetPeriod ();
        if (s.LoopLen > 1)
          v.Trigger (SData[c.Sample], 2 * (s.LoopStart + s.LoopLen), 2 * s.LoopLen, offset);
        else
          v.Trigger (SData[c.Sample], v.SampleLen = 2 * s.Length, 1, offset);
        if (!c.VibRetr)
          c.VibPos = 0;
        if (!c.TremRetr)
          c.TremPos = 0;
      }
  }							// Trigger note end

// ************ RESET ************
private:
void Reset ()						// reset function
  {
    CalcTickRate (125);					// default tick rate = 125
    Speed = 6;						// default speed = 6
    TRCounter = 0;					// tick rate counter = 0
    CurTick = 0;					// current tick = 0
    CurRow = 0;						// current row = 0
    CurPos = 0;						// current song position = 0
    Delay = 0;						// delay = 0
  }


// ************ TICK ************
private:
void Tick ()						// start tick routine, cycle (50Hz, 20ms)
  {
    const Pattern &p = Patterns[PatternList[CurPos]];
    const Pattern::Event *re = p.Events[CurRow];
    for (sInt ch = 0; ch < 4; ch++)
      {
        const Pattern::Event &e = re[ch];
        Paula::Voice &v = P->V[ch];
        Chan &c = Chans[ch];
        const sInt fxpl = e.FXParm & 0x0f;		// pattern list
        sInt TremVol = 0;
        if (!CurTick)
          {
            if (e.Sample)
              {
                c.Sample = e.Sample;
                c.FineTune = Samples[c.Sample].Finetune;
                c.Volume = Samples[c.Sample].Volume;
              }

            if (e.FXParm)
              c.FXBuf[e.FX] = e.FXParm;

            if (e.Note && (e.FX != 14 || ((e.FXParm >> 4) != 13)))
              {
                c.Note = e.Note;
                TrigNote (ch, e);
              }

            switch (e.FX)
              {
              case 4:					// vibrato (4) / vibrato + volume slide (6)
              case 6:

                if (c.FXBuf[4] & 0x0f)
                  c.VibAmpl = c.FXBuf[4] & 0x0f;
                if (c.FXBuf[4] & 0xf0)
                  c.VibSpeed = c.FXBuf[4] >> 4;
                c.SetPeriod (0,
                             VibTable[c.VibWave][(c.VibAmpl) - 1][c.VibPos]);
                break;

              case 7:					// tremolo (7)
                if (c.FXBuf[7] & 0x0f)
                  c.TremAmpl = c.FXBuf[7] & 0x0f;
                if (c.FXBuf[7] & 0xf0)
                  c.TremSpeed = c.FXBuf[7] >> 4;
                TremVol = VibTable[c.TremWave][(c.TremAmpl) - 1][c.TremPos];
                break;

              case 12:					// set volume (C)
                c.Volume = sClamp (e.FXParm, 0, 64);
                break;

              case 14:					// special (Exx)
							// (E0X - turn filter on/off),
							// (E1x - porta up, fine),
							// (E2x - porta down, fine),
							// (E3x - glissando control),
							// (E4x - vibratio waveform),
							// (E5x - set finetune),
							// (E6x - pattern loop),
							// (E7x - tremolo waveform),
							// (E8x - not implemented),
							// (E9x - retrigger note),
							// (EAx - volume slide up, fine),
							// (EBx - volume slide down, fine),
							// (ECx - cut note),
							// (EDx - delay note),
							// (EEx - pattern delay),
							// (EFx - not implemented)
                if (fxpl)
                  c.FXBuf14[e.FXParm >> 4] = fxpl;
                switch (e.FXParm >> 4)
                  {
                  case 0:				// set filter (0x)
                    break;

                  case 1:				// fineslide up (1x)
                    c.Period = sMax (113, c.Period - c.FXBuf14[1]);
                    break;

                  case 2:				// slide down (2x)
                    c.Period = sMin (856, c.Period + c.FXBuf14[2]);
                    break;

                  case 3:				// set glissando sucks! (0/1)
                    break;

                  case 4:				// set vib waveform (1/2)
                    c.VibWave = fxpl & 3;
                    if (c.VibWave == 3)
                      c.VibWave = 0;
                    c.VibRetr = fxpl & 4;
                    break;

                  case 5:				// set finetune
                    c.FineTune = fxpl;
                    if (c.FineTune >= 8)
                      c.FineTune -= 16;
                    break;

                  case 7:				// set tremolo (1/2)
                    c.TremWave = fxpl & 3;
                    if (c.TremWave == 3)
                      c.TremWave = 0;
                    c.TremRetr = fxpl & 4;
                    break;

                  case 9:				// retrigger
                    if (c.FXBuf14[9] && !e.Note)
                      TrigNote (ch, e);
                    c.RetrigCount = 0;
                    break;

                  case 10:				// fine volslide up
                    c.Volume = sMin (c.Volume + c.FXBuf14[10], 64);
                    break;

                  case 11:				// fine volslide down;
                    c.Volume = sMax (c.Volume - c.FXBuf14[11], 0);
                    break;

                  case 14:				// delay pattern
                    Delay = c.FXBuf14[14];
                    break;

                  case 15:				// invert loop (WTF)
                    break;

                  }
                break;					// case 14 end

              case 15:					// set speed (F)
                if (e.FXParm)
                  if (e.FXParm <= 32)
                    Speed = e.FXParm;
                  else
                    CalcTickRate (e.FXParm);
                break;
              }
          }
        else
          {
            switch (e.FX)
              {
              case 0:					// arpeggio (or normal play)
                if (e.FXParm)
                  {
                    sInt no = 0;
                    switch (CurTick % 3)
                      {
                      case 1:
                        no = e.FXParm >> 4;
                        break;
                      case 2:
                        no = e.FXParm & 0x0f;
                        break;
                      }
                    c.SetPeriod (no);
                  }
                break;

              case 1:					// slide up
                c.Period = sMax (113, c.Period - c.FXBuf[1]);
                break;

              case 2:					// slide down
                c.Period = sMin (856, c.Period + c.FXBuf[2]);
                break;

              case 5:					// Tone Portamento + volume slide slide
                if (c.FXBuf[5] & 0xf0)
                  c.Volume = sMin (c.Volume + (c.FXBuf[5] >> 4), 0x40);
                else
                  c.Volume = sMax (c.Volume - (c.FXBuf[5] & 0x0f), 0);
							// no break!
              case 3:					// tone portamento (slide to note) slide speed
                {
                  sInt np = c.GetPeriod ();
                  if (c.Period > np)
                    c.Period = sMax (c.Period - c.FXBuf[3], np);
                  else if (c.Period < np)
                    c.Period = sMin (c.Period + c.FXBuf[3], np);
                }
                break;

              case 6:					// vibrato plus volslide
                if (c.FXBuf[6] & 0xf0)
                  c.Volume = sMin (c.Volume + (c.FXBuf[6] >> 4), 0x40);
                else
                  c.Volume = sMax (c.Volume - (c.FXBuf[6] & 0x0f), 0);
							// no break!
              case 4:					// vibrato (speed + depth)
                c.SetPeriod (0, VibTable[c.VibWave][c.VibAmpl - 1][c.VibPos]);
                c.VibPos = (c.VibPos + c.VibSpeed) & 0x3f;
                break;

              case 7:					// tremolo (rate + depth)
                TremVol = VibTable[c.TremWave][c.TremAmpl - 1][c.TremPos];
                c.TremPos = (c.TremPos + c.TremSpeed) & 0x3f;
                break;

              case 10:					// volume slide
                if (c.FXBuf[10] & 0xf0)
                  c.Volume = sMin (c.Volume + (c.FXBuf[10] >> 4), 0x40);
                else
                  c.Volume = sMax (c.Volume - (c.FXBuf[10] & 0x0f), 0);
                break;

              case 11:					// position jump
                if (CurTick == Speed - 1)
                  {
                    CurRow = -1;
                    CurPos = e.FXParm;
                  }
                break;

              case 13:					// pattern break
                if (CurTick == Speed - 1)
                  {
                    CurPos++;
                    CurRow = (10 * (e.FXParm >> 4) + (e.FXParm & 0x0f)) - 1;
                  }
                break;

              case 14:					// set filter (special)
                switch (e.FXParm >> 4)
                  {
                  case 6:				// loop pattern
                    if (!fxpl)				// loop start
                      c.LoopStart = CurRow;
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

                  case 9:				// set sample offset (re-trigger)
                    if (++c.RetrigCount == c.FXBuf14[9])
                      {
                        c.RetrigCount = 0;
                        TrigNote (ch, e);
                      }
                    break;

                  case 12:				// set volume
                    if (CurTick == c.FXBuf14[12])
                      c.Volume = 0;
                    break;

                  case 13:				// pattern break
                    if (CurTick == c.FXBuf14[13])
                      TrigNote (ch, e);
                    break;

                  }
                break;
              }
          }

        v.Volume = sClamp (c.Volume + TremVol, 0, 64);
        v.Period = c.Period;
      }

    CurTick++;
    if (CurTick >= Speed * (Delay + 1))
      {
        CurTick = 0;
        CurRow++;
        Delay = 0;
      }
    if (CurRow >= 64)
      {
        CurRow = 0;
        CurPos++;
      }
    if (CurPos >= PositionCount)
      CurPos = 0;
  };							// end tick routine

// ************ MODPLAYER CONSTRUCTOR ************
public:
  char Name[21];					// song name

  ModPlayer (Paula *p, sU8 *moddata) : P (p)		// ModPlayer constructor (paula object and MOD data)
  {
    for (sInt ft = 0; ft < 16; ft++)			// calc ptable (period table) - finetune
      {
        sInt rft = -((ft >= 8) ? ft - 16 : ft);
        sF32 fac = sFPow (2.0f, sF32 (rft) / (12.0f * 16.0f));
        for (sInt i = 0; i < 60; i++)
          PTable[ft][i] = sInt (sF32 (BasePTable[i]) * fac + 0.5f);
      }

    for (sInt ampl = 0; ampl < 15; ampl++)		// calc vibtable - vibrato amplitude
      {
        sF32 scale = ampl + 1.5f;
        sF32 shift = 0;
        for (sInt x = 0; x < 64; x++)
          {
            VibTable[0][ampl][x] = sInt (scale * sFSin (x * sFPi / 32.0f) + shift);
            VibTable[1][ampl][x] = sInt (scale * ((63 - x) / 31.5f - 1.0f) + shift);
            VibTable[2][ampl][x] = sInt (scale * ((x < 32) ? 1 : -1) + shift);
          }
      }
							// == "load" the mod
    memcpy (Name, moddata, 20);				// copy from source to destination
    Name[20] = 0;
    moddata += 20;					// begining of sample data

    SampleCount = 32;					// 32 samples (default was 16)
    ChannelCount = 4;					// 4 channels
    Samples = (Sample *)(moddata - sizeof (Sample));
    moddata += 15 * sizeof (Sample);			// number of positions
    sU32 &tag = *(sU32 *)(moddata + 130 + 16 * sizeof (Sample)); // get tag info (treat result as unsigned 32 then dereference)
    switch (tag)					// magic number / signature / i.d. / tag string
      {
      case '.K.M':					// Michael Kleps (M.K.)
      case '4TLF':					// Startrekker 4 channel (fairlight) (FLT4)
      case '!K!M':					// more than 100 patterns (M!K!)
        SampleCount = 32;				// 32 samples if M.K. FLT4, M!K!
        break;
      }
    if (SampleCount > 16)
      moddata += (SampleCount - 16) * sizeof (Sample);	// moddata=moddata+(Sampl.......

    for (sInt i = 1; i < SampleCount; i++)
      Samples[i].Prepare ();

    PositionCount = *moddata;
    moddata += 2;					// + skip unused byte
    memcpy (PatternList, moddata, 128);
    moddata += 128;
    if (SampleCount > 15)
      moddata += 4;					// skip tag

    PatternCount = 0;
    for (sInt i = 0; i < 128; i++)
      PatternCount = sClamp (PatternCount, PatternList[i] + 1, 128);

    for (sInt i = 0; i < PatternCount; i++)
      {
        Patterns[i].Load (moddata);
        moddata += 1024;
      }

    sZeroMem (SData, sizeof (SData));			// zap memory?
    for (sInt i = 1; i < SampleCount; i++)
      {
        SData[i] = (sS8 *)moddata;
        moddata += 2 * Samples[i].Length;
      }

    Reset ();
  }							// ModPlayer constructor end

// ************ RENDER output************
  sU32 Render (sF32 *buf, sU32 len)			// Render paramaters (pointer to buffer and length of buffer)
  {
    while (len)
      {
        sInt todo = sMin<sInt> (len, TRCounter);	// sMin function using template

        if (todo)
          {
            P->Render (buf, todo);			// paula buffer and todo
            buf += 2 * todo;
            len -= todo;
            TRCounter -= todo;				// tick rate counter
          }
        else
          {
            Tick ();
            TRCounter = TickRate;
          }
      }
    return 1;
  }							// Render end

// ************ CALLBACK FUNCTION ************
  static sU32 __stdcall RenderProxy (void *parm, sF32 *buf, sU32 len)	// (modplayer is parm)
  {
    return ((ModPlayer *)parm)->Render (buf, len);	// typecast void pointer (parm) into modplayer object and access 'Render' member.
  }							// (returns buffer to the audio out)
};

// ************ PERIOD TABLE ************

sInt ModPlayer::BasePTable[61] = 						// scope resolution (belongs to ModPlayer class)
    {
	 0,									// finetune = 0
      1712, 1616, 1525, 1440, 1357, 1281, 1209, 1141, 1077, 1017, 961, 907,	// C-0 to B-0 (octave 0)
       856,  808,  762,  720,  678,  640,  604,  570,  538,  508, 480, 453,	// C-1 to B-1 (octave 1)
       428,  404,  381,  360,  339,  320,  302,  285,  269,  254, 240, 226,	// C-2 to B-2 (octave 2)
       214,  202,  190,  180,  170,  160,  151,  143,  135,  127, 120, 113,	// C-3 to B-3 (octave 3)
       107,  101,   95,   90,   85,   80,   76,   71,   67,   64,  60,  57,	// C-4 to B-4 (octave 4)
    };

sInt ModPlayer::PTable[16][60];
sInt ModPlayer::VibTable[3][15][64];

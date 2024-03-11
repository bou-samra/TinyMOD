// ========================= Paula Class (Paula Emulator) =======================

class Paula
{
public:
  static const sInt FIR_WIDTH = 512;			// Finite Impulse Response (FIR) filter width
  sF32 FIRMem[2 * FIR_WIDTH + 1];			// FIR memory (1025), one dimensional array

  struct Voice						// Start Voice Structure
  {
  private:
    sInt Pos;						// position ?
    sInt PWMCnt, DivCnt;				// Pulse Width Modulation, pwm division count?
    sIntFlt Cur;					// current ?

  public:
    sS8 *Sample;					// audio channel data (sample) location
    sInt SampleLen;					// audio channel data (sample) length
    sInt LoopLen;					// loop length
    sInt Period;					// 124 .. 65535 (audio channel period (rate))
    sInt Volume;					// 0 .. 64 AUDxVOL

    Voice ()
        : Period (65535), Volume (0), Sample (0), Pos (0), PWMCnt (0), DivCnt (0), LoopLen (1)
    {
      Cur.F32 = 0;
    }							// voice constructor ( initailization list - zero everything)

public:
    void Render (sF32 *buffer, sInt samples)		// define render function
    {
      if (!Sample)					// return if no samples... i think
        return;

      sU8 *smp = (sU8 *)Sample;
      for (sInt i = 0; i < samples; i++)
        {
          if (!DivCnt)
            {						// todo: use a fake d/a table for this
              Cur.U32 = ((smp[Pos] ^ 0x80) << 15) | 0x40000000;	// smp[pos] XOR 0x80 << 15 OR 4000 0000
              Cur.F32 -= 3.0f;
              if (++Pos == SampleLen)
                Pos -= LoopLen;
              DivCnt = Period;
            }
          if (PWMCnt < Volume)
            buffer[i] += Cur.F32;			// PWM counter
          PWMCnt = (PWMCnt + 1) & 0x3f;			// 0x3f = 63
          DivCnt--;
        }
    }							// end render function
public:
    void Trigger (sS8 *smp, sInt sl, sInt ll, sInt offs = 0)	// define trigger function (trigger voice data)
    {
      Sample = smp;					// sample
      SampleLen = sl;					// sample length
      LoopLen = ll;					// looplength
      Pos = sMin (offs, SampleLen - 1);			// offset
    }							// end trigger function
//  };							// end voice structure
//  Voice V[4];
} V[4];							// create array of instance of voice structure

  // --
  // rendering in paula freq
  static const sInt RBSIZE = 4096;			// ring buffer (aka circular buffer) size
  sF32 RingBuf[2 * RBSIZE];
  sInt WritePos;					// write position
  sInt ReadPos;						// read position
  sF32 ReadFrac;					// fraction?
public:
  void CalcFrag (sF32 *out, sInt samples)		// i believe this function transfers
							// samples into ring buffer
  {
    sZeroMem (out, sizeof (sF32) * samples);		// zero-out mem
    sZeroMem (out + RBSIZE, sizeof (sF32) * samples);
    for (sInt i = 0; i < 4; i++)			// four voices(0 - 3)
      {
        if (i == 1 || i == 2)
          V[i].Render (out + RBSIZE, samples);
        else
          V[i].Render (out, samples);
      }
  }

  // =================================== Calc
public:
  void Calc ()
  {
    sInt RealReadPos = ReadPos - FIR_WIDTH - 1;
    sInt samples = (RealReadPos - WritePos) & (RBSIZE - 1);

    sInt todo = sMin (samples, RBSIZE - WritePos);
    CalcFrag (RingBuf + WritePos, todo);
    if (todo < samples)
      {
        WritePos = 0;
        todo = samples - todo;
        CalcFrag (RingBuf, todo);
      }
    WritePos += todo;
  };							// Calc end

  // =================== rendering in output freq P->Render
public:
  sF32 MasterVolume;					// master volume
  sF32 MasterSeparation;				// master stereo separation

  void Render (sF32 *outbuf, sInt samples)		// iutput buffer
  {
    const sF32 step = sF32 (PAULARATE) / sF32 (OUTRATE);// ratio paula/output rate step (3740000/48000 = 77.92)
    const sF32 pan = 0.5f + 0.5f * MasterSeparation;	// audio panning (50% each left/right) (0.5 + 0.5 * 0.5 = 0.75)
    const sF32 vm0 = MasterVolume * sFSqrt (pan);	// master volume 0
    const sF32 vm1 = MasterVolume * sFSqrt (1 - pan);	// master volume 1

    for (sInt s = 0; s < samples; s++)
      {
        sInt ReadEnd = ReadPos + FIR_WIDTH + 1;
        if (WritePos < ReadPos)
          ReadEnd -= RBSIZE;
        if (ReadEnd > WritePos)
          Calc ();					// call calc() - render in paula rate
        sF32 outl0 = 0, outl1 = 0;			// out left
        sF32 outr0 = 0, outr1 = 0;			// out right

        sInt offs
            = (ReadPos - FIR_WIDTH - 1) & (RBSIZE - 1);	// offset [this needs optimization. SSE would
							// come to mind. (streaming SMID extensions)]
        sF32 vl = RingBuf[offs];
        sF32 vr = RingBuf[offs + RBSIZE];
        for (sInt i = 1; i < 2 * FIR_WIDTH - 1; i++)
          {
            sF32 w = FIRMem[i];				// w = FIRMem[i]
            outl0 += vl * w;				// outl0 = outl0 + (vl * w)
            outr0 += vr * w;				// outr0 = outr0 + (vl * w)
            offs = (offs + 1) & (RBSIZE - 1);
            vl = RingBuf[offs];
            vr = RingBuf[offs + RBSIZE];
            outl1 += vl * w;
            outr1 += vr * w;
          }
        sF32 outl = sLerp (outl0, outl1, ReadFrac);	// output left
        sF32 outr = sLerp (outr0, outr1, ReadFrac);	// output right
        *outbuf++ = vm0 * outl + vm1 * outr;
        *outbuf++ = vm1 * outl + vm0 * outr;

        ReadFrac += step;
        sInt rfi = sInt (ReadFrac);
        ReadPos = (ReadPos + rfi) & (RBSIZE - 1);
        ReadFrac -= rfi;
      }
  }							// Render end

  // --
public:
  Paula ()						// paula constructor
  {
							// make Finite Impulse Response (FIR) table (for low pass filter?)
    sF32 *FIRTable = FIRMem + FIR_WIDTH;		// FIR table size
    sF32 yscale = sF32 (OUTRATE) / sF32 (PAULARATE);	// Y scale
    sF32 xscale = sFPi * yscale;			// X scale
    for (sInt i = -FIR_WIDTH; i <= FIR_WIDTH; i++)	// windowed-sinc FIR filter (product of sinc & window function)
      FIRTable[i]
          = yscale * sFSinc (sF32 (i) * xscale)
            * sFHamming (sF32 (i)
                         / sF32 (FIR_WIDTH
                                 - 1));			// Firtable = (yscale) * (sinc(i) * xscale) * hamming(i) / (fir_width-1)

    sZeroMem (RingBuf, sizeof (RingBuf));
    ReadPos = 0;
    ReadFrac = 0;
    WritePos = FIR_WIDTH;				// reset ring buffer

    MasterVolume = 0.66f;				// master volume 66%
    MasterSeparation = 0.5f;				// stereo seperation 50:50
//  FltBuf = 0;
  }							// Paula Constructor end
};

typedef int sInt;					// signed Integer
typedef unsigned int sUInt;				// unsigned Integer

typedef sInt sBool;					// signed Bool
typedef char sChar;					// signed Character

typedef signed char sS8;				// 8 bit signed int
typedef signed short sS16;				// 16 bit signed int
typedef signed long sS32;				// 32 bit signed int
typedef int64_t sS64;					// 64 bit signed int (long long) (signed)

typedef unsigned char sU8;				// 8 bit unsigned int
typedef unsigned short sU16;				// 16 bit unsigned int
typedef unsigned long sU32;				// 32 bit unsigned int
typedef uint64_t sU64;					// 64 bit unsigned int (long long) (unsigned)

typedef float sF32;					// 32 bit signed float
typedef double sF64;					// 64 bit signed float

// #define _CRT_SECURE_NO_DEPRECATE			// disable C Runtime Library deprecation warnings

inline void sZeroMem(void *dest, sInt size)
{ memset(dest, 0, size);}				// declare & define function to zero memory

inline sF32 sFSqrt(sF32 x)
{ return sqrtf(x); }					// 32 bit signed float square root

inline sF32 sFSin(sF32 x)
{ return sinf(x); }					// 32 bit signed float sine

inline sF32 sFCos(sF32 x)
{ return cosf(x); }					// 32 bit signed float cosine

inline sF32 sFAtan(sF32 x)
{ return atanf(x); }					// 32 bit signed float arc tangent

inline sF32 sFPow(sF32 b, sF32 e)
{ return powf(b, e); }					// 32 bit signed float base to the power exponent

inline void sSwapEndian(sU16 &v)
{ v = ((v & 0xff) << 8) | (v >> 8); }			// endian swap

const sF32 sFPi = 4 * sFAtan(1);			// signed floating Pi

// =================== system dependent stuff ends here ========================

template <typename T> inline T sMin(const T a, const T b)
{ return (a < b) ? a : b; }				// min function, return smallest

template <typename T> inline T sMax(const T a, const T b)
{ return (a > b) ? a : b; }				// max function, return largest

template <typename T> inline T sClamp(const T x, const T min, const T max)
{ return sMax(min, sMin(max, x)); }			// variable clamp

template <typename T> T sSqr(T v)
{ return v * v; }					// square root

template <typename T> T sLerp(T a, T b, sF32 f)
{ return a + f * (b - a); }				// linear interpolation

template <typename T> T sAbs(T x)
{ return abs(x); }					// absolute value

inline sF32 sFSinc(sF32 x)
{ return x ? sFSin(x) / x : 1; }			// low pass filter: (sinc function) sin(x)/x or 1

inline sF32 sFHamming(sF32 x)
{ return (x > -1 && x < 1) ? sSqr(sFCos(x * sFPi / 2)) : 0; }	// hamming window: cos(X * PI / 2)^2 or 0

union sIntFlt {
    sU32 U32;
    sF32 F32;
};							// union integer/float

const sInt PAULARATE = 3740000;				// approx. pal timing   amiga paula rate (3.546895MHz DAC base clock)
const sInt OUTRATE = 48000;				// approx. pal timing   output rate (48Khz)
const sInt OUTFPS = 50;					// approx. pal timing   frames per second (50Hz - PAL)

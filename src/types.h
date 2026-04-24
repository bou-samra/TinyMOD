// =================== Type Definitions & Utilities ===================
// Standard type definitions, memory utilities, and math functions

#ifndef TYPES_H
#define TYPES_H

#include <inttypes.h>    // Fixed size integer types (C standard library)
#include <math.h>        // Mathematical operations (C standard library)
#include <string.h>      // String handling (C standard library)
#include <cstdint>       // C++ fixed size integer types

// =================== Type Definitions ===================
// Signed integer types
typedef int sInt;                          // Signed integer (platform dependent)
typedef signed char sS8;                   // 8-bit signed integer
typedef signed short sS16;                 // 16-bit signed integer
typedef signed long sS32;                  // 32-bit signed integer
typedef int64_t sS64;                      // 64-bit signed integer

// Unsigned integer types
typedef unsigned int sUInt;                // Unsigned integer (platform dependent)
typedef unsigned char sU8;                 // 8-bit unsigned integer
typedef unsigned short sU16;               // 16-bit unsigned integer
typedef unsigned long sU32;                // 32-bit unsigned integer
typedef uint64_t sU64;                     // 64-bit unsigned integer

// Floating point types
typedef float sF32;                        // 32-bit floating point (single precision)
typedef double sF64;                       // 64-bit floating point (double precision)

// Boolean type
typedef signed int sBool;                  // Boolean (0=false, non-zero=true)

// Character type
typedef char sChar;                        // Character

// =================== Float/Integer Union ===================
// Used for bit-level manipulation of floating point values
union sIntFlt {
    sU32 U32;                              // 32-bit unsigned integer view
    sF32 F32;                              // 32-bit floating point view
};

// =================== Memory Utilities ===================
// Zero out a memory block
inline void sZeroMem(void *dest, sInt size)
{
    memset(dest, 0, size);
}

// =================== Math Utilities ===================
// Min: return smallest of two values
template <typename T> inline T sMin(const T a, const T b)
{
    return (a < b) ? a : b;
}

// Max: return largest of two values
template <typename T> inline T sMax(const T a, const T b)
{
    return (a > b) ? a : b;
}

// Clamp: constrain value to min/max range
template <typename T> inline T sClamp(const T x, const T min, const T max)
{
    return sMax(min, sMin(max, x));
}

// Square: return x * x
template <typename T> T sSqr(T v)
{
    return v * v;
}

// Linear interpolation: a + f * (b - a)
template <typename T> T sLerp(T a, T b, sF32 f)
{
    return a + f * (b - a);
}

// Absolute value
template <typename T> T sAbs(T x)
{
    return (x < 0) ? -x : x;
}

// =================== Floating Point Math ===================
// Square root (32-bit float)
inline sF32 sFSqrt(sF32 x)
{
    return sqrtf(x);
}

// Sine (32-bit float)
inline sF32 sFSin(sF32 x)
{
    return sinf(x);
}

// Cosine (32-bit float)
inline sF32 sFCos(sF32 x)
{
    return cosf(x);
}

// Arc tangent (32-bit float)
inline sF32 sFAtan(sF32 x)
{
    return atanf(x);
}

// Power: base to the power of exponent (32-bit float)
inline sF32 sFPow(sF32 b, sF32 e)
{
    return powf(b, e);
}

// Pi constant: calculated as 4 * arctan(1)
const sF32 sFPi = 4 * sFAtan(1);

// =================== Signal Processing ===================
// Sinc function: sin(x)/x or 1 if x=0
// Used in windowed-sinc FIR filter design
inline sF32 sFSinc(sF32 x)
{
    return x ? sFSin(x) / x : 1;
}

// Hamming window: cos(X * PI / 2)^2 or 0
// Used to window the sinc function, reduces spectral leakage
inline sF32 sFHamming(sF32 x)
{
    return (x > -1 && x < 1) ? sSqr(sFCos(x * sFPi / 2)) : 0;
}

// =================== Endian Utilities ===================
// Swap endianness of 16-bit value (big-endian <-> little-endian)
inline void sSwapEndian(sU16 &v)
{
    v = ((v & 0xff) << 8) | (v >> 8);
}

// Compiler pragmas for optimization (optional)
// #pragma intrinsic(memset, sqrt, sin, cos, atan, powf)

#endif // TYPES_H

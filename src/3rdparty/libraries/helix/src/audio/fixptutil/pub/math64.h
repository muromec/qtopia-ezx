/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: math64.h,v 1.31 2006/06/06 22:14:02 gwright Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the RealNetworks Public
 * Source License (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the current version of the RealNetworks Community
 * Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply. You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.
 * 
 * This file, and the files included with this file, is distributed
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 * ENJOYMENT OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

/* platform-specific routines and macros. */

///////////////////////////////////////////////////////////////////////////////////////
// MSVC / i386
///////////////////////////////////////////////////////////////////////////////////////

#if (defined(_M_IX86) && defined(_MSC_VER)) || (defined(__WINS__) && defined(_SYMBIAN)) || (defined(__WINS__) && defined(WINCE_EMULATOR)) || (defined(_OPENWAVE_SIMULATOR))

#define HAVE_PLATFORM_MACROS

#pragma warning(disable:4035)
/* Compute a * b / c, using 64-bit intermediate result */
static __inline int MulDiv64(int a, int b, int c)
{
	__asm mov	eax, a
	__asm imul	b
	__asm idiv	c
}

/* Compute (a * b) >> 32, using 64-bit intermediate result */
static __inline int MulShift32(int a, int b)
{
	__asm mov	eax, a
	__asm imul	b
  __asm mov eax, edx
}

/* Compute (a * b) >> 31, using 64-bit intermediate result */
static __inline int MulShift31(int a, int b)
{
	__asm mov	eax, a
	__asm imul	b
	__asm shrd	eax, edx, 31
}

/* Compute (a * b) >> 30, using 64-bit intermediate result */
static __inline int MulShift30(int a, int b)
{
	__asm mov	eax, a
	__asm imul	b
	__asm shrd	eax, edx, 30
}

/* Compute (a * b) >> n, using 64-bit intermediate result */
static __inline int MulShiftN(int a, int b, int n)
{
	__asm mov	eax, a
	__asm imul	b
	__asm mov	ecx, n
	__asm shrd	eax, edx, cl
}
#ifdef DEBUG
#ifdef ASSERT
#undef ASSERT
#endif
#define ASSERT(x) if (!(x)) __asm int 3;
#endif

#ifdef TIMING
__int64 _timestamp;
__inline __int64 rdtsc() {
//    __asm   rdtsc  /* timestamp in edx:eax */
    __asm _emit 0x0f __asm _emit 0x31 // MSVC5 does not know rdtsc
}

#define TICK() _timestamp = rdtsc();
#define TOCK(nsamples) (_timestamp = rdtsc() - _timestamp, \
	printf("cycles =%4.0f\n", _timestamp / (double)(nsamples)) , _timestamp)
#endif // TIMING

#pragma warning(default:4035)

///////////////////////////////////////////////////////////////////////////////////////
// GCC / i386
///////////////////////////////////////////////////////////////////////////////////////

#elif !defined(_MAC_UNIX) && defined(__GNUC__) && (defined(__i386__) || defined(__amd64__))

#define HAVE_PLATFORM_MACROS

/* Compute a * b / c, using 64-bit intermediate result */
static __inline__ int MulDiv64(register int x, register int y, register int z)
{
    return (int)(((INT64)x*(INT64)y)/(INT64)z);
}

/* Compute (a * b) >> 32, using 64-bit intermediate result */
static __inline__ int MulShift32(int x, int y)
{
    return (int)(((INT64)x*(INT64)y)>>32);
}

/* Compute (a * b) >> 31, using 64-bit intermediate result */
static __inline__ int MulShift31(int x, int y)
{
    return (int)(((INT64)x*(INT64)y)>>31);
}

static __inline__ int MulShift30(int x, int y)
{
    return (int)(((INT64)x*(INT64)y)>>30);
}

/* Compute (a * b) >> n, using 64-bit intermediate result */
static __inline__ int MulShiftN(register int x, register int y, register int n)
{
    return (int)(((INT64)x*(INT64)y)>>n);
}

#ifdef TIMING
long long _timestamp;
static __inline__ long long rdtsc() {
    long long r ;
    __asm__ __volatile ("rdtsc" : "=A" (r)) ;
    return r ;    
}

#define TICK() _timestamp = rdtsc();
#define TOCK(nsamples) (_timestamp = rdtsc() - _timestamp, \
	printf("cycles =%4.0f\n", _timestamp / (double)(nsamples)), _timestamp)
#endif

#ifdef DEBUG
#  ifdef ASSERT
#    undef ASSERT
#  endif
#define ASSERT(x) if (!(x)) __asm__ __volatile ("int $3" :: )
#endif

///////////////////////////////////////////////////////////////////////////////////////
// Codewarrior / PowerPC
///////////////////////////////////////////////////////////////////////////////////////

#elif defined(__MWERKS__) && defined(__POWERPC__)

/*if your compiler can compile 64-bit instructions, define this. CW 8 cannot */
/* #define USE_64BIT_INSNS */

#define HAVE_PLATFORM_MACROS

/* Compute a * b / c, using 64-bit intermediate result */
#ifdef USE_64BIT_INSNS
inline int MulDiv64(register int a, register int b, register int c)
{
	asm {
		mulhd r0,a,b
		divd r0,r0,c
	}
}
#else
inline int MulDiv64(double a, double b, double c)
{
	return (int)( (a/c) * b ) ;
}
#endif

/* Compute (a * b) >> 30, using 64-bit intermediate result */
inline int MulShift30(register int a, register int b)
{
	register int res ;
#ifdef USE_64BIT_INSNS
	asm {
		mulhd res,a,b
		srd res,30
	}
#else
	asm {
		mulhw res,a,b
		slwi res,res,2 // not exact; last two bits are wrong
	}
#endif	
	return res ;
}

/* Compute (a * b) >> 31, using 64-bit intermediate result */
inline int MulShift31(register int a, register int b)
{
	register int res ;
#ifdef USE_64BIT_INSNS
	asm {
		mulhd res,a,b
		srd res,31
	}
#else
	asm {
		mulhw res,a,b
		slwi res,res,1 // not exact; last bit is wrong half the time
	}
#endif	
	return res ;
}

/* Compute (a * b) >> 32, using 64-bit intermediate result */
inline int MulShift32(register int a, register int b)
{
	register int res ;
	asm {
		mulhw res,a,b
	}
	return res ;
}

/* Compute (a * b) >> n, using 64-bit intermediate result */
//inline int MulShiftN(register int a, register int b, register int n)
static int MulShiftN(register int a, register int b, int n)
{
#ifdef USE_64BIT_INSNS
	register int res ;
	asm {
		mulhd res,a,b
		srd res,n
	}
	return res ;
#else
	register unsigned int temp ;
	int result ;

	asm {
		mullw  temp,a,b
	}
	result = temp >> n ;

	asm {	
		mulhw  temp,a,b
	}
	result |= (temp << (32-n)) ;
	
	return result ;
#endif
}

#ifdef TIMING
static unsigned int tick,tock ;
inline void fTICK()
{ register int t ; asm { mftb t } ; tick = t ; }
inline void fTOCK()
{ register int t ; asm { mftb t } ; tock = t ;
  if (tock < tick) {
  	tock += 65536 ; tick -= 65536 ; 
  }
}

#define TICK() fTICK()
#define TOCK(nsamples) ( fTOCK() , printf("cycles = %4.1f\n",4.0f*(tock-tick)/(float)(nsamples)), \
	tock-tick )

#endif // TIMING



///////////////////////////////////////////////////////////////////////////////////////
// GCC / PowerPC
///////////////////////////////////////////////////////////////////////////////////////

#elif defined(__GNUC__) && (defined(__POWERPC__) || defined(__powerpc__))

/*if your compiler can compile 64-bit instructions, and your CPU has them,
 define this. */
// #define USE_64BIT_INSNS

#define HAVE_PLATFORM_MACROS

/* Compute a * b / c, using 64-bit intermediate result */
static __inline__ int MulDiv64(int a, int b, int c)
{
	int res ;
#ifdef USE_64BIT_INSNS
	__asm__ volatile ("mulhd %0,%2,%3\n\t"
					  "divd %0,%0,%1"
					  : "=&r" (res) : "r" (c), "%r" (a), "r" (b) ) ;
#else
	res = (int)(((double)a*(double)b - (double)(c>>1)) / (double)c) ;
#endif
	return res ;
}

/* Compute (a * b) >> 32, using 64-bit intermediate result */
static __inline__ int MulShift32(int a, int b)
{
	int res ;
	__asm__ ("mulhw %0,%1,%2" : "=r" (res) : "%r" (a) , "r" (b) ) ;
	return res ;
}

/* Compute (a * b) >> 30, using 64-bit intermediate result */
static __inline__ int MulShift30(int a, int b)
{
	int res ;
#ifdef USE_64BIT_INSNS
	__asm__ ("mulhd %0,%1,%2\n\t"
			 "srd %0,30" : "=r" (res) : "%r" (a), "r" (b) ) ;
#else
	res = MulShift32(a,b) << 2 ;
#endif
	return res ;
}

/* Compute (a * b) >> 31, using 64-bit intermediate result */
static __inline__ int MulShift31(int a, int b)
{
	int res ;
#ifdef USE_64BIT_INSNS
	__asm__ ("mulhd %0,%1,%2\n\t"
			 "srd %0,31" : "=r" (res) : "%r" (a), "r" (b) ) ;
#else
	res = MulShift32(a,b) << 1 ;
#endif
	return res ;
}

/* Compute (a * b) >> n, using 64-bit intermediate result */
static __inline__ int MulShiftN(int a, int b, int n)
{
	int res ;

#ifdef USE_64BIT_INSNS
	__asm__ ("mulhd %0,%1,%2\n\t"
			 "srd %0,%3" : "=&r" (res) : "%r" (a), "r" (b), "r" (n) ) ;
#else
	unsigned int temp ;

	__asm__ ("mullw %0,%1,%2" : "=r" (temp) : "%r" (a) , "r" (b) ) ;

	res = temp >> n ;

	__asm__ ("mulhw %0,%1,%2" : "=r" (temp) : "%r" (a) , "r" (b) ) ;

	res |= (temp << (32-n)) ;
#endif
	return res ;
}

#ifdef TIMING
static unsigned int tick,tock ;
inline void fTICK()
{ register int t ; __asm__ ( "mftb %0" : "=r" (t) ) ; tick = t ; }
inline void fTOCK()
{ register int t ; __asm__ ( "mftb %0" : "=r" (t) ) ; tock = t ;
  if (tock < tick) {
  	tock += 65536 ; tick -= 65536 ; 
  }
}

#define TICK() fTICK()
#define TOCK(nsamples) ( fTOCK() , printf("cycles = %4.1f\n",4.0f*(tock-tick)/(float)(nsamples)), \
	tock-tick )

#endif // TIMING


///////////////////////////////////////////////////////////////////////////////////////
// EVC3.0 / ARM
///////////////////////////////////////////////////////////////////////////////////////

#elif (defined(_ARM) && defined(_MSC_VER))

/* EVC does not allow us to use inline assembly. Thus, you'll only see prototypes here.
 */

#define HAVE_PLATFORM_MACROS

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/* Compute a * b / c, using 64-bit intermediate result. */
extern int MulDiv64(int a, int b, int c); 

/* Compute (a * b) >> 32, using 64-bit intermediate result */
extern int MulShift32(int a, int b);

/* Compute (a * b) >> 31, using 64-bit intermediate result */
extern int MulShift31(int a, int b);

/* Compute (a * b) >> 30, using 64-bit intermediate result */
extern int MulShift30(int a, int b);

/* Compute (a * b) >> n, using 64-bit intermediate result */
extern int MulShiftN(int a, int b, int n);
#ifdef __cplusplus
}
#endif // __cplusplus


///////////////////////////////////////////////////////////////////////////////////////
// GNUC / ARM
///////////////////////////////////////////////////////////////////////////////////////

#elif (defined(_ARM) && defined(__GNUC__))

#define HAVE_PLATFORM_MACROS

#if defined(__MARM_THUMB__)

/* Compute a * b / c, using 64-bit intermediate result. Since the ARM does not have
   a division instruction, we code a totally lame C version here. TODO wschildbach
 */
static __inline int MulDiv64(int a, int b, int c)
{
  long long t = (long long)a * (long long)b ;
  return (int)(t / c) ;
}

/* Compute (a * b) >> 32, using 64-bit intermediate result */
static __inline__ int MulShift32(int x, int y)
{
    INT64 a = x;
    INT64 b = y;
    a *= b;
    a >>= 32;
    return INT64_TO_INT32(a);
}

/* Compute (a * b) >> 31, using 64-bit intermediate result */
static __inline__ int MulShift31(int x, int y)
{
    INT64 a = x;
    INT64 b = y;
    a *= b;
    a >>= 31;
    return INT64_TO_INT32(a);
}

/* Compute (a * b) >> 30, using 64-bit intermediate result */
static __inline__ int MulShift30(int x, int y)
{
    INT64 a = x;
    INT64 b = y;
    a *= b;
    a >>= 30;
    return INT64_TO_INT32(a);
}

/* Compute (a * b) >> n, using 64-bit intermediate result */
static __inline__ int MulShiftN(int x, int y, int n)
{
    INT64 a = x;
    INT64 b = y;
    a *= b;
    a >>= n;
    return INT64_TO_INT32(a);
}

#define HAVE_FASTABS
static __inline int FASTABS(int x)
{
    if (x >= 0)
	return x;
    return -x;
}

#else

#ifdef __cplusplus
extern "C" {
#endif

/* Compute a * b / c, using 64-bit intermediate result */
extern int MulDiv64(int a, int b, int c);

#ifdef __cplusplus
}
#endif

/* Compute (a * b) >> 32, using 64-bit intermediate result */
static __inline__ int MulShift32(int x, int y)
{
  int zlow ;
  __asm__ volatile ("smull %0,%1,%2,%3" : "=&r" (zlow), "=r" (x) : "%r" (y), "1" (x)) ;
  return x ;
}

/* Compute (a * b) >> 31, using 64-bit intermediate result */
static __inline__ int MulShift31(int x, int y)
{
  int zlow ;
  __asm__ volatile ("smull %0,%1,%2,%3" : "=&r" (zlow), "=r" (x) : "%r" (y), "1" (x)) ;
  __asm__ volatile ("mov %0,%1, lsr #31" : "=r" (zlow) : "r" (zlow)) ;
  __asm__ volatile ("orr %0,%1,%2, lsl #1" : "=r" (x) : "r" (zlow), "r" (x)) ;
  return x ;
}

/* Compute (a * b) >> 30, using 64-bit intermediate result */
static __inline__ int MulShift30(int x, int y)
{
  int zlow ;
  __asm__ volatile ("smull %0,%1,%2,%3" : "=&r" (zlow), "=r" (x) : "%r" (y), "1" (x)) ;
  __asm__ volatile ("mov %0,%1, lsr #30" : "=r" (zlow) : "r" (zlow)) ;
  __asm__ volatile ("orr %0,%1,%2, lsl #2" : "=r" (x) : "r" (zlow), "r" (x)) ;
  return x ;
}

/* Compute (a * b) >> n, using 64-bit intermediate result */
static __inline__ int MulShiftN(int x, int y, int n)
{
  int zlow ;
  __asm__ volatile ("smull %0,%1,%2,%3" : "=&r" (zlow), "=r" (x) : "%r" (y), "1" (x)) ;
  __asm__ volatile ("mov %0,%1, lsr %2" : "=r" (zlow) : "r" (zlow), "r" (n)) ;
  __asm__ volatile ("orr %0,%1,%2, lsl %3" : "=r" (x) : "r" (zlow), "r" (x), "r" (32-n)) ;
  return x ;
}

#define HAVE_FASTABS
static __inline int FASTABS(int x)
{
	int s;

  // s = x ^ (x >> 31)
  __asm__ volatile ("eor %0, %1, %1, asr #31" : "=r" (s) : "r" (x)) ;
  // x = s - (x >> 31)
  __asm__ volatile ("sub %0, %1, %2, asr #31" : "=r" (x) : "r" (s), "r" (x)) ;

	return x;
}

#endif // defined(__MARM_THUMB__)


///////////////////////////////////////////////////////////////////////////////////////
// ARM_ADS / ARM
///////////////////////////////////////////////////////////////////////////////////////

#elif defined(ARM_ADS)

#ifdef __cplusplus
extern "C" {
#endif
/* compute (a*b)/c, using 64 bit intermediate result. Use external assembly. */
extern int MulDiv64(int a, int b, int c); 
#ifdef __cplusplus
}
#endif

static __inline int MulShift32(int x, int y)
{
    /* JR - important rules for smull RdLo, RdHi, Rm, Rs:
     *        RdHi and Rm can't be the same register
     *        RdLo and Rm can't be the same register
     *        RdHi and RdLo can't be the same register
     *      for MULSHIFT0(x,y), x = R0 and y = R1
     *      therefore we do y*x instead of x*y so that top 32 can go in R0 (the return register)
     */
    int zlow;
    __asm {
    	smull zlow,x,y,x
   	}

    return x;
}

static __inline int MulShift31(int x, int y)
{
    /* JR - store result in z (instead of reusing zlow) so that gcc optimizes properly */
    int zlow, z;

    __asm {
    	smull 	zlow, x, y, x
		mov 	zlow, zlow, lsr #31
    	orr 	z, zlow, x, lsl #1
   	}

    return z;
}

static __inline int MulShift30(int x, int y)
{
    /* JR - store result in z (instead of reusing zlow) so that gcc optimizes properly */
    int zlow, z;
    
    __asm {
    	smull 	zlow, x, y, x
		mov 	zlow, zlow, lsr #30
    	orr 	z, zlow, x, lsl #2
   	}

    return z;
}

static __inline int MulShiftN(int x, int y, int n)
{
    int zlow, z;
    
    __asm {
        smull 	zlow, x, y, x
        mov 	zlow, zlow, lsr n
        rsb	n,n,#32
        orr 	z, zlow, x, lsl n
   	}

    return z;
}

#define HAVE_FASTABS
static __inline int FASTABS(int x) 
{
	int s;

	__asm {
		 eor	s, x, x, asr #31
		 sub	x, s, x, asr #31 
	}

	return x;
}


///////////////////////////////////////////////////////////////////////////////////////
// platform independent implementations
///////////////////////////////////////////////////////////////////////////////////////

#elif (defined(_MAC_UNIX) && defined(__i386__)) || (defined(_HPUX) || (defined(_SOLARIS) && !defined(__GNUC__)))

#ifndef ASSERT
#define ASSERT(x)
#endif

#ifndef TICK
#define TICK()
#endif

#ifndef TOCK
#define TOCK(nsamples) 1
#endif

#define HAVE_PLATFORM_MACROS
static __inline int MulDiv64(int a, int b, int c)
{
    long long t = (long long)a * (long long)b ;
    return (int)(t / c) ;
}

/* Compute (a * b) >> 32, using 64-bit intermediate result */
static __inline int MulShift32(int x, int y)
{
    long long t = (long long)x * (long long)y ;
    return (int)(t >> 32);
}

/* Compute (a * b) >> 31, using 64-bit intermediate result */
static __inline int MulShift31(int x, int y)
{
    long long t = (long long)x * (long long)y ;
    return (int)(t >> 31);
}

/* Compute (a * b) >> 30, using 64-bit intermediate result */
static __inline int MulShift30(int x, int y)
{
    long long t = (long long)x * (long long)y ;
    return (int)(t >> 30);
}

/* Compute (a * b) >> n, using 64-bit intermediate result */
static __inline int MulShiftN(int x, int y, int n)
{
    long long t = (long long)x * (long long)y ;
    return (int)(t >> n);
}

#define HAVE_FASTABS
static __inline int FASTABS(int x)
{
    if (x >= 0)
	return x;
    return -x;
}
#else
#error "You need to define 64 bit operations for your platform"
#endif

#ifndef HAVE_FASTABS
static __inline int FASTABS(int x) 
{
	int sign;

	sign = x >> 31;
	x ^= sign;
	x -= sign;

	return x;
}
#endif


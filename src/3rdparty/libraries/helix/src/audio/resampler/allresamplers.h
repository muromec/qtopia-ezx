/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: allresamplers.h,v 1.11 2007/07/06 20:21:28 jfinnecy Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#ifndef _ALLRESAMPLERS_H_
#define _ALLRESAMPLERS_H_

#ifdef __cplusplus
extern "C" {
#endif

struct CVTSTATEMACHINE {
  struct CVTSTATEMACHINE *pNext ;
  int incInput, incOutput ;
} ;

typedef int (* cvtFunctionType)(void* dst, const void* src, int n, const struct CVTSTATEMACHINE *pState);

typedef struct
{
  cvtFunctionType pfCvt ;
  struct CVTSTATEMACHINE *pStateMachine ;
} tConverter ;

#define NBLOCK 2058

#define MONO	1
#define STEREO	2

#ifndef MAX
#define MAX(a,b) (((b) > (a)) ? (b) : (a))
#endif
#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

/* defaults */

#define DEF_ATTEN 90.0f

#define DEF_PASSBAND 0.88f
#define DEF_STOPBAND 1.0f
#define DEF_DCGAIN 1.0f

/*
 * Compute greatest common divisor.
 */
static int
gcd(int a, int b)
{
  while (a != b) {
      if (a > b)
        a -= b;
      else
        b -= a;
  }
  return a;
}

/* float to short, with rounding and clipping */
static
#if !defined(_AIX) && !defined(_SOLARIS) && !defined(_HPUX) && !defined(_OSF1)
__inline
#endif
short
RoundFtoS(float f) {
	long l;
#if defined(_M_IX86)
	__asm fld	f
	__asm fistp	l
#elif defined(__GNUC__) && defined(__i386__)
	__asm__ __volatile ("fistl %0" : "=m" (l) : "t" (f)) ;
#else
	l = (long)(f < 0.0f ? f - 0.5f : f + 0.5f);
#endif
	if (l > 32767) l = 32767;
	else if (l < -32768) l = -32768;
	return(short)l;
}

#ifndef _AIX
typedef unsigned char uchar;
#if !defined(_HPUX) && !defined(_LINUX) && !defined(_MAC_MACHO) && !defined(_FREEBSD4)
typedef unsigned int uint;
#endif
#endif

// Rational

void *
RAInitResamplerRat(int inrate, int outrate, int chans, float atten, float passband, float stopband, float dcgain);
void *
RAInitResamplerCopyRat(int nchans, const void *inst);
void
RAFreeResamplerRat(void *inst);

int
RAResampleMonoRat(void *inbuf, int insamps, tConverter *pCvt, short *outbuf, int outstride, void *inst) ;
int
RAResampleStereoRat(void *inbuf, int insamps, tConverter *pCvt, short *outbuf, int outstride, void *inst) ;

int
RAGetMaxOutputRat(int insamps, void *inst);
int
RAGetMinInputRat(int outsamps, void *inst);
int
RAGetDelayRat(void *inst);

// Arbitrary

void *
RAInitResamplerArb(int inrate, int outrate, int chans, float atten, float passband, float stopband, float dcgain);
void *
RAInitResamplerCopyArb(int nchans, const void *inst);
void
RAFreeResamplerArb(void *inst);

int
RAResampleMonoArb(void *inbuf, int insamps, tConverter *pCvt, short *outbuf, int outstride, void *inst) ;
int
RAResampleStereoArb(void *inbuf, int insamps, tConverter *pCvt, short *outbuf, int outstride, void *inst) ;

int
RAGetMaxOutputArb(int insamps, void *inst);
int
RAGetMinInputArb(int outsamps, void *inst);
int
RAGetDelayArb(void *inst);

// MMX resamplers

void *
RAInitResamplerMMX(int inrate, int outrate, int chans);
void *
RAInitResamplerCopyMMX(int nchans, const void *inst);
void
RAFreeResamplerMMX(void *inst);

int
RAResampleMonoMMX(void *inbuf, int insamps, tConverter *pCvt, short *outbuf, int outstride, void *inst) ;
int
RAResampleStereoMMX(void *inbuf, int insamps, tConverter *pCvt, short *outbuf, int outstride, void *inst) ;

int
RAGetMaxOutputMMX(int insamps, void *inst);
int
RAGetMinInputMMX(int outsamps, void *inst);
int
RAGetDelayMMX(void *inst);


// Hermite interpolation

void *
RAInitResamplerHermite(int inrate, int outrate, int chans);
void *
RAInitResamplerCopyHermite(int nchans, const void *inst);
void
RAFreeResamplerHermite(void *inst);

int
RAResampleMonoHermite(void *inbuf, int insamps, tConverter *pCvt, short *outbuf, int outstride, void *inst);
int
RAResampleStereoHermite(void *inbuf, int insamps, tConverter *pCvt, short *outbuf, int outstride, void *inst);

int
RAGetMaxOutputHermite(int insamps, void *inst);
int
RAGetMinInputHermite(int outsamps, void *inst);
int
RAGetDelayHermite(void *inst);

#ifdef __cplusplus
}
#endif

#endif /* _ALLRESAMPLERS_H_ */

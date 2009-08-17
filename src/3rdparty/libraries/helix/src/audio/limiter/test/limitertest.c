/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: limitertest.c,v 1.5 2004/07/09 18:36:51 hubbe Exp $
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

/* unit test for fixed point helper routines and limiter. */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.141592654
#endif

#define TIMING
#include "math64.h"
#include "limiter.h"
#include "cpuident.h"

#define N 2048
static float f[N] ;

static int a[N] ;
static int b[N] ;
static int c[N] ;
static int d[N] ;

/* see NR in C, chap. 7 */
unsigned int my_rand(unsigned int state)
{
  return state * 1664525L + 1013904223L ;
}

static void prepare_rand(int r[])
{
    int i ;
    unsigned int randstate = 0xDEADBEEF ;
    for (i = 0 ; i < N ; i++)
    {
        // generate random numbers between -2^16 and +2^16
        r[i] = (randstate >> 16) - (1<<15) ;
        randstate = my_rand(randstate) ;
    }
}

#if 1
#undef ASSERT
#define ASSERT(x) if(!(x)){printf("line %d: arg1=%d,arg2=%d,res=%d,true=%d\n",\
  __LINE__,arg1,a[i],res2,res1);exit(10);}
#endif

int main(void)
{
    int i,j ;
    int res1,res2 ;
	LIMSTATE* lim ;
	clock_t t ;
	long cycles ;
	float cyclesPerSec ;

    CPUInformation cpuInfo ;
    CPUIdentify(&cpuInfo) ;

    prepare_rand(a);
    prepare_rand(b);
    for (i = 0 ; i < N ; i++)
    {
        c[i] = a[i] ;
	if (!c[i]) c[i] = 1 ;
        if (fabs((float)a[i] * (float)b[i] / (float)c[i]) > (float)0x7fffffffUL)
	  c[i] = 0x7fffffffUL ;
    }

    // test the 64-bit commands

    for (i = 0 ; i < N ; i++)
    {
      signed int arg1 = (-1L<<31);
        res1 = a[i] ;
        res2 = -MulShift31(arg1,a[i]) ;
        ASSERT(res1 == res2) ;
    }
    printf("MulShift31 test passed.\n");

    for (j = 0 ; j < 32 ; j++)
    {
        for (i = 0 ; i < N ; i++)
        {
            int arg1 = (-1L<<j) ;
            res1 = a[i] ;
            res2 = -MulShiftN(arg1,a[i],j) ;
            ASSERT(res1 == res2) ;
            res2 = -MulShiftN(a[i],arg1,j) ;
            ASSERT(res1 == res2) ;
        }
    }
#if 0 // don't test for shifts of >= 32
    for (j = 32 ; j < 64 ; j++)
    {
        for (i = 0 ; i < N ; i++)
        {
            res1 = a[i]>>(j-31) ;
            res2 = -MulShiftN((-1L<<31),a[i],j) ;
            ASSERT(res1 == res2) ;
        }
    }
#endif
    printf("MulShiftN test passed.\n");

    for (j = 0 ; j < 32 ; j++)
    {
        for (i = 0 ; i < N ; i++)
        {
            signed int arg1 = (-1L<<j) ;
            res1 = a[i] ;
            res2 = MulDiv64(arg1,a[i],arg1) ;
            ASSERT(res1 == res2) ;
            if (a[i])
            {
              res2 = MulDiv64(arg1,a[i],a[i]) ;
              ASSERT(arg1 == res2) ;
            }
        }
    }
    printf("MulDiv64 test passed.\n");

    printf("measuring clock frequency\n");
	// try to measure cpu clock
	t = clock() ; TICK() ;
        j = 3;
	for (i = 0 ; i < 5000000 ; i++) j *= j ; // do something... do anything
	cycles = (long)TOCK(i) ;
	t = clock()-t ;
	cyclesPerSec = (float)cycles * CLOCKS_PER_SEC / t ;
	printf("\rapproximate CPU clock: %3.1f MHz\n",1E-6*cyclesPerSec) ;

    // timing tests
    printf("MulShift30: ");

    TICK() ;
    for (i = 0 ; i < N ; i+=4)
    {
        d[i  ] = MulShift30(a[i  ],b[i  ]) ;
        d[i+1] = MulShift30(a[i+1],b[i+1]) ;
        d[i+2] = MulShift30(a[i+2],b[i+2]) ;
        d[i+3] = MulShift30(a[i+3],b[i+3]) ;
    }
    cycles = (long)TOCK(N) ;

    // timing tests
    printf("MulShift31: ");

    TICK() ;
    for (i = 0 ; i < N ; i+=4)
    {
        d[i  ] = MulShift31(a[i  ],b[i  ]) ;
        d[i+1] = MulShift31(a[i+1],b[i+1]) ;
        d[i+2] = MulShift31(a[i+2],b[i+2]) ;
        d[i+3] = MulShift31(a[i+3],b[i+3]) ;
    }
    cycles = (long)TOCK(N) ;

    // timing tests
    printf("MulShift32: ");

    TICK() ;
    for (i = 0 ; i < N ; i+=4)
    {
        d[i  ] = MulShift32(a[i  ],b[i  ]) ;
        d[i+1] = MulShift32(a[i+1],b[i+1]) ;
        d[i+2] = MulShift32(a[i+2],b[i+2]) ;
        d[i+3] = MulShift32(a[i+3],b[i+3]) ;
    }
    cycles = (long)TOCK(N) ;

    // timing tests
    printf("MulShiftN: ");

    TICK() ;
    for (i = 0 ; i < N ; i+=4)
    {
        d[i  ] = MulShiftN(a[i  ],b[i  ],31) ;
        d[i+1] = MulShiftN(a[i+1],b[i+1],31) ;
        d[i+2] = MulShiftN(a[i+2],b[i+2],31) ;
        d[i+3] = MulShiftN(a[i+3],b[i+3],31) ;
    }
    TOCK(N) ;

    // timing tests
    printf("MulDiv64: ");

    TICK() ;
    for (i = 0 ; i < N ; i+=4)
    {
        d[i  ] = MulDiv64(a[i  ],b[i  ],c[i  ]) ;
        d[i+1] = MulDiv64(a[i+1],b[i+1],c[i+1]) ;
        d[i+2] = MulDiv64(a[i+2],b[i+2],c[i+2]) ;
        d[i+3] = MulDiv64(a[i+3],b[i+3],c[i+3]) ;
    }
    TOCK(N) ;

	// test the limiter, no clipping
	for (i = 0 ; i < N ; i++)
	{
		d[i] = (signed int)((1UL<<31)*sin(i * 2.0*M_PI / N)) ;
	}

	printf("memcpy: ");
	TICK() ;
	for (i = 0 ; i < 100 ; i++)
	{
		memcpy(a,d,sizeof(d)) ; /* Flawfinder: ignore */
	}
	TOCK(100*N) ;

	printf("Mono limiter: ");
	lim = LimiterInit(44100,1,0) ; // 0dB headroom
	TICK() ;
	for (i = 0 ; i < 100 ; i++)
	{
		memcpy(a,d,sizeof(d)) ; /* Flawfinder: ignore */
		LimiterProcess(a,N,lim) ;
	}
	cycles = (long)TOCK(100*N) ;
	printf("%1.1lf%% realtime\n",100.0*cycles/cyclesPerSec * (44100.0/(100*N))) ;
	LimiterFree(lim) ;

	printf("Mono limiter, overgain: ");
	lim = LimiterInit(44100,1,1) ; // 1dB headroom
	TICK() ;
	for (i = 0 ; i < 100 ; i++)
	{
		memcpy(a,d,sizeof(d)) ; /* Flawfinder: ignore */
		LimiterProcess(a,N,lim) ;
	}
	cycles = (long)TOCK(100*N) ;
	printf("%1.1lf%% realtime\n",100.0*cycles/cyclesPerSec * (44100.0/(100*N))) ;
	LimiterFree(lim) ;

	printf("Stereo limiter: ");
	lim = LimiterInit(44100,2,0) ; // 0dB headroom
	TICK() ;
	for (i = 0 ; i < 100 ; i++)
	{
		memcpy(a,d,sizeof(d)) ; /* Flawfinder: ignore */
		LimiterProcess(a,N,lim) ;
	}
	cycles = (long)TOCK(50*N) ;
	printf("%1.1lf%% realtime\n",100.0*cycles/cyclesPerSec * (44100.0/(50*N))) ;
	LimiterFree(lim) ;

	printf("Stereo limiter, overgain: ");
	lim = LimiterInit(44100,2,1) ; // 1dB headroom
	TICK() ;
	for (i = 0 ; i < 100 ; i++)
	{
		memcpy(a,d,sizeof(d)) ; /* Flawfinder: ignore */
		LimiterProcess(a,N,lim) ;
	}
	cycles = (long)TOCK(50*N) ;
	printf("%1.1lf%% realtime\n",100.0*cycles/cyclesPerSec * (44100.0/(50*N))) ;
	LimiterFree(lim) ;
	
    return 0;
}

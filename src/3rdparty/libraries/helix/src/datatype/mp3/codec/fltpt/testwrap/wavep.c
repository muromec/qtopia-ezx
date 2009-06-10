/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#include <stdlib.h>
#include <stdio.h>
#include "hlxclib/float.h"
#include <math.h>
#include <io.h>
#include "port.h"

typedef struct {
unsigned char riff[4]; /* Flawfinder: ignore */
unsigned char size[4]; /* Flawfinder: ignore */
unsigned char wave[4]; /* Flawfinder: ignore */
unsigned char fmt[4]; /* Flawfinder: ignore */
unsigned char fmtsize[4]; /* Flawfinder: ignore */
unsigned char tag[2]; /* Flawfinder: ignore */
unsigned char nChannels[2]; /* Flawfinder: ignore */
unsigned char nSamplesPerSec[4]; /* Flawfinder: ignore */
unsigned char nAvgBytesPerSec[4]; /* Flawfinder: ignore */
unsigned char nBlockAlign[2];
unsigned char nBitsPerSample[2];
unsigned char data[4]; /* Flawfinder: ignore */
unsigned char pcm_bytes[4]; /* Flawfinder: ignore */
} BYTE_WAVE;

static BYTE_WAVE wave = {
"RIFF",
sizeof(BYTE_WAVE) - 8, 0, 0, 0,
"WAVE",
"fmt ",
16, 0, 0, 0,
1, 0,
1, 0,
34, 86, 0, 0,   /* 86 * 256 + 34 = 22050 */
172, 68, 0, 0,  /* 172 * 256 + 68 = 44100 */
2, 0,
16, 0,
"data",
0, 0, 0, 0
};
/*---------------------------------------------------------*/
static void set_wave( unsigned char w[], int n, long x)
{
int i;
for(i=0;i<n;i++) {
  w[i] = (unsigned char)(x & 0xff);
  x >>= 8;
}
}
/*---------------------------------------------------------*/
int write_pcm_header_wave( int handout,
        long samprate, int channels, int bits, int type)
{
int nwrite;

     if( type == 0 )  set_wave(wave.tag, sizeof(wave.tag), 1);
else if( type == 10 ) set_wave(wave.tag, sizeof(wave.tag), 7);
else return 0;

set_wave(wave.size, sizeof(wave.size), sizeof(wave) - 8);
set_wave(wave.nChannels, sizeof(wave.nChannels), channels);
set_wave(wave.nSamplesPerSec, sizeof(wave.nSamplesPerSec), samprate);
set_wave(wave.nAvgBytesPerSec, sizeof(wave.nAvgBytesPerSec),
                                   (channels*samprate*bits + 7)/8);
set_wave(wave.nBlockAlign, sizeof(wave.nBlockAlign), (channels*bits+7)/8);
set_wave(wave.nBitsPerSample, sizeof(wave.nBitsPerSample), bits);
set_wave(wave.pcm_bytes, sizeof(wave.pcm_bytes), 0);

nwrite = write(handout, &wave, sizeof(wave));
if( nwrite != sizeof(wave) )  return 0;

return 1;
}
/*-----------------------------------------------*/
int write_pcm_tailer_wave( int handout, unsigned long pcm_bytes)
{
unsigned long pos;
int nwrite;


set_wave(wave.size, sizeof(wave.size), sizeof(wave) - 8 + pcm_bytes);
set_wave(wave.pcm_bytes, sizeof(wave.pcm_bytes), pcm_bytes);


pos = lseek(handout, 0L, 2);   /*-- save current position */
lseek(handout, 0L, 0);         /*-- pos to header --*/
nwrite = write(handout, &wave, sizeof(wave));
lseek(handout, pos, 0);        /*-- restore pos --*/

if( nwrite != sizeof(wave) )  return 0;
return 1;
}
/*-----------------------------------------------*/
/*----------------------------------------------------------------
  pcm conversion to wave format

  This conversion code required for big endian machine, or,
  if sizeof(short) != 16 bits.
  Conversion routines may be used on any machine, but if
  not required, the do nothing macros in port.h can be used instead
  to reduce overhead.

-----------------------------------------------------------------*/
#ifndef LITTLE_SHORT16
#include "wcvt.c"
#endif
/*-----------------------------------------------*/
int cvt_to_wave_test()
{
/*-- test for valid compile ---*/

  return sizeof(short) - 2;


}
/*-----------------------------------------------*/

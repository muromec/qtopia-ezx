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

#include "statname.h"


#ifdef __cplusplus
extern "C" {
#endif

void fdct32(float *, float *);
void fdct32_dual(float *, float *);
void fdct32_dual_mono(float *, float *);
void fdct16(float *, float *);
void fdct16_dual(float *, float *);
void fdct16_dual_mono(float *, float *);
void fdct8(float *, float *);
void fdct8_dual(float *, float *);
void fdct8_dual_mono(float *, float *);

void window(float *vbuf, int vb_ptr, unsigned char *pcm);
void window_dual(float *vbuf, int vb_ptr, unsigned char *pcm);
void window16(float *vbuf, int vb_ptr, unsigned char *pcm);
void window16_dual(float *vbuf, int vb_ptr, unsigned char *pcm);
void window8(float *vbuf, int vb_ptr, unsigned char *pcm);
void window8_dual(float *vbuf, int vb_ptr, unsigned char *pcm);

void windowB(float *vbuf, int vb_ptr, unsigned char *pcm);
void windowB_dual(float *vbuf, int vb_ptr, unsigned char *pcm);
void windowB16(float *vbuf, int vb_ptr, unsigned char *pcm);
void windowB16_dual(float *vbuf, int vb_ptr, unsigned char *pcm);
void windowB8(float *vbuf, int vb_ptr, unsigned char *pcm);
void windowB8_dual(float *vbuf, int vb_ptr, unsigned char *pcm);

void sbt_mono_L3(float *sample, unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr_arg[]);
void sbt_dual_L3(float *sample, unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr_arg[]);

#ifdef __cplusplus
}
#endif


/*============================================================*/
/*============ Layer III =====================================*/
/*============================================================*/
/*------------------------------------------------------------*/
/*------- short (usually 16 bit) bit output ------------------*/
/*------------------------------------------------------------*/
void sbt_mono_L3(float *sample, unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr_arg[])
{
int i;
int vb_ptr;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<18;i++) {
  fdct32(sample, vbuf[0]+vb_ptr);
  window(vbuf[0], vb_ptr, pcm);
  sample += 32;
  vb_ptr = (vb_ptr-32) & 511;
  pcm += 32*sizeof(short);
}
	
vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbt_dual_L3(float *sample, unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr_arg[])
{
int i;
int vb_ptr;

pcm += ch*sizeof(short);
vb_ptr = vb_ptr_arg[ch];
for(i=0;i<18;i++) {
  fdct32(sample, vbuf[ch]+vb_ptr);
  window_dual(vbuf[ch], vb_ptr, pcm);
  sample += 32;
  vb_ptr = (vb_ptr-32) & 511;
  pcm += 64*sizeof(short);
}


vb_ptr_arg[ch] = vb_ptr;
}

#ifdef REDUCTION
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*---------------- 16 pt sbt's  -------------------------------*/
/*------------------------------------------------------------*/
void sbt16_mono_L3(float *sample, unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr_arg[])
{
int i;
int vb_ptr;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<18;i++) {
  fdct16(sample, vbuf[0]+vb_ptr);
  window16(vbuf[0], vb_ptr, pcm);
  sample += 32;
  vb_ptr = (vb_ptr-16) & 255;
  pcm += 16*sizeof(short);
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbt16_dual_L3(float *sample, unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr_arg[])
{
int i;
int vb_ptr;

pcm += ch*sizeof(short);
vb_ptr = vb_ptr_arg[ch];
for(i=0;i<18;i++) {
  fdct16(sample, vbuf[ch]+vb_ptr);
  window16_dual(vbuf[ch], vb_ptr, pcm);
  sample += 32;
  vb_ptr = (vb_ptr-16) & 255;
  pcm += 32*sizeof(short);
}

vb_ptr_arg[ch] = vb_ptr;
}
/*------------------------------------------------------------*/
/*---------------- 8 pt sbt's  -------------------------------*/
/*------------------------------------------------------------*/
void sbt8_mono_L3(float *sample, unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr_arg[])
{
int i;
int vb_ptr;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<18;i++) {
  fdct8(sample, vbuf[0]+vb_ptr);
  window8(vbuf[0], vb_ptr, pcm);
  sample += 32;
  vb_ptr = (vb_ptr-8) & 127;
  pcm += 8*sizeof(short);
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbt8_dual_L3(float *sample, unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr_arg[])
{
int i;
int vb_ptr;

pcm += ch*sizeof(short);
vb_ptr = vb_ptr_arg[ch];
for(i=0;i<18;i++) {
  fdct8(sample, vbuf[ch]+vb_ptr);
  window8_dual(vbuf[ch], vb_ptr, pcm);
  sample += 32;
  vb_ptr = (vb_ptr-8) & 127;
  pcm += 16*sizeof(short);
}


vb_ptr_arg[ch] = vb_ptr;
}
#endif      // end reduction
/*===========================================================*/
/*===========================================================*/

#ifdef EIGHT_BIT
/*------------------------------------------------------------*/
/*------- 8 bit output ---------------------------------------*/
/*------------------------------------------------------------*/
void sbtB_mono_L3(float *sample, unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr_arg[])
{
int i;
int vb_ptr;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<18;i++) {
  fdct32(sample, vbuf[0]+vb_ptr);
  windowB(vbuf[0], vb_ptr, pcm);
  sample += 32;
  vb_ptr = (vb_ptr-32) & 511;
  pcm += 32;
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbtB_dual_L3(float *sample, unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr_arg[])
{
int i;
int vb_ptr;

pcm += ch;
vb_ptr = vb_ptr_arg[ch];
for(i=0;i<18;i++) {
  fdct32(sample, vbuf[ch]+vb_ptr);
  windowB_dual(vbuf[ch], vb_ptr, pcm);
  sample += 32;
  vb_ptr = (vb_ptr-32) & 511;
  pcm += 64;
}

vb_ptr_arg[ch] = vb_ptr;
}
#ifdef EIGHT_BIT
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*---------------- 16 pt sbtB's  -------------------------------*/
/*------------------------------------------------------------*/
void sbtB16_mono_L3(float *sample, unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr_arg[])
{
int i;
int vb_ptr;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<18;i++) {
  fdct16(sample, vbuf[0]+vb_ptr);
  windowB16(vbuf[0], vb_ptr, pcm);
  sample += 32;
  vb_ptr = (vb_ptr-16) & 255;
  pcm += 16;
}


vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbtB16_dual_L3(float *sample, unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr_arg[])
{
int i;
int vb_ptr;

pcm += ch;
vb_ptr = vb_ptr_arg[ch];
for(i=0;i<18;i++) {
  fdct16(sample, vbuf[ch]+vb_ptr);
  windowB16_dual(vbuf[ch], vb_ptr, pcm);
  sample += 32;
  vb_ptr = (vb_ptr-16) & 255;
  pcm += 32;
}

vb_ptr_arg[ch] = vb_ptr;
}
/*------------------------------------------------------------*/
/*---------------- 8 pt sbtB's  -------------------------------*/
/*------------------------------------------------------------*/
void sbtB8_mono_L3(float *sample, unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr_arg[])
{
int i;
int vb_ptr;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<18;i++) {
  fdct8(sample, vbuf[0]+vb_ptr);
  windowB8(vbuf[0], vb_ptr, pcm);
  sample += 32;
  vb_ptr = (vb_ptr-8) & 127;
  pcm += 8;
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbtB8_dual_L3(float *sample, unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr_arg[])
{
int i;
int vb_ptr;

pcm += ch;
vb_ptr = vb_ptr_arg[ch];
for(i=0;i<18;i++) {
  fdct8(sample, vbuf[ch]+vb_ptr);
  windowB8_dual(vbuf[ch], vb_ptr, pcm);
  sample += 32;
  vb_ptr = (vb_ptr-8) & 127;
  pcm += 16;
}

vb_ptr_arg[ch] = vb_ptr;
}
/*------------------------------------------------------------*/
#endif      // end reduction
#endif      // end 8 BIT

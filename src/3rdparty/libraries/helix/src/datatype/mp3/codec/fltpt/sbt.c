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

//#define REDUCTION
//#define EIGHT_BIT

#include "statname.h"



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


/*============================================================*/
/*============== Layer I/II ==================================*/
/*============================================================*/
void sbt_mono(float *sample, unsigned char *pcm, 
              int n, float vbuf[][512], int vb_ptr_arg[])
{
int i, vb_ptr;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct32(sample, vbuf[0]+vb_ptr);
  window(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-32) & 511;
  pcm += 32*sizeof(short);
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbt_dual(float *sample, unsigned char *pcm, 
              int n, float vbuf[][512], int vb_ptr_arg[])
{
int i, vb_ptr;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct32_dual(sample, vbuf[0]+vb_ptr);
  fdct32_dual(sample+1, vbuf[1]+vb_ptr);
  window_dual(vbuf[0], vb_ptr, pcm);
  window_dual(vbuf[1], vb_ptr, pcm+sizeof(short));
  sample += 64;
  vb_ptr = (vb_ptr-32) & 511;
  pcm += 64*sizeof(short);
}


vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
/* convert dual to mono */
void sbt_dual_mono(float *sample, unsigned char *pcm, 
                   int n, float vbuf[][512], int vb_ptr_arg[])
{
int i, vb_ptr;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct32_dual_mono(sample, vbuf[0]+vb_ptr);
  window(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-32) & 511;
  pcm += 32*sizeof(short);
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
#ifdef REDUCTION
/*------------------------------------------------------------*/
/* convert dual to left */
void sbt_dual_left(float *sample, unsigned char *pcm, 
                   int n, float vbuf[][512], int vb_ptr_arg[])
{
int i, vb_ptr;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct32_dual(sample, vbuf[0]+vb_ptr);
  window(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-32) & 511;
  pcm += 32*sizeof(short);
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
/* convert dual to right */
void sbt_dual_right(float *sample, unsigned char *pcm, 
                    int n, float vbuf[][512], int vb_ptr_arg[])
{
int i, vb_ptr;

vb_ptr = vb_ptr_arg[0];
sample++;   /* point to right chan */
for(i=0;i<n;i++) {
  fdct32_dual(sample, vbuf[0]+vb_ptr);
  window(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-32) & 511;
  pcm += 32*sizeof(short);
}
vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
/*---------------- 16 pt sbt's  -------------------------------*/
/*------------------------------------------------------------*/
void sbt16_mono(float *sample, unsigned char *pcm, 
                int n, float vbuf[][512], int vb_ptr_arg[])
{
int i, vb_ptr;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct16(sample, vbuf[0]+vb_ptr);
  window16(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-16) & 255;
  pcm += 16*sizeof(short);
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbt16_dual(float *sample, unsigned char *pcm, 
                int n, float vbuf[][512], int vb_ptr_arg[])
{
int i, vb_ptr;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct16_dual(sample, vbuf[0]+vb_ptr);
  fdct16_dual(sample+1, vbuf[1]+vb_ptr);
  window16_dual(vbuf[0], vb_ptr, pcm);
  window16_dual(vbuf[1], vb_ptr, pcm+sizeof(short));
  sample += 64;
  vb_ptr = (vb_ptr-16) & 255;
  pcm += 32*sizeof(short);
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbt16_dual_mono(float *sample, unsigned char *pcm, 
                     int n, float vbuf[][512], int vb_ptr_arg[])
{
int i, vb_ptr;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct16_dual_mono(sample, vbuf[0]+vb_ptr);
  window16(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-16) & 255;
  pcm += 16*sizeof(short);
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbt16_dual_left(float *sample, unsigned char *pcm, 
                     int n, float vbuf[][512], int vb_ptr_arg[])
{
int i, vb_ptr;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct16_dual(sample, vbuf[0]+vb_ptr);
  window16(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-16) & 255;
  pcm += 16*sizeof(short);
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbt16_dual_right(float *sample, unsigned char *pcm, 
                      int n, float vbuf[][512], int vb_ptr_arg[])
{
int i, vb_ptr;

vb_ptr = vb_ptr_arg[0];
sample++;
for(i=0;i<n;i++) {
  fdct16_dual(sample, vbuf[0]+vb_ptr);
  window16(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-16) & 255;
  pcm += 16*sizeof(short);
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
/*---------------- 8 pt sbt's  -------------------------------*/
/*------------------------------------------------------------*/
void sbt8_mono(float *sample, unsigned char *pcm, 
               int n, float vbuf[][512], int vb_ptr_arg[])
{
int i, vb_ptr;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct8(sample, vbuf[0]+vb_ptr);
  window8(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-8) & 127;
  pcm += 8*sizeof(short);
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbt8_dual(float *sample, unsigned char *pcm, 
               int n, float vbuf[][512], int vb_ptr_arg[])
{
int i, vb_ptr;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct8_dual(sample, vbuf[0]+vb_ptr);
  fdct8_dual(sample+1, vbuf[1]+vb_ptr);
  window8_dual(vbuf[0], vb_ptr, pcm);
  window8_dual(vbuf[1], vb_ptr, pcm+sizeof(short));
  sample += 64;
  vb_ptr = (vb_ptr-8) & 127;
  pcm += 16*sizeof(short);
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbt8_dual_mono(float *sample, unsigned char *pcm, 
                    int n, float vbuf[][512], int vb_ptr_arg[])
{
int i, vb_ptr;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct8_dual_mono(sample, vbuf[0]+vb_ptr);
  window8(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-8) & 127;
  pcm += 8*sizeof(short);
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbt8_dual_left(float *sample, unsigned char *pcm, 
                    int n, float vbuf[][512], int vb_ptr_arg[])
{
int i, vb_ptr;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct8_dual(sample, vbuf[0]+vb_ptr);
  window8(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-8) & 127;
  pcm += 8*sizeof(short);
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbt8_dual_right(float *sample, unsigned char *pcm, 
                     int n, float vbuf[][512], int vb_ptr_arg[])
{
int i, vb_ptr;

vb_ptr = vb_ptr_arg[0];
sample++;
for(i=0;i<n;i++) {
  fdct8_dual(sample, vbuf[0]+vb_ptr);
  window8(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-8) & 127;
  pcm += 8*sizeof(short);
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
// end reduction 16 bit
#endif
/*------------------------------------------------------------*/
#ifdef EIGHT_BIT
#include "csbtb.c"   /* 8 bit output */
#endif
/*------------------------------------------------------------*/

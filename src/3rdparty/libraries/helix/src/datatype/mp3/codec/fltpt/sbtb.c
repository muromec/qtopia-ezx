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

void windowB(float *vbuf, int vb_ptr, unsigned char *pcm);
void windowB_dual(float *vbuf, int vb_ptr, unsigned char *pcm);
void windowB16(float *vbuf, int vb_ptr, unsigned char *pcm);
void windowB16_dual(float *vbuf, int vb_ptr, unsigned char *pcm);
void windowB8(float *vbuf, int vb_ptr, unsigned char *pcm);
void windowB8_dual(float *vbuf, int vb_ptr, unsigned char *pcm);

/*============================================================*/
void sbtB_mono(float *sample, unsigned char *pcm, 
               int n, float vbuf[][512], int vb_ptr_arg[])
{
int i;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct32(sample, vbuf[0]+vb_ptr);
  windowB(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-32) & 511;
  pcm += 32;
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbtB_dual(float *sample, unsigned char *pcm, 
               int n, float vbuf[][512], int vb_ptr_arg[])
{
int i;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct32_dual(sample, vbuf[0]+vb_ptr);
  fdct32_dual(sample+1, vbuf[1]+vb_ptr);
  windowB_dual(vbuf[0], vb_ptr, pcm);
  windowB_dual(vbuf[1], vb_ptr, pcm+1);
  sample += 64;
  vb_ptr = (vb_ptr-32) & 511;
  pcm += 64;
}


vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
#ifdef REDUCTION
/*------------------------------------------------------------*/
/* convert dual to mono */
void sbtB_dual_mono(float *sample, unsigned char *pcm, 
                    int n, float vbuf[][512], int vb_ptr_arg[])
{
int i;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct32_dual_mono(sample, vbuf[0]+vb_ptr);
  windowB(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-32) & 511;
  pcm += 32;
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
/* convert dual to left */
void sbtB_dual_left(float *sample, unsigned char *pcm, 
                    int n, float vbuf[][512], int vb_ptr_arg[])
{
int i;
vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct32_dual(sample, vbuf[0]+vb_ptr);
  windowB(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-32) & 511;
  pcm += 32;
}
vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
/* convert dual to right */
void sbtB_dual_right(float *sample, unsigned char *pcm, 
                     int n, float vbuf[][512], int vb_ptr_arg[])
{
int i;

vb_ptr = vb_ptr_arg[0];
sample++;   /* point to right chan */
for(i=0;i<n;i++) {
  fdct32_dual(sample, vbuf[0]+vb_ptr);
  windowB(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-32) & 511;
  pcm += 32;
}
vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
/*---------------- 16 pt sbt's  -------------------------------*/
/*------------------------------------------------------------*/
void sbtB16_mono(float *sample, unsigned char *pcm, 
                 int n, float vbuf[][512], int vb_ptr_arg[])
{
int i;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct16(sample, vbuf[0]+vb_ptr);
  windowB16(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-16) & 255;
  pcm += 16;
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbtB16_dual(float *sample, unsigned char *pcm, 
                 int n, float vbuf[][512], int vb_ptr_arg[])
{
int i;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct16_dual(sample, vbuf[0]+vb_ptr);
  fdct16_dual(sample+1, vbuf[1]+vb_ptr);
  windowB16_dual(vbuf[0], vb_ptr, pcm);
  windowB16_dual(vbuf[1], vb_ptr, pcm+1);
  sample += 64;
  vb_ptr = (vb_ptr-16) & 255;
  pcm += 32;
}
vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbtB16_dual_mono(float *sample, unsigned char *pcm, 
                      int n, float vbuf[][512], int vb_ptr_arg[])
{
int i;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct16_dual_mono(sample, vbuf[0]+vb_ptr);
  windowB16(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-16) & 255;
  pcm += 16;
}
vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbtB16_dual_left(float *sample, unsigned char *pcm, 
                      int n, float vbuf[][512], int vb_ptr_arg[])
{
int i;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct16_dual(sample, vbuf[0]+vb_ptr);
  windowB16(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-16) & 255;
  pcm += 16;
}
vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbtB16_dual_right(float *sample, unsigned char *pcm, 
                       int n, float vbuf[][512], int vb_ptr_arg[])
{
int i;

vb_ptr = vb_ptr_arg[0];
sample++;
for(i=0;i<n;i++) {
  fdct16_dual(sample, vbuf[0]+vb_ptr);
  windowB16(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-16) & 255;
  pcm += 16;
}
vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
/*---------------- 8 pt sbt's  -------------------------------*/
/*------------------------------------------------------------*/
void sbtB8_mono(float *sample, unsigned char *pcm, 
                int n, float vbuf[][512], int vb_ptr_arg[])
{
int i;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct8(sample, vbuf[0]+vb_ptr);
  windowB8(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-8) & 127;
  pcm += 8;
}

vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbtB8_dual(float *sample, unsigned char *pcm, 
                int n, float vbuf[][512], int vb_ptr_arg[])
{
int i;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct8_dual(sample, vbuf[0]+vb_ptr);
  fdct8_dual(sample+1, vbuf[1]+vb_ptr);
  windowB8_dual(vbuf[0], vb_ptr, pcm);
  windowB8_dual(vbuf[1], vb_ptr, pcm+1);
  sample += 64;
  vb_ptr = (vb_ptr-8) & 127;
  pcm += 16;
}
vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbtB8_dual_mono(float *sample, unsigned char *pcm, 
                     int n, float vbuf[][512], int vb_ptr_arg[])
{
int i;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct8_dual_mono(sample, vbuf[0]+vb_ptr);
  windowB8(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-8) & 127;
  pcm += 8;
}
vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbtB8_dual_left(float *sample, unsigned char *pcm, 
                     int n, float vbuf[][512], int vb_ptr_arg[])
{
int i;

vb_ptr = vb_ptr_arg[0];
for(i=0;i<n;i++) {
  fdct8_dual(sample, vbuf[0]+vb_ptr);
  windowB8(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-8) & 127;
  pcm += 8;
}
vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
void sbtB8_dual_right(float *sample, unsigned char *pcm, 
                      int n, float vbuf[][512], int vb_ptr_arg[])
{
int i;

vb_ptr = vb_ptr_arg[0];
sample++;
for(i=0;i<n;i++) {
  fdct8_dual(sample, vbuf[0]+vb_ptr);
  windowB8(vbuf[0], vb_ptr, pcm);
  sample += 64;
  vb_ptr = (vb_ptr-8) & 127;
  pcm += 8;
}
vb_ptr_arg[0] = vb_ptr;
}
/*------------------------------------------------------------*/
// end reduction 8 bit
#endif

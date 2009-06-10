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

#ifdef __cplusplus
extern "C" {
#endif
void window(float *vbuf, int vb_ptr, short *pcm);
void window_dual(float *vbuf, int vb_ptr, short *pcm);
#ifdef __cplusplus
}
#endif

#ifdef _M_IX86	/* Asm versions */
__inline short
RoundFtoS(float f) {
	long l;
	__asm fld	f
	__asm fistp	l
	if (l < -32768)
		l = -32768;
	else if (l > 32767)
		l = 32767;
	return (short)l;
}
#else /* C versions */
__inline short
RoundFtoS(float f) {
	long l = (long)(f < 0.0f ? f - 0.5f : f + 0.5f);
	if (l < -32768)
		l = -32768;
	else if (l > 32767)
		l = 32767;
	return (short)l;
}
#endif /* _M_IX86 */

/*-------------------------------------------------------------------------*/
void window(float *vbuf, int vb_ptr, short *pcm)
{
int i, j;
int si, bx;
const float *coef;
float sum;

si = vb_ptr + 16;
bx = (si+32) & 511;
coef = wincoef;

/*-- first 16 --*/
for(i=0;i<16;i++) {
   sum = 0.0F;
   for(j=0;j<8;j++) {
      sum += (*coef++)*vbuf[si];
      si = (si+64) & 511;
      sum -= (*coef++)*vbuf[bx];
      bx = (bx+64) & 511;
   }
   si++;
   bx--;
   *pcm++ = RoundFtoS(sum) ;
}
/*--  special case --*/
sum = 0.0F;
for(j=0;j<8;j++) {
   sum += (*coef++)*vbuf[bx];
   bx = (bx+64) & 511;
}
*pcm++ = RoundFtoS(sum) ;
/*-- last 15 --*/
coef = wincoef + 255;    /* back pass through coefs */
for(i=0;i<15;i++) {
   si--;
   bx++;
   sum = 0.0F;
   for(j=0;j<8;j++) {
      sum += (*coef--)*vbuf[si];
      si = (si+64) & 511;
      sum += (*coef--)*vbuf[bx];
      bx = (bx+64) & 511;
   }
   *pcm++ = RoundFtoS(sum) ;
}
}
/*------------------------------------------------------------*/
void window_dual(float *vbuf, int vb_ptr, short *pcm)
{
int i, j;       /* dual window interleaves output */
int si, bx;
const float *coef;
float sum;

si = vb_ptr + 16;
bx = (si+32) & 511;
coef = wincoef;

/*-- first 16 --*/
for(i=0;i<16;i++) {
   sum = 0.0F;
   for(j=0;j<8;j++) {
      sum += (*coef++)*vbuf[si];
      si = (si+64) & 511;
      sum -= (*coef++)*vbuf[bx];
      bx = (bx+64) & 511;
   }
   si++;
   bx--;
   *pcm = RoundFtoS(sum) ;
   pcm += 2;
}
/*--  special case --*/
sum = 0.0F;
for(j=0;j<8;j++) {
   sum += (*coef++)*vbuf[bx];
   bx = (bx+64) & 511;
}
*pcm = RoundFtoS(sum) ;
pcm += 2;
/*-- last 15 --*/
coef = wincoef + 255;    /* back pass through coefs */
for(i=0;i<15;i++) {
   si--;
   bx++;
   sum = 0.0F;
   for(j=0;j<8;j++) {
      sum += (*coef--)*vbuf[si];
      si = (si+64) & 511;
      sum += (*coef--)*vbuf[bx];
      bx = (bx+64) & 511;
   }
   *pcm = RoundFtoS(sum) ;
   pcm += 2;
}
}
/*------------------------------------------------------------*/
#ifdef REDUCTION
/*------------------------------------------------------------*/
/*------------------- 16 pt window ------------------------------*/
void window16(float *vbuf, int vb_ptr, short *pcm)
{
int i, j;
unsigned char si, bx;
float *coef;
float sum;
long tmp;

si = vb_ptr + 8;
bx = si+16;
coef = wincoef;

/*-- first 8 --*/
for(i=0;i<8;i++) {
   sum = 0.0F;
   for(j=0;j<8;j++) {
      sum += (*coef++)*vbuf[si];
      si += 32;
      sum -= (*coef++)*vbuf[bx];
      bx += 32;
   }
   si++;
   bx--;
   coef+=16;
   *pcm++ = RoundFtoS(sum) ;
}
/*--  special case --*/
sum = 0.0F;
for(j=0;j<8;j++) {
   sum += (*coef++)*vbuf[bx];
   bx += 32;
}
*pcm++ = RoundFtoS(sum) ;
/*-- last 7 --*/
coef = wincoef + 255;    /* back pass through coefs */
for(i=0;i<7;i++) {
   coef-=16;
   si--;
   bx++;
   sum = 0.0F;
   for(j=0;j<8;j++) {
      sum += (*coef--)*vbuf[si];
      si +=32;
      sum += (*coef--)*vbuf[bx];
      bx +=32;
   }
   *pcm++ = RoundFtoS(sum) ;
}
}
/*--------------- 16 pt dual window (interleaved output) -----------------*/
void window16_dual(float *vbuf, int vb_ptr, short *pcm)
{
int i, j;
unsigned char si, bx;
float *coef;
float sum;
long tmp;

si = vb_ptr + 8;
bx = si+16;
coef = wincoef;

/*-- first 8 --*/
for(i=0;i<8;i++) {
   sum = 0.0F;
   for(j=0;j<8;j++) {
      sum += (*coef++)*vbuf[si];
      si += 32;
      sum -= (*coef++)*vbuf[bx];
      bx += 32;
   }
   si++;
   bx--;
   coef+=16;
   *pcm = RoundFtoS(sum) ;
   pcm+=2;
}
/*--  special case --*/
sum = 0.0F;
for(j=0;j<8;j++) {
   sum += (*coef++)*vbuf[bx];
   bx += 32;
}
*pcm = RoundFtoS(sum) ;
pcm+=2;
/*-- last 7 --*/
coef = wincoef + 255;    /* back pass through coefs */
for(i=0;i<7;i++) {
   coef-=16;
   si--;
   bx++;
   sum = 0.0F;
   for(j=0;j<8;j++) {
      sum += (*coef--)*vbuf[si];
      si +=32;
      sum += (*coef--)*vbuf[bx];
      bx +=32;
   }
   *pcm = RoundFtoS(sum) ;
   pcm+=2;
}
}
/*------------------- 8 pt window ------------------------------*/
void window8(float *vbuf, int vb_ptr, short *pcm)
{
int i, j;
int si, bx;
float *coef;
float sum;
long tmp;

si = vb_ptr + 4;
bx = (si+8)&127;
coef = wincoef;

/*-- first 4 --*/
for(i=0;i<4;i++) {
   sum = 0.0F;
   for(j=0;j<8;j++) {
      sum += (*coef++)*vbuf[si];
      si = (si+16) & 127;
      sum -= (*coef++)*vbuf[bx];
      bx = (bx+16) & 127;
   }
   si++;
   bx--;
   coef+=48;
   *pcm++ = RoundFtoS(sum) ;
}
/*--  special case --*/
sum = 0.0F;
for(j=0;j<8;j++) {
   sum += (*coef++)*vbuf[bx];
   bx = (bx+16) & 127;
}
*pcm++ = RoundFtoS(sum) ;
/*-- last 3 --*/
coef = wincoef + 255;    /* back pass through coefs */
for(i=0;i<3;i++) {
   coef-=48;
   si--;
   bx++;
   sum = 0.0F;
   for(j=0;j<8;j++) {
      sum += (*coef--)*vbuf[si];
      si = (si+16) & 127;
      sum += (*coef--)*vbuf[bx];
      bx = (bx+16) & 127;
   }
   *pcm++ = RoundFtoS(sum) ;
}
}
/*--------------- 8 pt dual window (interleaved output) -----------------*/
void window8_dual(float *vbuf, int vb_ptr, short *pcm)
{
int i, j;
int si, bx;
float *coef;
float sum;
long tmp;

si = vb_ptr + 4;
bx = (si+8) & 127;
coef = wincoef;

/*-- first 4 --*/
for(i=0;i<4;i++) {
   sum = 0.0F;
   for(j=0;j<8;j++) {
      sum += (*coef++)*vbuf[si];
      si = (si+16) & 127;
      sum -= (*coef++)*vbuf[bx];
      bx = (bx+16) & 127;
   }
   si++;
   bx--;
   coef+=48;
   *pcm = RoundFtoS(sum) ;
   pcm+=2;
}
/*--  special case --*/
sum = 0.0F;
for(j=0;j<8;j++) {
   sum += (*coef++)*vbuf[bx];
   bx = (bx+16) & 127;
}
*pcm = RoundFtoS(sum) ;
pcm+=2;
/*-- last 3 --*/
coef = wincoef + 255;    /* back pass through coefs */
for(i=0;i<3;i++) {
   coef-=48;
   si--;
   bx++;
   sum = 0.0F;
   for(j=0;j<8;j++) {
      sum += (*coef--)*vbuf[si];
      si = (si+16) & 127;
      sum += (*coef--)*vbuf[bx];
      bx = (bx+16) & 127;
   }
   *pcm = RoundFtoS(sum) ;
   pcm+=2;
}
}
/*------------------------------------------------------------*/
#endif    // end reduction

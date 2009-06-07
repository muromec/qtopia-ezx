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
void fdct32(float x[], float c[]);
#ifdef __cplusplus
}
#endif

/* JR - converted to ROM table */
extern const float coef32[31];            /* 32 pt dct coefs */

/* JR - made coef[] const */
static void forward_bf(int m, int n, float x[], float f[], const float coef[])
{
int i, j, n2;
int p, q, p0, k;

p0 = 0;
n2 = n >> 1;
for(i=0; i<m; i++, p0+=n) {
   k = 0;
   p = p0;
   q = p+n-1;
   for(j=0; j<n2; j++, p++, q--, k++) {
        f[p] = x[p]+x[q];
        f[n2+p] = coef[k]*(x[p]-x[q]);
        }
}
}
/*------------------------------------------------------------*/
static void back_bf(int m, int n, float x[], float f[])
{
int i, j, n2, n21;
int p, q, p0;

p0 = 0;
n2 = n >> 1;
n21 = n2-1;
for(i=0; i<m; i++, p0+=n) {
   p = p0;
   q = p0;
   for(j=0; j<n2; j++, p+=2, q++) f[p] = x[q];
   p = p0+1;
   for(j=0; j<n21; j++, p+=2, q++) f[p] = x[q] + x[q+1];
   f[p] = x[q];
}
}
/*------------------------------------------------------------*/
void fdct32(float x[], float c[])
{
float a[32];        /* ping pong buffers */
float b[32];
int p, q;

/* special first stage */
for(p=0, q=31; p<16; p++, q--) {
     a[p] = x[p]+x[q];
     a[16+p] = coef32[p]*(x[p]-x[q]);
     }
forward_bf(2, 16, a, b, coef32+16);
forward_bf(4,  8, b, a, coef32+16+8);
forward_bf(8,  4, a, b, coef32+16+8+4);
forward_bf(16, 2, b, a, coef32+16+8+4+2);
back_bf(8, 4, a, b);
back_bf(4, 8, b, a);
back_bf(2, 16, a, b);
back_bf(1, 32, b, c);
}
/*------------------------------------------------------------*/
void fdct32_dual(float x[], float c[])
{
float a[32];        /* ping pong buffers */
float b[32];
int p, pp, qq;

/* special first stage for dual chan (interleaved x) */
pp = 0;
qq = 2*31;
for(p=0; p<16; p++, pp+=2, qq-=2) {
     a[p] = x[pp]+x[qq];
     a[16+p] = coef32[p]*(x[pp]-x[qq]);
     }
forward_bf(2, 16, a, b, coef32+16);
forward_bf(4,  8, b, a, coef32+16+8);
forward_bf(8,  4, a, b, coef32+16+8+4);
forward_bf(16, 2, b, a, coef32+16+8+4+2);
back_bf(8, 4, a, b);
back_bf(4, 8, b, a);
back_bf(2, 16, a, b);
back_bf(1, 32, b, c);
}
/*---------------convert dual to mono------------------------------*/
void fdct32_dual_mono(float x[], float c[])
{
float a[32];        /* ping pong buffers */
float b[32];
float t1, t2;
int p, pp, qq;

/* special first stage  */
pp = 0;
qq = 2*31;
for(p=0; p<16; p++, pp+=2, qq-=2) {
     t1 = 0.5F*(x[pp]+x[pp+1]);
     t2 = 0.5F*(x[qq]+x[qq+1]);
     a[p] = t1+t2;
     a[16+p] = coef32[p]*(t1-t2);
     }
forward_bf(2, 16, a, b, coef32+16);
forward_bf(4,  8, b, a, coef32+16+8);
forward_bf(8,  4, a, b, coef32+16+8+4);
forward_bf(16, 2, b, a, coef32+16+8+4+2);
back_bf(8, 4, a, b);
back_bf(4, 8, b, a);
back_bf(2, 16, a, b);
back_bf(1, 32, b, c);
}
/*------------------------------------------------------------*/
#ifdef REDUCTION
/*------------------------------------------------------------*/
/*---------------- 16 pt fdct -------------------------------*/
void fdct16(float x[], float c[])
{
float a[16];        /* ping pong buffers */
float b[16];
int p, q;

/* special first stage (drop highest sb) */
a[0] = x[0];
a[8] = coef32[16]*x[0];
for(p=1, q=14; p<8; p++, q--) {
     a[p] = x[p]+x[q];
     a[8+p] = coef32[16+p]*(x[p]-x[q]);
     }
forward_bf(2,  8, a, b, coef32+16+8);
forward_bf(4,  4, b, a, coef32+16+8+4);
forward_bf(8,  2, a, b, coef32+16+8+4+2);
back_bf(4,  4, b, a);
back_bf(2,  8, a, b);
back_bf(1, 16, b, c);
}
/*------------------------------------------------------------*/
/*---------------- 16 pt fdct dual chan---------------------*/
void fdct16_dual(float x[], float c[])
{
float a[16];        /* ping pong buffers */
float b[16];
int p, pp, qq;

/* special first stage for interleaved input */
a[0] = x[0];
a[8] = coef32[16]*x[0];
pp = 2;
qq = 2*14;
for(p=1; p<8; p++, pp+=2, qq-=2) {
     a[p] = x[pp]+x[qq];
     a[8+p] = coef32[16+p]*(x[pp]-x[qq]);
     }
forward_bf(2,  8, a, b, coef32+16+8);
forward_bf(4,  4, b, a, coef32+16+8+4);
forward_bf(8,  2, a, b, coef32+16+8+4+2);
back_bf(4,  4, b, a);
back_bf(2,  8, a, b);
back_bf(1, 16, b, c);
}
/*------------------------------------------------------------*/
/*---------------- 16 pt fdct dual to mono-------------------*/
void fdct16_dual_mono(float x[], float c[])
{
float a[16];        /* ping pong buffers */
float b[16];
float t1, t2;
int p, pp, qq;

/* special first stage  */
a[0] = 0.5F*(x[0]+x[1]);
a[8] = coef32[16]*a[0];
pp = 2;
qq = 2*14;
for(p=1; p<8; p++, pp+=2, qq-=2) {
     t1 = 0.5F*(x[pp]+x[pp+1]);
     t2 = 0.5F*(x[qq]+x[qq+1]);
     a[p] = t1 + t2;
     a[8+p] = coef32[16+p]*(t1-t2);
     }
forward_bf(2,  8, a, b, coef32+16+8);
forward_bf(4,  4, b, a, coef32+16+8+4);
forward_bf(8,  2, a, b, coef32+16+8+4+2);
back_bf(4,  4, b, a);
back_bf(2,  8, a, b);
back_bf(1, 16, b, c);
}
/*------------------------------------------------------------*/
/*---------------- 8 pt fdct -------------------------------*/
void fdct8(float x[], float c[])
{
float a[8];        /* ping pong buffers */
float b[8];
int p, q;

/* special first stage  */

b[0] = x[0]+x[7];
b[4] = coef32[16+8]*(x[0]-x[7]);
for(p=1, q=6; p<4; p++, q--) {
     b[p] = x[p]+x[q];
     b[4+p] = coef32[16+8+p]*(x[p]-x[q]);
     }

forward_bf(2,  4, b, a, coef32+16+8+4);
forward_bf(4,  2, a, b, coef32+16+8+4+2);
back_bf(2,  4, b, a);
back_bf(1,  8, a, c);
}
/*------------------------------------------------------------*/
/*---------------- 8 pt fdct dual chan---------------------*/
void fdct8_dual(float x[], float c[])
{
float a[8];        /* ping pong buffers */
float b[8];
int p, pp, qq;

/* special first stage for interleaved input */
b[0] = x[0]+x[14];
b[4] = coef32[16+8]*(x[0]-x[14]);
pp = 2;
qq = 2*6;
for(p=1; p<4; p++, pp+=2, qq-=2) {
     b[p] = x[pp]+x[qq];
     b[4+p] = coef32[16+8+p]*(x[pp]-x[qq]);
     }
forward_bf(2,  4, b, a, coef32+16+8+4);
forward_bf(4,  2, a, b, coef32+16+8+4+2);
back_bf(2,  4, b, a);
back_bf(1,  8, a, c);
}
/*------------------------------------------------------------*/
/*---------------- 8 pt fdct dual to mono---------------------*/
void fdct8_dual_mono(float x[], float c[])
{
float a[8];        /* ping pong buffers */
float b[8];
float t1, t2;
int p, pp, qq;

/* special first stage  */
t1 = 0.5F*(x[0]+x[1]);
t2 = 0.5F*(x[14]+x[15]);
b[0] = t1+t2;
b[4] = coef32[16+8]*(t1-t2);
pp = 2;
qq = 2*6;
for(p=1; p<4; p++, pp+=2, qq-=2) {
     t1 = 0.5F*(x[pp]+x[pp+1]);
     t2 = 0.5F*(x[qq]+x[qq+1]);
     b[p] = t1 + t2;
     b[4+p] = coef32[16+8+p]*(t1 - t2);
     }
forward_bf(2,  4, b, a, coef32+16+8+4);
forward_bf(4,  2, a, b, coef32+16+8+4+2);
back_bf(2,  4, b, a);
back_bf(1,  8, a, c);
}
/*------------------------------------------------------------*/
#endif      // end conditional reduction

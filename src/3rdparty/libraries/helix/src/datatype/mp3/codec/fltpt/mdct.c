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

/****  mdct.c  ***************************************************

Layer III 

  cos transform for n=18, n=6

computes  c[k] =  Sum( cos((pi/4*n)*(2*k+1)*(2*p+1))*f[p] )
                k = 0, ...n-1,  p = 0...n-1


inplace ok.

******************************************************************/


/*------ 18 point xform -------

n = 18;
pi = 4.0*atan(1.0);
t = pi/(4*n);
for(p=0;p<n;p++) w[p] =  (float)(2.0*cos(t*(2*p+1)));
for(p=0;p<9;p++) w2[p] = (float)2.0*cos(2*t*(2*p+1));
t = pi/(2*n);
for(k=0;k<9;k++) {
    for(p=0;p<4;p++) 
		coef[k][p] = (float)cos(t*(2*k)*(2*p+1));
}
*/

/* JR - made into ROM table */
static const float w[18] = {
	 1.9980964661e+000f,  1.9828897715e+000f,  1.9525920153e+000f,  1.9074338675e+000f, 
	 1.8477590084e+000f,  1.7740216255e+000f,  1.6867828369e+000f,  1.5867066383e+000f, 
	 1.4745546579e+000f,  1.3511804342e+000f,  1.2175228596e+000f,  1.0745992661e+000f, 
	 9.2349720001e-001f,  7.6536685228e-001f,  6.0141158104e-001f,  4.3287923932e-001f, 
	 2.6105237007e-001f,  8.7238773704e-002f, 
};

/* JR - made into ROM table */
static const float w2[9] = {
	 1.9923894405e+000f,  1.9318516254e+000f,  1.8126156330e+000f,  1.6383041143e+000f, 
	 1.4142135382e+000f,  1.1471529007e+000f,  8.4523653984e-001f,  5.1763808727e-001f, 
	 1.7431148887e-001f, 
};

/* JR - made into ROM table */
static const float coef[9][4] = {
	 {1.0000000000e+000f,  1.0000000000e+000f,  1.0000000000e+000f,  1.0000000000e+000f}, 
	 {9.8480772972e-001f,  8.6602538824e-001f,  6.4278763533e-001f,  3.4202015400e-001f}, 
	 {9.3969261646e-001f,  5.0000000000e-001f, -1.7364817858e-001f, -7.6604443789e-001f}, 
	 {8.6602538824e-001f,  6.1230317691e-017f, -8.6602538824e-001f, -8.6602538824e-001f}, 
	 {7.6604443789e-001f, -5.0000000000e-001f, -9.3969261646e-001f,  1.7364817858e-001f}, 
	 {6.4278763533e-001f, -8.6602538824e-001f, -3.4202015400e-001f,  9.8480772972e-001f}, 
	 {5.0000000000e-001f, -1.0000000000e+000f,  5.0000000000e-001f,  5.0000000000e-001f}, 
	 {3.4202015400e-001f, -8.6602538824e-001f,  9.8480772972e-001f, -6.4278763533e-001f}, 
	 {1.7364817858e-001f, -5.0000000000e-001f,  7.6604443789e-001f, -9.3969261646e-001f}, 
};

/*--- 6 point transform

v  = addr->w;
v2 = addr->w2;
coef87 = (float*) addr->coef;

n = 6;
pi = 4.0*atan(1.0);
t = pi/(4*n);
for(p=0;p<n;p++) 
	v[p] =  (float)2.0*cos(t*(2*p+1));
for(p=0;p<3;p++) 
	v2[p] = (float)2.0*cos(2*t*(2*p+1));
t = pi/(2*n);
k = 1;
p = 0;
*coef87 = (float)cos(t*(2*k)*(2*p+1));

for(p=0;p<6;p++) 
	v[p] = v[p]/2.0f;
*coef87 = (float)2.0*(*coef87);
*/

/* JR - made into ROM table */
static const float v[6] = {
	 9.9144488573e-001f,  9.2387950420e-001f,  7.9335331917e-001f,  6.0876142979e-001f, 
	 3.8268342614e-001f,  1.3052618504e-001f, 
};

/* JR - made into ROM table */
static const float v2[3] = {
	 1.9318516254e+000f,  1.4142135382e+000f,  5.1763808727e-001f, 
};

/* JR - made into ROM table */
static const float coef87 = 1.7320507765e+000f;

/*--------------------------------------------------------------------*/
void imdct18(float f[18])  /* 18 point */
{
int p;
float a[9], b[9];
float ap, bp, a8p, b8p;
float g1, g2;


for(p=0;p<4;p++) {
    g1 = w[p]*f[p];
    g2 = w[17-p]*f[17-p];
    ap  =          g1 + g2;     // a[p]
    bp  =   w2[p]*(g1 - g2);    // b[p]
    g1 = w[8-p]*f[8-p];
    g2 = w[9+p]*f[9+p];
    a8p =          g1 + g2;    // a[8-p]
    b8p = w2[8-p]*(g1 - g2);   // b[8-p]
    a[p]   = ap + a8p;
    a[5+p] = ap - a8p;
    b[p]   = bp + b8p;
    b[5+p] = bp - b8p;
}
g1 = w[p]*f[p];
g2 = w[17-p]*f[17-p];
a[p] = g1 + g2;
b[p] = w2[p]*(g1 - g2);


f[0] = 0.5f*(a[0] + a[1] + a[2] + a[3] + a[4]);
f[1] = 0.5f*(b[0] + b[1] + b[2] + b[3] + b[4]);

f[2] = coef[1][0]*a[5] + coef[1][1]*a[6] + coef[1][2]*a[7]
        + coef[1][3]*a[8];
f[3] = coef[1][0]*b[5] + coef[1][1]*b[6] + coef[1][2]*b[7]
        + coef[1][3]*b[8]                   - f[1];
f[1] = f[1] - f[0];
f[2] = f[2] - f[1];

f[4] = coef[2][0]*a[0] + coef[2][1]*a[1] + coef[2][2]*a[2]
        + coef[2][3]*a[3] - a[4];
f[5] = coef[2][0]*b[0] + coef[2][1]*b[1] + coef[2][2]*b[2]
        + coef[2][3]*b[3] - b[4]            - f[3];
f[3] = f[3] - f[2];
f[4] = f[4] - f[3];

f[6] = coef[3][0]*( a[5] - a[7] - a[8]);
f[7] = coef[3][0]*( b[5] - b[7] - b[8])     - f[5];
f[5] = f[5] - f[4];
f[6] = f[6] - f[5];

f[8] = coef[4][0]*a[0] + coef[4][1]*a[1] + coef[4][2]*a[2]
        + coef[4][3]*a[3] + a[4];
f[9] = coef[4][0]*b[0] + coef[4][1]*b[1] + coef[4][2]*b[2]
        + coef[4][3]*b[3] + b[4]            - f[7];
f[7] = f[7] - f[6];
f[8] = f[8] - f[7];

f[10]= coef[5][0]*a[5] + coef[5][1]*a[6] + coef[5][2]*a[7]
        + coef[5][3]*a[8];
f[11]= coef[5][0]*b[5] + coef[5][1]*b[6] + coef[5][2]*b[7]
        + coef[5][3]*b[8]                    - f[9];
f[9]  = f[9]  - f[8];
f[10] = f[10] - f[9];

f[12]= 0.5f*(a[0] + a[2] + a[3]) - a[1] - a[4];
f[13]= 0.5f*(b[0] + b[2] + b[3]) - b[1] - b[4] - f[11];
f[11] = f[11] - f[10];
f[12] = f[12] - f[11];

f[14]= coef[7][0]*a[5] + coef[7][1]*a[6] + coef[7][2]*a[7]
        + coef[7][3]*a[8];
f[15]= coef[7][0]*b[5] + coef[7][1]*b[6] + coef[7][2]*b[7]
        + coef[7][3]*b[8]                    - f[13];
f[13] = f[13] - f[12];
f[14] = f[14] - f[13];

f[16]= coef[8][0]*a[0] + coef[8][1]*a[1] + coef[8][2]*a[2]
        + coef[8][3]*a[3] + a[4];
f[17]= coef[8][0]*b[0] + coef[8][1]*b[1] + coef[8][2]*b[2]
        + coef[8][3]*b[3] + b[4]             - f[15];
f[15] = f[15] - f[14];
f[16] = f[16] - f[15];
f[17] = f[17] - f[16];


return;
}
/*--------------------------------------------------------------------*/
/* does 3, 6 pt dct.  changes order from f[i][window] c[window][i] */
void imdct6_3(float f[])  /* 6 point */
{
int w;
float buf[18];
float *a, *c;       // b[i] = a[3+i]
float g1, g2;
float a02, b02;

c = f;
a = buf;
for(w=0;w<3;w++) {
    g1 = v[0]*f[3*0];
    g2 = v[5]*f[3*5];
    a[0] = g1 + g2;
    a[3+0] = v2[0]*(g1 - g2);

    g1 = v[1]*f[3*1];
    g2 = v[4]*f[3*4];
    a[1] = g1 + g2;
    a[3+1] = v2[1]*(g1 - g2);

    g1 = v[2]*f[3*2];
    g2 = v[3]*f[3*3];
    a[2] = g1 + g2;
    a[3+2] = v2[2]*(g1 - g2);

    a+=6;
    f++;
}

a = buf;
for(w=0;w<3;w++) {
    a02 = (a[0] + a[2]);
    b02 = (a[3+0] + a[3+2]);
    c[0] = a02 + a[1];
    c[1] = b02 + a[3+1];
    c[2] = coef87*(a[0] - a[2]);
    c[3] = coef87*(a[3+0] - a[3+2]) - c[1];
    c[1] = c[1] - c[0];
    c[2] = c[2] - c[1];
    c[4] = a02 - a[1] - a[1];
    c[5] = b02 - a[3+1] - a[3+1]    - c[3];
    c[3] = c[3] - c[2];
    c[4] = c[4] - c[3];
    c[5] = c[5] - c[4];
    a+=6;
    c+=6;
}

return;
}
/*--------------------------------------------------------------------*/

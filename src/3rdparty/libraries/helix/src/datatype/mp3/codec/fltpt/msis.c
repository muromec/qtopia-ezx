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

#include "hlxclib/string.h"
#include "statname.h"

/****  msis.c  ***************************************************
  Layer III  
 antialias, ms and is stereo precessing

**** is_process assumes never switch 
      from short to long in is region *****
     
is_process does ms or stereo in "forbidded sf regions"
    //ms_mode = 0 
    lr[0][i][0] = 1.0f;
    lr[0][i][1] = 0.0f;
    // ms_mode = 1, in is bands is routine does ms processing 
    lr[1][i][0] = 1.0f;
    lr[1][i][1] = 1.0f;

******************************************************************/

#include "l3.h"

#ifdef __cplusplus
extern "C" {
#endif
void antialias(float x[], int n);
void ms_process(float x[][1152], int n);
void is_process_MPEG1(float x[][1152],
                SCALEFACT *sf, 
                CB_INFO cb_info[2],
                int nsamp, int ms_mode, int nBand[2][22], int sfBandIndex[2][22]);
void is_process_MPEG2(float x[][1152],
                SCALEFACT *sf, 
                CB_INFO cb_info[2],
                IS_SF_INFO *is_sf_info,
                int nsamp, int ms_mode, int nBand[2][22], int sfBandIndex[2][22]);  
#ifdef __cplusplus
}
#endif

typedef float ARRAY2[2];
typedef float ARRAY8_2[8][2];
typedef float ARRAY2_64_2[2][64][2];
typedef float ARRAY64_2[64][2];

extern const float lr[2][8][2];
extern const float lr2[2][2][64][2];

/* antialias 

for(i=0;i<8;i++) {
    csa[i][0] = (float)(1.0/sqrt(1.0 + Ci[i]*Ci[i]));
    csa[i][1] = (float)(Ci[i]/sqrt(1.0 + Ci[i]*Ci[i]));
}
*/
static const float csa[8][2] = {
	{8.5749292374e-001f, -5.1449579000e-001f},  {8.8174200058e-001f, -4.7173199058e-001f}, 
	{9.4962865114e-001f, -3.1337746978e-001f},  {9.8331457376e-001f, -1.8191319704e-001f}, 
	{9.9551779032e-001f, -9.4574190676e-002f},  {9.9916058779e-001f, -4.0965583175e-002f}, 
	{9.9989920855e-001f, -1.4198568650e-002f},  {9.9999314547e-001f, -3.6999746226e-003f}, 
};

/*===============================================================*/
void antialias(float x[], int n)
{
int i, k;
float a, b;

for(k=0;k<n;k++) {
    for(i=0;i<8;i++) {
        a = x[17-i];
        b = x[18+i];
        x[17-i] = a*csa[i][0] - b*csa[i][1];
        x[18+i] = b*csa[i][0] + a*csa[i][1];
    }
    x+=18;
}

}
/*===============================================================*/
void ms_process(float x[][1152], int n)  /* sum-difference stereo */
{
int i;
float xl, xr;
/*-- note: sqrt(2) done scaling by dequant ---*/
for(i=0;i<n;i++) {
    xl = x[0][i] + x[1][i];
    xr = x[0][i] - x[1][i];
    x[0][i] = xl;
    x[1][i] = xr;
}
return;
}
/*===============================================================*/
void is_process_MPEG1(float x[][1152],    /* intensity stereo */
                SCALEFACT *sf, 
                CB_INFO cb_info[2],         /* [ch] */
                int nsamp, int ms_mode, int nBand[2][22], int sfBandIndex[2][22])  
{
int i, j, n, cb, w;
float fl, fr;
int m;
int isf;
float fls[3], frs[3];
int cb0;


cb0  = cb_info[1].cbmax;  /* start at end of right */
i    = sfBandIndex[cb_info[1].cbtype][cb0];
cb0++;
m    = nsamp - i;    /* process to len of left */

if( cb_info[1].cbtype ) goto short_blocks;
/*------------------------*/
/* long_blocks: */
for(cb=cb0;cb<21;cb++) {
    isf = sf->l[cb];
    n = nBand[0][cb];
    fl = lr[ms_mode][isf][0];
    fr = lr[ms_mode][isf][1];
    for(j=0;j<n;j++,i++) {
        if( --m < 0 ) goto exit;
        x[1][i] = fr*x[0][i];
        x[0][i] = fl*x[0][i];
    }
}
return;
/*------------------------*/
short_blocks:
for(cb=cb0;cb<12;cb++) {
    for(w=0;w<3;w++) {
        isf = sf->s[w][cb];
        fls[w] = lr[ms_mode][isf][0];
        frs[w] = lr[ms_mode][isf][1];
    }
    n = nBand[1][cb];
    for(j=0;j<n;j++) {
        m-=3;
        if( m < 0 ) goto exit;
        x[1][i]   = frs[0]*x[0][i];
        x[0][i]   = fls[0]*x[0][i];
        x[1][1+i] = frs[1]*x[0][1+i];
        x[0][1+i] = fls[1]*x[0][1+i];
        x[1][2+i] = frs[2]*x[0][2+i];
        x[0][2+i] = fls[2]*x[0][2+i];
        i+=3;
    }
}

exit:
return;
}
/*===============================================================*/
void is_process_MPEG2(float x[][1152],    /* intensity stereo */
                SCALEFACT *sf, 
                CB_INFO cb_info[2],     /* [ch] */
                IS_SF_INFO *is_sf_info,
                int nsamp, int ms_mode, int nBand[2][22], int sfBandIndex[2][22])  
{
int i, j, k, n, cb, w;
float fl, fr;
int m;
int isf;
int il[21];
int tmp;
int r;
const ARRAY2 *lr;
int cb0, cb1;

memset((void*)il, 0, sizeof(il));

lr = lr2[is_sf_info->intensity_scale][ms_mode];

if( cb_info[1].cbtype ) goto short_blocks;

/*------------------------*/
/* long_blocks: */
cb0  = cb_info[1].cbmax;  /* start at end of right */
i    = sfBandIndex[0][cb0];
m    = nsamp - i;    /* process to len of left */
/* gen sf info */
for(k=r=0;r<3;r++) {
    tmp = (1 << is_sf_info->slen[r]) -1;
    for(j=0;j<is_sf_info->nr[r];j++, k++) il[k] = tmp;
}
for(cb=cb0+1; cb<21;cb++) {
    isf = il[cb] + sf->l[cb];
    fl = lr[isf][0];
    fr = lr[isf][1];
    n = nBand[0][cb];
    for(j=0;j<n;j++,i++) {
        if( --m < 0 ) goto exit;
        x[1][i] = fr*x[0][i];
        x[0][i] = fl*x[0][i];
    }
}
return;
/*------------------------*/
short_blocks:

for(k=r=0;r<3;r++) {
    tmp = (1 << is_sf_info->slen[r]) -1;
    for(j=0;j<is_sf_info->nr[r];j++, k++) il[k] = tmp;
}

for(w=0;w<3;w++) {
cb0  = cb_info[1].cbmax_s[w];  /* start at end of right */
i    = sfBandIndex[1][cb0] + w;
cb1  = cb_info[0].cbmax_s[w];  /* process to end of left */

for(cb=cb0+1;cb<=cb1;cb++) {
    isf = il[cb] + sf->s[w][cb];
    fl = lr[isf][0];
    fr = lr[isf][1];
    n = nBand[1][cb];
    for(j=0;j<n;j++) {
        x[1][i]   = fr*x[0][i];
        x[0][i]   = fl*x[0][i];
        i+=3;
    }
}

}

exit:
return;
}
/*===============================================================*/

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

#ifndef _CARBON
#include "hlxclib/math.h"
#endif
#include "hlxclib/string.h"
#include "l3.h"

#ifdef __cplusplus
extern "C" {
#endif
void dequant(SAMPLE Sample[], int *nsamp, 
             SCALEFACT *sf, 
             GR *gr,
             CB_INFO *cb_info, int ncbl_mixed, int nBand[2][22], float re_buf[192][3]);
#ifdef __cplusplus
}
#endif

/* JR - added undefs */
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#define min(a,b)  ((((a)>=(b))?(b):(a))) 
#define max(a,b)  ((((a)>=(b))?(a):(b))) 

/*----------
static struct  {
int l[23];
int s[14];} sfBandTable[3] =   
{{{0,4,8,12,16,20,24,30,36,44,52,62,74,90,110,134,162,196,238,288,342,418,576},
 {0,4,8,12,16,22,30,40,52,66,84,106,136,192}},
{{0,4,8,12,16,20,24,30,36,42,50,60,72,88,106,128,156,190,230,276,330,384,576},
 {0,4,8,12,16,22,28,38,50,64,80,100,126,192}},
{{0,4,8,12,16,20,24,30,36,44,54,66,82,102,126,156,194,240,296,364,448,550,576},
 {0,4,8,12,16,22,30,42,58,78,104,138,180,192}}};
----------*/

/*--------------------------------*/
static const int pretab[2][22] = {
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,3,3,3,2,0},
};

extern const float look_global[256+2+4];	/* JR - made into ROM table */
extern const float look_scale[2][4][32];	/* JR - made into ROM table */

#define ISMAX 32
extern const float look_pow[2*ISMAX];  
extern const float look_subblock[8];

/*-- reorder buffer ---*/
typedef float ARRAY3[3];

/*=============================================================*/
void dequant(SAMPLE Sample[], int *nsamp, 
             SCALEFACT *sf, 
             GR *gr,
             CB_INFO *cb_info, int ncbl_mixed, int nBand[2][22], float re_buf[192][3])
{
int i, j;
int cb, n, w;
float x0, xs;
float xsb[3];
double tmp;
int ncbl;
int cbs0;
ARRAY3 *buf;    /* short block reorder */
int nbands;
int i0;
int non_zero;
int cbmax[3];
int p;

nbands = *nsamp;


ncbl = 22;      /* long block cb end */
cbs0 = 12;      /* short block cb start */
/* ncbl_mixed = 8 or 6  mpeg1 or 2 */
if( gr->block_type == 2 ) {
    ncbl = 0; cbs0 = 0;
    if( gr->mixed_block_flag ) { ncbl = ncbl_mixed; cbs0 = 3; }
}
/* fill in cb_info --*/
cb_info->lb_type = gr->block_type;
if( gr->block_type == 2 ) cb_info->lb_type;
cb_info->cbs0 = cbs0;
cb_info->ncbl = ncbl;

cbmax[2] = cbmax[1] = cbmax[0] = 0;
/* global gain pre-adjusted by 2 if ms_mode, 0 otherwise */
// mod 7/23/99  min max on global gain lookup
//     may use global gain for volume control
p = max(0, (2+4)+gr->global_gain);
p = min(p, 255+(2+4) );
x0 = look_global[p];
i=0;
/*----- long blocks ---*/
for(cb=0;cb<ncbl;cb++) {
    non_zero = 0;
    xs = x0*look_scale[gr->scalefac_scale][pretab[gr->preflag][cb]][sf->l[cb]];
    n = nBand[0][cb];
    for(j=0;j<n;j++,i++) {
        if( Sample[i].s == 0 ) Sample[i].x = 0.0F;
        else {
            non_zero = 1;
            if( (Sample[i].s >= (-ISMAX) ) && (Sample[i].s < ISMAX) ) 
                    Sample[i].x = xs*look_pow[ISMAX+Sample[i].s];
            else {
                tmp = (double)Sample[i].s;
                Sample[i].x = (float)(xs*tmp*pow(fabs(tmp),(1.0/3.0)));
            }
        }
    }
    if( non_zero ) cbmax[0] = cb;
    if( i >= nbands ) break;
}

cb_info->cbmax  = cbmax[0];
cb_info->cbtype = 0;     // type = long
if( cbs0 >= 12 ) return;
/*---------------------------
block type = 2  short blocks
----------------------------*/
cbmax[2] = cbmax[1] = cbmax[0] = cbs0;
i0 = i;     /* save for reorder */
buf = re_buf;
for(w=0;w<3;w++) xsb[w] = x0*look_subblock[gr->subblock_gain[w]];
for(cb=cbs0;cb<13;cb++) {
    n = nBand[1][cb];
    for(w=0;w<3;w++) {
        non_zero =  0;
        xs = xsb[w]*look_scale[gr->scalefac_scale][0][sf->s[w][cb]];
        for(j=0;j<n;j++,i++) {
            if( Sample[i].s == 0 ) buf[j][w] = 0.0F;
            else {
                non_zero = 1;
                if( (Sample[i].s >= (-ISMAX) ) && (Sample[i].s < ISMAX) ) 
                    buf[j][w] = xs*look_pow[ISMAX+Sample[i].s];
                else {
                    tmp = (double)Sample[i].s;
                    buf[j][w] = (float)(xs*tmp*pow(fabs(tmp),(1.0/3.0)));
                }
            }
        }
        if( non_zero ) cbmax[w] = cb;
    }
    if( i >= nbands ) break;
    buf += n;
}


memmove(&Sample[i0].x, &re_buf[0][0], sizeof(float)*(i-i0));

*nsamp = i;   /* update nsamp */
cb_info->cbmax_s[0] = cbmax[0];
cb_info->cbmax_s[1] = cbmax[1];
cb_info->cbmax_s[2] = cbmax[2];
if( cbmax[1] > cbmax[0] ) cbmax[0] = cbmax[1];
if( cbmax[2] > cbmax[0] ) cbmax[0] = cbmax[2];

cb_info->cbmax  = cbmax[0];
cb_info->cbtype = 1;     /* type = short */


return;
}
/*-------------------------------------------------------------*/






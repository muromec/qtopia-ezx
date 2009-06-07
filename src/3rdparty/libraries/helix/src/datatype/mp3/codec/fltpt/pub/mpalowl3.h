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

#ifndef _MPALOWL3_H_
#define _MPALOWL3_H_
/*****************************************************************
low level decoder functions Layer III

******************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#include "bitget.h"

// sbtL3.c
void sbt_mono_L3(float *sample,   unsigned char  *pcm, int ch, float vbuf[][512], int vb_ptr[]);
void sbt_dual_L3(float *sample,   unsigned char  *pcm, int ch, float vbuf[][512], int vb_ptr[]);
void sbt16_mono_L3(float *sample, unsigned char  *pcm, int ch, float vbuf[][512], int vb_ptr[]);
void sbt16_dual_L3(float *sample, unsigned char  *pcm, int ch, float vbuf[][512], int vb_ptr[]);
void sbt8_mono_L3(float *sample,  unsigned char  *pcm, int ch, float vbuf[][512], int vb_ptr[]);
void sbt8_dual_L3(float *sample,  unsigned char  *pcm, int ch, float vbuf[][512], int vb_ptr[]);

void sbtB_mono_L3(float *sample,   unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr[]);
void sbtB_dual_L3(float *sample,   unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr[]);
void sbtB16_mono_L3(float *sample, unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr[]);
void sbtB16_dual_L3(float *sample, unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr[]);
void sbtB8_mono_L3(float *sample,  unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr[]);
void sbtB8_dual_L3(float *sample,  unsigned char *pcm, int ch, float vbuf[][512], int vb_ptr[]);

// hwin.c
int hybrid(void *xin, void *xprev, float *y,
            int btype, int nlong, int ntot, int nprev, int band_limit_nsb);
int hybrid_sum(void *xin, void *xin_left, float *y,
            int btype, int nlong, int ntot);
void sum_f_bands( void *a, void *b, int n);
void FreqInvert(float *y, int n);

// msis.c
void antialias(void *x, int n);
void ms_process(void *x, int n);  /* sum-difference stereo */
void is_process_MPEG1(void *x,    /* intensity stereo */
                SCALEFACT *sf, 
                CB_INFO cb_info[2],         /* [ch] */
                int nsamp, int ms_mode, int nBand[2][22], int sfBandIndex[2][22]);
void is_process_MPEG2(void *x,    /* intensity stereo */
                SCALEFACT *sf, 
                CB_INFO cb_info[2],     /* [ch] */
                IS_SF_INFO *is_sf_info,
                int nsamp, int ms_mode, int nBand[2][22], int sfBandIndex[2][22]);

// uph.c
void unpack_huff(void *xy, int n, int ntable, BITDAT *bitdat);
int unpack_huff_quad(void *vwxy, int n, int nbits, int ntable, BITDAT *bitdat);

// L3dq.c
void dequant(SAMPLE sample[], int *nsamp, 
             SCALEFACT *sf, 
             GR *gr,
             CB_INFO *cb_info, int ncbl_mixed, int nBand[2][22], float re_buf[192][3]);

// upsf.c
void unpack_sf_sub_MPEG1(SCALEFACT *scalefac, GR *gr, 
                           int scfsi,   /* bit flag */
                           int igr, BITDAT *bitdat);
void unpack_sf_sub_MPEG2(SCALEFACT sf[],  /* return intensity scale */
                            GR *grdat,
                            int is_and_ch, IS_SF_INFO *is_sf_info, BITDAT *bitdat);

#ifdef __cplusplus
}  // end extern C
#endif
#endif //__MPALOWL3_H_

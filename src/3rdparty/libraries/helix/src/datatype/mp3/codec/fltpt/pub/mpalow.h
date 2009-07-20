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

#ifndef _MPALOW_H_
#define _MPALOW_H_
/*****************************************************************
low level decoder functions Layer I/II

******************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
// Functions from csbt.c
void sbt_mono(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbt_dual(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbt_dual_mono(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbt_dual_left(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbt_dual_right(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);

void sbt16_mono(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbt16_dual(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbt16_dual_mono(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbt16_dual_left(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbt16_dual_right(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);

void sbt8_mono(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbt8_dual(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbt8_dual_mono(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbt8_dual_left(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbt8_dual_right(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);


// eight bit (byte) output)
void sbtB_mono(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbtB_dual(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbtB_dual_mono(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbtB_dual_left(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbtB_dual_right(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);

void sbtB16_mono(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbtB16_dual(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbtB16_dual_mono(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbtB16_dual_left(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbtB16_dual_right(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);

void sbtB8_mono(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbtB8_dual(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbtB8_dual_mono(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbtB8_dual_left(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
void sbtB8_dual_right(float *sample,   unsigned char  *pcm, int n, float vbuf[][512], int vb_ptr[]);
     
#ifdef __cplusplus
}  // end extern C
#endif

#endif //__MPALOW_H_


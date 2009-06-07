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

#ifndef _STATNAME_H
#define _STATNAME_H

/* define STAT_PREFIX to a unique name for static linking 
 * all the C functions and global variables will be mangled by the preprocessor
 *   e.g. void FFT(int *fftbuf) becomes void cook_FFT(int *fftbuf)
 */
#define STAT_PREFIX		xmp3

#define STATCC1(x,y,z)	STATCC2(x,y,z)
#define STATCC2(x,y,z)	x##y##z  

#ifdef STAT_PREFIX
#define STATNAME(func)	STATCC1(STAT_PREFIX, _, func)
#else
#define STATNAME(func)	func
#endif

#define dct_coef_addr STATNAME(dct_coef_addr)
#define fdct32 STATNAME(fdct32)
#define fdct32_dual STATNAME(fdct32_dual)
#define fdct32_dual_mono STATNAME(fdct32_dual_mono)
#define window STATNAME(window)
#define window_dual STATNAME(window_dual)
#define unpack_sf_sub_MPEG1 STATNAME(unpack_sf_sub_MPEG1)
#define unpack_sf_sub_MPEG2 STATNAME(unpack_sf_sub_MPEG2)
#define unpack_huff STATNAME(unpack_huff)
#define unpack_huff_quad STATNAME(unpack_huff_quad)
#define sbt_dual_L3 STATNAME(sbt_dual_L3)
#define sbt_mono_L3 STATNAME(sbt_mono_L3)
#define sbt_dual STATNAME(sbt_dual)
#define sbt_dual_mono STATNAME(sbt_dual_mono)
#define sbt_mono STATNAME(sbt_mono)
#define antialias STATNAME(antialias)
#define is_process_MPEG1 STATNAME(is_process_MPEG1)
#define is_process_MPEG2 STATNAME(is_process_MPEG2)
#define ms_process STATNAME(ms_process)
#define imdct18 STATNAME(imdct18)
#define imdct6_3 STATNAME(imdct6_3)
#define dequant STATNAME(dequant)
#define FreqInvert STATNAME(FreqInvert)
#define hybrid STATNAME(hybrid)
#define hybrid_sum STATNAME(hybrid_sum)
#define sum_f_bands STATNAME(sum_f_bands)
#define bitget STATNAME(bitget)
#define bitget_1bit STATNAME(bitget_1bit)
#define bitget_bits_used STATNAME(bitget_bits_used)
#define bitget_check STATNAME(bitget_check)
#define bitget_init STATNAME(bitget_init)
#define bitget_init_end STATNAME(bitget_init_end)
#define bitget_overrun STATNAME(bitget_overrun)

#define look_global STATNAME(look_global)
#define look_scale STATNAME(look_scale)
#define look_pow STATNAME(look_pow)
#define look_subblock STATNAME(look_subblock)
#define lr STATNAME(lr)
#define lr2 STATNAME(lr2)
#define coef32 STATNAME(coef32)

#endif	/* _STATNAME_H */

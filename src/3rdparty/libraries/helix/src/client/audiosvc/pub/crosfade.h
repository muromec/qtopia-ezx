/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: crosfade.h,v 1.3 2004/07/09 18:44:55 hubbe Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the RealNetworks Public
 * Source License (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the current version of the RealNetworks Community
 * Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply. You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.
 * 
 * This file, and the files included with this file, is distributed
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 * ENJOYMENT OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

#ifndef _CROSS_FADER_
#define _CROSS_FADER_

/*
 * Since (oldsamp-newsamp) can be -65535..65535 (17 bits),
 * could use 17.15 format (without rounding),
 * or use 18.14 format to allow rounding without overflow.
 */
#define FRACBITS	14		/* allow rounding */
#define FRACMASK	((1<<FRACBITS) - 1)
#define FRACROUND	(1 << (FRACBITS-1))

/* table parameters */
#define NALPHA		128		/* size of alpha table */
#define DBSTART		0.0		/* starting gain, in dB */
#define DBEND		-90.0	/* ending gain, in dB */

#define DB2GAIN(d) (pow(10.0, (d)/20.0))

// XXXHP: NEED TO REGENERATE ON WIN16 EVERYTIME THIS ARRAY
//	  IS MODIFIED!!!
#ifdef _WIN16
extern INT32* alpha;
extern INT32* adelta;
#else

#ifdef GENERATE_TABLE
static INT32 alpha[NALPHA+1] = {
#else
static const INT32 alpha[NALPHA+1] = {
#endif
				16384, 15100, 13917, 12827, 11822, 
				10896, 10042, 9255, 8530, 7862, 
				7246, 6678, 6155, 5673, 5228, 
				4819, 4441, 4093, 3772, 3477, 
				3204, 2953, 2722, 2509, 2312, 
				2131, 1964, 1810, 1668, 1538, 
				1417, 1306, 1204, 1110, 1023, 
				942, 869, 801, 738, 680, 
				627, 578, 532, 491, 452, 
				417, 384, 354, 326, 301, 
				277, 255, 235, 217, 200, 
				184, 170, 157, 144, 133, 
				123, 113, 104, 96, 88, 
				82, 75, 69, 64, 59, 
				54, 50, 46, 42, 39, 
				36, 33, 31, 28, 26, 
				24, 22, 20, 19, 17, 
				16, 15, 14, 12, 12, 
				11, 10, 9, 8, 8, 
				7, 6, 6, 6, 5, 
				5, 4, 4, 4, 3, 
				3, 3, 3, 2, 2, 
				2, 2, 2, 2, 1, 
				1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 
				1, 1, 1, 1
};

#ifdef GENERATE_TABLE
static INT32 adelta[NALPHA] = {
#else
static const INT32 adelta[NALPHA] = {
#endif
				-1284, -1183, -1090, -1005, -926, 
				-854, -787, -725, -668, -616, 
				-568, -523, -482, -445, -409, 
				-378, -348, -321, -295, -273, 
				-251, -231, -213, -197, -181, 
				-167, -154, -142, -130, -121, 
				-111, -102, -94, -87, -81, 
				-73, -68, -63, -58, -53, 
				-49, -46, -41, -39, -35, 
				-33, -30, -28, -25, -24, 
				-22, -20, -18, -17, -16, 
				-14, -13, -13, -11, -10, 
				-10, -9, -8, -8, -6, 
				-7, -6, -5, -5, -5, 
				-4, -4, -4, -3, -3, 
				-3, -2, -3, -2, -2, 
				-2, -2, -1, -2, -1, 
				-1, -1, -2, 0, -1, 
				-1, -1, -1, 0, -1, 
				-1, 0, 0, -1, 0, 
				-1, 0, 0, -1, 0, 
				0, 0, -1, 0, 0, 
				0, 0, 0, -1, 0, 
				0, 0, 0, 0, 0, 
				0, 0, 0, 0, 0, 
				0, 0, 0 
};
#endif

class CrossFader
{
public:

    CrossFader();
    ~CrossFader();
    HX_RESULT	Initialize(UINT16 uNumSamplesToFadeOn, UINT16 uNumChannels);
    void	CrossFade(INT16* sampold, INT16* sampnew, UINT16 uNumSamples);

    
#ifdef GENERATE_TABLE
    /* This is for reference use only. this routine is used to generate
     * the static tables 
     */
    void	CrossFadeInit(void);
#endif

protected:

    /* table steppers */
    INT32	tabstep;
    INT32	tabacc;
    INT32	tabint;	
    UINT16	m_uNumChannels : 16;
    HX_BITFIELD	m_bInitialized : 1;

};
#endif /*_CROSS_FADER_*/

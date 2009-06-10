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
 * directly from RealNetworks.	You may not use this file except in 
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
 *	  http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *	
 * ***** END LICENSE BLOCK ***** */ 
#include "hxassert.h"
#include "pcm_convert.h"

const unsigned short g_wA2L16[] = {
 60032, 60288, 59520, 59776, 61056, 61312, 60544, 60800,
 57984, 58240, 57472, 57728, 59008, 59264, 58496, 58752,
 62784, 62912, 62528, 62656, 63296, 63424, 63040, 63168,
 61760, 61888, 61504, 61632, 62272, 62400, 62016, 62144,

 43520, 44544, 41472, 42496, 47616, 48640, 45568, 46592,
 35328, 36352, 33280, 34304, 39424, 40448, 37376, 38400,
 54528, 55040, 53504, 54016, 56576, 57088, 55552, 56064,
 50432, 50944, 49408, 49920, 52480, 52992, 51456, 51968,
 
 65192, 65208, 65160, 65176, 65256, 65272, 65224, 65240,
 65064, 65080, 65032, 65048, 65128, 65144, 65096, 65112,
 65448, 65464, 65416, 65432, 65512, 65528, 65480, 65496,
 65320, 65336, 65288, 65304, 65384, 65400, 65352, 65368,
 
 64160, 64224, 64032, 64096, 64416, 64480, 64288, 64352,
 63648, 63712, 63520, 63584, 63904, 63968, 63776, 63840,
 64848, 64880, 64784, 64816, 64976, 65008, 64912, 64944,	
 64592, 64624, 64528, 64560, 64720, 64752, 64656, 64688,

  5504,  5248,  6016,  5760,  4480,  4224,  4992,  4736,	
  7552,  7296,  8064,  7808,  6528,  6272,  7040,  6784,
  2752,  2624,  3008,  2880,  2240,  2112,  2496,  2368,
  3776,  3648,  4032,  3904,  3264,  3136,  3520,  3392,
 
 22016, 20992, 24064, 23040, 17920, 16896, 19968, 18944,
 30208, 29184, 32256, 31232, 26112, 25088, 28160, 27136,
 11008, 10496, 12032, 11520,  8960,  8448,  9984,  9472,
 15104, 14592, 16128, 15616, 13056, 12544, 14080, 13568,

   344,   328,   376,   360,   280,   264,   312,   296,
   472,   456,   504,   488,   408,   392,   440,   424,
    88,    72,   120,   104,    24,     8,    56,    40,	
   216,   200,   248,   232,   152,   136,   184,   168,

  1376,  1312,  1504,  1440,  1120,  1056,  1248,  1184,
  1888,  1824,  2016,  1952,  1632,  1568,  1760,  1696,
   688,   656,   752,   720,   560,   528,   624,   592,
   944,   912,  1008,   976,   816,   784,   880,   848

};

const unsigned short g_wU2L16[] = {
	33280, 34308, 35336, 36364, 37393, 38421, 39449, 40477,
	41505, 42534, 43562, 44590, 45618, 46647, 47675, 48703,
	49474, 49988, 50503, 51017, 51531, 52045, 52559, 53073,
	53587, 54101, 54616, 55130, 55644, 56158, 56672, 57186,
	57572, 57829, 58086, 58343, 58600, 58857, 59114, 59371,
	59628, 59885, 60142, 60399, 60656, 60913, 61171, 61428,
	61620, 61749, 61877, 62006, 62134, 62263, 62392, 62520,
	62649, 62777, 62906, 63034, 63163, 63291, 63420, 63548,
	63645, 63709, 63773, 63838, 63902, 63966, 64030, 64095,
	64159, 64223, 64287, 64352, 64416, 64480, 64544, 64609,
	64657, 64689, 64721, 64753, 64785, 64818, 64850, 64882,
	64914, 64946, 64978, 65010, 65042, 65075, 65107, 65139,
	65163, 65179, 65195, 65211, 65227, 65243, 65259, 65275,
	65291, 65308, 65324, 65340, 65356, 65372, 65388, 65404,
	65416, 65424, 65432, 65440, 65448, 65456, 65464, 65472,
	65480, 65488, 65496, 65504, 65512, 65520, 65528,     0,
	32256, 31228, 30200, 29172, 28143, 27115, 26087, 25059,
	24031, 23002, 21974, 20946, 19918, 18889, 17861, 16833,
	16062, 15548, 15033, 14519, 14005, 13491, 12977, 12463,
	11949, 11435, 10920, 10406,  9892,  9378,  8864,  8350,
	 7964,  7707,  7450,  7193,  6936,  6679,  6422,  6165, 
	 5908,  5651,  5394,  5137,  4880,  4623,  4365,  4108, 
	 3916,  3787,  3659,  3530,  3402,  3273,  3144,  3016, 
	 2887,  2759,  2630,  2502,  2373,  2245,  2116,  1988, 
	 1891,  1827,  1763,  1698,  1634,  1570,  1506,  1441, 
	 1377,  1313,  1249,  1184,  1120,  1056,   992,   927, 
	  879,   847,   815,   783,   751,   718,   686,   654,
	  622,   590,   558,   526,   494,   461,   429,   397,
	  373,   357,   341,   325,   309,   293,   277,   261,
	  245,   228,   212,   196,   180,   164,   148,   132,
	  120,   112,   104,    96,    88,    80,    72,    64,
 	   56,    48,    40,    32,    24,    16,    8,      0
};

unsigned int PCM_CONVERTER_ALaw2Linear(unsigned char* pbSrc,unsigned short* pwDest, unsigned int dwSampleNum)
{
	unsigned char* pSrcEnd;

	HX_ASSERT(pbSrc && pwDest);
	
	for(pSrcEnd=pbSrc+dwSampleNum; pbSrc<pSrcEnd; pbSrc++,pwDest++)
		*pwDest = PCM_CONVERTER_A_TO_L16(*pbSrc);
	
	return dwSampleNum;
}


unsigned int PCM_CONVERTER_ULaw2Linear(unsigned char* pbSrc,unsigned short* pwDest, unsigned int dwSampleNum)
{
	unsigned char* pSrcEnd;
	
	HX_ASSERT(pbSrc && pwDest);
		
	for(pSrcEnd=pbSrc+dwSampleNum; pbSrc<pSrcEnd; pbSrc++,pwDest++)
		*pwDest = PCM_CONVERTER_U_TO_L16(*pbSrc);
	
	return dwSampleNum;
}

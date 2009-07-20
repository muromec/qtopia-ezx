/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: resize.cpp,v 1.11 2007/07/06 22:00:23 jfinnecy Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

////////////////////////////////////////////////////////
//	include files
////////////////////////////////////////////////////////

#include "hlxclib/stdlib.h"
#include "hlxclib/string.h"
#include "mmx_util.h"
#include "resize.h"


////////////////////////////////////////////////////////
//	internal prototypes
////////////////////////////////////////////////////////

static void
interpolate_ii (
    unsigned char *dest, int dest_width, int dest_height, int dest_pitch,
    unsigned char *src,  int src_width,  int src_height,  int src_pitch);

static void
decimate (
    unsigned char *dest, int dest_width, int dest_height, int dest_pitch,
    unsigned char *src,  int src_width,  int src_height,  int src_pitch, 
	RESIZE_BUFF_TYPE *buff);

#ifdef _M_IX86

static void
decimate_half_horiz_MMX (
    unsigned char *dest, int dest_width, int dest_height,
    unsigned char *src,  int src_width,  int src_height);

#endif

static void 
decimate_half_horiz_accurate(
    unsigned char *dest, int dest_width, int dest_height, int dest_pitch,
    unsigned char *src,  int src_width,  int src_height, int src_pitch);


////////////////////////////////////////////////////////
// Select interpolation or decimation
////////////////////////////////////////////////////////

void resize_plane(
    unsigned char *dest, int dest_width, int dest_height, int dest_pitch,
    unsigned char *src,  int src_width,  int src_height,  int src_pitch,
	RESIZE_BUFF_TYPE *buff, int accuracy)
{

	if ((dest_width > src_width) || (dest_height > src_height) || 
		(accuracy == 0) || (buff == NULL))
	{
		// interpolation
		interpolate_ii (dest,dest_width, dest_height, dest_pitch,
						src, src_width, src_height, src_pitch);
	}
	else
	{
		// decimation -- check for special case functions
#ifdef _M_IX86
		if (((dest_width) == (src_width >> 1)) &&
			((dest_height) == (src_height)) &&
			((dest_pitch) == (dest_width)) &&
			((src_pitch) == (src_width)) &&
            (checkMmxAvailablity()&CPU_HAS_MMX) && (accuracy == 0))
		{
			decimate_half_horiz_MMX (
				dest,dest_width, dest_height,
				src, src_width, src_height);
		}
		else
#endif
		if (((dest_width) == (src_width >> 1)) &&
			((dest_height) == (src_height)))
		{
			decimate_half_horiz_accurate(
				dest,dest_width, dest_height, dest_pitch,
				src, src_width, src_height, src_pitch);
		}
		else
		{
			decimate (dest,dest_width, dest_height, dest_pitch,
						src, src_width, src_height, src_pitch, buff);
		}
	}

}

#if 1 // simpler filter

#define FULL_PEL_INC 256
#define INT_FILT_SIZE 1025
#define INT_FILT_CENTER 512

static const int int_filt_tab[INT_FILT_SIZE] = 
{
	0,
	-3,	-5,	-8,	-11,	-13,	-16,	-19,	-21,	-24,	-27,	-29,	-32,	-35,	-37,	-40,	-43,	-45,	-48,	-50,	-53,
	-56,	-58,	-61,	-63,	-66,	-69,	-71,	-74,	-76,	-79,	-81,	-84,	-87,	-89,	-92,	-94,	-97,	-99,	-102,	-104,
	-107,	-109,	-111,	-114,	-116,	-119,	-121,	-124,	-126,	-128,	-131,	-133,	-135,	-138,	-140,	-142,	-144,	-147,	-149,	-151,
	-153,	-156,	-158,	-160,	-162,	-164,	-166,	-169,	-171,	-173,	-175,	-177,	-179,	-181,	-183,	-185,	-187,	-189,	-191,	-193,
	-194,	-196,	-198,	-200,	-202,	-203,	-205,	-207,	-209,	-210,	-212,	-214,	-215,	-217,	-218,	-220,	-222,	-223,	-225,	-226,
	-227,	-229,	-230,	-232,	-233,	-234,	-235,	-237,	-238,	-239,	-240,	-242,	-243,	-244,	-245,	-246,	-247,	-248,	-249,	-250,
	-251,	-251,	-252,	-253,	-254,	-255,	-255,	-256,	-257,	-257,	-258,	-258,	-259,	-259,	-260,	-260,	-261,	-261,	-261,	-262,
	-262,	-262,	-262,	-263,	-263,	-263,	-263,	-263,	-263,	-263,	-263,	-262,	-262,	-262,	-262,	-262,	-261,	-261,	-260,	-260,
	-260,	-259,	-258,	-258,	-257,	-257,	-256,	-255,	-254,	-253,	-253,	-252,	-251,	-250,	-249,	-248,	-246,	-245,	-244,	-243,
	-241,	-240,	-239,	-237,	-236,	-234,	-233,	-231,	-229,	-228,	-226,	-224,	-222,	-220,	-218,	-216,	-214,	-212,	-210,	-208,
	-206,	-203,	-201,	-199,	-196,	-194,	-191,	-189,	-186,	-183,	-180,	-178,	-175,	-172,	-169,	-166,	-163,	-160,	-157,	-153,
	-150,	-147,	-143,	-140,	-137,	-133,	-129,	-126,	-122,	-118,	-114,	-111,	-107,	-103,	-99,	-94,	-90,	-86,	-82,	-78,
	-73,	-69,	-64,	-60,	-55,	-50,	-45,	-41,	-36,	-31,	-26,	-21,	-16,	-11,	-5,	0,	16,	32,	48,	64,
	81,	97,	113,	130,	146,	163,	180,	196,	213,	230,	247,	264,	280,	297,	314,	332,	349,	366,	383,	400,
	418,	435,	452,	470,	487,	505,	522,	540,	558,	575,	593,	611,	629,	646,	664,	682,	700,	718,	736,	754,
	772,	790,	808,	827,	845,	863,	881,	899,	918,	936,	954,	973,	991,	1009,	1028,	1046,	1065,	1083,	1102,	1120,
	1139,	1157,	1176,	1194,	1213,	1231,	1250,	1268,	1287,	1306,	1324,	1343,	1362,	1380,	1399,	1418,	1436,	1455,	1473,	1492,
	1511,	1529,	1548,	1567,	1585,	1604,	1623,	1641,	1660,	1679,	1697,	1716,	1735,	1753,	1772,	1790,	1809,	1828,	1846,	1865,
	1883,	1902,	1920,	1939,	1957,	1976,	1994,	2013,	2031,	2049,	2068,	2086,	2104,	2123,	2141,	2159,	2177,	2195,	2214,	2232,
	2250,	2268,	2286,	2304,	2322,	2340,	2358,	2376,	2394,	2411,	2429,	2447,	2465,	2482,	2500,	2518,	2535,	2553,	2570,	2588,
	2605,	2622,	2640,	2657,	2674,	2691,	2708,	2725,	2742,	2759,	2776,	2793,	2810,	2827,	2843,	2860,	2877,	2893,	2910,	2926,
	2942,	2959,	2975,	2991,	3007,	3023,	3039,	3055,	3071,	3087,	3103,	3119,	3134,	3150,	3165,	3181,	3196,	3211,	3226,	3242,
	3257,	3272,	3287,	3301,	3316,	3331,	3345,	3360,	3374,	3389,	3403,	3417,	3432,	3446,	3460,	3473,	3487,	3501,	3515,	3528,
	3542,	3555,	3568,	3582,	3595,	3608,	3621,	3633,	3646,	3659,	3671,	3684,	3696,	3708,	3721,	3733,	3745,	3757,	3768,	3780,
	3792,	3803,	3814,	3826,	3837,	3848,	3859,	3870,	3880,	3891,	3902,	3912,	3922,	3932,	3943,	3953,	3962,	3972,	3982,	3991,
	4001,	4010,	4019,	4028,	4037,	4046,	4054,	4063,	4071,	4080,	4088,	4096,	4088,	4080,	4071,	4063,	4054,	4046,	4037,	4028,
	4019,	4010,	4001,	3991,	3982,	3972,	3962,	3953,	3943,	3932,	3922,	3912,	3902,	3891,	3880,	3870,	3859,	3848,	3837,	3826,
	3814,	3803,	3792,	3780,	3768,	3757,	3745,	3733,	3721,	3708,	3696,	3684,	3671,	3659,	3646,	3633,	3621,	3608,	3595,	3582,
	3568,	3555,	3542,	3528,	3515,	3501,	3487,	3473,	3460,	3446,	3432,	3417,	3403,	3389,	3374,	3360,	3345,	3331,	3316,	3301,
	3287,	3272,	3257,	3242,	3226,	3211,	3196,	3181,	3165,	3150,	3134,	3119,	3103,	3087,	3071,	3055,	3039,	3023,	3007,	2991,
	2975,	2959,	2942,	2926,	2910,	2893,	2877,	2860,	2843,	2827,	2810,	2793,	2776,	2759,	2742,	2725,	2708,	2691,	2674,	2657,
	2640,	2622,	2605,	2588,	2570,	2553,	2535,	2518,	2500,	2482,	2465,	2447,	2429,	2411,	2394,	2376,	2358,	2340,	2322,	2304,
	2286,	2268,	2250,	2232,	2214,	2195,	2177,	2159,	2141,	2123,	2104,	2086,	2068,	2049,	2031,	2013,	1994,	1976,	1957,	1939,
	1920,	1902,	1883,	1865,	1846,	1828,	1809,	1790,	1772,	1753,	1735,	1716,	1697,	1679,	1660,	1641,	1623,	1604,	1585,	1567,
	1548,	1529,	1511,	1492,	1473,	1455,	1436,	1418,	1399,	1380,	1362,	1343,	1324,	1306,	1287,	1268,	1250,	1231,	1213,	1194,
	1176,	1157,	1139,	1120,	1102,	1083,	1065,	1046,	1028,	1009,	991,	973,	954,	936,	918,	899,	881,	863,	845,	827,
	808,	790,	772,	754,	736,	718,	700,	682,	664,	646,	629,	611,	593,	575,	558,	540,	522,	505,	487,	470,
	452,	435,	418,	400,	383,	366,	349,	332,	314,	297,	280,	264,	247,	230,	213,	196,	180,	163,	146,	130,
	113,	97,	81,	64,	48,	32,	16,	0,	-5,	-11,	-16,	-21,	-26,	-31,	-36,	-41,	-45,	-50,	-55,	-60,
	-64,	-69,	-73,	-78,	-82,	-86,	-90,	-94,	-99,	-103,	-107,	-111,	-114,	-118,	-122,	-126,	-129,	-133,	-137,	-140,
	-143,	-147,	-150,	-153,	-157,	-160,	-163,	-166,	-169,	-172,	-175,	-178,	-180,	-183,	-186,	-189,	-191,	-194,	-196,	-199,
	-201,	-203,	-206,	-208,	-210,	-212,	-214,	-216,	-218,	-220,	-222,	-224,	-226,	-228,	-229,	-231,	-233,	-234,	-236,	-237,
	-239,	-240,	-241,	-243,	-244,	-245,	-246,	-248,	-249,	-250,	-251,	-252,	-253,	-253,	-254,	-255,	-256,	-257,	-257,	-258,
	-258,	-259,	-260,	-260,	-260,	-261,	-261,	-262,	-262,	-262,	-262,	-262,	-263,	-263,	-263,	-263,	-263,	-263,	-263,	-263,
	-262,	-262,	-262,	-262,	-261,	-261,	-261,	-260,	-260,	-259,	-259,	-258,	-258,	-257,	-257,	-256,	-255,	-255,	-254,	-253,
	-252,	-251,	-251,	-250,	-249,	-248,	-247,	-246,	-245,	-244,	-243,	-242,	-240,	-239,	-238,	-237,	-235,	-234,	-233,	-232,
	-230,	-229,	-227,	-226,	-225,	-223,	-222,	-220,	-218,	-217,	-215,	-214,	-212,	-210,	-209,	-207,	-205,	-203,	-202,	-200,
	-198,	-196,	-194,	-193,	-191,	-189,	-187,	-185,	-183,	-181,	-179,	-177,	-175,	-173,	-171,	-169,	-166,	-164,	-162,	-160,
	-158,	-156,	-153,	-151,	-149,	-147,	-144,	-142,	-140,	-138,	-135,	-133,	-131,	-128,	-126,	-124,	-121,	-119,	-116,	-114,
	-111,	-109,	-107,	-104,	-102,	-99,	-97,	-94,	-92,	-89,	-87,	-84,	-81,	-79,	-76,	-74,	-71,	-69,	-66,	-63,
	-61,	-58,	-56,	-53,	-50,	-48,	-45,	-43,	-40,	-37,	-35,	-32,	-29,	-27,	-24,	-21,	-19,	-16,	-13,	-11,
	-8,	-5,	-3,	0
};

#else  // complex filter

#define FULL_PEL_INC 128
#define INT_FILT_SIZE 1025
#define INT_FILT_CENTER 512

static const int int_filt_tab[INT_FILT_SIZE] = 
{
	0,
	0,	0,	-1,	-1,	-1,	-1,	-2,	-2,	-2,	-2,	-2,	-3,	-3,	-3,	-3,	-4,	-4,	-4,	-4,	-4,
	-5,	-5,	-5,	-5,	-5,	-6,	-6,	-6,	-6,	-6,	-7,	-7,	-7,	-7,	-7,	-7,	-8,	-8,	-8,	-8,
	-8,	-8,	-8,	-8,	-9,	-9,	-9,	-9,	-9,	-9,	-9,	-9,	-9,	-10,	-10,	-10,	-10,	-10,	-10,	-10,
	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,
	-10,	-9,	-9,	-9,	-9,	-9,	-9,	-9,	-9,	-9,	-8,	-8,	-8,	-8,	-8,	-8,	-8,	-7,	-7,	-7,
	-7,	-7,	-6,	-6,	-6,	-6,	-6,	-5,	-5,	-5,	-5,	-4,	-4,	-4,	-4,	-3,	-3,	-3,	-3,	-2,
	-2,	-2,	-1,	-1,	-1,	-1,	0,	0,	2,	4,	6,	9,	11,	13,	15,	17,	19,	21,	23,	25,
	28,	30,	32,	34,	36,	38,	40,	42,	44,	46,	48,	50,	51,	53,	55,	57,	59,	61,	62,	64,
	66,	67,	69,	70,	72,	74,	75,	76,	78,	79,	81,	82,	83,	84,	85,	87,	88,	89,	90,	91,
	91,	92,	93,	94,	95,	95,	96,	96,	97,	97,	98,	98,	98,	99,	99,	99,	99,	99,	99,	99,
	99,	98,	98,	98,	97,	97,	96,	96,	95,	94,	94,	93,	92,	91,	90,	89,	88,	87,	85,	84,
	83,	81,	80,	78,	77,	75,	73,	71,	70,	68,	66,	64,	62,	60,	57,	55,	53,	51,	48,	46,
	43,	41,	38,	35,	33,	30,	27,	24,	21,	18,	16,	12,	9,	6,	3,	0,	-10,	-19,	-29,	-39,
	-48,	-58,	-68,	-78,	-87,	-97,	-107,	-117,	-126,	-136,	-146,	-155,	-165,	-175,	-184,	-194,	-203,	-212,	-222,	-231,
	-240,	-249,	-258,	-267,	-276,	-285,	-293,	-302,	-310,	-318,	-326,	-335,	-342,	-350,	-358,	-365,	-373,	-380,	-387,	-394,
	-400,	-407,	-413,	-420,	-425,	-431,	-437,	-442,	-447,	-452,	-457,	-462,	-466,	-470,	-474,	-478,	-481,	-484,	-487,	-490,
	-492,	-495,	-496,	-498,	-500,	-501,	-501,	-502,	-502,	-502,	-502,	-501,	-500,	-499,	-498,	-496,	-494,	-491,	-488,	-485,
	-482,	-478,	-474,	-470,	-465,	-460,	-455,	-449,	-443,	-436,	-429,	-422,	-415,	-407,	-399,	-390,	-381,	-372,	-362,	-352,
	-342,	-331,	-320,	-309,	-297,	-285,	-272,	-259,	-246,	-232,	-218,	-203,	-188,	-173,	-158,	-142,	-125,	-108,	-91,	-74,
	-56,	-38,	-19,	0,	32,	65,	98,	131,	165,	198,	233,	267,	302,	338,	373,	409,	445,	481,	518,	555,
	592,	629,	667,	705,	743,	781,	820,	858,	897,	936,	975,	1015,	1054,	1094,	1133,	1173,	1213,	1253,	1293,	1333,
	1374,	1414,	1454,	1495,	1535,	1575,	1616,	1656,	1697,	1737,	1778,	1818,	1858,	1898,	1939,	1979,	2019,	2059,	2098,	2138,
	2178,	2217,	2256,	2295,	2334,	2373,	2412,	2450,	2488,	2526,	2564,	2601,	2639,	2676,	2712,	2749,	2785,	2821,	2857,	2892,
	2927,	2961,	2996,	3030,	3063,	3097,	3129,	3162,	3194,	3226,	3257,	3288,	3318,	3348,	3378,	3407,	3436,	3464,	3492,	3519,
	3546,	3572,	3598,	3623,	3648,	3672,	3696,	3719,	3742,	3764,	3785,	3806,	3826,	3846,	3865,	3884,	3902,	3919,	3936,	3952,
	3968,	3983,	3997,	4011,	4024,	4036,	4048,	4059,	4069,	4079,	4088,	4096,	4088,	4079,	4069,	4059,	4048,	4036,	4024,	4011,
	3997,	3983,	3968,	3952,	3936,	3919,	3902,	3884,	3865,	3846,	3826,	3806,	3785,	3764,	3742,	3719,	3696,	3672,	3648,	3623,
	3598,	3572,	3546,	3519,	3492,	3464,	3436,	3407,	3378,	3348,	3318,	3288,	3257,	3226,	3194,	3162,	3129,	3097,	3063,	3030,
	2996,	2961,	2927,	2892,	2857,	2821,	2785,	2749,	2712,	2676,	2639,	2601,	2564,	2526,	2488,	2450,	2412,	2373,	2334,	2295,
	2256,	2217,	2178,	2138,	2098,	2059,	2019,	1979,	1939,	1898,	1858,	1818,	1778,	1737,	1697,	1656,	1616,	1575,	1535,	1495,
	1454,	1414,	1374,	1333,	1293,	1253,	1213,	1173,	1133,	1094,	1054,	1015,	975,	936,	897,	858,	820,	781,	743,	705,
	667,	629,	592,	555,	518,	481,	445,	409,	373,	338,	302,	267,	233,	198,	165,	131,	98,	65,	32,	0,
	-19,	-38,	-56,	-74,	-91,	-108,	-125,	-142,	-158,	-173,	-188,	-203,	-218,	-232,	-246,	-259,	-272,	-285,	-297,	-309,
	-320,	-331,	-342,	-352,	-362,	-372,	-381,	-390,	-399,	-407,	-415,	-422,	-429,	-436,	-443,	-449,	-455,	-460,	-465,	-470,
	-474,	-478,	-482,	-485,	-488,	-491,	-494,	-496,	-498,	-499,	-500,	-501,	-502,	-502,	-502,	-502,	-501,	-501,	-500,	-498,
	-496,	-495,	-492,	-490,	-487,	-484,	-481,	-478,	-474,	-470,	-466,	-462,	-457,	-452,	-447,	-442,	-437,	-431,	-425,	-420,
	-413,	-407,	-400,	-394,	-387,	-380,	-373,	-365,	-358,	-350,	-342,	-335,	-326,	-318,	-310,	-302,	-293,	-285,	-276,	-267,
	-258,	-249,	-240,	-231,	-222,	-212,	-203,	-194,	-184,	-175,	-165,	-155,	-146,	-136,	-126,	-117,	-107,	-97,	-87,	-78,
	-68,	-58,	-48,	-39,	-29,	-19,	-10,	0,	3,	6,	9,	12,	16,	18,	21,	24,	27,	30,	33,	35,
	38,	41,	43,	46,	48,	51,	53,	55,	57,	60,	62,	64,	66,	68,	70,	71,	73,	75,	77,	78,
	80,	81,	83,	84,	85,	87,	88,	89,	90,	91,	92,	93,	94,	94,	95,	96,	96,	97,	97,	98,
	98,	98,	99,	99,	99,	99,	99,	99,	99,	99,	98,	98,	98,	97,	97,	96,	96,	95,	95,	94,
	93,	92,	91,	91,	90,	89,	88,	87,	85,	84,	83,	82,	81,	79,	78,	76,	75,	74,	72,	70,
	69,	67,	66,	64,	62,	61,	59,	57,	55,	53,	51,	50,	48,	46,	44,	42,	40,	38,	36,	34,
	32,	30,	28,	25,	23,	21,	19,	17,	15,	13,	11,	9,	6,	4,	2,	0,	0,	-1,	-1,	-1,
	-1,	-2,	-2,	-2,	-3,	-3,	-3,	-3,	-4,	-4,	-4,	-4,	-5,	-5,	-5,	-5,	-6,	-6,	-6,	-6,
	-6,	-7,	-7,	-7,	-7,	-7,	-8,	-8,	-8,	-8,	-8,	-8,	-8,	-9,	-9,	-9,	-9,	-9,	-9,	-9,
	-9,	-9,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,
	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-10,	-9,	-9,	-9,	-9,	-9,	-9,	-9,	-9,	-9,	-8,
	-8,	-8,	-8,	-8,	-8,	-8,	-8,	-7,	-7,	-7,	-7,	-7,	-7,	-6,	-6,	-6,	-6,	-6,	-5,	-5,
	-5,	-5,	-5,	-4,	-4,	-4,	-4,	-4,	-3,	-3,	-3,	-3,	-2,	-2,	-2,	-2,	-2,	-1,	-1,	-1,
	-1,	0,	0, 0
};

#endif



// Clipping table
#define CLAMP_BIAS  128 // Bias in clamping table 
#define CLIP_RANGE	(CLAMP_BIAS + 256 + CLAMP_BIAS)

const unsigned char ClampTbl[CLIP_RANGE] = { /* Flawfinder: ignore */
             0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
            ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
            ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
            ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
            ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
            ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
            ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
            ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
            ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
            ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
            ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
            ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
            ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
            ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
            ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
            ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
            ,0x00 ,0x01 ,0x02 ,0x03 ,0x04 ,0x05 ,0x06 ,0x07 
            ,0x08 ,0x09 ,0x0a ,0x0b ,0x0c ,0x0d ,0x0e ,0x0f 
            ,0x10 ,0x11 ,0x12 ,0x13 ,0x14 ,0x15 ,0x16 ,0x17 
            ,0x18 ,0x19 ,0x1a ,0x1b ,0x1c ,0x1d ,0x1e ,0x1f 
            ,0x20 ,0x21 ,0x22 ,0x23 ,0x24 ,0x25 ,0x26 ,0x27 
            ,0x28 ,0x29 ,0x2a ,0x2b ,0x2c ,0x2d ,0x2e ,0x2f 
            ,0x30 ,0x31 ,0x32 ,0x33 ,0x34 ,0x35 ,0x36 ,0x37 
            ,0x38 ,0x39 ,0x3a ,0x3b ,0x3c ,0x3d ,0x3e ,0x3f 
            ,0x40 ,0x41 ,0x42 ,0x43 ,0x44 ,0x45 ,0x46 ,0x47 
            ,0x48 ,0x49 ,0x4a ,0x4b ,0x4c ,0x4d ,0x4e ,0x4f 
            ,0x50 ,0x51 ,0x52 ,0x53 ,0x54 ,0x55 ,0x56 ,0x57 
            ,0x58 ,0x59 ,0x5a ,0x5b ,0x5c ,0x5d ,0x5e ,0x5f 
            ,0x60 ,0x61 ,0x62 ,0x63 ,0x64 ,0x65 ,0x66 ,0x67 
            ,0x68 ,0x69 ,0x6a ,0x6b ,0x6c ,0x6d ,0x6e ,0x6f 
            ,0x70 ,0x71 ,0x72 ,0x73 ,0x74 ,0x75 ,0x76 ,0x77 
            ,0x78 ,0x79 ,0x7a ,0x7b ,0x7c ,0x7d ,0x7e ,0x7f 
            ,0x80 ,0x81 ,0x82 ,0x83 ,0x84 ,0x85 ,0x86 ,0x87 
            ,0x88 ,0x89 ,0x8a ,0x8b ,0x8c ,0x8d ,0x8e ,0x8f 
            ,0x90 ,0x91 ,0x92 ,0x93 ,0x94 ,0x95 ,0x96 ,0x97 
            ,0x98 ,0x99 ,0x9a ,0x9b ,0x9c ,0x9d ,0x9e ,0x9f 
            ,0xa0 ,0xa1 ,0xa2 ,0xa3 ,0xa4 ,0xa5 ,0xa6 ,0xa7 
            ,0xa8 ,0xa9 ,0xaa ,0xab ,0xac ,0xad ,0xae ,0xaf 
            ,0xb0 ,0xb1 ,0xb2 ,0xb3 ,0xb4 ,0xb5 ,0xb6 ,0xb7 
            ,0xb8 ,0xb9 ,0xba ,0xbb ,0xbc ,0xbd ,0xbe ,0xbf 
            ,0xc0 ,0xc1 ,0xc2 ,0xc3 ,0xc4 ,0xc5 ,0xc6 ,0xc7 
            ,0xc8 ,0xc9 ,0xca ,0xcb ,0xcc ,0xcd ,0xce ,0xcf 
            ,0xd0 ,0xd1 ,0xd2 ,0xd3 ,0xd4 ,0xd5 ,0xd6 ,0xd7 
            ,0xd8 ,0xd9 ,0xda ,0xdb ,0xdc ,0xdd ,0xde ,0xdf 
            ,0xe0 ,0xe1 ,0xe2 ,0xe3 ,0xe4 ,0xe5 ,0xe6 ,0xe7 
            ,0xe8 ,0xe9 ,0xea ,0xeb ,0xec ,0xed ,0xee ,0xef 
            ,0xf0 ,0xf1 ,0xf2 ,0xf3 ,0xf4 ,0xf5 ,0xf6 ,0xf7 
            ,0xf8 ,0xf9 ,0xfa ,0xfb ,0xfc ,0xfd ,0xfe ,0xff 
            ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff 
            ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff 
            ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff 
            ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff 
            ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff 
            ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff 
            ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff 
            ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff 
            ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff 
            ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff 
            ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff 
            ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff 
            ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff 
            ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff 
            ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff 
            ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff 
   };

#ifdef CLIP_PEL
#undef CLIP_PEL
#endif

#define CLIP_PEL(p) (ClampTbl[ (p) + CLAMP_BIAS ])


#define POS_SCALE_BITS		16
#define FILT_COEFF_BITS		12
#define FILT_SCALE_BITS		8

#define USE_SOURCE_LEVEL	1


////////////////////////////////////////////////////////
//
//	Performs fast half horizontal w/ MMX
//		Greg Conklin - 9/21/99
//
////////////////////////////////////////////////////////

#ifdef _M_IX86

static void
decimate_half_horiz_MMX (
    unsigned char *dest, int dest_width, int dest_height,
    unsigned char *src,  int src_width,  int src_height)
{
	unsigned char word_mask[8] = {0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00}; /* Flawfinder: ignore */
	int	num_itrs;

	// must be doing half horizontal resolution
	if (((dest_width) != (src_width >> 1)) ||
		((dest_height) != (src_height)))
		return;

	num_itrs = ((dest_height*dest_width) >> 3);

	__asm {

		mov			ecx, num_itrs
		mov			esi, src			// esi -> source frame
		mov			edi, dest			// edi -> destination frame
		movq		mm7, [word_mask]	// mm7 holds the bytes->words mask

ALIGN 16
pel_loop:
		movq		mm0, [esi]
		movq		mm1, mm0
		psrlw		mm1, 8	
		pand		mm0, mm7
		paddw		mm0, mm1
		psrlw		mm0, 1

		lea			esi, [esi+8]

		movq		mm1, [esi]
		movq		mm2, mm1
		psrlw		mm2, 8
		pand		mm1, mm7
		paddw		mm1, mm2
		psrlw		mm1, 1

		lea			esi, [esi+8]		

		packuswb	mm0, mm1
		movq		[edi], mm0

		lea			edi, [edi+8]		

		dec			ecx
		jnz			pel_loop

		emms
	}
}

#endif

////////////////////////////////////////////////////////
//
//	Performs fast and accurate half horizontal
//		Greg Conklin - 9/05/00
//
////////////////////////////////////////////////////////

static void 
decimate_half_horiz_accurate(
    unsigned char *dest, int dest_width, int dest_height, int dest_pitch,
    unsigned char *src,  int src_width,  int src_height, int src_pitch)
{
	unsigned char *dd;
	unsigned char *ss;
	unsigned char *src_end;
	unsigned char *line_end;
	int dest_skip;
	int src_skip;
	int a,b,c,d;

	dd = dest;
	ss = src;
	dest_skip = dest_pitch - dest_width + 1;
	src_skip = src_pitch - src_width + 2;
	src_end = src + src_width * src_height;

	while (ss < src_end)
	{
		line_end = ss + src_width - 2;

		a = ss[0];
		b = a;
		c = ss[1];
		d = ss[2];

		b += c;
		a += d;
		b *= 9;
		b -= a;
		b += 7;
		b >>= 4;

		dd[0] = CLIP_PEL(b);

		ss += 2;
		dd ++;

		while (ss < line_end)
		{
			a = c;
			b = d;
			c = ss[1];
			d = ss[2];

			b += c;
			a += d;
			b *= 9;
			b -= a;
			b += 7;
			b >>= 4;

			dd[0] = CLIP_PEL(b);

			ss += 2;
			dd ++;
		}

		a = c;
		b = d;
		c = ss[1];
		d = c;

		b += c;
		a += d;
		b *= 9;
		b -= a;
		b += 7;
		b >>= 4;

		dd[0] = CLIP_PEL(b);

		ss += src_skip;
		dd += dest_skip;
	}

}

////////////////////////////////////////////////////////
//
//	Performs accurate decimation
//		Greg Conklin - 9/21/99
//
////////////////////////////////////////////////////////

static void
decimate (
    unsigned char *dest, int dest_width, int dest_height, int dest_pitch,
    unsigned char *src,  int src_width,  int src_height,  int src_pitch, 
	RESIZE_BUFF_TYPE *buff)
{

	unsigned char *sp, *dp;
	RESIZE_BUFF_TYPE *tp;
	int pel;			// destination x position
	int line;			// destination y position
	int tmp;			// temp storage for result pel
	int pos;			// fractional position of source (x or y) location
	int pos_int;		// integer position of source
	int pos_bgn;		// the starting location of pos
	int pos_inc;		// the increment of 'pos' to the next pel
	const int *filt_tab_ptr;	// pointer to the current filter tap
	int filt_tab_inc;	// spacing between tabs in the filter table
	int filt_tap_num;	// number of filter taps
	int	filt_scale;		// filter gain 

#if USE_SOURCE_LEVEL
	int source_level;
#else
#define source_level 0
#endif

	if (dest_height == src_height)
	{
		// no vertical resizing
		for (line = 0; line < src_height; line++)
		{
			tp = buff + line * src_pitch;
			sp = src + line * src_pitch;

			for (pel = 0; pel < src_width - 3; pel += 4)
			{
				tp[0] = sp[0];
				tp[1] = sp[1];
				tp[2] = sp[2];
				tp[3] = sp[3];
				tp += 4;
				sp += 4;
			}
			for (; pel < dest_width; pel++)
			{
				*tp = *sp;
				tp ++;
				sp ++;
			}
		}

	}
	else if (dest_height == (src_height >> 1))
	{
		// decimate vertical by 2

		int src_skip = src_pitch - src_width;
		int a, b, c, d;

		tp = buff;
		sp = src;

		// top line
		for (pel = 0; pel < src_width; pel ++)
		{
			b = *sp;
			c = *(sp + src_pitch);
			d = *(sp + 2 * src_pitch);

			c += b;
			b += d;
			c *= 9;
			c -= b;
			c += 7;
			c >>= 4;

			*tp = CLIP_PEL(c);

			sp++;
			tp++;
		}
		tp += src_skip;
		sp += src_skip + src_pitch;

		// middle lines
		for (line = 2; line < dest_height; line ++)
		{
			for (pel = 0; pel < src_width; pel++)
			{
				a = *(sp - src_pitch);
				b = *sp;
				c = *(sp + src_pitch);
				d = *(sp + 2 * src_pitch);

				b += c;
				a += d;
				b *= 9;
				b -= a;
				b += 7;
				b >>= 4;

				*tp = CLIP_PEL(b);

				sp++;
				tp++;
			}
			tp += src_skip;
			sp += src_skip + src_pitch;
		}

		// bottom line
		for (pel = 0; pel < src_width; pel++)
		{
			a = *(sp - src_pitch);
			b = *sp;
			c = *(sp + src_pitch);

			b += c;
			a += c;
			b *= 9;
			b -= a;
			b += 7;
			b >>= 4;

			*tp = CLIP_PEL(b);

			sp++;
			tp++;
		}
	}
	else
	{
		// arbitrary vertical resize

		pos_inc = ((src_height << POS_SCALE_BITS) + (dest_height >> 1)) / (dest_height);
		pos_bgn = (pos_inc - (1 << POS_SCALE_BITS)) >> 1;
		filt_tab_inc = (FULL_PEL_INC * dest_height + (src_height >> 1)) / (src_height);
		if (filt_tab_inc > FULL_PEL_INC)
			filt_tab_inc = FULL_PEL_INC;
		filt_tap_num = (INT_FILT_SIZE - 1) / (filt_tab_inc << 1);
		filt_scale = ((dest_height << FILT_SCALE_BITS) + (src_height >> 1)) / (src_height);

		for (pel = 0; pel < src_width; pel++)
		{
			tp = buff + pel;
			
			line = 0;
			pos = pos_bgn;
			pos_int = pos >> POS_SCALE_BITS;

			// Top edge pels
			while (pos_int < filt_tap_num)
			{
				sp = src + (pos_int) * src_pitch + pel;
#if USE_SOURCE_LEVEL
				source_level = *sp;
#endif
				sp -= filt_tap_num * src_pitch;

				filt_tab_ptr = int_filt_tab;
				filt_tab_ptr -= (filt_tab_inc * (pos & ((1L << POS_SCALE_BITS) - 1))) >> POS_SCALE_BITS;
				filt_tab_ptr += INT_FILT_CENTER - (filt_tap_num * filt_tab_inc);
				if (filt_tab_ptr < int_filt_tab)
				{
					sp += src_pitch;
					filt_tab_ptr += filt_tab_inc;
				}

				tmp = 0;
				while (filt_tab_ptr < int_filt_tab + INT_FILT_SIZE)
				{
					if (sp < src)
						tmp += (*filt_tab_ptr) * (src[pel] - source_level);
					else
						tmp += (*filt_tab_ptr) * (*sp - source_level);
					sp += src_pitch;;
					filt_tab_ptr += filt_tab_inc;
				}
				tmp *= filt_scale;
				tmp += (source_level << (FILT_COEFF_BITS + FILT_SCALE_BITS));
				tmp += (1U << (FILT_COEFF_BITS + FILT_SCALE_BITS - 1));
				tmp >>= (FILT_COEFF_BITS + FILT_SCALE_BITS);
				*tp = tmp;

				tp += src_pitch;
				pos += pos_inc;
				pos_int = pos >> POS_SCALE_BITS;
				line++;
			}
			// Center pels
			while (pos_int < src_height - filt_tap_num - 1)
			{
				sp = src + (pos_int) * src_pitch + pel;
#if USE_SOURCE_LEVEL
				source_level = *sp;
#endif
				sp -= filt_tap_num * src_pitch;

				filt_tab_ptr = int_filt_tab;
				filt_tab_ptr -= (filt_tab_inc * (pos & ((1L << POS_SCALE_BITS) - 1))) >> POS_SCALE_BITS;
				filt_tab_ptr += INT_FILT_CENTER - (filt_tap_num * filt_tab_inc);
				if (filt_tab_ptr < int_filt_tab)
				{
					sp += src_pitch;
					filt_tab_ptr += filt_tab_inc;
				}

				// There are at least 4 taps...
				tmp = (*filt_tab_ptr) * (*sp - source_level);
				sp += src_pitch;
				filt_tab_ptr += filt_tab_inc;

				tmp += (*filt_tab_ptr) * (*sp - source_level);
				sp += src_pitch;
				filt_tab_ptr += filt_tab_inc;

				tmp += (*filt_tab_ptr) * (*sp - source_level);
				sp += src_pitch;
				filt_tab_ptr += filt_tab_inc;

				tmp += (*filt_tab_ptr) * (*sp - source_level);
				sp += src_pitch;
				filt_tab_ptr += filt_tab_inc;
				
				// Remaining taps...
				while (filt_tab_ptr < int_filt_tab + INT_FILT_SIZE)
				{
					tmp += (*filt_tab_ptr) * (*sp - source_level);
					sp += src_pitch;
					filt_tab_ptr += filt_tab_inc;
				}

				// scale and store result...
				tmp *= filt_scale;
				tmp += (source_level << (FILT_COEFF_BITS + FILT_SCALE_BITS));
				tmp += (1U << (FILT_COEFF_BITS + FILT_SCALE_BITS - 1));
				tmp >>= (FILT_COEFF_BITS + FILT_SCALE_BITS);
				*tp = tmp;

				tp += src_pitch;
				pos += pos_inc;
				pos_int = pos >> POS_SCALE_BITS;
				line++;
			}
			// Bottom edge pels
			while (line < dest_height)
			{
				sp = src + (pos_int) * src_pitch + pel;
#if USE_SOURCE_LEVEL
				source_level = *sp;
#endif
				sp -= filt_tap_num * src_pitch;

				filt_tab_ptr = int_filt_tab;
				filt_tab_ptr -= (filt_tab_inc * (pos & ((1L << POS_SCALE_BITS) - 1))) >> POS_SCALE_BITS;
				filt_tab_ptr += INT_FILT_CENTER - (filt_tap_num * filt_tab_inc);
				if (filt_tab_ptr < int_filt_tab)
				{
					sp += src_pitch;
					filt_tab_ptr += filt_tab_inc;
				}

				tmp = 0;
				while (filt_tab_ptr < int_filt_tab + INT_FILT_SIZE)
				{
					if (sp >= src + src_height*src_pitch)
						tmp += (*filt_tab_ptr) * (src[(src_height - 1) * src_pitch + pel] - source_level);
					else
						tmp += (*filt_tab_ptr) * (*sp - source_level);
					sp += src_pitch;
					filt_tab_ptr += filt_tab_inc;
				}
				tmp *= filt_scale;
				tmp += (source_level << (FILT_COEFF_BITS + FILT_SCALE_BITS));
				tmp += (1U << (FILT_COEFF_BITS + FILT_SCALE_BITS - 1));
				tmp >>= (FILT_COEFF_BITS + FILT_SCALE_BITS);
				*tp = tmp;
				
				tp += src_pitch;
				pos += pos_inc;
				pos_int = pos >> POS_SCALE_BITS;
				line++;
			}
		}
	}

	if (dest_width == src_width)
	{
		// no horizontal resizing
		for (line = 0; line < dest_height; line++)
		{
			tp = buff + line * src_pitch;
			dp = dest + line * dest_pitch;

			for (pel = 0; pel < dest_width - 3; pel += 4)
			{
				dp[0] = CLIP_PEL(tp[0]);
				dp[1] = CLIP_PEL(tp[1]);
				dp[2] = CLIP_PEL(tp[2]);
				dp[3] = CLIP_PEL(tp[3]);
				dp += 4;
				tp += 4;
			}
			for (; pel < dest_width; pel++)
			{
				*dp = CLIP_PEL(*tp);
				dp ++;
				tp ++;
			}
		}
	}
	else if (dest_width == (src_width >> 1))
	{
		// decimate horizontally by 2

		int src_skip = src_pitch - src_width + 2;
		int dest_skip = dest_pitch - dest_width + 1;
		int a, b, c, d;

		tp = buff;
		dp = dest;

		for (line = 0; line < dest_height; line ++)
		{
			a = tp[0];
			b = a;
			c = tp[1];
			d = tp[2];

			b += c;
			a += d;
			b *= 9;
			b -= a;
			b += 7;
			b >>= 4;

			*dp = CLIP_PEL(b);

			tp += 2;
			dp ++;

			for (pel = 1; pel < dest_width - 1; pel++)
			{
				a = c;
				b = d;
				c = tp[1];
				d = tp[2];

				b += c;
				a += d;
				b *= 9;
				b -= a;
				b += 7;
				b >>= 4;

				*dp = CLIP_PEL(b);

				tp += 2;
				dp ++;
			}
			a = c;
			b = d;
			c = tp[1];
			d = c;

			b += c;
			a += d;
			b *= 9;
			b -= a;
			b += 7;
			b >>= 4;

			*dp = CLIP_PEL(b);

			tp += src_skip;
			dp += dest_skip;
		}
	}
	else
	{
		// horizonal filtering
		pos_inc = ((src_width << POS_SCALE_BITS) + (dest_width >> 1)) / (dest_width);
		pos_bgn = (pos_inc - (1 << POS_SCALE_BITS)) >> 1;
		filt_tab_inc = (FULL_PEL_INC * dest_width + (src_width >> 1)) / (src_width);
		if (filt_tab_inc > FULL_PEL_INC)
			filt_tab_inc = FULL_PEL_INC;
		filt_tap_num = (INT_FILT_SIZE - 1) / (filt_tab_inc << 1);
		filt_scale = ((dest_width << FILT_SCALE_BITS) + (src_width >> 1)) / (src_width);

		for (line = 0; line < dest_height; line++)
		{
			dp = dest + line * dest_pitch;
			
			pel = 0;
			pos = pos_bgn;
			pos_int = pos >> POS_SCALE_BITS;

			// Left edge pels
			while (pos_int < filt_tap_num)
			{
				tp = buff + line * src_pitch + pos_int;

#if USE_SOURCE_LEVEL
				source_level = *tp;
#endif
				tp -= filt_tap_num;

				filt_tab_ptr = int_filt_tab;
				filt_tab_ptr -= (filt_tab_inc * (pos & ((1L << POS_SCALE_BITS) - 1))) >> POS_SCALE_BITS;
				filt_tab_ptr += INT_FILT_CENTER - (filt_tap_num * filt_tab_inc);
				if (filt_tab_ptr < int_filt_tab)
				{
					tp ++;
					filt_tab_ptr += filt_tab_inc;
				}

				tmp = 0;
				while (filt_tab_ptr < int_filt_tab + INT_FILT_SIZE)
				{
					if (tp < buff)
						tmp += (*filt_tab_ptr) * (buff[line * src_pitch] - source_level);
					else
						tmp += (*filt_tab_ptr) * (*tp - source_level);
					tp++;
					filt_tab_ptr += filt_tab_inc;
				}
				tmp *= filt_scale;
				tmp += (source_level << (FILT_COEFF_BITS + FILT_SCALE_BITS));
				tmp += (1U << (FILT_COEFF_BITS + FILT_SCALE_BITS - 1));
				tmp >>= (FILT_COEFF_BITS + FILT_SCALE_BITS);
				*dp = CLIP_PEL(tmp);

				dp++;
				pos += pos_inc;
				pos_int = pos >> POS_SCALE_BITS;
				pel++;
			}
			// Center pels
			while (pos_int < src_width - filt_tap_num - 1)
			{
				tp = buff + line * src_pitch + pos_int;

#if USE_SOURCE_LEVEL
				source_level = *tp;
#endif
				tp -= filt_tap_num;

				filt_tab_ptr = int_filt_tab;
				filt_tab_ptr -= (filt_tab_inc * (pos & ((1L << POS_SCALE_BITS) - 1))) >> POS_SCALE_BITS;
				filt_tab_ptr += INT_FILT_CENTER - (filt_tap_num * filt_tab_inc);
				if (filt_tab_ptr < int_filt_tab)
				{
					tp ++;
					filt_tab_ptr += filt_tab_inc;
				}

				// There are at least 4 taps...
				tmp = (*filt_tab_ptr) * (tp[0] - source_level);
				filt_tab_ptr += filt_tab_inc;
				tmp += (*filt_tab_ptr) * (tp[1] - source_level);
				filt_tab_ptr += filt_tab_inc;
				tmp += (*filt_tab_ptr) * (tp[2] - source_level);
				filt_tab_ptr += filt_tab_inc;
				tmp += (*filt_tab_ptr) * (tp[3] - source_level);
				filt_tab_ptr += filt_tab_inc;

				tp += 4;
				
				// Remaining taps...
				while (filt_tab_ptr < int_filt_tab + INT_FILT_SIZE)
				{
					tmp += (*filt_tab_ptr) * (*tp - source_level);
					tp++;
					filt_tab_ptr += filt_tab_inc;
				}
				tmp *= filt_scale;
				tmp += (source_level << (FILT_COEFF_BITS + FILT_SCALE_BITS));
				tmp += (1U << (FILT_COEFF_BITS + FILT_SCALE_BITS - 1));
				tmp >>= (FILT_COEFF_BITS + FILT_SCALE_BITS);

				*dp = CLIP_PEL(tmp);

				dp++;
				pos += pos_inc;
				pos_int = pos >> POS_SCALE_BITS;
				pel++;
			}
			// Right edge pels
			while (pel < dest_width)
			{
				tp = buff + line * src_pitch + pos_int;

#if USE_SOURCE_LEVEL
				source_level = *tp;
#endif
				tp -= filt_tap_num;

				filt_tab_ptr = int_filt_tab;
				filt_tab_ptr -= (filt_tab_inc * (pos & ((1L << POS_SCALE_BITS) - 1))) >> POS_SCALE_BITS;
				filt_tab_ptr += INT_FILT_CENTER - (filt_tap_num * filt_tab_inc);
				if (filt_tab_ptr < int_filt_tab)
				{
					tp ++;
					filt_tab_ptr += filt_tab_inc;
				}

				tmp = 0;
				while (filt_tab_ptr < int_filt_tab + INT_FILT_SIZE)
				{
					if (tp >= buff + line * src_pitch + src_width)
						tmp += (*filt_tab_ptr) * (buff[line * src_pitch + src_width - 1] - source_level);
					else
						tmp += (*filt_tab_ptr) * (*tp - source_level);
					tp++;
					filt_tab_ptr += filt_tab_inc;
				}
				tmp *= filt_scale;
				tmp += (source_level << (FILT_COEFF_BITS + FILT_SCALE_BITS));
				tmp += (1U << (FILT_COEFF_BITS + FILT_SCALE_BITS - 1));
				tmp >>= (FILT_COEFF_BITS + FILT_SCALE_BITS);
				*dp = CLIP_PEL(tmp);

				dp++;
				pos += pos_inc;
				pos_int = pos >> POS_SCALE_BITS;
				pel++;
			}
		}
	}
}

	
/* 12-19-98 02:36am, written by Yuriy A. Reznik, yreznik@real.com */

#define TOTAL_SCALE_BITS    16  /* precision of inverse coordinate mapping */
#define COEF_SCALE_BITS     3   /* actual grid used to select scale coeffs */

/*
 * Interpolate image.
 * Use:
 *  void interpolate_x (
 *       unsigned char *dest, int dest_width, int dest_height, int dest_pitch,
 *       unsigned char *src,  int src_width,  int src_height,  int src_pitch);
 */
static void  /** G2H263Codec:: NEVER make this function a class method!!!! */
interpolate_ii (
    unsigned char *dest, int dest_width, int dest_height, int dest_pitch,
    unsigned char *src,  int src_width,  int src_height,  int src_pitch)
{
    /* scaled row/column counters, limits, & increment values: */
    int src_x, src_y;
    int src_x_inc = ((src_width << TOTAL_SCALE_BITS) + dest_width / 2) / dest_width;
    int src_y_inc = ((src_height << TOTAL_SCALE_BITS) + dest_height / 2) / dest_height;
    int src_x_max = (src_width - 1) << TOTAL_SCALE_BITS;
    int src_y_max = (src_height - 1) << TOTAL_SCALE_BITS;
    int src_x_end = src_x_inc * dest_width;
    int src_y_end = src_y_inc * dest_height;
	int src_x_bgn = (src_width < dest_width)?(0):((src_x_inc  - (1 << TOTAL_SCALE_BITS)) >> 1);
	int src_y_bgn = (src_height < dest_height)?(0):((src_y_inc - (1 << TOTAL_SCALE_BITS)) >> 1);

	src_x_bgn = 0;
	src_y_bgn = 0;

    /* source/destination pointers: */
    unsigned char *s1, *s2, *d1 = dest;

    /* perform interpolation by inverse mapping from destination to source: */
    for (src_y = src_y_bgn; src_y < src_y_max; src_y += src_y_inc) {

        /* map row: */
        int src_y_i = src_y >> TOTAL_SCALE_BITS;
        int q       = (src_y >> (TOTAL_SCALE_BITS - COEF_SCALE_BITS)) & ((1U << COEF_SCALE_BITS) - 1);

        /* get source row pointers: */
        s1 = src + src_y_i * src_pitch;
        s2 = s1 + src_pitch;

        src_x = src_x_bgn;
        do {
            register int src_x_i, p;
            register unsigned int a, b, c;

            /* get first pixel: */
            src_x_i = src_x >> TOTAL_SCALE_BITS;
            p = (src_x >> (TOTAL_SCALE_BITS - COEF_SCALE_BITS)) & ((1U << COEF_SCALE_BITS) - 1);
            /* load 4 pixels in 2 registers: */
            a = (s2 [src_x_i] << 16) + s1 [src_x_i]; 
            c = (s2 [src_x_i + 1] << 16) + s1 [src_x_i + 1]; 
            /* perform 1st horisontal projection: */
            a = (a << COEF_SCALE_BITS) + p * (c - a);

            src_x  += src_x_inc;	/* 1! */

            /* get second pixel: */
            src_x_i = src_x >> TOTAL_SCALE_BITS;
            p = (src_x >> (TOTAL_SCALE_BITS - COEF_SCALE_BITS)) & ((1U << COEF_SCALE_BITS) - 1);
            /* load 4 pixels in 2 registers: */
            b = (s2 [src_x_i] << 16) + s1 [src_x_i]; 
            c = (s2 [src_x_i + 1] << 16) + s1 [src_x_i + 1]; 
            /* perform 2nd horisontal projection: */
            b = (b << COEF_SCALE_BITS) + p * (c - b);

            /* repack & perform vertical projection: */
            c = a;
            a = (a & 0xFFFF) + (b << 16);
            b = (b & 0xFFFF0000) + ((unsigned int)c >> 16);
            a = (a << COEF_SCALE_BITS) + q * (b - a);
            
            /* store pixels: */
            *d1++ = (unsigned int)a >> 2 * COEF_SCALE_BITS;
            *d1++ = (unsigned int)a >> (16 + 2 * COEF_SCALE_BITS);

            src_x  += src_x_inc;	/* 2! */

        } while (src_x < src_x_max);

        /* last pixels: */
        {
            int a = s1 [src_width - 1];
            int c = s2 [src_width - 1];
            a = (unsigned int)((a << COEF_SCALE_BITS) + q * (c - a)) >> COEF_SCALE_BITS;

            for (; src_x < src_x_end; src_x += src_x_inc)
                *d1++ = a;
        }

        d1 = (dest += dest_pitch);
    }

    /* last rows: */
    for (; src_y < src_y_end; src_y += src_y_inc) {

        s1 = src + (src_height-1) * src_pitch;

        for (src_x = 0; src_x < src_x_max; src_x += src_x_inc) {

            int src_x_i = src_x >> TOTAL_SCALE_BITS;
            int p       = (src_x >> (TOTAL_SCALE_BITS - COEF_SCALE_BITS)) & ((1U << COEF_SCALE_BITS) - 1);

            /* get four pixels: */
            int a = s1 [src_x_i];
            int b = s1 [src_x_i + 1];

            /* compute the interpolated pixel value: */
            a = (a << COEF_SCALE_BITS) + p * (b - a);

            *d1++ = (unsigned int)a >> COEF_SCALE_BITS;
        }

        /* last delta_x pixels: */
        for (; src_x < src_x_end; src_x += src_x_inc)
            *d1++ = s1 [src_width - 1];

        d1 = (dest += dest_pitch);
    }
}



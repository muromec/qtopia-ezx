/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: deintl.cpp,v 1.11 2005/05/05 16:15:08 albertofloyd Exp $
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


////////////////////////////////////////////////////////
//	defines
////////////////////////////////////////////////////////

// Defineing TIME_DEINTERLACE will time the deinterlacer
// and write a file with filename TIME_DEINTERLACE_FILENAME
// with the times.

//#define TIME_DEINTERLACE
//#define TIME_DEINTERLACE_FILENAME "d:\\dintl.log"

////////////////////////////////////////////////////////
//	include files
////////////////////////////////////////////////////////

#include "hlxclib/stdlib.h"
#include "hlxclib/string.h"

#define INTL_I420_CODE

#ifdef TIME_DEINTERLACE
#include "hlxclib/stdio.h"
#endif

#include "mmx_util.h"
#include "deintl.h"


////////////////////////////////////////////////////////
//	internal prototypes
////////////////////////////////////////////////////////

void
Deinterlace_RGB24(unsigned char *frame, unsigned char *prev_frame, int pels, int lines, int pitch, int format);

void
Deinterlace_I420(unsigned char *frame, unsigned char *prev_frame, int pels, int lines, int pitch, int format);

void
Deinterlace_RGB24_Fast(unsigned char *frame, unsigned char *prev_frame, int pels, int lines, int pitch, int format);

void
Deinterlace_I420_Fast(unsigned char *frame, unsigned char *prev_frame, int pels, int lines, int pitch, int format);

void
Deinterlace_I420_Advanced(unsigned char *frame, unsigned char *prev_frame, int pels, int lines, int pitch, int format);

void
Deinterlace_I420_EdgeInterp(unsigned char *frame, unsigned char *prev_frame, int pels, int lines, int pitch, int format);

void
Deinterlace_RGB24_Field(unsigned char *frame, unsigned char *prev_frame, int pels, int lines, int pitch, int format, int field);

void
Deinterlace_I420_Field(unsigned char *frame, unsigned char *prev_frame, int pels, int lines, int pitch, int format, int field);

void
Deinterlace_I420_FlipFlop(unsigned char *frame, unsigned char *temp_frame, int pels, int lines, int pitch, int format);

#ifdef _M_IX86

void
Deinterlace_I420_MMX(unsigned char *frame, unsigned char *prev_frame, int pels, int lines, int pitch, int format);

#endif

int
C_DetectInterlace_I420(unsigned char *frame, unsigned char *prev_frame, int first_frame, int pels, int lines, int pitch);


////////////////////////////////////////////////////////
//	macros
////////////////////////////////////////////////////////

#define MEDIAN_3(a,b,c)			\
	((a > b)?					\
		(						\
			(b > c)?			\
			(b):				\
			(					\
				(a > c)?		\
				(c):			\
				(a)				\
			)					\
		):						\
		(						\
			(a > c)?			\
			(a):				\
			(					\
				(b > c)?		\
				(c):			\
				(b)				\
			)					\
		))						\

#define MABS(v) (tmp=(v),((tmp)^(tmp>>31)))
#define ABS(a)  (((a) < 0) ? (-(a)) : (a))
#define DIFF_FCN(v) ((v)*(v))		// Squared difference

static const int SQUARED_TAB[255+255+1] = {
    65025,  64516,  64009,  63504,  63001,  62500,
    62001,  61504,  61009,  60516,  60025,  59536,  59049,  58564,  58081,  57600,
    57121,  56644,  56169,  55696,  55225,  54756,  54289,  53824,  53361,  52900,
    52441,  51984,  51529,  51076,  50625,  50176,  49729,  49284,  48841,  48400,
    47961,  47524,  47089,  46656,  46225,  45796,  45369,  44944,  44521,  44100,
    43681,  43264,  42849,  42436,  42025,  41616,  41209,  40804,  40401,  40000,
    39601,  39204,  38809,  38416,  38025,  37636,  37249,  36864,  36481,  36100,
    35721,  35344,  34969,  34596,  34225,  33856,  33489,  33124,  32761,  32400,
    32041,  31684,  31329,  30976,  30625,  30276,  29929,  29584,  29241,  28900,
    28561,  28224,  27889,  27556,  27225,  26896,  26569,  26244,  25921,  25600,
    25281,  24964,  24649,  24336,  24025,  23716,  23409,  23104,  22801,  22500,
    22201,  21904,  21609,  21316,  21025,  20736,  20449,  20164,  19881,  19600,
    19321,  19044,  18769,  18496,  18225,  17956,  17689,  17424,  17161,  16900,
    16641,  16384,  16129,  15876,  15625,  15376,  15129,  14884,  14641,  14400,
    14161,  13924,  13689,  13456,  13225,  12996,  12769,  12544,  12321,  12100,
    11881,  11664,  11449,  11236,  11025,  10816,  10609,  10404,  10201,  10000,
    9801,   9604,   9409,   9216,   9025,   8836,   8649,   8464,   8281,   8100,
    7921,   7744,   7569,   7396,   7225,   7056,   6889,   6724,   6561,   6400,
    6241,   6084,   5929,   5776,   5625,   5476,   5329,   5184,   5041,   4900,
    4761,   4624,   4489,   4356,   4225,   4096,   3969,   3844,   3721,   3600,
    3481,   3364,   3249,   3136,   3025,   2916,   2809,   2704,   2601,   2500,
    2401,   2304,   2209,   2116,   2025,   1936,   1849,   1764,   1681,   1600,
    1521,   1444,   1369,   1296,   1225,   1156,   1089,   1024,   961,    900,
    841,    784,    729,    676,    625,    576,    529,    484,    441,    400,
    361,    324,    289,    256,    225,    196,    169,    144,    121,    100,
    81,     64,     49,     36,     25,     16,     9,      4,      1,      0,
    1,      4,      9,      16,     25,     36,     49,     64,     81,     100,
    121,    144,    169,    196,    225,    256,    289,    324,    361,    400,
    441,    484,    529,    576,    625,    676,    729,    784,    841,    900,
    961,    1024,   1089,   1156,   1225,   1296,   1369,   1444,   1521,   1600,
    1681,   1764,   1849,   1936,   2025,   2116,   2209,   2304,   2401,   2500,
    2601,   2704,   2809,   2916,   3025,   3136,   3249,   3364,   3481,   3600,
    3721,   3844,   3969,   4096,   4225,   4356,   4489,   4624,   4761,   4900,
    5041,   5184,   5329,   5476,   5625,   5776,   5929,   6084,   6241,   6400,
    6561,   6724,   6889,   7056,   7225,   7396,   7569,   7744,   7921,   8100,
    8281,   8464,   8649,   8836,   9025,   9216,   9409,   9604,   9801,   10000,
    10201,  10404,  10609,  10816,  11025,  11236,  11449,  11664,  11881,  12100,
    12321,  12544,  12769,  12996,  13225,  13456,  13689,  13924,  14161,  14400,
    14641,  14884,  15129,  15376,  15625,  15876,  16129,  16384,  16641,  16900,
    17161,  17424,  17689,  17956,  18225,  18496,  18769,  19044,  19321,  19600,
    19881,  20164,  20449,  20736,  21025,  21316,  21609,  21904,  22201,  22500,
    22801,  23104,  23409,  23716,  24025,  24336,  24649,  24964,  25281,  25600,
    25921,  26244,  26569,  26896,  27225,  27556,  27889,  28224,  28561,  28900,
    29241,  29584,  29929,  30276,  30625,  30976,  31329,  31684,  32041,  32400,
    32761,  33124,  33489,  33856,  34225,  34596,  34969,  35344,  35721,  36100,
    36481,  36864,  37249,  37636,  38025,  38416,  38809,  39204,  39601,  40000,
    40401,  40804,  41209,  41616,  42025,  42436,  42849,  43264,  43681,  44100,
    44521,  44944,  45369,  45796,  46225,  46656,  47089,  47524,  47961,  48400,
    48841,  49284,  49729,  50176,  50625,  51076,  51529,  51984,  52441,  52900,
    53361,  53824,  54289,  54756,  55225,  55696,  56169,  56644,  57121,  57600,
    58081,  58564,  59049,  59536,  60025,  60516,  61009,  61504,  62001,  62500,
    63001,  63504,  64009,  64516,  65025
};

#define MSQUARED(v) (SQUARED_TAB[(v) + 255])

// Some thresholds
#define INTL_DIFF_THRESH	40000
#define INTL_HORIZ_THRESH	80

#ifdef TIME_DEINTERLACE
static unsigned int num_avg = 0;
static unsigned int num_med = 0;
#endif


int
InitDeinterlace (
	int pels, int lines, int pitch, 
	int format, 
	INTL_MODE mode, 
	T_DEINTL_STATE **state)
{
	T_DEINTL_STATE *new_state = 0;

	if (*state == 0)
	{
		new_state = (T_DEINTL_STATE *)malloc(sizeof(T_DEINTL_STATE));
	}

	if (new_state == 0)
		return 1;

	new_state->pels = pels;
	new_state->lines = lines;
	new_state->pitch = pitch;
	new_state->format = format;
	new_state->detection_measure = (lines > 242)?(16):(0);
	new_state->commit_deinterlace = FALSE;

	*state = new_state;

	return 0;
}

void
FreeDeinterlace (T_DEINTL_STATE **state)
{
	if (*state != 0)
		free(*state);
	*state = 0;
}

HXBOOL
IsContentInterlaced (T_DEINTL_STATE *state)
{
	if (state != 0)
		return (state->commit_deinterlace == 1)?(TRUE):(FALSE);

	return FALSE;
}

void
ResetDeinterlace (T_DEINTL_STATE *state)
{
	state->detection_measure = 0;
	state->commit_deinterlace = FALSE;
}


////////////////////////////////////////////////////////
//
//	Deinterlace
//
//	Top level function to deinterlace a video frame
//
//	Parameters:
//		frame:		Pointer to the frame to deinterlace
//		prev_frame:	Pointer to the previous frame.
//					Better results are achieved by giving
//					the previous *interlaced* frame
//		pels,
//		lines,
//		pitch:		Frame dimensions
//		format:		INTL_FORMAT_RGB24 or INTL_FORMAT_I420
//		mode:		Choose either using both fields or
//					to remove one field entirely
//					(see 'deintl.h')
//
////////////////////////////////////////////////////////

int
Deinterlace(
	unsigned char *frame, 
	unsigned char *prev_frame,
	int pels, int lines, int pitch,
	int format,
	int first_frame, 
	INTL_MODE mode,
	T_DEINTL_STATE *state)
{
	if (pels == 0)
		pels = state->pels;
	if (lines == 0)
		lines = state->lines;
	if (pitch == 0)
		pitch = state->pitch;
	if (format == 0)
		format = state->format;

	unsigned char *pfp = (first_frame == TRUE)?(NULL):(prev_frame);

	// Unless were going to do detection first (i.e. in INTL_MODE_SMART_AUTO mode),
	// don't de-interlace when there is less than 242 lines.  This is a little
	// check to prevent unintentional de-interlacing of probable progressive frames.
	if (lines < 242 && mode != INTL_MODE_SMART_AUTO)
		return 0;

	// Run detection
	if (mode == INTL_MODE_SMART_AUTO)
	{
		unsigned char *detection_frame = NULL;
		unsigned char *detection_prev_frame = NULL;
		int res;

		if (format == INTL_FORMAT_I420)
		{
			// point to the Y-Plane
			detection_frame = frame;
			detection_prev_frame = prev_frame;
		}
		if (format == INTL_FORMAT_RGB24)
		{
			// point to the G-Plane
			detection_frame = frame + pels * lines;
			detection_prev_frame = prev_frame + pels * lines;
		}

		// Perform detection
		res = C_DetectInterlace_I420(
			detection_frame, 
			detection_prev_frame, 
			first_frame, 
			pels, lines, pitch);

		// Update the "detection measure" based on result of detection
		switch (res)
		{
		case INTL_STRONG_INTERLACE:
			{
				state->detection_measure = (31 * state->detection_measure + 256) >> 5;
			}
			break;

		case INTL_WEAK_INTERLACE:
			{
				state->detection_measure = (31 * state->detection_measure + 256) >> 5;
			}
			break;

		case INTL_WEAK_PROGRESSIVE:
			{
				state->detection_measure = (31 * state->detection_measure + 0) >> 5;
			}
			break;

		case INTL_STRONG_PROGRESSIVE:
			{
				state->detection_measure = 0;
			}
			break;
		}

		// If the detection measure is above a threshold, set this flag used
		// to indicate we are confident the content is interlaced.
		if (state->detection_measure > 128)
		{
			state->commit_deinterlace = TRUE;
		}

		// If the detection measure is above this threshold, we should
		// de-interlace this frame. Otherwise, return without de-interlacing
		if (state->detection_measure > 16)
		{
			mode = INTL_MODE_SMART;
		}
		else
		{
			return 0;
		}
	}

#ifdef TIME_DEINTERLACE
	FILE *fp = NULL;
	double proc_time;

	USE_CODEC_TIMER;

	num_avg = 0;
	num_med = 0;

	START_CODEC_TIMER;
#endif

	switch (format)
	{

#ifdef INTL_RBG24_CODE
	case INTL_FORMAT_RGB24:
		Deinterlace_RGB24_Fast(frame, pfp, pels, lines, pitch, format);
		break;
#endif

#ifdef INTL_I420_CODE

	case INTL_FORMAT_I420:

#ifdef USE_MMX_DEINTERLACING		//_M_IX86
        if (checkMmxAvailablity() & CPU_HAS_MMX)
			Deinterlace_I420_MMX(frame, pfp, pels, lines, pitch, format);
		else
#endif
		// Neelesh's new deinterlacer.
		Deinterlace_I420_Advanced(frame, pfp, pels, lines, pitch, format);

		break;

#endif
	}

#ifdef TIME_DEINTERLACE
	STOP_CODEC_TIMER(proc_time);
	
	fp = fopen(TIME_DEINTERLACE_FILENAME,"a+");
	if (fp != NULL)
	{
		fprintf(fp,"deinterlace time\t%f\t%d\t%d\n",proc_time,num_avg,num_med);
		fclose(fp);
	}
#endif

	return 1;
}


////////////////////////////////////////////////////////
//
//	C_DetectInterlace_I420
//
//	Top level function to detect the presense of 
//	interlaced video content
//
//	Parameters:
//		frame:		Pointer to the frame to deinterlace
//		prev_frame:	Pointer to the previous frame.
//		pels,
//		lines,
//		pitch:		Frame dimensions
//
////////////////////////////////////////////////////////

// Threshold: If 8x8 SAD is above MOVEMENT_THRESH then
// we'll decide that the block is in motion
#define MOVEMENT_THRESH (400)

// This is the factor by which 8x8 are skipped (not checked).
// SKIP_FACTOR = 1 means no skipping.
#define SKIP_FACTOR 4

#ifdef DEBUG
// This increments with each call, 
// and is useful for debugging a specific frame.
static unsigned int call_count = 0;
#endif

int
C_DetectInterlace_I420(
	unsigned char *frame, 
	unsigned char *prev_frame, 
	int first_frame, 
	int pels, int lines, int pitch)
{
	unsigned int image_size = (unsigned int)(pels * lines);
		// pre-calculated image size used for determining thresholds

	unsigned int interlaced_blocks = 0;			
		// count of the number of 8x8 blocks that look to be interlaced

	unsigned int progressive_blocks = 0;
		// count of the number of 8x8 blocks that look to be progressive

	unsigned int step1v_tot = 0;
		// running total of the 1-step squared vertical difference of pixels

	unsigned int step2v_tot = 0;
		// running total of the 2-step squared vertical difference of pixels

	unsigned int step1h_tot = 0;
		// running total of the 1-step squared horizontal difference of pixels

	unsigned int step2h_tot = 0;
		// running total of the 2-step squared horizontal difference of pixels

	unsigned char *fp, *pp;
	unsigned char *f0, *f1, *f2, *f3;
		// various temporary frame pointers

	int i, j, k;	// loop counter	
	int tmp;		// intermediate values for correlation and SAD macro
	int d0, d1;		// SAD counters

#ifdef DEBUG
	call_count++;
#endif

	// We have no detection scheme for the first frame yet.
	if (first_frame)
	{
		return INTL_NO_DETECTION;
	}

	// Loop through tiled 8x8 blocks of the image
	for (i = 8; i < lines - 8; i += 8)
	{
		// Start pointer at new line
		fp = frame + i * pitch + (i & (8*(SKIP_FACTOR - 1))) + 8;
		pp = prev_frame + i * pitch + (i &  (8*(SKIP_FACTOR - 1))) + 8;

		for (j = 8; j < pels - 8; j += 8 * SKIP_FACTOR)
		{
			f0 = fp;
			f1 = f0 + pitch;
			f2 = pp;
			f3 = f2 + pitch;

			d0 = d1 = 0;

			// Calculate SAD for even and odd lines of 8x8 block.
			// We're doing a partial SAD here for speed.
			for (k = 0; k < 4; k++)
			{
				d0 += MABS(f0[0] - f2[0]);
				d0 += MABS(f0[2] - f2[2]);
				d0 += MABS(f0[4] - f2[4]);
				d0 += MABS(f0[6] - f2[6]);
				d1 += MABS(f1[1] - f3[1]);
				d1 += MABS(f1[3] - f3[3]);
				d1 += MABS(f1[5] - f3[5]);
				d1 += MABS(f1[7] - f3[7]);

				f0 += 2 * pitch;
				f1 += 2 * pitch;
				f2 += 2 * pitch;
				f3 += 2 * pitch;
			}
			d0 <<= 1;
			d1 <<= 1;
			
			// If there is enough difference to determine movement,
			// check this region for indications of interlaced artifacts
			if (d0 > MOVEMENT_THRESH || d1 > MOVEMENT_THRESH)
			{
				int step1v_corr = 0;
				int step2v_corr = 0;
				int step1h_corr = 0;
				int step2h_corr = 0;

				f0 = fp;
				f1 = f0 + pitch;
				f2 = f1 + pitch;

				// Determine 1 and 2 step pixel differences for 8x8 region
				for (k = 0; k < 4; k++)
				{
					tmp = f0[1] - f1[1];
					step1v_corr += MSQUARED(tmp);
					tmp = f0[1] - f2[1];
					step2v_corr += MSQUARED(tmp);
					tmp = f1[0] - f1[1];
					step1h_corr += MSQUARED(tmp);
					tmp = f1[0] - f1[2];
					step2h_corr += MSQUARED(tmp);

					tmp = f0[3] - f1[3];
					step1v_corr += MSQUARED(tmp);
					tmp = f0[3] - f2[3];
					step2v_corr += MSQUARED(tmp);
					tmp = f1[2] - f1[3];
					step1h_corr += MSQUARED(tmp);
					tmp = f1[2] - f1[4];
					step2h_corr += MSQUARED(tmp);

					tmp = f0[5] - f1[5];
					step1v_corr += MSQUARED(tmp);
					tmp = f0[5] - f2[5];
					step2v_corr += MSQUARED(tmp);
					tmp = f1[4] - f1[5];
					step1h_corr += MSQUARED(tmp);
					tmp = f1[4] - f1[6];
					step2h_corr += MSQUARED(tmp);

					tmp = f0[7] - f1[7];
					step1v_corr += MSQUARED(tmp);
					tmp = f0[7] - f2[7];
					step2v_corr += MSQUARED(tmp);
					tmp = f1[6] - f1[7];
					step1h_corr += MSQUARED(tmp);
					tmp = f1[6] - f1[8];
					step2h_corr += MSQUARED(tmp);

					f0 += 2 * pitch;
					f1 += 2 * pitch;
					f2 += 2 * pitch;
				}

				// These conditions indicate the block 
				// is very likely interlaced.
				if (step1v_corr > step2v_corr && 
					step1h_corr < (40*40*4*4) &&
					step2v_corr < (40*40*4*4))
				{
					interlaced_blocks++;
				}

				// These conditions indicate the block 
				// is very likely progressive.
				if (step1v_corr < 2 * step2v_corr && 
					step1h_corr < (40*40*4*4) &&
					step1v_corr < (40*40*4*4))
				{
					progressive_blocks++;
				}

				// Maintain totals
				step1v_tot += step1v_corr;
				step2v_tot += step2v_corr;
				step1h_tot += step1h_corr;
				step2h_tot += step2h_corr;
			}

			// Jump to the next 8x8 block
			fp += 8 * SKIP_FACTOR;
			pp += 8 * SKIP_FACTOR;
		}
	}

#if (SKIP_FACTOR != 1)
	// If we're skipping some 8x8 block, 
	// acount for this by multiplying by skip factor

	interlaced_blocks *= SKIP_FACTOR;
	progressive_blocks *= SKIP_FACTOR;

	step1v_tot *= SKIP_FACTOR;
	step2v_tot *= SKIP_FACTOR;
	step1h_tot *= SKIP_FACTOR;
	step2h_tot *= SKIP_FACTOR;

#endif

	// Scale these values. 
	// (Done to prevent overflow during later multiplies)
	step1v_tot /= image_size;
	step2v_tot /= image_size;
	step1h_tot /= image_size;
	step2h_tot /= image_size;

	// Interlaced Tests
	if (interlaced_blocks > progressive_blocks &&
		interlaced_blocks > (image_size >> 12))
	{
		return INTL_WEAK_INTERLACE;
	}

	if (step1v_tot * step2h_tot > step1h_tot * step2v_tot &&
		step1v_tot > step2v_tot &&
		2 * interlaced_blocks > progressive_blocks &&
		interlaced_blocks > (image_size >> 10))
	{
		return INTL_WEAK_INTERLACE;
	}

	// Progressive Tests
	if (progressive_blocks > (image_size >> 10) &&
		progressive_blocks > (interlaced_blocks << 4))
	{
		return INTL_WEAK_PROGRESSIVE;
	}

	return INTL_NO_DETECTION;

}

////////////////////////////////////////////////////////
//
//	Deinterlace_RGB24_Fast
//
//	Fast deinterlacing of RGB24 data, using both fields
//
////////////////////////////////////////////////////////

void
Deinterlace_RGB24_Fast(unsigned char *frame, unsigned char *prev_frame, int pels, int lines, int pitch, int format)
{
	int line, pel;
	unsigned char *fp1,*pfp1;
	unsigned char *fp2,*pfp2;
	unsigned char *fp3,*pfp3;

	unsigned char *fpr;
	unsigned char *fpb;

	// Only handle RGB for now...
	if (format != INTL_FORMAT_RGB24)
		return;

	for (line = 1; line < lines - 2; line += 2)
	{
		unsigned int a,b,c,d,e,f;
		int c_nw,c_ne,c_se,c_sw;
		int diff,tmp;
		int prev_mode = INTL_LINE_REMOVE_AVG;
		int mode = INTL_LINE_REMOVE_AVG;
		int next_mode = INTL_LINE_REMOVE_AVG;

		// current frame
		fpr = frame + line * pitch;
		fp2 = fpr + pels * lines;
		fp1 = fp2 - pitch;
		fp3 = fp2 + pitch;
		fpb = fp2 + pels * lines;

		// previous frame
		if (prev_frame != 0)
		{
			pfp2 = prev_frame + pels * lines + line * pitch;
			pfp1 = pfp2 - pitch;
			pfp3 = pfp2 + pitch;
		}
		else
		{
			pfp2 = fp2;
			pfp1 = fp1;
			pfp3 = fp3;
		}

		// initialize
		for (pel = 0; pel < pels - 4; pel += 4)
		{
			// Load *next* 4 pels
			a = ((unsigned int *)fp1)[1];
			b = ((unsigned int *)fp2)[1];
			c = ((unsigned int *)fp3)[1];

			// Get corners
			c_nw = (a >> 24);
			c_ne = (a & 0xff);
			c_sw = (c >> 24);
			c_se = (c & 0xff);

			// Edge detect
			tmp = c_nw + c_ne - c_sw - c_se;
			if ((tmp < 100) && (tmp > -100))
			{
				next_mode = INTL_LINE_REMOVE_AVG;
				goto proc;
			}

			tmp = c_ne + c_se - c_nw - c_sw;
			if (tmp > 100)
			{
				next_mode = INTL_LINE_REMOVE_AVG;
				goto proc;
			}
			if (tmp < -100)
			{
				next_mode = INTL_LINE_REMOVE_AVG;
				goto proc;
			}

			// Load previous pels
			d = ((unsigned int *)pfp1)[1];
			e = ((unsigned int *)pfp2)[1];
			f = ((unsigned int *)pfp3)[1];

			// Diff with previous pels
			tmp = c_nw;
			tmp -= (d >> 24);
			diff = tmp ^ (tmp >> 31);

			tmp = c_ne;
			tmp -= (d & 0xff);
			diff += tmp ^ (tmp >> 31);

			if (diff > 100)
			{
				next_mode = INTL_LINE_REMOVE_AVG;
				goto proc;
			}

			tmp = c_sw;
			tmp -= (f >> 24);
			diff += tmp ^ (tmp >> 31);

			tmp = c_se;
			tmp -= (f & 0xff);
			diff += tmp ^ (tmp >> 31);

			if (diff > 200)
			{
				next_mode = INTL_LINE_REMOVE_AVG;
				goto proc;
			}

			tmp = (b >> 24);
			tmp -= (e >> 24);
			diff += tmp ^ (tmp >> 31);
			tmp = (b & 0xff);
			tmp -= (e & 0xff);
			diff += tmp ^ (tmp >> 31);

			if (diff > 300)
			{
				next_mode = INTL_LINE_REMOVE_AVG;
				goto proc;
			}

			next_mode = INTL_LINE_REMOVE_MEDIAN;

proc:
			if (mode == INTL_LINE_REMOVE_MEDIAN || prev_mode == INTL_LINE_REMOVE_MEDIAN || next_mode == INTL_LINE_REMOVE_MEDIAN)
			{
				// median
				fp2[0] = MEDIAN_3(fp1[0],fp2[0],fp3[0]);
				fp2[1] = MEDIAN_3(fp1[1],fp2[1],fp3[1]);
				fp2[2] = MEDIAN_3(fp1[2],fp2[2],fp3[2]);
				fp2[3] = MEDIAN_3(fp1[3],fp2[3],fp3[3]);

				fpr[0] = MEDIAN_3(fpr[-pitch],fpr[0],fpr[pitch]);
				fpr[1] = MEDIAN_3(fpr[-pitch+1],fpr[1],fpr[pitch+1]);
				fpr[2] = MEDIAN_3(fpr[-pitch+2],fpr[2],fpr[pitch+2]);
				fpr[3] = MEDIAN_3(fpr[-pitch+3],fpr[3],fpr[pitch+3]);

				fpb[0] = MEDIAN_3(fpb[-pitch],fpb[0],fpb[pitch]);
				fpb[1] = MEDIAN_3(fpb[-pitch+1],fpb[1],fpb[pitch+1]);
				fpb[2] = MEDIAN_3(fpb[-pitch+2],fpb[2],fpb[pitch+2]);
				fpb[3] = MEDIAN_3(fpb[-pitch+3],fpb[3],fpb[pitch+3]);

#ifdef TIME_DEINTERLACE
				num_med++;
#endif

			}
			else
			{
				// average
				a = ((unsigned int *)fp1)[0];
				c = ((unsigned int *)fp3)[0];
				((unsigned int*)fp2)[0] = (((a^c)>>1) & 0x7F7F7F7F) + (a&c);

				a = ((unsigned int *)(fpr - pitch))[0];
				c = ((unsigned int *)(fpr + pitch))[0];
				((unsigned int*)fpr)[0] = (((a^c)>>1) & 0x7F7F7F7F) + (a&c);

				a = ((unsigned int *)(fpb - pitch))[0];
				c = ((unsigned int *)(fpb + pitch))[0];
				((unsigned int*)fpb)[0] = (((a^c)>>1) & 0x7F7F7F7F) + (a&c);

#ifdef TIME_DEINTERLACE
				num_avg++;
#endif
			}

			prev_mode = mode;
			mode = next_mode;

			fp1 += 4;
			fp2 += 4;
			fp3 += 4;

			pfp1 += 4;
			pfp2 += 4;
			pfp3 += 4;

			fpr += 4;
			fpb += 4;
		}

		// last 4 pels
		if (mode == INTL_LINE_REMOVE_MEDIAN || prev_mode == INTL_LINE_REMOVE_MEDIAN)
		{
			// median
			fp2[0] = MEDIAN_3(fp1[0],fp2[0],fp3[0]);
			fp2[1] = MEDIAN_3(fp1[1],fp2[1],fp3[1]);
			fp2[2] = MEDIAN_3(fp1[2],fp2[2],fp3[2]);
			fp2[3] = MEDIAN_3(fp1[3],fp2[3],fp3[3]);

			fpr[0] = MEDIAN_3(fpr[-pitch],fpr[0],fpr[pitch]);
			fpr[1] = MEDIAN_3(fpr[-pitch+1],fpr[1],fpr[pitch+1]);
			fpr[2] = MEDIAN_3(fpr[-pitch+2],fpr[2],fpr[pitch+2]);
			fpr[3] = MEDIAN_3(fpr[-pitch+3],fpr[3],fpr[pitch+3]);

			fpb[0] = MEDIAN_3(fpb[-pitch],fpb[0],fpb[pitch]);
			fpb[1] = MEDIAN_3(fpb[-pitch+1],fpb[1],fpb[pitch+1]);
			fpb[2] = MEDIAN_3(fpb[-pitch+2],fpb[2],fpb[pitch+2]);
			fpb[3] = MEDIAN_3(fpb[-pitch+3],fpb[3],fpb[pitch+3]);

#ifdef TIME_DEINTERLACE
			num_med++;
#endif

		}
		else
		{
			// average
			a = ((unsigned int *)fp1)[0];
			c = ((unsigned int *)fp3)[0];
			((unsigned int*)fp2)[0] = (((a^c)>>1) & 0x7F7F7F7F) + (a&c);

			a = ((unsigned int *)(fpr - pitch))[0];
			c = ((unsigned int *)(fpr + pitch))[0];
			((unsigned int*)fpr)[0] = (((a^c)>>1) & 0x7F7F7F7F) + (a&c);

			a = ((unsigned int *)(fpb - pitch))[0];
			c = ((unsigned int *)(fpb + pitch))[0];
			((unsigned int*)fpb)[0] = (((a^c)>>1) & 0x7F7F7F7F) + (a&c);

#ifdef TIME_DEINTERLACE
			num_avg++;
#endif
		}
	}

}


////////////////////////////////////////////////////////
//
//	Deinterlace_RGB24
//
//	Slow deinterlacing of RGB24 data, using both fields
//
////////////////////////////////////////////////////////

void
Deinterlace_RGB24(unsigned char *frame, unsigned char *prev_frame, int pels, int lines, int pitch, int format)
{
	int line, pel;
	unsigned char *fp1,*pfp1;
	unsigned char *fp2,*pfp2;
	unsigned char *fp3,*pfp3;
	int tmp;

	// Only handle RGB for now...
	if (format != INTL_FORMAT_RGB24)
		return;

	for (line = 1; line < lines - 2; line += 2)
	{
		int a,b,c,d,e;
		int mode = INTL_LINE_REMOVE_AVG;

		fp1 = frame + line * pitch;
		fp2 = frame + pels*lines + line * pitch;
		fp3 = frame + 2 * pels*lines + line * pitch;

		if (prev_frame != 0)
		{
			pfp1 = prev_frame + line * pitch;
			pfp2 = prev_frame + pels*lines + line * pitch;
			pfp3 = prev_frame + 2 * pels*lines + line * pitch;
		}
		else
		{
			pfp1 = fp1;
			pfp2 = fp2;
			pfp3 = fp3;
		}

		// initialize
		tmp = (int)(fp1[-pitch]) + (int)(fp2[-pitch]) + (int)(fp3[-pitch]) - (int)(pfp1[-pitch]) - (int)(pfp2[-pitch]) - (int)(pfp3[-pitch]);
		a = DIFF_FCN(tmp);
		tmp = (int)(fp1[0]) + (int)(fp2[0]) + (int)(fp3[0]) - (int)(pfp1[0]) - (int)(pfp2[0]) - (int)(pfp3[0]);
		a += DIFF_FCN(tmp);
		tmp = (int)(fp1[pitch]) + (int)(fp2[pitch]) + (int)(fp3[pitch]) - (int)(pfp1[pitch]) - (int)(pfp2[pitch]) - (int)(pfp3[pitch]);
		a += DIFF_FCN(tmp);

		fp1++;pfp1++;
		fp2++;pfp2++;
		fp3++;pfp3++;

		tmp = (int)(fp1[-pitch]) + (int)(fp2[-pitch]) + (int)(fp3[-pitch]) - (int)(pfp1[-pitch]) - (int)(pfp2[-pitch]) - (int)(pfp3[-pitch]);
		b = DIFF_FCN(tmp);
		tmp = (int)(fp1[0]) + (int)(fp2[0]) + (int)(fp3[0]) - (int)(pfp1[0]) - (int)(pfp2[0]) - (int)(pfp3[0]);
		b += DIFF_FCN(tmp);
		tmp = (int)(fp1[pitch]) + (int)(fp2[pitch]) + (int)(fp3[pitch]) - (int)(pfp1[pitch]) - (int)(pfp2[pitch]) - (int)(pfp3[pitch]);
		b += DIFF_FCN(tmp);

		fp1++;pfp1++;
		fp2++;pfp2++;
		fp3++;pfp3++;

		tmp = (int)(fp1[-pitch]) + (int)(fp2[-pitch]) + (int)(fp3[-pitch]) - (int)(pfp1[-pitch]) - (int)(pfp2[-pitch]) - (int)(pfp3[-pitch]);
		c = DIFF_FCN(tmp);
		tmp = (int)(fp1[0]) + (int)(fp2[0]) + (int)(fp3[0]) - (int)(pfp1[0]) - (int)(pfp2[0]) - (int)(pfp3[0]);
		c += DIFF_FCN(tmp);
		tmp = (int)(fp1[pitch]) + (int)(fp2[pitch]) + (int)(fp3[pitch]) - (int)(pfp1[pitch]) - (int)(pfp2[pitch]) - (int)(pfp3[pitch]);
		c += DIFF_FCN(tmp);

		tmp = (int)(fp1[-pitch+1]) + (int)(fp2[-pitch+1]) + (int)(fp3[-pitch+1]) - (int)(pfp1[-pitch+1]) - (int)(pfp2[-pitch+1]) - (int)(pfp3[-pitch+1]);
		d = DIFF_FCN(tmp);
		tmp = (int)(fp1[1]) + (int)(fp2[1]) + (int)(fp3[1]) - (int)(pfp1[1]) - (int)(pfp2[1]) - (int)(pfp3[1]);
		d += DIFF_FCN(tmp);
		tmp = (int)(fp1[pitch+1]) + (int)(fp2[pitch+1]) + (int)(fp3[pitch+1]) - (int)(pfp1[pitch+1]) - (int)(pfp2[pitch+1]) - (int)(pfp3[pitch+1]);
		d += DIFF_FCN(tmp);

		for (pel = 2; pel < pels - 2; pel++)
		{
			tmp = (int)(fp1[-pitch+2]) + (int)(fp2[-pitch+2]) + (int)(fp3[-pitch+2]) - (int)(pfp1[-pitch+2]) - (int)(pfp2[-pitch+2]) - (int)(pfp3[-pitch+2]);
			e = DIFF_FCN(tmp);
			tmp = (int)(fp1[2]) + (int)(fp2[2]) + (int)(fp3[2]) - (int)(pfp1[2]) - (int)(pfp2[2]) - (int)(pfp3[2]);
			e += DIFF_FCN(tmp);
			tmp = (int)(fp1[pitch+2]) + (int)(fp2[pitch+2]) + (int)(fp3[pitch+2]) - (int)(pfp1[pitch+2]) - (int)(pfp2[pitch+2]) - (int)(pfp3[pitch+2]);
			e += DIFF_FCN(tmp);

			if (mode == INTL_LINE_REMOVE_MEDIAN)
			{
				if (a + b + c + d + e > INTL_DIFF_THRESH)
				{
					a =  MABS(fp2[-pitch-2] - fp2[-pitch+1]);
					a += MABS(fp2[pitch-2] - fp2[pitch+1]);
					a += MABS(fp2[-pitch-1] - fp2[-pitch+2]);
					a += MABS(fp2[pitch-1] - fp2[pitch+2]);
					a <<= 1;
					a -= MABS(fp2[-pitch-2] - fp2[pitch-2]);
					a -= MABS(fp2[-pitch-1] - fp2[pitch-1]);
					a -= MABS(fp2[-pitch+1] - fp2[pitch+1]);
					a -= MABS(fp2[-pitch+2] - fp2[pitch+2]);

					if (a > 0)
					{
						mode = INTL_LINE_REMOVE_AVG;
					}
				}
			}
			else
			{
				if (a + b + c + d + e < INTL_DIFF_THRESH)
				{
					a =  MABS(fp2[-pitch-2] - fp2[-pitch+1]);
					a += MABS(fp2[pitch-2] - fp2[pitch+1]);
					a += MABS(fp2[-pitch-1] - fp2[-pitch+2]);
					a += MABS(fp2[pitch-1] - fp2[pitch+2]);
					a <<= 1;
					a -= MABS(fp2[-pitch-2] - fp2[pitch-2]);
					a -= MABS(fp2[-pitch-1] - fp2[pitch-1]);
					a -= MABS(fp2[-pitch+1] - fp2[pitch+1]);
					a -= MABS(fp2[-pitch+2] - fp2[pitch+2]);

					if (a < 0)
					{
						mode = INTL_LINE_REMOVE_MEDIAN;
					}
				}
			}
				
			if (mode == INTL_LINE_REMOVE_MEDIAN)
			{
				// median
				fp1[0] = MEDIAN_3(fp1[-pitch],fp1[0],fp1[pitch]);
				fp2[0] = MEDIAN_3(fp2[-pitch],fp2[0],fp2[pitch]);
				fp3[0] = MEDIAN_3(fp3[-pitch],fp3[0],fp3[pitch]);
			}
			else
			{
				// average
				tmp = fp1[-pitch];
				tmp += fp1[pitch];
				fp1[0] = (tmp >> 1);
				tmp = fp2[-pitch];
				tmp += fp2[pitch];
				fp2[0] = (tmp >> 1);
				tmp = fp3[-pitch];
				tmp += fp3[pitch];
				fp3[0] = (tmp >> 1);
			}
				
			a = b;
			b = c;
			c = d;
			d = e;

			fp1++;pfp1++;
			fp2++;pfp2++;
			fp3++;pfp3++;

		}
	}
}


////////////////////////////////////////////////////////
//
//	Deinterlace_I420_Fast
//
//	Fast deinterlacing of I420 data, using both fields
//
////////////////////////////////////////////////////////

void
Deinterlace_I420_Fast(unsigned char *frame, unsigned char *prev_frame, int pels, int lines, int pitch, int format)
{
	int line, pel;
	unsigned char *fp1,*pfp1;
	unsigned char *fp2,*pfp2;
	unsigned char *fp3,*pfp3;

	// Only handle RGB for now...
	if (format != INTL_FORMAT_I420)
		return;

	for (line = 1; line < lines - 2; line += 2)
	{
		unsigned int a,b,c,d,e,f;
		int c_nw,c_ne,c_se,c_sw;
		int diff,tmp;
		int prev_mode = INTL_LINE_REMOVE_AVG;
		int mode = INTL_LINE_REMOVE_AVG;
		int next_mode = INTL_LINE_REMOVE_AVG;

		// current frame
		fp2 = frame + line * pitch;
		fp1 = fp2 - pitch;
		fp3 = fp2 + pitch;

		// previous frame
		if (prev_frame != 0)
		{
			pfp2 = prev_frame + line * pitch;
			pfp1 = pfp2 - pitch;
			pfp3 = pfp2 + pitch;
		}
		else
		{
			pfp2 = fp2;
			pfp1 = fp1;
			pfp3 = fp3;
		}

		// initialize
		for (pel = 0; pel < pels - 4; pel += 4)
		{
			// Load *next* 4 pels
			a = ((unsigned int *)fp1)[1];
			b = ((unsigned int *)fp2)[1];
			c = ((unsigned int *)fp3)[1];

			// Get corners
			c_nw = (a >> 24);
			c_ne = (a & 0xff);
			c_sw = (c >> 24);
			c_se = (c & 0xff);

			// Edge detect
			tmp = c_nw + c_ne - c_sw - c_se;
			if ((tmp < 100) && (tmp > -100))
			{
				next_mode = INTL_LINE_REMOVE_AVG;
				goto proc;
			}

			tmp = c_ne + c_se - c_nw - c_sw;
			if (tmp > 100)
			{
				next_mode = INTL_LINE_REMOVE_AVG;
				goto proc;
			}
			if (tmp < -100)
			{
				next_mode = INTL_LINE_REMOVE_AVG;
				goto proc;
			}

			// Load previous pels
			d = ((unsigned int *)pfp1)[1];
			e = ((unsigned int *)pfp2)[1];
			f = ((unsigned int *)pfp3)[1];

			// Diff with previous pels
			tmp = c_nw;
			tmp -= (d >> 24);
			diff = tmp ^ (tmp >> 31);

			tmp = c_ne;
			tmp -= (d & 0xff);
			diff += tmp ^ (tmp >> 31);

			if (diff > 100)
			{
				next_mode = INTL_LINE_REMOVE_AVG;
				goto proc;
			}

			tmp = c_sw;
			tmp -= (f >> 24);
			diff += tmp ^ (tmp >> 31);

			tmp = c_se;
			tmp -= (f & 0xff);
			diff += tmp ^ (tmp >> 31);

			if (diff > 200)
			{
				next_mode = INTL_LINE_REMOVE_AVG;
				goto proc;
			}

			tmp = (b >> 24);
			tmp -= (e >> 24);
			diff += tmp ^ (tmp >> 31);
			tmp = (b & 0xff);
			tmp -= (e & 0xff);
			diff += tmp ^ (tmp >> 31);

			if (diff > 300)
			{
				next_mode = INTL_LINE_REMOVE_AVG;
				goto proc;
			}

			next_mode = INTL_LINE_REMOVE_MEDIAN;

proc:
			if (mode == INTL_LINE_REMOVE_MEDIAN || prev_mode == INTL_LINE_REMOVE_MEDIAN || next_mode == INTL_LINE_REMOVE_MEDIAN)
			{
				// median
				fp2[0] = MEDIAN_3(fp1[0],fp2[0],fp3[0]);
				fp2[1] = MEDIAN_3(fp1[1],fp2[1],fp3[1]);
				fp2[2] = MEDIAN_3(fp1[2],fp2[2],fp3[2]);
				fp2[3] = MEDIAN_3(fp1[3],fp2[3],fp3[3]);

#ifdef TIME_DEINTERLACE
				num_med++;
#endif

			}
			else
			{
				// average
				a = ((unsigned int *)fp1)[0];
				c = ((unsigned int *)fp3)[0];
				((unsigned int*)fp2)[0] = (((a^c)>>1) & 0x7F7F7F7F) + (a&c);

#ifdef TIME_DEINTERLACE
				num_avg++;
#endif
			}

			prev_mode = mode;
			mode = next_mode;

			fp1 += 4;
			fp2 += 4;
			fp3 += 4;

			pfp1 += 4;
			pfp2 += 4;
			pfp3 += 4;
		}

		// last 4 pels
		if (mode == INTL_LINE_REMOVE_MEDIAN || prev_mode == INTL_LINE_REMOVE_MEDIAN)
		{
			// median
			fp2[0] = MEDIAN_3(fp1[0],fp2[0],fp3[0]);
			fp2[1] = MEDIAN_3(fp1[1],fp2[1],fp3[1]);
			fp2[2] = MEDIAN_3(fp1[2],fp2[2],fp3[2]);
			fp2[3] = MEDIAN_3(fp1[3],fp2[3],fp3[3]);

#ifdef TIME_DEINTERLACE
			num_med++;
#endif

		}
		else
		{
			// average
			a = ((unsigned int *)fp1)[0];
			c = ((unsigned int *)fp3)[0];
			((unsigned int*)fp2)[0] = (((a^c)>>1) & 0x7F7F7F7F) + (a&c);

#ifdef TIME_DEINTERLACE
			num_avg++;
#endif
		}
	}
}

////////////////////////////////////////////////////////
//
//	Deinterlace_I420_Advanced
//
//	Slow deinterlacing of I420 data, using Edge Interpolation
//
////////////////////////////////////////////////////////

void
Deinterlace_I420_Advanced(unsigned char *frame, unsigned char *prev_frame, int pels, int lines, int pitch, int format)
{
	int line, pel;
	unsigned char *fpy, *pfpy;
	unsigned char *fp1, *pfp1;
	unsigned char *fp2, *pfp2;
	unsigned char *fp3, *pfp3;
	
	int edge1, edge2, pedge1, pedge2, diff1, diff2;
	unsigned int a, c;

	int tmp, pitch2;
	pitch2= pitch<<1;

	for (line = 1; line < lines - 2; line += 2)
	{
	
		fpy = frame + line*pitch;				
		
		// current frame
		// current line
		fp2 = fpy;
		// prev line
		fp1 = fp2 - pitch;
		// next line
		fp3 = fp2 + pitch;
		// current frame

		// previous frame
		if (prev_frame != 0)
		{
			pfpy = prev_frame + line * pitch;
			pfp2 = pfpy;
			pfp1 = pfp2 - pitch;
			pfp3 = pfp2 + pitch;
		}
		else
		{			
			pfp2 = fp2;
			pfp1 = fp1;
			pfp3 = fp3;
		}
				
		// handle first pel
		tmp = (*fp1 + *fp3) >> 1;
		*fp2 = (unsigned char) tmp;
		fp1++;
		fp2++;
		fp3++;
		pfp1++;

		edge1 = (*fp1) + *(fp1+1) +*(fp1+2);
		edge2=  (*fp3) + *(fp3+1) +*(fp3+2);
				
		for (pel = 1; pel < pels-4; pel += 1)
		{					
			edge1+=*(fp1+3);
			edge2+=*(fp3+3);
			tmp = (edge1 - edge2);

			if( tmp > 160 || tmp < -160) {
				// horiz
				pfp3 = pfp1 + pitch2;
				pedge1= *pfp1 + *(pfp1+1) + *(pfp1+2) + *(pfp1+3);
				pedge2= *pfp3 + *(pfp3+1) + *(pfp3+2) + *(pfp3+3);				
				
				diff1 = (edge1 - pedge1);				
				if (diff1 < 0) diff1=-(diff1);
				//diff1 = ABS(diff1);
				diff2 = (edge2  - pedge2);
				if (diff2 < 0) diff2=-(diff2);
				//diff2 = ABS(diff2);

				if ((diff1 + diff2) < 200) {
					// not moving					
					*fp2 = MEDIAN_3(*fp1,*fp2,*fp3);
					*(fp2+1) = MEDIAN_3(*(fp1+1),*(fp2+1),*(fp3+1));
					
#ifdef TIME_DEINTERLACE
						num_med++;
#endif				
					edge1 -= *fp1;
					edge2 -= *fp3;					
					fp1++;
					fp2++;
					fp3++;
					pfp1++;					
					pel++;
					edge1+=*(fp1+3);
					edge2+=*(fp3+3);					
				} else {		
					tmp = *(fp2-1) + *fp1 + *fp3;	
					tmp *= 21845;
					*fp2 = 	(unsigned char) (tmp >> 16);
					//tmp = (*(fp2-1)<<1) + *fp1 + *fp3;	
					//*fp2 = 	(unsigned char) (tmp >> 2);
#ifdef TIME_DEINTERLACE
						num_avg++;
#endif									
				}	
			} else {
				// not horiz				
				// average
				tmp = (*fp1 + *fp3) >> 1;
				*fp2 = (unsigned char) tmp;
				
#ifdef TIME_DEINTERLACE
					num_avg++;
#endif				
			}			
			edge1 -= (*fp1);
			edge2 -= (*fp3);					
			fp1++;
			fp2++;
			fp3++;
			pfp1++;			
		}
		// handle last four pels
		// last 4 pels
		pfp3 = pfp1 + pitch2;
		pedge1= *pfp1 + *(pfp1+1) + *(pfp1+2) + *(pfp1+3);
		pedge2= *pfp3 + *(pfp3+1) + *(pfp3+2) + *(pfp3+3);		
			
		diff1 = (edge1 - pedge1);
		diff1 = ABS(diff1);
		diff2 = (edge2  - pedge2);
		diff2 = ABS(diff2);
		if ((diff1 + diff2) < 200) {
					// not moving					
					// median
				fp2[0] = MEDIAN_3(fp1[0],fp2[0],fp3[0]);
				fp2[1] = MEDIAN_3(fp1[1],fp2[1],fp3[1]);
				fp2[2] = MEDIAN_3(fp1[2],fp2[2],fp3[2]);
				fp2[3] = MEDIAN_3(fp1[3],fp2[3],fp3[3]);

#ifdef TIME_DEINTERLACE
					num_med++;
#endif
		}
		else
		{
			// average
			a = ((unsigned int *)fp1)[0];
			c = ((unsigned int *)fp3)[0];
			((unsigned int*)fp2)[0] = (((a^c)>>1) & 0x7F7F7F7F) + (a&c);

#ifdef TIME_DEINTERLACE
				num_avg++;
#endif
		}
	}
	
}	


////////////////////////////////////////////////////////
//
//	Deinterlace_I420
//
//	Slow deinterlacing of I420 data, using both fields
//
////////////////////////////////////////////////////////

void
Deinterlace_I420(unsigned char *frame, unsigned char *prev_frame, int pels, int lines, int pitch, int format)
{
	int line, pel;
	unsigned char *fp1=NULL,*pfp1=NULL;			// line above
	unsigned char *fp2=NULL,*pfp2=NULL;			// current line
	unsigned char *fp3=NULL,*pfp3=NULL;			// line below
	int tmp;

	// Only handle RGB for now...
	if (format != INTL_FORMAT_I420)
		return;

	for (line = 1; line < lines - 2; line += 2)
	{
		int a,b,c,d,e;
		int mode = INTL_LINE_REMOVE_AVG;

		// set pointers
		fp1 = frame + (line - 1) * pitch;
		fp2 = fp1 + pitch;
		fp3 = fp2 + pitch;

		if (prev_frame != 0)
		{
			pfp2 = prev_frame + line * pitch;
			pfp1 = pfp1 - pitch;
			pfp3 = pfp2 + pitch;
		}
		else
		{
			pfp2 = fp2;
			pfp1 = fp1;
			pfp3 = fp3;
		}

		// initialize
		// a
		tmp = (int)(*fp1++);
		tmp -= (int)(*pfp1++);
		a = DIFF_FCN(tmp);
		tmp = (int)(*fp2++);
		tmp -= (int)(*pfp2++);
		a += DIFF_FCN(tmp);
		tmp = (int)(*fp3++);
		tmp -= (int)(*pfp3++);
		a += DIFF_FCN(tmp);

		// b
		tmp = (int)(*fp1++);
		tmp -= (int)(*pfp1++);
		b = DIFF_FCN(tmp);
		tmp = (int)(*fp2++);
		tmp -= (int)(*pfp2++);
		b += DIFF_FCN(tmp);
		tmp = (int)(*fp3++);
		tmp -= (int)(*pfp3++);
		b += DIFF_FCN(tmp);

		// c
		tmp = (int)(*fp1);
		tmp -= (int)(*pfp1);
		c = DIFF_FCN(tmp);
		tmp = (int)(*fp2);
		tmp -= (int)(*pfp2);
		c += DIFF_FCN(tmp);
		tmp = (int)(*fp3);
		tmp -= (int)(*pfp3);
		c += DIFF_FCN(tmp);

		// d
		tmp = (int)(fp1[1]);
		tmp -= (int)(pfp1[1]);
		d = DIFF_FCN(tmp);
		tmp = (int)(fp2[1]);
		tmp -= (int)(pfp2[1]);
		d += DIFF_FCN(tmp);
		tmp = (int)(fp3[1]);
		tmp -= (int)(pfp3[1]);
		d += DIFF_FCN(tmp);

		for (pel = 2; pel < pels - 2; pel++)
		{
			// e
			tmp = (int)(fp1[2]);
			tmp -= (int)(pfp1[2]);
			e = DIFF_FCN(tmp);
			tmp = (int)(fp2[2]);
			tmp -= (int)(pfp2[2]);
			e += DIFF_FCN(tmp);
			tmp = (int)(fp3[2]);
			tmp -= (int)(pfp3[2]);
			e += DIFF_FCN(tmp);

			// should we switch line removal mode?
			if (mode == INTL_LINE_REMOVE_MEDIAN)
			{
				if (a + b + c + d + e > INTL_DIFF_THRESH)
				{
					a =  MABS(fp1[-2] - fp1[+1]);
					a += MABS(fp3[-2] - fp3[+1]);
					a += MABS(fp1[-1] - fp1[+2]);
					a += MABS(fp3[-1] - fp3[+2]);
					a <<= 1;
					a += MABS(fp1[-2] - fp3[-2]);
					a += MABS(fp1[-1] - fp3[-1]);
					a += MABS(fp1[+1] - fp3[+1]);
					a += MABS(fp1[+2] - fp3[+2]);
					if (a > 0)
					{
						mode = INTL_LINE_REMOVE_AVG;
					}
				}
			}
			else
			{
				if (a + b + c + d + e < INTL_DIFF_THRESH)
				{
					a =  MABS(fp1[-2] - fp1[+1]);
					a += MABS(fp3[-2] - fp3[+1]);
					a += MABS(fp1[-1] - fp1[+2]);
					a += MABS(fp3[-1] - fp3[+2]);
					a <<= 1;
					a += MABS(fp1[-2] - fp3[-2]);
					a += MABS(fp1[-1] - fp3[-1]);
					a += MABS(fp1[+1] - fp3[+1]);
					a += MABS(fp1[+2] - fp3[+2]);
					if (a < 0)
					{
						mode = INTL_LINE_REMOVE_MEDIAN;
					}
				}
			}
				
			if (mode == INTL_LINE_REMOVE_MEDIAN)
			{
				// median
				fp2[0] = MEDIAN_3(fp1[0],fp2[0],fp3[0]);
			}
			else
			{
				// average
				tmp = fp1[0];
				tmp += fp3[0];
				fp2[0] = (tmp >> 1);
			}
			
			// shift difference measures
			a = b;
			b = c;
			c = d;
			d = e;

			// move pointers
			fp1++;pfp1++;
			fp2++;pfp2++;
			fp3++;pfp3++;

		}
	}
}



////////////////////////////////////////////////////////
//
//	Deinterlace_RGB24_Field
//
//	Replaces 'frame' with only one field.
//  The result is an image with dimesions pels x lines/2
//
////////////////////////////////////////////////////////

void
Deinterlace_RGB24_Field(unsigned char *frame, unsigned char *prev_frame, int pels, int lines, int pitch, int format, int field)
{
	int line;
	unsigned char *in,*out;

	lines >>= 1;

	// R
	in  = frame + field * pitch;
	out = frame;
	for (line = field; line < lines; line++)
	{
		memcpy(out, in, pels); /* Flawfinder: ignore */
		out += pitch;
		in  += (pitch << 1);
	}

	// G
	in  = frame + pels * (lines << 1);
	out = frame + pels * lines;
	for (line = 0; line < lines; line++)
	{
		memcpy(out, in, pels); /* Flawfinder: ignore */
		out += pitch;
		in  += (pitch << 1);
	}

	// B
	in  = frame + pels * (lines << 1) * 2;
	out = frame + pels * lines * 2;
	for (line = 0; line < lines; line++)
	{
		memcpy(out, in, pels); /* Flawfinder: ignore */
		out += pitch;
		in  += (pitch << 1);
	}
}



////////////////////////////////////////////////////////
//
//	Deinterlace_I420_Field
//
//	Replaces 'frame' with only one field.
//  The result is an image with dimesions pels x lines/2
//
////////////////////////////////////////////////////////

void
Deinterlace_I420_Field(unsigned char *frame, unsigned char *prev_frame, int pels, int lines, int pitch, int format, int field)
{
	register int line;
	register unsigned char *in,*out;
	int in_pitch;
	int out_pitch;
	int out_lines;

	// Y
	in_pitch = (pitch << 1);
	out_pitch = pitch;
	out_lines = (lines >> 1) & ~3;

	in  = frame + field * pitch;
	out = frame;
	for (line = 0; line < out_lines; line++)
	{
		memcpy(out, in, pels); /* Flawfinder: ignore */
		out += out_pitch;
		in  += in_pitch;
	}

	pels >>= 1;
	lines >>= 1;
	in_pitch >>= 1;
	out_pitch >>= 1;
	out_lines >>= 1;

	// U
	in  = frame + ((pels * lines) << 2);
	out = frame + ((pels * out_lines) << 2);
	for (line = 0; line < out_lines; line++)
	{
		memcpy(out, in, pels); /* Flawfinder: ignore */
		out += out_pitch;
		in  += in_pitch;
	}

	// V
	in  = frame + pels * lines * 5;
	out = frame + pels * out_lines * 5;
	for (line = 0; line < out_lines; line++)
	{
		memcpy(out, in, pels); /* Flawfinder: ignore */
		out += out_pitch;
		in  += in_pitch;
	}
}



////////////////////////////////////////////////////////
//
//	Deinterlace_I420_FlipFlop
//
//	The Odd field is put at the top of the frame,
//  the Even field is put up-side-down at the bottom.
//
////////////////////////////////////////////////////////

void
Deinterlace_I420_FlipFlop(unsigned char *frame, unsigned char *tempFrame, int pels, int lines, int pitch, int format)
{
	int lineCounter;
	
	//  reorder the frame, even field on top, Y, Cr then Cb
	for(lineCounter=0;lineCounter<lines;lineCounter+=2)
	{
		memcpy(tempFrame+((lineCounter>>1)*(pels)), /* Flawfinder: ignore */
			frame+lineCounter*pels,pels);
	}
	for(lineCounter=0;lineCounter<(lines>>1);lineCounter+=2)
	{
		memcpy(tempFrame+((lineCounter>>1)*(pels>>1)) + (pels*lines) /* Flawfinder: ignore */
			,frame+(pels*lines)+lineCounter*(pels>>1),(pels>>1));
	}
	for(lineCounter=0;lineCounter<(lines>>1);lineCounter+=2)
	{
		memcpy(tempFrame+((lineCounter>>1)*(pels>>1)) +(pels*lines*5/4) /* Flawfinder: ignore */
			,frame+(pels*lines*5/4)+lineCounter*(pels>>1),(pels>>1));
	}
	for(lineCounter=1;lineCounter<lines+1;lineCounter+=2)
	{
		memcpy(tempFrame+((lines-1)-(lineCounter>>1))*pels, /* Flawfinder: ignore */
			frame+lineCounter*pels,pels);
	}
	for(lineCounter=1;lineCounter<(lines>>1)+1;lineCounter+=2)
	{
		memcpy(tempFrame + (pels*lines) + /* Flawfinder: ignore */
			(((lines>>1)-1)-(lineCounter>>1))*(pels>>1),frame+(pels*lines)+lineCounter*(pels>>1),(pels>>1));
	}
	for(lineCounter=1;lineCounter<(lines>>1)+1;lineCounter+=2)
	{
		memcpy(tempFrame + (pels*lines*5/4) + /* Flawfinder: ignore */
			(((lines>>1)-1)-(lineCounter>>1))*(pels>>1),frame+(pels*lines*5/4)+lineCounter*(pels>>1),(pels>>1));
	}
	memcpy(frame,tempFrame,(pels*lines*3)/2); /* Flawfinder: ignore */

	return;
}


#ifdef _M_IX86

////////////////////////////////////////////////////////
//
//	Deinterlace_I420_MMX
//
//	MMX optimized deinterlacing of I420 data, using both fields
//
////////////////////////////////////////////////////////

static void
Deinterlace_I420_MMX(unsigned char *frame, unsigned char *prev_frame, int pels, int lines, int pitch, int format)
{
	int line_shift;
	int pels_div8;

	pels_div8 = (pels >> 3);			// round down
	line_shift = ((pitch << 1) - (pels & ~7));

	__asm {

		mov			ecx, lines			// ecx -> lines		
		shr			ecx, 1				// ecx -> lines/2

		mov			edx, pitch			// pitch
		mov			eax, edx
		neg			eax					// -pitch

		mov			esi, frame			// esi -> current frame
		add			esi, pitch;			// move esi to the next line

		pxor		mm7, mm7

ALIGN 16
line_loop:
		mov			ebx, pels_div8		// ebx -> pels/8 (rounded down)
ALIGN 16
pel_loop:
										// median = (upper & A) | (mid & B) | (lower & C)
										// e.g....
		movq		mm0,[esi+eax]		//	1	2	1	3	2	3	1	3	= mm0	= (upper)
		movq		mm1,[esi]			//	2	1	3	1	3	2	1	3	= mm1	= (mid)
		movq		mm2,[esi+edx]		//	3	3	2	2	1	1	2	3	= mm2	= (lower)

		movq		mm3,mm1				//	2	1	3	1	3	2	1	3	= mm3
		psubusb		mm3,mm0				//	1	0	2	0	1	0	0	0	= mm3	
		pcmpeqb		mm3,mm7				//	0	1	0	1	0	1	1	1	= mm3	= (upper > mid)

		movq		mm4,mm2				//	3	3	2	2	1	1	2	3	= mm4	
		psubusb		mm4,mm1				//	1	2	0	1	0	0	1	0	= mm4
		pcmpeqb		mm4,mm7				//	0	0	1	0	1	1	0	1	= mm4	= (mid > lower)

		movq		mm6,mm3				//	0	1	0	1	0	1	1	1	= mm6
		pxor		mm6,mm4				//	0	1	1	1	1	0	1	0	= mm6	= ~(B)
		pandn		mm6,mm1				//	2	0	0	0	0	2	1	0	= mm6	= (mid & B)

		movq		mm5,mm2				//	3	3	2	2	1	1	2	3	= mm5	
		psubusb		mm5,mm0				//	2	1	1	0	0	0	1	0	= mm5
		pcmpeqb		mm5,mm7				//	0	0	0	1	1	1	0	1	= mm5	= (upper > lower)

		movq		mm1,mm5				//	0	0	0	1	1	1	0	1	= mm1
		pxor		mm1,mm3				//	0	1	0	0	1	0	1	0	= mm1	= (A)
		pand		mm1,mm0				//	0	2	0	0	2	0	1	0	= mm1	= (upper & A)

		por  		mm6,mm1				//	2	2	0	0	2	2	1	0	= mm6	= (upper & A) | (mid & B)

		pxor		mm4,mm5				//	0	0	1	1	0	0	0	1	= mm4	= (C)
		pand		mm4,mm2				//	0	0	2	2	0	0	0	3	= mm4	= (lower & C)

		por  		mm6,mm4				//	2	2	2	2	2	2	1	3	= mm6	= (upper & A) | (mid & B) | (lower & C)

		movq		[esi],mm6			

		lea			esi,[esi+8]		

		dec			ebx
		jnz			pel_loop

		// any pels left???
		mov			ebx, pels
		and			ebx, 0x07
		jnz			last_pels

		add			esi, line_shift;	// move esi to the next line

		dec			ecx
		jnz			line_loop
		jmp			done

last_pels:
		// take care of last four pels
		movd		mm0,[esi+eax]
		movd		mm1,[esi]
		movd		mm2,[esi+edx]

		movq		mm3,mm1				//	2	1	3	1	3	2	1	3	= mm3
		psubusb		mm3,mm0				//	1	0	2	0	1	0	0	0	= mm3	
		pcmpeqb		mm3,mm7				//	0	1	0	1	0	1	1	1	= mm3	= (upper > mid)

		movq		mm4,mm2				//	3	3	2	2	1	1	2	3	= mm4	
		psubusb		mm4,mm1				//	1	2	0	1	0	0	1	0	= mm4
		pcmpeqb		mm4,mm7				//	0	0	1	0	1	1	0	1	= mm4	= (mid > lower)

		movq		mm6,mm3				//	0	1	0	1	0	1	1	1	= mm6
		pxor		mm6,mm4				//	0	1	1	1	1	0	1	0	= mm6	= ~(B)
		pandn		mm6,mm1				//	2	0	0	0	0	2	1	0	= mm6	= (mid & B)

		movq		mm5,mm2				//	3	3	2	2	1	1	2	3	= mm5	
		psubusb		mm5,mm0				//	2	1	1	0	0	0	1	0	= mm5
		pcmpeqb		mm5,mm7				//	0	0	0	1	1	1	0	1	= mm5	= (upper > lower)

		movq		mm1,mm5				//	0	0	0	1	1	1	0	1	= mm1
		pxor		mm1,mm3				//	0	1	0	0	1	0	1	0	= mm1	= (A)
		pand		mm1,mm0				//	0	2	0	0	2	0	1	0	= mm1	= (upper & A)

		por  		mm6,mm1				//	2	2	0	0	2	2	1	0	= mm6	= (upper & A) | (mid & B)

		pxor		mm4,mm5				//	0	0	1	1	0	0	0	1	= mm4	= (C)
		pand		mm4,mm2				//	0	0	2	2	0	0	0	3	= mm4	= (lower & C)

		por  		mm6,mm4				//	2	2	2	2	2	2	1	3	= mm6	= (upper & A) | (mid & B) | (lower & C)

		movd		[esi],mm6			

		add			esi, line_shift;	// move esi to the next line

		dec			ecx
		jnz			line_loop

done:
		emms
	}
}


////////////////////////////////////////////////////////
//
//	Deinterlace_I420_Slow
//
//	Slow deinterlacing of I420 data, 
//  with Rigorous Edge Interpolation using 
//  Optical Flow
//
////////////////////////////////////////////////////////
void
Deinterlace_I420_EdgeInterp(unsigned char *frame, unsigned char *prev_frame, int pels, int lines, int pitch, int format)
{
	int line, pel, mcountfr=1, mcountfi=1, D1=0;
	unsigned char *fpr, *fpg, *pfpg, *pfpr;
	unsigned char *fp1, *pfp1;
	unsigned char *fp2, *pfp2;
	unsigned char *fp3, *pfp3;
	# define IFrMThresh 10
	# define IFsMThresh 20
	# define FlowThresh 1200
	# define StrictFlowThresh 5	
	# define adj 10
	int frm =1;
	int diff1;
	
	int tmp;
	int *tempLine, *ptempLine;
	tempLine = (int*)malloc(pels*sizeof(int));		

	for (line = 1; line < lines - 2; line += 2)
	{
	
		int a,b,c,d,e,f;
		int D1 =0;		
		unsigned char *tfp3;
		memset(tempLine, -1, pels*sizeof(int));

		fpr = frame + line*pitch;
		
		fpg = fpr;

		
		fp2 = fpg + adj;
		
		ptempLine = &tempLine[0] + adj;


		// prev line
		fp1 = fp2 - pitch;
		
		// next line
		fp3 = fp2 + pitch;
		
		// previous frame
		if (prev_frame != 0)
		{
			pfpr = prev_frame + line * pitch;
			pfpg = pfpr;
			pfp2 = pfpg + adj;
			pfp1 = pfp2 - pitch;
			pfp3 = pfp2 + pitch;
		}
		else
		{
			Deinterlace_RGB24_Fast(frame, prev_frame, pels, lines, pitch, format);
			free(tempLine);
			return;
		}

		
		// initialize
		for (pel = adj; pel < (pels - adj); pel += 1)
		{
			
			int moving = 0;
			int fi, fD;
			int minf;
			int pix;
			int error2, error21, error22, error3, d2=0, j;

			// Load *next* pels
			a = (int)(*fp1);
			b = (int)(*fp2);
			c = (int)(*fp3);

			// intra frame
			D1 = *(fp1+1) - *(fp1-1);
			D1 = ABS(D1);
			if( D1 > 20) { 
					d = *(fp1+1);
					e = *(fp1-1);
					tfp3 = fp3;
					tmp = (a - *(tfp3));					
					minf = DIFF_FCN(tmp);
					tmp = (d - *(tfp3+1));
					minf += DIFF_FCN(tmp);
					tmp = (e - *(tfp3-1));
					minf += DIFF_FCN(tmp);

					fi = 0;
					tfp3--;					
					for (j=-1; j>=-adj; j--) {	
						tmp = (a - *(tfp3));
						fD = DIFF_FCN(tmp);
						if (fD > minf)
							goto gskip;
						tmp = (d - *(tfp3+1));
						fD= fD + DIFF_FCN(tmp);
						if (fD > minf)
							goto gskip;
						tmp = (e - *(tfp3-1));
						fD= fD + DIFF_FCN(tmp);
						if(fD<minf) {
							minf = fD;
							fi = j;
						}
gskip:
						tfp3--;						
					}

					tfp3 = fp3 + 1;
					for (j=1; j<=adj; j++) {	
						tmp = (a - *(tfp3));
						fD = DIFF_FCN(tmp);
						if (fD > minf)
							goto gskip2;
						tmp = (d - *(tfp3+1));
						fD += DIFF_FCN(tmp);
						if (fD > minf)
							goto gskip2;
						tmp = (e - *(tfp3-1));
						fD += DIFF_FCN(tmp);
						if(fD<minf) {
							minf = fD;
							fi = j;
						}
gskip2:
						tfp3++;						
					}

					#ifdef TIME_DEINTERLACE
					//num_flow++;
					#endif
					
					if (minf < FlowThresh)  {
						
						if ((fi == 1) || (fi == -1) || (fi == 0)) {							
							tempLine[pel] = (*fp1 + *fp3)/2;
						} else {
							
							f = *(fp3+ fi);
							pix = (a + f) / 2;
							d2 = (int)(fi)/2;
						
							f = *(fp1 + d2);
							tmp = (pix - f);
							error2 = ABS(tmp);
							
							f = *(fp1 + d2 + 1);
							pix = (d + *(fp3 + fi + 1))/2;
							tmp = (pix - f);
							error21 = ABS(tmp);

							f = *(fp1 + d2 -1);
							pix = (e + *(fp3 + fi - 1))/2;
							tmp = (pix - f);
							error22 = ABS(tmp);
														

							if(error2 < 15 && ( ((error21 < 15) && (error22 > 25)) || ((error21 > 25) && (error22 < 15)) ) ) {
								
								pix = (a + *(fp3+fi)) / 2;
								tempLine[pel + d2] = pix;
							
								pix = (d + *(fp3 + fi + 1))/2;
								tempLine[pel + d2 + 1] = pix;

								pix = (e + *(fp3 + fi - 1))/2;
								tempLine[pel + d2 - 1] = pix;								

								goto gadvance;
							}
							
							pix = (a + *(fp3 + fi))/2;
							f = *(fp3 + d2);
							tmp = (pix - f);
							error3 = ABS(tmp);
							
							f = *(fp3 + d2 + 1);
							pix = (d + *(fp3 + fi + 1))/2;
							tmp = (pix - f);
							error21 = ABS(tmp);
							
							f = *(fp3 + d2 -1);
							pix = (e + *(fp3 + fi - 1))/2;
							tmp = (pix - f);
							error22 = ABS(tmp);	
							
							pix = (a + *(fp3 + fi))/2;

							if( error3 < 15 && ( ((error21 < 15) && (error22 > 25)) || ((error21 > 25) && (error22 < 15))) ) {
							
								tempLine[pel + d2] = pix;
							
								pix = (d + *(fp3 + fi + 1))/2;
								tempLine[pel + d2 + 1] = pix;

								pix = (e + *(fp3 + fi - 1))/2;
								tempLine[pel + d2 - 1] = pix;
													
							
								goto gadvance;
							} else {
								/*
								*fp2 = 0;
								*fpr2 = 0;
								*fpb2 = 255;
								tempLine[pel] = 0;
								*/
								
							}
							
						}
					} else {
						/*
						*fp2 = 0;
						*fpr2 = 255;
						*fpb2 = 0;
						tempLine[pel] = 0;
						*/
						
					}

			}
gadvance:
			fp1++;
			fp2++;
			fp3++;
			pfp1++;
			pfp2++;
			pfp3++;
			ptempLine++;
		}

		// current frame
		// current line
		fp2 = fpg;
		// prev line
		fp1 = fp2 - pitch;
		// next line
		fp3 = fp2 + pitch;
		// current frame
		
		//prev frame
		// current line
		pfp2 = pfpg;
		// prev line
		pfp1 = pfp2 - pitch;
		// next line
		pfp3 = pfp2 + pitch;

		ptempLine = &tempLine[0];		

		for (pel = 0; pel < pels-1; pel += 1)		
		{			
			// change
					
				tmp = (*fp1 - *(fp3));
				diff1 = DIFF_FCN(tmp);
				tmp = (*(fp1-1) - *(fp3-1));
				diff1 += DIFF_FCN(tmp);
				tmp = (*(fp3+1) - *(pfp3+1));
				diff1 += DIFF_FCN(tmp);				

				if( ABS(diff1) > 4800 ) {
					// horiz
					tmp = (*fp1 - *(pfp1));
					diff1 = DIFF_FCN(tmp);
					tmp = (*(fp1-1) - *(pfp1-1));
					diff1 += DIFF_FCN(tmp);
					tmp = (*(fp1+1) - *(pfp1+1));
					diff1 += DIFF_FCN(tmp);
					tmp = (*fp3 - *(pfp3));
					diff1 += DIFF_FCN(tmp);
					tmp = (*(fp3-1) - *(pfp3-1));
					diff1 += DIFF_FCN(tmp);
					tmp = (*(fp3+1) - *(pfp3+1));
					diff1 += DIFF_FCN(tmp);
					
				
					if (diff1 < 2400) {
						// not moving						
					
						*fp2 = MEDIAN_3(*fp1,*fp2,*fp3);											
					
					} else {

						if (*ptempLine != -1) {
							*fp2 = (unsigned char) *ptempLine;
						} else {
							tmp = (*fp1 + *fp3)/2;
							*fp2 = 	(unsigned char) tmp;	
						}						

						#ifdef TIME_DEINTERLACE
							num_avg++;
						#endif				
			
					}
				} else {
				// not horiz
				// avg
					if (*ptempLine != -1) {
						D1 = *(fp1+1) - *(fp1-1);
						D1 = ABS(D1);
						if( D1 > 20) { 
							*fp2 = (unsigned char) *ptempLine;
						} else {
							tmp = (*fp1 + *fp3)/2;
							*fp2 = 	(unsigned char) tmp;															
								// not vertical								
						}
					} else {
						tmp = (*fp1 + *fp3)/2;
						*fp2 = 	(unsigned char) tmp;							
					}					
			
					#ifdef TIME_DEINTERLACE
						num_avg++;
					#endif	
											
				}

			
			fp1++;
			fp2++;
			fp3++;
			pfp1++;
			pfp2++;
			pfp3++;
			ptempLine++;
		}
	}
	
	free(tempLine);	

	return;

}	


#endif






















































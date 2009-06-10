/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: invtelec.cpp,v 1.10 2007/07/06 22:00:23 jfinnecy Exp $
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

#include "invtelec.h"
#include "hxtick.h"
#include "mmx_util.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/string.h"
#include "hlxclib/math.h"

////////////////////////////////////////////////////////
//	internal prototypes
////////////////////////////////////////////////////////

static T_INVTELE_RESULT 
InvTelecineDetect(
	UCHAR *data, UCHAR *prevData, 
	double frameRate, 
	ULONG32 timestamp, 
	ULONG32 pels, ULONG32 lines,
	HXBOOL bDeInterlaced,
	T_INVTELE_STATE *state);

//#define CODEC_DEBUG_32PULLDOWN
#ifdef CODEC_DEBUG_32PULLDOWN
#include <stdio.h>
static FILE *fp_log = NULL;
#endif


const T_INVTELE_FLOAT inThresh[PULLDOWN_HIST_LEN+1] = {3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4};
const T_INVTELE_FLOAT outThresh[PULLDOWN_HIST_LEN+1] = {1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2};

#ifdef ALLOW_MMX_INVTELE

void
SumSqaredFrameDiff_MMX(
	unsigned char *frame, 
	unsigned char *prev_frame,
	int pels,
	int lines,
	int pitch,
	float *ssd_odd,
	float *ssd_even);

void
RestitchCheck_MMX(
	unsigned char *frame, 
	unsigned char *prev_frame,
	int pels,
	int lines,
	int pitch,
	float *ssd_new,
	float *ssd_old);

#endif

////////////////////////////////////////////////////////
//
//	InitInvTelecine
//
//	Allocates and initializes memory for state information
//
//	Parameters:
//		state:	Pointer to the allocated state pointer
//
////////////////////////////////////////////////////////

INT32
InitInvTelecine(T_INVTELE_STATE **state)
{
	T_INVTELE_STATE *new_state = 0;

	if (*state != 0)
		return 1;

	new_state = (T_INVTELE_STATE *)malloc(sizeof(T_INVTELE_STATE));

	if (new_state == 0)
	{
		return 1;
	}

	new_state->impl_id = INVTELE_IMPL_ID_C;
	new_state->firstFrame = TRUE;
	new_state->ulPulldownActiveTimerIntl = 0;
	new_state->ulPulldownActiveTimerProg = 0;
	new_state->lastRemovedTimestamp = 0;
	new_state->frameCountMod = 4;
	new_state->frameRemovalPattern = 4;
	new_state->NTSCTrackingFrameCounter = 500;
	new_state->interleaveEvenFlag = FALSE;
	new_state->interleaveOddFlag = FALSE;
	new_state->checkNextFrameForInterlace = FALSE;
	new_state->checkNextFrameForProgressive = FALSE;
	new_state->bProgressiveTelecineSeen = FALSE;
	new_state->bInterlacedTelecineSeen = FALSE;
	new_state->pulldownTimeBuffer = 0;

	for (int i = 0; i < PULLDOWN_HIST_LEN; i++)
	{
		new_state->pulldownSadHistEven[i] = UN_INIT_SAD;
		new_state->pulldownSadHistOdd[i] = UN_INIT_SAD;
		new_state->pulldownSadHistAll[i] = UN_INIT_SAD;
		new_state->pulldownTimeHist[i] = 0;
	}

#ifdef ALLOW_MMX_INVTELE
	// Check for MMx availability
    if (checkMmxAvailablity() & CPU_HAS_MMX)
		new_state->impl_id = INVTELE_IMPL_ID_MMX;
#endif

	*state = new_state;

	return 0;
}


////////////////////////////////////////////////////////
//
//	FreeInvTelecine
//
//	Frees memory for state information
//
//	Parameters:
//		state:	Pointer to the allocated state pointer
//
////////////////////////////////////////////////////////

void
FreeInvTelecine(T_INVTELE_STATE **state)
{
	if (*state != 0)
		free(*state);

	*state = 0;
}


////////////////////////////////////////////////////////
//
//	IsContentProgressiveTelecine
//
//	Returns TRUE if the telecine detector has seen 
//	  interlaced telecine content
//
//	Parameters:
//		state:	State pointer
//
////////////////////////////////////////////////////////

HXBOOL
IsContentProgressiveTelecine(T_INVTELE_STATE *state)
{
	if (state != 0)
		return (state->bProgressiveTelecineSeen)?(TRUE):(FALSE);

	return FALSE;
}


////////////////////////////////////////////////////////
//
//	IsContentInterlacedTelecine
//
//	Returns TRUE if the telecine detector has seen 
//	  progressive telecine content
//
//	Parameters:
//		state:	State pointer
//
////////////////////////////////////////////////////////

HXBOOL
IsContentInterlacedTelecine(T_INVTELE_STATE *state)
{
	if (state != 0)
		return (state->bInterlacedTelecineSeen)?(TRUE):(FALSE);

	return FALSE;
}


////////////////////////////////////////////////////////
//
//	GetTelecinePattern
//
//	Returns the frame modulo for the removal pattern
//
//	Parameters:
//		state:	State pointer
//
////////////////////////////////////////////////////////

UCHAR
GetTelecinePattern(T_INVTELE_STATE *state)
{
	INT32 pattern;

	if (state != 0)
		pattern = (state->frameRemovalPattern - state->frameCountMod) % 5;
	else
		pattern = 0;

	if (pattern < 0)
		pattern +=5;
		
	return (UCHAR)pattern;
}


////////////////////////////////////////////////////////
//
//	SetTelecinePattern
//
//	Sets the frame modulo for the removal pattern
//
//	Parameters:
//		state:	State pointer
//
////////////////////////////////////////////////////////

void
SetTelecinePattern(T_INVTELE_STATE *state, UCHAR telecine_pattern)
{
	if (state != 0)
		state->frameRemovalPattern = ((telecine_pattern + state->frameCountMod) % 5);
}



////////////////////////////////////////////////////////
//
//	DoInvTelecine
//
//	Performs inverse Telecine
//
//	Parameters:
//		data:			Pointer to the current planar frame.
//		prevData:		Pointer to the previous source frame.
//		frameRate:		The input frame rate.
//		timestamp:		The timestamp of the current frame.
//						This value will be adjusted to account
//						for the change to 24 fps
//		pels, lines:	Frame dimensions.
//		bDeInterlaced:	Should be set to TRUE if frame is known
//						to be progressive.
//		state:			State pointer
//
////////////////////////////////////////////////////////

T_INVTELE_RESULT
DoInvTelecine(
	UCHAR *data, 
	UCHAR *prevData, 
	double frameRate, 
	ULONG32 &timestamp, 
	ULONG32 pels, ULONG32 lines, 
	HXBOOL bDeInterlaced, 
	T_INVTELE_STATE *state)
{
	T_INVTELE_RESULT ret;

	if (frameRate < 25.5)
		return (INVTELE_RESULT_LOW_FRAMERATE);

#ifdef CODEC_DEBUG_32PULLDOWN
  if(fp_log == NULL)
	  fp_log = fopen("c:\\pulldown.log","w");
#endif

	ret = InvTelecineDetect(data, prevData, frameRate, timestamp, pels, lines, bDeInterlaced, state);

	// hack -- don't drop more than 1 out of every 5 progressive frames
	if ((state->pulldownTimeBuffer > 1) && 
		(bDeInterlaced || lines < 242) &&
		(ret == INVTELE_RESULT_DROP_FRAME))
	{
		ret = INVTELE_RESULT_FRAME_OK;
	}

	if (ret == INVTELE_RESULT_DROP_FRAME)
	{
		state->pulldownTimeBuffer = 33;
	}
	else
	{
		// adjust timestamp 
		// -- allowed offsets will be -33, -25, -17, -9, -1, 0
		timestamp -= state->pulldownTimeBuffer;

		if (state->pulldownTimeBuffer > 8)
		{
			state->pulldownTimeBuffer -= 8;
		}
		else
		{
			state->pulldownTimeBuffer = 0;
		}
	}

	return (ret);
}



////////////////////////////////////////////////////////
//
//	InvTelecineDetect
//
//	Performs detection of the inverse telecine pattern
//
//	Parameters:
//		data:			Pointer to the current planar frame.
//		prevData:		Pointer to the previous source frame.
//		frameRate:		The input frame rate.
//		timestamp:		The timestamp of the current frame.
//		pels, lines:	Frame dimensions.
//		bDeInterlaced:	Should be set to TRUE if frame is known
//						to be progressive.
//		state:			State pointer
//
////////////////////////////////////////////////////////

T_INVTELE_RESULT
InvTelecineDetect
(
	 UCHAR *data, 
	 UCHAR *prevData, 
	 double frameRate, 
	 ULONG32 timestamp, 
	 ULONG32 pels, ULONG32 lines,
	 HXBOOL bDeInterlaced,
	 T_INVTELE_STATE *state
)
{
	unsigned int i, j, k, ll;
	ULONG32 patternStart, histLength;
	LONG32 *pNew, *pOld;
	float temp;
	float sumEven = 0.0f, sumOdd = 0.0f;
	float sumOld = 0.0f, sumNew = 0.0f;
	float sumAll = 0.0f;

	float	inGroupMeanEven[5],outGroupMeanEven[5];
	float	inGroupStdEven[5],outGroupStdEven[5];
	ULONG32 inGroupCountEven,outGroupCountEven;
	float	outMinEven[5],outMaxEven[5];
	float	inMaxEven[5],inMinEven[5];
	HXBOOL	groupValidFlagEven[5];

	float	inGroupMeanOdd[5],outGroupMeanOdd[5];
	float	inGroupStdOdd[5],outGroupStdOdd[5];
	ULONG32	inGroupCountOdd,outGroupCountOdd;
	float	outMinOdd[5],outMaxOdd[5];
	float	inMaxOdd[5],inMinOdd[5];
	HXBOOL	groupValidFlagOdd[5];

	float	inGroupMean[5], outGroupMean[5];
	float	inGroupStd[5], outGroupStd[5];
	ULONG32 inGroupCount,outGroupCount;
	float	outMin[5],outMax[5],inMax[5];
	HXBOOL	groupValidFlag[5];

	ULONG32 timeSinceLastFrame;
	HXBOOL	obviousPatternFlag = FALSE;
	HXBOOL	sceneChangeFlag = FALSE;

	if (state->firstFrame == TRUE)
	{
		// Initialize history timestamps with this first timestamp
		for (i = 0; i < PULLDOWN_HIST_LEN; i++)
		{
			state->pulldownTimeHist[i] = timestamp;
		}
		state->lastRemovedTimestamp = timestamp;
		state->firstFrame = FALSE;
		goto TOO_EARLY;
	}

	// Calculate Sum of Differences for even and odd lines...
	// If we know that the frame is de-interlaced, then the stats
	// for the "odd" lines are invalid.  So just measure "even"
	// lines then copy into "odd" SAD (so the rest of the algorithm
	// will work).
	ll = (pels >> 2);

	if (bDeInterlaced)
	{
		pNew = (LONG32 *)data;
		pOld = (LONG32 *)prevData;
		ll = (pels >> 2);

		for (i = 0; i < lines; i += 2)  //  only do the luma.
		{
			for (j = 0; j < ll; j++)
			{
				temp = (float)((pNew[j]&0xff00) - (pOld[j]&0xFF00));
				sumEven += ((float)(1./(256.*256.)))*(temp*temp);
			}

			pOld += 2*ll;
			pNew += 2*ll;
		}
		sumOdd = sumEven;
	}
	else
	{
		switch (state->impl_id)
		{
#ifdef ALLOW_MMX_INVTELE
		case INVTELE_IMPL_ID_MMX:
			{
				SumSqaredFrameDiff_MMX(
					data, prevData,
					pels, lines, pels,
					&sumEven, &sumOdd);
				sumAll = 0.5f * (sumEven + sumOdd);
			}
			break;
#endif
		case INVTELE_IMPL_ID_C:
		default:
			{
				pNew = (LONG32 *)data;
				pOld = (LONG32 *)prevData;

				for (i = 0; i < lines; i += 2)  //  only do the luma.
				{
					for (j = 0; j < ll; j++)
					{
						temp = (float)((pNew[j]&0xff00) - (pOld[j]&0xFF00));
						sumEven += ((float)(1./(256.*256.)))*(temp*temp);
						temp = (float)((pNew[j+ll]&0xFF00) - (pOld[j+ll]&0xFF00));
						sumOdd += ((float)(1./(256.*256.)))*(temp*temp);
					}

					pOld += 2*ll;
					pNew += 2*ll;
				}
				sumAll = sumEven + sumOdd;
				sumEven /= (ll * (lines>>1));
				sumOdd /= (ll * (lines>>1));
				sumAll /= (ll * lines);
			}
			break;
		}
	}

	sceneChangeFlag = (sumAll > 7500)?(TRUE):(FALSE);
	if (sumEven > 100) sumEven=100;
	if (sumOdd > 100) sumOdd=100;
	if (sumAll > 100) sumAll=100;

#ifdef CODEC_DEBUG_32PULLDOWN
	fprintf(fp_log,"ssd are %f %f at time %d\n",sumEven,sumOdd,timestamp);
#endif

	// Compensate for 30 vs 29.97fps captures.
	if ((sumEven == 0) && (sumOdd == 0) && 
		(state->NTSCTrackingFrameCounter > 500) && (frameRate > 29.98))
	{
		state->pulldownTimeHist[PULLDOWN_HIST_LEN-1] = timestamp;
		state->NTSCTrackingFrameCounter = 0;
 		goto REMOVE_FRAME;
	}
	state->NTSCTrackingFrameCounter++;

	// In case we dropped a frame
	timeSinceLastFrame = CALCULATE_ELAPSED_TICKS(state->pulldownTimeHist[PULLDOWN_HIST_LEN-1], timestamp);

	while(timeSinceLastFrame > 50)
	{
		for(i=0;i<PULLDOWN_HIST_LEN-1;i++)
		{
			state->pulldownSadHistEven[i] = state->pulldownSadHistEven[i+1];
			state->pulldownSadHistOdd[i] = state->pulldownSadHistOdd[i+1];
			state->pulldownSadHistAll[i] = state->pulldownSadHistAll[i+1];
			state->pulldownTimeHist[i] = state->pulldownTimeHist[i+1];
		}

		state->pulldownSadHistEven[i] = MISSING_SAD;
		state->pulldownSadHistOdd[i] = MISSING_SAD;
		state->pulldownSadHistAll[i] = MISSING_SAD;
		state->pulldownTimeHist[i] = timestamp;
		timeSinceLastFrame -= 33;
		state->frameRemovalPattern--;
		if (state->frameRemovalPattern < 0)
			state->frameRemovalPattern = 4;
		state->frameCountMod--;
		if (state->frameCountMod < 0)
			state->frameCountMod = 4;
	}

	//  Update the history 
	for(i = 0; i < PULLDOWN_HIST_LEN - 1; i++)
	{
		state->pulldownSadHistEven[i] = state->pulldownSadHistEven[i+1];
		state->pulldownSadHistOdd[i] = state->pulldownSadHistOdd[i+1];
		state->pulldownSadHistAll[i] = state->pulldownSadHistAll[i+1];
		state->pulldownTimeHist[i] = state->pulldownTimeHist[i+1];
	}
	state->frameRemovalPattern--;
	if (state->frameRemovalPattern < 0)
		state->frameRemovalPattern = 4;
	state->frameCountMod--;
	if (state->frameCountMod < 0)
		state->frameCountMod = 4;

	state->pulldownSadHistEven[i] = sumEven;
	state->pulldownSadHistOdd[i] = sumOdd;
	state->pulldownSadHistAll[i] = sumAll;
	state->pulldownTimeHist[i] = timestamp;

	// If removal on, and we have no pattern established, wait a while.
	if ((state->pulldownSadHistEven[PULLDOWN_HIST_LEN - 5] == UN_INIT_SAD) || 
		(state->pulldownSadHistOdd[PULLDOWN_HIST_LEN - 5] == UN_INIT_SAD) || 
		(state->pulldownSadHistAll[PULLDOWN_HIST_LEN - 5] == UN_INIT_SAD))
	{
#ifdef CODEC_DEBUG_32PULLDOWN
		fprintf(fp_log,"gotoing - too early\n");
#endif
		goto TOO_EARLY;
	}

	histLength = 0;

	while (state->pulldownSadHistAll[histLength] == UN_INIT_SAD)
	{
		histLength += 5;
		if (histLength > PULLDOWN_HIST_LEN - 10)
			goto TOO_EARLY;
	}

	if ((bDeInterlaced) || (state->ulPulldownActiveTimerProg > 90 && state->ulPulldownActiveTimerIntl < 10))
	{
		// Skip interlaced telecine tests
		goto PROGRESSIVE_TESTS;
	}

	// Gather statistics from SAD history
	// Run through tests looking at the last 20, then 15, then 10, then 5 frames
	for (patternStart = histLength; patternStart < PULLDOWN_HIST_LEN; patternStart += 5)
	{
		for (i = 0; i < 5;i++)
		{
			// Stats for "even" lines
			inGroupMeanEven[i]  = 0;
			outGroupMeanEven[i] = 0;
			inGroupStdEven[i]   = 0;
			outGroupStdEven[i]  = 0;
			inGroupCountEven    = 0;
			outGroupCountEven   = 0;
			outMinEven[i] = 255*255;
			inMinEven[i]  = 255*255;
			outMaxEven[i] = 0;
			inMaxEven[i]  = 0;
			for (j = patternStart + i; j < PULLDOWN_HIST_LEN; j++)
			{
				if (state->pulldownSadHistEven[j] == MISSING_SAD ||
					state->pulldownSadHistEven[j] == UN_INIT_SAD)
					continue;

				if (((j - i) % 5) == 0)
				{
					inGroupMeanEven[i] += state->pulldownSadHistEven[j];
					inGroupStdEven[i] += state->pulldownSadHistEven[j]*state->pulldownSadHistEven[j];
					if (inMaxEven[i] < state->pulldownSadHistEven[j])
						inMaxEven[i] = state->pulldownSadHistEven[j];
					if (inMinEven[i] > state->pulldownSadHistOdd[j])
						inMinEven[i] = state->pulldownSadHistOdd[j];
					inGroupCountEven++;
				}
				else
				{
					outGroupMeanEven[i]+= state->pulldownSadHistEven[j];
					outGroupStdEven[i]+= state->pulldownSadHistEven[j]*state->pulldownSadHistEven[j];
					if (outMinEven[i] > state->pulldownSadHistEven[j])
						outMinEven[i] = state->pulldownSadHistEven[j];
					if (outMaxEven[i] < state->pulldownSadHistEven[j])
						outMaxEven[i] = state->pulldownSadHistEven[j];
					outGroupCountEven++;
				}
			}
			// Is there enough valid data to analyze?
			if ((inGroupCountEven > 1) && (outGroupCountEven > 3))
			{
				inGroupMeanEven[i] = inGroupMeanEven[i]/inGroupCountEven;
				if ((inGroupStdEven[i]/inGroupCountEven)-(inGroupMeanEven[i]*inGroupMeanEven[i]) > 0.0f)
					inGroupStdEven[i] = (float)sqrt((inGroupStdEven[i]/inGroupCountEven)-(inGroupMeanEven[i]*inGroupMeanEven[i]));
				else
					inGroupStdEven[i] = 0.0f;

				outGroupMeanEven[i] = outGroupMeanEven[i]/outGroupCountEven;
				if ((outGroupStdEven[i]/outGroupCountEven)-(outGroupMeanEven[i]*outGroupMeanEven[i]) > 0.0f)
					outGroupStdEven[i] = (float)sqrt((outGroupStdEven[i]/outGroupCountEven)-(outGroupMeanEven[i]*outGroupMeanEven[i]));
				else
					outGroupStdEven[i] = 0.0f;

				groupValidFlagEven[i] = TRUE;
			}
			else
			{
				inGroupMeanEven[i] = 0;
				outGroupMeanEven[i] = 0;
				inGroupStdEven[i] = 1;
				outGroupStdEven[i] = 1;
				groupValidFlagEven[i] = FALSE;
			}

			// Stats for "odd" lines
			inGroupMeanOdd[i]  = 0;
			outGroupMeanOdd[i] = 0;
			inGroupStdOdd[i]   = 0;
			outGroupStdOdd[i]  = 0;
			inGroupCountOdd    = 0;
			outGroupCountOdd   = 0;
			outMinOdd[i] = 255*255;
			inMinOdd[i]  = 255*255;
			outMaxOdd[i] = 0;
			inMaxOdd[i]  = 0;
			for (j = patternStart + i; j < PULLDOWN_HIST_LEN; j++)
			{
				if (state->pulldownSadHistOdd[j] == MISSING_SAD ||
					state->pulldownSadHistOdd[j] == UN_INIT_SAD)
					continue;

				if (((j - i) % 5) == 0)
				{
					inGroupMeanOdd[i] += state->pulldownSadHistOdd[j];
					inGroupStdOdd[i] += state->pulldownSadHistOdd[j]*state->pulldownSadHistOdd[j];
					if (inMaxOdd[i] < state->pulldownSadHistOdd[j])
						inMaxOdd[i] = state->pulldownSadHistOdd[j];
					if (inMinOdd[i] > state->pulldownSadHistOdd[j])
						inMinOdd[i] = state->pulldownSadHistOdd[j];
					inGroupCountOdd++;
				}
				else
				{
					outGroupMeanOdd[i] += state->pulldownSadHistOdd[j];
					outGroupStdOdd[i] += state->pulldownSadHistOdd[j]*state->pulldownSadHistOdd[j];
					if (outMinOdd[i] > state->pulldownSadHistOdd[j])
						outMinOdd[i] = state->pulldownSadHistOdd[j];
					if (outMaxOdd[i] < state->pulldownSadHistOdd[j])
						outMaxOdd[i] = state->pulldownSadHistOdd[j];
					outGroupCountOdd++;
				}
			}
			// Is there enough valid data to analyze?
			if ((inGroupCountOdd > 1) && (outGroupCountOdd > 3))
			{
				inGroupMeanOdd[i] = inGroupMeanOdd[i]/inGroupCountOdd;
				if ((inGroupStdOdd[i]/inGroupCountOdd)-(inGroupMeanOdd[i]*inGroupMeanOdd[i]) > 0.0f)
					inGroupStdOdd[i] = (float)sqrt((inGroupStdOdd[i]/inGroupCountOdd)-(inGroupMeanOdd[i]*inGroupMeanOdd[i]));
				else
					inGroupStdOdd[i] = 0.0f;

				outGroupMeanOdd[i] = outGroupMeanOdd[i]/outGroupCountOdd;
				if ((outGroupStdOdd[i]/outGroupCountOdd)-(outGroupMeanOdd[i]*outGroupMeanOdd[i]) > 0.0f)
					outGroupStdOdd[i] = (float)sqrt((outGroupStdOdd[i]/outGroupCountOdd)-(outGroupMeanOdd[i]*outGroupMeanOdd[i]));
				else
					outGroupStdOdd[i] = 0.0f;

				groupValidFlagOdd[i] = TRUE;
			}
			else
			{
				inGroupMeanOdd[i] = 0;
				outGroupMeanOdd[i] = 0;
				inGroupStdOdd[i] = 1;
				outGroupStdOdd[i] = 1;
				groupValidFlagOdd[i] = FALSE;
			}
		}

		// Do we have a clear pattern? Always trust this test.
		for (i = 0; i < 5; i++)
		{
			if (groupValidFlagEven[i] == FALSE)
				continue;
			if (groupValidFlagOdd[i] == FALSE)
				continue;
			if (groupValidFlagEven[(i+2)%5] == FALSE)
				continue;
			if (groupValidFlagOdd[(i+2)%5] == FALSE)
				continue;

			if ((inGroupMeanEven[i]+inThresh[patternStart]*inGroupStdEven[i] < outGroupMeanEven[i] - outThresh[patternStart]*outGroupStdEven[i]) &&
				(inGroupMeanOdd[(i+2)%5]+inThresh[patternStart]*inGroupStdOdd[(i+2)%5] < outGroupMeanOdd[(i+2)%5] - outThresh[patternStart]*outGroupStdOdd[(i+2)%5]) &&
				(inGroupMeanEven[i]+inGroupStdEven[i] < inGroupMeanOdd[i]) &&
				(inGroupMeanOdd[(i+2)%5]+inGroupStdOdd[(i+2)%5] < inGroupMeanEven[(i+2)%5]))
			{
#ifdef CODEC_DEBUG_32PULLDOWN
				fprintf(fp_log,"clear pattern, i is %d, pulldown timer is %d, patternStart is %d\n",i,state->ulPulldownActiveTimerIntl,patternStart);
#endif
				// Set the removal pattern phase
				state->frameRemovalPattern = i;

				// If this is the right frame remove it!
				if (i == (PULLDOWN_HIST_LEN - 1) % 5)
				{
					// Set a counter that goes up if we have a consistent pattern 80+% of the time, down otherwise
					if ((timestamp - state->lastRemovedTimestamp > 145) &&
						(timestamp - state->lastRemovedTimestamp < 175))
					{
						if (state->ulPulldownActiveTimerIntl < 95)
							state->ulPulldownActiveTimerIntl += 5;
						else
						{
							state->ulPulldownActiveTimerIntl = 100;
							state->bInterlacedTelecineSeen = TRUE;
						}

						if (state->ulPulldownActiveTimerProg > 5)
							state->ulPulldownActiveTimerProg -= 5;
						else
							state->ulPulldownActiveTimerProg = 0;
					}
					else if (timestamp - state->lastRemovedTimestamp < 300)
					{
						if (state->ulPulldownActiveTimerIntl > 5)
							state->ulPulldownActiveTimerIntl -= 5;
						else
							state->ulPulldownActiveTimerIntl = 0;
					}
					state->lastRemovedTimestamp = timestamp;

					goto REMOVE_FRAME;
				}
				else if (i == ((PULLDOWN_HIST_LEN - 2) % 5))
				{
					obviousPatternFlag = TRUE;
					goto INTERLEAVE_ODD;
				}
				else 
					goto DO_NOTHING;
			}

			if ((inGroupMeanOdd[i]+inThresh[patternStart]*inGroupStdOdd[i] < outGroupMeanOdd[i] - outThresh[patternStart]*outGroupStdOdd[i]) &&
				(inGroupMeanEven[(i+2)%5]+inThresh[patternStart]*inGroupStdEven[(i+2)%5] < outGroupMeanEven[(i+2)%5] - outThresh[patternStart]*outGroupStdEven[(i+2)%5]) &&
				(inGroupMeanOdd[i]+inGroupStdOdd[i] < inGroupMeanEven[i]) &&
				(inGroupMeanEven[(i+2)%5]+inGroupStdEven[(i+2)%5] < inGroupMeanOdd[(i+2)%5]))
			{
#ifdef CODEC_DEBUG_32PULLDOWN
				fprintf(fp_log,"clear pattern, i is %d, pulldown timer is %d, patternStart is %d\n",i,state->ulPulldownActiveTimerIntl,patternStart);
#endif
				// Set the removal pattern phase
				state->frameRemovalPattern=i;

				// If this is the right frame remove it!
				if (i == (PULLDOWN_HIST_LEN - 1) % 5)
				{
					// Set a counter that goes up if we have a consistent pattern 80+% of the time, down otherwise
					if ((timestamp - state->lastRemovedTimestamp > 145) &&
						(timestamp - state->lastRemovedTimestamp < 175))
					{
						if (state->ulPulldownActiveTimerIntl < 95)
							state->ulPulldownActiveTimerIntl += 5;
						else
						{
							state->ulPulldownActiveTimerIntl = 100;
							state->bInterlacedTelecineSeen = TRUE;
						}

						if (state->ulPulldownActiveTimerProg > 5)
							state->ulPulldownActiveTimerProg -= 5;
						else
							state->ulPulldownActiveTimerProg = 0;
					}
					else if (timestamp - state->lastRemovedTimestamp < 300)
					{
						if (state->ulPulldownActiveTimerIntl > 5)
							state->ulPulldownActiveTimerIntl -= 5;
						else
							state->ulPulldownActiveTimerIntl = 0;
					}
					state->lastRemovedTimestamp = timestamp;

					goto REMOVE_FRAME;
				}
				else if (i == ((PULLDOWN_HIST_LEN - 2) % 5))
				{
					obviousPatternFlag = TRUE;
					goto INTERLEAVE_EVEN;
				}
				else 
					goto DO_NOTHING;
			}
		}

		// Do we have a pretty clear pattern? Only trust this test 
		// if we have succeeded with the strongest test a couple of times
		for (i = 0; i < 5; i++)
		{
			if (groupValidFlagEven[i] == FALSE)
				continue;
			if (groupValidFlagOdd[i] == FALSE)
				continue;
			if (groupValidFlagEven[(i+2)%5] == FALSE)
				continue;
			if (groupValidFlagOdd[(i+2)%5] == FALSE)
				continue;

			if ((state->ulPulldownActiveTimerIntl > 10) &&
				(inGroupMeanEven[i]      + inThresh[patternStart]*inGroupStdEven[i]      < outMinEven[i]     ) &&
				(inGroupMeanOdd[(i+2)%5] + inThresh[patternStart]*inGroupStdOdd[(i+2)%5] < outMinOdd[(i+2)%5]) &&
				(inGroupMeanEven[i]      + inGroupStdEven[i]      < inGroupMeanOdd[i]       ) &&
				(inGroupMeanOdd[(i+2)%5] + inGroupStdOdd[(i+2)%5] < inGroupMeanEven[(i+2)%5]))
			{
#ifdef CODEC_DEBUG_32PULLDOWN
				fprintf(fp_log,"pretty clear pattern, i is %d, pulldown timer is %d, patternStart is %d\n",i,state->ulPulldownActiveTimerIntl,patternStart);
#endif
				// Set the removal pattern phase
				state->frameRemovalPattern=i;

				// If this is the right frame remove it!
				if(i == (PULLDOWN_HIST_LEN - 1) % 5)
				{
					// Set a counter that goes up if we have a consistent pattern 80+% of the time, down otherwise
					if ((timestamp - state->lastRemovedTimestamp > 145) &&
						(timestamp - state->lastRemovedTimestamp < 175))
					{
						if (state->ulPulldownActiveTimerIntl < 95)
							state->ulPulldownActiveTimerIntl += 5;
						else
						{
							state->ulPulldownActiveTimerIntl = 100;
							state->bInterlacedTelecineSeen = TRUE;
						}

						if (state->ulPulldownActiveTimerProg > 5)
							state->ulPulldownActiveTimerProg -= 5;
						else
							state->ulPulldownActiveTimerProg = 0;
					}
					else if (timestamp - state->lastRemovedTimestamp < 300)
					{
						if (state->ulPulldownActiveTimerIntl > 5)
							state->ulPulldownActiveTimerIntl -= 5;
						else
							state->ulPulldownActiveTimerIntl = 0;
					}
					state->lastRemovedTimestamp = timestamp;

					goto REMOVE_FRAME;
				}
				else if (i == ((PULLDOWN_HIST_LEN - 2) % 5))
				{
					obviousPatternFlag = TRUE;
					goto INTERLEAVE_ODD;
				}
				else 
					goto DO_NOTHING;
			}

			if ((state->ulPulldownActiveTimerIntl > 10) &&
				(inGroupMeanOdd[i]        + inThresh[patternStart]*inGroupStdOdd[i]        < outMinOdd[i]       ) &&
				(inGroupMeanEven[(i+2)%5] + inThresh[patternStart]*inGroupStdEven[(i+2)%5] < outMinEven[(i+2)%5]) &&
				(inGroupMeanOdd[i]        + inGroupStdOdd[i]        < inGroupMeanEven[i]     ) &&
				(inGroupMeanEven[(i+2)%5] + inGroupStdEven[(i+2)%5] < inGroupMeanOdd[(i+2)%5]))
			{
#ifdef CODEC_DEBUG_32PULLDOWN
				fprintf(fp_log,"pretty clear pattern, i is %d, pulldown timer is %d, patternStart is %d\n",i,state->ulPulldownActiveTimerIntl,patternStart);
#endif
				// Set the removal pattern phase
				state->frameRemovalPattern=i;

				// If this is the right frame remove it!
				if(i == (PULLDOWN_HIST_LEN - 1) % 5)
				{
					// Set a counter that goes up if we have a consistent pattern 80+% of the time, down otherwise
					if ((timestamp - state->lastRemovedTimestamp > 145) && 
						(timestamp - state->lastRemovedTimestamp < 175))
					{
						if (state->ulPulldownActiveTimerIntl < 95)
							state->ulPulldownActiveTimerIntl += 5;
						else
						{
							state->ulPulldownActiveTimerIntl = 100;
							state->bInterlacedTelecineSeen = TRUE;
						}

						if (state->ulPulldownActiveTimerProg > 5)
							state->ulPulldownActiveTimerProg -= 5;
						else
							state->ulPulldownActiveTimerProg = 0;
					}
					else if (timestamp - state->lastRemovedTimestamp < 300)
					{
						if (state->ulPulldownActiveTimerIntl > 5)
							state->ulPulldownActiveTimerIntl -= 5;
						else
							state->ulPulldownActiveTimerIntl = 0;
					}
					state->lastRemovedTimestamp = timestamp;

					goto REMOVE_FRAME;
				}
				else if (i == ((PULLDOWN_HIST_LEN - 2) % 5))
				{
					obviousPatternFlag = TRUE;
					goto INTERLEAVE_EVEN;
				}
				else 
					goto DO_NOTHING;
			}
		}
	}

	// Rule that maintains the pattern
	// No pattern, but things are VERY quiet, and we have been seeing a 3:2 pattern 
	if ((state->frameRemovalPattern == ((PULLDOWN_HIST_LEN-1)%5)) &&
		(state->ulPulldownActiveTimerIntl > 50) &&
		(inMaxEven [(PULLDOWN_HIST_LEN-1)%5] < 13) && 
		(inMaxOdd  [(PULLDOWN_HIST_LEN-1)%5] < 13) && 
		(outMaxOdd [(PULLDOWN_HIST_LEN-1)%5] < 13) &&
		(outMaxEven[(PULLDOWN_HIST_LEN-1)%5] < 13) && 
		(groupValidFlagEven[(PULLDOWN_HIST_LEN-1)%5] == TRUE) &&
		(groupValidFlagOdd[(PULLDOWN_HIST_LEN-1)%5] == TRUE))
	{
#ifdef CODEC_DEBUG_32PULLDOWN
		fprintf(fp_log,"maintain pattern 1 - things are very quiet, pulldown timer is %d, patternStart is %d\n",state->ulPulldownActiveTimerIntl,patternStart);
#endif
		state->lastRemovedTimestamp = timestamp;
		state->checkNextFrameForInterlace = TRUE;

		goto REMOVE_FRAME;
	}

	// Rule that maintains the pattern
	// If we have been seeing consistent pulldown, 
	// and the last frame looks a bit interlaced, but it is in the right place.
	// This is a weak test.
	if ((state->frameRemovalPattern == ((PULLDOWN_HIST_LEN-1)%5)) &&
		(state->ulPulldownActiveTimerIntl > 50) &&
		(!state->checkNextFrameForInterlace) &&
		(state->pulldownSadHistEven[(PULLDOWN_HIST_LEN-1)] < (.6*state->pulldownSadHistOdd[(PULLDOWN_HIST_LEN-1)]) ||
		 state->pulldownSadHistOdd[(PULLDOWN_HIST_LEN-1)] < (.6*state->pulldownSadHistEven[(PULLDOWN_HIST_LEN-1)])))
	{
#ifdef CODEC_DEBUG_32PULLDOWN
		fprintf(fp_log,"maintain pattern 2, pulldown timer is %d, patternStart is %d\n",state->ulPulldownActiveTimerIntl,patternStart);
#endif
		state->lastRemovedTimestamp = timestamp;
		state->checkNextFrameForInterlace = TRUE;
		goto REMOVE_FRAME;
	}

	// If we have been seeing consistent pulldown, and the last frame looks very interlaced.
	// This is a weak test
	if ((state->ulPulldownActiveTimerIntl > 50) &&
		(!state->checkNextFrameForInterlace) &&
		(
		 (state->pulldownSadHistEven[(PULLDOWN_HIST_LEN-1)] < (state->pulldownSadHistOdd[(PULLDOWN_HIST_LEN-1)]*.3)) &&
		 (state->pulldownSadHistOdd[(PULLDOWN_HIST_LEN-1)] > 9)
		) || (
		 (state->pulldownSadHistOdd[(PULLDOWN_HIST_LEN-1)] < (state->pulldownSadHistEven[(PULLDOWN_HIST_LEN-1)]*.3)) &&
		 (state->pulldownSadHistEven[(PULLDOWN_HIST_LEN-1)] > 9)
		)
	   )
	{
#ifdef CODEC_DEBUG_32PULLDOWN
		fprintf(fp_log,"things look interlaced, pulldown timer is %d, patternStart is %d\n",state->ulPulldownActiveTimerIntl,patternStart);
#endif
		state->lastRemovedTimestamp = timestamp;
		state->checkNextFrameForInterlace = TRUE;
		goto REMOVE_FRAME;
	}

	if(state->checkNextFrameForInterlace)
	{
#ifdef CODEC_DEBUG_32PULLDOWN
		fprintf(fp_log,"checking frame for interlace\n",i,state->ulPulldownActiveTimerIntl,patternStart);
#endif
		state->checkNextFrameForInterlace = FALSE;
		if(state->interleaveEvenFlag)
			goto INTERLEAVE_EVEN;
		if(state->interleaveOddFlag)
			goto INTERLEAVE_ODD;
	}

	// Otherwise no pulldown!
#ifdef CODEC_DEBUG_32PULLDOWN
	fprintf(fp_log,"no pattern, pulldown timer is %d, patternStart is %d\n",state->ulPulldownActiveTimerIntl,patternStart);
#endif
	if ((inMaxEven[(PULLDOWN_HIST_LEN-1)%5] > 30) || 
		(outMaxEven[(PULLDOWN_HIST_LEN-1)%5] > 30))
	{
		if (state->ulPulldownActiveTimerIntl > 0)
			state->ulPulldownActiveTimerIntl--;
	}

	// should we attempt progressive patterns?
	if ((lines > 242) || 
		(state->ulPulldownActiveTimerIntl > 90 && state->ulPulldownActiveTimerProg < 10))
	{
		goto NO_PATTERN;
	}

PROGRESSIVE_TESTS:

	// Now test to see if there is an entire repeated frame
	for (patternStart = histLength; patternStart < PULLDOWN_HIST_LEN; patternStart += 5)
	{
		for (i = 0; i < 5; i++)
		{
			inGroupMean[i]  = 0;
			outGroupMean[i] = 0;
			inGroupStd[i]   = 0;
			outGroupStd[i]  = 0;
			inGroupCount    = 0;
			outGroupCount   = 0;
			outMin[i] = 255*255;
			outMax[i] = 0;
			inMax[i]  = 0;

			for (j = patternStart + i; j < PULLDOWN_HIST_LEN; j++)
			{
				if (state->pulldownSadHistAll[j] == MISSING_SAD ||
					state->pulldownSadHistAll[j] == UN_INIT_SAD)
					continue;

				if (((j - i) % 5) == 0)
				{
					inGroupMean[i] += state->pulldownSadHistAll[j];
					inGroupStd[i] += state->pulldownSadHistAll[j] * state->pulldownSadHistAll[j];
					if (inMax[i] < state->pulldownSadHistAll[j])
						inMax[i] = state->pulldownSadHistAll[j];
					inGroupCount++;
				}
				else
				{
					outGroupMean[i] += state->pulldownSadHistAll[j];
					outGroupStd[i] += state->pulldownSadHistAll[j] * state->pulldownSadHistAll[j];
					if (outMin[i] > state->pulldownSadHistAll[j])
						outMin[i] = state->pulldownSadHistAll[j];
					if (outMax[i] < state->pulldownSadHistAll[j])
						outMax[i] = state->pulldownSadHistAll[j];
					outGroupCount++;
				}
			}
			if ((inGroupCount > 1) && (outGroupCount > 3))
			{
				groupValidFlag[i] = TRUE;

				inGroupMean[i] = inGroupMean[i]/inGroupCount;
				if ((inGroupStd[i]/inGroupCount)-(inGroupMean[i]*inGroupMean[i]) > 0.0f)
					inGroupStd[i] = (float)sqrt((inGroupStd[i]/inGroupCount)-(inGroupMean[i]*inGroupMean[i]));
				else
					inGroupStd[i] = 0.0f;

				outGroupMean[i] = outGroupMean[i]/outGroupCount;
				if ((outGroupStd[i]/outGroupCount)-(outGroupMean[i]*outGroupMean[i]) > 0.0f)
					outGroupStd[i] = (float)sqrt((outGroupStd[i]/outGroupCount)-(outGroupMean[i]*outGroupMean[i]));
				else
					outGroupStd[i] = 0.0f;
			}
			else
			{
				groupValidFlag[i] = FALSE;
				inGroupMean[i] = 0;
				outGroupMean[i] = 0;
				inGroupStd[i] = 1;
				outGroupStd[i] = 1;
			}
		}

		// Do we have a clear pattern? Always trust this test.
		for (i = 0;i < 5; i++)
		{
			if ((inGroupMean[i]+inThresh[patternStart]*inGroupStd[i] < outGroupMean[i] - outThresh[patternStart]*outGroupStd[i]) &&
				(groupValidFlag[i] == TRUE))
			{
#ifdef CODEC_DEBUG_32PULLDOWN
				fprintf(fp_log,"clear pattern, i is %d, pulldown timer is %d, patternStart is %d\n",i,state->ulPulldownActiveTimerProg,patternStart);
#endif
				// If this is the right frame remove it!
				state->frameRemovalPattern=i;
				if(i==(PULLDOWN_HIST_LEN-1)%5)
				{
					// Set a counter that goes up if we have a consistent pattern 80+% of the time, down otherwise
					if ((timestamp - state->lastRemovedTimestamp > 145) &&
						(timestamp - state->lastRemovedTimestamp < 175))
					{
						if (state->ulPulldownActiveTimerProg < 95)
							state->ulPulldownActiveTimerProg += 5;
						else
						{
							state->ulPulldownActiveTimerProg = 100;
							state->bProgressiveTelecineSeen = TRUE;
						}

						if (state->ulPulldownActiveTimerIntl > 5)
							state->ulPulldownActiveTimerIntl -= 5;
						else
							state->ulPulldownActiveTimerIntl = 0;
					}
					else if (timestamp - state->lastRemovedTimestamp < 300)
					{
						if (state->ulPulldownActiveTimerProg > 5)
							state->ulPulldownActiveTimerProg -= 5;
						else
							state->ulPulldownActiveTimerProg = 0;
					}
					state->lastRemovedTimestamp = timestamp;
					goto REMOVE_FRAME;
				}
				else 
					goto DO_NOTHING;
			}
		}

		// Do we have a pretty clear pattern? Only trust this test if we have succeeded with the strongest test a couple of times
		for(i = 0; i < 5; i++)
		{
			if ((inGroupMean[i]+inThresh[patternStart]*inGroupStd[i] < outMin[i]) &&
				(state->ulPulldownActiveTimerProg > 10) &&
				(groupValidFlag[i] == TRUE))
			{
#ifdef CODEC_DEBUG_32PULLDOWN
				fprintf(fp_log,"pretty clear pattern, i is %d, pulldown timer is %d, patternStart is %d\n",i,state->ulPulldownActiveTimerProg,patternStart);
#endif
				// If this is the right frame remove it!
				state->frameRemovalPattern = i;
				if (i == (PULLDOWN_HIST_LEN-1)%5)
				{
					// Set a counter that goes up if we have a consistent pattern 80+% of the time, down otherwise
					if ((timestamp - state->lastRemovedTimestamp > 145) && 
						(timestamp - state->lastRemovedTimestamp < 175))
					{
						if (state->ulPulldownActiveTimerProg < 95)
							state->ulPulldownActiveTimerProg += 5;
						else
						{
							state->ulPulldownActiveTimerProg = 100;
							state->bProgressiveTelecineSeen = TRUE;
						}

						if (state->ulPulldownActiveTimerIntl > 5)
							state->ulPulldownActiveTimerIntl -= 5;
						else
							state->ulPulldownActiveTimerIntl = 0;
					}
					else if (timestamp - state->lastRemovedTimestamp < 300)
					{
						if (state->ulPulldownActiveTimerProg > 5)
							state->ulPulldownActiveTimerProg -= 5;
						else
							state->ulPulldownActiveTimerProg = 0;
					}
					state->lastRemovedTimestamp = timestamp;
					goto REMOVE_FRAME;
				}
				else 
					goto DO_NOTHING;
			}
		}

		// No pattern, but things are VERY quiet, and we have been seeing a 3:2 pattern 
		if ((inMax[(PULLDOWN_HIST_LEN-1)%5] < 9) && (state->ulPulldownActiveTimerProg > 50) &&
			(outMax[(PULLDOWN_HIST_LEN-1)%5] < 9) && (groupValidFlag[(PULLDOWN_HIST_LEN-1)%5] == TRUE) &&
			(state->frameRemovalPattern==((PULLDOWN_HIST_LEN-1)%5))
		   )
		{
#ifdef CODEC_DEBUG_32PULLDOWN
			fprintf(fp_log,"things are quiet, pulldown timer is %d, patternStart is %d\n",state->ulPulldownActiveTimerProg,patternStart);
#endif
			state->lastRemovedTimestamp = timestamp;
			goto REMOVE_FRAME;
		}
	}

	// If we have been seeing consistent pulldown, and the last two frames have been quiet frames.
	// This is our weakest test
	if (((state->pulldownSadHistAll[(PULLDOWN_HIST_LEN-1)] < 9) ||
		(state->pulldownSadHistAll[(PULLDOWN_HIST_LEN-1)] < state->pulldownSadHistAll[(PULLDOWN_HIST_LEN-2)])) &&
		(state->ulPulldownActiveTimerProg > 50) &&
		(groupValidFlag[(PULLDOWN_HIST_LEN-1)%5] == TRUE)&&(state->frameRemovalPattern==((PULLDOWN_HIST_LEN-1)%5))
	   )
	{
#ifdef CODEC_DEBUG_32PULLDOWN
		fprintf(fp_log,"things are quiet, pulldown timer is %d, patternStart is %d\n",state->ulPulldownActiveTimerProg,patternStart);
#endif
		state->lastRemovedTimestamp = timestamp;
		goto REMOVE_FRAME;
	}


	// If we have been seeing consistent pulldown, and the last frame looks very interlaced.
	// This is a weak test
	if ((state->pulldownSadHistAll[(PULLDOWN_HIST_LEN-1)] < state->pulldownSadHistAll[(PULLDOWN_HIST_LEN-2)] * 0.5) &&
		(state->ulPulldownActiveTimerProg > 50) &&
		(groupValidFlag[(PULLDOWN_HIST_LEN-1)%5] == TRUE))
	{
		state->frameRemovalPattern = (PULLDOWN_HIST_LEN-1)%5;
#ifdef CODEC_DEBUG_32PULLDOWN
		fprintf(fp_log,"things look interlaced, pulldown timer is %d, patternStart is %d\n",state->ulPulldownActiveTimerProg,patternStart);
#endif
		state->lastRemovedTimestamp = timestamp;
		goto REMOVE_FRAME;
	}

	// Otherwise no pulldown!
#ifdef CODEC_DEBUG_32PULLDOWN
	fprintf(fp_log,"no pattern, pulldown timer is %d, patternStart is %d\n",state->ulPulldownActiveTimerProg,patternStart);
#endif

	if((inMax[(PULLDOWN_HIST_LEN-1)%5] > 30) || (outMax[(PULLDOWN_HIST_LEN-1)%5] >30))
		if (state->ulPulldownActiveTimerProg > 0)
			state->ulPulldownActiveTimerProg--;

NO_PATTERN:

	// If there was no clear pattern, do nothing
	state->checkNextFrameForProgressive = FALSE;
	return INVTELE_RESULT_NO_PATTERN;

INTERLEAVE_ODD:

	state->checkNextFrameForProgressive = FALSE;

	// Test if data should be re-interleaved.
	// Look at cross difference of old vs. re-stitched. Only replace if better
	if (obviousPatternFlag == FALSE || sceneChangeFlag == TRUE)
	{
		switch (state->impl_id)
		{
#ifdef ALLOW_MMX_INVTELE
		case INVTELE_IMPL_ID_MMX:
			{
				RestitchCheck_MMX(data, prevData, 
					pels, lines, pels, &sumOld, &sumNew);
			}
			break;
#endif
		case INVTELE_IMPL_ID_C:
		default:
			{
				pNew = (LONG32 *)(data);
				pOld = (LONG32 *)(prevData);
				for (i = 0; i < lines; i += 2)  //  only do the luma.
				{
					for(j = 0; j < ll; j++)
					{
						temp = (float)((pNew[j]&0xff00) - (pNew[j+ll]&0xff00));
						sumOld += (temp*temp);
						temp = (float)((pNew[j]&0xff00) - (pOld[j+ll]&0xff00));
						sumNew += (temp*temp);
					}
					pOld += 2*ll;
					pNew += 2*ll;
				}
			}
			break;
		}

		if (sumNew > sumOld)
		{
			return INVTELE_RESULT_FRAME_OK;
		}
	}

	state->interleaveOddFlag = TRUE;

#ifdef CODEC_DEBUG_32PULLDOWN
	fprintf(fp_log,"interleaving odd\n");
#endif

	// Re-interleave
	pNew = ((LONG32 *)data)+ll;
	pOld = ((LONG32 *)prevData)+ll;

	for (k = 1; k < lines; k += 2)  //  only do the luma.
	{
		memcpy(pNew,pOld,sizeof(ULONG32)*ll); /* Flawfinder: ignore */
		pOld+=2*ll;
		pNew+=2*ll;
	}

	return INVTELE_RESULT_FRAME_OK;

INTERLEAVE_EVEN:

	state->checkNextFrameForProgressive = FALSE;

	// Test if data should be re-interleaved.
	// Look at cross difference of old vs. re-stitched. Only replace if better

	if (obviousPatternFlag == FALSE || sceneChangeFlag == TRUE)
	{
		switch (state->impl_id)
		{
#ifdef ALLOW_MMX_INVTELE
		case INVTELE_IMPL_ID_MMX:
			{
				RestitchCheck_MMX(data + pels, prevData + pels, 
					pels, lines - 2, pels, &sumOld, &sumNew);
			}
			break;
#endif
		case INVTELE_IMPL_ID_C:
		default:
			{
				pNew = (LONG32 *)(data);
				pOld = (LONG32 *)(prevData);
				for (i = 0; i < lines; i += 2)  //  only do the luma.
				{
					for(j = 0; j < ll; j++)
					{
						temp = (float)((pNew[j]&0xff00) - (pNew[j+ll]&0xff00));
						sumOld += (temp*temp);
						temp = (float)((pNew[j+ll]&0xff00) - (pOld[j]&0xff00));
						sumNew += (temp*temp);
					}
					pOld+=2*ll;
					pNew+=2*ll;
				}
			}
			break;
		}

		if (sumNew > sumOld)
		{
			return INVTELE_RESULT_FRAME_OK;
		}
	}

	state->interleaveEvenFlag = TRUE;

#ifdef CODEC_DEBUG_32PULLDOWN
	fprintf(fp_log,"interleaving even\n");
#endif
	
	// Re-interleave
	pNew = ((LONG32 *)data);
	pOld = ((LONG32 *)prevData);

	for (k = 0; k < lines; k += 2)  //  only do the luma.
	{
		memcpy(pNew,pOld,sizeof(ULONG32)*ll); /* Flawfinder: ignore */
		pOld+=2*ll;
		pNew+=2*ll;
	}

	return INVTELE_RESULT_FRAME_OK;

DO_NOTHING:

	// One final check in this case
	// If we are here, we think the frame is progressive.
	// If we think the previous frame was also progressive, then
	// the odd and even SADs should be somewhat similar.  If not,
	// we're in trouble.

	if (state->checkNextFrameForProgressive)
	{
		if (state->ulPulldownActiveTimerProg < 10 &&
			state->ulPulldownActiveTimerIntl > 90 &&
			(sumEven > 8 * sumOdd || sumOdd > 8 * sumEven))
		{
			goto NO_PATTERN;
		}

		if (sceneChangeFlag == TRUE)
		{
			// Just return here because we want to be sure
			// state->checkNextFrameForProgressive is TRUE
			return INVTELE_RESULT_NO_PATTERN;
		}
	}

	state->checkNextFrameForProgressive = TRUE;

	return INVTELE_RESULT_FRAME_OK;

REMOVE_FRAME:

#ifdef CODEC_DEBUG_32PULLDOWN
	fprintf(fp_log,"removing frame\n");
#endif

	state->checkNextFrameForProgressive = FALSE;

	return INVTELE_RESULT_DROP_FRAME;

TOO_EARLY:

	state->checkNextFrameForProgressive = FALSE;

	return INVTELE_RESULT_TOO_EARLY;

}

// Platform specific SSD functions

#ifdef ALLOW_MMX_INVTELE

void
SumSqaredFrameDiff_MMX(
	unsigned char *frame, 
	unsigned char *prev_frame,
	int pels,
	int lines,
	int pitch,
	float *ssd_odd,
	float *ssd_even)
{
	unsigned int res_odd[2];
	unsigned int res_even[2];

	float pel_avg;

	const unsigned int mask[2] = {0x00FF00FF, 0x00FF00FF};
	int line_shift;
	int pels_div8;

	pels_div8 = (pels >> 3);			// round down
	line_shift = ((pitch) - (pels & ~7));

	__asm {

		mov			ecx, lines			// ecx -> lines		
		shr			ecx, 1				// ecx -> lines/2

		mov			esi, frame			// esi -> current frame
		mov			edi, prev_frame		// edi -> previous frame

		movq		mm5, [mask]
		pxor		mm6, mm6			// mm6 -> odd ssd
		pxor		mm7, mm7			// mm7 -> even ssd

ALIGN 16
line_loop:
		mov			ebx, pels_div8		// ebx -> pels/8 (rounded down)
ALIGN 16
pel_loop_even:
		movq		mm0,[esi]
		movq		mm1,[edi]

		pand		mm0, mm5
		pand		mm1, mm5

		psubw		mm0, mm1
		pmaddwd		mm0, mm0
		paddd		mm6, mm0

		lea			esi,[esi+8]		
		lea			edi,[edi+8]		

		dec			ebx
		jnz			pel_loop_even

		add			esi, line_shift;	// move esi to the next line
		add			edi, line_shift;	// move edi to the next line

		mov			ebx, pels_div8		// ebx -> pels/8 (rounded down)
ALIGN 16
pel_loop_odd:
		movq		mm0,[esi]
		movq		mm1,[edi]

		pand		mm0, mm5
		pand		mm1, mm5

		psubw		mm0, mm1
		pmaddwd		mm0, mm0
		paddd		mm7, mm0

		lea			esi,[esi+8]		
		lea			edi,[edi+8]		

		dec			ebx
		jnz			pel_loop_odd

		add			esi, line_shift;	// move esi to the next line
		add			edi, line_shift;	// move edi to the next line

		dec			ecx
		jnz			line_loop

		movq		res_odd, mm6
		movq		res_even, mm7

		emms
	}

	// fill "even" result
	pel_avg = (float)(res_even[0] + res_even[1]);
	pel_avg /= (float)((pels*lines)>>2);
	*ssd_even = pel_avg;

	// fill "odd" result
	pel_avg = (float)(res_odd[0] + res_odd[1]);
	pel_avg /= (float)((pels*lines)>>2);
	*ssd_odd = pel_avg;

}

void
RestitchCheck_MMX(
	unsigned char *frame, 
	unsigned char *prev_frame,
	int pels,
	int lines,
	int pitch,
	float *ssd_new,
	float *ssd_old)
{
	unsigned int res_new[2];
	unsigned int res_old[2];

	const unsigned int mask[2] = {0x00FF00FF, 0x00FF00FF};
	int line_shift;
	int pels_div8;

	pels_div8 = (pels >> 3);			// round down
	line_shift = ((pitch << 1) - (pels & ~7));

	__asm {

		mov			ecx, lines			// ecx -> lines		
		shr			ecx, 1				// ecx -> lines/2

		mov			esi, frame			// esi -> current frame
		mov			edi, prev_frame		// edi -> previous frame

		mov			edx, pitch			// pitch
		lea			edi, [edi+edx]		// edi -> previous frame + pitch

		movq		mm5, [mask]
		pxor		mm6, mm6			// mm6 -> odd ssd
		pxor		mm7, mm7			// mm7 -> even ssd

ALIGN 16
line_loop:
		mov			ebx, pels_div8		// ebx -> pels/8 (rounded down)
ALIGN 16
pel_loop:
		movq		mm0,[esi]
		movq		mm1,[esi + edx]
		movq		mm2,[edi]

		pand		mm0, mm5
		pand		mm1, mm5
		pand		mm2, mm5

		psubw		mm1, mm0
		pmaddwd		mm1, mm1
		paddd		mm6, mm1

		psubw		mm2, mm0
		pmaddwd		mm2, mm2
		paddd		mm7, mm2

		lea			esi, [esi+8]		
		lea			edi, [edi+8]		

		dec			ebx
		jnz			pel_loop

		add			esi, line_shift		// move esi down 2 lines
		add			edi, line_shift		// move edi down 2 lines

		dec			ecx
		jnz			line_loop

		movq		res_new, mm6
		movq		res_old, mm7

		emms
	}

	// fill "new" result
	*ssd_new = (float)(res_new[0] + res_new[1]);

	// fill "old" result
	*ssd_old = (float)(res_old[0] + res_old[1]);

}

#endif






















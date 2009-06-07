/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: invtelec.h,v 1.5 2005/03/14 19:24:45 bobclark Exp $
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


#ifndef INVTELEC_H__
#define INVTELEC_H__

#include "hxtypes.h"

#ifdef _M_IX86
#define ALLOW_MMX_INVTELE
#endif

#define T_INVTELE_FLOAT	float
#define T_INVTELE_RESULT ULONG32

#define INVTELE_RESULT_DROP_FRAME			0
#define INVTELE_RESULT_FRAME_OK				1
#define INVTELE_RESULT_TOO_EARLY			2
#define INVTELE_RESULT_LOW_FRAMERATE		3
#define INVTELE_RESULT_NO_PATTERN			4
#define INVTELE_RESULT_UNINITIALIZED		5

#define INVTELE_RESULT_PATTERN_FOUND(res)	\
	((res) == INVTELE_RESULT_DROP_FRAME ||	\
	 (res) == INVTELE_RESULT_FRAME_OK)

#define MISSING_SAD (-2.0f)
#define UN_INIT_SAD (-1.0f)

#define INVTELE_IMPL_ID_C		0
#define INVTELE_IMPL_ID_MMX		1

#define PULLDOWN_HIST_LEN 20

// Inverse-Telecine State
typedef struct tag_T_INVTELE_STATE
{
	ULONG32			impl_id;
	T_INVTELE_FLOAT	pulldownSadHistEven[PULLDOWN_HIST_LEN];
	T_INVTELE_FLOAT	pulldownSadHistOdd[PULLDOWN_HIST_LEN];
	T_INVTELE_FLOAT	pulldownSadHistAll[PULLDOWN_HIST_LEN];
	ULONG32			pulldownTimeHist[PULLDOWN_HIST_LEN];
	HXBOOL			firstFrame;
	ULONG32			ulPulldownActiveTimerIntl;
	ULONG32			ulPulldownActiveTimerProg;
	ULONG32			lastRemovedTimestamp;
	INT32			frameCountMod;
	INT32			frameRemovalPattern;
	INT32			NTSCTrackingFrameCounter;
	HXBOOL			interleaveEvenFlag;
	HXBOOL			interleaveOddFlag;
	HXBOOL			checkNextFrameForInterlace;
	HXBOOL			checkNextFrameForProgressive;
	ULONG32			pulldownTimeBuffer;
	HXBOOL			bInterlacedTelecineSeen;
	HXBOOL			bProgressiveTelecineSeen;

} T_INVTELE_STATE;


// Inverse Telecine Interface
INT32
InitInvTelecine(T_INVTELE_STATE **state);

void
FreeInvTelecine(T_INVTELE_STATE **state);

T_INVTELE_RESULT 
DoInvTelecine(
	UCHAR *data, 
	UCHAR *prevData, 
	double frameRate, 
	ULONG32 &timestamp, 
	ULONG32 pels, ULONG32 lines, 
	HXBOOL bDeInterlaced, 
	T_INVTELE_STATE *state);

UCHAR
GetTelecinePattern(T_INVTELE_STATE *state);

void
SetTelecinePattern(T_INVTELE_STATE *state, UCHAR telecine_pattern);

HXBOOL
IsContentProgressiveTelecine(T_INVTELE_STATE *state);

HXBOOL
IsContentInterlacedTelecine(T_INVTELE_STATE *state);

#endif


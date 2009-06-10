/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: deintl.h,v 1.7 2007/07/06 22:00:24 jfinnecy Exp $
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


#ifndef DEINTL_H__
#define DEINTL_H__

#include "hxtypes.h"

// color formats
#define INTL_FORMAT_UNDEF			0
#define INTL_FORMAT_RGB24			1
#define INTL_FORMAT_I420			2

// Deinterlace mode
#define INTL_MODE int

#define INTL_MODE_NONE				0
#define INTL_MODE_ODD				1
#define INTL_MODE_EVEN				2
#define INTL_MODE_SMART				3
#define INTL_MODE_FLIPFLOP			4
#define INTL_MODE_SMART_AUTO		5

// Deinterlace detection results
#define INTL_STRONG_INTERLACE		(2)
#define INTL_WEAK_INTERLACE			(1)
#define INTL_NO_DETECTION			(0)
#define INTL_WEAK_PROGRESSIVE		(-1)
#define INTL_STRONG_PROGRESSIVE		(-2)

// line removal modes
#define INTL_LINE_REMOVE_MEDIAN		0
#define INTL_LINE_REMOVE_AVG		1
#define INTL_LINE_REMOVE_DOWN		2
#define INTL_LINE_REMOVE_UP			3

// misc
#ifdef TRUE
#undef TRUE
#endif

#define TRUE 1

#ifdef FALSE
#undef FALSE
#endif

#define FALSE 0

// deinterlace filter state
typedef struct tag_T_DEINTL_STATE
{
	int				pels;
	int				lines;
	int				pitch;
	int				format;
	unsigned int	detection_measure;
	unsigned int	commit_deinterlace;
} T_DEINTL_STATE;

// prototypes
int
InitDeinterlace(
	int pels, int lines, int pitch, 
	int format, 
	INTL_MODE mode, 
	T_DEINTL_STATE **state);

int
Deinterlace(
	unsigned char *frame, 
	unsigned char *prev_frame, 
	int pels, int lines, int pitch,
	int format,
	int first_frame, 
	INTL_MODE mode,
	T_DEINTL_STATE *state);

HXBOOL
IsContentInterlaced(T_DEINTL_STATE *state);

void
ResetDeinterlace (T_DEINTL_STATE *state);

void
FreeDeinterlace(T_DEINTL_STATE **state);

#ifdef TIME_DEINTERLACE

#include "hlxclib/windows.h"

// Macros for timing

// USE_CODEC_TIMER

#if (defined _WIN32)
#define USE_CODEC_TIMER \
		LARGE_INTEGER tick1,tick2,freq; \
		QueryPerformanceFrequency(&freq)
#endif

#if (defined _MACINTOSH)
#define USE_CODEC_TIMER UnsignedWide tick1,tick2
#endif

#ifndef USE_CODEC_TIMER
#define USE_CODEC_TIMER int tick1,tick2
#endif

// START_CODEC_TIMER

#if (defined _WIN32)
#define START_CODEC_TIMER QueryPerformanceCounter(&tick1)
#endif

#if (defined _MACINTOSH)
#define START_CODEC_TIMER Microseconds(&tick1)
#endif

#ifndef START_CODEC_TIMER
#define START_CODEC_TIMER tick1 = HX_GET_TICKCOUNT()
#endif

// STOP_CODEC_TIMER

#if (defined _WIN32)
#define STOP_CODEC_TIMER(d) \
		QueryPerformanceCounter(&tick2); \
		(d) = (1000.*((tick2.LowPart + 4294967296.0*tick2.HighPart)-(tick1.LowPart + 4294967296.0*tick1.HighPart))/ \
			(freq.LowPart + 4294967296.0*freq.HighPart))
#endif

#if (defined _MACINTOSH)
#define STOP_CODEC_TIMER(d) \
		Microseconds(&tick2); \
		(d) = ((double)((tick2.lo + 4294967296.0*tick2.hi)-(tick1.lo + 4294967296.0*tick1.hi))/1000)
#endif

#ifndef STOP_CODEC_TIMER
#define STOP_CODEC_TIMER(d) \
		tick2 = HX_GET_TICKCOUNT(); \
		(d) = tick2-tick1
#endif


#endif //TIME_DEINTERLACE

#endif





































































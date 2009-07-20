/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mtime.cpp,v 1.5 2005/03/14 19:35:26 bobclark Exp $
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

#include "hxtypes.h"
#include "hxtime.h"
#include "hxassert.h"

#include <limits.h>
#ifndef _MAC_MACHO
#include <Timer.h>
#endif
#include <time.h>

#define kMaxMicroSecondComponent	ULONG_MAX
#define kLoMicroSecondsPerSecond	1000000
#define kSecondsPerHiMicroSecond	( kMaxMicroSecondComponent / kLoMicroSecondsPerSecond )
#define kLoMicroSecondsRoundOff		( kMaxMicroSecondComponent % kLoMicroSecondsPerSecond )

static HXBOOL gHasCalculatedReferenceTime = FALSE;
static time_t gReferenceTime;
static UnsignedWide gReferenceMicroSeconds;

int gettimeofday(HXTime* pTime, void* unused)
{
	// (?) Warning: Overflow not currently checked.
	if ( !gHasCalculatedReferenceTime )
	{
    	// convert to Windows time ( secs since 1/1/70 )
    	struct tm the1970Time = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    	the1970Time.tm_year = 70;
    	the1970Time.tm_mday = 1;
    	the1970Time.tm_mon  = 0;
    	time_t theWinTime = mktime( &the1970Time );
        time_t theMacTime;
        
        UnsignedWide theRefMicroSecs1, theRefMicroSecs2;
        Microseconds( &theRefMicroSecs1 );
        time( &theMacTime ); // mac time in secs since 1/1/00
        Microseconds( &theRefMicroSecs2 );

		/* Calculate the average of theRefMicroSecs1 and theRefMicroSecs2
		   to correspond with the time calculated with time(). */
    	gReferenceTime = theMacTime - theWinTime;
    	gReferenceMicroSeconds.lo =     ( theRefMicroSecs1.lo >> 1 ) + ( theRefMicroSecs2.lo >> 1 ) +
    								( ( ( theRefMicroSecs1.lo  & 1 ) + ( theRefMicroSecs2.lo  & 1 ) ) >> 1 );
    	gReferenceMicroSeconds.hi =     ( theRefMicroSecs1.hi >> 1 ) + ( theRefMicroSecs2.hi >> 1 ) +
    								( ( ( theRefMicroSecs1.hi  & 1 ) + ( theRefMicroSecs2.hi  & 1 ) ) >> 1 );
    	
    	pTime->tv_sec  = gReferenceTime;
    	pTime->tv_usec = 0;
    	
    	gHasCalculatedReferenceTime = TRUE;
    }
    else
    {
    	UnsignedWide theCurrentMicroSecs, theDifferenceMicroSecs;
    	Microseconds( &theCurrentMicroSecs );
    	HX_ASSERT(   ( theCurrentMicroSecs.hi >  gReferenceMicroSeconds.hi ) ||
    			   ( ( theCurrentMicroSecs.hi == gReferenceMicroSeconds.hi ) &&
    			   	 ( theCurrentMicroSecs.lo >= gReferenceMicroSeconds.lo ) ) );
    	
    	// Calculate theDifferenceMicroSecs = theCurrentMicroSecs - gReferenceMicroSeconds;
    	HXBOOL hasLoComponentRolledOver;
    	if ( theCurrentMicroSecs.lo >= gReferenceMicroSeconds.lo )
    	{
    		theDifferenceMicroSecs.lo = theCurrentMicroSecs.lo - gReferenceMicroSeconds.lo;
    		hasLoComponentRolledOver = FALSE;
    	}
    	else
    	{
    		theDifferenceMicroSecs.lo = kMaxMicroSecondComponent - gReferenceMicroSeconds.lo + theCurrentMicroSecs.lo;
    		hasLoComponentRolledOver = TRUE;
    	}
    	theDifferenceMicroSecs.hi = theCurrentMicroSecs.hi - gReferenceMicroSeconds.hi;
    	if ( hasLoComponentRolledOver )
    	{
    		theDifferenceMicroSecs.hi--;
    	}
    	
    	// Return Seconds and Microseconds portions of theDifferenceMicroSecs added to the gReferenceTime.
    	pTime->tv_sec  = gReferenceTime + ( theDifferenceMicroSecs.lo / kLoMicroSecondsPerSecond );
    	pTime->tv_usec = theDifferenceMicroSecs.lo % kLoMicroSecondsPerSecond;

		if ( theDifferenceMicroSecs.hi > 0 )
		{
			ULONG32 theMicroSecondRoundOff = theDifferenceMicroSecs.hi * kLoMicroSecondsRoundOff;
			pTime->tv_sec  += theDifferenceMicroSecs.hi * kSecondsPerHiMicroSecond;
			pTime->tv_sec  += theMicroSecondRoundOff / kLoMicroSecondsPerSecond;
			pTime->tv_usec += theMicroSecondRoundOff % kLoMicroSecondsPerSecond;
			pTime->tv_sec  += pTime->tv_usec / kLoMicroSecondsPerSecond;
			pTime->tv_usec %= kLoMicroSecondsPerSecond;
		}
    }
	return 0;
		

}

time_t WinToMacTime(time_t timeT)
{
    // convert to Mac time (secs since 1/1/04)
    struct tm t1970 = {0,0,0,0,0,0,0,0,0};
    t1970.tm_year=70;
    t1970.tm_mday=1;
    t1970.tm_mon=0;
    time_t macT = mktime(&t1970);
    return (macT+timeT);
}

time_t MacToWinTime(time_t timeT)
{
    // convert to Win time (secs since 1/1/70)
    struct tm t1970 = {0,0,0,0,0,0,0,0,0};
    t1970.tm_year=70;
    t1970.tm_mday=1;
    t1970.tm_mon=0;
    time_t winT = mktime(&t1970);
    return (timeT-winT);
}

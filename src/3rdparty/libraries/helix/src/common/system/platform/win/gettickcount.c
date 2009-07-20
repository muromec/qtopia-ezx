/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: gettickcount.c,v 1.5 2005/03/14 19:35:26 bobclark Exp $
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
#include "hxtick.h"

#ifdef _WIN32

/*
 * In 32bit Windows we have a HX_GET_BETTERTICKCOUNT()
 * which uses the queryPerformance interface for highly accurate times.
 * XXXKB: This implementation is now thread safe;
 */
ULONG32 GetBetterTickCount()
{
    static ULARGE_INTEGER s_ullPerformanceFrequency = { 0, 0 };
    static HXBOOL s_bFrequencyChecked = FALSE;
    ULARGE_INTEGER ullPerformanceCounter;
    
    do
    {
        if (s_ullPerformanceFrequency.LowPart)
        {
            if (QueryPerformanceCounter((union _LARGE_INTEGER*)
                                          &ullPerformanceCounter))
            {
                return (ULONG32) ((ullPerformanceCounter.QuadPart*1000) / 
                                  s_ullPerformanceFrequency.QuadPart);
            }
        }

        if (!s_bFrequencyChecked)
        {
            // Hopefully this is atomic kernel call:
            if (!QueryPerformanceFrequency((union _LARGE_INTEGER*)
                                           &s_ullPerformanceFrequency)
                // If the frequency > 32 bits, we lose range
                || s_ullPerformanceFrequency.HighPart 
                // If the frequency < 1000, we lose precision
                || (s_ullPerformanceFrequency.LowPart < 1000))
            {
                s_ullPerformanceFrequency.LowPart = 0;
            }

            s_bFrequencyChecked = TRUE;
        }
        
    } while (s_ullPerformanceFrequency.LowPart);
    
    return GetTickCount();
}

/*
 * In 32bit Windows we have a GetMSTickDouble32()
 * which uses the queryPerformance interface for highly accurate times.
 * XXXKB: This implementation is thread safe, returns milliseconds with
 * double precision.  We still wrap around at 2^32 -1 milliseconds.
 * We will have wraparound error if (maxCounter*1000 / maxFrequency) % (2^32 - 1)
 */                                 
#ifndef MAX_ULONG32_AS_DOUBLE
#define MAX_ULONG32_AS_DOUBLE	((double) ((ULONG32) 0xFFFFFFFF))
#endif

double GetMSTickDouble32()
{
    static double s_dFrequency = 0;
    static HXBOOL s_bFrequencyChecked = FALSE;
    double dTick;
    UINT32 ulQuotient;

    do
    {
        if (s_dFrequency)
        {
            ULARGE_INTEGER ullPerformanceCounter;

            if (QueryPerformanceCounter((union _LARGE_INTEGER*)
                                         &ullPerformanceCounter))
            {
                double dPerformanceCounter = 4294967296.0;
                dPerformanceCounter *= ullPerformanceCounter.HighPart;
                dPerformanceCounter += ullPerformanceCounter.LowPart;                         
                    
                // Convert out of range doubles here; is there a standard 
                // conversion from an out of range float64 to a uint32?
                // We can't use fmod here; it's not in the pncrt nor in
                // every Windows runtime:
                dTick = dPerformanceCounter*1000 / s_dFrequency;
                ulQuotient = (UINT32) (dTick / (MAX_ULONG32_AS_DOUBLE + 1));
                return dTick - (ulQuotient*(MAX_ULONG32_AS_DOUBLE + 1));
            }
        }

        if (!s_bFrequencyChecked)
        {
            ULARGE_INTEGER ullPerformanceFrequency;

            // Hopefully this is atomic kernel call:
            if (QueryPerformanceFrequency((union _LARGE_INTEGER*)
                                           &ullPerformanceFrequency)
                // If the frequency > 32 bits, we lose range
                && !(ullPerformanceFrequency.HighPart 
                     // If the frequency < 1000, we lose precision
                     || (ullPerformanceFrequency.LowPart < 1000)))
            {
                s_dFrequency = 4294967296.0;
                s_dFrequency *= ullPerformanceFrequency.HighPart;
                s_dFrequency += ullPerformanceFrequency.LowPart;
            }

            s_bFrequencyChecked = TRUE;
        }
        
    } while (s_dFrequency);
    
    return GetTickCount();
}

// return microseconds
UINT32
GetTickCountInUSec()
{
    static ULARGE_INTEGER s_ullPerformanceFrequency = { 0, 0 };
    static HXBOOL s_bFrequencyChecked = FALSE;
    ULARGE_INTEGER ullPerformanceCounter;
    
    do
    {
        if (s_ullPerformanceFrequency.LowPart)
        {
            if (QueryPerformanceCounter((union _LARGE_INTEGER*)
                                          &ullPerformanceCounter))
            {
                return (DWORD) ((ullPerformanceCounter.QuadPart*1000*1000) / 
                                  s_ullPerformanceFrequency.QuadPart);
            }
        }

        if (!s_bFrequencyChecked)
        {
            // Hopefully this is atomic kernel call:
            if (!QueryPerformanceFrequency((union _LARGE_INTEGER*)
                                           &s_ullPerformanceFrequency)
                // If the frequency > 32 bits, we lose range
                || s_ullPerformanceFrequency.HighPart 
                // If the frequency < 1000, we lose precision
                || (s_ullPerformanceFrequency.LowPart < 1000))
            {
                s_ullPerformanceFrequency.LowPart = 0;
            }

            s_bFrequencyChecked = TRUE;
        }
        
    } while (s_ullPerformanceFrequency.LowPart);
    
    return GetTickCount();
}
#endif // _WIN32

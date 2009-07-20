/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: gettickcount.c,v 1.10 2008/03/27 15:38:25 gajia Exp $
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

#include <sys/time.h>
#include <unistd.h>
#ifdef HELIX_CONFIG_USE_NTICKCOUNT
#include <hal.h>
#endif
#include "hxtypes.h"
#include "hxtick.h"
#include "globals/hxglobals.h"

ULONG32 GetTickCount()
{
#ifdef HELIX_CONFIG_USE_NTICKCOUNT
    TInt nanokernel_tick_period;
    //call HAL::Get to get the number of USeconds in one tick, it's hardware related
    HAL::Get(HAL::ENanoTickPeriod, nanokernel_tick_period);
    //we need re-visit using the NTickCount in the future, currently only the period=1000 is taking this.
    //For other cases the NTickCount can overflow faster/slower than 2^32 ms. Period=1000 is true on most of the hardwares
    if (nanokernel_tick_period==1000)
    {
        //This is the current value of the machine's millisecond tick counter.
        return User::NTickCount();
    }
    else
    {
        struct timeval tv;
        gettimeofday( &tv, NULL );
        return (ULONG32)((tv.tv_sec) * 1000 + tv.tv_usec / 1000);
    }
#else
    struct timeval tv;
    gettimeofday( &tv, NULL );
    return (ULONG32)((tv.tv_sec) * 1000 + tv.tv_usec / 1000);
#endif
}

// return microseconds
UINT32
GetTickCountInUSec()
{
    struct timeval tv;
    gettimeofday( &tv, NULL );
    return (ULONG32)((tv.tv_sec) * 1000 * 1000 + tv.tv_usec);
}

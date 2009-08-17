/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxnetutil.cpp,v 1.7 2006/08/16 17:50:57 gwright Exp $
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

#include "hxnetutil.h"
#include "hxprefs.h"  // IHXPreferences
#include "ihxpckts.h" // IHXBuffer
#include "hxcore.h"   // IHXClientEngine

#if defined(HELIX_FEATURE_PREFERENCES)
#include "hxprefs.h"
#include "hxprefutil.h"
#endif /* HELIX_FEATURE_PREFERENCES */

#include <stdlib.h>

HX_RESULT ConvertNetworkError(HX_RESULT theErr)
{
    HX_RESULT lResult = theErr;

    if (!IS_SERVER_ALERT(theErr))
    {
        switch(theErr)
        {
            case HXR_AT_INTERRUPT:                      // mask out these errors
            case HXR_WOULD_BLOCK:
            case HXR_NO_DATA:
            case HXR_OK:
                lResult = HXR_OK;
                break;

            case HXR_DNR:
            case HXR_DOC_MISSING:
            case HXR_OUTOFMEMORY:
            case HXR_ADVANCED_SERVER:
            case HXR_BAD_SERVER:
            case HXR_OLD_SERVER:
            case HXR_INVALID_FILE:
            case HXR_REDIRECTION:
            case HXR_PROXY:
            case HXR_PROXY_RESPONSE:
            case HXR_ADVANCED_PROXY:
            case HXR_OLD_PROXY:
            case HXR_PERFECTPLAY_NOT_SUPPORTED:
            case HXR_NO_LIVE_PERFECTPLAY:
            case HXR_PERFECTPLAY_NOT_ALLOWED:
            case HXR_NET_SOCKET_INVALID:
            case HXR_GENERAL_NONET:
            case HXR_BLOCK_CANCELED:
            case HXR_MSG_TOOLARGE:
            case HXR_SERVER_DISCONNECTED:
            case HXR_BAD_TRANSPORT:
                break;

            default:
                lResult = HXR_SERVER_DISCONNECTED;
                break;
        }
    }

    return lResult;
}

#if defined( _UNIX )

HXBOOL ReadNetworkThreadingPref( IUnknown* pContext )
{
    static HXBOOL bNetworkThreading = TRUE;

#if defined(HELIX_FEATURE_PREFERENCES)
    
    static HXBOOL bNeedToLoad       = TRUE;
    if( bNeedToLoad && NULL!=pContext)
    {
        ReadPrefBOOL(pContext, "NetworkThreading", bNetworkThreading );
    }

#endif /* HELIX_FEATURE_PREFERENCES */

    return bNetworkThreading;
}

#endif

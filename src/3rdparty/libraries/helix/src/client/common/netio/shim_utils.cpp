/* ***** BEGIN LICENSE BLOCK *****
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
#include "hxnet.h"
#include "hxengin.h"
#include "shim_utils.h"

HX_RESULT CHXShimUtils::TranslateFromHXSockOpt(HXSockOpt eIn, REF(HX_SOCKET_OPTION) rulOut)
{
    HX_RESULT retVal = HXR_OK;

    switch (eIn)
    {
        case HX_SOCKOPT_REUSEADDR :       rulOut = HX_SOCKOPT_REUSE_ADDR;       break;
        case HX_SOCKOPT_BCAST:            rulOut = HX_SOCKOPT_BROADCAST;        break;
        case HX_SOCKOPT_RCVBUF:           rulOut = HX_SOCKOPT_SET_RECVBUF_SIZE; break;
        case HX_SOCKOPT_SNDBUF:           rulOut = HX_SOCKOPT_SET_SENDBUF_SIZE; break;
        case HX_SOCKOPT_IN4_MULTICAST_IF: rulOut = HX_SOCKOPT_MULTICAST_IF;     break;
        case HX_SOCKOPT_IN4_TOS:          rulOut = HX_SOCKOPT_IP_TOS;           break;
        default: retVal = HXR_FAIL;
    }

    return retVal;
}

UINT32 CHXShimUtils::SwapUINT32(UINT32 ulVal)
{
    UINT32 ulRet = ((ulVal & 0xFF000000) >> 24) |
                   ((ulVal & 0x00FF0000) >>  8) |
                   ((ulVal & 0x0000FF00) <<  8) |
                   ((ulVal & 0x000000FF) << 24);
    return ulRet;
}

UINT16 CHXShimUtils::SwapUINT16(UINT16 usVal)
{
    UINT16 usRet = ((usVal & 0xFF00) >> 8) |
                   ((usVal & 0x00FF) << 8);
    return usRet;
}

/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: trmimemp.cpp,v 1.8 2007/07/06 20:51:32 jfinnecy Exp $
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

#include "hxcom.h"
#include <stdlib.h>
#include "hxtypes.h"
#include "hxstring.h"
#include "hxslist.h"
#include "hxengin.h"
#include "ihxpckts.h"
#include "chxpckts.h"
#include "mimehead.h"
#include "mimescan.h"
#include "timerep.h"
#include "basepkt.h"
#include "rtspbase.h"
#include "dbcs.h" // for HXIsEqual
#include "trmimemp.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

const RTSPTransportMimeType RTSPTransportMimeTypeTable[] =
{
    { RTSP_TR_RDT_MCAST, RTSP_SUBTR_MCAST, "x-real-rdt/mcast" },
    { RTSP_TR_RDT_UDP, RTSP_SUBTR_UDP, "x-real-rdt/udp" },
    { RTSP_TR_RDT_TCP, RTSP_SUBTR_TCP, "x-real-rdt/tcp" },
    { RTSP_TR_TNG_UDP, RTSP_SUBTR_UDP, "x-pn-tng/udp" },
    { RTSP_TR_TNG_TCP, RTSP_SUBTR_TCP, "x-pn-tng/tcp" },
    { RTSP_TR_RTP_UDP,	    RTSP_SUBTR_NONE,	"RTP/AVP;unicast" },
    { RTSP_TR_RTP_UDP,	    RTSP_SUBTR_UDP,	"RTP/AVP/UDP;unicast" },
    { RTSP_TR_RTP_MCAST,    RTSP_SUBTR_NONE,	"RTP/AVP;multicast" },
    { RTSP_TR_RTP_MCAST,    RTSP_SUBTR_MCAST,	"RTP/AVP/UDP;multicast" },
    { RTSP_TR_RTP_MCAST,    RTSP_SUBTR_NONE,	"RTP/AVP" },
    { RTSP_TR_RTP_MCAST,    RTSP_SUBTR_NONE,	"RTP/AVP/UDP" },
    { RTSP_TR_RTP_TCP,	    RTSP_SUBTR_TCP,	"RTP/AVP/TCP;unicast" },
    { RTSP_TR_RTP_TCP,	    RTSP_SUBTR_TCP,	"RTP/AVP/TCP" },
    { RTSP_TR_NULLSET,      RTSP_SUBTR_NULLSET, "x-real-nullsetup" },
    { RTSP_TR_BCNG_UDP,     RTSP_SUBTR_UDP,     "x-real-bcng/udp" },
    { RTSP_TR_BCNG_MCAST,   RTSP_SUBTR_MCAST,   "x-real-bcng/mcast" },
    { RTSP_TR_BCNG_TCP,     RTSP_SUBTR_TCP,     "x-real-bcng/tcp" }
};

const char*
RTSPTransportMimeMapper::getTransportMimeType(RTSPTransportTypeEnum tType)
{
    int tabSize = sizeof(RTSPTransportMimeTypeTable) /
		    sizeof(RTSPTransportMimeTypeTable[0]);
    for (int i = 0; i < tabSize; ++i)
    {
	if (RTSPTransportMimeTypeTable[i].m_lTransportType == tType)
	{
	    return RTSPTransportMimeTypeTable[i].m_pMimeType;
	}
    }    
    return "";
}

RTSPTransportTypeEnum
RTSPTransportMimeMapper::getTransportType(const char* pMimeType)
{
    int tabSize = sizeof(RTSPTransportMimeTypeTable) /
        sizeof(RTSPTransportMimeTypeTable[0]);
    for (int i=0; i<tabSize; ++i)
    {
	if (strcasecmp(RTSPTransportMimeTypeTable[i].m_pMimeType,
                       pMimeType) == 0)
	{
	    return RTSPTransportMimeTypeTable[i].m_lTransportType;
	}
    }    
    return RTSP_TR_NONE;
}

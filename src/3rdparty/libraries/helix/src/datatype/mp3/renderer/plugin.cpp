/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#define INITGUID
// system
#include "hlxclib/stdio.h"
// include
#include "hxcom.h"              // IUnknown
#include "hxcomm.h"            // IHXCommonClassFactory
#include "ihxpckts.h"           // IHXBuffer, IHXPacket, IHXValues
#include "hxplugn.h"           // IHXPlugin
#include "hxrendr.h"           // IHXRenderer
#include "hxengin.h"           // IHXInterruptSafe
#include "hxcore.h"            // IHXStream
#include "hxausvc.h"           // Audio Services
#include "hxmon.h"             // IHXStatistics
#include "hxupgrd.h"           // IHXUpgradeCollection
#include "rmfftype.h"		// AudioTypeSpecificData
#include "hxprefs.h"
#include "hxcore.h"		// IHXUpdateProperties
// pnmisc
#include "hxstrutl.h"           // strcasecmp()
// rmpadec
#include "mpadecobj.h"          // MPEG Audio Decoder (selects fixed-pt or floating-pt based on HELIX_CONFIG_FIXEDPOINT)
// rmp3lib
#include "mp3format.h"          // MP3 formatter
// mpgcommon
#ifdef DEMUXER
#include "xmddemuxer.h"         // Demuxer
#include "xmdtypes.h"
#endif
// mp3rend
#include "mp3rend.h"           // CRnMp3Ren
#include "pktparse.h"           // CPacketParser
#if defined (MPA_FMT_RAW)
#include "mpapktparse.h"        // CMpaPacketParser, C2250PacketParser
#endif /* #if defined (MPA_FMT_RAW) */
#if defined(MPA_FMT_DRAFT00)
#include "fmtpktparse.h"        // CFmtPacketParser
#endif /* #if defined(MPA_FMT_DRAFT00) */
#if defined(MPA_FMT_RFC3119)
#include "robpktparse.h"        // CRobustPacketParser
#endif /* #if defined(MPA_FMT_RFC3119) */
#include "hxdefpackethookhlp.h"

#include "ihxtlogsystem.h"
#include "ihxtlogsystemcontext.h"
#include "hxdllaccess.h"

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(HXCREATEINSTANCE)(IUnknown** ppExRendererObj)
{
    *ppExRendererObj = (IUnknown*)(IHXPlugin*)new CRnMp3Ren();
	if (*ppExRendererObj != NULL)
	{
		(*ppExRendererObj)->AddRef();
		return HXR_OK;
	}

	return HXR_OUTOFMEMORY;
}

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(CanUnload)(void)
{
    return (CHXBaseCountingObject::ObjectsActive() > 0 ? HXR_FAIL : HXR_OK );
}

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(CanUnload2)(void)
{
    return ENTRYPOINT(CanUnload)();
}


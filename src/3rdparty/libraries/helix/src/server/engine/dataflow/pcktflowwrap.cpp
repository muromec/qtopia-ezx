/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: pcktflowwrap.cpp,v 1.17 2008/03/24 06:58:37 manvendras Exp $
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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
#include "hxcom.h"
#include "hxerror.h"
#include "proc.h"
#include "server_engine.h"
#include "ihxpckts.h"
#include "hxdtcvt.h"
#include "hxformt.h"
#include "hxasm.h"
#include "source.h"
#include "hxassert.h"
#include "hxslist.h"
#include "hxmap.h"
#include "servpckts.h"
#include "base_errmsg.h"
#include "asmrulep.h"
#include "server_info.h"
#include "servreg.h"
#include "bwcalc.h"
#include "bandcalc.h"
#include "mutex.h"
#include "loadinfo.h"

#include "dtcvtcon.h"
#include "strmdata.h"
#include "mem_cache.h"
#include "hxpiids.h"
#include "globals.h"
#include "ppm.h"

#include "hxpcktflwctrl.h"
#include "pcktflowmgr.h"
#include "pcktflowwrap.h"

PacketFlowWrapper::PacketFlowWrapper(Process* pProc, BOOL bIsRTP) :
    m_pProc(pProc),
    m_bIsRTP(bIsRTP),
    m_pPPM(0),
    m_pFlowMgr(0)
{
}

PacketFlowWrapper::~PacketFlowWrapper()
{
    HX_DELETE(m_pPPM);
    HX_DELETE(m_pFlowMgr);
}

HX_RESULT
PacketFlowWrapper::RegisterSource(IUnknown* pSourceCtrl,
                                  REF(IHXPacketFlowControl*) pPacketFlowControl,
                                  IHXSessionStats* pSessionStats,
                                  UINT16 unStreamCount,
                                  BOOL bUseMDP,
                                  BOOL bIsLive,
                                  BOOL bIsMulticast,
                                  DataConvertShim* pDataConv,
                                  BOOL bIsFCS)
{
    if (!bUseMDP)
    {
        if (!m_pPPM)
        {
            m_pPPM = new PPM(m_pProc, m_bIsRTP);
        }

        m_pPPM->RegisterSource(pSourceCtrl, &pPacketFlowControl, unStreamCount,
                               bIsLive, bIsMulticast, pDataConv, bIsFCS);
    }
    else
    {
        if (!m_pFlowMgr)
        {
            m_pFlowMgr = new PacketFlowManager(m_pProc, m_bIsRTP);
        }

        m_pFlowMgr->RegisterSource(pSourceCtrl, &pPacketFlowControl, pSessionStats, unStreamCount,
                                   bIsLive, bIsMulticast, pDataConv, bIsFCS);
    }

    return HXR_OK;
}
    
HX_RESULT
PacketFlowWrapper::RegisterSource(IUnknown* pSourceCtrl,
                                  REF(IHXPacketFlowControl*) pPacketFlowControl,
                                  IHXSessionStats* pSessionStats,
                                  UINT16 unStreamCount,
                                  BOOL bUseMDP,
                                  BOOL bIsLive,
                                  BOOL bIsMulticast,
                                  DataConvertShim* pDataConv,
                                  Client* pClient,
                                  const char* szPlayerSessionId,
                                  BOOL bIsFCS)
{
    if (!bUseMDP)
    {
        if (!m_pPPM)
        {
            m_pPPM = new PPM(m_pProc, m_bIsRTP);
        }

        m_pPPM->RegisterSource(pSourceCtrl, &pPacketFlowControl, unStreamCount,
                               bIsLive, bIsMulticast, pDataConv, pClient,
                               szPlayerSessionId, pSessionStats, bIsFCS);
    }
    else
    {
        if (!m_pFlowMgr)
        {
            m_pFlowMgr = new PacketFlowManager(m_pProc, m_bIsRTP);
        }

        m_pFlowMgr->RegisterSource(pSourceCtrl, &pPacketFlowControl, pSessionStats, unStreamCount,
                                   bIsLive, bIsMulticast, pDataConv, pClient,
                                   szPlayerSessionId, bIsFCS);        
    }

    return HXR_OK;
}


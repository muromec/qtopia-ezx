/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: ntsrcbufstats.cpp,v 1.3 2007/01/11 21:12:21 milko Exp $ 
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

#include "ntsrcbufstats.h"
#include "rtspclnt.h"

HXNetSourceBufStats::HXNetSourceBufStats(RTSPClientProtocol* pProto) :
    m_pProto(pProto)
{
    if (m_pProto)
    {
        m_pProto->AddRef();
    }
}

HXNetSourceBufStats::~HXNetSourceBufStats()
{
    DoClose();
}

void HXNetSourceBufStats::Close()
{
    DoClose();
}

// We need to overide Release() because
// the base class doesn't know how to
// delete this derived class

/*
 * IUnknown methods
 */
STDMETHODIMP_(ULONG32)
HXNetSourceBufStats::Release(THIS)
{
    ULONG32 ulRet = DecRefCount();

    if (!ulRet)
    {
        delete this;
    }

    return ulRet;
}

/*
 * IHXSourceBufferingStats3 methods
 */
STDMETHODIMP
HXNetSourceBufStats::GetCurrentBuffering(THIS_ UINT16 uStreamNumber,
                                         REF(UINT32) ulLowestTimestamp, 
                                         REF(UINT32) ulHighestTimestamp,
                                         REF(UINT32) ulNumBytes,
                                         REF(HXBOOL) bDone)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (GetPktBufInfo(uStreamNumber))
    {
        if (m_pProto)
        {
            res = m_pProto->GetCurrentBuffering(uStreamNumber,
                                                ulLowestTimestamp, 
                                                ulHighestTimestamp,
                                                ulNumBytes,
                                                bDone);
        }
        else
        {
            res = HXR_UNEXPECTED;
        }
    }

    return res;
}

void HXNetSourceBufStats::DoClose()
{
    HX_RELEASE(m_pProto);

    HXSourceBufferStats::DoClose();
}

/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ut_rdtvptran.cpp,v 1.2 2006/12/06 20:53:46 seansmith Exp $
 *
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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

#include "hxslist.h"
#include "chxmaplongtoobj.h"
#include "ihxpckts.h"
#include "rtspif.h"
#include "rtsptran.h"
#include "rdt_udp.h"

#include "ut_rdtvptran.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

RDTUDPVectorPacketTransport::RDTUDPVectorPacketTransport (BOOL bIsServer,
        BOOL bIsSource,
        RTSPTransportTypeEnum lType,
        UINT32 uFeatureLevel,
        BOOL bDisableResend,
        bool bCallParentVectorPacket)
    : RDTUDPTransport (bIsServer, bIsSource, lType, uFeatureLevel, bDisableResend)
    , m_bCallParentVectorPacket (bCallParentVectorPacket)
{
    // Nothing, yet
}

RDTUDPVectorPacketTransport::~RDTUDPVectorPacketTransport ()
{
    // Nothing, yet
}

HX_RESULT
RDTUDPVectorPacketTransport::VectorPacket (BasePacket* pBasePacket, IHXBuffer** ppCur)
{
    if (m_bCallParentVectorPacket)
    {
        return RDTUDPTransport::VectorPacket (pBasePacket, ppCur);
    }

    SPIHXPacket spPacket = pBasePacket->PeekPacket();
    if (!spPacket.IsValid())
    {
        return HXR_FAIL;
    }

    SPIHXBuffer spPayload;
    HXAdoptInterfacePointer (spPayload, spPacket->GetBuffer());
    if (!spPayload.IsValid())
    {
        return HXR_FAIL;
    }

    SPIHXBuffer spHeaderBuffer;
    if(!(SUCCEEDED(m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
            (void**)spHeaderBuffer.AsInOutParam()))) || !spHeaderBuffer.IsValid())
    {
        return HXR_OUTOFMEMORY;
    }

    if (FAILED(MakeRDTHeader(pBasePacket, spHeaderBuffer.Ptr(), ppCur != NULL)))
    {
        return HXR_FAIL;
    }

    if(ppCur)
    {
        *(ppCur) = spHeaderBuffer.Ptr();
        spHeaderBuffer->AddRef();
        *(ppCur + 1) = spPayload.Ptr();
        spPayload->AddRef();
    }
    else
    {

        IHXBuffer* pWriteVec[2];
        pWriteVec[0] = spHeaderBuffer.Ptr();
        pWriteVec[1] = spPayload.Ptr();

        {
            m_pSocket->WriteV(2, pWriteVec);
        }
    }
    return HXR_OK;
}


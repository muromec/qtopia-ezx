/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: h263pyld.cpp,v 1.8 2005/03/14 19:24:47 bobclark Exp $
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

#include "h263pyld.h"
#include "debug.h"
#include "ihxpckts.h"
#include <string.h>
#include "rfc2190hlpr.h"
#include "rfc2429hlpr.h"
#include "bitstream.h"
#include "bitpack.h"

#define D_H263 0 // D_INFO

CH263PayloadFormat::CH263PayloadFormat() :
    m_lRefCount(0),
    m_pCCF(0),
    m_state(NeedPSC),
    m_ulFrameSize(0),
    m_ulTimestamp(0),
    m_pDepackHlpr(0)
{
#ifdef DEBUG
    debug_level() |= D_H263;
#endif /* DEBUG */
}

CH263PayloadFormat::~CH263PayloadFormat()
{
    HX_RELEASE(m_pCCF);

    FlushInput();
    FlushOutput();

    delete m_pDepackHlpr;
    m_pDepackHlpr = 0;
}

/*
 *	IUnknown methods
 */
STDMETHODIMP CH263PayloadFormat::QueryInterface(REFIID riid,
						   void** ppvObj)
{
    HX_RESULT res = HXR_NOINTERFACE;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	res = HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPayloadFormatObject))
    {
	AddRef();
	*ppvObj = (IHXPayloadFormatObject*)this;
	res = HXR_OK;
    }

    return res;
}

STDMETHODIMP_(ULONG32) CH263PayloadFormat::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CH263PayloadFormat::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *	IHXPayloadFormatObject methods
 */
STDMETHODIMP CH263PayloadFormat::Init(THIS_
				      IUnknown* pContext,
				      HXBOOL bPacketize)
{
    DPRINTF(D_H263, ("CH263PayloadFormat::Init(%p, %d)\n",
		     pContext, bPacketize));

    HX_RESULT res = HXR_UNEXPECTED;

    if (!bPacketize)
    {
	// Currently we only support depacketization
	res = pContext->QueryInterface(IID_IHXCommonClassFactory,
				       (void**)&m_pCCF);
    }

    return res;
}

STDMETHODIMP CH263PayloadFormat::Close()
{
    DPRINTF(D_H263, ("CH263PayloadFormat::Close()\n"));

    Flush();

    return HXR_OK;
}

STDMETHODIMP CH263PayloadFormat::Reset()
{
    // Called on Seeks
    DPRINTF(D_H263, ("CH263PayloadFormat::Reset()\n"));

    FlushInput();
    FlushOutput();
    m_state = NeedPSC;

    return HXR_OK;
}

STDMETHODIMP CH263PayloadFormat::SetStreamHeader(IHXValues* pHeader)
{
    DPRINTF(D_H263, ("CH263PayloadFormat::SetStreamHeader()\n"));

    return HXR_OK;
}

STDMETHODIMP CH263PayloadFormat::GetStreamHeader(REF(IHXValues*) pHeader)
{
    DPRINTF(D_H263, ("CH263PayloadFormat::GetStreamHeader()\n"));

    return HXR_OK;
}

STDMETHODIMP CH263PayloadFormat::SetPacket(IHXPacket* pPacket)
{
    DPRINTF(D_H263, ("CH263PayloadFormat::SetPacket()\n"));

    HX_RESULT res = HXR_UNEXPECTED;

    if (!pPacket->IsLost())
    {
	IHXBuffer* pBuf = pPacket->GetBuffer();
	UINT32 ulDataSize = 0;

	res = HXR_OK;

	if (!m_pDepackHlpr)
	    res = CreateHelper(pBuf->GetBuffer(), pBuf->GetSize());

	if (SUCCEEDED(res) &&
	    SUCCEEDED(res = m_pDepackHlpr->OnPacket(pBuf->GetBuffer(),
						    pBuf->GetSize())))
	{
	    HXBOOL bDone = FALSE;
	    HXBOOL bPacketAdded = FALSE;

	    /* Cache values since ProcessPacket() uses
	     * m_pDepackHlpr and can change it's state.
	     */
	    ulDataSize = m_pDepackHlpr->DataSize();
	    HXBOOL bIsPictureStart = m_pDepackHlpr->IsPictureStart();

	    while(!bDone && SUCCEEDED(res))
	    {
		switch(m_state) {
		case NeedPSC:
		    // See if this packet has a PSC
		    if (bIsPictureStart)
		    {
			m_ulFrameSize = 0; // Make sure the frame size is 0

			// Add the packet to the input queue
			res = AddPacket(ulDataSize, pPacket);

			// Signal that we added this packet
			bPacketAdded = TRUE;

			// Cache the timestamp for this frame
			m_ulTimestamp = pPacket->GetTime();

			// Change state
			m_state = GotPSC;
		    }
		    else
		    {
			// Signal that we are done handling this packet
			bDone = TRUE;
		    }
		    break;
		case GotPSC:
		    if (pPacket->GetTime() == m_ulTimestamp)
		    {
			// See if the packet has been added yet
			if (!bPacketAdded)
			{
			    DPRINTF (D_H263,("Multi-packet\n"));
			    // Add the packet to the input queue
			    res = AddPacket(ulDataSize, pPacket);

			    // Signal that we added this packet
			    bPacketAdded = TRUE;
			}

			// Check for the marker bit. The marker bit
			// is always mapped to odd ASM rule numbers
			if ((pPacket->GetASMRuleNumber() & 0x1) == 1)
			    m_state = GotFrame;
			else
			    bDone = TRUE;
		    }
		    else
		    {
			// This appears to be a different frame.
			// Goto the GotFrame state so that we
			// process the current frame
			m_state = GotFrame;
		    }
		    break;
		case GotFrame:
		    if (!m_inputQueue.IsEmpty())
		    {
			// Process the packets in the input queue
			res = ProcessPackets();
		    }

		    // Change to the NeedPSC state
		    m_state = NeedPSC;

		    if (bPacketAdded)
			bDone = TRUE;
		};
	    }
	}

	if (!SUCCEEDED(res))
	{
	    /* Something unexpected happened.
	     * Flush everything.
	     */
	    FlushInput();
	}

	HX_RELEASE(pBuf);
    }
    else
    {
	FlushInput();
	res = HXR_OK;
    }

    return res;
}

STDMETHODIMP CH263PayloadFormat::GetPacket(REF(IHXPacket*) pPacket)
{
    DPRINTF(D_H263, ("CH263PayloadFormat::GetPacket()\n"));
    HX_RESULT res = HXR_FAILED;

    if (!m_outputQueue.IsEmpty())
    {
	pPacket = (IHXPacket*)m_outputQueue.RemoveHead();
	res = HXR_OK;
    }

    DPRINTF(D_H263, ("CH263PayloadFormat::GetPacket() : %08x\n", res));

    return res;
}

STDMETHODIMP CH263PayloadFormat::Flush()
{
    // Called at the end of the clip
    DPRINTF(D_H263, ("CH263PayloadFormat::Flush()\n"));

    FlushInput();

    return HXR_OK;
}

static HX_RESULT GetH263FrameSize(int fmt, HXxSize &FrameDim)
{
    HX_RESULT res = HXR_NO_DATA;

    if (fmt == 1)
    {
	FrameDim.cx = 128;
	FrameDim.cy = 96;

	res = HXR_OK;
    }
    else if ((fmt >= 2) && (fmt < 6))
    {
	FrameDim.cx = 176;
	FrameDim.cy = 144;

	for(;fmt > 2; fmt--)
	{
	    FrameDim.cx <<= 1;
	    FrameDim.cy <<= 1;
	}

	res = HXR_OK;
    }
    else
	HX_ASSERT(FALSE);

    return res;
}

static HX_RESULT HandleH263Plus(Bitstream& bs, HXxSize &FrameDim)
{
    HX_RESULT res = HXR_UNEXPECTED;

    if (bs.GetBits(3) == 1) // UFEP
    {
	int fmt = bs.GetBits(3); // OPPTYPE (bits 1-3)

	if (fmt != 0x6)
	    res = GetH263FrameSize(fmt, FrameDim);
	else
	{
	    // This frame has custom dimensions

	    bs.GetBits(11) ; // OPPTYPE (bits 4-14)

	    if (bs.GetBits(4) == 0x8) // OPPTYPE (bits 15-18)
	    {
		bs.GetBits(6); // MPPTYPE (bits 1-6)
		if (bs.GetBits(3) == 0x1) // MPPTYPE (bits 7-9)
		{
		    if (bs.GetBits(1)) // CPM
			bs.GetBits(2); // PSBI

		    bs.GetBits(4); // CPFMT (bits 1-4)

		    int pwi = (bs.GetBits(9) + 1) * 4;

		    if (bs.GetBits(1)) // CPFMT (bit 14)
		    {
			int phi = (bs.GetBits(9)) * 4;

			if ((phi >= 1) && (phi <= 288))
			{
			    FrameDim.cx = pwi;
			    FrameDim.cy = phi;
			    res = HXR_OK;
			}
		    }
		}
	    }
	}
    }

    return res;
}

HX_RESULT CH263PayloadFormat::GetFrameDimensions(IHXBuffer* pBuffer,
						 HXxSize &FrameDim)
{
    DPRINTF(D_H263, ("CH263PayloadFormat::GetFrameDimensions()\n"));

    HX_RESULT res = HXR_UNEXPECTED;
    Bitstream bs;

    if (pBuffer->GetSize() >= 5)
    {
	bs.SetBuffer(pBuffer->GetBuffer());

	if (bs.GetBits(22) == 0x20) // Check PSC
	{
	    bs.GetBits(8); // Skip TR

	    if (bs.GetBits(2) == 0x02) // PTYPE(bits 1 & 2)
	    {
		bs.GetBits(3); // PTYPE(bits 3-5)

		int fmt = bs.GetBits(3); // Get Format

		if (fmt != 0x7)
		{
		    res = GetH263FrameSize(fmt, FrameDim);
		}
		else if (pBuffer->GetSize() >= 13)
		{
		    // This has an extended PTYPE
		    res = HandleH263Plus(bs, FrameDim);
		}
	    }
	}
    }

    DPRINTF(D_H263, ("CH263PayloadFormat::GetFrameDimensions() : %d\n", res));
    return res;
}


HX_RESULT CH263PayloadFormat::ProcessPackets()
{
    HX_RESULT res = HXR_UNEXPECTED;

    if (m_pCCF)
    {
	IHXBuffer* pBuf = 0;
	IHXRTPPacket* pOutRTPPkt = 0;
	ULONG32 ulFrameBytes = (m_ulFrameSize + 7) >> 3;

	if (!m_pDepackHlpr)
	    res = CreateHelper(pBuf->GetBuffer(), pBuf->GetSize());
	else
	    res = HXR_OK;

	if (SUCCEEDED(res) &&
	    SUCCEEDED((res = m_pCCF->CreateInstance(IID_IHXBuffer,
						    (void**)(&pBuf)))) &&
	    SUCCEEDED((res = pBuf->SetSize(ulFrameBytes))) &&
	    SUCCEEDED((res = m_pCCF->CreateInstance(IID_IHXRTPPacket,
						    (void**)(&pOutRTPPkt)))))
	{
	    BitPacker bp(pBuf->GetBuffer(), pBuf->GetSize());

	    LISTPOSITION pos = m_inputQueue.GetHeadPosition();

	    while((pos != 0) && SUCCEEDED(res))
	    {
		IHXPacket* pPacket = (IHXPacket*)m_inputQueue.GetNext(pos);
		IHXBuffer* pPktBuf = pPacket->GetBuffer();

		res = m_pDepackHlpr->OnPacket(pPktBuf->GetBuffer(),
					      pPktBuf->GetSize());
		if (SUCCEEDED(res))
		    res = m_pDepackHlpr->CopyPayload(bp);

		ULONG32 used = 0;
                used = bp.BytesUsed();
		HX_ASSERT(used <= ulFrameBytes);

		// Release the packet buffer
		HX_RELEASE(pPktBuf);
	    }

	    if (SUCCEEDED(res))
	    {
		IHXPacket* pFirstPkt = (IHXPacket*)m_inputQueue.GetHead();
		IHXRTPPacket* pRTPPkt = 0;

		if (SUCCEEDED(pFirstPkt->QueryInterface(IID_IHXRTPPacket,
							(void**)&pRTPPkt)))
		{
		    res = pOutRTPPkt->SetRTP(pBuf,
					     pRTPPkt->GetTime(),
					     pRTPPkt->GetRTPTime(),
					     pRTPPkt->GetStreamNumber(),
					     pRTPPkt->GetASMFlags(),
					     pRTPPkt->GetASMRuleNumber());
		}
		else
		{
		    res = pOutRTPPkt->Set(pBuf,
					  pFirstPkt->GetTime(),
					  pFirstPkt->GetStreamNumber(),
					  pFirstPkt->GetASMFlags(),
					  pFirstPkt->GetASMRuleNumber());
		}

		IHXPacket* pOutPkt = 0;
		res = pOutRTPPkt->QueryInterface(IID_IHXPacket,
						 (void**)&pOutPkt);

		if (SUCCEEDED(res))
		    m_outputQueue.AddTail(pOutPkt);

		HX_RELEASE(pRTPPkt);
	    }

	}

	HX_RELEASE(pBuf);
	HX_RELEASE(pOutRTPPkt);
    }

    FlushInput();

    return res;
}

HX_RESULT CH263PayloadFormat::AddPacket(ULONG32 ulDataSize, IHXPacket* pPacket)
{
    HX_RESULT res = HXR_OUTOFMEMORY;

    if (m_inputQueue.AddTail(pPacket))
    {
	m_ulFrameSize += ulDataSize;

	pPacket->AddRef();

	res = HXR_OK;
    }

    return res;
}

void CH263PayloadFormat::FlushInput()
{
    LISTPOSITION pos = m_inputQueue.GetHeadPosition();

    while(pos != 0)
    {
	IHXPacket* pPacket = (IHXPacket*)m_inputQueue.GetNext(pos);

	HX_RELEASE(pPacket);
    }

    m_inputQueue.RemoveAll();
    m_ulFrameSize = 0;
    m_state = NeedPSC;
}

void CH263PayloadFormat::FlushOutput()
{
    LISTPOSITION pos = m_outputQueue.GetHeadPosition();

    while(pos != 0)
    {
	IHXPacket* pPacket = (IHXPacket*)m_outputQueue.GetNext(pos);

	HX_RELEASE(pPacket);
    }

    m_outputQueue.RemoveAll();
}

HX_RESULT CH263PayloadFormat::CreateHelper(const UINT8* pBuf,
                                           ULONG32 ulBufSize)
{
    HX_RESULT res = HXR_UNEXPECTED;

#if defined(HELIX_FEATURE_H263_RFC2429)
    // Try the RFC2429 helper first
    if (FAILED(res))
    {
        HX_DELETE(m_pDepackHlpr);
        m_pDepackHlpr = new CRFC2429Helper();
        if (m_pDepackHlpr)
        {
            res = m_pDepackHlpr->OnPacket(pBuf, ulBufSize);
        }
    }
#endif

#if defined(HELIX_FEATURE_H263_RFC2190)
    // If the RFC2429 helper didn't succeed, then
    // try the RFC2190 helper next.
    if (FAILED(res))
    {
        HX_DELETE(m_pDepackHlpr);
        m_pDepackHlpr = new CRFC2190Helper();
        if (m_pDepackHlpr)
        {
            res = m_pDepackHlpr->OnPacket(pBuf, ulBufSize);
        }
    }
#endif

    return res;
}


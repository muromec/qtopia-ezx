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

#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxcom.h"
#include "hxcomm.h"

#include "hxassert.h"
#include "hxslist.h"
#include "hxmarsh.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxformt.h"
#include "hxrendr.h"
#include "hxformt.h"
#include "hxengin.h"
#include "mhead.h"

#include "mpapayld.h"


MPAPayloadFormat::MPAPayloadFormat()
    : m_lRefCount	    (0)
    , m_pContext	    (NULL)
    , m_pClassFactory	    (NULL)
    , m_pStreamHeader	    (NULL)
    , m_ulStreamNum	    (0)
    , m_bPacketizing	    (FALSE)
    , m_ulFrameSize	    (0)
    , m_bFragmentFrames	    (FALSE)
    , m_ulFramesPerPacket   (0)
    , m_ulPacketsPerFrame   (0)
    , m_ulFrameOffset	    (0)
    , m_bFlushed	    (FALSE)
    , m_pInputPackets	    (NULL)
    , m_ulInputSize	    (0)
    , m_ulInputUsed	    (0)
    , m_pPartialPacket	    (NULL)
    , m_pPartialBuffer	    (NULL)
    , m_ulMaxPacketSize	    (0)
{
    m_pInputPackets = new CHXSimpleList();
}

MPAPayloadFormat::~MPAPayloadFormat()
{
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pStreamHeader);

    if (!m_pInputPackets->IsEmpty())
    {
	IHXPacket* pPacket = (IHXPacket*)m_pInputPackets->RemoveHead();
	HX_RELEASE(pPacket);
    }
    HX_DELETE(m_pInputPackets);

    HX_RELEASE(m_pPartialPacket);
    HX_RELEASE(m_pPartialBuffer);
}

// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP 
MPAPayloadFormat::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPayloadFormatObject))
    {
	AddRef();
	*ppvObj = (IHXPayloadFormatObject*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
MPAPayloadFormat::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
MPAPayloadFormat::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP 
MPAPayloadFormat::Init(IUnknown* pContext,
		       HXBOOL bPacketize)
{
    HX_ASSERT(pContext);

    HX_RELEASE(m_pContext);
    m_pContext = pContext;
    m_pContext->AddRef();

    m_pContext->QueryInterface(IID_IHXCommonClassFactory, 
	(void**)&m_pClassFactory);
    HX_ASSERT(m_pClassFactory);

    // Remember if we are packetizing or depacketizing
    m_bPacketizing = bPacketize;

    return HXR_OK;
}

STDMETHODIMP
MPAPayloadFormat::Reset()
{
    // Release all input packets we have stored
    if (!m_pInputPackets->IsEmpty())
    {
	IHXPacket* pPacket = (IHXPacket*)m_pInputPackets->RemoveHead();
	HX_RELEASE(pPacket);
    }

    // Reset all relevant state variables
    m_ulFrameSize	= 0;
    m_bFragmentFrames	= FALSE;
    m_ulFramesPerPacket	= 0;
    m_ulPacketsPerFrame	= 0;
    m_ulFrameOffset	= 0;
    m_bFlushed		= FALSE;
    m_ulInputSize	= 0;
    m_ulInputUsed	= 0;
    HX_RELEASE(m_pPartialPacket);
    HX_RELEASE(m_pPartialBuffer);

    return HXR_OK;
}

STDMETHODIMP 
MPAPayloadFormat::SetStreamHeader(IHXValues* pHeader)
{
    HX_ASSERT(pHeader);
    m_pStreamHeader = pHeader;
    m_pStreamHeader->AddRef();

    m_pStreamHeader->GetPropertyULONG32("StreamNumber", m_ulStreamNum);

    m_pStreamHeader->GetPropertyULONG32("MaxPacketSize", m_ulMaxPacketSize);

    HX_ASSERT(m_ulMaxPacketSize > 4);
    // We will add 4 Bytes of header data,
    m_ulMaxPacketSize -= 4;

    return HXR_OK;
}

STDMETHODIMP 
MPAPayloadFormat::GetStreamHeader(REF(IHXValues*) pHeader)
{
    HX_ASSERT(m_pStreamHeader);
    pHeader = m_pStreamHeader;
    pHeader->AddRef();

    return HXR_OK;
}

STDMETHODIMP 
MPAPayloadFormat::SetPacket(IHXPacket* pPacket)
{
    HX_RESULT hResult = HXR_OK;
    IHXBuffer* pBuffer = NULL;

    HX_ASSERT(pPacket);
    pBuffer = pPacket->GetBuffer();

    // Add this packet to our list of input packets
    pPacket->AddRef();
    m_pInputPackets->AddTail(pPacket);
    m_ulInputSize += pBuffer->GetSize();

    if (m_bPacketizing && !m_ulFrameSize)
    {
	UINT32 ulBytesSkipped = 0;

	m_ulFrameSize = GetFrameSize(pBuffer->GetBuffer(), 
	    pBuffer->GetSize(), ulBytesSkipped);

	if (ulBytesSkipped)
	{
	    // If we had to skip any Bytes to find the header
	    // info, discard those leading Bytes as garbage
	    m_ulInputUsed = ulBytesSkipped;
	    m_ulInputSize -= ulBytesSkipped;
	}

	if (m_ulFrameSize &&
	    m_ulFrameSize <= m_ulMaxPacketSize)
	{
	    // An integral number of frames can fit into
	    // one packet, so pack them accordingly
	    m_bFragmentFrames = FALSE;
	    m_ulFramesPerPacket = m_ulMaxPacketSize / m_ulFrameSize;
	}
	else if (m_ulFrameSize)
	{
	    // Not even 1 entire frame will fit into one packet, 
	    // so break each frame up across multiple packets
	    m_bFragmentFrames = TRUE;
	    m_ulPacketsPerFrame = (m_ulFrameSize / m_ulMaxPacketSize) + 1;
	}
	else
	{
	    // We couldn't figure out the frame size, so toss
	    // this packet and return a failure code
	    m_pInputPackets->RemoveHead();
	    HX_RELEASE(pPacket);

	    hResult = HXR_FAIL;
	}
    }
    HX_RELEASE(pBuffer);

    return hResult;
}

STDMETHODIMP 
MPAPayloadFormat::GetPacket(REF(IHXPacket*) pPacket)
{
    HX_RESULT hResult = HXR_OK;
    pPacket = NULL;

    if (m_pInputPackets->IsEmpty())
    {
	if (m_bFlushed)
	{
	    // We have used up all available input
	    hResult = HXR_STREAM_DONE;
	}
	else
	{
	    // We don't have enough input 
	    // data to produce a packet
	    hResult = HXR_INCOMPLETE;
	}
    }
    else if (m_bPacketizing)
    {
	// We are converting normal packets into
	// RTP packets...

	if (m_bFragmentFrames)
	{
	    hResult = CreateFragmentedPacket(pPacket);
	}
	else
	{
	    hResult = CreateNormalPacket(pPacket);
	}
    }
    else
    {
	// We are converting RTP packets back into
	// normal packets...

	hResult = UnformatRTPPacket(pPacket);
    }

    return hResult;
}

STDMETHODIMP 
MPAPayloadFormat::Flush()
{
    m_bFlushed = TRUE;

    return HXR_OK;
}

UINT32
MPAPayloadFormat::GetFrameSize(UCHAR* pBuffer, 
			       UINT32 ulSize, 
			       REF(UINT32) ulBytesSkipped)
{
    UCHAR* pStart = pBuffer;
    UINT32 ulFrameSize = 0;
    ulBytesSkipped = ulSize;

    HX_ASSERT(pBuffer);
    HX_ASSERT(ulSize);

    if (pBuffer && ulSize)
    {
	MPEG_HEAD h;
	INT32 iFrameSize = 0;
	HXBOOL bCheckWord = FALSE;

	for (UINT32 i = 0; i < ulSize; i++)
	{
            bCheckWord = ((pStart[i] & 0xF0) == 0xF0) |
                         ((pStart[i] & 0xF0) == 0xE0);

	    if (bCheckWord)
	    {
		memset(&h, 0, sizeof(h));
		iFrameSize = head_info(&pStart[i], ulSize - i, &h, 0);

		if (iFrameSize)
		{
		    ulFrameSize = (UINT32)iFrameSize;
		    ulBytesSkipped = i;
		    break;
		}
	    }
	}
    }

    return ulFrameSize;
}

UINT32
MPAPayloadFormat::GetFrameOffset(IHXBuffer* pBuffer)
{
    UCHAR* pBuf = NULL;
    
    HX_ASSERT(pBuffer);
    pBuf = pBuffer->GetBuffer();
    HX_ASSERT(pBuf);

    // Skip the first 2 Bytes which are specified
    // as "reserved" in RFC 2250
    pBuf += 2;

    // The next two Bytes contain the frame offset
    return (UINT32)getshort(pBuf);
}

HX_RESULT
MPAPayloadFormat::CreateNormalPacket(REF(IHXPacket*) pPacket)
{
    HX_RESULT hResult = HXR_OK;
    UINT32 ulBufSize = 0;
    UCHAR* pBuf = NULL;
    IHXPacket* pNewPacket = NULL;
    IHXBuffer* pNewBuffer = NULL;

    if ((m_ulInputSize < (m_ulFramesPerPacket * m_ulFrameSize)) && 
	!m_bFlushed)
    {
	// We don't have enough input 
	// data to produce a packet
	hResult = HXR_INCOMPLETE;
    }
    else
    {
	// Create a new packet
	m_pClassFactory->CreateInstance(CLSID_IHXPacket, 
	    (void**)&pNewPacket);

	// Create a new buffer
	m_pClassFactory->CreateInstance(CLSID_IHXBuffer, 
	    (void**)&pNewBuffer);

	// Figure out what the timestamp should be
	UINT32 ulNewTimestamp = GetNextTimestamp();

	// Set the size of the buffer appropriately
	ulBufSize = (m_ulFrameSize * m_ulFramesPerPacket);
	if (m_ulInputSize < ulBufSize)
	{
	    // We must have been flushed, so fill the packet
	    // with whatever remaining data we have
	    HX_ASSERT(m_bFlushed);
	    ulBufSize = m_ulInputSize;
	}
	ulBufSize += 4;

	pNewBuffer->SetSize(ulBufSize);
	pBuf = pNewBuffer->GetBuffer();

	// Set the 4-Byte audio header for this packet 
	// (as dictated by RFC 2250)
	putshort(pBuf, 0);
	pBuf += 2;
	putshort(pBuf, 0);
	pBuf += 2;

	// Copy the data from the input packet list 
	// into the new packet buffer
	CopyInput(pBuf, ulBufSize - 4);

	// Finalize the new packet
	pNewPacket->Set(pNewBuffer, ulNewTimestamp, 
	    (UINT16)m_ulStreamNum, HX_ASM_SWITCH_ON, 0);
	HX_RELEASE(pNewBuffer);

	pPacket = pNewPacket;
    }

    return hResult;
}

HX_RESULT
MPAPayloadFormat::CreateFragmentedPacket(REF(IHXPacket*) pPacket)
{
    HX_RESULT hResult = HXR_OK;
    UINT32 ulBufSize = 0;
    UCHAR* pBuf = NULL;
    IHXPacket* pNewPacket = NULL;
    IHXBuffer* pNewBuffer = NULL;

    // Figure out how much we want to put in this packet, based on the
    // frame size, frame offset, and max packet size
    ulBufSize = m_ulFrameSize - m_ulFrameOffset;
    if (ulBufSize > m_ulMaxPacketSize)
    {
	ulBufSize = m_ulMaxPacketSize;
    }

    if ((m_ulInputSize < ulBufSize) && 
	!m_bFlushed)
    {
	// We don't have enough input 
	// data to produce a full packet
	hResult = HXR_INCOMPLETE;
    }
    else
    {
	// Create a new packet
	m_pClassFactory->CreateInstance(CLSID_IHXPacket, 
	    (void**)&pNewPacket);

	// Create a new buffer
	m_pClassFactory->CreateInstance(CLSID_IHXBuffer, 
	    (void**)&pNewBuffer);

	// Figure out what the timestamp should be
	UINT32 ulNewTimestamp = GetNextTimestamp();

	// Set the size of the buffer appropriately
	if (m_ulInputSize < ulBufSize)
	{
	    // We must have been flushed, so fill the packet
	    // with whatever remaining data we have
	    HX_ASSERT(m_bFlushed);
	    ulBufSize = m_ulInputSize;
	}
	ulBufSize += 4;

	pNewBuffer->SetSize(ulBufSize);
	pBuf = pNewBuffer->GetBuffer();

	// Set the 4-Byte audio header for this packet 
	// (as dictated by RFC 2250)
	putshort(pBuf, 0);
	pBuf += 2;
	putshort(pBuf, (UINT16)m_ulFrameOffset);
	pBuf += 2;

	// Update the current frame offset
	m_ulFrameOffset += ulBufSize - 4;
	if (m_ulFrameOffset == m_ulFrameSize)
	{
	    // We finished a frame, so reset our offset
	    m_ulFrameOffset = 0;
	}

	// Copy the data from the input packet list 
	// into the new packet buffer
	CopyInput(pBuf, ulBufSize - 4);

	// Finalize the new packet
	pNewPacket->Set(pNewBuffer, ulNewTimestamp, 
	    (UINT16)m_ulStreamNum, HX_ASM_SWITCH_ON, 0);
	HX_RELEASE(pNewBuffer);

	pPacket = pNewPacket;
    }

    return hResult;
}

HX_RESULT
MPAPayloadFormat::UnformatRTPPacket(REF(IHXPacket*) pPacket)
{
    HX_RESULT hResult = HXR_OK;
    HXBOOL bDone = FALSE;

    while (!bDone)
    {
	if (m_pPartialPacket)
	{
	    // We've already unformatted part of an audio frame,
	    // so grab then next packet and add it to the end
	    hResult = UnformatNextPacket(pPacket, bDone);
	}
	else
	{
	    // We don't have any partially unformatted data,
	    // so just grab the first packet and deal with it
	    hResult = UnformatFirstPacket(pPacket, bDone);
	}
    }

    return hResult;
}

HX_RESULT
MPAPayloadFormat::UnformatFirstPacket(REF(IHXPacket*) pPacket, 
				      REF(HXBOOL) bDone)
{
    IHXPacket* pNextPacket = NULL;
    IHXBuffer* pNextBuffer = NULL;
    bDone = FALSE;

    // Get the next input packet
    pNextPacket = (IHXPacket*)m_pInputPackets->RemoveHead();
    HX_ASSERT(pNextPacket);

    // If this is a lost packet, give it
    // straight back to the caller
    if (pNextPacket->IsLost())
    {
	pPacket = pNextPacket;
	bDone = TRUE;
	return HXR_OK;
    }

    pNextBuffer = pNextPacket->GetBuffer();
    HX_ASSERT(pNextBuffer);

    // Create a new packet
    m_pClassFactory->CreateInstance(CLSID_IHXPacket, 
	(void**)&m_pPartialPacket);
    HX_ASSERT(m_pPartialPacket);

    // Create a new buffer
    m_pClassFactory->CreateInstance(CLSID_IHXBuffer, 
	(void**)&m_pPartialBuffer);
    HX_ASSERT(m_pPartialBuffer);

    // Add this onto the new packet
    m_pPartialBuffer->SetSize(pNextBuffer->GetSize() - 4);
    memcpy(m_pPartialBuffer->GetBuffer(), /* Flawfinder: ignore */
	   pNextBuffer->GetBuffer() + 4, 
	   pNextBuffer->GetSize() - 4);

    // Set all properties except buffer from this packet
    m_pPartialPacket->Set(m_pPartialBuffer, 
			  pNextPacket->GetTime(), 
			  pNextPacket->GetStreamNumber(), 
			  pNextPacket->GetASMFlags(), 
			  pNextPacket->GetASMRuleNumber());

    // Find out if this packet contains an integral number
    // of full frames or a fragment of a single large frame
    UINT32 ulOffset = GetFrameOffset(pNextBuffer);
    if (!ulOffset && !m_ulFrameSize)
    {
	UINT32 ulBytesSkipped = 0;

	m_ulFrameSize = GetFrameSize(pNextBuffer->GetBuffer() + 4,
	    pNextBuffer->GetSize() - 4, ulBytesSkipped);
	HX_ASSERT(m_ulFrameSize);

	if (ulBytesSkipped)
	{
	    // If we had to skip any Bytes to find the header
	    // info, discard those leading Bytes as garbage
	    m_ulInputUsed = ulBytesSkipped;
	    m_ulInputSize -= ulBytesSkipped;
	}
    }

    if (ulOffset)
    {
	// We must have lost the previous packet in this
	// frame, so toss out the rest for now
	//
	// XXXDPS - If we can actually make some use of a
	// partial audio frame, we may want to do something
	// more intelligent here...
	HX_RELEASE(m_pPartialBuffer);
	HX_RELEASE(m_pPartialPacket);

	bDone = FALSE;
    }
    else
    {
	if (m_ulFrameSize <= (pNextBuffer->GetSize() - 4))
	{
	    // The packet contains an integral number of full frames

	    // Return the packet we've constructed
	    pPacket = m_pPartialPacket;
	    pPacket->AddRef();

	    HX_RELEASE(m_pPartialBuffer);
	    HX_RELEASE(m_pPartialPacket);

	    bDone = TRUE;
	}
	else
	{
	    // The packet contains the first fragment of a large
	    // frame that spans several packets

	    // We will keep looping and adding data to this buffer
	    // until we have constructed an entire frame of audio...
	}
    }

    HX_RELEASE(pNextBuffer);
    HX_RELEASE(pNextPacket);

    return HXR_OK;
}

HX_RESULT
MPAPayloadFormat::UnformatNextPacket(REF(IHXPacket*) pPacket, 
				     REF(HXBOOL) bDone)
{
    HX_RESULT hResult = HXR_OK;
    IHXPacket* pNextPacket = NULL;
    IHXBuffer* pNextBuffer = NULL;
    bDone = FALSE;

    if (m_pInputPackets->IsEmpty())
    {
	if (!m_bFlushed)
	{
	    // We don't have enough input 
	    // data to produce a full packet
	    hResult = HXR_INCOMPLETE;
	}
	else
	{
	    // Return the (partial) packet we've constructed
	    pPacket = m_pPartialPacket;
	    pPacket->AddRef();

	    HX_RELEASE(m_pPartialBuffer);
	    HX_RELEASE(m_pPartialPacket);
	}
	bDone = TRUE;
    }
    else
    {
	// Get the next input packet
	pNextPacket = (IHXPacket*)m_pInputPackets->RemoveHead();
	HX_ASSERT(pNextPacket);

	if (pNextPacket->IsLost())
	{
	    // If the packet is a lost packet, discard any
	    // partial packet we may have been building and
	    // forward this lost packet back to the caller
	    pPacket = pNextPacket;
	    pPacket->AddRef();

	    HX_RELEASE(m_pPartialBuffer);
	    HX_RELEASE(m_pPartialPacket);

	    bDone = TRUE;
	}
	else
	{
	    pNextBuffer = pNextPacket->GetBuffer();
	    HX_ASSERT(pNextBuffer);

	    // Add the data from this packet onto our cumulative buffer

	    // Temporarily Release() our references on the buffer, since
	    // m_pPartialPacket has a reference on it and we can't set a
	    // buffer with 2 references. We will re-AddRef() it in a sec.
	    UINT32 refCount = m_pPartialBuffer->Release();
            HX_ASSERT(refCount != 0 );

	    UINT32 ulPrevSize = m_pPartialBuffer->GetSize();

	    m_pPartialBuffer->SetSize(ulPrevSize + 
		pNextBuffer->GetSize() - 4);

	    memcpy(m_pPartialBuffer->GetBuffer() + ulPrevSize, /* Flawfinder: ignore */
		   pNextBuffer->GetBuffer() + 4,
		   pNextBuffer->GetSize() - 4);

	    // Re-AddRef() the buffer now that we have set it
	    m_pPartialBuffer->AddRef();

	    m_pPartialPacket->Set(m_pPartialBuffer, 
				  m_pPartialPacket->GetTime(), 
				  m_pPartialPacket->GetStreamNumber(), 
				  m_pPartialPacket->GetASMFlags(), 
				  m_pPartialPacket->GetASMRuleNumber());

	    if (m_pPartialBuffer->GetSize() >= m_ulFrameSize)
	    {
		// Return the (full) packet we've constructed
		pPacket = m_pPartialPacket;
		pPacket->AddRef();

		HX_RELEASE(m_pPartialBuffer);
		HX_RELEASE(m_pPartialPacket);

		bDone = TRUE;
	    }

	    HX_RELEASE(pNextBuffer);
	}
	HX_RELEASE(pNextPacket);
    }

    return hResult;
}

HX_RESULT
MPAPayloadFormat::CreateLostPacket(IHXPacket* pOldPacket, 
				   REF(IHXPacket*) pNewPacket)
{
    // Create a new packet
    m_pClassFactory->CreateInstance(CLSID_IHXPacket, 
	(void**)&pNewPacket);
    HX_ASSERT(pNewPacket);

    // Set all properties except buffer and time from
    // the old packet
    pNewPacket->Set(NULL, 
		    0, 
		    pOldPacket->GetStreamNumber(), 
		    pOldPacket->GetASMFlags(), 
		    pOldPacket->GetASMRuleNumber());

    pNewPacket->SetAsLost();

    return HXR_OK;
}

UINT32
MPAPayloadFormat::GetNextTimestamp()
{
    if (m_pInputPackets->IsEmpty())
    {
	return 0;
    }
    else
    {
	IHXPacket* pPacket = (IHXPacket*)m_pInputPackets->GetHead();
	HX_ASSERT(pPacket);

	return pPacket->GetTime();
    }
}

void
MPAPayloadFormat::CopyInput(UCHAR* pDest, UINT32 ulSize)
{
    UINT32 ulBytesCopied = 0;

    while (ulBytesCopied < ulSize)
    {
	IHXPacket* pPacket = (IHXPacket*)m_pInputPackets->RemoveHead();
	HX_ASSERT(pPacket);

	IHXBuffer* pBuffer = pPacket->GetBuffer();
	HX_ASSERT(pBuffer);

	UCHAR* pSrc = pBuffer->GetBuffer();
	HX_ASSERT(pSrc);

	UINT32 ulSrcSize = pBuffer->GetSize();
	HX_ASSERT(ulSrcSize);

	// Offset the input buffer based on 
	// how much we have previously used
	pSrc += m_ulInputUsed;
	ulSrcSize -= m_ulInputUsed;

	if (ulSrcSize < ulSize - ulBytesCopied)
	{
	    memcpy(pDest, pSrc, ulSrcSize); /* Flawfinder: ignore */
	    pDest += ulSrcSize;
	    ulBytesCopied += ulSrcSize;

	    m_ulInputUsed = 0;
	}
	else if (ulSrcSize > ulSize - ulBytesCopied)
	{
	    // This packet is more than big enough
	    // to fill our requested size
	    UINT32 ulBytesToCopy = ulSize - ulBytesCopied;

	    memcpy(pDest, pSrc, ulBytesToCopy); /* Flawfinder: ignore */
	    pDest += ulBytesToCopy;
	    ulBytesCopied += ulBytesToCopy;

	    // Add this packet back onto the list
	    pPacket->AddRef();
	    m_pInputPackets->AddHead(pPacket);

	    // Remember how much of it we have used
	    m_ulInputUsed += ulBytesToCopy;
	}
	else
	{
	    // An exact match!
	    memcpy(pDest, pSrc, ulSrcSize); /* Flawfinder: ignore */
	    pDest += ulSrcSize;
	    ulBytesCopied += ulSrcSize;

	    m_ulInputUsed = 0;
	}

	HX_RELEASE(pBuffer);
	HX_RELEASE(pPacket);
    }

    m_ulInputSize -= ulBytesCopied;
}

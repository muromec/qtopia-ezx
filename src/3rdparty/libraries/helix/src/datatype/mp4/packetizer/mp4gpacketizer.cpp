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
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.  * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#include "mp4gpacketizer.h"
#include "bitpack.h"
#include "hxassert.h"

BEGIN_INTERFACE_LIST_NOCREATE(CMP4GPacket)
    INTERFACE_LIST_ENTRY(IID_IHXPacketizer, IHXPacketizer)
END_INTERFACE_LIST




CMP4GPacket::CMP4GPacket(const tConfigParms &config)
: m_bFragmentsPending(FALSE)
, m_ulBytesInQueue(0)
, m_ulAUOffset(0)
, m_bPacketFull(FALSE)
, m_config(config)
{
}


CMP4GPacket::~CMP4GPacket()
{
/* the queue contains smart pointers, so when the queue gets released,
    the underlying media samples will be released, too. */
}


HX_RESULT 
CMP4GPacket::AddSample(IHXTMediaSample *pSample)
{
    HX_RESULT res = HXR_OK ;
    
    IHXTMediaSamplePtr spSample = pSample ;

    // if this packet is larger than can be signalled in the header,
    // return an error and don't enter the packet into the queue.
    // NB: this has nothing to do with whether this AU fits into this packet.
    if (pSample->GetDataSize() >> GetSizeLength())
    {
	return HXR_FAIL ;
    }
    
    // pushing the sample will AddRef() correctly.
    m_sampleQueue.push_back(spSample) ;
    m_ulBytesInQueue += spSample->GetDataSize() ;

    m_bPacketFull =
	(m_ulBytesInQueue + BITS_TO_BYTES(headerSize(m_sampleQueue.size())) > GetMTU() ||
	GetQueueDuration() >= GetMaxPacketDuration()) ;

    return res ;
}

BOOL 
CMP4GPacket::IsPacketReady() const
{
/* we can assemble if
 * - the number of bytes in the queue (plus header) is >= MTU
 * - the duration of the packet(s) in the queue is >= maxDuration
 * - we are in flushFragments state.
 */

    return (m_bFragmentsPending || m_bPacketFull) ;
}

UINT32
CMP4GPacket::FitPacket(UINT32 &size) const
{
    size = 0 ;
    if (m_sampleQueue.empty()) return 0 ;

    UINT32 i ;
    HXT_TIME tStartTime, tEndTime ;

    m_sampleQueue.front()->GetTime(&tStartTime, 0);

    for (i = 0; i < m_sampleQueue.size(); i++)
    {
	m_sampleQueue[i]->GetTime(0, &tEndTime) ;

	if (size + m_sampleQueue[i]->GetDataSize() + BITS_TO_BYTES(headerSize(i+1))>= GetMTU() ||
	    (i > 0 && tEndTime - tStartTime >= GetMaxPacketDuration()))
	{
	    break ;
	}
	size += m_sampleQueue[i]->GetDataSize() ;
    }

    if (i == 0)
    {
	if(m_config.bAllowFragmentation)
	{
	    // we need to fragment this packet. Reserve the maximum space.
	    size = GetMTU() ;
	}
	else
	{
	    //	Not fragmenting so reserve enough space for data and header
	    size = m_sampleQueue[i]->GetDataSize();
	    size += BITS_TO_BYTES(headerSize(1));
	    i = 1;
	}
    }
    else
    {
        size += BITS_TO_BYTES(headerSize(i)) ;
    }

    if(m_config.bAllowFragmentation)
    {
	HX_ASSERT(size <= GetMTU()) ;
    }

    return i ;
}

UINT32
CMP4GPacket::GetPacketSize() const
{
    UINT32 size ;
    FitPacket(size) ;
    return size ;
}

HX_RESULT 
CMP4GPacket::AssemblePacket(IHXTMediaSample* pOutputSample)
{
    HX_RESULT res = HXR_OK ;
    
    if (m_sampleQueue.empty())
	return HXR_NO_DATA ;
    
    // figure out how many AUs will go into this packet.
    UINT32 size ;
    UINT32 numAUs = FitPacket(size) ;

    // if the output sample is not large enough, bail out.
    if (pOutputSample->GetDataSize() < size)
    {
	HX_ASSERT(FALSE) ;
	return HXR_INVALID_PARAMETER ;
    }

    // first packet needs to be fragmented if numAUs==0
    m_bFragmentsPending |= (numAUs == 0) ;

    // if fragments are pending, only write (part of) one AU
    if (m_bFragmentsPending)
	numAUs = 1 ;
    
    HXT_TIME tStartTime, tEndTime ;
    
    // set the time on the outgoing packet.
    res = m_sampleQueue.front()->GetTime(&tStartTime, 0) ;
    res = m_sampleQueue[numAUs-1]->GetTime(0,&tEndTime) ;

    if (SUCCEEDED(res))
	res = pOutputSample->SetTime(tStartTime, tEndTime) ;

    BitPacker bs(pOutputSample->GetDataStartForWriting(), pOutputSample->GetDataSize()) ;
    
    bs.PackBits(headerSize(numAUs) - headerLengthBits, headerLengthBits) ;

    UINT32 ulIndex = 0 ;
    if (SUCCEEDED(res))
        res = m_sampleQueue[0]->GetSampleField(HXT_FIELD_SEQUENCE_NUMBER, &ulIndex) ;

    UINT32 i ;
    for (i = 0; i < numAUs && SUCCEEDED(res); i++)
    {
	bs.PackBits(m_sampleQueue[i]->GetDataSize(), GetSizeLength());
	// write AU index or -delta

        UINT32 ulNewIndex = 0 ;
        res = m_sampleQueue[i]->GetSampleField(HXT_FIELD_SEQUENCE_NUMBER, &ulNewIndex) ;

	bs.PackBits(ulNewIndex-ulIndex, i > 0 ? GetIndexDeltaLength() : GetIndexLength()) ;
	ulIndex = ulNewIndex + 1 ;

	// for all AUs after the first, write CTS delta
	bs.PackBits(i>0, m_config.ulCTSDeltaPresentLength) ;
	
	if (i>0 && m_config.ulCTSDeltaPresentLength)
	{
	    HXT_TIME tTime ;
	    m_sampleQueue[i]->GetTime(&tTime, 0) ;

	    HX_ASSERT(((tTime - tStartTime) >> GetCTSDeltaLength()) == 0) ;
	    bs.PackBits((UINT32)(tTime - tStartTime), GetCTSDeltaLength()) ;
	}
    }
    bs.ByteAlign() ;
    
    UINT8* pDest = pOutputSample->GetDataStartForWriting() + bs.BytesUsed() ;
    for (i = 0; i < numAUs ; i++)
    {
	IHXTMediaSamplePtr spSample = m_sampleQueue.front() ;
	
	UINT32 nBytes = spSample->GetDataSize() - m_ulAUOffset ;

	UINT32 bytesToCopy = m_config.bAllowFragmentation ? min(GetMTU() - bs.BytesUsed(), nBytes) : nBytes ;	

	memcpy(pDest, spSample->GetDataStartForReading() + m_ulAUOffset, bytesToCopy) ;
	pDest += bytesToCopy ;
	m_ulBytesInQueue -= bytesToCopy ;
	
	if (bytesToCopy < nBytes)
	{
	    // if we fragment, make sure this is the only AU
	    HX_ASSERT(i == 0) ;
	    // leave this sample in the queue
	    m_ulAUOffset += bytesToCopy ;
	}
	else
	{
	    // we're done with any fragments, clear fragmentsPending
	    m_bFragmentsPending = FALSE ;
	    m_ulAUOffset = 0 ;
	    // and pop sample off the queue
	    m_sampleQueue.pop_front() ;
	}
    }
    res = pOutputSample->SetDataSize(pDest - pOutputSample->GetDataStartForReading()) ;

    m_bPacketFull =
	(m_ulBytesInQueue + BITS_TO_BYTES(headerSize(m_sampleQueue.size())) > GetMTU() ||
	GetQueueDuration() >= GetMaxPacketDuration()) ;

    return res ;
}

HXT_TIME 
CMP4GPacket::GetQueueDuration() const
{
    HXT_TIME tStartTime, tEndTime ;
    if (m_sampleQueue.empty()) return 0 ;

    m_sampleQueue.front()->GetTime(&tStartTime, 0);
    m_sampleQueue.back()->GetTime(0, &tEndTime);

    return tEndTime - tStartTime ;
}

UINT32
CMP4GPacket::headerSize(UINT32 n) const
{
    UINT32 s = headerLengthBits + GetIndexLength() + n * (GetSizeLength() + m_config.ulCTSDeltaPresentLength);
    // the if() is necessary to correctly handle n == 0
    if (n > 1) s += (n-1)*(GetCTSDeltaLength() + GetIndexDeltaLength()) ;

    return s ;
}

BEGIN_INTERFACE_LIST_NOCREATE(CMP4GPacketizer)
    INTERFACE_LIST_ENTRY(IID_IHXPacketizer, IHXPacketizer)
END_INTERFACE_LIST

CMP4GPacketizer::CMP4GPacketizer(IHXPacketizerStrategy *pPacketizerStrategy)
: CBasePacketizer(pPacketizerStrategy)
{
    m_config.ulIndexLength = 0 ;
    for (m_config.ulIndexDeltaLength = 0 ;
	 (1UL<<m_config.ulIndexDeltaLength) < pPacketizerStrategy->GetMaxIndexDelta() ;
	 m_config.ulIndexDeltaLength++) ;

    SetMTU(1500) ;
    SetAUSize(1500,FALSE) ;
    SetAUDuration(511,FALSE) ;
    SetMaxPacketDuration(511) ;
    SetAllowFragmentation(TRUE);
}

HX_RESULT 
CMP4GPacketizer::SetMTU(UINT32 mtu)
{
    m_config.ulMTU = mtu ;
    return HXR_OK ;
}

HX_RESULT
CMP4GPacketizer::SetAUSize(UINT32 ulMaxAUSize, BOOL bConstantSize)
{
    // adjust m_SizeLength so that it holds up to maxAUSize
    m_config.ulMaxAUSize = ulMaxAUSize ;
    m_config.ulSizeLength = 0 ;

    if (ulMaxAUSize == 0)
	return HXR_INVALID_PARAMETER ;

    if (!bConstantSize)
    {
	while (ulMaxAUSize >> m_config.ulSizeLength)
	{
	    m_config.ulSizeLength++ ;
	}
    }

    return HXR_OK ;
}

HX_RESULT
CMP4GPacketizer::SetAUDuration(UINT32 ulMaxDuration, BOOL bConstantDuration)
{
    m_config.ulMaxAUDuration = ulMaxDuration ;
    m_config.ulCTSDeltaPresentLength = bConstantDuration ? 0:1 ;

    return HXR_OK ;
}

HX_RESULT
CMP4GPacketizer::SetMaxPacketDuration(HXT_TIME tMaxDuration)
{
    // adjust m_CTSDeltaLength so that it holds up to MaxDuration.
    m_config.tMaxPacketDuration = tMaxDuration ;
    m_config.ulCTSDeltaLength = 0 ;
    while (tMaxDuration >> m_config.ulCTSDeltaLength)
    {
	m_config.ulCTSDeltaLength++ ;
    }

    return HXR_OK ;
}

HX_RESULT
CMP4GPacketizer::SetAllowFragmentation(BOOL bAllowFragmentation)
{
    m_config.bAllowFragmentation = bAllowFragmentation;

    return HXR_OK;
}

IHXPacketizer*
CMP4GPacketizer::newPacket() const
{
    CMP4GPacket *p = new CMP4GPacket(m_config) ;
    return p ;
}

// return duration if constant, zero otherwise
UINT32 CMP4GPacketizer::GetConstantDuration() const
{
    return m_config.ulCTSDeltaPresentLength ?
	0 : m_config.ulMaxAUDuration ;
}

// return size in bytes if constant, zero otherwise
UINT32 CMP4GPacketizer::GetConstantSize() const
{
    return m_config.ulSizeLength ? 0 : m_config.ulMaxAUSize ;
}

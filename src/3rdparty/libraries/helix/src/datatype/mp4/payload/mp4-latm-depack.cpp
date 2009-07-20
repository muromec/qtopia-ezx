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

#include "hlxclib/string.h"
#include "mp4-latm-depack.h"

#define DEFAULT_PAYLOAD_BUF_SIZE 1024
#define DEFAULT_STAGING_BUF_SIZE 1024

MP4LATMDepack::MP4LATMDepack() :
    m_ulSampleRate(0),
    m_ulFrameDuration(0),
    m_pUserData(0),
    m_pCallback(0),
    m_bConfigPresent(FALSE),
    m_bNeedTime(TRUE),
    m_ulCurrentTime(0),
    m_pStagingBuf(new UINT8[DEFAULT_STAGING_BUF_SIZE]),
    m_ulStagingBufSize(DEFAULT_PAYLOAD_BUF_SIZE),
    m_ulBytesStaged(0),
    m_pSlotLengths(0),
    m_pPayloadBuf(new UINT8[DEFAULT_PAYLOAD_BUF_SIZE]),
    m_ulPayloadBufSize(DEFAULT_PAYLOAD_BUF_SIZE),
    m_bHadLoss(FALSE),
    m_bNeedMarker(FALSE)
{}

MP4LATMDepack::~MP4LATMDepack()
{
    delete [] m_pStagingBuf;
    m_pStagingBuf = 0;

    delete [] m_pSlotLengths;
    m_pSlotLengths = 0;

    delete [] m_pPayloadBuf;
    m_pPayloadBuf = 0;
}

HXBOOL MP4LATMDepack::Init(ULONG32 ulProfileID,
			 ULONG32 ulObject,
			 ULONG32 ulBitrate,
			 HXBOOL bConfigPresent,
			 const UINT8* pStreamMuxConfig,
			 ULONG32 ulMuxConfigSize,
			 OnFrameCB pFrameCB,
			 void* pUserData)
{
    HXBOOL ret = FALSE;

    if (pStreamMuxConfig)
    {
	Bitstream bs;

	bs.SetBuffer(pStreamMuxConfig, ulMuxConfigSize);

	// A config was specified so try to unpack it
	ret = HandleMuxConfig(bs);
    }
    else if (bConfigPresent)
    {
	// A config will be specified in the stream so signal success
	// since we know we are going to get a config soon
	ret = TRUE;
    }

    if (ret)
    {
	m_bConfigPresent = bConfigPresent;
	m_pCallback = pFrameCB;
	m_pUserData = pUserData;
    }

    return ret;
}

HXBOOL MP4LATMDepack::GetCodecConfig(const UINT8*& pConfig, 
				   ULONG32& ulConfigSize)
{
    HXBOOL ret = FALSE;

    if (m_muxConfig.NumStreams() > 0)
    {
	// Currently we just hand out the config info for stream 0
	const MP4AAudioSpec& audioSpec = 
	    m_muxConfig.GetStream(0).GetAudioSpec();

	pConfig = audioSpec.Config();
	ulConfigSize = audioSpec.ConfigSize();
	ret = TRUE;
    }
    return ret;
}

HXBOOL MP4LATMDepack::SetTimeBase(ULONG32 ulSampleRate)
{
    m_ulSampleRate = ulSampleRate;

    return TRUE;
}

HXBOOL MP4LATMDepack::SetFrameDuration(ULONG32 ulFrameDuration)
{
    /* MBO: While setting of m_ulFrameDuration is conceptually correct,
	    it requires m_ulFrameDuration to be correct at all times.
	    Thus, it introduces a burden on external components 
	    to provide this information in timely manner and adjust 
	    when AUDuration changes mid-stream.
	    Since rendering component will handle consecutive, equally
	    time-stamped audio packets as contiguous audio (at least
	    for a significant time-span), the setting of m_ulFrameDuration
	    to 0 will work reliably in all cases.
    m_ulFrameDuration = ulFrameDuration; */

    return TRUE;
}

HXBOOL MP4LATMDepack::Reset() // Completely reset the depacketizer state
{
    m_bNeedTime = TRUE;

    m_ulBytesStaged = 0;
    m_bHadLoss = FALSE;
    m_bNeedMarker = FALSE;

    return FALSE;
}

HXBOOL MP4LATMDepack::Flush() // Indicates end of stream
{
    return FALSE;
}

HXBOOL MP4LATMDepack::OnPacket(ULONG32 ulTime, const UINT8* pData, 
			     ULONG32 ulSize, HXBOOL bMarker)
{
    HXBOOL failed = FALSE;

    if (!m_bNeedMarker)
    {
	const UINT8* pBuffer = pData;

	if (m_bNeedTime)
	{
	    m_ulCurrentTime = ulTime;
	    
	    m_bNeedTime = FALSE;
	}
	
	if ((!bMarker) || (m_ulBytesStaged != 0))
	{
	    if ((m_ulBytesStaged + ulSize) > m_ulStagingBufSize)
	    {
		// We need to make the staging buffer larger
		UINT8* pNewBuf = new UINT8[m_ulBytesStaged + ulSize];
		
		memcpy(pNewBuf, m_pStagingBuf, m_ulBytesStaged); /* Flawfinder: ignore */
		
		delete [] m_pStagingBuf;
		m_pStagingBuf = pNewBuf;
	    }

	    memcpy(&m_pStagingBuf[m_ulBytesStaged], pData, ulSize); /* Flawfinder: ignore */
	    m_ulBytesStaged += ulSize;

	    pBuffer = m_pStagingBuf;
	    ulSize = m_ulBytesStaged;
	}

	if (bMarker)
	{
           failed = !ProcessStagingBuffer(pBuffer,ulSize);
	}
    }
    else if (bMarker)
    {
	// We just got the marker we needed. We should be
	// synced up again now
	m_bNeedMarker = FALSE;
    }

    return !failed;
}

HXBOOL MP4LATMDepack::OnLoss(ULONG32 ulNumPackets) // called to indicate lost 
                                                 // packets
{
    m_bHadLoss = TRUE;

    // Signal that we need a marked packet before we are
    // synced up again
    m_bNeedMarker = TRUE;

    // Toss the bytes we currently have staged since we
    // have no way of determining how much data was lost
    m_ulBytesStaged = 0;

    // Signal that we need to update m_ulCurrentTime
    // when we lock back onto the bitstream.
    m_bNeedTime = TRUE;

    return TRUE;
}

HXBOOL MP4LATMDepack::ProcessStagingBuffer(const UINT8* pBuffer,ULONG32 ulSize)
{
    HXBOOL failed = FALSE;

    Bitstream bs;
    
    bs.SetBuffer(pBuffer, ulSize);

    if (m_bConfigPresent)
    {
        UINT32 ulLeft = bs.BitsLeft();
        if (ulLeft < 1)
        {
            failed = TRUE;
        }
        else if (bs.GetBits(1) == 0)
        {
	    // Not sure what we are supposed to do if this fails
            if (!HandleMuxConfig(bs))
                failed = TRUE;
        }
    }

    if (!failed)
    {	
	for (ULONG32 i = 0; i < m_muxConfig.NumSubFrames(); i++)
	{
	    ULONG32 ulFrameTime = m_ulCurrentTime;

	    ulFrameTime += (i * m_ulFrameDuration); 

	    if (!GetPayloadLengths(bs))
            {
                failed = TRUE;
                break;
            }
	    if (!GetPayloads(bs, ulFrameTime))
	    {
	        failed = TRUE;
	        break;
	    }
	}

	m_bHadLoss = FALSE;
	m_ulBytesStaged = 0;
	m_bNeedTime = TRUE;
    }

    return !failed;
}

HXBOOL MP4LATMDepack::HandleMuxConfig(Bitstream& bs)
{
    HXBOOL ret = m_muxConfig.Unpack(bs);
    
    if (ret)
    {
	delete [] m_pSlotLengths;
	m_pSlotLengths = new ULONG32[m_muxConfig.NumStreams()];
    }

    return ret;
}

HXBOOL MP4LATMDepack::GetPayloadLengths(Bitstream& bs)
{
    HXBOOL failed = FALSE;
    if (m_muxConfig.AllSameTiming())
    {
        for (ULONG32 prog = 0; !failed && (prog < m_muxConfig.NumPrograms()) ; prog++)
	{
	    for (ULONG32 layer = 0; layer < m_muxConfig.NumLayers(prog); layer++)
	    {
		ULONG32 streamID = m_muxConfig.GetStreamID(prog, layer);
		
		if (m_muxConfig.GetStream(streamID).GetLengthType() == 0)
		{	
		    ULONG32 tmp;
		    UINT32 ulSizeByteCount = 0;
		    m_pSlotLengths[streamID] = 0;
		    UINT32 ulLeft = bs.BitsLeft();
		    	
		    do
		    { 	
		        ulLeft =  bs.BitsLeft();
		        if (ulLeft < 8)
		        {
		            return FALSE;
		        }
		        tmp = bs.GetBits(8);
		        ulSizeByteCount++;
		        m_pSlotLengths[streamID] += tmp;
		    } while (tmp == 0xff );

                    if (m_pSlotLengths[streamID] + ulSizeByteCount > bs.GetBufSize())
                    {
                        failed = TRUE;
                    }

		    // Resize the payload buffer if we encounter a payload
		    // size that is larger than our buffer
		    if (m_pSlotLengths[streamID] > m_ulPayloadBufSize)
		    {
			m_ulPayloadBufSize = m_pSlotLengths[streamID];

			delete [] m_pPayloadBuf;
			m_pPayloadBuf = new UINT8 [m_ulPayloadBufSize];
		    }
		}
	    }
	}
    }
    return !failed;
}

HXBOOL MP4LATMDepack::GetPayloads(Bitstream& bs, ULONG32 ulTime)
{
    HXBOOL failed = FALSE;
    if (m_muxConfig.AllSameTiming())
    {
	for (ULONG32 prog = 0; prog < m_muxConfig.NumPrograms(); prog++)
	{
	    for (ULONG32 layer = 0; layer < m_muxConfig.NumLayers(prog); layer++)
	    {
	        ULONG32 streamID = m_muxConfig.GetStreamID(prog, layer);
	        UINT32 ulLeft = bs.BitsLeft();
	        if (ulLeft < (m_pSlotLengths[streamID] << 3))
	        {
	            return FALSE;		 
	        }
	        bs.GetBits(m_pSlotLengths[streamID] << 3, m_pPayloadBuf);

		// Currently we just hand out packets for stream 0
		if ((streamID == 0) && m_pCallback)
		{
		    m_pCallback( m_pUserData, ulTime, 
				 m_pPayloadBuf,
				 m_pSlotLengths[streamID],
				 m_bHadLoss);
		}
	    }
	}
    }
    return !failed;
}


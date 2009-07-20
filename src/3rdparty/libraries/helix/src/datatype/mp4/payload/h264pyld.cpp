/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: h264pyld.cpp,v 1.7 2007/01/11 16:31:00 aperiquet Exp $
 * 
 * Portions Copyright (c) 1995-2005 RealNetworks, Inc. All Rights Reserved.
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
/****************************************************************************
 /*
 *  Includes
 */
#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxcom.h"
#include "hxcomm.h"

#include "hxassert.h"
#include "hxslist.h"
#include "hxstrutl.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxformt.h"
#include "hxrendr.h"
#include "hxformt.h"
#include "hxengin.h"

#include "bitstuff.h"

#include "sdpchunk.h"
#include "sdptools.h"
#include "rtsputil.h" 
#include "h264pyld.h"

 /*  Defines
 */
#define _OVERALLOC_CODEC_DATA	3

#define NALU_SIZE_FIELD 4  // lengthSizeMinusOne = 11b in AVCConfigurationRecord

#define FLUSH_ALL_PACKETS   0xFFFFFFFF

#define MP4V_H264_PAYLOAD_MIME_TYPE	    "video/H264"

#define MAX_NALU_PACKETS	256 

#define SINGLE_NAL		1
#define AGGREGATED_NAL	2
#define FRAGMENTED_NAL	3
#define STAP_A			24
#define STAP_B			25
#define MTAP16			26
#define MTAP24			27
#define FU_A			28
#define FU_B			29
#define INTERLEAVE_MODE 2
#define AVC_DECODER_CONFIGURATION_RECORD_HEADER_SIZE 6
const char* const H264PayloadFormat::m_pCodecId = "AVC1";

/*****************************************************************************
aligned(8) class AVCDecoderConfigurationRecord {
        unsigned int(8) configurationVersion = 1;
        unsigned int(8) AVCProfileIndication;
        unsigned int(8) profile_compatibility;
        unsigned int(8) AVCLevelIndication; 
        bit(6) reserved = ‘111111’b;
        unsigned int(2) lengthSizeMinusOne; 
        bit(3) reserved = ‘111’b;
        unsigned int(5) numOfSequenceParameterSets;
        for (i=0; i< numOfSequenceParameterSets;  i++) {
                 unsigned int(16) sequenceParameterSetLength ;
                 bit(8*sequenceParameterSetLength) sequenceParameterSetNALUnit;
        }
        unsigned int(8) numOfPictureParameterSets;
        for (i=0; i< numOfPictureParameterSets;  i++) {
                 unsigned int(16) pictureParameterSetLength;
                  bit(8*pictureParameterSetLength) pictureParameterSetNALUnit;
        }
}

*****************************************************************************/

H264PayloadFormat::CNALUPacket::~CNALUPacket()
{       
    HX_RELEASE(pBuffer);
    DeallocateData();
}
    
void H264PayloadFormat::CNALUPacket::DeallocateData()
{
    // deallocate the old data only if we own it
    if (bDataIsOwned)
    {
        HX_VECTOR_DELETE(pData);
    }

    pData = NULL;
    ulSize = 0;
}

HX_RESULT H264PayloadFormat::CNALUPacket::ResizeData
                        (UINT32 ulNewSize, 
                         UINT32 ulSizeToCopy, 
                         UINT8* pDataToCopy)
{
    HX_RESULT retVal = HXR_OK;

    // allocate memory for the new data
    UINT8* pNewData = new UINT8[ulNewSize];
    if(!pNewData)
    {
        retVal = HXR_OUTOFMEMORY;
    }
       
    if(SUCCEEDED(retVal))
    {
        // copy the old data, if any exists
        if(ulSize > 0)
        {
            memcpy(pNewData, pData, ulSize);
        }

        // copy the new data, if any exists, starting after the old data
        if(ulSizeToCopy)
        { 
            if(ulSize + ulSizeToCopy <= ulNewSize)
            {
                memcpy(pNewData + ulSize, pDataToCopy, ulSizeToCopy);
            }
            else
            {
                retVal = HXR_BUFFERTOOSMALL;

                // note: we'll keep the newly allocated data if on 
                //       this failure and deallocate it on the destructor
            }
        }

        DeallocateData();

        // save the new data and size
        pData = pNewData;
        ulSize = ulNewSize;

        // we now own this data
        bDataIsOwned = TRUE;
    }

    return retVal;
}

H264PayloadFormat::H264PayloadFormat(CHXBufferMemoryAllocator* pAllocator)
    : m_lRefCount(0)
    , m_pClassFactory(NULL)
    , m_pAllocator(NULL)
    , m_ulPacketizationMode(0)
    , m_ulSpropInterleavingDepth(1)
    , m_ulSpropDeintBufReq(0)
    , m_ulSpropInitBufTime(0)
    , m_ulSpropMaxDonDiff(1)
    , m_pStreamHeader(NULL)
    , m_pBitstreamHeader(NULL)
    , m_ulBitstreamHeaderSize(0)
    , m_pNALUPackets(NULL)
    , m_ulMaxNALUPackets(MAX_NALU_PACKETS)
    , m_ulNumNALUPackets(0)
    , m_ulSamplesPerSecond(1000)
    , m_ulRTPSamplesPerSecond(0)
    , m_ulLastParsedPktTime(0)
    , m_ulPrevDON(0)
    , m_ulSequenceNumber(0)
    , m_ulCodecDataSeqNumber(0)
    , m_bFlushed(FALSE)
    , m_bUsesRTPPackets(FALSE)
    , m_bRTPPacketTested(FALSE)
    , m_bPacketize(FALSE)
    , m_bInitialBufferingDone(FALSE)
    , m_bStartPacket(TRUE)
    , m_bFirstPacket(TRUE)
    , m_bStartNALFragment(FALSE)
    , m_bFragmentLost(FALSE)
{
    ;
}

H264PayloadFormat::~H264PayloadFormat()
{
    Close();
}

HX_RESULT H264PayloadFormat::Build(REF(IMP4VPayloadFormat*) pFmt)
{
    pFmt = new H264PayloadFormat();

    HX_RESULT res = HXR_OUTOFMEMORY;
    if (pFmt)
    {
        pFmt->AddRef();
        res = HXR_OK;
    }

    return res;
}

// *** IUnknown methods ***

/******************************************************************************
  Method:
  IUnknown::QueryInterface
  Purpose: Everyone usually implements this the same... feel free to use
    this implementation.
******************************************************************************/
STDMETHODIMP
H264PayloadFormat::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
    {
        { GET_IIDHANDLE(IID_IUnknown), this },
        { GET_IIDHANDLE(IID_IHXPayloadFormatObject), (IHXPayloadFormatObject*) this },
    };
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/******************************************************************************
  Method:
  IUnknown::AddRef
  Purpose: Everyone usually implements this the same... feel free to use
    this implementation.
******************************************************************************/
STDMETHODIMP_(ULONG32)
H264PayloadFormat::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/******************************************************************************
    Method:
    IUnknown::Release
    Purpose:
    Everyone usually implements this the same... feel free to use 
    this implementation.
******************************************************************************/
STDMETHODIMP_(ULONG32)
H264PayloadFormat::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP
H264PayloadFormat::Init(IUnknown* pContext,
			HXBOOL bPacketize)
{
    HX_RESULT retVal = HXR_OK;

    HX_RELEASE(m_pClassFactory);

    m_bPacketize = bPacketize;

    if (SUCCEEDED(retVal))
    {
        retVal = pContext->QueryInterface(IID_IHXCommonClassFactory,
                                          (void**) &m_pClassFactory);
    }

    if (SUCCEEDED(retVal))
    {
        retVal = HXR_OUTOFMEMORY;

        m_pNALUPackets = new CNALUPacket* [m_ulMaxNALUPackets];

        if (m_pNALUPackets)
        {
            memset(m_pNALUPackets, 0, sizeof(CNALUPacket*) * m_ulMaxNALUPackets);
            retVal = HXR_OK;
        }
    }

    return retVal;
}

STDMETHODIMP
H264PayloadFormat::Close()
{
    Reset();
    HX_VECTOR_DELETE(m_pNALUPackets);
    HX_VECTOR_DELETE(m_pBitstreamHeader);
	if (m_pAllocator)
    {
        m_pAllocator->Release();
        m_pAllocator = NULL;
    }
    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pStreamHeader);

    return HXR_OK;
}


STDMETHODIMP
H264PayloadFormat::Reset()
{
    // Release all input packets we have stored
    FlushQueues();
    FlushArrays();

    m_bFlushed = FALSE;
    m_bStartPacket = TRUE;
    m_bInitialBufferingDone = FALSE;
    m_bFirstPacket = FALSE;
    m_bStartNALFragment = FALSE;
    m_bFragmentLost = FALSE;
    m_ulPrevDON = 0;

    m_TSConverter.Reset();

    return HXR_OK;
}

STDMETHODIMP
H264PayloadFormat::SetStreamHeader(IHXValues* pHeader)
{
    HX_RESULT retVal;

    if (pHeader)
    {
        m_pStreamHeader = pHeader;
        m_pStreamHeader->AddRef();
    }

    if (m_bPacketize)
    {
        retVal = SetPacketizerHeader(pHeader);
    }
    else
    {
        retVal = SetAssemblerHeader(pHeader);
    }

    return retVal;
}

void H264PayloadFormat::SetAllocator(CHXBufferMemoryAllocator*	pAllocator)
{
    if (pAllocator)
    {
        m_pAllocator = pAllocator; 
        m_pAllocator->AddRef();
    }

}

HX_RESULT H264PayloadFormat::SetPacketizerHeader(IHXValues* pHeader)
{
    return HXR_OK;
}

HX_RESULT H264PayloadFormat::SetAssemblerHeader(IHXValues* pHeader)
{
    IHXBuffer* pMimeType = NULL;
    const char* pMimeTypeData = NULL;
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    if (pHeader)
    {
        retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
        retVal = pHeader->GetPropertyCString("MimeType", pMimeType);
    }

    if (SUCCEEDED(retVal))
    {
        pMimeTypeData = (char*) pMimeType->GetBuffer();

        retVal = HXR_FAIL;
        if (pMimeTypeData)
        {
            retVal = HXR_OK;
        }
    }

    // Determine payload type here based on mime type
    if (SUCCEEDED(retVal))
    {
        retVal = HXR_FAIL;

	    if (strcasecmp(pMimeTypeData, MP4V_H264_PAYLOAD_MIME_TYPE) == 0)
        {
            m_PayloadID = PYID_X_HX_H264;
            retVal = HXR_OK;
        }
    }

    if (SUCCEEDED(retVal))
    {
        retVal = SetAssemblerConfig(pHeader);
    }

    if (SUCCEEDED(retVal))
    {
         m_pStreamHeader->GetPropertyULONG32("SamplesPerSecond",
					    m_ulRTPSamplesPerSecond);
    }

    HX_RELEASE(pMimeType);

    return retVal;
}


HX_RESULT H264PayloadFormat::SetAssemblerConfig(IHXValues* pHeader)
{
 
    UINT8 profileLevelID[3];
    HX_RESULT retVal = HXR_OK;
    CHXSimpleList		SPSQueue;
    CHXSimpleList       PPSQueue;
    UINT32 SPSCount = 0;
    UINT32 PPSCount = 0;
    IHXBuffer* pProfileLevelIDString = NULL;
    IHXBuffer* pSpropParameterSets = NULL;
    UINT32 offset = 0;
    CNALUnit *pNALUnit = NULL;
    UINT32 ulConfigBuffSize = AVC_DECODER_CONFIGURATION_RECORD_HEADER_SIZE;
    UCHAR tmp[2];

    HX_ASSERT(pHeader);

    // Obtain Required parameters
    pHeader->GetPropertyCString("FMTPprofile-level-id",
						pProfileLevelIDString);
    if(pProfileLevelIDString)
    {
        retVal = HexStringToBinary(profileLevelID, 
                            (char*)pProfileLevelIDString->GetBuffer());
	
    }
    if(retVal != HXR_OK)
    {
        profileLevelID[0] = 0x42;
        profileLevelID[1] = 0xE0;
        profileLevelID[2] = 0x01;   
    }

    pHeader->GetPropertyCString("FMTPsprop-parameter-sets", pSpropParameterSets);

    if(pSpropParameterSets)
    {
        retVal = ParseSpropParameterSet(pSpropParameterSets);
    }

    if(SUCCEEDED(retVal))
    {
        UINT8 uNALUHeader;
        while (!m_ParameterSetQueue.IsEmpty())
        {
            pNALUnit = (CNALUnit *)m_ParameterSetQueue.RemoveHead();
            uNALUHeader = pNALUnit->pData[0];
            uNALUHeader = uNALUHeader & 0x1F ; // To get 5 bit NALU type
            // // Add 2 bytes for sequenceParameterSetLength 
            ulConfigBuffSize += pNALUnit->usSize + 2 ; ;
            if(uNALUHeader == 7)
            {
                // Sequence parameter set
                SPSQueue.AddTail(pNALUnit);
                SPSCount++;
            }
            else if(uNALUHeader == 8)
            {
                // Picture parameter set
                PPSQueue.AddTail(pNALUnit);
                PPSCount++;     
            }
            else
            {
                HX_DELETE(pNALUnit);
            }
        }   
    }

    UINT8* pBuffer = (UINT8 *)new UINT8[ulConfigBuffSize +1];

    // Prepare m_pBitstreamHeader
    pBuffer[0] = 1;
    pBuffer[1] = profileLevelID[0];
    pBuffer[2] = profileLevelID[1];
    pBuffer[3] = profileLevelID[2];
    pBuffer[4] = 0xFF; //as  bit(6) reserved = 111111b and lengthSizeMinusOne = 11b;  

    pBuffer[5] = 0xE0; // as bit(3) reserved = 111b 
    offset = 6;
				
    UINT8 uCountSPS = SPSQueue.GetCount();

    if(uCountSPS > 0)
    {
        pBuffer[5] = uCountSPS | 0xE0; // as bit(3) reserved = 111b 
											
        while (uCountSPS > 0)
        {
            pNALUnit = (CNALUnit *) SPSQueue.RemoveHead();
            tmp[0] = pNALUnit->usSize >> 8;
            tmp[1] = (UINT8)pNALUnit->usSize;
            memcpy(pBuffer + offset, tmp, sizeof(UINT16)); 
            offset += 2;				
            memcpy(pBuffer + offset, pNALUnit->pData, pNALUnit->usSize); 
            offset += pNALUnit->usSize;
            uCountSPS--;
            HX_DELETE(pNALUnit);
        }   
    }
    else
    {
        pBuffer[5] = uCountSPS | 0xE0; // as bit(3) reserved = 111b 				
    }


    UINT8 uCountPPS = PPSQueue.GetCount();
    pBuffer[offset++] = uCountPPS;
    while (uCountPPS > 0)
    {
        pNALUnit = (CNALUnit *)PPSQueue.RemoveHead();
        tmp[0] = pNALUnit->usSize >> 8;
        tmp[1] = (UINT8)pNALUnit->usSize;
        memcpy(pBuffer + offset, tmp, sizeof(UINT16));
        offset += 2;				
        memcpy(pBuffer + offset, pNALUnit->pData, pNALUnit->usSize); 
        offset += pNALUnit->usSize;
        uCountPPS--;
		HX_DELETE(pNALUnit);
    }
	
    m_pBitstreamHeader = pBuffer;   
    m_ulBitstreamHeaderSize = offset;

		
    HX_RELEASE(pSpropParameterSets);
    HX_RELEASE(pProfileLevelIDString);
	
    pHeader->GetPropertyULONG32("FMTPpacketization-mode",
                                            m_ulPacketizationMode);
    if(m_ulPacketizationMode == INTERLEAVE_MODE)
    {
        pHeader->GetPropertyULONG32("FMTPsprop-interleaving-depth",
                                        m_ulSpropInterleavingDepth);
		
        pHeader->GetPropertyULONG32("FMTPsprop-deint-buf-req",
                                            m_ulSpropDeintBufReq);
	     
        pHeader->GetPropertyULONG32("FMTPsprop-init-buf-time",
                                        m_ulSpropInitBufTime);
        pHeader->GetPropertyULONG32("FMTPsprop-max-don-diff",
                                            m_ulSpropMaxDonDiff);
	
    }
    return retVal;
}


HX_RESULT
H264PayloadFormat::ParseSpropParameterSet(IHXBuffer *pParameterString)
{
    // parse the string, extract NAL unit and store it in parameter set queue
    UINT32 ulSize = pParameterString->GetSize();
    UINT8* pData = pParameterString->GetBuffer();
    CNALUnit* pNALUnit = NULL;
    INT32 lNumChar = 0;

    while (ulSize > 0)
    {
        if((lNumChar == ulSize -1) || (*(pData+lNumChar) == ','))
        {
            pNALUnit = new(CNALUnit);
            pNALUnit->pData = new(UINT8[(lNumChar/4) *3 +1]);
            pNALUnit->usSize = BinFrom64((const char*) pData, lNumChar,
                                                        pNALUnit->pData);
            m_ParameterSetQueue.AddTail(pNALUnit);
            if(	lNumChar == ulSize)
                break;
            else
            {
                ulSize -= lNumChar + 1; // Added 1 for ',' char
                pData += lNumChar + 1;
                lNumChar = 0;
            }

        }
        else
        {
            lNumChar++;
        }
    }
    return HXR_OK;
}


STDMETHODIMP
H264PayloadFormat::GetStreamHeader(REF(IHXValues*) pHeader)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pStreamHeader)
    {
        retVal = HXR_OK;
        pHeader = m_pStreamHeader;
        pHeader->AddRef();
    }

    return retVal;
}

STDMETHODIMP
H264PayloadFormat::SetPacket(IHXPacket* pPacket)
{
    HX_RESULT retVal = HXR_OK;

    HX_ASSERT(pPacket);

    if (!m_bRTPPacketTested)
    {
        IHXRTPPacket* pRTPPacket = NULL;

        m_bUsesRTPPackets = (pPacket->QueryInterface(IID_IHXRTPPacket,
                                        (void**) &pRTPPacket) == HXR_OK);

    m_bRTPPacketTested = TRUE;

    HX_RELEASE(pRTPPacket);

    if (m_bUsesRTPPackets)
    {
	    if (m_ulRTPSamplesPerSecond == 0)
	    {
            m_ulRTPSamplesPerSecond = m_ulSamplesPerSecond;
	    }

	    HX_ASSERT(m_ulSamplesPerSecond != 0);
	}
    else
    {
        m_ulRTPSamplesPerSecond = 1000; // RDT time stamp
	}

    m_TSConverter.SetBase(m_ulRTPSamplesPerSecond,
                            m_ulSamplesPerSecond);
    }

    // Add this packet to our list of input packets
    pPacket->AddRef();
    m_InputQueue.AddTail(pPacket);

    if (m_bPacketize)
    {
        retVal = SetPacketizerPacket(pPacket);
    }
    else
    {
	    retVal = SetAssemblerPacket(pPacket);
    }

    if (retVal == HXR_NO_DATA)
    {
        retVal = HXR_OK;
    }

    return retVal;
}

HX_RESULT H264PayloadFormat::SetPacketizerPacket(IHXPacket* pPacket)
{
    return HXR_OK;
}

HX_RESULT H264PayloadFormat::SetAssemblerPacket(IHXPacket* pPacket)
{
    return HXR_OK;
}

STDMETHODIMP
H264PayloadFormat::GetPacket(REF(IHXPacket*) pOutPacket)
{
    HX_RESULT retVal = HXR_OK;

    if (m_bPacketize)
    {
        retVal = GetPacketizerPacket(pOutPacket);
    }
    else
    {   
        retVal = GetAssemblerPacket(pOutPacket);
    }

    return retVal;
}

HX_RESULT H264PayloadFormat::GetPacketizerPacket(IHXPacket* &pOutPacket)
{
    HX_RESULT retVal = HXR_NOTIMPL;

    return retVal;
}

HX_RESULT H264PayloadFormat::GetAssemblerPacket(IHXPacket* &pOutPacket)
{
    HX_RESULT retVal = HXR_NOTIMPL;

    return retVal;
}


/******************************************************************************

    This function extracts one AU from m_OutputQueue through StripPacket(),
    if it fails to get AU, it calls ProducePackets(), which produces AU and
    put it in m_OutputQueue so that this function can again access AU from 
    m_OutputQueue and returns it to renderer

 *****************************************************************************/

HX_RESULT H264PayloadFormat::CreateHXCodecPacket(UINT32* &pHXCodecDataOut)
{
    HX_RESULT retVal = HXR_OK;
    HXCODEC_DATA* pHXCodecData = NULL;
	
    pHXCodecData = StripPacket();

    if (!pHXCodecData)
    {
        retVal = ProducePackets();

        if (retVal == HXR_OK)
        {
            pHXCodecData = StripPacket();
        }
        else if ((retVal == HXR_NO_DATA) && m_bFlushed)
        {
            retVal = HXR_STREAM_DONE;
        }
    }


    pHXCodecDataOut = (UINT32*)pHXCodecData;

    return retVal;
}

/******************************************************************************

  Returns one AU from m_OutputQueue

******************************************************************************/

HXCODEC_DATA* H264PayloadFormat::StripPacket(void)
{
    HXCODEC_DATA* pHXCodecData = NULL;

    if (!m_OutputQueue.IsEmpty())
    {
        pHXCodecData = (HXCODEC_DATA*) m_OutputQueue.RemoveHead();
    }

    return pHXCodecData;
}


HXBOOL H264PayloadFormat::IsInitialBufferingDone()
{
    HXBOOL bRetVal = TRUE;
    if(m_ulPacketizationMode == INTERLEAVE_MODE)
    {
        bRetVal = m_bInitialBufferingDone;
    }

    return bRetVal;
}


/******************************************************************************

  Produces one Access Unit by calling functions ParsePacket(),
  DeinterleavePackets(), ReapMediaPacket() successively

******************************************************************************/


HX_RESULT H264PayloadFormat::ProducePackets(void)
{
    HX_RESULT retVal = HXR_NO_DATA; 
    IHXPacket* pPacket;

    while (!m_InputQueue.IsEmpty())
    {
    pPacket = (IHXPacket*) m_InputQueue.RemoveHead();
	    	
        if(pPacket)
        {
            retVal = ParsePacket(pPacket);
        }

        // Don't deinterleave if a NALU is fragmented till 
        // all fragments are collected and NALU is complete
        if (SUCCEEDED(retVal) && !m_bStartNALFragment && IsInitialBufferingDone())
        {
            retVal = DeinterleavePackets();

            while(IsAUComplete())
            {
                retVal = ReapMediaPacket();
            }
        }
        pPacket->Release();
    }

    return retVal;
}

H264PayloadFormat::CNALUPacket* H264PayloadFormat::AllocNALUPacket(IHXBuffer* pBuffer)
{
    CNALUPacket* pNALUPacket = NULL;

    pNALUPacket = new CNALUPacket;
    if (pNALUPacket)
    {
		pNALUPacket->bLost = FALSE;
        pNALUPacket->pBuffer = pBuffer;
        pBuffer->AddRef();
    }
    return pNALUPacket;
}

/******************************************************************************
  Parce received RTP packets based on its type and insert data in 
  m_pNALUPackets in the form of NALU packet. The NALU packet maintns a DON 
  field which is filled with DON number in case of interleave mode and by 
  packet sequence number in non- interleave modeIf the received packet is
  single nal unit it just  puts it into m_pNALUPackets array; if the packet is 
  aggregation packet, breaks it into nal units and stores them into 
  m_pNALUPackets array.If the received packet is fragmentation unit, then insert
  a packet into   m_pNALUPackets and then add remaining fragments in this packet.
  If a fragment lost is detected then discard this packet and repalce it with a 
  lost packet array. If the packet is a lost packet add it in to array without 
  parsing.
  *****************************************************************************/
  


HX_RESULT H264PayloadFormat::ParsePacket(IHXPacket* pPacket)
{
    HX_RESULT retVal = HXR_OK;
    IHXBuffer* pBuffer = NULL;
    UINT8* pData = NULL;
    UINT8 uPacketHeader = 0; 

    CNALUPacket* pNALUPacket = NULL;
    UINT32 ulBuffSize = 0;
    UINT16 usDON = 0;
    UINT16 usDONB = 0;
    UINT32 offset = 0;
    UINT32 ulTimeOffset = 0;
    UINT8 uFUHeader;  
    UINT8 uFUHeaderSE;  //fu indicator
    UINT8 uNALOctet = 0;
    UINT16 usASMRule = pPacket->GetASMRuleNumber();

    HXBOOL bLost = pPacket->IsLost();


        if(!bLost)
        {
            pBuffer = pPacket->GetBuffer();
            UINT32 ulTimeStamp = GetPacketTime(pPacket);

            if(pBuffer)
            {
                pData = pBuffer->GetBuffer();
                ulBuffSize = pBuffer->GetSize();
                if(pData)
                {
                    retVal=HXR_OK;
                }
                pBuffer->Release(); 
            }

            if(SUCCEEDED(retVal))
            {
                if( m_ulLastParsedPktTime < ulTimeStamp)
                {
                    m_ulLastParsedPktTime = ulTimeStamp;  
                }

                uPacketHeader = pData[0];
                UINT8 uPacketType = uPacketHeader & 0x1F;   
                UINT8 uType = 0;
                if(uPacketType >= 1 && uPacketType <= 23)  
                { 
                    uType = SINGLE_NAL; // single nal unit   			
                }
                else if(uPacketType >= STAP_A && uPacketType <= MTAP24)
                {
                    uType = AGGREGATED_NAL; // aggregation nal unit   
                }
                else if(uPacketType == FU_A || uPacketType == FU_B )
                {   
                    uType = FRAGMENTED_NAL; // fragmentation unit   
				}
                else
                {
					HX_DELETE(pNALUPacket);
                    retVal = HXR_FAIL;
                }

                if(uPacketType < 28 && m_bStartNALFragment)
                {
                    // last fragment was lost
                    m_bStartNALFragment = FALSE;
                    m_bFragmentLost = FALSE;			
                }
                switch(uType)
                {
                    case SINGLE_NAL : 
		        			
                        pNALUPacket = AllocNALUPacket(pBuffer);
                        if(pNALUPacket)
                        {
                            pNALUPacket->usDON = (UINT16)m_ulSequenceNumber++;
                            pNALUPacket->ulTimeStamp = ulTimeStamp;
                            pNALUPacket->usASMRule = usASMRule;

                            pNALUPacket->SetDataPtr(pData); 
                            pNALUPacket->SetDataSize(ulBuffSize);
                            
                            m_pNALUPackets[m_ulNumNALUPackets++] = pNALUPacket;
                        }
                        else
                        {
                            retVal = HXR_OUTOFMEMORY;
                        }
                        break;
							 
			
                    case AGGREGATED_NAL:
                        pData++;
                        ulBuffSize--;
 
                        if(uPacketType == STAP_B ) //stap-b
                        {
                            usDON =  (((UINT16)pData[0]<<8) + pData[1]);  
                            pData += 2;
                            ulBuffSize -= 2;
                        }
                        else if( uPacketType == MTAP16 || uPacketType == MTAP24)
                        {
                            usDONB = ((UINT16)pData[0]<<8) + pData[1];
                            pData += 2;
                            ulBuffSize -= 2;					
                        }
                        do{
                            offset = 0 ;
                            ulTimeOffset = 0;

                            pNALUPacket = AllocNALUPacket(pBuffer);
                            if(!pNALUPacket)
                            {
                                retVal = HXR_OUTOFMEMORY;
                                break;
                            }
                            else
                            {
                                pNALUPacket->SetDataSize( ((UINT16)pData[0]<<8) + pData[1] ); 
                                offset = 2;
                                if (uPacketType == MTAP16)
                                {
                                    usDON = usDONB + pData[2];
                                    ulTimeOffset = ((UINT16)pData[3]<<8) + pData[4];
                                    offset += 3;
                                    pNALUPacket->usDON = usDON;
                                }
                                else if (uPacketType == MTAP24)
                                {
                                    usDON = usDONB + pData[2];
                                    ulTimeOffset = ((UINT32)pData[3]<<16) + 
                                            ((UINT32)pData[4]<<8) + pData[5];
                                    offset += 4;
                                    pNALUPacket->usDON = usDON;
                                }
                                else if(uPacketType == STAP_B)
							    {
                                    pNALUPacket->usDON = usDON++;
                                }
                                else
                                {                                    
                                    pNALUPacket->usDON = (UINT16)m_ulSequenceNumber++;
                                }
                                pNALUPacket->ulTimeStamp = ulTimeStamp + ulTimeOffset;
                                pNALUPacket->SetDataPtr(pData + offset); 
                                m_pNALUPackets[m_ulNumNALUPackets++] = pNALUPacket;
                                pData += pNALUPacket->GetDataSize() + offset;
                                ulBuffSize -= pNALUPacket->GetDataSize() + offset;
                            }
					
                        }while(ulBuffSize > 3);
                        m_pNALUPackets[m_ulNumNALUPackets-1]->usASMRule = 
                                                                        usASMRule;
                        /* To find last nalu it is 
                        assumed that if nal unit is present then it will have at
                        least two bytes NALU size and 1 byte NALU octet 
                        otherwise we have reached the end of packet and remaining
                        bytes are rtp padding */				
                        break;
				
                    case FRAGMENTED_NAL:
	
                        // Determine if its a start of nal unit or
                        // fragment of earlier nal 
								
                        uFUHeader = pData[1];
                        uFUHeader = uFUHeader & 0xc0;
                        uFUHeaderSE = uFUHeader >> 6;
                        offset =0;

                        if(uFUHeaderSE == 2) //starting fragment
                        {
                            pNALUPacket = AllocNALUPacket(pBuffer);
                            if(!pNALUPacket)
                            {
                                retVal = HXR_OUTOFMEMORY;
                                break;
                            }
                            else
                            {
                                m_bStartNALFragment = TRUE;
                                m_bFragmentLost = FALSE;
                                // In case of Fragment Unit the NAL unit type octet
                                // of the fragmented NAL unit is not included in 
                                // the fragmentation unit payload,but rather the 
                                // information of NAL unit type octet is conveyed							
                                // in F and NRI fields of indicater octet and in 
                                // the type field of the FU header. We need to 
                                // construct this octet and add before NAL unit data

                                uNALOctet =	(pData[0] & 0xE0) + (pData[1] & 0x1F);

                                // Remove indicater and header octet
                                pData += 2;
                                ulBuffSize -= 2;
    
                                if(uPacketType == FU_B)
                                {
                                    pNALUPacket->usDON = (pData[0] << 8) + pData[1];
                                    offset = 2;
                                }
                                else
                                {
                                    pNALUPacket->usDON = (UINT16)m_ulSequenceNumber++;								
                                }
				    		
                                pNALUPacket->ulTimeStamp = ulTimeStamp;
                                // Add 1 byte for NALU octet
                                retVal = pNALUPacket->ResizeData(ulBuffSize - offset + 1, 0, NULL);  
                                if (FAILED(retVal))
                                {
                                    HX_DELETE(pNALUPacket);
                                    break;
                                }
                                pNALUPacket->GetDataPtr()[0] = uNALOctet;
                                memcpy(pNALUPacket->GetDataPtr() + 1, 
                                       pData + offset, 
                                       pNALUPacket->GetDataSize() - 1);
                                m_pNALUPackets[m_ulNumNALUPackets++] = pNALUPacket;
                            }
                        }
                        else 
                        {
                            // Remove indicater and header octet
                            pData += 2;
                            ulBuffSize -= 2;

                            if(!m_bFragmentLost && m_bStartNALFragment)
                            {
                                // Increase the allocated memory to account 
                                // for additional data and append new data.
                                retVal = m_pNALUPackets[m_ulNumNALUPackets-1] 
                                            ->ResizeData(m_pNALUPackets[m_ulNumNALUPackets-1]->GetDataSize()
                                                            + ulBuffSize,
                                                         ulBuffSize,
                                                         pData);
                                if (FAILED(retVal))
                                {
                                    break;
                                }
                                HX_DELETE(pNALUPacket);
                                m_pNALUPackets[m_ulNumNALUPackets-1]->usASMRule = 
                                                                        usASMRule;
                            }
                            if(uFUHeaderSE == 1)
                            {
                                m_bStartNALFragment = FALSE;
                                m_bFragmentLost = FALSE;								
                            }
                        }
                        break;
                    default : 
                        retVal = HXR_FAIL;						
                        break;
                }
            }		
        }
        else //lost packet
        {
            if(m_bStartNALFragment)
            {
                m_bFragmentLost = TRUE;
                m_pNALUPackets[m_ulNumNALUPackets-1]->bLost = TRUE;
                HX_DELETE(pNALUPacket);
                retVal = HXR_OK;
            }
            else
            {
                pNALUPacket = new CNALUPacket;
                if(!pNALUPacket)
                {
                    retVal = HXR_OUTOFMEMORY;
                }
                else
                {
                    pNALUPacket->bLost = TRUE;
                    pNALUPacket->usDON = (UINT16)m_ulSequenceNumber++;
                    m_pNALUPackets[m_ulNumNALUPackets++] = pNALUPacket;
                    retVal = HXR_OK;
                }
            }
        }

        if(SUCCEEDED(retVal) && (m_ulPacketizationMode == INTERLEAVE_MODE))
        {
            // Determine wether initial buffering is done based on number of NAL
            // units received and the last received packet time. Strip Packet will
            // start taking out packets only if initial buffering is completer. 
            // This is required in case of interleave mode.
            if( (!m_bInitialBufferingDone && 
                (m_ulNumNALUPackets > m_ulSpropInterleavingDepth) &&
                ( m_ulLastParsedPktTime >= m_ulSpropInitBufTime)))
            {
                m_bInitialBufferingDone = TRUE; 
            }
        }
    return retVal;
}


/**********************************************************************

  Sort out the nal units in m_pNALUPackets array by calling the function 
  DeinterleavePacket() for each element in m_pNALUPackets

  **********************************************************************/

HX_RESULT H264PayloadFormat::DeinterleavePackets(void)
{
    UINT32 ulIdx;
    HX_RESULT retVal = HXR_OK;

    for (ulIdx = 0; ulIdx < m_ulNumNALUPackets; ulIdx++)
    {
        if (SUCCEEDED(DeinterleavePacket(m_pNALUPackets[ulIdx])))
        {
            m_pNALUPackets[ulIdx] = NULL;
        }
        else
        {
            HX_DELETE(m_pNALUPackets[ulIdx]);
        }
    }

    m_ulNumNALUPackets = 0;

    return retVal;
}


/***********************************************************************

  Inserts each nal unit into m_DeinterleaveQueue according to their DON

  **********************************************************************/


HX_RESULT H264PayloadFormat::DeinterleavePacket(CNALUPacket* pNALUPacket)
{
    UINT32 ulCount = m_DeinterleaveQueue.GetCount();

    if(pNALUPacket->bLost)
    {
        // Don't add lost packet. In interleave mode it will not 
        // be having correct DON and lost packet can be determine 
        // by missing DON/sequence number in ReapMediaPacket 
        return HXR_FAIL;
    }
    if (ulCount == 0)
    {
        m_DeinterleaveQueue.AddTail(pNALUPacket);
    }
    else
    {
        CNALUPacket* pListNALUPacket;
        LISTPOSITION lPos = m_DeinterleaveQueue.GetTailPosition();
        LISTPOSITION lInsertPos = lPos;
        pListNALUPacket = (CNALUPacket*) m_DeinterleaveQueue.GetAt(lPos);

        do
        {
            if (!MayNALUPacketPrecede(pNALUPacket, pListNALUPacket))
            {
                m_DeinterleaveQueue.InsertAfter(lInsertPos, pNALUPacket);
                break;
            }

            ulCount--;
            if (ulCount == 0)
            {
                if (MayNALUPacketPrecede(pListNALUPacket, pNALUPacket))
                {
                    m_DeinterleaveQueue.InsertBefore(lInsertPos, pNALUPacket);
				}
                else
                {
                    m_DeinterleaveQueue.InsertAfter(lInsertPos, pNALUPacket);
                }
                break;
            }

            pListNALUPacket = (CNALUPacket*) m_DeinterleaveQueue.GetAtPrev(lPos);

            if (MayNALUPacketPrecede(pListNALUPacket, pNALUPacket))
            {
                lInsertPos = lPos;
            }
        } while (TRUE);
    }

    return HXR_OK;
}


/*********************************************************************************

  Checks whether AU can be constructed with so far received nalu packets or has 
  to collect more nalus based on parameter m_ulSpropMaxDonDiff received through sdp 

  ********************************************************************************/

HXBOOL H264PayloadFormat::IsAUComplete()
{
    CNALUPacket* pPacket;
    UINT32 ulCount = 0;
    UINT32 ulNALCount = 1;
    LISTPOSITION lPos = NULL;
    HXBOOL retVal = FALSE;

    ulCount = m_DeinterleaveQueue.GetCount();

    if(ulCount > 1)
    {
        lPos = m_DeinterleaveQueue.GetHeadPosition();
        pPacket = (CNALUPacket*)m_DeinterleaveQueue.GetNext(lPos);
        UINT32 ulPacketTime = pPacket->ulTimeStamp;
			
        while ((ulNALCount < ulCount) && (pPacket->usASMRule%2 != 1))
        {
            pPacket = (CNALUPacket*)m_DeinterleaveQueue.GetNext(lPos);
            if(pPacket->ulTimeStamp != ulPacketTime)
            {
                break;
            }
            ulNALCount++;

        }  /* to count number of nal units that 
             belongs to same AU based on time stamp */

        if((ulCount != ulNALCount) && (ulCount - ulNALCount) >= m_ulSpropMaxDonDiff)
        {
            retVal = TRUE;
        }
    }

    return retVal;
}

/****************************************************************************

  Constructs single AU from nal units in m_DeinterleaveQueue and puts this AU
  into m_OutputQueue

  ***************************************************************************/

HX_RESULT H264PayloadFormat::ReapMediaPacket(void)
{
    CNALUPacket* pPacket = NULL;
    CNALUPacket* pNextpacket = NULL;
    HXCODEC_DATA* pHXCodecData = NULL;	
    HXCODEC_SEGMENTINFO* pHXCodecSegmentInfo = NULL;
    HX_RESULT retVal = HXR_NO_DATA;
    UINT32 ulCount = 0;
    UINT32 ulNALCount = 0;
    UINT32 ulSize;
    CNALUPacket* pNextPacket;
    LISTPOSITION lPos;
    UINT32 ulOffset = 0;
    UINT32* pData = NULL;
    UINT32 ulNumSegments = 0;
    UINT32 ulAUSize = 0;
    UINT32 ulNALSize = 0;
    UINT8 tmp[4];

    if (!m_DeinterleaveQueue.IsEmpty() && (!m_bFlushed))
    {
        pPacket = (CNALUPacket*)m_DeinterleaveQueue.GetHead();
        lPos = m_DeinterleaveQueue.GetHeadPosition();
        ulCount = m_DeinterleaveQueue.GetCount();
        ulSize = pPacket->GetDataSize();
        ulNALCount = 1;
        ulCount--; //Deduct Head position from count

        if((ulCount > 0) || (pPacket->usASMRule%2 == 1))
        {
            // Collect NAL Unit belongs to same AU.
            pNextPacket = (CNALUPacket*)m_DeinterleaveQueue.GetAtNext(lPos);
            while((pNextPacket) && 
                    (pNextPacket->ulTimeStamp == pPacket->ulTimeStamp))
            {
                ulSize += pNextPacket->GetDataSize();
                ulCount--;
                ulNALCount++;
                if(pNextPacket->usASMRule%2 == 1)
                {
                    break;
                }
                pNextPacket = (CNALUPacket*)m_DeinterleaveQueue.GetAtNext(lPos);
            }

#ifdef _APPEND_BITSTREAM_HEADER
		    if (m_bFirstFrame)
		    {
                ulSize += m_ulBitStreamHeaderSize;
            }
#endif	// _APPEND_BITSTREAM_HEADER

            // Allocate codec data header

            pData = new UINT32[(sizeof(HXCODEC_DATA) +
                                sizeof(HXCODEC_SEGMENTINFO) *
                                        ulNALCount) / 4 + 1];

            if(!pData)
            {
                return retVal = HXR_OUTOFMEMORY;
            }

            // Init. codec data header
            pHXCodecData = (HXCODEC_DATA*) pData;;
            pHXCodecData->dataLength = ulSize + NALU_SIZE_FIELD*ulNALCount;
            pHXCodecData->timestamp = pPacket->ulTimeStamp;
            pHXCodecData->sequenceNum = (UINT16)m_ulCodecDataSeqNumber; 
            pHXCodecData->flags = 0;
            pHXCodecData->lastPacket = FALSE;
            pHXCodecData->numSegments = ulNALCount; 


#ifdef _OVERALLOC_CODEC_DATA
            // Over allocate since codec reads 24bits at a time
            ulSize += _OVERALLOC_CODEC_DATA;  
#endif	// _OVERALLOC_CODEC_DATA

            if (m_pAllocator)
            {
                IHXUnknown* pIUnkn = NULL;
                HX20ALLOCPROPS allocRequest;
                HX20ALLOCPROPS allocActual;
	
                allocRequest.uBufferSize = pHXCodecData->dataLength;
                allocRequest.nNumBuffers = 0;
                m_pAllocator->SetProperties(&allocRequest, &allocActual);
                pHXCodecData->data = m_pAllocator->GetPacketBuffer(&pIUnkn);
            }

            else
            {
                pHXCodecData->data = 
                    (UINT8*) new UINT32[pHXCodecData->dataLength / 4 + 1];
            }

            if (pHXCodecData->data == NULL)
            {
                HX_DELETE(pHXCodecData);
                return retVal = HXR_OUTOFMEMORY;
            }

            // Build Codec Data

            pHXCodecSegmentInfo = (HXCODEC_SEGMENTINFO*)&(pHXCodecData->Segments[0]);

            for ( UINT32 ulIdx = 0; ulIdx < ulNALCount ; ulIdx++)
            {
                HX_ASSERT(!m_DeinterleaveQueue.IsEmpty());

                pPacket = (CNALUPacket*) m_DeinterleaveQueue.RemoveHead();
                HX_ASSERT(pPacket);					
                ulNALSize = 0;
                pHXCodecSegmentInfo[ulNumSegments].ulSegmentOffset = ulAUSize;
                // Insert only one lost segment for multiple missing NALUs
                if((pPacket->usDON - m_ulPrevDON) > 1)
                {
                    // Packet is lost so add a lost segment
                    pHXCodecData->numSegments = ulNumSegments++;
                    pHXCodecSegmentInfo[ulNumSegments].bIsValid = FALSE;
                }
                if(m_bFirstPacket)
                {
#ifdef _APPEND_BITSTREAM_HEADER
                    memcpy(pData, m_pBitStreamHeader, m_ulBitStreamHeaderSize);
                    m_bFirstPacket = FALSE;
                    ulAUSize += m_ulBitStreamHeaderSize;
#endif	// _APPEND_BITSTREAM_HEADER
                }

                ulNALSize = pPacket->GetDataSize();
                tmp[0] = UINT8 (ulNALSize >> 24);
                tmp[1] = UINT8 (ulNALSize >> 16);
                tmp[2] = UINT8 (ulNALSize >> 8);
                tmp[3] = (UINT8)ulNALSize;

                HX_ASSERT(pHXCodecData->dataLength >= (ulAUSize + ulNALSize));
                memcpy(pHXCodecData->data + ulAUSize, tmp, NALU_SIZE_FIELD);
                memcpy(pHXCodecData->data + ulAUSize+NALU_SIZE_FIELD,
                       pPacket->GetDataPtr(),
                       ulNALSize);
                ulAUSize += (ulNALSize+NALU_SIZE_FIELD);
                m_ulPrevDON = pPacket->usDON;
                HX_DELETE(pPacket);
							
                pHXCodecData->numSegments = ulNumSegments++;
                pHXCodecSegmentInfo[ulNumSegments].bIsValid = TRUE;

            }
            m_OutputQueue.AddTail(pHXCodecData);
            retVal=HXR_OK;
            m_ulCodecDataSeqNumber++;
        }
    }
    return retVal;
}


STDMETHODIMP
H264PayloadFormat::Flush()
{
    m_bFlushed = TRUE;

    return HXR_OK;
}

void H264PayloadFormat::FlushQueues(void)
{
    FlushInputQueue(FLUSH_ALL_PACKETS);
    FlushDeinterleaveQueue(FLUSH_ALL_PACKETS);
    FlushOutputQueue(FLUSH_ALL_PACKETS);
}

void H264PayloadFormat::FlushInputQueue(UINT32 ulCount)
{
    IHXPacket* pDeadPacket;

    while ((ulCount > 0) && (!m_InputQueue.IsEmpty()))
    {
        pDeadPacket = (IHXPacket*) m_InputQueue.RemoveHead();
        HX_RELEASE(pDeadPacket);
        if (ulCount != FLUSH_ALL_PACKETS)
        {
            ulCount--;
	    }
    }
}

void H264PayloadFormat::FlushDeinterleaveQueue(UINT32 ulCount)
{
    CNALUPacket* pDeadPacket;

    while ((ulCount > 0) && (!m_DeinterleaveQueue.IsEmpty()))
    {
        pDeadPacket = (CNALUPacket*) m_DeinterleaveQueue.RemoveHead();
        HX_DELETE(pDeadPacket);
        if (ulCount != FLUSH_ALL_PACKETS)
        {
            ulCount--;
        }
    }
}

void H264PayloadFormat::FlushOutputQueue(UINT32 ulCount)
{
    HXCODEC_DATA* pDeadPacket;

    while ((ulCount > 0) && (!m_OutputQueue.IsEmpty()))
    {
        pDeadPacket = (HXCODEC_DATA*) m_OutputQueue.RemoveHead();
        HX_DELETE(pDeadPacket);
        if (ulCount != FLUSH_ALL_PACKETS)
        {
            ulCount--;
        }
    }
}


void H264PayloadFormat::FlushArrays(void)
{
    FlushNALUArray(m_pNALUPackets, m_ulNumNALUPackets);
}


UINT32 H264PayloadFormat::GetPacketTime(IHXPacket* pPacket)
{
    UINT32 ulTime;

    HX_ASSERT(pPacket);

    if (m_bUsesRTPPackets)
    {
        ulTime = ((IHXRTPPacket*) pPacket)->GetRTPTime();
    }
    else
    {
        ulTime = pPacket->GetTime();
    }

    ulTime = m_TSConverter.Convert(ulTime);

    return ulTime;
}

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
#include "hxcom.h"
#include "hxplugn.h"
#include "ihxpckts.h"
#include "hxformt.h"
#include "hxccf.h"
#include "hxfiles.h"
#include "hxengin.h"
#include "baseobj.h"
#include "pckunpck.h"
#include "hxver.h"
#include "hxamrpack.h"
#include "amr_frame_info.h"
#include "amr_frame_hdr.h"
#include "amrffmlog.h"
#include "amrff.h"
#include "amrff.ver"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

const char* const CAMRFileFormat::m_pszDescription        = "Helix AMR File Format Plugin";
const char* const CAMRFileFormat::m_pszCopyright          = HXVER_COPYRIGHT;
const char* const CAMRFileFormat::m_pszMoreInfoURL        = HXVER_MOREINFO;
const char* const CAMRFileFormat::m_ppszFileMimeTypes[]   = {"audio/amr", "audio/awb", NULL};
const char* const CAMRFileFormat::m_ppszFileExtensions[]  = {"amr", "awb", NULL};
const char* const CAMRFileFormat::m_ppszFileOpenNames[]   = {"AMR Files (*.amr,*.awb)", NULL};
const char* const CAMRFileFormat::m_ppszStreamMimeTypes[] = {"audio/amr",
                                                             "audio/awb",
                                                             "audio/X-RN-3GPP-AMR",
                                                             "audio/X-RN-3GPP-AMR-WB",
                                                             NULL};
const BYTE        CAMRFileFormat::m_pMagicSingle[6]       = {0x23,0x21,0x41,0x4d,0x52,0x0a};
const BYTE        CAMRFileFormat::m_pMagicSingleWB[9]     = {0x23,0x21,0x41,0x4d,0x52,0x2d,0x57,0x42,0x0a};
const BYTE        CAMRFileFormat::m_pMagicMulti[12]       = {0x23,0x21,0x41,0x4d,0x52,0x5F,0x4D,0x43,0x31,
                                                             0x2E,0x30,0x0a};
const BYTE        CAMRFileFormat::m_pMagicMultiWB[15]     = {0x23,0x21,0x41,0x4d,0x52,0x2d,0x57,0x42,0x5F,
                                                             0x4D,0x43,0x31,0x2E,0x30,0x0a};
const BYTE        CAMRFileFormat::m_ucNumChannelsMap[16]  = {0,2,3,4,4,5,6,0,0,0,0,0,0,0,0,0};

//#define DEBUG_TEST_FRAME_SCAN

CAMRFileFormat::CAMRFileFormat()
{
    MLOG_LEAK("CON CAMRFileFormat this=0x%08x\n", this);
    m_lRefCount           = 0;
    m_pContext            = NULL;
    m_pCommonClassFactory = NULL;
    m_pFileObject         = NULL;
    m_pFormatResponse     = NULL;
    m_pFileStat           = NULL;
    m_eState              = StateReady;
    m_ulHeaderSize        = 0;
    m_ulBytesPerFrame     = 0;
    m_ulFileSize          = 0;
    m_ulNumChannels       = 0;
    m_ulNextFileOffset    = 0;
    m_ulNextTimeStamp     = 0;
    m_pPayloadFormat      = NULL;
    m_bWideBand           = FALSE;
    m_bScanForFrameBegin  = FALSE;
}

CAMRFileFormat::~CAMRFileFormat()
{
    MLOG_LEAK("DES CAMRFileFormat this=0x%08x\n", this);
    Close();
}

STDMETHODIMP CAMRFileFormat::QueryInterface(REFIID riid, void** ppvObj)
{
    HX_RESULT retVal = HXR_OK;

    if (ppvObj)
    {
        // NULL out by default
        *ppvObj = NULL;
        // Switch based on IID
        if (IsEqualIID(riid, IID_IUnknown))
        {
            AddRef();
            *ppvObj = (IUnknown*) (IHXPlugin*) this;
        }
        else if (IsEqualIID(riid, IID_IHXPlugin))
        {
            AddRef();
            *ppvObj = (IHXPlugin*) this;
        }
        else if (IsEqualIID(riid, IID_IHXFileFormatObject))
        {
            AddRef();
            *ppvObj = (IHXFileFormatObject*) this;
        }
        else if (IsEqualIID(riid, IID_IHXFileResponse))
        {
            AddRef();
            *ppvObj = (IHXFileResponse*) this;
        }
        else if (IsEqualIID(riid, IID_IHXFileStatResponse))
        {
            AddRef();
            *ppvObj = (IHXFileStatResponse*) this;
        }
        else if (IsEqualIID(riid, IID_IHXInterruptSafe))
        {
            AddRef();
            *ppvObj = (IHXInterruptSafe*) this;
        }
        else
        {
            retVal = HXR_NOINTERFACE;
        }
    }
    else
    {
        retVal = HXR_FAIL;
    }

    return retVal;
}

STDMETHODIMP_(ULONG32) CAMRFileFormat::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CAMRFileFormat::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP CAMRFileFormat::InitPlugin(IUnknown* pContext)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pContext)
    {
        // Save the context
        HX_RELEASE(m_pContext);
        m_pContext = pContext;
        m_pContext->AddRef();
        // Get the commom class factory
        HX_RELEASE(m_pCommonClassFactory);
        retVal = m_pContext->QueryInterface(IID_IHXCommonClassFactory,
                                            (void**) &m_pCommonClassFactory);
    }

    return retVal;
}

STDMETHODIMP CAMRFileFormat::GetPluginInfo(REF(HXBOOL)        rbLoadMultiple,
                                           REF(const char*) rpszDescription,
                                           REF(const char*) rpszCopyright,
                                           REF(const char*) rpszMoreInfoURL,
                                           REF(ULONG32)     rulVersionNumber)
{
    rbLoadMultiple   = TRUE;   // Must be true for file formats.
    rpszDescription  = (const char*) m_pszDescription;
    rpszCopyright    = (const char*) m_pszCopyright;
    rpszMoreInfoURL  = (const char*) m_pszMoreInfoURL;
    rulVersionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}

STDMETHODIMP CAMRFileFormat::GetFileFormatInfo(REF(const char**) rppszFileMimeTypes,
                                               REF(const char**) rppszFileExtensions,
                                               REF(const char**) rppszFileOpenNames)
{
    rppszFileMimeTypes  = (const char**) m_ppszFileMimeTypes;
    rppszFileExtensions = (const char**) m_ppszFileExtensions;
    rppszFileOpenNames  = (const char**) m_ppszFileOpenNames;

    return HXR_OK;
}

STDMETHODIMP CAMRFileFormat::InitFileFormat(IHXRequest*        pRequest, 
                                            IHXFormatResponse* pFormatResponse,
                                            IHXFileObject*     pFileObject)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_eState == StateReady)
    {
        retVal = HXR_FAIL;
        if (pFormatResponse && pFileObject)
        {
            // Save response interface
            HX_RELEASE(m_pFormatResponse);
            m_pFormatResponse = pFormatResponse;
            m_pFormatResponse->AddRef();
            // Save file object
            HX_RELEASE(m_pFileObject);
            m_pFileObject = pFileObject;
            m_pFileObject->AddRef();
            // Get our own IHXFileResponse interface
            IHXFileResponse* pResponse = NULL;
            retVal = QueryInterface(IID_IHXFileResponse, (void**) &pResponse);
            if (pResponse)
            {
                // Set the state
                m_eState = StateInitFileFormatInitDonePending;
                // Init the file object
                retVal   = m_pFileObject->Init(HX_FILE_READ | HX_FILE_BINARY, pResponse);
            }
            HX_RELEASE(pResponse);
        }
        if (FAILED(retVal) && pFormatResponse)
        {
            pFormatResponse->InitDone(retVal);
        }
    }

    return retVal;
}

STDMETHODIMP CAMRFileFormat::Close()
{
    if (m_pFileObject)
    {
        m_pFileObject->Close();
    }
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pCommonClassFactory);
    HX_RELEASE(m_pFileObject);
    HX_RELEASE(m_pFormatResponse);
    HX_RELEASE(m_pFileStat);
    HX_DELETE(m_pPayloadFormat);
    m_eState            = StateReady;
    m_ulFileSize        = 0;
    m_ulNumChannels     = 0;
    m_ulNextFileOffset  = 0;
    m_ulNextTimeStamp   = 0;
    m_bWideBand         = FALSE;

    return HXR_OK;
}

STDMETHODIMP CAMRFileFormat::GetFileHeader()
{
    HX_RESULT retVal = HXR_OK;

    if (m_eState == StateReady)
    {
        // Set our new state
        m_eState = StateFileHeaderSeekDonePending;
        // Seek back to the beginning of the file
        retVal = m_pFileObject->Seek(0, FALSE);
    }
    else
    {
        retVal = HXR_UNEXPECTED;
    }

    return retVal;
}

STDMETHODIMP CAMRFileFormat::GetStreamHeader(UINT16 unStreamNumber)
{
    HX_RESULT retVal = HXR_OK;

    if (m_eState == StateReady)
    {
        // Set the state
        m_eState = StateStreamHeaderSeekDonePending;
        // m_ulNextFileOffset should be set the offset of
        // the first AMR frame in the file
        retVal = m_pFileObject->Seek(m_ulNextFileOffset, FALSE);
    }
    else
    {
        retVal = HXR_UNEXPECTED;
    }

    return retVal;
}

STDMETHODIMP CAMRFileFormat::GetPacket(UINT16 unStreamNumber)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_eState == StateReady)
    {
        if (m_pPayloadFormat)
        {
            // Try and get a packet from the payload format object
            IHXPacket* pPacket = NULL;
            retVal = m_pPayloadFormat->GetPacket(pPacket);
            if (SUCCEEDED(retVal))
            {
                // The payload format object had a packet ready,
                // so send it. We don't have to change the state
                retVal = m_pFormatResponse->PacketReady(HXR_OK, pPacket);
            }
            else
            {
                // Clear the return value. It's not an error
                // if the payload format object doesn't have any
                // packets - it just means we need to provide more data.
                retVal = HXR_OK;
                // We need to provide the payload format object
                // some more of the file, so we will seek to the
                // new file offset.
                m_eState = StateGetPacketSeekDonePending;
                // Seek the file
                retVal = m_pFileObject->Seek(m_ulNextFileOffset, FALSE);
            }
            HX_RELEASE(pPacket);
        }
    }

    return retVal;
}

STDMETHODIMP CAMRFileFormat::Seek(ULONG32 ulOffset)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pFormatResponse && m_pPayloadFormat)
    {
        // Clear out the packets from the payload format object
        m_pPayloadFormat->Reset();
        // First convert the time offset to a file offset,
        // assuming a constant bitrate and therefore a 
        // constant number of bytes per frame. We know that
        // AMR frames always have a duration of 20ms.
        UINT32 ulFrameNum   = ulOffset / 20;
        UINT32 ulFileOffset = ulFrameNum * m_ulBytesPerFrame + m_ulHeaderSize;
        // Compute the next time stamp
        m_ulNextTimeStamp = ulFrameNum * 20;
        // Set the next file offset
        m_ulNextFileOffset = ulFileOffset;
        // In the case of files where the bytes per frame is not constant
        // throughout the file, the following file seek might
        // not land on a frame boundary. Therefore, this flag
        // will tell us that we need to scan the buffer we read
        // for a frame boundary.
        m_bScanForFrameBegin = TRUE;
        // Tell the format response we're ready
        retVal = m_pFormatResponse->SeekDone(HXR_OK);
    }

    return retVal;
}

STDMETHODIMP CAMRFileFormat::InitDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_OK;

    if (m_eState == StateInitFileFormatInitDonePending)
    {
        // Get the IHXFileStat interface
        HX_RELEASE(m_pFileStat);
        retVal = m_pFileObject->QueryInterface(IID_IHXFileStat, (void**) &m_pFileStat);
        if (SUCCEEDED(retVal))
        {
            // Get our own IHXFileStatResponse interface
            IHXFileStatResponse* pStatResponse = NULL;
            retVal = QueryInterface(IID_IHXFileStatResponse, (void**) &pStatResponse);
            if (SUCCEEDED(retVal))
            {
                // Set the state
                m_eState = StateInitFileFormatStatDonePending;
                // Call Stat
                retVal = m_pFileStat->Stat(pStatResponse);
            }
            HX_RELEASE(pStatResponse);
        }
        if (FAILED(retVal))
        {
            // Set the state
            m_eState = StateReady;
            // Call back to the response
            retVal = m_pFormatResponse->InitDone(retVal);
        }
    }
    else
    {
        retVal = HXR_UNEXPECTED;
    }

    return retVal;
}

STDMETHODIMP CAMRFileFormat::SeekDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_OK;

    if (m_eState == StateFileHeaderSeekDonePending)
    {
        if (SUCCEEDED(status))
        {
            // Set the new state
            m_eState = StateFileHeaderReadDonePending;
            // Now read AMR_HEADER_READ_SIZE bytes
            retVal = m_pFileObject->Read(AMR_HEADER_READ_SIZE);
        }
        else
        {
            // Go back to the StateReady state
            m_eState = StateReady;
            // Fail out to the response interface
            retVal = m_pFormatResponse->FileHeaderReady(status, NULL);
        }
    }
    else if (m_eState == StateStreamHeaderSeekDonePending)
    {
        if (SUCCEEDED(status))
        {
            // Set the new state
            m_eState = StateStreamHeaderReadDonePending;
            // Now read AMR_READ_SIZE bytes
            retVal = m_pFileObject->Read(AMR_READ_SIZE);
        }
        else
        {
            // Go back to StateReady state
            m_eState = StateReady;
            // Fail out to response interface
            retVal = m_pFormatResponse->StreamHeaderReady(status, NULL);
        }
    }
    else if (m_eState == StateGetPacketSeekDonePending)
    {
        if (SUCCEEDED(status))
        {
            // Set the new state
            m_eState = StateGetPacketReadDonePending;
            // Read AMR_READ_SIZE more bytes
            retVal = m_pFileObject->Read(AMR_READ_SIZE);
        }
        else
        {
            // Go back to ready
            m_eState = StateReady;
            // Issue a StreamDone()
            retVal = m_pFormatResponse->StreamDone(0);
        }
    }
    else
    {
        retVal = HXR_UNEXPECTED;
    }

    return retVal;
}

STDMETHODIMP CAMRFileFormat::ReadDone(HX_RESULT status, IHXBuffer* pBuffer)
{
    HX_RESULT retVal = HXR_OK;

    if (m_eState == StateFileHeaderReadDonePending)
    {
        // Assume the worst
        retVal = HXR_FAIL;
        // Process the first few bytes of the file
        if (SUCCEEDED(status) && pBuffer)
        {
            // Check the first five bytes to identify
            // this as an AMR file
            BYTE* pBuf = (BYTE*) pBuffer->GetBuffer();
            if (pBuf)
            {
                // Test the magic number
                if (!memcmp(pBuf, m_pMagicSingle, sizeof(m_pMagicSingle)))
                {
                    m_bWideBand        = FALSE;
                    m_ulNumChannels    = 1;
                    m_ulHeaderSize = sizeof(m_pMagicSingle);
                    retVal             = HXR_OK;
                }
                else if (!memcmp(pBuf, m_pMagicSingleWB, sizeof(m_pMagicSingleWB)))
                {
                    m_bWideBand        = TRUE;
                    m_ulNumChannels    = 1;
                    m_ulHeaderSize = sizeof(m_pMagicSingleWB);
                    retVal             = HXR_OK;
                }
                else if (!memcmp(pBuf, m_pMagicMulti, sizeof(m_pMagicMulti)))
                {
                    m_bWideBand        = FALSE;
                    m_ulNumChannels    = m_ucNumChannelsMap[pBuf[sizeof(m_pMagicMulti) + 3] & 0x0F];
                    m_ulHeaderSize = sizeof(m_pMagicMulti) + 4;
                    if (m_ulNumChannels != 0)
                    {
                        retVal = HXR_OK;
                    }
                }
                else if (!memcmp(pBuf, m_pMagicMultiWB, sizeof(m_pMagicMultiWB)))
                {
                    m_bWideBand        = TRUE;
                    m_ulNumChannels    = m_ucNumChannelsMap[pBuf[sizeof(m_pMagicMultiWB) + 3] & 0x0F];
                    m_ulHeaderSize = sizeof(m_pMagicMultiWB) + 4;
                    if (m_ulNumChannels != 0)
                    {
                        retVal = HXR_OK;
                    }
                }
                else
                {
                    // This is not an AMR file
                    retVal = HXR_FAIL;
                }
                if (SUCCEEDED(retVal))
                {
                    // Set the next file offset to right after
                    // the file header
                    m_ulNextFileOffset = m_ulHeaderSize;
                    // Create an IHXValues
                    IHXValues* pHdr = NULL;
                    retVal = m_pCommonClassFactory->CreateInstance(CLSID_IHXValues, (void**) &pHdr);
                    if (SUCCEEDED(retVal))
                    {
                        // Set the StreamCount property
                        pHdr->SetPropertyULONG32("StreamCount", 1);
                        // Set the new state
                        m_eState = StateReady;
                        // Send the file header
                        retVal = m_pFormatResponse->FileHeaderReady(HXR_OK, pHdr);
                    }
                    HX_RELEASE(pHdr);
                }
            }
        }
        if (FAILED(retVal))
        {
            // Go back to the StateFileFormatInitialized state
            m_eState = StateReady;
            // Fail out to the response interface
            retVal = m_pFormatResponse->FileHeaderReady(retVal, NULL);
        }
    }
    else if (m_eState == StateStreamHeaderReadDonePending)
    {
        retVal = status;
        if (SUCCEEDED(retVal))
        {
            // Create our CHXAMRPayloadFormat object
            retVal = HXR_OUTOFMEMORY;
            HX_DELETE(m_pPayloadFormat);
            m_pPayloadFormat = new CHXAMRPayloadFormatPacketizer();
            if (m_pPayloadFormat)
            {
                // Init the payload format
                retVal = m_pPayloadFormat->Init(m_pContext, TRUE);
                if (SUCCEEDED(retVal))
                {
                    // Create an IHXValues
                    IHXValues* pHdr = NULL;
                    retVal = m_pCommonClassFactory->CreateInstance(CLSID_IHXValues, (void**) &pHdr);
                    if (SUCCEEDED(retVal))
                    {
                        // Set the MimeType
                        INT32 lIndx = (m_bWideBand ? 3 : 2);
                        SetCStringPropertyCCF(pHdr, "MimeType", m_ppszStreamMimeTypes[lIndx], m_pContext);
                        // Compute the average bitrate
                        UINT32 ulAvgBitRate = 0;
                        GetStreamInfo(pBuffer, 0, ulAvgBitRate, m_ulBytesPerFrame);
                        // Set the ASM rule book
                        char szASMRuleBook[64]; /* Flawfinder: ignore */
                        sprintf(szASMRuleBook, "AverageBandwidth=%lu,Priority=5;", ulAvgBitRate); /* Flawfinder: ignore */
                        SetCStringPropertyCCF(pHdr, "ASMRuleBook", szASMRuleBook, m_pContext);
                        // Compute the duration
                        UINT32 ulDur = 1000;
                        if (ulAvgBitRate != 0)
                        {
                        	ulDur = (ULONG32)((double)m_ulFileSize * 8000.0 / (double)ulAvgBitRate);
                        }
                        // Set some properties
                        pHdr->SetPropertyULONG32("StreamNumber",  0);
                        pHdr->SetPropertyULONG32("MaxBitRate",    ulAvgBitRate);
                        pHdr->SetPropertyULONG32("AvgBitRate",    ulAvgBitRate);
                        pHdr->SetPropertyULONG32("MaxPacketSize", AMR_IDEAL_PACKET_SIZE);
                        pHdr->SetPropertyULONG32("AvgPacketSize", AMR_IDEAL_PACKET_SIZE);
                        pHdr->SetPropertyULONG32("MinPacketSize", AMR_IDEAL_PACKET_SIZE);
                        pHdr->SetPropertyULONG32("StartTime",     0);
                        pHdr->SetPropertyULONG32("Duration",      ulDur);
                        pHdr->SetPropertyULONG32("SamplesPerSecond",m_bWideBand? 16000 : 8000);
                        pHdr->SetPropertyULONG32("Channels",   m_ulNumChannels);
                        // Pass this stream header into the payload format object
                        retVal = m_pPayloadFormat->SetStreamHeader(pHdr);
                        if (SUCCEEDED(retVal))
                        {
                            // Create a raw file packet
                            IHXPacket* pPacket = NULL;
                            retVal = MakeRawFilePacket(pBuffer, m_ulNextTimeStamp, pPacket);
                            if (SUCCEEDED(retVal))
                            {
                                // Add this packet to the payload object
                                retVal = m_pPayloadFormat->SetPacket(pPacket);
                                if (SUCCEEDED(retVal))
                                {
                                    // Update the next file offset
                                    m_ulNextFileOffset += m_pPayloadFormat->GetPacketBytesConsumed();
                                    // Update the next time stamp
                                    m_ulNextTimeStamp  += m_pPayloadFormat->GetDurationConsumed();
                                    // Set the state
                                    m_eState = StateReady;
                                    // Send the stream header
                                    retVal = m_pFormatResponse->StreamHeaderReady(HXR_OK, pHdr);
                                }
                            }
                            HX_RELEASE(pPacket);
                        }
                    }
                    HX_RELEASE(pHdr);
                }
            }
        }
        if (FAILED(retVal))
        {
            // Return to StateReady
            m_eState = StateReady;
            // Fail out to response interface
            retVal = m_pFormatResponse->StreamHeaderReady(retVal, NULL);
        }
    }
    else if (m_eState == StateGetPacketReadDonePending)
    {
        retVal = status;
        if (SUCCEEDED(retVal))
        {
            // Do we need to scan this buffer for a frame begin?
            IHXBuffer* pNewBuffer = NULL;
            UINT32     ulNewBufferOffset = 0;
            if (m_bScanForFrameBegin)
            {
#ifdef DEBUG_TEST_FRAME_SCAN
                // Force the first fifth of the buffer to be invalid
                memset(pBuffer->GetBuffer(), 0x83, pBuffer->GetSize() / 5);
#endif
                // Scan the buffer
                UINT32 ulOffset = 0;
                BYTE   bRet = CAMRFrameInfo::FindFrameBegin((m_bWideBand ? WideBand : NarrowBand),
                                                            pBuffer->GetBuffer(),
                                                            pBuffer->GetSize(),
                                                            AMR_MIN_CONSEC_FRAMES,
                                                            ulOffset);
                if (bRet)
                {
                    // If the offset is at the beginning
                    // of the buffer, then we don't have to
                    // do anything. If the offset is NOT at the
                    // beginning, then we need to create a new
                    // buffer and copy the old one into it.
                    if (ulOffset > 0 && ulOffset < pBuffer->GetSize())
                    {
                        // We have a valid offset
                        retVal = m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                                                       (void**) &pNewBuffer);
                        if (SUCCEEDED(retVal))
                        {
                            retVal = pNewBuffer->Set(pBuffer->GetBuffer() + ulOffset,
                                                     pBuffer->GetSize()   - ulOffset);
                            if (SUCCEEDED(retVal))
                            {
                                ulNewBufferOffset = ulOffset;
                            }
                        }
                    }
                    else if (ulOffset != 0)
                    {
                        // Huh? the offset was outside the buffer? Surrender!
                        retVal = HXR_FAIL;
                    }
                }
                else
                {
                    // Yikes! We didn't find a frame in this
                    // buffer at all? We surrender!
                    retVal = HXR_FAIL;
                }
                // Clear the flag
                m_bScanForFrameBegin = FALSE;
            }
            if (SUCCEEDED(retVal))
            {
                // Create a raw file packet
                IHXPacket* pRawPacket = NULL;
                retVal = MakeRawFilePacket((pNewBuffer ? pNewBuffer : pBuffer),
                                           m_ulNextTimeStamp, pRawPacket);
                if (SUCCEEDED(retVal))
                {
                    // If the buffer was less than we read, then we are 
                    // probably at the end of the file. Therefore, we want
                    // to process ALL of this buffer, regardless of the
                    // minimum packet size. So we need to tell the payload
                    // format object to Flush().
                    if (pBuffer->GetSize() < AMR_READ_SIZE)
                    {
                        m_pPayloadFormat->Flush();
                    }
                    // Add this packet to the payload object
                    retVal = m_pPayloadFormat->SetPacket(pRawPacket);
                    if (SUCCEEDED(retVal))
                    {
                        // Update the next file offset
                        m_ulNextFileOffset += m_pPayloadFormat->GetPacketBytesConsumed() + ulNewBufferOffset;
                        // Update the next time stamp
                        m_ulNextTimeStamp  += m_pPayloadFormat->GetDurationConsumed();
                        // Did we get here from a GetPacket() or a seek
                        // Now try and get a packet from the payload format object
                        IHXPacket* pPacket = NULL;
                        retVal = m_pPayloadFormat->GetPacket(pPacket);
                        if (SUCCEEDED(retVal))
                        {
                            // Set the state
                            m_eState = StateReady;
                            // We have a packet to give, so give it
                            retVal = m_pFormatResponse->PacketReady(retVal, pPacket);
                        }
                        HX_RELEASE(pPacket);
                    }
                }
                HX_RELEASE(pRawPacket);
            }
            HX_RELEASE(pNewBuffer);
        }
        if (FAILED(retVal))
        {
            // We either got here because of something bad, or because
            // we simply got to the end of the file and couldn't read
            // any more. Either way, we issue a StreamDone().
            //
            // Go back to ready
            m_eState = StateReady;
            // Issue a StreamDone
            retVal = m_pFormatResponse->StreamDone(0);
        }
    }
    else
    {
        retVal = HXR_UNEXPECTED;
    }

    return retVal;
}

STDMETHODIMP CAMRFileFormat::WriteDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_OK;

    return retVal;
}

STDMETHODIMP CAMRFileFormat::CloseDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_OK;

    return retVal;
}

STDMETHODIMP CAMRFileFormat::StatDone(HX_RESULT status, UINT32 ulSize, UINT32 ulCreationTime,
                                      UINT32 ulAccessTime, UINT32 ulModificationTime, UINT32 ulMode)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_eState == StateInitFileFormatStatDonePending)
    {
        // Did we succeed?
        if (SUCCEEDED(status))
        {
            // Save the file size
            m_ulFileSize = ulSize;
            // Now we are done with the stat interface
            HX_RELEASE(m_pFileStat);
        }
        // Set the state
        m_eState = StateReady;
        // Call back to response
        retVal = m_pFormatResponse->InitDone(status);
    }

    return retVal;
}

HX_RESULT STDAPICALLTYPE CAMRFileFormat::HXCreateInstance(IUnknown** ppIUnknown)
{
    HX_RESULT retVal = HXR_FAIL;

    if (ppIUnknown)
    {
        // Set default
        *ppIUnknown = NULL;
        // Create the object
        CAMRFileFormat *pObj = new CAMRFileFormat();
        if (pObj)
        {
            // QI for IUnknown
            retVal = pObj->QueryInterface(IID_IUnknown, (void**) ppIUnknown);
        }
        if (FAILED(retVal))
        {
            HX_DELETE(pObj);
        }
    }

    return retVal;
}

HX_RESULT STDAPICALLTYPE CAMRFileFormat::CanUnload2(void)
{
    return ((CHXBaseCountingObject::ObjectsActive() > 0) ? HXR_FAIL : HXR_OK);
}

HX_RESULT CAMRFileFormat::GetStreamInfo(IHXBuffer*  pBuffer,
                                        UINT32      ulOffset,
                                        REF(UINT32) rulAvgBitRate,
                                        REF(UINT32) rulBytesPerFrame)
{
    HX_RESULT retVal = HXR_OK;

    if (pBuffer && ulOffset < pBuffer->GetSize())
    {
        BYTE*     pBuf      = pBuffer->GetBuffer() + ulOffset;
        BYTE*     pBufLimit = pBuffer->GetBuffer() + pBuffer->GetSize();
        UINT32    ulByteSum = 0;
        UINT32    ulMSSum   = 0;
        UINT32    ulNumFr   = 0;
        CAMRFrameHdr cHdr((m_bWideBand ? WideBand : NarrowBand));
        while (1)
        {
            if (pBuf + cHdr.HdrBytes() >= pBufLimit)
            {
                break;
            }
            cHdr.Unpack(pBuf);
            if(pBuf + cHdr.DataBytes() >= pBufLimit)
            {
                break;
            }
            pBuf += cHdr.DataBytes();
            ulByteSum += cHdr.DataBytes() + cHdr.HdrBytes();
            ulMSSum   += CAMRFrameInfo::FrameDuration();
            ulNumFr++;
        }
        if (ulNumFr) rulBytesPerFrame = ulByteSum / ulNumFr;
        if (ulMSSum) rulAvgBitRate = ulByteSum * 8000 / ulMSSum;
    }
    else
    {
        retVal = HXR_FAIL;
    }

    return retVal;
}

HX_RESULT CAMRFileFormat::MakeRawFilePacket(IHXBuffer* pBuffer, UINT32 ulTimeStamp,
                                            REF(IHXPacket*) rpPacket)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pBuffer && m_pCommonClassFactory)
    {
        IHXPacket* pPacket = NULL;
        retVal = m_pCommonClassFactory->CreateInstance(CLSID_IHXPacket, (void**) &pPacket);
        if (SUCCEEDED(retVal))
        {
            retVal = pPacket->Set(pBuffer, ulTimeStamp, 0, HX_ASM_SWITCH_ON, 0);
            if (SUCCEEDED(retVal))
            {
                HX_RELEASE(rpPacket);
                rpPacket = pPacket;
                rpPacket->AddRef();
            }
        }
        HX_RELEASE(pPacket);
    }

    return retVal;
}


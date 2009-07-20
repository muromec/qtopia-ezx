/* ***** BEGIN LICENSE BLOCK *****  
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
#include "hxcom.h"
#include "fileformat_handler.h"

#include "ihxpckts.h"
#include "hxpiids.h"
#include "proc.h"

#include "chxmaplongtoobj.h"
#include "tconverter.h"
#include "asmrulep.h"
#include "hxvalues.h"
#include "hxstring.h"
#include "hxstrutl.h"

#include "hxmon.h"
#include "hxengin.h"

#include "servpckts.h"
#include "streamgroupmgr.h"

const UINT32 zm_dprintf = 0x0400022;
static const UINT32 DEFAULT_PREROLL = 1000;
const UINT16 MAX_RULE_COUNT = 0xffff;

CHeaderHandler::CHeaderHandler(Process* pProc,
                               IHXFileFormatObject* pFF,
                               IUnknown* pFileObject,
                               IHXRequest* pRequest)
    : m_ulRefCount(0)
    , m_pProc(pProc)
    , m_pFileFormat(pFF)
    , m_pFileObject(pFileObject)
    , m_pRequest(pRequest)
    , m_pFormatResponse(NULL)
    , m_pHeaderSink(NULL)
    , m_bDone(FALSE)
    , m_pFileHeader(NULL)
    , m_pAsmSource(NULL)
    , m_pStreamHeaderMap(NULL)
    , m_ulNumLogicalStreams(0)
    , m_ulNumStreamGroups(0)
    , m_aulActualLogicalStream(NULL)
    , m_bSimulateStreamGroups(FALSE)
    , m_pPullPacketSink(NULL)
    , m_pInitialSubscription(FALSE)
    , m_bStreamErrorReported(FALSE)
{
    DPRINTF(zm_dprintf, ("%p: CHeaderHandler::CHeaderHandler()\n", this));

    HX_ASSERT(m_pProc && m_pFileFormat && m_pFileObject);

    if (m_pFileFormat)
    {
        m_pFileFormat->AddRef();
	m_pFileFormat->QueryInterface(IID_IHXASMSource, (void**)&m_pAsmSource);
    }
    if (pFileObject)
    {
        m_pFileObject->AddRef();
    }
    if (m_pRequest)
    {
        m_pRequest->AddRef();
    }
}

CHeaderHandler::~CHeaderHandler()
{
    DPRINTF(zm_dprintf, ("%p: CHeaderHandler::~CHeaderHandler()\n", this));

    _Done();
}

void
CHeaderHandler::_Done()
{
    DPRINTF(zm_dprintf, ("%p: CHeaderHandler::_Done()\n", this));

    if (m_bDone)
    {
        return;
    }

    m_bDone = TRUE;

    HX_DELETE(m_pInitialSubscription);

    if (m_pFileFormat)
    {
        m_pFileFormat->Close();
        m_pFileFormat->Release();
        m_pFileFormat = NULL;
    }

    HX_RELEASE(m_pFileObject);
    HX_RELEASE(m_pRequest);
    HX_RELEASE(m_pFormatResponse);

    if (m_pHeaderSink)
    {
        for (UINT16 i = 0; i < m_ulNumLogicalStreams; i++)
        {
            m_pHeaderSink->StreamDone(i);
        }
        m_pHeaderSink->Release();
        m_pHeaderSink = NULL;
    }



//    HX_RELEASE(m_pProfile);
    HX_RELEASE(m_pFileHeader);

    if (m_pStreamHeaderMap)
    {
        IHXBuffer* pHdr = NULL;
        CHXMapLongToObj::Iterator i;
        for (i = m_pStreamHeaderMap->Begin(); i != m_pStreamHeaderMap->End(); ++i)
        {
            pHdr = (IHXBuffer*)(*i);
            HX_ASSERT(pHdr);
            HX_RELEASE(pHdr);
        }
        HX_DELETE(m_pStreamHeaderMap);
    }

    HX_RELEASE(m_pAsmSource);
    HX_VECTOR_DELETE(m_aulActualLogicalStream);

    HX_RELEASE(m_pPullPacketSink);
}

void
CHeaderHandler::ReportInitFileFormat(HX_RESULT status, const char* pc)
{
    
    HX_ASSERT(m_pHeaderSink);

    if (pc)
    {
            DPRINTF(zm_dprintf, ("%p: CStreamFilter: %u: %s\n", this, status, pc));
    }

    if (m_pHeaderSink)
    {
        m_pHeaderSink->InitDone(status);        
    }
}


/*
 * IHXFormatResponse
 */
STDMETHODIMP CHeaderHandler::InitDone(HX_RESULT status)
{
    if (HXR_OK == status)
    {
        return m_pFileFormat->GetFileHeader();
    }
    else
    {
        ReportInitFileFormat(status, "m_pFileFomrat->Init() failed\n");
    }
    return status;
}

//XXXGo - need it for later
#ifdef INPUTSOURCE_HEADER_DEBUG
void Dump(IHXValues* pHeader)
{
    IHXBuffer* pRuleBook = NULL;
    IHXBuffer* pSDPData = NULL;
    HX_RESULT res;

    IHXBuffer* pVal;
    UINT32 ulVal;
    const char* pName;

    res = pHeader->GetFirstPropertyULONG32(pName, ulVal);
    while(SUCCEEDED(res))
    {
        printf("%s: %u\n", pName, ulVal);
        res = pHeader->GetNextPropertyULONG32(pName, ulVal);
    }

    res = pHeader->GetFirstPropertyBuffer(pName, pVal);
    while(SUCCEEDED(res))
    {
        printf("%s: %s\n", pName, pVal->GetBuffer());
        pVal->Release();
        res = pHeader->GetNextPropertyBuffer(pName, pVal);
    }

    res = pHeader->GetFirstPropertyCString(pName, pVal);
    while(SUCCEEDED(res))
    {
        if (strcasecmp(pName, "ASMRuleBook") == 0)
        {
            pRuleBook = pVal;
        }
        else if (strcasecmp(pName, "SDPData") == 0)
        {
            pSDPData = pVal;
        }
        else
        {
            printf("%s: %s\n", pName, pVal->GetBuffer());
            pVal->Release();
        }
        res = pHeader->GetNextPropertyCString(pName, pVal);
    }
    
    if (pSDPData)
    {
        UINT32 ulLen = pSDPData->GetSize();
        char* szSDPData = new char[ulLen];
        strncpy(szSDPData, (const char*)pSDPData->GetBuffer(), ulLen);
        szSDPData[ulLen-1] = '\0';
        
        printf("\nSDPData:\n");
        char* szLine = strtok(szSDPData, "\r\n");
        while(szLine)
        {
            printf("%s\n", szLine);
            szLine = strtok(NULL, "\r\n");
        }
        delete[] szSDPData;
        pSDPData->Release();
    }

    if (pRuleBook)
    {
        BYTE* pc = new BYTE[pRuleBook->GetSize() + 1];
        memcpy(pc, pRuleBook->GetBuffer(), pRuleBook->GetSize());
        pc[pRuleBook->GetSize()] = '\0';
        BYTE* pcStart = pc;
        
        for (pc = (BYTE*)strchr((const char*)pc, '#'); pc; pc = (BYTE*)strchr((const char*)pc, '#'))
        {
	    *pc = '\n';	
        }

        printf("\nASMRuleBook:%s\n", pcStart);
        delete[] pcStart;
        pRuleBook->Release();
    }

    printf("\n");
    fflush(0);
}
#endif /* INPUTSOURCE_HEADER_DEBUG */

STDMETHODIMP CHeaderHandler::FileHeaderReady(HX_RESULT status, IHXValues* pHeader)
{
    HX_RESULT theErr = status;
    HX_ASSERT(!m_ulNumLogicalStreams);
    HX_ASSERT(!m_pFileHeader);

    if (HXR_OK == theErr)
    {
        m_pFileHeader = pHeader;
        m_pFileHeader->AddRef();

        theErr = m_pFileHeader->GetPropertyULONG32("StreamCount", m_ulNumLogicalStreams);
    }

//XXXGo - need it for later
#ifdef INPUTSOURCE_HEADER_DEBUG
    printf("\nFile Header:\n\n");
    Dump(m_pFileHeader);
#endif

    if (HXR_OK == theErr)
    {
        m_pStreamHeaderMap = new CHXMapLongToObj();
    }

    if (HXR_OK == theErr && m_ulNumLogicalStreams)
    {
            m_pInitialSubscription = new InitialSubscription();
            theErr = m_pInitialSubscription->Init(m_ulNumLogicalStreams);
            if (FAILED(theErr))
            {
                // we can still play this clip
                HX_DELETE(m_pInitialSubscription);
            }
        
        m_bStreamErrorReported = FALSE;

        // ask for stream headers
        theErr = m_pFileFormat->GetStreamHeader(0);
    }

    if ((HXR_OK != theErr) && (m_bStreamErrorReported == FALSE) ) //Report only if no error and already stream error not reported (- bool set to true in ::StreamHeaderReady)
    {
        ReportInitFileFormat(theErr, "StreamCount missing or GetStreamHeader failing");
    }

    return theErr;
}
STDMETHODIMP CHeaderHandler::StreamHeaderReady(HX_RESULT status, IHXValues* pHeader)
{
    HX_RESULT theErr = status;
    UINT32 ulStreamNo = MAX_UINT32;

    if (HXR_OK == theErr)
    {
        theErr = pHeader->GetPropertyULONG32("StreamNumber", ulStreamNo);
    }

#ifdef INPUTSOURCE_HEADER_DEBUG
    printf("\nStream %u:\n\n", ulStreamNo);
    Dump(pHeader);
#endif

    if (HXR_OK == theErr)
    {
        HX_ASSERT(!m_pStreamHeaderMap->Lookup(ulStreamNo));

        pHeader->AddRef();
        if (!m_pStreamHeaderMap->SetAt(ulStreamNo, pHeader))
        {
            theErr = HXR_FAIL;
        }
    }

    if (SUCCEEDED(theErr))
    {
        // Add necessary values
        theErr = UpdateStreamHeader(pHeader);
    }

    if (SUCCEEDED(theErr))
    {
        if (ulStreamNo + 1 < m_ulNumLogicalStreams)
        {
            // Get the next stream header
            theErr = m_pFileFormat->GetStreamHeader((UINT16)ulStreamNo + 1);
        }
        else
        {
	    // Determine whether to simulate stream group semantics, or not
	    IUnknown* pContext = (IUnknown*)m_pProc->pc->server_context;
	    IHXRegistry2* pRegistry = NULL;
	    if (pContext && SUCCEEDED(pContext->QueryInterface(IID_IHXRegistry2, (void**)&pRegistry)))
	    {
		INT32 lTemp = 0;
		pRegistry->GetIntByName("config.SimulateStreamGroups", lTemp);
		m_bSimulateStreamGroups = (BOOL)lTemp;
		pRegistry->Release();
	    }

	    if (m_bSimulateStreamGroups)
	    {
		SimulateStreamGroups();
	    }

            // we've got all stream headers
            HX_ASSERT((UINT32)m_pStreamHeaderMap->GetCount() ==
                      m_ulNumLogicalStreams);
            ReportInitFileFormat(HXR_OK, "Found all Stream Headers");
        }
    }

    if (FAILED(theErr) && (m_bStreamErrorReported == FALSE))
    {
        m_bStreamErrorReported = TRUE;
        ReportInitFileFormat(theErr,
            "StreamNumber missing, SetAt failed or GetStreamHeader failed");
    }

    return theErr;
}

STDMETHODIMP CHeaderHandler::PacketReady(HX_RESULT status, IHXPacket* pPacket)
{
    HX_ASSERT(!"do not call this");
    return HXR_NOTIMPL;
}
STDMETHODIMP CHeaderHandler::SeekDone(HX_RESULT status)
{
    if (m_pHeaderSink)
    {
        return m_pHeaderSink->SeekDone(status);
    }
    HX_ASSERT(!"no header sink");
    return HXR_OK;
}
STDMETHODIMP CHeaderHandler::StreamDone(UINT16 unStreamNumber)
{
    if (m_pHeaderSink)
    {
        return m_pHeaderSink->StreamDone(unStreamNumber);
    }
    HX_ASSERT(!"no header sink");
    return HXR_OK;
}

STDMETHODIMP
CHeaderHandler::Init(IHXPSinkControl* pSink)
{
    HX_ASSERT(!m_pHeaderSink);

    if (pSink)
    {
        pSink->AddRef();
        m_pHeaderSink = pSink;
        return HXR_OK;
    }
    else
    {
        HX_ASSERT(!"no header sink");
        return HXR_INVALID_PARAMETER;
    }
}

STDMETHODIMP
CHeaderHandler::Done()
{
    _Done();
    return HXR_OK;
}

STDMETHODIMP
CHeaderHandler::GetFileHeader(IHXPSinkControl* pSink)
{
    HX_ASSERT(pSink && pSink == m_pHeaderSink);

    if (m_pFileHeader)
    {
        m_pHeaderSink->FileHeaderReady(HXR_OK, m_pFileHeader);
        return HXR_OK;
    }

    m_pHeaderSink->FileHeaderReady(HXR_FAIL, NULL);
    return HXR_FAIL;

}
STDMETHODIMP
CHeaderHandler::GetStreamHeader(IHXPSinkControl* pSink, UINT16 unStreamNumber)
{
    HX_ASSERT(pSink && pSink == m_pHeaderSink);

    if (m_pStreamHeaderMap)
    {
        IHXValues* pHdr = NULL;
        m_pStreamHeaderMap->Lookup(unStreamNumber, (void*&)pHdr);
        if (pHdr)
        {
            pHdr->AddRef();
            m_pHeaderSink->StreamHeaderReady(HXR_OK, pHdr);
            pHdr->Release();
            return HXR_OK;
        }
    }

    m_pHeaderSink->StreamHeaderReady(HXR_FAIL, NULL);
    return HXR_FAIL;
}

STDMETHODIMP
CHeaderHandler::Seek(UINT32 ulSeekTime)
{
    DPRINTF(D_PLMINFO, ("CHeaderHandler::Seek(%u)\n", ulSeekTime));
    HX_RESULT theErr = HXR_OK;

    // some fileformats(asf) needs the subscription information to
    // perform a successful seek, so here we need to call InitialStartup
    // to pass the subscription information down to fileformat before calling
    // seek.

    theErr = InitialStartup();

    if(theErr == HXR_OK)
    {
        theErr= m_pFileFormat->Seek(ulSeekTime);
    }

    if (theErr == HXR_OK)
    {
        return HXR_OK;
    }
    else if (theErr == HXR_NOTIMPL)
    {
        SeekDone(HXR_OK);
    }
    else
    {
        SeekDone(theErr);
    }

    return HXR_OK;
}

STDMETHODIMP
CHeaderHandler::GetFileHeader(REF(IHXValues*)pHeader)
{
    if (m_pFileHeader)
    {
        pHeader = m_pFileHeader;
        pHeader->AddRef();
        return HXR_OK;
    }
    else
    {
	HX_ASSERT(FALSE);
        return HXR_FAIL;
    }
}
STDMETHODIMP
CHeaderHandler::GetStreamHeader(UINT32 ulStreamNo, REF(IHXValues*)pHeader)
{
    if (m_pStreamHeaderMap && m_pStreamHeaderMap->Lookup(ulStreamNo, (void*&)pHeader))
    {
        pHeader->AddRef();
        return HXR_OK;
    }

    HX_ASSERT(FALSE);
    return HXR_FAIL;
}

STDMETHODIMP
CHeaderHandler::GetFileFormatInfo(REF(const char**) pFileMimeTypes,
			    REF(const char**) pFileExtensions, REF(const char**) pFileOpenNames)
{
    // Validate params
    if (!m_pFileFormat)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    return m_pFileFormat->GetFileFormatInfo(pFileMimeTypes, pFileExtensions, pFileOpenNames);
}

STDMETHODIMP
CHeaderHandler::InitFileFormat(IHXRequest* pRequest, IHXFormatResponse* pFormatResponse, IHXFileObject* pFileObject)
{
    // Validate params
    if (!m_pFileFormat )
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    if (pFormatResponse)
    {
	HX_RELEASE(m_pFormatResponse);
	m_pFormatResponse = pFormatResponse;
	m_pFormatResponse->AddRef();
    }

    return m_pFileFormat->InitFileFormat(pRequest, this, pFileObject);
}

STDMETHODIMP
CHeaderHandler::GetFileHeader()
{
    HX_RESULT res = HXR_OK;

    // Validate params
    if (!m_pFormatResponse)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // If there is no file header, assume initialization error, and report
    // failure to the response interface
    if (!m_pFileHeader)
    {
	HX_ASSERT(FALSE);
	res = HXR_FAIL;
    }

    // Callback the response interface with the file header
    res = m_pFormatResponse->FileHeaderReady(res, m_pFileHeader);

    return res;
}

STDMETHODIMP
CHeaderHandler::GetStreamHeader(UINT16 unStreamNumber)
{
    HX_RESULT res = HXR_OK;

    // Validate params
    if (!m_pFormatResponse)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // If there is no stream header, assume initialization error, and report
    // failure to the response interface
    if (!m_pStreamHeaderMap)
    {
	HX_ASSERT(FALSE);
	res = HXR_FAIL;
    }

    // Get the stream header
    IHXValues* pHeader = NULL;
    if (SUCCEEDED(res))
    {
	m_pStreamHeaderMap->Lookup(unStreamNumber, (void*&)pHeader);

	if (!pHeader)
	{
	    HX_ASSERT(FALSE);
	    res = HXR_FAIL;
	}
    }

    // Callback the response interface with the stream header
    res = m_pFormatResponse->StreamHeaderReady(res, pHeader);

    return res;
}

STDMETHODIMP
CHeaderHandler::GetPacket(UINT16 unStreamNumber)
{
    // Validate params
    if (!m_pFileFormat)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    HX_RESULT hRet = HXR_OK;
    hRet = InitialStartup();

    if (SUCCEEDED(hRet))
    {
    	unStreamNumber = GetActualLogicalStreamNumber(unStreamNumber);
    	hRet = m_pFileFormat->GetPacket(unStreamNumber);
    }

    if (HXR_OK != hRet)
    {
        StreamDone(unStreamNumber);

        /*
            * XXXSCR
            * I am ifdefing this code because it needs to be revisited.
            * It was put here to disconnect players who have lost
            * vital pieces of the protocol exchange due high load
            * in cloaking, causing PLAY to occur with no stream
            * subscription.  Unfortuneately, we see GetPacket() failing
            * in other places.  This shouldn't happen.  We should
            * be able to treat GetPacket failure as a fatal error
            *
            * XXXJC :
            * Calling StreamDone should be enough, lets leave this out
            *
            */
#if 0
        if (client)
        {
            client->protocol()->sendAlert(SE_INVALID_PROTOCOL);
        }
#endif
    }
    return hRet;
}

STDMETHODIMP
CHeaderHandler::Close()
{
    HX_RESULT res = HXR_OK;

    HX_RELEASE(m_pFormatResponse);

    if (m_pFileFormat)
    {
	res = m_pFileFormat->Close();
    }

    return res;
}

CHeaderHandler::InitialSubscription::InitialSubscription()
    : m_unStreamCount(0)
    , m_ppSubscription(NULL)
    , m_unActualHiRuleNum(0)
{
}

CHeaderHandler::InitialSubscription::~InitialSubscription()
{
    for (UINT16 i = 0; i < m_unStreamCount; i++)
    {
	HX_VECTOR_DELETE(m_ppSubscription[i]);
    }
    HX_VECTOR_DELETE(m_ppSubscription);
}

HX_RESULT
CHeaderHandler::InitialSubscription::Init(UINT16 unStreamCount)
{
    HX_ASSERT(!m_unStreamCount && !m_ppSubscription);
    
    m_unStreamCount = unStreamCount;    
    if (!m_unStreamCount) 
    {
	return HXR_INVALID_PARAMETER;
    }

    HX_RESULT theErr = HXR_OK;
    m_ppSubscription = new BOOL*[m_unStreamCount];    
    if (m_ppSubscription)
    {
    	memset(m_ppSubscription, FALSE, sizeof(BOOL*)*m_unStreamCount);
    }
    else
    {
	theErr = HXR_OUTOFMEMORY;
    }

    for (UINT16 i = 0; i < m_unStreamCount && SUCCEEDED(theErr); i++)
    {
	m_ppSubscription[i] = new BOOL[MAX_RULE_COUNT];
	if (m_ppSubscription[i])
	{
	    memset(m_ppSubscription[i], FALSE, sizeof(BOOL)*MAX_RULE_COUNT);
	}
	else
	{
	    theErr = HXR_OUTOFMEMORY;
	}
    }

    return theErr;        
}


void 
CHeaderHandler::InitialSubscription::Update(BOOL bSubscribe, UINT16 unStreamNum, UINT16 unRuleNum)
{
    if (unStreamNum < m_unStreamCount && unRuleNum < MAX_RULE_COUNT)
    {
	if (unRuleNum > m_unActualHiRuleNum)
	{
	    m_unActualHiRuleNum = unRuleNum;
	}
	
	m_ppSubscription[unStreamNum][unRuleNum] = bSubscribe;
    }
}

HX_RESULT 
CHeaderHandler::InitialSubscription::Subscribe(IHXASMSource* pASMSource)
{
    if (!pASMSource)
    {
	return HXR_INVALID_PARAMETER;
    }

    HX_RESULT theErr = HXR_UNEXPECTED;    
    for (UINT16 i = 0; i < m_unStreamCount; i++)
    {
	for (UINT16 j = 0; j <= m_unActualHiRuleNum; j++)
	{
	    if (m_ppSubscription[i][j])
	    {
	    	theErr = pASMSource->Subscribe(i, j);

	    	if (FAILED(theErr))
	    	{
		    HX_ASSERT(SUCCEEDED(theErr));
		    break;
	    	}
	    }	    
	}
    }
    return theErr;
}

HX_RESULT
CHeaderHandler::InitialStartup()
{
    HX_RESULT theErr = HXR_OK;
    if (m_pInitialSubscription)
    {
	theErr = m_pInitialSubscription->Subscribe(m_pAsmSource);
	HX_DELETE(m_pInitialSubscription);
    }
    return HXR_OK;
}

STDMETHODIMP
CHeaderHandler::Subscribe(UINT16 uStreamNumber, UINT16 uRuleNumber)
{
    // Validate params
    if (!m_pAsmSource)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    uStreamNumber = GetActualLogicalStreamNumber(uStreamNumber);

    if (m_pInitialSubscription)
    {
	// remember the sub/unsub state
	m_pInitialSubscription->Update(TRUE, uStreamNumber, uRuleNumber);
	return HXR_OK;
    }   
    else
    {
    	return m_pAsmSource->Subscribe(uStreamNumber, uRuleNumber);
    }    	
}

STDMETHODIMP
CHeaderHandler::Unsubscribe(UINT16 uStreamNumber, UINT16 uRuleNumber)
{
    // Validate params
    if (!m_pAsmSource)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }
    
    uStreamNumber = GetActualLogicalStreamNumber(uStreamNumber);

    if (m_pInitialSubscription)
    {
	// remember the sub/unsub state
	m_pInitialSubscription->Update(FALSE, uStreamNumber, uRuleNumber);
	return HXR_OK;
    }   
    else
    {
    	return m_pAsmSource->Unsubscribe(uStreamNumber, uRuleNumber);
    }    	
}

STDMETHODIMP
CHeaderHandler::Init(IHXPSinkPackets* pSink)
{
    HX_ASSERT(!m_pPullPacketSink && pSink);
    if (pSink)
    {
	m_pPullPacketSink = pSink;
	m_pPullPacketSink->AddRef();
	return HXR_OK;
    }
    return HXR_INVALID_PARAMETER;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CHeaderHandler::SimulateStreamGroups
// Purpose:
//  Creates fake stream groups from existing logical streams -- for debugging purposes only
HX_RESULT
CHeaderHandler::SimulateStreamGroups()
{
    HX_RESULT res = HXR_OK;

    // Validate params
    if (!m_pFileHeader || m_ulNumLogicalStreams == 0 || !m_pStreamHeaderMap)
    {
	HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    // Quick sanity check -- no FF should support StreamGroup semantics yet
    if (SUCCEEDED(res))
    {
	HX_RESULT resProp = m_pFileHeader->GetPropertyULONG32("StreamGroupCount", m_ulNumStreamGroups) ;
	if (SUCCEEDED(resProp))
	{
	    HX_ASSERT(FALSE);
	    return HXR_FAIL;
	}
    }

    // Detect the number of physical streams in each logical stream.  If the original logical stream contains
    // more than one physical stream, create additional logical streams to represent them
    UINT32 ulNumTracks = 0;
    UINT32 ulOrigNumLogicalStreams = m_ulNumLogicalStreams;
    for (UINT32 i=0; i<ulOrigNumLogicalStreams && SUCCEEDED(res); i++)
    {
	IHXValues* pOrigHeader = NULL;
	m_pStreamHeaderMap->Lookup(i, (void*&)pOrigHeader);

	// Determine the number of physical streams
	if (pOrigHeader)
	{
	    // Set the stream group number
	    if (SUCCEEDED(res))
		res = pOrigHeader->SetPropertyULONG32("StreamGroupNumber", m_ulNumStreamGroups);

	    // Set the switch group ID -- in this contrived case, all stream group members are in same switch group
	    if (SUCCEEDED(res))
		res = pOrigHeader->SetPropertyULONG32("SwitchGroupID", m_ulNumStreamGroups);

	    UINT32 ulNumPhysicalStreams = 0;
	    UINT32* aulAvgBandwidth = 0;

	    // Note that stream group number and switch group ID must be set before calling GetPhysicalStreams
	    if (SUCCEEDED(res))
		res = GetPhysicalStreams(i, pOrigHeader, ulNumPhysicalStreams, aulAvgBandwidth);

	    // Fix up props on the original logical stream
	    HX_ASSERT(ulNumPhysicalStreams > 0);
	    if (SUCCEEDED(res) && ulNumPhysicalStreams > 0)
	    {
		// Set the track number -- 1-based number
		if (SUCCEEDED(res))
		{
		    res = pOrigHeader->SetPropertyULONG32("TrackID", ++ulNumTracks);
		}

		// Arbitrary choice -- original logical streams are the default streams
		if (SUCCEEDED(res))
		    res = pOrigHeader->SetPropertyULONG32("DefaultStream", 1);

		// Set the avg bitrate
		if (SUCCEEDED(res))
		    res = pOrigHeader->SetPropertyULONG32("AvgBitRate", aulAvgBandwidth[ulNumPhysicalStreams-1]);
	    }


	    // If there is more than one physical stream, create additional logical streams
	    if (SUCCEEDED(res) && ulNumPhysicalStreams > 1)
	    {
		for (UINT32 j=0; j<ulNumPhysicalStreams-1 && SUCCEEDED(res); j++)
		{
		    IHXValues* pNewHeader = NULL;
		    res = CloneHeader(pOrigHeader, &pNewHeader);

		    // Set the track number -- 1-based number
		    if (SUCCEEDED(res))
			res = pNewHeader->SetPropertyULONG32("TrackID", ++ulNumTracks);

		    // Set the stream group number
		    if (SUCCEEDED(res))
			res = pNewHeader->SetPropertyULONG32("StreamGroupNumber", m_ulNumStreamGroups);

		    // Set the switch group ID -- in this contrived case, all stream group members are in same switch group
		    if (SUCCEEDED(res))
			res = pNewHeader->SetPropertyULONG32("SwitchGroupID", m_ulNumStreamGroups);

		    // Set the avg bitrate
		    if (SUCCEEDED(res))
			res = pNewHeader->SetPropertyULONG32("AvgBitRate", aulAvgBandwidth[j]);

		    // Save the stream header
		    if (SUCCEEDED(res))
		    {
			//pNewHeader->AddRef();
			m_pStreamHeaderMap->SetAt(m_ulNumLogicalStreams++, pNewHeader);
		    }
		}
	    }

	    HX_DELETE(aulAvgBandwidth);

	    HX_ASSERT(SUCCEEDED(res));
	    m_ulNumStreamGroups++;
	}
    }

    // Sanity check
    if (SUCCEEDED(res))
    {
	HX_ASSERT(ulOrigNumLogicalStreams == m_ulNumStreamGroups);
	HX_ASSERT(m_ulNumLogicalStreams == ulNumTracks);

    }

    // Set StreamGroupCount -- equal to the original number of logical streams
    if (SUCCEEDED(res))
	res = m_pFileHeader->SetPropertyULONG32("StreamGroupCount", m_ulNumStreamGroups);


    // Set new StreamCount -- equal to the number of physical streams
    if (SUCCEEDED(res))
	res = m_pFileHeader->SetPropertyULONG32("StreamCount", m_ulNumLogicalStreams);

    // Create lookup array containing actual logical stream numbers
    // Also set track/logical stream numbers
    if (SUCCEEDED(res))
    {
	m_aulActualLogicalStream = new UINT32[m_ulNumLogicalStreams];

	for (UINT32 i=0; i<m_ulNumLogicalStreams; i++)
	{
	    IHXValues* pHeader = NULL;
	    m_pStreamHeaderMap->Lookup(i, (void*&)pHeader);

	    if (pHeader)
	    {
		res = pHeader->GetPropertyULONG32("StreamGroupNumber", m_aulActualLogicalStream[i]);

		// Set the logical stream number
		if (SUCCEEDED(res))
		    res = pHeader->SetPropertyULONG32("StreamNumber", i);
	    }
	    else
	    {
		res = HXR_FAIL;
	    }
	}
    }

    HX_ASSERT(SUCCEEDED(res));
    return res;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CHeaderHandler::GetActualLogicalStreamNumber
// Purpose:
//  Gets the actual logical stream number when creating fake stream groups.  Will
//  just return the original stream number if not creating fake stream groups.
UINT16 CHeaderHandler::GetActualLogicalStreamNumber(UINT16 unLogicalStreamNum)
{
    HX_RESULT res = HXR_OK;

    if (m_aulActualLogicalStream && unLogicalStreamNum < m_ulNumLogicalStreams)
    {
	HX_ASSERT(m_bSimulateStreamGroups);

	unLogicalStreamNum = (UINT16)m_aulActualLogicalStream[unLogicalStreamNum];
    }

    return unLogicalStreamNum;
}

/////////////////////////////////////////////////////////////////////////
// Method:
//  CHeaderHandler::GetPhysicalStreams
// Purpose:
//  Determines the number of physical streams that a logical stream
//  contains.  Also gets the avg bandwidth of each physical stream.
// Notes:
//  Caller is responsible for cleaning up aulAvgBandwidth.
HX_RESULT CHeaderHandler::GetPhysicalStreams(UINT32 ulLogicalStreamNum, IHXValues* pHeader, REF(UINT32)ulNumPhysicalStreams, REF(UINT32*)aulAvgBandwidth)
{
    HX_RESULT res = HXR_OK;

    ulNumPhysicalStreams = 0;

    if (SUCCEEDED(res))
    {
	// Create rule handler
	CStreamGroupManager* pStreamGroupMgr = new CStreamGroupManager(m_pProc->pc->common_class_factory);
	if (!pStreamGroupMgr)
	{
	    res = HXR_OUTOFMEMORY;
	}
	else
	{
	    pStreamGroupMgr->AddRef();
	}

	if (SUCCEEDED(res))
	    res = pStreamGroupMgr->Init(ulLogicalStreamNum, 1, FALSE);

	if (SUCCEEDED(res))
	    res = pStreamGroupMgr->SetStreamHeader(ulLogicalStreamNum, pHeader);

	// Get the number of physical streams / avg bitrate for each stream
	if (SUCCEEDED(res))
	{
	    ulNumPhysicalStreams = pStreamGroupMgr->GetNumRateDescriptions();

	    if (ulNumPhysicalStreams > 0)
	    {
		aulAvgBandwidth = new UINT32[ulNumPhysicalStreams];
	    }

	    for (UINT32 i=0; i<ulNumPhysicalStreams && SUCCEEDED(res); i++)
	    {
		IHXRateDescription* pRate = NULL;
		res = pStreamGroupMgr->GetRateDescription(i, pRate);

		if (SUCCEEDED(res))
		{
		    res = pRate->GetAvgRate(aulAvgBandwidth[i]);
		}

		HX_RELEASE(pRate);
	    }
	}

	HX_RELEASE(pStreamGroupMgr);
    }

    HX_ASSERT(SUCCEEDED(res));

    return res;
}


/////////////////////////////////////////////////////////////////////////
// Method:
//  CHeaderHandler::CloneHeader
// Purpose:
//  Internal helper to Clone file or stream headers
HX_RESULT CHeaderHandler::CloneHeader(IHXValues* pSourceHeader, IHXValues** ppClonedHeader)
{
    // Validate params
    if (!pSourceHeader || !ppClonedHeader)
    {
	HX_ASSERT(FALSE);
	return HXR_POINTER;
    }

    ULONG32 propValue;
    const char* pPropName = 0;
    IHXBuffer* pPropBuffer = 0;
    IHXValues* pNewHeader = NULL;
    HX_RESULT rc = HXR_OK;
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    if (pSourceHeader)
    {
	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	// Use CHXUniquelyKeyedList instead of CHXHeader to get IHXValuesRemove support
	pNewHeader = (IHXValues*) new CHXUniquelyKeyedList;

	if (pNewHeader)
	    pNewHeader->AddRef();
	else
	    retVal = HXR_OUTOFMEMORY;
    }

    // Copy ULONG32s
    if (SUCCEEDED(retVal))
    {
	rc = pSourceHeader->GetFirstPropertyULONG32(pPropName,propValue);
    }

    while(SUCCEEDED(retVal) && (rc == HXR_OK))
    {
	retVal = pNewHeader->SetPropertyULONG32(pPropName, propValue);
	rc = pSourceHeader->GetNextPropertyULONG32(pPropName, propValue);
    }

    // Copy Buffers -- shallow copy
    if (SUCCEEDED(retVal))
    {
	rc = pSourceHeader->GetFirstPropertyBuffer(pPropName, pPropBuffer);
    }

    while(SUCCEEDED(retVal) && (rc == HXR_OK))
    {
	retVal = pNewHeader->SetPropertyBuffer(pPropName, pPropBuffer);
	HX_RELEASE(pPropBuffer);
	rc = pSourceHeader->GetNextPropertyBuffer(pPropName, pPropBuffer);
    }

    HX_RELEASE(pPropBuffer);

    // Copy CStrings -- shallow copy
    if (SUCCEEDED(retVal))
    {
	rc = pSourceHeader->GetFirstPropertyCString(pPropName, pPropBuffer);
    }

    while(SUCCEEDED(retVal) && (rc == HXR_OK))
    {
	retVal = pNewHeader->SetPropertyCString(pPropName, pPropBuffer);
	HX_RELEASE(pPropBuffer);
	rc = pSourceHeader->GetNextPropertyCString(pPropName, pPropBuffer);
    }

    HX_RELEASE(pPropBuffer);

    if (FAILED(retVal))
    {
	HX_RELEASE(pNewHeader);
    }

    else
    {
	*ppClonedHeader = pNewHeader;
    }

    return retVal;
}

HX_RESULT
CHeaderHandler::UpdateStreamHeader(IHXValues* pHeader)
{
    // Add ServerPreroll, the preroll value we will use
    // internally in the server core.

    // Preroll is, in order of preference
    // 1. ActualPreroll
    // 2. X-InitPreDecBufPeriod + X-InitPostDecBufPeriod
    // 3. Preroll
    // 4. Default (1 sec)
    UINT32 ulPreroll = DEFAULT_PREROLL;
    UINT32 ulTemp;
    if (SUCCEEDED(pHeader->GetPropertyULONG32("ActualPreroll", ulTemp)))
    {
        ulPreroll = ulTemp;
    }
    else if (SUCCEEDED(pHeader->GetPropertyULONG32("X-InitPreDecBufPeriod",
        ulTemp)))
    {
        ulPreroll = ulTemp;
        if (SUCCEEDED(pHeader->GetPropertyULONG32("X-InitPostDecBufPeriod",
            ulTemp)))
        {
            ulPreroll += ulTemp;
        }
        ulPreroll /= 90;
    }
    else if (SUCCEEDED(pHeader->GetPropertyULONG32("Preroll", ulTemp)))
    {
        ulPreroll = ulTemp;
    }

    return pHeader->SetPropertyULONG32("ServerPreroll", ulPreroll);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// CPacketHandler
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
CPacketHandler::CPacketHandler(Process* pProc,
                               IHXFileFormatObject* pFF,
                               IUnknown* pFileObject,
                               IHXRequest* pRequest)
    : CHeaderHandler(pProc, pFF, pFileObject, pRequest)
    , m_ppPacketSinks(NULL)
    , m_pPacketSink(NULL)
    , m_bSuspended(FALSE)

{
}

CPacketHandler::~CPacketHandler()
{
    Done();
}

STDMETHODIMP
CPacketHandler::Init(IHXPSinkControl* pSink)
{
    HX_ASSERT(!m_ppPacketSinks);

    HX_RESULT theErr = CHeaderHandler::Init(pSink);

    IHXFileObject* pFO = NULL;

    if (HXR_OK == theErr)
    {
        /*
         * We need to set the outer most class in order to get
         * IHXFormatResponse methods to be the IHXFormatResponse in
         * FileFormat
         */
        theErr = m_pFileObject->QueryInterface(IID_IHXFileObject, (void**)&pFO);
    }

    if (HXR_OK == theErr)
    {
        theErr = m_pFileFormat->InitFileFormat(m_pRequest, this, pFO);
        pFO->Release();
    }
    // continue on CHeaderHandler::InitDone()
    return theErr;
}

STDMETHODIMP
CPacketHandler::Seek(UINT32 ulSeekTime)
{
    // to be cleard in StartPackets()
    m_bSuspended = TRUE;

    if (m_ppPacketSinks)
    {
            for (UINT32 i = 0; i < m_ulNumLogicalStreams; i++)
            {
            m_ppPacketSinks[i]->Reset();
            }
    }

    return CHeaderHandler::Seek(ulSeekTime);
}

STDMETHODIMP
CPacketHandler::Done()
{
    if (m_ppPacketSinks)
    {
            for (UINT32 i = 0; i < m_ulNumLogicalStreams; i++)
            {
                HX_ASSERT(!m_pPacketSink);
                if (m_ppPacketSinks[i])
                {
                    m_ppPacketSinks[i]->Close();
                    m_ppPacketSinks[i]->Release();
                    m_ppPacketSinks[i] = NULL;
                }
            }
        HX_VECTOR_DELETE(m_ppPacketSinks);
    }
    else
    {
        if (m_pPacketSink)
        {
            m_pPacketSink->Flush();
            m_pPacketSink->SourceDone();
        }
    }

    HX_RELEASE(m_pPacketSink);
    return CHeaderHandler::Done();
}

HX_RESULT
CPacketHandler::CreateSinglePushStreams()
{
    HX_ASSERT(m_pPacketSink);
    HX_ASSERT(m_ulNumLogicalStreams);

    if (!m_pPacketSink || !m_ulNumLogicalStreams)
    {
        // no can't do.
        return HXR_FAIL;
    }

    /*
     * all streams share the same sink
     */
    HX_RESULT theErr = HXR_OK;

    m_ppPacketSinks = new CSingleStreamPush*[m_ulNumLogicalStreams];
    if (m_ppPacketSinks)
    {
        memset(m_ppPacketSinks, 0, sizeof(CSingleStreamPush*)*m_ulNumLogicalStreams);
    }
    else
    {
        theErr = HXR_OUTOFMEMORY;
    }

    for (UINT32 i = 0; HXR_OK == theErr && i < m_ulNumLogicalStreams; i++)
    {
        m_ppPacketSinks[i] = new CSingleStreamPush(this, i);
        if (m_ppPacketSinks[i])
        {
            m_ppPacketSinks[i]->AddRef();
            theErr = m_ppPacketSinks[i]->Init(m_pPacketSink);
        }
        else
        {
            theErr = HXR_OUTOFMEMORY;
        }
    }

    HX_RELEASE(m_pPacketSink);
    return theErr;
}


/* IHXServerPacketSource */
STDMETHODIMP
CPacketHandler::SetSink(IHXServerPacketSink* pSink)
{
    HX_ASSERT(pSink && !m_ppPacketSinks);

    if (!pSink)
    {
        return HXR_INVALID_PARAMETER;
    }
    else if (m_ulNumLogicalStreams)
    {
        m_pPacketSink = pSink;
        m_pPacketSink->AddRef();
        return HXR_OK;
    }
    else if (!m_pFileHeader)
    {
        m_pPacketSink = pSink;
        m_pPacketSink->AddRef();
        return HXR_OK;
    }
    else
    {
        // have fileheader yet no stream count.
        return HXR_UNEXPECTED;
    }
}


HX_RESULT
CPacketHandler::Start(UINT32 ulStreamNo)
{
    // start GetPacket()/PacketReady() sequence
    return Fetch(ulStreamNo);
}

HX_RESULT
CPacketHandler::Fetch(UINT32 ulStreamNo)
{
    if (!m_bSuspended)
    {
        return m_pFileFormat->GetPacket((UINT16)ulStreamNo);
    }
    else
    {
        return HXR_OK;
    }
}

STDMETHODIMP
CPacketHandler::StartPackets()
{   
    HX_RESULT theErr = HXR_OK;
    theErr = InitialStartup();
    
    m_bSuspended = FALSE;

    if (m_ppPacketSinks)
    {
        for (UINT32 i = 0; HXR_OK == theErr && i < m_ulNumLogicalStreams; i++)
        {
            theErr = m_ppPacketSinks[i]->Restart();
    	}	
    }
    else
    {
        theErr = CreateSinglePushStreams();
    
    	for (UINT32 i = 0; HXR_OK == theErr && i < m_ulNumLogicalStreams; i++)
    	{
            theErr = m_ppPacketSinks[i]->Start();
    	}
    }

    return theErr;
}

STDMETHODIMP
CPacketHandler::GetPacket()
{
    HX_ASSERT(!"don't call this");
    return HXR_NOTIMPL;
}

STDMETHODIMP
CPacketHandler::SinkBlockCleared(UINT32 ulStream)
{
    // XXXGo - Flow shouldn't be calling this after it Done() the source!
    if (!m_bDone)
    {
        m_ppPacketSinks[ulStream]->Start();
    }
    return HXR_OK;
}

STDMETHODIMP
CPacketHandler::EnableTCPMode()
{
    return HXR_OK;
}

STDMETHODIMP
CPacketHandler::PacketReady(HX_RESULT status, IHXPacket* pPacket)
{
    if (m_pPullPacketSink)
    {
	if (HXR_OK != status)
	{
            for (UINT16 i = 0; i < m_ulNumLogicalStreams; i++)
            {
                DoneStream(i);
            }

            return status;
        }
        else
        {
	    return m_pPullPacketSink->PacketReady(status, pPacket);
	}
    }

    HX_ASSERT(!m_bSuspended);

    UINT32 ulStreamNo = 0;
    if (HXR_OK == status)
    {
        ulStreamNo = pPacket->GetStreamNumber();
        status = m_ppPacketSinks[ulStreamNo]->PacketReady(pPacket);
    }

    //short-circuit here. "this" might be delted at this point for mdpshim switch.
    if(status == HXR_CLOSED)
    {
        //we can't return HXR_CLOSED, because it will cause ff to do extra work, but ff 
        //might be deleted as well.
        return HXR_OK;
    }

    if (HXR_OK == status)
    {
        Fetch(ulStreamNo);
    }
    else if (HXR_BLOCKED == status)
    {
        HX_ASSERT(!"shouldn't be happening...");
    }

    return HXR_OK;
}
STDMETHODIMP
CPacketHandler::SeekDone(HX_RESULT status)
{
    return CHeaderHandler::SeekDone(status);
}

STDMETHODIMP
CPacketHandler::StreamDone(UINT16 unStreamNumber)
{
    if (m_ppPacketSinks && m_ppPacketSinks[unStreamNumber])
    {
        m_ppPacketSinks[unStreamNumber]->Done();
        // continue in DoneStream()
    }
    else
    {
    	CHeaderHandler::StreamDone(unStreamNumber);
    }
    return HXR_OK;
}

void
CPacketHandler::DoneStream(UINT32 ulStreamNo)
{
    CHeaderHandler::StreamDone((UINT16)ulStreamNo);
}

STDMETHODIMP
CPacketHandler::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXPSourceControl*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPSourceControl))
    {
        AddRef();
        *ppvObj = (IHXPSourceControl*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXServerPacketSource))
    {
        AddRef();
        *ppvObj = (IHXServerPacketSource*)this;
        return HXR_OK;
    }

    else if(IsEqualIID(riid, IID_IHXPSourcePackets))
    {
        AddRef();
        *ppvObj = (IHXPSourcePackets*)this;
        return HXR_OK;
    }

    else if(IsEqualIID(riid, IID_IHXFormatResponse))
    {
        AddRef();
        *ppvObj = (IHXFormatResponse*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXSyncHeaderSource))
    {
        AddRef();
        *ppvObj = (IHXSyncHeaderSource*)this;
        return HXR_OK;
    }
    else if ( (IsEqualIID(riid, IID_IHXFileFormatObject)))
    {
        AddRef();
        *ppvObj = (IHXFileFormatObject*)this;
        return HXR_OK;
    }
    else if ( (IsEqualIID(riid, IID_IHXPacketTimeOffsetHandler)) && m_pFileFormat)
    {
        return m_pFileFormat->QueryInterface(riid, ppvObj);
    }
    else if ( (IsEqualIID(riid, IID_IHXLiveFileFormatInfo)) && m_pFileFormat)
    {
        return m_pFileFormat->QueryInterface(riid, ppvObj);
    }
    // Note: Only expose IHXASMSource if the underlying FF supports it (some datatypes do not)
    else if ( (IsEqualIID(riid, IID_IHXASMSource)) && m_pAsmSource)
    {
        AddRef();
        *ppvObj = (IHXASMSource*)this;
        return HXR_OK;
    }
    // XXXJJ This makes the above two(IID_IHXPacketTimeOffsetHandler && IID_IHXLiveFileFormatInfo)
    // redundant.  But we want to leave those two there to make it clear that we query them from
    // file format.
    else if ( m_pFileFormat)
    {
        return m_pFileFormat->QueryInterface(riid, ppvObj);
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CPacketHandler::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CPacketHandler::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// CSingleStreamPush
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

CPacketHandler::CSingleStreamPush::CSingleStreamPush(CPacketHandler* pParent, UINT32 ulStreamNo)
    : m_pParent(pParent) // private class, don't AddRef()
    , m_ulRefCount(0)
    , m_ulStreamNo(ulStreamNo)
    , m_state(STREAM_CLOSED)
    , m_pPacket(NULL)
    , m_pPacketSink(NULL)
    , m_pTSConverter(NULL)
    , m_pPktConverter(NULL)
{
}

CPacketHandler::CSingleStreamPush::~CSingleStreamPush()
{
    HX_ASSERT(!m_pPacket);
    Close();
}

STDMETHODIMP_(ULONG32)
CPacketHandler::CSingleStreamPush::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CPacketHandler::CSingleStreamPush::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
CPacketHandler::CSingleStreamPush::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXPSourceControl*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

void
CPacketHandler::CSingleStreamPush::Close()
{
    // close immediately
    m_state = STREAM_CLOSED;
    HX_RELEASE(m_pPacket);

    if (m_pPacketSink)
    {
        m_pPacketSink->Flush();
        m_pPacketSink->SourceDone();
        m_pPacketSink->Release();
        m_pPacketSink = NULL;
    }

    HX_DELETE(m_pTSConverter);
    m_pPktConverter = NULL;
}

void
CPacketHandler::CSingleStreamPush::Done()
{
    if (STREAM_CLOSED == m_state)
    {
        return;
    }
    else if (STREAM_BLOCKED == m_state && m_pPacket)
    {
        m_state = STREAM_FLUSH;
        // once m_pPacket has been sent, continue in DoneStream()
    }
    else
    {
        DoneStream();
    }
}

void
CPacketHandler::CSingleStreamPush::DoneStream()
{
    HX_ASSERT(STREAM_CLOSED != m_state);

    m_state = STREAM_READY;
    m_pPacketSink->Flush();
    m_pParent->DoneStream(m_ulStreamNo);
}

HX_RESULT
CPacketHandler::CSingleStreamPush::Init(IHXServerPacketSink* pSink)
{
    HX_ASSERT(STREAM_CLOSED == m_state);
    HX_ASSERT(pSink && !m_pPacketSink);
    HX_ASSERT(!m_pTSConverter);

    if (pSink)
    {
        m_state = STREAM_READY;
        m_pPacketSink = pSink;
        m_pPacketSink->AddRef();
        return HXR_OK;
    }
    else
    {
        return HXR_FAIL;
    }
}

HX_RESULT
CPacketHandler::CSingleStreamPush::Start()
{
    HX_ASSERT(m_pParent && m_pPacketSink);
   HX_ASSERT(STREAM_CLOSED != m_state);
    HX_ASSERT(STREAM_READY == m_state ||
              STREAM_BLOCKED == m_state ||
             STREAM_FLUSH == m_state ||
                    STREAM_STARTED == m_state);

    HX_RESULT theErr = HXR_OK;

    switch (m_state)
    {
    case STREAM_BLOCKED:
        theErr = HandleStreamBlocked();
        break;
    case STREAM_FLUSH:
        theErr = HandleStreamFlush();
        break;
    case STREAM_READY:
        theErr = HandleStreamReady();
        break;
    case STREAM_STARTED:
        theErr = HandleStreamReady();
        break;
    default:
        return HXR_UNEXPECTED;
    }

    return theErr;
}

/*
 * Start after Pause
 */
HX_RESULT
CPacketHandler::CSingleStreamPush::Restart()
{
    HX_ASSERT((m_pPacket && STREAM_BLOCKED == m_state) ||
              (!m_pPacket == STREAM_STARTED));

    Reset();

    switch (m_state)
    {
    case STREAM_BLOCKED:
    case STREAM_FLUSH:
        // don't do anything, wait for SinkBlockCleared() that calls Start()
        return HXR_OK;
        break;
    case STREAM_READY:
    case STREAM_STARTED:
        return Start();
        break;
    }

    return HXR_UNEXPECTED;
}


HX_RESULT
CPacketHandler::CSingleStreamPush::HandleStreamReady()
{
    m_state = STREAM_STARTED;
    return m_pParent->Start(m_ulStreamNo);
}

HX_RESULT
CPacketHandler::CSingleStreamPush::HandleStreamBlocked()
{
    m_state = STREAM_STARTED;

    HX_RESULT theErr = HXR_OK;
    if (m_pPacket)
    {
        theErr = FlushPacket();
    }

    if (STREAM_STARTED == m_state)
    {
        m_pParent->Start(m_ulStreamNo);
    }

    return theErr;
}

HX_RESULT
CPacketHandler::CSingleStreamPush::HandleStreamFlush()
{
    HX_ASSERT(m_pPacket);

    m_state = STREAM_STARTED;

    HX_RESULT theErr = FlushPacket();

    if (STREAM_BLOCKED == m_state)
    {
        // try again
        m_state = STREAM_FLUSH;
    }
    else
    {
        DoneStream();
    }
    return theErr;
}

void
CPacketHandler::CSingleStreamPush::Reset()
{
    // ignore old pkt
    HX_RELEASE(m_pPacket);

    switch (m_state)
    {
    case STREAM_FLUSH:
        // don't want to end
        m_state = STREAM_BLOCKED;
        break;
    case STREAM_BLOCKED:
    case STREAM_READY:
    case STREAM_STARTED:
        // unchanged
        break;
    default:
        HX_ASSERT(!"unknown state");
    }
}

HX_RESULT
CPacketHandler::CSingleStreamPush::FlushPacket()
{
    HX_ASSERT(m_pPacket);

    // take the AddRef()
    ServerPacket* pPkt = m_pPacket;
    m_pPacket = NULL;
    HX_RESULT theErr = TransmitPacket(pPkt);
    pPkt->Release();
    return theErr;
}

HX_RESULT
CPacketHandler::CSingleStreamPush::PacketReady(IHXPacket* pPacket)
{
    HX_ASSERT(pPacket);

    if (STREAM_STARTED == m_state)
    {
        if (!m_pPktConverter)
        {
            HX_RESULT theErr = FindPktConverter(pPacket);
            if (HXR_OK != theErr)
            {
                return theErr;
            }
        }

        HX_ASSERT(m_pPktConverter);
        return TransmitPacket((this->*m_pPktConverter)(pPacket));
    }
    else
    {
        HX_ASSERT(!"StreamPush in wrong state\n");
        return HXR_UNEXPECTED;
    }
}

/*
 */
HX_RESULT
CPacketHandler::CSingleStreamPush::TransmitPacket(ServerPacket* pPacket)
{
    HX_ASSERT(STREAM_STARTED == m_state);
    HX_ASSERT(pPacket);
    HX_ASSERT(!m_pPacket);
    HX_ASSERT(!m_pParent->m_bSuspended);


    HX_RESULT theErr = m_pPacketSink->PacketReady(pPacket);
    if (HXR_OK == theErr || HXR_CLOSED == theErr)
    {
    }
    else if (HXR_BLOCKED == theErr)
    {
        m_state = STREAM_BLOCKED;

        // take this packet, and tell source not to send anymore until cleared
        theErr = HXR_WOULD_BLOCK;
    }
    else if (HXR_WOULD_BLOCK == theErr)
    {
        m_state = STREAM_BLOCKED;
        // pPacket has been taken care of
    }
    else
    {
        /*
         * There was an assert here.  Changing the behavior of the auth API
         * to keep the control socket open makes it possible to have pending
         * packets in the transmit queue when StreamDone() is called.  When
         * this happens, the above call to PacketReady() returns HXR_FAIL.
         * The assert has been removed but someone should take a better look
         * at this case to make sure it's doing the Right Thing.  -- TDM
         */
        DoneStream();
    }
    return theErr;
}

ServerPacket*
CPacketHandler::CSingleStreamPush::PacketConvert(IHXPacket* pPkt)
{
    HX_ASSERT(!m_pTSConverter);
    HX_ASSERT(pPkt->QueryInterface(IID_ServerPacket,(void **)0xffffd00d)==HXR_OK);

    ((ServerPacket*)pPkt)->SetMediaTimeInMs(pPkt->GetTime());
    return (ServerPacket*)pPkt;
}

ServerPacket*
CPacketHandler::CSingleStreamPush::PacketTSConvert(IHXPacket* pPkt)
{
    HX_ASSERT(m_pTSConverter);
#ifdef _DEBUG
    IHXRTPPacket* pRTPPkt = NULL;
    HX_ASSERT(pPkt->QueryInterface(IID_IHXRTPPacket,(void **)&pRTPPkt) == HXR_OK);
    HX_RELEASE(pRTPPkt);
#endif

    ServerRTPPacket* pServerRTPPacket = (ServerRTPPacket*)pPkt;
    pServerRTPPacket->SetMediaTimeInMs(m_pTSConverter->rtp2hxa_raw(pServerRTPPacket->GetRTPTime()));
    return (ServerPacket*)pServerRTPPacket;
}

HX_RESULT
CPacketHandler::CSingleStreamPush::FindPktConverter(IHXPacket* pPacket)
{
    HX_ASSERT(!m_pPktConverter);
    HX_RESULT theErr = HXR_OK;

    IHXRTPPacket* pRTPPkt = NULL;

    if (pPacket->QueryInterface(IID_ServerPacket,
                                        (void **)0xffffd00d) != HXR_OK)
    {
        // we only know how to handle ServerPacket/ServerRTPPacket
        return HXR_UNEXPECTED;
    }

    if (pPacket->QueryInterface(IID_IHXRTPPacket, (void**)&pRTPPkt) == HXR_OK)
    {
        UINT32 ulSamplesPerSecond = 0;
        IHXValues* pHdr = NULL;

        theErr = HXR_OK;

            if (m_pParent->GetStreamHeader(m_ulStreamNo, pHdr) == HXR_OK)
            {
            if (pHdr->GetPropertyULONG32("SamplesPerSecond", ulSamplesPerSecond) == HXR_OK &&
                ulSamplesPerSecond)
                {
                    m_pTSConverter = new CHXTimestampConverter(CHXTimestampConverter::SAMPLES, ulSamplesPerSecond);

                if (m_pTSConverter)
                {
                    // only case for PacketTSConvert
                    m_pPktConverter = &CSingleStreamPush::PacketTSConvert;
                }
                else
                {
                    theErr = HXR_OUTOFMEMORY;
                }
                }
                HX_RELEASE(pHdr);
            }
            else
            {
                HX_ASSERT(!"no header...\n");
                theErr = HXR_UNEXPECTED;
            }

        HX_RELEASE(pRTPPkt);
    }

    if (!m_pPktConverter && HXR_OK == theErr)
    {
        HX_ASSERT(!m_pTSConverter);
        m_pPktConverter = &CSingleStreamPush::PacketConvert;
    }

    if (m_pPktConverter)
    {
        return HXR_OK;
    }
    else
    {
        HX_VERIFY(!"no pkt converter\n");
        return HXR_FAIL;

    }
}

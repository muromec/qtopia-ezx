/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: inputsrc_push.cpp,v 1.9 2007/06/19 01:57:36 seansmith Exp $ 
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

//XXXSMP FIX THE INTERFACES TO STDMETHODIMP

#ifdef _UNIX
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#endif // _UNIX

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "ihxpckts.h"
#include "hxplugn.h"
#include "hxpiids.h"

#include "hxbdwdth.h"

#include "hxmap.h"

#include "hxslist.h"
#include "hxstring.h"
#include "plgnhand.h"
#include "hxstrutl.h"

#include "srcerrs.h"
#include "hxprot.h"
#include "servpckts.h"

#include "hxasm.h"
#include "asmrulep.h"

#include "proc.h"
#include "proc_container.h"
#include "engine.h"
#include "fsys_wrap.h"
#include "base_errmsg.h"

#include "inputsrc_push.h"

StreamPacketState::StreamPacketState () :
    m_bBlocked (FALSE),
    m_bDone (FALSE),
    m_pPacket (NULL)
{
}

void
StreamPacketState::SetPacket(ServerPacket* pPacket)
{
    HX_ASSERT(pPacket);
    HX_ASSERT(!m_pPacket);

    HX_RELEASE(m_pPacket);
  
    if (pPacket)
    {
	m_pPacket = pPacket;
	m_pPacket->AddRef();
    }
}

ServerPacket* 
StreamPacketState::GetPacket()
{
    ServerPacket* pRet = NULL;

    if (m_pPacket)
    {
	pRet = m_pPacket;
	//One AddRef+One Release
	m_pPacket = NULL;
    }
    
    return pRet;
}
    
StreamPacketState::~StreamPacketState ()
{
    HX_RELEASE(m_pPacket);
}

/* StaticPushSource Implementation: */
StaticPushSource::StaticPushSource(Process* proc, 
				   IHXFileFormatObject* pFileFormat,
				   IUnknown* pFileObject,
				   IHXRequest* pRequest) :
    m_lNumStreams (0)
    , m_lRefCount (0)
    , m_nCurrentStream(0)
    , m_pBWEValues (NULL)
    , m_pStreams (NULL)
    , m_pSinkControl (NULL)
    , m_pSink (NULL)
    , m_proc (proc)
    , m_pFileObject (pFileObject)
    , m_pFileFormat (pFileFormat)
    , m_pRequest(pRequest)
    , m_ulRecursionCounter(0)
{
    if (m_pFileObject)
    {
	m_pFileObject->AddRef();
    }

    if (m_pFileFormat)
    {
	m_pFileFormat->AddRef();
    }

    if (m_pRequest)
    {
	m_pRequest->AddRef();
    }
}

StaticPushSource::~StaticPushSource()
{
    Done();
}

STDMETHODIMP 
StaticPushSource::QueryInterface(REFIID riid, void** ppvObj)
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
    else if(IsEqualIID(riid, IID_IHXFormatResponse))
    {
	AddRef();
	*ppvObj = (IHXFormatResponse*)this;
	return HXR_OK;
    }
    else if ( (IsEqualIID(riid, IID_IHXFileFormatObject)) && m_pFileFormat)
    {
	return m_pFileFormat->QueryInterface(riid, ppvObj);
    }
    else if ( (IsEqualIID(riid, IID_IHXPacketTimeOffsetHandler)) && m_pFileFormat)
    {
	return m_pFileFormat->QueryInterface(riid, ppvObj);
    }
    else if ( (IsEqualIID(riid, IID_IHXLiveFileFormatInfo)) && m_pFileFormat)
    {
	return m_pFileFormat->QueryInterface(riid, ppvObj);
    }
    else if ( (IsEqualIID(riid, IID_IHXASMSource)) && m_pFileFormat)
    {
	return m_pFileFormat->QueryInterface(riid, ppvObj);
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) 
StaticPushSource::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) 
StaticPushSource::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT
StaticPushSource::Init(IHXPSinkControl* pSink, IHXValues* values)
{
    m_pBWEValues = values;
    m_pBWEValues->AddRef();

    return Init(pSink);
}

STDMETHODIMP
StaticPushSource::Done()
{
    HX_VECTOR_DELETE(m_pStreams);
    HX_RELEASE(m_pSinkControl);
    HX_RELEASE(m_pFileObject);
    HX_RELEASE(m_pBWEValues);
    HX_RELEASE(m_pRequest);

    if (m_pSink)
    {
	m_pSink->SourceDone();
	m_pSink->Release();
	m_pSink = NULL;
    }

    if (m_pFileFormat)
    {
        /*
         * Close() must be called before Release() so that the
         * IHXFileFormatObject will properly free its IHXFileObject
         */

        m_pFileFormat->Close();
        m_pFileFormat->Release();
        m_pFileFormat = NULL;
    }

    return HXR_OK;
}

HX_RESULT
StaticPushSource::Init(IHXPSinkControl* pSink)
{
    HX_RESULT			h_result = HXR_OK;
    IHXFileObject*             pFileObject;
    IHXBandwidthNegotiator*    pBandwidthNegotiator;

    pSink->AddRef();
    m_pSinkControl = pSink;

    AddRef();
    if (HXR_OK == h_result)
    {
	if(HXR_OK == m_pFileObject->QueryInterface(IID_IHXFileObject,
						 (void**)&pFileObject))
	{
	  
	    if(m_pBWEValues && 
	       (HXR_OK == m_pFileFormat->QueryInterface(
		IID_IHXBandwidthNegotiator,
		(void**)&pBandwidthNegotiator)))
	    {
		pBandwidthNegotiator->SetBandwidthInfo(m_pBWEValues);
		pBandwidthNegotiator->Release();
		m_pBWEValues->Release();
		m_pBWEValues = NULL;
	    }

	    h_result = m_pFileFormat->InitFileFormat(m_pRequest, this,
						     pFileObject);
	    /*
	     * The IHXFileObject belongs to the IHXFileFormatObject...
	     */
	    
	    HX_RELEASE(m_pFileObject);

	    HX_RELEASE(pFileObject);

	    /*
	     * so does the IHXRequest
	     */

	    HX_RELEASE(m_pRequest);

	    if (HXR_OK != h_result)
	    {

		ERRMSG(m_proc->pc->error_handler,
		       "File Format initialization failed\n");
		m_pFileFormat->Close();
		m_pFileFormat->Release();
		m_pFileFormat = 0;
	    }
	}

    }

    Release();

    return h_result;
}

HX_RESULT
StaticPushSource::GetFileHeader(IHXPSinkControl* pSink)
{
    ASSERT(pSink == m_pSinkControl);
    HX_RESULT rc;

    rc = m_pFileFormat->GetFileHeader();

    return rc;
}

HX_RESULT
StaticPushSource::GetStreamHeader(IHXPSinkControl* pSink, UINT16 stream_number)
{
    ASSERT(pSink == m_pSinkControl);
    HX_RESULT rc;
    
    rc =  m_pFileFormat->GetStreamHeader(stream_number);

    return rc;
}

STDMETHODIMP
StaticPushSource::SetSink(IHXServerPacketSink* pSink)
{
    HX_ASSERT(pSink);

    if (!pSink)
    {
	return HXR_INVALID_PARAMETER;
    }

    HX_RELEASE(m_pSink);
    m_pSink = pSink;
    m_pSink->AddRef();

    return HXR_OK;
}

STDMETHODIMP
StaticPushSource::StartPackets()
{
    m_nCurrentStream = 0;    
    RequestPacket();    
    return HXR_OK;
}

STDMETHODIMP
StaticPushSource::GetPacket()
{
    HX_ASSERT(0);
    return HXR_NOTIMPL;
}


STDMETHODIMP
StaticPushSource::SinkBlockCleared(UINT32 ulStream)
{
    HX_ASSERT(m_pStreams);
    HX_ASSERT(m_pStreams[ulStream].m_bBlocked);

    m_pStreams[ulStream].m_bBlocked = FALSE;
    m_nCurrentStream = ulStream;

    // we should already have a packet ready to be sent
    ServerPacket* pPkt = m_pStreams [m_nCurrentStream].GetPacket();	
    if (pPkt)
    {
	TransmitPacket(pPkt);
	HX_RELEASE(pPkt);
    }
    else
    {
	HX_ASSERT(!"no pkt!!!");
    }			

    return HXR_OK;
}

void
StaticPushSource::RequestPacket()
{
    HX_ASSERT(!m_pStreams [m_nCurrentStream].m_bBlocked && 
	      !m_pStreams [m_nCurrentStream].m_bDone);

    // XXXGo - make sure to break recursion at some point
    ++m_ulRecursionCounter;    
    HX_ASSERT(m_ulRecursionCounter <= 5); // SCREAM if recurses more than 5
    
    m_pFileFormat->GetPacket(m_nCurrentStream);
    (++m_nCurrentStream) %= m_lNumStreams;
    
    --m_ulRecursionCounter;    
    HX_ASSERT((INT32)m_ulRecursionCounter >= 0);

    return;
}

void
StaticPushSource::TransmitPacket(ServerPacket* pPacket)
{    
    if (HXR_BLOCKED == m_pSink->PacketReady(pPacket))
    {
	m_pStreams [pPacket->GetStreamNumber()].m_bBlocked = TRUE;
	m_pStreams [pPacket->GetStreamNumber()].SetPacket(pPacket);
    }
    else
    {
	// not BLOCKED.  Keep fetching
	RequestPacket();
    }

    return;
}	

HX_RESULT
StaticPushSource::PacketReady(HX_RESULT status, IHXPacket* packet)
{
    if (HXR_OK == status)
    {
	TransmitPacket(HXPacketToServerPacket(packet));
    }
    else
    {
	UINT16 i;
	for (i = 0; i < m_lNumStreams; i++)
	{
	    StreamDone(i);
	    m_pStreams [i].m_bDone = TRUE;
	}
    }

    return HXR_OK;
}

HX_RESULT
StaticPushSource::InitDone(HX_RESULT status)
{
    if (m_pSinkControl)
        m_pSinkControl->InitDone(status);

    return HXR_OK;
}

HX_RESULT
StaticPushSource::FileHeaderReady(HX_RESULT status, IHXValues* header)
{
    HX_ASSERT(header || (status != HXR_OK));

    if (header)
    {
        header->GetPropertyULONG32("StreamCount", m_lNumStreams);

	m_pStreams = new StreamPacketState [m_lNumStreams];
    }

    if (m_pSinkControl)
    {
        m_pSinkControl->FileHeaderReady(status, header);
    }

    return HXR_OK;
}

HX_RESULT
StaticPushSource::StreamHeaderReady(HX_RESULT status, IHXValues* header)
{    
    if (m_pSinkControl)
    {    	
        m_pSinkControl->StreamHeaderReady(status, header);
    }

    return HXR_OK;
}

HX_RESULT
StaticPushSource::SeekDone(HX_RESULT status)
{
    if (m_pSinkControl)
        m_pSinkControl->SeekDone(status);

    return HXR_OK;
}

HX_RESULT
StaticPushSource::StreamDone(UINT16 stream_number)
{
    m_pStreams[stream_number].m_bDone = TRUE;

    if (m_pSinkControl)
        m_pSinkControl->StreamDone(stream_number);

    return HXR_OK;
}

HX_RESULT
StaticPushSource::Seek(UINT32 seek_time)
{
    HX_RESULT ulResult;

    ulResult = m_pFileFormat->Seek(seek_time);

    if (ulResult == HXR_OK)
    {
	return HXR_OK;
    }
    else if (ulResult == HXR_NOTIMPL)
    {
	SeekDone(HXR_OK);
    }
    else
    {
	SeekDone(ulResult);
    }

    return HXR_OK;
}

BOOL
StaticPushSource::IsLive()
{
    return FALSE;
}

ServerPacket*
StaticPushSource::HXPacketToServerPacket(IHXPacket* pPacket)
{
    ServerPacket* pServerPacket;

    if (pPacket->QueryInterface(IID_ServerPacket,
                                (void **)0xffffd00d) == HXR_OK)
    {
	pServerPacket = (ServerPacket *)pPacket;
    }
    else
    {
	HX_ASSERT(0);
    }

    // XXXDWL don't set the TSD flag
    
    return pServerPacket;
}


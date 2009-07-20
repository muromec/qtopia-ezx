/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: dtcvtcon.cpp,v 1.6 2007/08/18 00:21:13 dcollins Exp $ 
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
#include "hxdtcvt.h"
#include "hxfiles.h"
#include "hxengin.h"
#include "server_context.h"

#include "hxmap.h"
#include "plgnhand.h"

#include "proc.h"
#include "dtcvtcon.h"

/*
 * DataConvertShim (there is one of these per IHXDataConvert)
 */
DataConvertShim::DataConvertShim(IHXDataConvert* pConvert, Process* proc)
: m_lRefCount(0)
, m_pControlResponse(0)
, m_pDataResponse(0)
, m_pConverter(0)
, m_bWaitForInitDone(FALSE)
, m_pCachePacketList(NULL)
{
    m_pConverter = pConvert;
    m_pConverter->AddRef();
}

DataConvertShim::~DataConvertShim()
{
    HX_RELEASE(m_pConverter);
    HX_RELEASE(m_pControlResponse);
    HX_RELEASE(m_pDataResponse);
    if (m_pCachePacketList)
    {
	delete m_pCachePacketList;
    }
}

STDMETHODIMP
DataConvertShim::QueryInterface(REFIID riid,
			       void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IHXDataConvertResponse*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXDataConvertResponse))
    {
	AddRef();
	*ppvObj = (IHXDataConvertResponse*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXDataConvert))
    {
	m_pConverter->AddRef();
	*ppvObj = m_pConverter;
	return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
DataConvertShim::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
DataConvertShim::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

void
DataConvertShim::DataConvertInit(IUnknown* pContext)
{
    IHXPlugin* pPlugin = NULL;
    m_pConverter->QueryInterface(IID_IHXPlugin, (void**)&pPlugin);
    if (pPlugin)
    {
	pPlugin->InitPlugin(pContext);
	pPlugin->Release();
    }
    m_pConverter->DataConvertInit(this);
}

void
DataConvertShim::ConvertFileHeader(IHXValues* pHeader)
{
    m_pConverter->ConvertFileHeader(pHeader);
}

void
DataConvertShim::ConvertStreamHeader(IHXValues* pHeader)
{
    m_pConverter->ConvertStreamHeader(pHeader);
}

void
DataConvertShim::ConvertData(IHXPacket* pPacket)
{
    if (m_bWaitForInitDone)
    {
	if (!m_pCachePacketList)
	{
	    m_pCachePacketList = new CHXSimpleList;
	}
	pPacket->AddRef();
	m_pCachePacketList->AddTail((void*)pPacket);
    }
    else
    {
	m_pConverter->ConvertData(pPacket);
    }
}

void
DataConvertShim::ControlBufferReady(IHXBuffer* pBuffer)
{
    m_pConverter->ControlBufferReady(pBuffer);
}

void
DataConvertShim::Done()
{
    if (m_pConverter)
    {
	m_pConverter->Done();
    }
    HX_RELEASE(m_pConverter);
}

void
DataConvertShim::GetConversionMimeType(REF(const char*)pConversionType)
{
    m_pConverter->GetConversionMimeType(pConversionType);
}

void
DataConvertShim::SetControlResponse(IHXDataConvertResponse* pResp)
{
    HX_RELEASE(m_pControlResponse);
    if (pResp)
    {
	m_pControlResponse = pResp;
	m_pControlResponse->AddRef();
    }
    else
    {
	m_bWaitForInitDone = TRUE;
    }
}

void
DataConvertShim::SetDataResponse(IHXDataConvertResponse* pResp)
{
    HX_RELEASE(m_pDataResponse);
    m_pDataResponse = pResp;
    m_pDataResponse->AddRef();
}

void
DataConvertShim::SetRequest(IHXRequest* pRequest)
{
    IHXRequestHandler* pRequestHandler = NULL;
    if (HXR_OK == m_pConverter->QueryInterface(IID_IHXRequestHandler,
		(void**)&pRequestHandler))
    {
	pRequestHandler->SetRequest(pRequest);
	pRequestHandler->Release();
    }
}


/************************************************************************
 *  IHXDataConvertResponse
 */
STDMETHODIMP
DataConvertShim::DataConvertInitDone(HX_RESULT status)
{
    if (m_pControlResponse)
    {
	return m_pControlResponse->DataConvertInitDone(status);
    }
    else if (m_bWaitForInitDone)
    {
	if (m_pCachePacketList)
	{
	    IHXPacket* pPacket;
	    while (!m_pCachePacketList->IsEmpty())
	    {
		pPacket = (IHXPacket*)m_pCachePacketList->RemoveHead();
		m_pConverter->ConvertData(pPacket);
		pPacket->Release();
	    }
	    delete m_pCachePacketList;
	}
	m_bWaitForInitDone = FALSE;
    }
    return HXR_OK;
}

STDMETHODIMP
DataConvertShim::ConvertedFileHeaderReady(HX_RESULT status,
					IHXValues* pFileHeader)
{
    HX_RESULT res = m_pControlResponse->ConvertedFileHeaderReady(status,
	    pFileHeader);
    return res;
}

STDMETHODIMP
DataConvertShim::ConvertedStreamHeaderReady(HX_RESULT status,
					IHXValues* pStreamHeader)
{
    HX_RESULT res = m_pControlResponse->ConvertedStreamHeaderReady(status,
							    pStreamHeader);
    return res;
}

STDMETHODIMP
DataConvertShim::ConvertedDataReady(HX_RESULT status, IHXPacket* pPacket)
{
    m_pDataResponse->ConvertedDataReady(status, pPacket);
    return HXR_OK;
}

STDMETHODIMP
DataConvertShim::SendControlBuffer(IHXBuffer* pBuffer)
{
    if (!m_pControlResponse)
    {
	return HXR_FAIL;
    }
    m_pControlResponse->SendControlBuffer(pBuffer);
    return HXR_OK;
}

void
DataConvertShim::SetMulticastTransport(DataConvertShim* pDTCShim)
{
    pDTCShim->SetMulticastTransportFor(m_pConverter);
}

void
DataConvertShim::SetMulticastTransportFor(IHXDataConvert* pTransport)
{
    pTransport->SetMulticastTransportConverter(m_pConverter);
}

void
DataConvertShim::AddMulticastControl(DataConvertShim* pDTCShim)
{
    pDTCShim->AddMulticastControlFor(m_pConverter);
}

void
DataConvertShim::AddMulticastControlFor(IHXDataConvert* pTransport)
{
    pTransport->AddMulticastControlConverter(m_pConverter);
}

/*
 * DataConvertController (there is only one of these per server)
 */
DataConvertController::DataConvertController()
{
}

DataConvertController::~DataConvertController()
{
}

IHXDataConvert*
DataConvertController::GetConverter(Process* proc, IHXBuffer* pUrlBuf)
{
    IHXDataConvert* pRet = NULL;
    HX_RESULT h_result;
    IUnknown* instance;

    if (!pUrlBuf)
    {
	return NULL;
    }
    const char* pURL = (const char*)pUrlBuf->GetBuffer();
    PluginHandler::DataConvert::PluginInfo* pPluginInfo;
    
    if (PluginHandler::NO_ERRORS == proc->pc->plugin_handler->
	    m_data_convert_handler->Find(pURL, pPluginInfo) && pPluginInfo)
    {
	PluginHandler::Plugin* pPlugin = pPluginInfo->m_pPlugin;
	IHXDataConvertSystemObject* pDataConvert;
	if (!pPlugin->m_load_multiple)
	{
	    HX_ASSERT(0);
	    return NULL;
	}
	if (!proc->pc->cached_dc_map->Lookup(pPluginInfo->m_ulID,
		    (void*&)pDataConvert))
	{
	    IHXPlugin* plugin_interface;
	    pPlugin->GetInstance(&instance);

	    h_result = instance->QueryInterface(IID_IHXPlugin, 
						(void**) &plugin_interface);
	    if (HXR_OK == h_result)
	    {
		h_result = plugin_interface->InitPlugin((IUnknown*)proc->pc->
							server_context);
	    }
	    plugin_interface->Release();
		    
	    if (HXR_OK == h_result)
	    {
		h_result = instance->QueryInterface(
			IID_IHXDataConvertSystemObject, (void**)&pDataConvert);
	    }
	    
	    if (h_result == HXR_OK)
	    {
		pDataConvert->InitDataConvertSystem(pPluginInfo->m_options);
	    }
	    /*
	     * Cache this one away in our map.
	     */
	    proc->pc->cached_dc_map->SetAt(pPluginInfo->m_ulID,
		    (void*)pDataConvert);

	    HX_RELEASE(instance);
	}
	IHXDataConvert* pRet;
	pDataConvert->CreateDataConvert((IUnknown**)&instance);
	instance->QueryInterface(IID_IHXDataConvert, (void**)&pRet);
	instance->Release();
	return pRet;

    }
    else
    {
	return NULL;
    }
}


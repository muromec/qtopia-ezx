/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: dtrvtcon.cpp,v 1.7 2006/10/06 21:03:40 ping Exp $
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

#include "hxcom.h"

#include "ihxpckts.h"
#include "pckunpck.h"
#include "hxdtcvt.h"
#include "dtrvtcon.h"
#include "hxmarsh.h"

#include "hxstrutl.h"

#include "rtsputil.h"
#include "hxbuffer.h"

#define HX_CONVERT_MIME_TYPE "application/vnd.rn.dataconvert."

DataRevertController::DataRevertController(IUnknown* pContext)
: m_pPlugin2Handler(0)
, m_pContext(0)
, m_pControlResp(0)
, m_lRefCount(0)
, m_pDataRevert(0)
, m_pStreamHeaders(0)
, m_pRevertedStreamHeaders(0)
, m_pFileHeaders(0)
, m_pResponseHeaders(0)
, m_pCurrentStreamHeader(0)
, m_pDataResp(0)
, m_pPacketList(0)
, m_pControlBufferList(0)
, m_bInited(FALSE)
{
    m_pContext = pContext;
    m_pContext->AddRef();
    m_pContext->QueryInterface(IID_IHXPlugin2Handler,
	    (void**)&m_pPlugin2Handler);
}

DataRevertController::~DataRevertController()
{
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pPlugin2Handler);
    HX_RELEASE(m_pDataRevert);
    HX_RELEASE(m_pCurrentStreamHeader);
    CleanStreamHeaders();
    CleanControlBuffers();
    HX_RELEASE(m_pFileHeaders);
    HX_RELEASE(m_pResponseHeaders);
    delete m_pPacketList;
}

void
DataRevertController::CleanControlBuffers()
{
    IHXBuffer* pBuffer;
    if (m_pControlBufferList)
    {
	while (!m_pControlBufferList->IsEmpty())
	{
	    pBuffer = (IHXBuffer*)m_pControlBufferList->RemoveHead();
	    pBuffer->Release();
	}
	delete m_pControlBufferList;
    }
}

void
DataRevertController::CleanStreamHeaders()
{
    IHXValues* pHeader;
    if (m_pStreamHeaders)
    {
	while (!m_pStreamHeaders->IsEmpty())
	{
	    pHeader = (IHXValues*)m_pStreamHeaders->RemoveHead();
	    pHeader->Release();
	}
	delete m_pStreamHeaders;
    }
    if (m_pRevertedStreamHeaders)
    {
	while (!m_pRevertedStreamHeaders->IsEmpty())
	{
	    pHeader = (IHXValues*)m_pRevertedStreamHeaders->RemoveHead();
	    pHeader->Release();
	}
	delete m_pRevertedStreamHeaders;
    }
}

void
DataRevertController::SetControlResponse(DataRevertControllerResponse* pResp)
{
    m_pControlResp = pResp;
}

void
DataRevertController::RevertHeaders(IHXValues* pFileHeader,
				    CHXSimpleList* pStreamHeaders,
				    IHXValues* pResponseHeaders)
{
    IHXBuffer* pMimeType = 0;
    IHXValues* pHeader;
    CHXSimpleList::Iterator i;
    char* pConversionType = NULL;
    IUnknown* pUnkReverter = NULL;
    HX_RELEASE(m_pDataRevert);
    
    i = pStreamHeaders->Begin();
    if (i != pStreamHeaders->End())
    {
	pHeader = (IHXValues*)(*i);
	pHeader->GetPropertyCString("MimeType", pMimeType);
	if (!pMimeType)
	{
	    HX_ASSERT(0);
	    goto exit;
	}
	if (strncasecmp((const char*)pMimeType->GetBuffer(),
		    HX_CONVERT_MIME_TYPE, (int)strlen(HX_CONVERT_MIME_TYPE)))
	{
	    goto exit;
	}
	
	pConversionType = (char*)pMimeType->GetBuffer() +
			    strlen(HX_CONVERT_MIME_TYPE);
	if (m_pPlugin2Handler &&
	    HXR_OK == m_pPlugin2Handler->FindPluginUsingStrings(
					PLUGIN_CLASS, PLUGIN_REVERTER_TYPE,
					PLUGIN_REVERTER_MIME, pConversionType,
					NULL, NULL, pUnkReverter))
	{
	    pUnkReverter->QueryInterface(IID_IHXDataRevert,
		    (void**)&m_pDataRevert);
	    pUnkReverter->Release();
	}
	if (!m_pDataRevert)
	{
	    goto exit;
	}
	IHXPlugin* pPlugin;
	m_pDataRevert->QueryInterface(IID_IHXPlugin, (void**)&pPlugin);
	pPlugin->InitPlugin(m_pContext);
	pPlugin->Release();
	HX_RELEASE(pMimeType);
	
	m_pStreamHeaders = new CHXSimpleList;
	m_pRevertedStreamHeaders = new CHXSimpleList;
	IHXBuffer* pConvertHeader = 0;
	for (i = pStreamHeaders->Begin(); i != pStreamHeaders->End(); ++i)
	{
	    pHeader = (IHXValues*)(*i);
	    /*
	     * If this stream header was converted and flattened then
	     * the one we want to give to the plugin is the result
	     * of re-inflating that.  If not, then just give the plugin
	     * the one we already got.
	     */
	    if (HXR_OK == pHeader->GetPropertyBuffer("DataConvertStreamHeader",
						    pConvertHeader))
	    {
		pHeader = InflateConvertHeader(pConvertHeader);
		pConvertHeader->Release();
	    }
	    else
	    {
		IHXBuffer* pPreConvertMimeType;
		if (HXR_OK == pHeader->GetPropertyCString("PreConvertMimeType",
							pPreConvertMimeType))
		{
		    pHeader->SetPropertyCString("MimeType",
						pPreConvertMimeType);
		    pPreConvertMimeType->Release();
		}
		pHeader->AddRef();
	    }
	    m_pStreamHeaders->AddTail((void*)pHeader);
	}
	m_pResponseHeaders = pResponseHeaders;
	m_pResponseHeaders->AddRef();
	
	/*
	 * If playing through an old proxy which does not support
	 * initiate-session then the DataConvertBuffer will come in here.
	 * This is not an ideal situation because only one can come in
	 * at this point, but it's better then nothing. 
	 */
	IHXBuffer* pConvertBuffer = 0;
	if (HXR_OK == pFileHeader->GetPropertyBuffer("DataConvertBuffer",
		    pConvertBuffer))
	{
	    const char* pContent = (const char*)pConvertBuffer->GetBuffer();
	    IHXBuffer* pNewBuffer = NULL;
	    if (HXR_OK == CreateBufferCCF(pNewBuffer, m_pContext))
	    {
		int contentLen = pConvertBuffer->GetSize();
		pNewBuffer->SetSize(contentLen);
		int offset = BinFrom64(pContent, contentLen,
				       (unsigned char*)pNewBuffer->GetBuffer());
		pNewBuffer->SetSize(offset);
		ControlBufferReady(pNewBuffer);
		HX_RELEASE(pNewBuffer);
	    }
	    HX_RELEASE(pConvertBuffer);
	}
	/*
	 * Again for file header, if the header was converted and
	 * flattened then give to plugin the inflated version of that.
	 * If not, then give the straight old header that we already
	 * have.
	 */
	if (HXR_OK == pFileHeader->GetPropertyBuffer("DataConvertFileHeader",
		    pConvertHeader))
	{
	    m_pFileHeaders = InflateConvertHeader(pConvertHeader);
	    pConvertHeader->Release();
	}
	else
	{
	    m_pFileHeaders = pFileHeader;
	    m_pFileHeaders->AddRef();
	}

	m_pDataRevert->DataRevertInit(this);
	return;
    }


exit:;
    HX_RELEASE(pMimeType);
    m_pControlResp->RevertHeadersDone(pFileHeader, 
				      pStreamHeaders,
				      pResponseHeaders,
				      FALSE);
}

void
DataRevertController::ControlBufferReady(IHXBuffer* pBuffer)
{
    if (!m_bInited)
    {
	if (!m_pControlBufferList)
	{
	    m_pControlBufferList = new CHXSimpleList;
	}
	m_pControlBufferList->AddTail((void*)pBuffer);
	pBuffer->AddRef();
    }
    else
    {
	m_pDataRevert->ControlBufferReady(pBuffer);
    }
}

/*
 *  IUnknown methods
 */
STDMETHODIMP
DataRevertController::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXDataRevertResponse), (IHXDataRevertResponse*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXDataRevertResponse*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32)
DataRevertController::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
DataRevertController::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

/************************************************************************
 *  IHXDataRevertResponse
 */
STDMETHODIMP
DataRevertController::DataRevertInitDone(HX_RESULT status)
{
    m_bInited = TRUE;
    if (m_pControlBufferList)
    {
	IHXBuffer* pBuffer;
	while (!m_pControlBufferList->IsEmpty())
	{
	    pBuffer = (IHXBuffer*)m_pControlBufferList->RemoveHead();
	    m_pDataRevert->ControlBufferReady(pBuffer);
	    pBuffer->Release();
	}
    }
    m_pDataRevert->RevertFileHeader(m_pFileHeaders);
    return HXR_OK;
}

STDMETHODIMP
DataRevertController::RevertedFileHeaderReady(HX_RESULT status,
						IHXValues* pHeader)
{
    if (pHeader)
    {
	HX_RELEASE(m_pFileHeaders);
	m_pFileHeaders = pHeader;
	pHeader->AddRef();
    }

    pHeader = (IHXValues*)m_pStreamHeaders->RemoveHead(); 
    if (!pHeader)
    {
	m_pControlResp->RevertHeadersDone(m_pFileHeaders,
					  m_pRevertedStreamHeaders, 
					  m_pResponseHeaders,
					  TRUE);
	return HXR_OK;
    }
    
    HX_RELEASE(m_pCurrentStreamHeader);
    m_pCurrentStreamHeader = pHeader;
    m_pDataRevert->RevertStreamHeader(pHeader);

    return HXR_OK;
}

STDMETHODIMP
DataRevertController::RevertedStreamHeaderReady(HX_RESULT status,
						IHXValues* pHeader)
{
    if (!pHeader)
    {
	pHeader = m_pCurrentStreamHeader;
    }
    else
    {
	pHeader->AddRef();
    }
    m_pRevertedStreamHeaders->AddTail((void*)pHeader);
    if (!m_pStreamHeaders->IsEmpty())
    {
	HX_RELEASE(m_pCurrentStreamHeader);
	m_pCurrentStreamHeader = (IHXValues*)m_pStreamHeaders->RemoveHead();
	m_pDataRevert->RevertStreamHeader(m_pCurrentStreamHeader);
    }
    else
    {
	m_pControlResp->RevertHeadersDone(m_pFileHeaders,
					  m_pRevertedStreamHeaders, 
					  m_pResponseHeaders,
					  TRUE);
    }

    return HXR_OK;
}

STDMETHODIMP
DataRevertController::RevertedDataReady(HX_RESULT status, IHXPacket* pPacket)
{
    IHXPacket* pThisPacket;
    pThisPacket = (IHXPacket*)m_pPacketList->RemoveTail();
    if (pPacket)
    {
	HX_RELEASE(pThisPacket);
	pThisPacket = pPacket;
	pThisPacket->AddRef();
    }
    m_pDataResp->FilterPacket(pThisPacket);
    pThisPacket->Release();
    return HXR_OK;
}

STDMETHODIMP
DataRevertController::SendControlBuffer(IHXBuffer* pBuffer)
{
    m_pControlResp->SendControlBuffer(pBuffer);
    return HXR_OK;
}

void
DataRevertController::FilterPacket(IHXPacket* pPacket)
{
    if (m_pDataRevert)
    {
	pPacket->AddRef();
	if (!m_pPacketList)
	{
	    m_pPacketList = new CHXSimpleList;
	}
	m_pPacketList->AddHead((void*)pPacket);
	m_pDataRevert->RevertData(pPacket);
    }
    else
    {
	m_pDataResp->FilterPacket(pPacket);
    }
}

void
DataRevertController::SetFilterResponse(RawPacketFilter* pFilt)
{
    m_pDataResp = pFilt;
}

IHXValues*
DataRevertController::InflateConvertHeader(IHXBuffer* pInflate)
{
    IHXValues* pNewHeader = NULL;
    IHXCommonClassFactory* pCCF;
    
    m_pContext->QueryInterface(IID_IHXCommonClassFactory,
	    (void**)&pCCF);
    
    pCCF->CreateInstance(CLSID_IHXValues, (void**)&pNewHeader);
    ULONG32 ul;
    IHXBuffer* pBuffer;
    const char* p;
    IHXBuffer* pTemp;
    ULONG32 ulTemp;

    pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pTemp);
    UINT32 limit = pInflate->GetSize();
    UINT32 i = 0;
    p = (const char*)pInflate->GetBuffer();
    while (i < pInflate->GetSize() - 5)
    {
	if (p[i] == 'u')
	{
	    i++;
	    ulTemp = getlong((UINT8*) &(p[i]));
	    i += 4;
	    if (i + ulTemp > limit)
	    {
		goto error;
	    }
	    pTemp->SetSize(ulTemp + 1);
	    memcpy((char *)pTemp->GetBuffer(), &(p[i]), ulTemp); /* Flawfinder: ignore */
	    ((char*)pTemp->GetBuffer())[ulTemp] = 0;
	    i += ulTemp;
	    if (i + 4 > limit)
	    {
		goto error;
	    }
	    ul = getlong((UINT8*) &(p[i]));
	    i += 4;
	    pNewHeader->SetPropertyULONG32((char*)pTemp->GetBuffer(), ul);
	}
	else if (p[i] == 's' || p[i] == 'b')
	{
	    int at = i;
	    i++;
	    ulTemp = getlong((UINT8*) &(p[i]));
	    i += 4;
	    if (i + ulTemp > limit)
	    {
		goto error;
	    }
	    pTemp->SetSize(ulTemp + 1);
	    memcpy((char*)pTemp->GetBuffer(), &(p[i]), ulTemp); /* Flawfinder: ignore */
	    ((char*)pTemp->GetBuffer())[ulTemp] = 0;
	    i += ulTemp;
	    if (i + 4 > limit)
	    {
		goto error;
	    }
	    ulTemp = getlong((UINT8*) &(p[i]));
	    i += 4;
	    if (i + ulTemp > limit)
	    {
		goto error;
	    }
	    pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);
	    pBuffer->SetSize(ulTemp);
	    memcpy((char*)pBuffer->GetBuffer(), &(p[i]), ulTemp); /* Flawfinder: ignore */

	    if (p[at] == 's')
	    {
		pNewHeader->SetPropertyCString((const char*)pTemp->GetBuffer(),
						pBuffer);
	    }
	    else
	    {
		pNewHeader->SetPropertyBuffer((const char*)pTemp->GetBuffer(),
						pBuffer);
	    }
	    pBuffer->Release();
	    i+= ulTemp;
	}
	else
	{
	    goto error;
	}
    }
    if (i != pInflate->GetSize())
    {
	goto error;
    }
    
    goto exit;
    
error:;

    
exit:;

    HX_RELEASE(pTemp);
    HX_RELEASE(pCCF);
    
    return pNewHeader;
}

void
DataRevertController::Done(void)
{
    HX_RELEASE(m_pDataRevert);
}

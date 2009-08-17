/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ramffpln.cpp,v 1.10 2007/02/08 18:24:16 ehyche Exp $
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

#define INITGUID

#define PACKET_SIZE	    8192

#define MAX_REASONABLE_RAM_FILE_SIZE    0x100000 /* limit file size to 1MB */

#include "ramfformat.ver"

#include "hlxclib/stdlib.h"
#include "hxtypes.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxcore.h"
#include "hxgroup.h"
#include "hxbuffer.h"
#include "hxfiles.h"
#include "hxplugn.h"
#include "hxstring.h"
#include "chxpckts.h"
#include "pckunpck.h"
#include "hxrquest.h"
#include "hxmime.h"
#include "hxformt.h"
#include "ramffpln.h"
#include "hxver.h"
#include "chxfgbuf.h"
#if defined(HELIX_FEATURE_VIEWSOURCE)
#include "hxvsrc.h"
#include "ramvsrc.h"
#endif /* #if defined(HELIX_FEATURE_VIEWSOURCE) */
#include "perplex.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

INT32 g_nRefCount_ramff = 0;

/****************************************************************************
 * 
 *  Function:
 * 
 *	HXCreateInstance()
 * 
 *  Purpose:
 * 
 *	Function implemented by all plugin DLL's to create an instance of 
 *	any of the objects supported by the DLL. This method is similar to 
 *	Window's CoCreateInstance() in its purpose, except that it only 
 *	creates objects from this plugin DLL.
 *
 *	NOTE: Aggregation is never used. Therefore and outer unknown is
 *	not passed to this function, and you do not need to code for this
 *	situation.
 * 
 */

STDAPI ENTRYPOINT(HXCREATEINSTANCE)
(
    IUnknown**  /*OUT*/	ppIUnknown
)
{
    *ppIUnknown = (IUnknown*)(IHXPlugin*)new CRAMFileFormat();
    if (*ppIUnknown)
    {
	(*ppIUnknown)->AddRef();
	return HXR_OK;
    }
    return HXR_OUTOFMEMORY;
}

/****************************************************************************
 * 
 *  Function:
 * 
 *	CanUnload()
 * 
 *  Purpose:
 * 
 *	Function implemented by all plugin DLL's if it returns HXR_OK 
 *	then the pluginhandler can unload the DLL
 *
 */
STDAPI ENTRYPOINT(CanUnload)(void)
{
    return (g_nRefCount_ramff ? HXR_FAIL : HXR_OK);
}

const char* CRAMFileFormat::zm_pDescription    = "RealNetworks RealMedia Meta File Format Plugin";
const char* CRAMFileFormat::zm_pCopyright      = HXVER_COPYRIGHT;
const char* CRAMFileFormat::zm_pMoreInfoURL    = HXVER_MOREINFO;

const char* CRAMFileFormat::zm_pFileMimeTypes[]  = {"application/ram", "audio/x-musicnet-stream", NULL};
							  
const char* CRAMFileFormat::zm_pStreamMimeTypes[]= {"application/ram", NULL};
const char* CRAMFileFormat::zm_pFileExtensions[] = {"ram", "rmm", "mns", NULL};
const char* CRAMFileFormat::zm_pFileOpenNames[]  = {"RAM Meta File (*.ram, *.rmm, *.mns)", NULL};

/************************************************************************
 *  Method:
 *    IHXPlugin::InitPlugin
 *  Purpose:
 *    Initializes the plugin for use. This interface must always be
 *    called before any other method is called. This is primarily needed 
 *    so that the plugin can have access to the context for creation of
 *    IHXBuffers and IMalloc.
 */
STDMETHODIMP CRAMFileFormat::InitPlugin(IUnknown* /*IN*/ pContext)
{
    m_pContext = pContext;
    m_pContext->AddRef();

    m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCommonClassFactory);

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXPlugin::GetPluginInfo
 *  Purpose:
 *    Returns the basic information about this plugin. Including:
 *
 *    bLoadMultiple	whether or not this plugin DLL can be loaded
 *			multiple times. All File Formats must set
 *			this value to TRUE.
 *    pDescription	which is used in about UIs (can be NULL)
 *    pCopyright	which is used in about UIs (can be NULL)
 *    pMoreInfoURL	which is used in about UIs (can be NULL)
 */
STDMETHODIMP CRAMFileFormat::GetPluginInfo
(
    REF(HXBOOL)        /*OUT*/ bLoadMultiple,
    REF(const char*) /*OUT*/ pDescription,
    REF(const char*) /*OUT*/ pCopyright,
    REF(const char*) /*OUT*/ pMoreInfoURL,
    REF(ULONG32)     /*OUT*/ ulVersionNumber
)
{
    bLoadMultiple = TRUE;   // Must be true for file formats.

    pDescription    = zm_pDescription;
    pCopyright	    = zm_pCopyright;
    pMoreInfoURL    = zm_pMoreInfoURL;
    ulVersionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}

STDMETHODIMP CRAMFileFormat::GetFileFormatInfo
(
    REF(const char**) /*OUT*/ pFileMimeTypes,
    REF(const char**) /*OUT*/ pFileExtensions,
    REF(const char**) /*OUT*/ pFileOpenNames
)
{
    pFileMimeTypes  = zm_pFileMimeTypes;
    pFileExtensions = zm_pFileExtensions;
    pFileOpenNames  = zm_pFileOpenNames;

    return HXR_OK;
}

CRAMFileFormat::CRAMFileFormat()
    :	m_lRefCount(0)
    ,	m_ulPersistentVersion(0)
    ,	m_pContext(NULL)
    ,	m_pFileObject(NULL)
    ,	m_pFFResponse(NULL)
    ,	m_state(Ready)
    ,	m_pRequest(NULL)
    ,	m_pBuffer(NULL)
    ,	m_bFirstReadDone(TRUE)		
    ,	m_bHeaderSent(FALSE)
    ,	m_pCommonClassFactory(NULL)
{
    g_nRefCount_ramff++;
}


CRAMFileFormat::~CRAMFileFormat()
{
    g_nRefCount_ramff--;
    Close();
}

// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP CRAMFileFormat::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPlugin))
    {
	AddRef();
	*ppvObj = (IHXPlugin*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileFormatObject))
    {
	AddRef();
	*ppvObj = (IHXFileFormatObject*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileResponse))
    {
	AddRef();
	*ppvObj = (IHXFileResponse*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPlaybackVelocity))
    {
	AddRef();
	*ppvObj = (IHXPlaybackVelocity*)this;
	return HXR_OK;
    }
#if defined(HELIX_FEATURE_VIEWSOURCE)
    else if ( IsEqualIID(riid, IID_IHXFileViewSource) )
    {
	CRAMViewSource* pRam = new CRAMViewSource(m_pContext, 
	    (IUnknown*)(IHXPlugin*)this);
	if ( pRam == NULL )
	{
	    return HXR_FAIL;
	}	
	return pRam->QueryInterface(riid, ppvObj);
    }
#endif /* HELIX_FEATURE_VIEWSOURCE */

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
STDMETHODIMP_(ULONG32) CRAMFileFormat::AddRef()
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
STDMETHODIMP_(ULONG32) CRAMFileFormat::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

// *** IHXFileFormatObject methods ***

STDMETHODIMP CRAMFileFormat::InitFileFormat
(
    IHXRequest*	        /*IN*/  pRequest,
    IHXFormatResponse*		/*IN*/  pFFResponse,
    IHXFileObject*		/*IN*/  pFileObject
)
{
    // We need this interface at a minumum!
    if (!pFFResponse) return HXR_INVALID_PARAMETER;
    if (!pRequest) return HXR_INVALID_PARAMETER;

    HX_RELEASE(m_pRequest);
    
    m_pRequest = pRequest;
    m_pRequest->AddRef();

    m_uLastError = HXR_OK;

    m_pFFResponse = pFFResponse;
    m_pFileObject = pFileObject;

    HX_ADDREF(m_pFFResponse);
    HX_ADDREF(m_pFileObject);
 
    m_state = InitPending;

    m_pFileObject->Init(HX_FILE_READ, this);

    if (m_uLastError && m_uLastError != HXR_INVALID_METAFILE &&
	m_uLastError != HXR_DOC_MISSING  &&
        // /Need to pass through out-of-mem error as well:
        HXR_OUTOFMEMORY != m_uLastError)
    {
	m_uLastError = HXR_OK;
    }

    return m_uLastError;
}	

STDMETHODIMP CRAMFileFormat::GetFileHeader()
{
    // If we are not ready then something has gone wrong
    if (m_state != Ready) return HXR_UNEXPECTED;

    IHXValues* pHeader = NULL;
    if (HXR_OK != m_pCommonClassFactory->CreateInstance(CLSID_IHXValues, (void**)&pHeader))
    {
	return HXR_UNEXPECTED;
    }

    pHeader->SetPropertyULONG32("StreamCount", 1);

    m_pFFResponse->FileHeaderReady(HXR_OK, pHeader);

    HX_RELEASE(pHeader);

    return HXR_OK;
}

STDMETHODIMP CRAMFileFormat::GetStreamHeader(UINT16 unStreamNumber)
{
    // If we are not ready then something has gone wrong
    if (m_state != Ready) return HXR_UNEXPECTED;

    IHXValues* pHeader = NULL;
    IHXBuffer* pBuffer = NULL;
    if (HXR_OK != m_pCommonClassFactory->CreateInstance(CLSID_IHXValues, (void**)&pHeader) ||
	HXR_OK != m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer))
    {
	return HXR_UNEXPECTED;
    }

    pBuffer->Set((const BYTE*)zm_pStreamMimeTypes[0], strlen(zm_pStreamMimeTypes[0])+1);
    
    pHeader->SetPropertyCString("MimeType", pBuffer);
    pHeader->SetPropertyULONG32("Duration", 0);
    pHeader->SetPropertyULONG32("StreamNumber", unStreamNumber);
    pHeader->SetPropertyULONG32("AvgBitRate", 1000);
    pHeader->SetPropertyULONG32("PersistentVersion", m_ulPersistentVersion);

    m_bHeaderSent = TRUE;
    m_pFFResponse->StreamHeaderReady(HXR_OK, pHeader);

    HX_RELEASE(pBuffer);    
    HX_RELEASE(pHeader);

    return HXR_OK;
}

STDMETHODIMP CRAMFileFormat::GetPacket(UINT16 unStreamNumber)
{
    HX_RESULT result = HXR_OK;

    if (m_state != Ready || !m_bHeaderSent) 
    {
	return HXR_UNEXPECTED;
    }

    // the content of RAM is sent in ONE packet
    if (!m_pBuffer)
    {
	m_pFFResponse->StreamDone(unStreamNumber);
	return HXR_OK;
    }

    IHXPacket* pPacket = NULL;
    result = m_pCommonClassFactory->CreateInstance(CLSID_IHXPacket, (void**)&pPacket);

    if(HXR_OK == result)
    {
	pPacket->Set(m_pBuffer, 0, unStreamNumber, HX_ASM_SWITCH_ON, 0);
	m_pFFResponse->PacketReady(HXR_OK, pPacket);

	HX_RELEASE(m_pBuffer);
    }
    
    HX_RELEASE(pPacket);    

    return result;
}

STDMETHODIMP CRAMFileFormat::Seek(ULONG32 ulOffset)
{
    m_pFFResponse->SeekDone(HXR_OK);
    return HXR_OK;
}

STDMETHODIMP CRAMFileFormat::Close()
{
    HX_RELEASE(m_pBuffer);    
    HX_RELEASE(m_pRequest);

    HX_RELEASE(m_pFileObject);
    HX_RELEASE(m_pFFResponse);
    HX_RELEASE(m_pCommonClassFactory);
    HX_RELEASE(m_pContext);

    return HXR_OK;
}

STDMETHODIMP CRAMFileFormat::InitDone
(
    HX_RESULT	status
)
{
    // If we are not ready then something has gone wrong
    if (m_state != InitPending) return HXR_UNEXPECTED;

    m_state = ReadPending;

    if (status != HXR_OK)
    {
	m_uLastError = HXR_DOC_MISSING;
    }

    if (m_uLastError == HXR_OK)
    {
	m_pFileObject->Read(PACKET_SIZE);
    }
    else
    {
	m_pFFResponse->InitDone(m_uLastError); 
    }

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileResponse::CloseDone
//  Purpose:
//	Notification interface provided by users of the IHXFileObject
//	interface. This method is called by the IHXFileObject when the
//	close of the file is complete.
//
STDMETHODIMP CRAMFileFormat::CloseDone(HX_RESULT status)
{
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileResponse::ReadDone
//  Purpose:
//	Notification interface provided by users of the IHXFileObject
//	interface. This method is called by the IHXFileObject when the
//	last read from the file is complete and a buffer is available.
//
STDMETHODIMP CRAMFileFormat::ReadDone
(
    HX_RESULT		status,
    IHXBuffer*		pBuffer
)
{
    HX_RESULT	    result = HXR_OK;
    HXBOOL	    bAllReadDone = FALSE;
    char*	    pContent = NULL;
    IHXBuffer*	    pMergedBuffer = NULL;
    IHXValidator*  pValidator = NULL;
    IHXValues*	    pResponseHeaders = NULL;

    if(m_state != ReadPending) return HXR_UNEXPECTED;

    if (SUCCEEDED(status))
    {
	pContent = (char*) pBuffer->GetBuffer();

	// validate RAM on first read
	if (m_bFirstReadDone	&&
	    m_pContext		&&
    	    HXR_OK == m_pContext->QueryInterface(IID_IHXValidator, (void**)&pValidator))
	{
	    result = pValidator->ValidateMetaFile(m_pRequest, pBuffer);

	    if (HXR_OK != result)
	    {
		bAllReadDone = TRUE;
		m_pFileObject->Seek(0, FALSE);
		m_uLastError = result;
		goto cleanup;
	    }

	    // retrieve the version info
	    if (HXR_OK == m_pRequest->GetResponseHeaders(pResponseHeaders) && 
		pResponseHeaders)
	    {
		pResponseHeaders->GetPropertyULONG32("PersistentVersion", m_ulPersistentVersion);
	    }
	    HX_RELEASE(pResponseHeaders);

	    m_bFirstReadDone = FALSE;
	}
    
	CreateBufferCCF(pMergedBuffer, m_pContext);
	if (!m_pBuffer)
	{
	    pMergedBuffer->SetSize(pBuffer->GetSize());
	    pMergedBuffer->Set(pBuffer->GetBuffer(), pBuffer->GetSize());
	}
	else
	{
            // /Check return value in case SetSize()'s alloc fails.  Overflow
            // was possible when this happened.  Fixes PR 93752:
            if (HXR_OK == (result = pMergedBuffer->SetSize(m_pBuffer->GetSize() + pBuffer->GetSize()) ) )
            {
                memcpy(pMergedBuffer->GetBuffer(), m_pBuffer->GetBuffer(), m_pBuffer->GetSize()); /* Flawfinder: ignore */
                memcpy(pMergedBuffer->GetBuffer()+m_pBuffer->GetSize(), pBuffer->GetBuffer(), pBuffer->GetSize()); /* Flawfinder: ignore */
            }
            else
            {
                m_pFileObject->Seek(0, FALSE);
                bAllReadDone = TRUE;
                m_uLastError = result;
            }
	}

	HX_RELEASE(m_pBuffer);
	m_pBuffer = pMergedBuffer;

        if (HXR_OK == result)
        {
            if (m_pBuffer->GetSize() > MAX_REASONABLE_RAM_FILE_SIZE)
            {
                // /We're past the reasonable size allowed for a ram file:
                result = HXR_INVALID_METAFILE;
                m_uLastError = result;
                bAllReadDone = TRUE;
                goto cleanup;
            }

	    m_pFileObject->Read(PACKET_SIZE);
        }
    }
    else
    {
	bAllReadDone = TRUE;

	if (m_pBuffer && m_pBuffer->GetSize())
	{	    
	    m_state = Ready;
	    m_uLastError = result;
	}
	else
	{	    
	    m_pFileObject->Seek(0, FALSE);
	    result = HXR_INVALID_METAFILE;
	    m_uLastError = result;
	}
    }

cleanup:
    
    HX_RELEASE(pValidator);

    if (bAllReadDone)
    {
	m_pFFResponse->InitDone(result); 
    }
    
    return result;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileResponse::WriteDone
//  Purpose:
//	Notification interface provided by users of the IHXFileObject
//	interface. This method is called by the IHXFileObject when the
//	last write to the file is complete.
//
STDMETHODIMP CRAMFileFormat::WriteDone(HX_RESULT status)
{
    // We don't ever write, so we don't expect to get this...
    return HXR_UNEXPECTED;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileResponse::SeekDone
//  Purpose:
//	Notification interface provided by users of the IHXFileObject
//	interface. This method is called by the IHXFileObject when the
//	last seek in the file is complete.
//
STDMETHODIMP CRAMFileFormat::SeekDone(HX_RESULT status)
{
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXFileResponse::FileObjectReady
 *	Purpose:
 *	    Notification interface provided by users of the IHXFileObject
 *	    interface. This method is called by the IHXFileObject when the
 *	    requested FileObject is ready. It may return NULL with 
 *	    HX_RESULT_FAIL if the requested filename did not exist in the 
 *	    same pool.
*/
STDMETHODIMP
CRAMFileFormat::FileObjectReady
(
    HX_RESULT status,
    IHXFileObject* pFileObject
)
{
    return HXR_OK;
}


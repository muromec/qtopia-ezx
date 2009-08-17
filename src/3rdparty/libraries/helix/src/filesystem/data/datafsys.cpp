/* ***** BEGIN LICENSE BLOCK *****
*
* Source last modified: $Id:
*
* Copyright Notices:
*
* Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
*
* Patent Notices: This file may contain technology protected by one or
* more of the patents listed at www.helixcommunity.org
*
* 1.   The contents of this file, and the files included with this file,
* are protected by copyright controlled by RealNetworks and its
* licensors, and made available by RealNetworks subject to the current
* version of the RealNetworks Public Source License (the "RPSL")
* available at  * http://www.helixcommunity.org/content/rpsl unless
* you have licensed the file under the current version of the
* RealNetworks Community Source License (the "RCSL") available at
* http://www.helixcommunity.org/content/rcsl, in which case the RCSL
* will apply.  You may also obtain the license terms directly from
* RealNetworks.  You may not use this file except in compliance with
* the RPSL or, if you have a valid RCSL with RealNetworks applicable
* to this file, the RCSL.  Please see the applicable RPSL or RCSL for
* the rights, obligations and limitations governing use of the
* contents of the file.
*
* 2.  Alternatively, the contents of this file may be used under the
* terms of the GNU General Public License Version 2 (the
* "GPL") in which case the provisions of the GPL are applicable
* instead of those above.  Please note that RealNetworks and its
* licensors disclaim any implied patent license under the GPL.
* If you wish to allow use of your version of this file only under
* the terms of the GPL, and not to allow others
* to use your version of this file under the terms of either the RPSL
* or RCSL, indicate your decision by deleting Paragraph 1 above
* and replace them with the notice and other provisions required by
* the GPL. If you do not delete Paragraph 1 above, a recipient may
* use your version of this file under the terms of any one of the
* RPSL, the RCSL or the GPL.
*
* This file is part of the Helix DNA Technology.  RealNetworks is the
* developer of the Original Code and owns the copyrights in the
* portions it created.   Copying, including reproducing, storing,
* adapting or translating, any or all of this material other than
* pursuant to the license terms referred to above requires the prior
* written consent of RealNetworks and its licensors
*
* This file, and the files included with this file, is distributed
* and made available by RealNetworks on an 'AS IS' basis, WITHOUT
* WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS
* AND ITS LICENSORS HEREBY DISCLAIM  ALL SUCH WARRANTIES, INCLUDING
* WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
*
* Technology Compatibility Kit Test Suite(s) Location:
*    http://www.helixcommunity.org/content/tck
*
* Contributor(s):
*
* ***** END LICENSE BLOCK ***** */

/****************************************************************************
*
*
*  Data File system RFC 2397 plugin for RealMedia
*
*   Some applications that use URLs also have a need to embed (small)
*   media type data directly inline. This file system defines a new URL
*   scheme that would work like 'immediate addressing'.
*
*     The URLs are of the form:
*
*       data:[<mediatype>][;base64],<data>
*
*   The <mediatype> is an Internet media type specification (with
*   optional parameters.) The appearance of ";base64" means that the data
*   is encoded as base64. Without ";base64", the data (as a sequence of
*   octets) is represented using ASCII encoding for octets inside the
*   range of safe URL characters and using the standard %xx hex encoding
*   of URLs for octets outside that range.  If <mediatype> is omitted, it
*   defaults to text/plain;charset=US-ASCII.
*
*   As a shorthand, "text/plain" can be omitted but the charset parameter
*   supplied.  The "data:" URL scheme is only useful for short values. Note
*   that some applications that use URLs may impose a length limit; for
*   example, URLs embedded within <A> anchors in HTML have a length limit
*   determined by the SGML declaration for HTML [RFC1866]. The LITLEN
*   (1024) limits the number of characters which can appear in a single
*   attribute value literal, the ATTSPLEN (2100) limits the sum of all
*   lengths of all attribute value specifications which appear in a tag,
*   and the TAGLEN (2100) limits the overall length of a tag.
*   The "data" URL scheme has no relative URL forms.
*
*/

//#define INITGUID    1

#include "hxcom.h"
#include "hxtypes.h"
#include "datafsys.ver"

#include "hxcomm.h"
#include "hxplugn.h"
#include "hxfiles.h"
#include "ihxpckts.h"
#include "hxver.h"
#include "hxcore.h"
#include "hxurl.h"
#include <math.h>
#undef INITGUID

#include "hxstrutl.h"
#include "hxstring.h"
#include "datafsys.h"
#include "rtsputil.h"
#include "unkimp.h"
#include "baseobj.h"
#include "nestbuff.h"
#ifdef HELIX_FEATURE_TONE_GENERATOR
#include "hxtonetype.h"
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

const char* DataFileSystem::zm_pDescription	= "RealNetworks RFC 2397 Data Scheme File System";
const char* DataFileSystem::zm_pCopyright	= HXVER_COPYRIGHT;
const char* DataFileSystem::zm_pMoreInfoURL	= HXVER_MOREINFO;
const char* DataFileSystem::zm_pShortName	= "pn-datafsys";
#ifdef HELIX_FEATURE_TONE_GENERATOR
const char* DataFileSystem::zm_pProtocol	= "data|tone";
#else
const char* DataFileSystem::zm_pProtocol	= "data";
#endif
int g_nRefCount_datafsys = 0;

const char tokenChars[] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1,0,1,
	0,0,1,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,0,0,
	0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/****************************************************************************
*
*  Function:
*
*	DataFileSystem::HXCreateInstance()
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
HX_RESULT STDAPICALLTYPE DataFileSystem::HXCreateInstance
(
 IUnknown**  /*OUT*/	ppIUnknown
 )
{
    *ppIUnknown = (IUnknown*)(IHXPlugin*)new DataFileSystem();
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
*	DataFileSystem::CanUnload()
*
*  Purpose:
*
*	Function implemented by all plugin DLL's if it returns HXR_OK
*	then the pluginhandler can unload the DLL
*
*/
HX_RESULT
DataFileSystem::CanUnload(void)
{
    return (g_nRefCount_datafsys ? HXR_FAIL : HXR_OK);
}

/****************************************************************************
*
*  Function:
*
*      DataFileSystem::HXShutdown()
*
*  Purpose:
*
*      Function implemented by all plugin DLL's to free any *global*
*      resources. This method is called just before the DLL is unloaded.
*
*/
HX_RESULT STDAPICALLTYPE DataFileSystem::HXShutdown(void)
{
    return HXR_OK;
}


DataFileSystem::DataFileSystem()
: m_lRefCount(0)
, m_pContext(0)
{
    ++g_nRefCount_datafsys;
}

DataFileSystem::~DataFileSystem()
{
    HX_RELEASE(m_pContext);
    --g_nRefCount_datafsys;
}

/************************************************************************
*  Method:
*    IHXPlugin::InitPlugin
*  Purpose:
*    Initializes the plugin for use. This interface must always be
*    called before any other method is called. This is primarily needed
*    so that the plugin can have access to the context for creation of
*    IHXBuffers and IMalloc.
*/
STDMETHODIMP DataFileSystem::InitPlugin(IUnknown* /*IN*/ pContext)
{
    if (pContext && !m_pContext)
    {
	m_pContext = pContext;
	m_pContext->AddRef();
    }

    return HXR_OK;
}

/************************************************************************
*  Method:
*    IHXPlugin::GetPluginInfo
*  Purpose:
*    Returns the basic information about this plugin. Including:
*
*    unInterfaceCount	the number of standard RMA interfaces
*			supported by this plugin DLL.
*    pIIDList		array of IID's for standard RMA interfaces
*			supported by this plugin DLL.
*    bLoadMultiple	whether or not this plugin DLL can be loaded
*			multiple times. All File Formats must set
*			this value to TRUE.
*    pDescription	which is used in about UIs (can be NULL)
*    pCopyright	which is used in about UIs (can be NULL)
*    pMoreInfoURL	which is used in about UIs (can be NULL)
*/
STDMETHODIMP DataFileSystem::GetPluginInfo
(
 REF(HXBOOL)      /*OUT*/ bLoadMultiple,
 REF(const char*) /*OUT*/ pDescription,
 REF(const char*) /*OUT*/ pCopyright,
 REF(const char*) /*OUT*/ pMoreInfoURL,
 REF(ULONG32)     /*OUT*/ ulVersionNumber
 )
{
    bLoadMultiple = TRUE;

    pDescription    = zm_pDescription;
    pCopyright	    = zm_pCopyright;
    pMoreInfoURL    = zm_pMoreInfoURL;
    ulVersionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}


// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your
//	object.
//
STDMETHODIMP DataFileSystem::QueryInterface(REFIID riid, void** ppvObj)
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
    else if (IsEqualIID(riid, IID_IHXFileSystemObject))
    {
	AddRef();
	*ppvObj = (IHXFileSystemObject*)this;
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
STDMETHODIMP_(ULONG32) DataFileSystem::AddRef()
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
STDMETHODIMP_(ULONG32) DataFileSystem::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP DataFileSystem::GetFileSystemInfo
(
 REF(const char*) /*OUT*/ pShortName,
 REF(const char*) /*OUT*/ pProtocol
 )
{
    pShortName	= zm_pShortName;
    pProtocol	= zm_pProtocol;

    return HXR_OK;
}

STDMETHODIMP
DataFileSystem::InitFileSystem(IHXValues* options)
{
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileSystemObject::CreateFile
//  Purpose:
//	TBD
//
STDMETHODIMP DataFileSystem::CreateFile
(
 IUnknown**	/*OUT*/	ppFileObject
 )
{
    DataFileObject* pFileObj =
	new DataFileObject(m_pContext);

    if (pFileObj)
    {
	if(HXR_OK == pFileObj->QueryInterface(IID_IUnknown,
	    (void**)ppFileObject))
	    return HXR_OK;
	return HXR_UNEXPECTED;
    }
    return HXR_OUTOFMEMORY;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	DataFileSystem::CreateDir
//  Purpose:
//	TBD
//
STDMETHODIMP DataFileSystem::CreateDir
(
 IUnknown**	/*OUT*/	ppDirObject
 )
{
    return HXR_NOTIMPL;
}

DataFileObject::DataFileObject(IUnknown* pContext)
: m_lRefCount(0)
, m_pContext(pContext)
, m_pRequest(NULL)
, m_pClassFactory(NULL)
, m_pFileResponse(NULL)
, m_pDataURL(NULL)
, m_ulFilePointer(0)
, m_MediaType("text/plain;charset=US-ASCII")
{
    if (m_pContext)
    {
	m_pContext->AddRef();
    }

    m_pContext->QueryInterface(IID_IHXCommonClassFactory,
	(void **)&m_pClassFactory);
}

DataFileObject::~DataFileObject()
{
    Close();
};


// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your
//  	object.
//
STDMETHODIMP DataFileObject::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileObject))
    {
	AddRef();
	*ppvObj = (IHXFileObject*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileStat))
    {
	AddRef();
	*ppvObj = (IHXFileStat*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileExists))
    {
	AddRef();
	*ppvObj = (IHXFileExists*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXRequestHandler))
    {
	AddRef();
	*ppvObj = (IHXRequestHandler*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXFileMimeMapper))
    {
	AddRef();
	*ppvObj = (IHXFileMimeMapper*)this;
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
STDMETHODIMP_(ULONG32) DataFileObject::AddRef()
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
STDMETHODIMP_(ULONG32) DataFileObject::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

/************************************************************************
*  Method:
*	IHXFileObject::Init
*  Purpose:
*	Associates a file object with the file response object it should
*	notify of operation completness. This method should also check
*	for validity of the object (for example by opening it if it is
*	a local file).
*/
STDMETHODIMP DataFileObject::Init
(
 ULONG32		/*IN*/	ulFlags,
 IHXFileResponse*   /*IN*/	pFileResponse
 )
{
    if (!pFileResponse) return HXR_INVALID_PARAMETER;

    /* Release any previous reponses */
    if (m_pFileResponse)
    {
	m_pFileResponse->Release();
    }

    m_pFileResponse = pFileResponse;
    m_pFileResponse->AddRef();

    /* Reset our file pointer */
    m_ulFilePointer = 0;

    /*
    * We better have the IHXRequest object by now.
    */
    m_pFileResponse->InitDone(m_pRequest ? HXR_OK : HXR_FAIL);

    return HXR_OK;
}

/************************************************************************
*  Method:
*      IHXFileObject::GetFilename
*  Purpose:
*      Returns the filename (without any path information) associated
*      with a file object.
*/
STDMETHODIMP DataFileObject::GetFilename
(
 REF(const char*) /*OUT*/ pFilename
 )
{
    pFilename = NULL;
    return HXR_UNEXPECTED;
}

/************************************************************************
*  Method:
*	IHXFileObject::Close
*  Purpose:
*	Closes the file resource and releases all resources associated
*	with the object.
*/
STDMETHODIMP DataFileObject::Close()
{
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pRequest);
    HX_RELEASE(m_pDataURL);

    if (m_pFileResponse)
    {
	AddRef();
	IHXFileResponse* pResponse = m_pFileResponse;
	m_pFileResponse = 0;
	pResponse->CloseDone(HXR_OK);
	pResponse->Release();
	Release();
    }

    return HXR_OK;
}

/************************************************************************
*  Method:
*	IHXFileObject::Read
*  Purpose:
*	Reads a buffer of data of the specified length from the file
*	and asynchronously returns it to the caller via the
*	IHXFileResponse interface passed in to Init.
*/
STDMETHODIMP DataFileObject::Read(ULONG32 ulCount)
{
    HX_RESULT hResult = HXR_OK;
    IHXBuffer* pBuffer = NULL;
    CHXNestedBuffer* pNestedBuffer = NULL;

    if (!m_pDataURL || m_ulFilePointer == m_pDataURL->GetSize())
    {
	m_pFileResponse->ReadDone(HXR_FAIL, 0);
	return HXR_OK;
    }

    // Don't read off the end of the buffer
    ulCount = HX_MIN(ulCount, (m_pDataURL->GetSize() - m_ulFilePointer));

    // Create a nested buffer object
    hResult = CHXNestedBuffer::CreateObject(&pNestedBuffer);

    if (FAILED(hResult))
    {
	m_pFileResponse->ReadDone(hResult, 0);
	return HXR_OK;
    }

    // AddRef the object
    pNestedBuffer->AddRef();

    // Init the object
    hResult = pNestedBuffer->Init(m_pDataURL, m_ulFilePointer, ulCount);

    if (FAILED(hResult))
    {
	HX_RELEASE(pNestedBuffer);

	m_pFileResponse->ReadDone(hResult, 0);
	return HXR_OK;
    }

    // Query for the IHXBuffer interface
    hResult = pNestedBuffer->QueryInterface(IID_IHXBuffer, (void**)&pBuffer);
    HX_ASSERT(SUCCEEDED(hResult));

    m_ulFilePointer += ulCount;
    m_pFileResponse->ReadDone(HXR_OK, pBuffer);

    HX_RELEASE(pNestedBuffer);
    HX_RELEASE(pBuffer);

    return HXR_OK;
}

/************************************************************************
*  Method:
*	IHXFileObject::Write
*  Purpose:
*	Writes a buffer of data to the file and asynchronously notifies
*	the caller via the IHXFileResponse interface passed in to Init,
*	of the completeness of the operation.
*/
STDMETHODIMP DataFileObject::Write(IHXBuffer* pBuffer)
{
    return HXR_UNEXPECTED;
}

/************************************************************************
*  Method:
*	IHXFileObject::Seek
*  Purpose:
*	Seeks to an offset in the file and asynchronously notifies
*	the caller via the IHXFileResponse interface passed in to Init,
*	of the completeness of the operation.
*/
STDMETHODIMP DataFileObject::Seek(ULONG32 ulOffset, HXBOOL bRelative)
{
    HX_RESULT pnr = HXR_OK;

    if (!m_pDataURL)
    {
	m_pFileResponse->SeekDone(HXR_FAIL);
	return HXR_OK;
    }

    if (bRelative && ulOffset <= m_pDataURL->GetSize() - m_ulFilePointer)
    {
	m_ulFilePointer += ulOffset;
    }
    else if (!bRelative && ulOffset <= m_pDataURL->GetSize())
    {
	m_ulFilePointer = ulOffset;
    }
    else
    {
	pnr = HXR_FAIL;
    }

    m_pFileResponse->SeekDone(pnr);
    return pnr;
}

/************************************************************************
* Method:
*	IHXFileObject::Stat
* Purpose:
*	Collects information about the file that is returned to the
*	caller in an IHXStat object
*/
STDMETHODIMP DataFileObject::Stat(IHXFileStatResponse* pFileStatResponse)
{
    HX_RESULT hResult = HXR_OK;

    if (!m_pDataURL)
    {
	hResult = HXR_FAIL;
	pFileStatResponse->StatDone(HXR_FAIL, 0, 0, 0, 0, 0);
    }
    else
    {
	pFileStatResponse->StatDone(HXR_OK,
	    m_pDataURL->GetSize(),
	    0,
	    0,
	    0,
	    0);
    }

    return hResult;
}

/************************************************************************
*  Method:
*	IHXFileObject::Advise
*  Purpose:
*	To pass information to the File Object
*/
STDMETHODIMP DataFileObject::Advise(ULONG32 ulInfo)
{
    HX_RESULT retVal = HXR_OK;

    if (ulInfo == HX_FILEADVISE_NETWORKACCESS)
    {
	retVal = HXR_ADVISE_LOCAL_ACCESS;
    }

    return retVal;
}

// IHXFileExists interface
/************************************************************************
*	Method:
*	    IHXFileExists::DoesExist
*	Purpose:
*/
STDMETHODIMP DataFileObject::DoesExist
(
 const char*             pPath,
 IHXFileExistsResponse* pFileResponse
 )
{
    pFileResponse->DoesExistDone(m_pDataURL ? TRUE : FALSE);
    return HXR_OK;
}

STDMETHODIMP DataFileObject::SetRequest
(
 IHXRequest* pRequest
 )
{
    HX_RESULT hresult = HXR_OK;

    HX_RELEASE(m_pRequest);

    m_pRequest = pRequest;
    if (m_pRequest)
    {
	m_pRequest->AddRef();
    }

    const char* pURL;
    IHXValues* pHeaders = 0;
    IHXBuffer* pBuffer0 = 0;
    hresult = m_pRequest->GetURL(pURL);

    if (hresult != HXR_OK)
    {
	goto RequestError;
    }

    hresult = m_pClassFactory->CreateInstance(CLSID_IHXBuffer,
	(void**)&pBuffer0);

    hresult = ParseURL(pURL, m_MediaType, pBuffer0,m_pRequest);
    if (SUCCEEDED(hresult))
    {
	m_pDataURL = pBuffer0;
	pBuffer0 = NULL;
    }


    /*
    * Now set the Response Headers
    */

    hresult = m_pClassFactory->CreateInstance(CLSID_IHXValues,
	(void**)&pHeaders);

    if (HXR_OK != hresult)
    {
	goto RequestError;
    }

    hresult = m_pClassFactory->CreateInstance(CLSID_IHXBuffer,
	(void**)&pBuffer0);

    if (HXR_OK != hresult)
    {
	goto RequestError;
    }

    pBuffer0->Set((Byte*)"no-cache", 9);
    pHeaders->SetPropertyCString("Pragma", pBuffer0);
    HX_RELEASE(pBuffer0);

    hresult = m_pClassFactory->CreateInstance(CLSID_IHXBuffer,
	(void**)&pBuffer0);
    if (HXR_OK != hresult)
    {
	goto RequestError;
    }

    pBuffer0->Set((Byte*)(const char*)m_MediaType, m_MediaType.GetLength()+1);

    pHeaders->SetPropertyCString("Content-Type", pBuffer0);
    HX_RELEASE(pBuffer0);

    m_pRequest->SetResponseHeaders(pHeaders);

RequestError:

    HX_RELEASE(pHeaders);
    HX_RELEASE(pBuffer0);
    return hresult;
}

STDMETHODIMP DataFileObject::GetRequest
(
 REF(IHXRequest*) pRequest
 )
{
    pRequest = m_pRequest;

    if (pRequest)
    {
	pRequest->AddRef();
    }

    return HXR_OK;
}

/************************************************************************
*	Method:
*	    IHXFileMimeMapper::FindMimeType
*	Purpose:
*/
STDMETHODIMP
DataFileObject::FindMimeType
(
 const char*		    /*IN*/  pURL,
 IHXFileMimeMapperResponse* /*IN*/  pMimeMapperResponse
 )
{
    HX_RESULT status = HXR_OK;
    CHXString mimeString;

    pMimeMapperResponse->AddRef();
    status = ParseURL(pURL, mimeString, NULL,NULL);
    status = pMimeMapperResponse->MimeTypeFound(status, (const char*)mimeString);
    pMimeMapperResponse->Release();

    return status;
}

/*
*    Syntax
*
*      dataurl    := "data:" [ mediatype ] [ ";base64" ] "," data
*      mediatype  := [ type "/" subtype ] *( ";" parameter )
* 		- Matching of media type and subtype is ALWAYS
* 		  case-insensitive.
* 	type	   := token
* 	subtype	   := token
*      parameter  := attribute "=" value
* 	attribute  := token
*              - Matching of attributes is ALWAYS case-insensitive.
* 	value	   := token | quoted-string
* 	token	   := (alphanum | untspecials)?
* 	quoted-string
* 		   := """ (token | tspecials)? """
* 	tspecials  := ";" | "/" | "?" | ":" | "@" | "=" | "(" | ")" |
* 		      "<" | ">" | "," | "\" | """ | "[" | "]"
* 	untspecials:= "-" | "_" | "." | "!" | "#" | "$" | "%" | "*" | "+"
* 		      "|" | "`" | "~" | "'"
*      data       := *urlchar
* 	urlchar    := reserved | unreserved | escaped
* 	reserved   := ";" | "/" | "?" | ":" | "@" | "&" | "=" | "+" |
*                      "$" | ","
* 	unreserved := alphanum | mark
* 	mark       := "-" | "_" | "." | "!" | "~" | "*" | "'" |
*                      "(" | ")"
* 	escaped       = "%" hex hex
* 	hex	   := digit | "A" | "B" | "C" | "D" | "E" | "F" |
*                    "a" | "b" | "c" | "d" | "e" | "f"
* 	alphanum   := alpha | digit
*	alpha      := lowalpha | upalpha
* 	lowalpha   := "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" | "i" |
* 		      "j" | "k" | "l" | "m" | "n" | "o" | "p" | "q" | "r" |
* 		      "s" | "t" | "u" | "v" | "w" | "x" | "y" | "z"
* 	upalpha	   := "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I" |
* 		      "J" | "K" | "L" | "M" | "N" | "O" | "P" | "Q" | "R" |
* 		      "S" | "T" | "U" | "V" | "W" | "X" | "Y" | "Z"
* 	digit	   := "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" |
* 		      "8" | "9"
*
*   Attribute values in [RFC2045] are allowed to be either represented as
*   tokens or as quoted strings. However, within a "data" URL, the
*   "quoted-string" representation would be awkward, since the quote mark
*   is itself not a valid urlchar. For this reason, parameter values
*   should use the URL Escaped encoding instead of quoted string if the
*   parameter values contain any "tspecial".
*
*   The ";base64" extension is distinguishable from a content-type
*   parameter by the fact that it doesn't have a following "=" sign.
*/

const char DATA_SCHEME[] = "data:";
#ifdef HELIX_FEATURE_TONE_GENERATOR
const char TONE_SCHEME[] = "tone:";
#define TONE_SCHEME_SIZE (sizeof(TONE_SCHEME) - 1)
#endif
#define DATA_SCHEME_SIZE (sizeof(DATA_SCHEME) - 1)
const char BASE64_TOKEN[] = ";base64";
#define BASE64_TOKEN_SIZE (sizeof(BASE64_TOKEN) - 1)

STDMETHODIMP DataFileObject::ParseURL(const char* pURL,
				      CHXString& mimeString,
				      IHXBuffer* pBuffer,
				      IHXRequest* pRequest)
{
    HX_RESULT pnr = HXR_OK;
    UINT32 ulDataURLSize = strlen(pURL);
    char* pCurrentChar = (char*)pURL;
    HXBOOL      bBase64          = FALSE;
    UINT32 ulDataLength = 0;
    char* pMimeString = NULL;
    const char* pMimeStringStart = NULL;
    const char* pMimeStringEnd = NULL;
    HXBOOL      bHasMimeString   = TRUE;

#ifdef HELIX_FEATURE_TONE_GENERATOR
    HXBOOL bIstoneprotocol = FALSE;
    CHXURL cURL(pURL, m_pContext);
    IHXValues*  pRequestHeaders  = NULL;
    pnr = cURL.GetLastError();

    if(HXR_OK == m_pRequest->GetRequestHeaders(pRequestHeaders) && pRequestHeaders)
    {
	IHXBuffer* pToneBuffer = NULL;
	HX_RESULT hr = pRequestHeaders->GetPropertyBuffer("ToneSequence", pToneBuffer);
	if (SUCCEEDED(hr) && pToneBuffer && pToneBuffer->GetSize() > 0)
	{
	    pBuffer->SetSize(pToneBuffer->GetSize());
	    memcpy(pBuffer->GetBuffer(), pToneBuffer->GetBuffer(), pBuffer->GetSize());
	    HX_RELEASE(pToneBuffer);
	    pnr = HXR_OK;
	    goto exit;
	}
	HX_RELEASE(pToneBuffer);

	UINT32 ulNote = 0;
	UINT32 ulToneDuration = 0;
	UINT32 ulToneVolume = 0;
	if (pRequestHeaders->GetPropertyULONG32("Note",ulNote) == HXR_OK &&
	    pRequestHeaders->GetPropertyULONG32("ToneDuration",ulToneDuration) == HXR_OK &&
	    pRequestHeaders->GetPropertyULONG32("ToneVolume",ulToneVolume) == HXR_OK)
	{
	    SetSequence(ulNote,ulToneDuration,ulToneVolume,pBuffer);
	    pnr = HXR_OK;
	    goto exit;
	}
    }

#endif
    // Skip any leading whitespace
    while (*pCurrentChar &&
	*pCurrentChar < 0x20)
    {
	pCurrentChar++;
    }

#ifdef HELIX_FEATURE_TONE_GENERATOR
    // pURL must begin with data|tone scheme
    // dataurl := "data:" [ mediatype ] [ ";base64" ] "," data
    /* toneurl := "tone"://[tonename]?Note=value&ToneDuration=value&ToneVolume=value */
    // toneurl := "tone"://[testtonename]
    if(strncasecmp(pCurrentChar, TONE_SCHEME, TONE_SCHEME_SIZE) == 0)
    {
	bIstoneprotocol = TRUE;
	pCurrentChar += TONE_SCHEME_SIZE;
    }
    else
#endif
    {
	// pURL must begin with data scheme
	// dataurl    := "data:" [ mediatype ] [ ";base64" ] "," data
	if (strncasecmp(pCurrentChar, DATA_SCHEME, DATA_SCHEME_SIZE))
	{
	    pnr = HXR_FAIL;
	    goto exit;
	}
	else
	{
	    pCurrentChar += DATA_SCHEME_SIZE;
	}
    }

    // The URL may begin with "data:", "data:/", or "data://"
    if (*pCurrentChar == '/')
    {
	pCurrentChar++;
    }
    if (*pCurrentChar == '/')
    {
	pCurrentChar++;
    }

#ifdef HELIX_FEATURE_TONE_GENERATOR
    if(bIstoneprotocol)
    {
	while (*pCurrentChar == '?')
	{
	    pCurrentChar++;
	}
	bHasMimeString = FALSE;
    }
    else
#endif
    {
	if (*pCurrentChar == ';' || *pCurrentChar == ',')
	{
	    bHasMimeString = FALSE;
	}
	else
	{
	    if (strncasecmp(pCurrentChar, BASE64_TOKEN, BASE64_TOKEN_SIZE))
	    {
		pMimeStringStart = pCurrentChar;
		pMimeStringEnd = pCurrentChar;

		// parse past mediatype
		// mediatype  := [ type "/" subtype ] *( ";" parameter )

		// walk past type
		while (tokenChars[(int)*pCurrentChar])
		{
		    pCurrentChar++;
		}

		// this better be a '/'
		if (*pCurrentChar != '/')
		{
		    pnr = HXR_FAIL;
		    goto exit;
		}
		else
		{
		    pCurrentChar++;
		}

		// walk past subtype
		while (tokenChars[(int)*pCurrentChar])
		{
		    pCurrentChar++;
		}
		pMimeStringEnd = pCurrentChar;

		if (*pCurrentChar == ';')
		{
		    while (*pCurrentChar == ';')
		    {
			// check for base64
			if (strncasecmp(pCurrentChar, BASE64_TOKEN,
			    BASE64_TOKEN_SIZE))
			{
			    pCurrentChar++; // step past the ';'

			    // walk past the attribute
			    while(tokenChars[(int)*pCurrentChar])
			    {
				pCurrentChar++;
			    }

			    // this better be a '='
			    if (*pCurrentChar != '=')
			    {
				pnr = HXR_FAIL;
				goto exit;
			    }
			    else
			    {
				pCurrentChar++;
			    }

			    // walk past value
			    // XXXJEFFA need to handle quoted-string
			    while(tokenChars[(int)*pCurrentChar])
			    {
				pCurrentChar++;
			    }
			    pMimeStringEnd = pCurrentChar;
			}
			else
			{
			    bBase64 = TRUE;
			    pCurrentChar += BASE64_TOKEN_SIZE;
			    break;
			}
		    }
		}
	    }
	    else
	    {
		bBase64 = TRUE;
		bHasMimeString = FALSE;
		pCurrentChar += BASE64_TOKEN_SIZE;
	    }

	}
    }
    if (bHasMimeString)
    {
	// copy the mime string into the parameter
	ulDataLength = (pMimeStringEnd - pMimeStringStart);
	pMimeString = mimeString.GetBuffer(ulDataLength + 1);
	strncpy(pMimeString, pMimeStringStart, ulDataLength); /* Flawfinder: ignore */
	pMimeString[ulDataLength] = '\0';
	mimeString.ReleaseBuffer(ulDataLength);
    }
    else
    {
#ifdef HELIX_FEATURE_TONE_GENERATOR
	if(bIstoneprotocol)
	{
	    mimeString = "audio/x-hx-tonesequence";
	}
	else
#endif
	{
	    mimeString = "text/plain";
	}
    }

#ifdef HELIX_FEATURE_TONE_GENERATOR
    if(bIstoneprotocol)
    {
	UINT32 ulNote = 0;
	UINT32 ulToneDuration = 0;
	UINT32 ulToneVolume = 0;
	BYTE* pOutput = NULL;
	if (pCurrentChar)
	{
	    // Check for a Note parameter. If it exists, remove it
	    // from the URL
	    const char *pEnd = NULL;
	    char ptemp[10] = {0};
	    char *pNote = strstr((char *)pCurrentChar,"Note=" );
	    if(pNote)
	    {
		pNote += strlen("Note=");
		pEnd = pNote;
		while (*pEnd != '&')
		{
		    pEnd++;
		}
		SafeStrCpy(ptemp,pNote,pEnd-pNote+1);
		ulNote = (UINT32) atol((const char*)(ptemp));
	    }
	    // Check for a ToneDuration parameter. If it exists,
	    //remove it from the URL
	    pNote = NULL;
	    pNote = strstr(pCurrentChar, "ToneDuration=");
	    if (pNote)
	    {
		pNote += strlen("ToneDuration=");
		pEnd = pNote;
		while (*pEnd != '&')
		{
		    pEnd++;
		}
		SafeStrCpy(ptemp,pNote,pEnd-pNote+1);
		ulToneDuration = (UINT32) atol((const char*)(ptemp));
	    }
	    // Check for a ToneVolume parameter. If it exists,
	    //remove it from the URL
	    pNote = NULL;
	    pNote = strstr(pCurrentChar, "ToneVolume=");
	    if (pNote)
	    {
		pNote += strlen("ToneVolume=");
		pEnd = pNote;
		while (*pEnd)
		{
		    pEnd++;
		}
		SafeStrCpy(ptemp,pNote,pEnd-pNote+1);
		ulToneVolume = (UINT32) atol((const char*)(ptemp));
	    }
	}
	SetSequence(ulNote,ulToneDuration,ulToneVolume,pBuffer);
    }
    else
#endif
    {
	// should be a comma between mediatype and data
	if (*pCurrentChar != ',')
	{
	    pnr = HXR_FAIL;
	    goto exit;
	}
	else
	{
	    pCurrentChar++;
	}

	if (pBuffer != NULL)
	{
	    BYTE* pOutput = NULL;
	    INT32 nLength = 0;

	    // The rest is the data. It needs to be un-escaped and possibly
	    // un-base64 encoded.
	    if (bBase64)
	    {
		// Un-base64 encode the data
		pBuffer->SetSize(ulDataURLSize - (pCurrentChar - pURL));
		pOutput = (BYTE*)pBuffer->GetBuffer();
		nLength = BinFrom64(pCurrentChar, pBuffer->GetSize(), pOutput);

		if (nLength == -1)
		{
		    pBuffer->SetSize(0);
		}
		else
		{
		    pBuffer->SetSize(nLength);
		}
	    }
	    else
	    {
		// Un-escape the data
		pBuffer->SetSize(ulDataURLSize - (pCurrentChar - pURL));
		pOutput = (BYTE*)pBuffer->GetBuffer();
		nLength = URLUnescapeBuffer(pCurrentChar,
		    ulDataURLSize - (pCurrentChar - pURL), (char*)pOutput);

		if (nLength == -1)
		{
		    pBuffer->SetSize(0);
		}
		else
		{
		    pBuffer->SetSize(nLength);
		}
	    }
	}
    }

exit:
#ifdef HELIX_FEATURE_TONE_GENERATOR
    HX_RELEASE(pRequestHeaders);
#endif
    return pnr;
}

#ifdef HELIX_FEATURE_TONE_GENERATOR
//Generate simple tone based on specified parameters
STDMETHODIMP DataFileObject::SetSequence(const UINT32 ulNote,
					 const UINT32 ulToneDuration,const UINT32 ulToneVolume, IHXBuffer* pData)
{
    HX_RESULT pnr = HXR_OK;
    UINT32 ulIndx = 0;
    BYTE uTempo = 30;//default tempo
    BYTE uResolution = 64;//default resolution
    UINT32 ulabsdur = 0;
    BYTE uRpt = 1;
    BYTE uRemdur = 0;
    pData->SetSize(50);
    BYTE* pOutput = (BYTE*)pData->GetBuffer();
    BYTE* pStart = pOutput;
    *pStart++ = HXTONE_VERSION;	//VERSION := -2
    *pStart++ = (UINT8)1;		//version_number = 1(default)
    *pStart++ = HXTONE_TEMPO;	//TEMPO = -3
    *pStart++ = (UINT8)(30);	//tempo_modifier = 30(default)
    *pStart++ = HXTONE_RESOLUTION; //RESOLUTION = -4
    *pStart++ = (UINT8)64;		//resolution_unit = 64(default)
    *pStart++ = HXTONE_BLOCK_START;//BLOCK_START = -5
    *pStart++ = (UINT8)0;		//block_number = 0(default)
    ulabsdur = (UINT32)((8* 60 * 1000)/( uResolution * uTempo));
    *pStart++ = (UINT8)ulNote;  //Note
    *pStart++ = (UINT8)8;       //ToneDuration
    if(ulToneDuration > ulabsdur)
    {
	UINT32 ultemp = ulToneDuration/ulabsdur;

	while(ultemp > 127)
	{
	    *pStart++ = HXTONE_REPEAT;	//REPEAT -9
	    *pStart++ = 127;			//REPEAT MULTIPLIER
	    ultemp = ulToneDuration/(127 * ulabsdur);
	}
	*pStart++ = HXTONE_REPEAT;     //REPEAT -9
	*pStart++ = (UINT8)(ultemp-1); //REPEAT MULTIPLIER
    }
    *pStart++ = HXTONE_SET_VOLUME;	//SET_VOLUME = -8;
    *pStart++ = (UINT8)ulToneVolume;//volume
    *pStart++ = HXTONE_BLOCK_END;	//BLOCK_END = -6
    *pStart++ = (UINT8)0;			//block_number = 0(default)
    *pStart++ = HXTONE_PLAY_BLOCK;	//PLAY_BLOCK = -7
    *pStart++ = (UINT8)0;			//block_number = 0(default)
    pData->SetSize(pStart - pOutput);
    return pnr;
}
#endif

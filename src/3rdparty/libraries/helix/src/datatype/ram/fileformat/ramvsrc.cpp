/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ramvsrc.cpp,v 1.11 2008/01/24 12:42:02 vkathuria Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#include "hlxclib/ctype.h"
#include "hxtypes.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxformt.h"
#include "hxvsrc.h"  /*IHXFileViewSource*/
#include "chxfgbuf.h"  /*CHXFragmentedBuffer*/
#include "hxurl.h"
#include "perplex.h"
#include "growingq.h"
#include "pckunpck.h"

#include "vsrcinfo.h"
#include "ramvsrc.h"

#include "hlxclib/string.h"

// as defined in smlffpln.cpp
static const UINT32 FileChunkSize = 10000;	//XXXBAB adjust

CRAMViewSource::CRAMViewSource(IUnknown* pContext, 
				     IUnknown* pContainer)
    :	m_lRefCount(0)
    ,	m_pContext(NULL)
    ,	m_pCommonClassFactory(NULL)
    ,	m_pFileObject(NULL)
    ,	m_pViewSourceResponse(NULL)
    ,	m_type(HTML_SOURCE)
    ,	m_pBuffer(NULL)
    ,	m_pContainer(NULL)
    ,	m_pOptions(NULL)
    ,	m_bMangleLinks(TRUE)
    ,	m_pServerUrl(NULL)
    ,	m_pOurPath(NULL)
    ,	m_pDefaultView(NULL)
    ,	m_pFileName(NULL)
    ,	m_ulModDate(0)
    ,	m_ulFileSize(0)
{
    m_pContext = pContext;
    HX_ASSERT(m_pContext != NULL);
    m_pContext->AddRef();

    m_pContainer = pContainer;
    HX_ASSERT(m_pContainer != NULL);
    m_pContainer->AddRef();
};
CRAMViewSource::~CRAMViewSource()
{
    Close();
}
/* *** IUnknown methods *** */

/************************************************************************
 *  Method:
 *	IUnknown::QueryInterface
 *  Purpose:
 *	Implement this to export the interfaces supported by your 
 *	object.
 */
STDMETHODIMP CRAMViewSource::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXFileViewSource))
    {
	AddRef();
	*ppvObj = (IHXFileViewSource*)this;
	return HXR_OK;
    }
    else if (m_pContainer != NULL)
    {
        // deligate to our container
        return m_pContainer->QueryInterface(riid, ppvObj);
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = m_pContainer;
        return HXR_OK;
    }
    *ppvObj = NULL;
    // deligate to our container
    return HXR_NOINTERFACE;
}

/************************************************************************
 *  Method:
 *	IUnknown::AddRef
 *  Purpose:
 *	Everyone usually implements this the same... feel free to use
 *	this implementation.
 */
STDMETHODIMP_(ULONG32) CRAMViewSource::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/************************************************************************
 *  Method:
 *	IUnknown::Release
 *  Purpose:
 *	Everyone usually implements this the same... feel free to use
 *	this implementation.
 */
STDMETHODIMP_(ULONG32) CRAMViewSource::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/************************************************************************
 *	Method:
 *	    IHXFileViewSource::Close()
 *	Purpose:
 *	    Close down...
 */
STDMETHODIMP 
CRAMViewSource::Close()
{
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pCommonClassFactory);
    HX_RELEASE(m_pOptions);
    if (m_pFileObject)
    {
	m_pFileObject->Close();
	HX_RELEASE(m_pFileObject);
    }
    HX_RELEASE(m_pBuffer);
    HX_RELEASE(m_pContainer);
    if ( m_pViewSourceResponse != NULL )
    {
	m_pViewSourceResponse->CloseDone(HXR_OK);
	HX_RELEASE(m_pViewSourceResponse);
    }
    HX_VECTOR_DELETE(m_pServerUrl);
    HX_VECTOR_DELETE(m_pOurPath);
    HX_VECTOR_DELETE(m_pDefaultView);
    HX_VECTOR_DELETE(m_pFileName);
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXFileViewSource::InitViewSource
 *	Purpose:
 *	    Called by the user to init before a viewsource.
 */

STDMETHODIMP
CRAMViewSource::InitViewSource(
	IHXFileObject* pFileObject,
	IHXFileViewSourceResponse* pResp,
	SOURCE_TYPE sourceType,
	IHXValues* pOptions)
{
    if ( sourceType == HTML_SOURCE )
    {
	m_type = HTML_SOURCE;
    }
    else if ( sourceType == RAW_SOURCE )
    {
	m_type = RAW_SOURCE;
    }
    else
    {
	HX_ASSERT(FALSE);
	return HXR_UNEXPECTED;
    }
    HX_RELEASE(m_pCommonClassFactory);
    HX_RESULT ret = m_pContext->QueryInterface(IID_IHXCommonClassFactory, 
	(void**)&m_pCommonClassFactory);
    if ( !SUCCEEDED(ret) )
    {
	return ret;
    }

    HX_ASSERT(pResp != NULL);
    
    HX_RELEASE(m_pOptions);
    m_pOptions = pOptions;
    m_pOptions->AddRef();

    HX_RELEASE(m_pViewSourceResponse);
    m_pViewSourceResponse = pResp;
    m_pViewSourceResponse->AddRef();
    if ( m_pFileObject != NULL )
    {
	m_pFileObject->Close();
        HX_RELEASE(m_pFileObject);
    }
    m_pFileObject = pFileObject;
    HX_ASSERT(m_pFileObject != NULL);
    m_pFileObject->AddRef();

    IHXFileStat* pStat = NULL;
    if ( SUCCEEDED(m_pFileObject->QueryInterface(IID_IHXFileStat, 
	(void**)&pStat)) )
    {
	//pStat->Stat(this);
	StatDone(HXR_OK, 0, 0, 0, 0, 0);
    }
    HX_RELEASE(pStat);
    return HXR_OK;
}


STDMETHODIMP
CRAMViewSource::StatDone(HX_RESULT status, UINT32 ulSize, UINT32 ulCreationTime,
			    UINT32 ulAccessTime, UINT32 ulModificationTime, 
			    UINT32 ulMode)
{
    m_pOptions->SetPropertyULONG32("FileSize", ulSize);
    m_pOptions->SetPropertyULONG32("ModificationTime", ulModificationTime);
    IHXBuffer* pName = NULL;
    const char* p = NULL;
    m_pFileObject->GetFilename(p);
    if ( SUCCEEDED(m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, 
	(void**)&pName)) )
    {
	pName->Set((const UCHAR*)p, strlen(p) + 1);
    }
    m_pOptions->SetPropertyCString("FileName", pName);
    HX_RELEASE(pName);
    return m_pFileObject->Init(HX_FILE_READ, this);
}

/************************************************************************
 *	Method:
 *	    IHXFileViewSource::GetSource
 *	Purpose:
 *	    Called to get source html source.  Return the source
 *	through m_pViewSourceResoponse
 */
STDMETHODIMP
CRAMViewSource::GetSource()
{
    HX_ASSERT(m_pViewSourceResponse != NULL);
    HX_ASSERT(m_pFileObject != NULL);
    return m_pFileObject->Read(FileChunkSize);
}

/************************************************************************
 *  Method:
 *    IHXFileResponse::InitDone
 *  Purpose:
 *    Notification interface provided by users of the IHXFileObject
 *    interface. This method is called by the IHXFileObject when the
 *    initialization of the file is complete.
 */
STDMETHODIMP CRAMViewSource::InitDone( HX_RESULT status )
{
    HX_ASSERT(m_pViewSourceResponse != NULL);
    return m_pViewSourceResponse->InitDone(status);
}

/************************************************************************
 *  Method:
 *	IHXFileResponse::ReadDone
 *  Purpose:
 *	Notification interface provided by users of the IHXFileObject
 *	interface. This method is called by the IHXFileObject when the
 *	last read from the file is complete and a buffer is available.
 */
STDMETHODIMP CRAMViewSource::ReadDone(HX_RESULT status, 
				       IHXBuffer* pBuffer)
{
    HX_RESULT result = HXR_OK;

    if ( m_pBuffer == NULL )
    {
	IHXBuffer* pTmpBuffer = NULL;

	CreateFragmentedBufferCCF(m_pBuffer, m_pContext);
	if ( m_pBuffer == NULL )
	{
	    return HXR_OUTOFMEMORY;
	}
	
	if (HXR_OK == m_pBuffer->QueryInterface(IID_IHXBuffer, (void**)&pTmpBuffer))
	{
	    pTmpBuffer->Set(pBuffer->GetBuffer(), pBuffer->GetSize());
	    HX_RELEASE(pTmpBuffer);
	}
    }
    else // we are in a recursion...
    {
	// guard against a unneeded recursion in the case that our file size was
	// an exact multiple of FileChunkSize
	if ( pBuffer != NULL )
	{
	    m_pBuffer->Append(pBuffer, 0, pBuffer->GetSize());
	}
    }

    IHXBuffer* pTmpBuffer = NULL;
    if (m_pBuffer)
    {
	m_pBuffer->QueryInterface(IID_IHXBuffer, (void**)&pTmpBuffer);
    }

    if ( pBuffer->GetSize() == FileChunkSize )
    {
	m_pFileObject->Read(FileChunkSize);	//XXXBAB recursive!!!
    }
    else
    {
	HX_ASSERT(m_pViewSourceResponse != NULL);
	if ( SUCCEEDED(result) )
	{
	    if ( m_type == HTML_SOURCE )
	    {
		IHXBuffer* pTheStuff = NULL;
		if ( SUCCEEDED(BuildSource(pTheStuff)) )
		{
    		    result = m_pViewSourceResponse->SourceReady(HXR_OK,
			pTheStuff);
		}
		else
		{
		    result = m_pViewSourceResponse->SourceReady(HXR_FAIL, NULL);
		}
		HX_RELEASE(pTheStuff);
	    }
	    else
	    {
		result = m_pViewSourceResponse->SourceReady(HXR_OK, pTmpBuffer);
	    }
	}
	else
	{
	    result = m_pViewSourceResponse->SourceReady(HXR_FAIL, NULL);
	}
    }

    HX_RELEASE(pTmpBuffer);
    return result;
}

/************************************************************************
 *  Method:
 *	IHXFileResponse::WriteDone
 *  Purpose:
 *	Notification interface provided by users of the IHXFileObject
 *	interface. This method is called by the IHXFileObject when the
 *	last write to the file is complete.
 */
STDMETHODIMP CRAMViewSource::WriteDone(HX_RESULT status)
{
    // We don't ever write, so we don't expect to get this...
    return HXR_UNEXPECTED;
}

/************************************************************************
 *  Method:
 *	IHXFileResponse::SeekDone
 *  Purpose:
 *	Notification interface provided by users of the IHXFileObject
 *	interface. This method is called by the IHXFileObject when the
 *	last seek in the file is complete.
 */
STDMETHODIMP CRAMViewSource::SeekDone(HX_RESULT status)
{
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *	IHXFileResponse::CloseDone
 *  Purpose:
 *	Notification interface provided by users of the IHXFileObject
 *	interface. This method is called by the IHXFileObject when the
 *	close of the file is complete.
 */
STDMETHODIMP CRAMViewSource::CloseDone(HX_RESULT status)
{
    return HXR_OK;
}

HX_RESULT
CRAMViewSource::BuildSource(REF(IHXBuffer*) pOutput)
{
    CollectOptions();

    IHXBuffer* pTmpBuffer = NULL;
    m_pBuffer->QueryInterface(IID_IHXBuffer, (void**)&pTmpBuffer);

    CBigByteGrowingQueue queue(pTmpBuffer->GetSize()*2);
    const char* tokenset = "\n\r";
    const char* charset = " \r\n\t";

    //output += (const char*) m_pBuffer->GetBuffer();

    char* buffer = new char[pTmpBuffer->GetSize()+1];
    memcpy(buffer, pTmpBuffer->GetBuffer(), pTmpBuffer->GetSize()); /* Flawfinder: ignore */
    buffer[pTmpBuffer->GetSize()] = '\0';

    PushHeader(&queue);
    queue.EnQueue(z_pEndLine);

    //queue->EnQueue("<pre><!--  Begin Source  -->\n");
    char* pLine = strtok(buffer, tokenset);
    while (pLine)
    {
	size_t pos = strspn(pLine,charset);
	pLine += pos;

	if (*pLine == '#' || *pLine == '/')
	{
	    queue.EnQueue(pLine);
	    queue.EnQueue(z_pEndLine);
	    pLine = strtok(NULL, tokenset);
	    continue;
	}

	if (strchr(pLine, ':'))
	{
	    HXBOOL bHREF = PushOpenningHREF(pLine, &queue);
	    PushMangledDisplayedPath(pLine, &queue);
	    if (bHREF)
	    {
		queue.EnQueue(tag_END_HREF);
	    }
	    queue.EnQueue(z_pEndLine);

	    pLine = strtok(NULL, tokenset);
	    continue;
	}
	else
	{
	    queue.EnQueue(pLine);
	    queue.EnQueue(z_pEndLine);
	    pLine = strtok(NULL, tokenset);
	    continue;
	}
    }
    
    HX_RELEASE(pTmpBuffer);
    //queue.EnQueue("\n<!--  End Source  --></pre>\n");

    m_pCommonClassFactory->CreateInstance(IID_IHXBuffer, (void**)&pOutput);
    if (!pOutput)
    {
	return HXR_OUTOFMEMORY;
    }
    if (FAILED(pOutput->SetSize(queue.GetQueuedItemCount())))
    {
	return HXR_OUTOFMEMORY;
    }
    
    UCHAR* chr = pOutput->GetBuffer();
    queue.DeQueue(chr, queue.GetQueuedItemCount());
    //pOutput->GetBuffer()[m_pBuffer->GetSize()] = '\0';
    HX_VECTOR_DELETE(buffer);
    return HXR_OK;
}

HXBOOL
CRAMViewSource::PushOpenningHREF(const char* pPositionPointer, CBigByteGrowingQueue* pQueue)
{
    UINT32 ulLen = strlen(pPositionPointer);

    if (strncmp(pPositionPointer, "rtsp://", 7) == 0 || strncmp(pPositionPointer, "pnm://", 6) == 0)
    {
        UINT32 ulCount = 7;
        if (strncmp(pPositionPointer, "pnm://", 6) == 0) ulCount = 6;
	const char* p = pPositionPointer;
	const char* pEnd = p + ulLen;
	p += ulCount;
	// move p to the end of server name find the first : or /
	while ( *p != ':' && *p != '/' && ++p != pEnd) {} ;

	pQueue->EnQueue(tag_BEGIN_HREF);

	// replacing rtsp with http
	
	//XXXJHUG  -- If the server in m_pServerUrl is the same as the 
	// server we are about queue we will use m_pServerUrl
	// instead of the default port and mountpoint.
	
	// mover past http://
	char* pBeginServer = m_pServerUrl + 7;
	
	UINT32 ulServerLen = 0;
	char* pServerPort = strchr(pBeginServer, ':');
	// m_pServerURL will always have a port.
	HX_ASSERT(pServerPort);
	if ( pServerPort )
	{
	    ulServerLen = pServerPort - pBeginServer;
	}
	// 7 for rtsp://
	UINT32 ulXMLServerLen = p - pPositionPointer - ulCount;
	if ( ulServerLen == ulXMLServerLen &&
	    strncmp(pBeginServer, pPositionPointer + ulCount, ulServerLen) == 0 )
	{
	    // use m_pServerURL
	    pQueue->EnQueue(m_pServerUrl);
	}
	else
	{
	    // use the Default Port
	    pQueue->EnQueue("http");
        UINT32 ulOffset = 4;
        if (ulCount == 6) ulOffset = 3;
	    pQueue->EnQueue((void*)(pPositionPointer + ulOffset), p - pPositionPointer - ulOffset);
	    pQueue->EnQueue(m_pDefaultView);
	}

	while ( *p != '/' && ++p != pEnd) {} ;
	char* pParam = GetParameter(p, pEnd - p, FALSE);

	pQueue->EnQueue("?");
	pQueue->EnQueue(pParam);
	HX_VECTOR_DELETE(pParam);
	pQueue->EnQueue("\">");
    }
    else if ( strnchr(pPositionPointer, ':', HX_MIN(6, ulLen)) )
    {
	if ( strncmp(pPositionPointer, "pnm://", 6) != 0 &&
	    (strncmp(m_pServerUrl, "http://localhost", sizeof("http://localhost") - 1) == 0 ||
	    strncmp(m_pServerUrl, "http://127.0.0.1", sizeof("http://127.0.0.1") - 1) == 0) )
	{
	    pQueue->EnQueue(tag_BEGIN_HREF);
	    pQueue->EnQueue(m_pServerUrl);
	    pQueue->EnQueue("?");
	    char* pParam = GetParameter(pPositionPointer, ulLen, TRUE);
	    pQueue->EnQueue(pParam);
	    HX_VECTOR_DELETE(pParam);
	    pQueue->EnQueue("\">");
	}
	else
	{
	    return FALSE;
	}
    }
    else
    {
	pQueue->EnQueue(tag_BEGIN_HREF);
	pQueue->EnQueue(m_pServerUrl);
	pQueue->EnQueue("?");
	char* pParam = GetParameter(pPositionPointer, ulLen, FALSE);

	pQueue->EnQueue(pParam);
	HX_VECTOR_DELETE(pParam);
	pQueue->EnQueue("\">");
    }
    return TRUE;
}

UINT32
CRAMViewSource::PushMangledDisplayedPath(const char* pIn, 
					    CBigByteGrowingQueue* pQueue)
{
    UINT32 ulLen = strlen(pIn);
    UINT32 ulPos = 0;
    if ( m_bMangleLinks )
    {
	const char* pProtocolEnd = strnstr(pIn, "://", ulLen);

	if ( pProtocolEnd )  // we have protocol
	{
	    pProtocolEnd += 2; // only push one slash
	    UINT32 ulLenOfProtocol = pProtocolEnd - pIn;
	    pQueue->EnQueue((void*)pIn, ulLenOfProtocol);
	    pIn += ulLenOfProtocol;
	    ulPos += ulLenOfProtocol;
	}

	// find last '/'
	while ( strnchr(pIn, '/', ulLen - ulPos) ) 
	{
	    UINT32 temp = ulPos;
	    temp += strnchr(pIn, '/', ulLen - ulPos) 
		    - pIn + 1;
	    pIn  = strnchr(pIn, '/', ulLen - ulPos) 
		    + 1;
	    ulPos = temp;
	    
	}

	pQueue->EnQueue("/.../");
    }
    // else we just do the whole thing...
    pQueue->EnQueue((void*)pIn, ulLen - ulPos);
    return ulLen;
}


char*
CRAMViewSource::GetParameter(const char* pPositionPointer, UINT32 ulNameLen, 
			       HXBOOL bFullPath)
{
    // allocate longest possible string
    char* pReturnBuffer = new char[(strlen(m_pOurPath) + ulNameLen) + 10]; 
    //  2 for a possible starting '/' and a null terminator
    // 4 for the starting src=
    // 4 for extra bytes in case we have to pad the buffer when we encypt it.
    strcpy(pReturnBuffer, "src="); /* Flawfinder: ignore */

    char* pLinkPath = pReturnBuffer + 4;
    

    // if it starts with '/' then it is a full path
    if ( *pPositionPointer == '/' || bFullPath )
    {
	strncpy(pLinkPath, pPositionPointer, ulNameLen); /* Flawfinder: ignore */
	pLinkPath[ulNameLen] = '\0';
    }
    // if it is alpha it is simply a file name
    else if ( isalnum(*pPositionPointer) )
    {
	// 1 for "/"
	UINT32 len = strlen(m_pOurPath) + ulNameLen + 1;
	strcpy(pLinkPath, m_pOurPath); /* Flawfinder: ignore */
	strcat(pLinkPath, "/"); /* Flawfinder: ignore */
	strncat(pLinkPath, pPositionPointer, ulNameLen); /* Flawfinder: ignore */
	pLinkPath[len] = '\0';
    }
    else if ( !strncmp(pPositionPointer, "./", 2) )
    {
	// -1 for .
	UINT32 len = strlen(m_pOurPath) + ulNameLen - 1;
	strcpy(pLinkPath, m_pOurPath); /* Flawfinder: ignore */
	pPositionPointer += 1;
	strncat(pLinkPath, pPositionPointer, ulNameLen - 1); /* Flawfinder: ignore */
	pLinkPath[len] = '\0';
    }
    else if ( !strncmp(pPositionPointer, "../", 3 ) )
    {
	int count = 0;

	// copy m_pOurPath into pLinkPath ourselves cause we need to be at
	// the end anyway.
	const char* pSrc = m_pOurPath;
	char* pCurrentEndOfPath = pLinkPath;
	const char* pRelativePath = pPositionPointer;
	
	// Walk to take care of any ../ that might be in the path...
	char* pDest = pCurrentEndOfPath;
	while ( *pSrc )
	{
	    while ( *pSrc == '.' && *(pSrc + 1) == '.' && *(pSrc + 2) == '/' )
	    {
		--pDest;
		while ( *(pDest-1) != '/' && (pDest-1) >= pLinkPath )
		{
		    --pDest;
		}
		pSrc += 3;
	    }
	    *pDest = *pSrc;
	    ++pDest;
	    ++pSrc;
	}
	*pDest = '\0';

	pCurrentEndOfPath += strlen(pCurrentEndOfPath);

	// back up a directory off of the file path for
	// every ../ we find
	while (!strncmp(pRelativePath, "../", 3 ))
	{
	    // we found a ../ so back up a directory on the path,
	    // walk backwards to the previous / and set it to null
	    while (*pCurrentEndOfPath != '/' && 
		pCurrentEndOfPath >= pLinkPath)
	    {
		pCurrentEndOfPath--;
	    }
	    // watch to make sure we don't have more ../ than directories
	    if ( pCurrentEndOfPath < pLinkPath)
	    {
		++pCurrentEndOfPath;
	    }
	    *pCurrentEndOfPath = '\0';
	    pRelativePath +=3;
	}

        UINT32 len = (pCurrentEndOfPath - pLinkPath) + 
	    ulNameLen - (pRelativePath - pPositionPointer) + 1;
	
	// back 1 off of pRelativePath so we get the / that's there.
	strncat(pLinkPath, pRelativePath - 1, /* Flawfinder: ignore */
	    ulNameLen - (pRelativePath - pPositionPointer) + 1);
	pLinkPath[len] = '\0';
    }
    else
    {
	HX_ASSERT(FALSE);
	pLinkPath = '\0';
    }

    char* pParam = EncryptParameter(pReturnBuffer);
    HX_VECTOR_DELETE(pReturnBuffer);
    return pParam;
}

char*
CRAMViewSource::EncryptParameter(char* pPath)
{
    UINT32 FinalLen;			    // length of encoded data
    UINT32 Offset = strlen(pPath);
    UINT32 nAlign = Offset % sizeof(ULONG32);
    if (nAlign > 0)
    {
	for (; nAlign < sizeof(ULONG32); nAlign++)
	{
	    pPath[Offset++] = 0;
	}
    }
    FinalLen = (Offset) * Perplex_PER_ULONG32 / sizeof(ULONG32);	
    // calc size of the outout (Perplexed) buffer.
    // alloc mem for final perplexed buffer
    // Add one to length 'cause low level perplex adds
    // a '\0' to the resulting string

    char* output = new char[FinalLen+1];

    CHXPerplex::DumpToPerplex((char*)output, FinalLen+1, (UCHAR*) pPath, Offset);
    return output;
}


HX_RESULT
CRAMViewSource::PushHeader(CBigByteGrowingQueue* queue)
{
    queue->EnQueue(z_pStream);
    queue->EnQueue(z_pRAMName);
    queue->EnQueue(z_pEndLine);

    queue->EnQueue(z_pFileName);
    queue->EnQueue(m_pFileName);
    queue->EnQueue(z_pEndLine);
    
    QueueModificationTime(queue, m_ulModDate);

    QueueFileSize(queue, m_ulFileSize);

    return HXR_OK;
}

HX_RESULT
CRAMViewSource::CollectOptions()
{
    IHXBuffer* pViewURL = NULL;
    IHXBuffer* pCurrentPath = NULL;
    IHXBuffer* pRemoteView = NULL;
    IHXBuffer* pFileName = NULL;
    UINT32 ulMangle = 0;
    UINT32 ulStyles = 0;
    if ( FAILED(m_pOptions->GetPropertyCString("ViewSourceURL", pViewURL)) ||
	 FAILED(m_pOptions->GetPropertyCString("CurrentPath", pCurrentPath)) ||
	 FAILED(m_pOptions->GetPropertyULONG32("HidePaths", ulMangle)) ||
	 FAILED(m_pOptions->GetPropertyCString("RemoteViewSourceURL", pRemoteView)) ||
	 FAILED(m_pOptions->GetPropertyCString("FileName", pFileName)) ||
	 FAILED(m_pOptions->GetPropertyULONG32("ModificationTime", m_ulModDate)) ||
	 FAILED(m_pOptions->GetPropertyULONG32("FileSize", m_ulFileSize)) )
    {
	HX_ASSERT(FALSE);
	// return HXR_INVALID_PARAMETER;
    }

    m_bMangleLinks = ulMangle ? TRUE : FALSE;

    m_pServerUrl = new char[pViewURL->GetSize()+1];
    strcpy(m_pServerUrl, (char*)pViewURL->GetBuffer()); /* Flawfinder: ignore */

    m_pOurPath = new char[pCurrentPath->GetSize()+1];
    strcpy(m_pOurPath, (char*)pCurrentPath->GetBuffer()); /* Flawfinder: ignore */

    m_pDefaultView = new char[pRemoteView->GetSize()+1];
    strcpy(m_pDefaultView, (char*)pRemoteView->GetBuffer()); /* Flawfinder: ignore */

    m_pFileName = new char[pFileName->GetSize()+1];
    strcpy(m_pFileName, (char*)pFileName->GetBuffer()); /* Flawfinder: ignore */

    HX_RELEASE(pRemoteView);
    HX_RELEASE(pViewURL);
    HX_RELEASE(pCurrentPath);
    HX_RELEASE(pFileName);

    return HXR_OK;
}


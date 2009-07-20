/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: validatr.cpp,v 1.14 2006/11/30 17:36:26 ping Exp $
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
#include "hlxclib/stdio.h"
#include "hxtypes.h"
#include "hxresult.h"
#include "hxassert.h"
#include "hxcomm.h"
#include "hxplugn.h"
#include "hxfiles.h"
#include "hxmeta.h"
#include "hxcore.h"
#include "hxpends.h"
#include "timeval.h"
#include "pq.h"
#include "hxstring.h"
#include "hxslist.h"
#include "hxrquest.h"
#include "hxbuffer.h"
#include "chxpckts.h"
#include "dbcs.h"
#include "hxmarsh.h"
#include "hxmime.h"
#include "hxstrutl.h"
#include "validatr.h"
#include "hxxfile.h"
#include "rmfftype.h"
#include "pckunpck.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define MAX_URL_STRING		4096

HXValidator::HXValidator(IUnknown* pContext)
	: m_lRefCount (0)
	, m_pContext (NULL)
	, m_bRefresh (TRUE)
{
    if (pContext)
    {
	m_pContext = pContext;
	m_pContext->AddRef();
    }
}

HXValidator::~HXValidator()
{
    CHXSimpleList::Iterator i;

    // cleanup the protocol list
    i = m_ProtocolList.Begin();
    for (; i != m_ProtocolList.End(); ++i)
    {
	CHXString* pProtocol = (CHXString*) (*i);
	HX_DELETE(pProtocol);
    }
    m_ProtocolList.RemoveAll();

    HX_RELEASE(m_pContext);
}

STDMETHODIMP
HXValidator::QueryInterface(REFIID riid, void**ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXValidator), (IHXValidator*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXValidator*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
HXValidator::AddRef()
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
STDMETHODIMP_(ULONG32) 
HXValidator::Release()
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
 *	    IHXValidator::ValidateProtocol
 *	Purpose:
 *	    Find out if the protocol is valid
 *	    
 *
 */
STDMETHODIMP_(HXBOOL)
HXValidator::ValidateProtocol(char* pProtocol)
{
    HXBOOL	bResult = FALSE;
    CHXString*	pValue = NULL;
    CHXSimpleList::Iterator i;

    if (!pProtocol)
    {
	goto cleanup;
    }

    if (m_bRefresh)
    {
	BuildProtocolList();
	m_bRefresh = FALSE;
    }

    i = m_ProtocolList.Begin();
    for (; i != m_ProtocolList.End(); ++i)
    {
	pValue = (CHXString*) (*i);

	if (strcasecmp(pProtocol, (const char*)(*pValue)) == 0)
	{
	    bResult = TRUE;
	    break;
	}
    }

cleanup:

    return bResult;
}

/************************************************************************
 *	Method:
 *	    IHXValidator::ValidateMetaFile
 *	Purpose:
 *	    Find out if it is a meta file
 *	    
 *
 */
STDMETHODIMP
HXValidator::ValidateMetaFile(IHXRequest* pRequest,
			       IHXBuffer* pData)
{
    HX_RESULT	hr = HXR_OK;    
    HXBOOL	bIsRAM = FALSE;
    HXBOOL	bTrackFound = FALSE;
    HXBOOL	bHeaderToBeSet = FALSE;
    UINT32	ulFileType = 0;
    UINT32	ulSize = 0;
    UINT32	ulMajorVersion = 0;
    int		i = 0; 
    int		j = 0;
    int		iLen = 0;
    int		iSize = 0;
    char*	pMimeType = NULL;
    char*	pFileExt = NULL;
    char*	pProtocol = NULL;
    char*	pCursor = NULL;
    char*	pLine = NULL;
    char*	pContent = NULL;
    char*	pTemp = NULL;
    char*	pURL = NULL;
    const char* pConstTemp = NULL;
    const char* charset = " \r\n\t";
    const char* tokenset = "\n\r";    
    UINT32	ulProtocol = 0;
    CHXString*	pString = NULL;
    IHXBuffer*	pValue = NULL;
    IHXValues*	pResponseHeaders = NULL;
    
    if (pRequest)
    {
	pRequest->AddRef();
    }

    if (pData)
    {
        pData->AddRef();
    }
    else
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    if (pRequest)
    {
        pRequest->GetURL(pConstTemp);
        if (HXXFile::IsPlusURL(pConstTemp))
        {
	    hr = HXR_FAILED;
	    goto cleanup;
        }
    }

    ulSize = pData->GetSize() + 1;
    pContent = new char[ulSize];
    memset(pContent, 0, ulSize);
    memcpy(pContent, (char*)pData->GetBuffer(), pData->GetSize()); /* Flawfinder: ignore */

    // if the first FOURCC is either ".RA " or ".RMF"
    if (pData->GetSize() >= 4)
    {
	ulFileType = (UINT32) getlong((UCHAR*) pContent);

	const char* pContentType = NULL;
	switch (ulFileType)
	{
	    case RM_HEADER_OBJECT:
	    case RMS_HEADER_OBJECT:
	    case RA_FILE_MAGIC_NUMBER:
	    {
		pContentType = REALAUDIO_MIME_TYPE;
	    }
	    break;

	    case REDHAT_PACKAGE_MAGIC_NUMBER:
	    {
		pContentType = REDHAT_PACKAGE_MIME_TYPE;
	    }
	    break;
	}
	
	if (pContentType)
	{
	    if (pRequest)
	    {
		pRequest->GetResponseHeaders(pResponseHeaders);
		if (!pResponseHeaders)
		{
		    CreateValuesCCF(pResponseHeaders, m_pContext);
		    bHeaderToBeSet = TRUE;
		}

		if (HXR_OK !=
		    pResponseHeaders->GetPropertyCString("Content-Type",
							 pValue) ||
		    !pValue)
		{
		    if (HXR_OK == CreateAndSetBufferCCF(pValue, (UCHAR*)pContentType,
							::strlen(pContentType) + 1, m_pContext))
		    {
			pResponseHeaders->SetPropertyCString("Content-Type",
							     pValue);

			if (bHeaderToBeSet)
			{
			    pRequest->SetResponseHeaders(pResponseHeaders);
			}
		    }
		}
	    }
	    
	    hr = HXR_FAILED;
	    goto cleanup;
	}
    }

    // retreive the MimeType and FileExt
    if (pRequest)
    {
	if (HXR_OK == pRequest->GetResponseHeaders(pResponseHeaders) &&
	    pResponseHeaders)
	{
	    if (HXR_OK == pResponseHeaders->GetPropertyCString("Content-Type", pValue) &&
		pValue)
	    {
		pMimeType = (char*)pValue->GetBuffer();
	    }
	}

	pTemp = (char*)pConstTemp;

	if (pTemp)
	{
	    pURL = new char[strlen(pTemp) + 1];
	    strcpy(pURL, pTemp); /* Flawfinder: ignore */

	    pTemp = strrchr(pURL, '?');

	    if (pTemp)
	    {
		*pTemp = '\0';
	    }

	    pFileExt = strrchr(pURL, '.');
	}
    }

    // determine whether it is a RAM file
    if ((pMimeType &&
	 ((strcasecmp(pMimeType, "audio/x-pn-realaudio") == 0) ||
	  (strcasecmp(pMimeType, "audio/x-pn-realaudio-plugin") == 0))) ||
	(pFileExt &&
	 ((strcasecmp(pFileExt, ".ram") == 0) ||
	  (strcasecmp(pFileExt, ".rpm") == 0) ||
	  (strcasecmp(pFileExt, ".rmm") == 0))))
    {
	bIsRAM = TRUE;

        iLen = pData->GetSize();
	char* ramFile = new char[iLen+1];
    
	while( i <= iLen && !bTrackFound)
	{
/*
 * XXXJHUG  12/07/00 change to allow unlimitted length ram files.
 *		     - we no longer need to check to see if the url
 *		       is longer than a fixed value.
	    if (j >= MAX_URL_STRING)
	    {
		// exceeds the max. length of URL
		// skip the whole line
		while (pContent[i] != '\n' && pContent[i] != '\r' &&
		       pContent[i] != 0 && i < iLen)
		{
		    i++;
		}

		memset(pUrl, 0, MAX_URL_STRING);
		j = 0;
		i++;

		continue;
	    }
*/
	    ramFile[j] = pContent[i]; 
	    
	    // Look for line terminators.
	    if ( ramFile[j] == '\n' || ramFile[j] == '\r' || ramFile[j] == 0 || i == iLen)
	    {
		ramFile[j] = 0;

		pString = new CHXString(ramFile);

		pString->TrimLeft();
		pString->TrimRight();

		iSize = pString->GetLength();
		if (iSize)
		{
		    memset(ramFile, 0, iLen+1);
		    SafeStrCpy(ramFile,  pString->GetBuffer(iSize), iLen+1);
		    
		    // A URL must have at least 4 chars. for protocol. This will
		    // take care of lines with junk or just CR on them.
		    
		    // detect RAM3.0 syntax
		    if (strncasecmp(ramFile, HX_RAM30_START_TAG, HX_RAM30_START_TAGSIZE) == 0)
		    {
			ulMajorVersion = 3;
		    }
		    else if (strncasecmp(ramFile, HX_RAM30_END_TAG, HX_RAM30_END_TAGSIZE) == 0)
		    {
			HX_DELETE(pString);
			break;
		    }
		    // detect RAM2.0 syntax
		    else if (strncasecmp(ramFile, HX_RAM20_START_TAG, HX_RAM20_START_TAGSIZE) == 0)
		    {
			ulMajorVersion = 2;
		    }
		    else if (strncasecmp(ramFile, HX_RAM20_END_TAG, HX_RAM20_END_TAGSIZE) == 0)
		    {
			HX_DELETE(pString);
			break;
		    }
		    // handle "--stop--" tag in 6.0
		    else if (strncasecmp(ramFile, "--stop--", 8) == 0)
		    {
			HX_DELETE(pString);
			break;
		    }
		    else 
		    {
			if (ulMajorVersion == 2 || ulMajorVersion == 3)
			{
			    if (strncasecmp(ramFile, HX_RAM_ENTRY_TAG, HX_RAM_ENTRY_TAGSIZE) == 0 && 
				(iSize >= (HX_RAM_ENTRY_TAGSIZE + 4)))
			    {
				CHXString* pStringAfterTag = new CHXString(ramFile+2);

				pStringAfterTag->TrimLeft();
				pStringAfterTag->TrimRight();
    
				iSize = pStringAfterTag->GetLength();
				if (iSize)
				{				
				    memset(ramFile, 0, iLen);
				    SafeStrCpy(ramFile,  pStringAfterTag->GetBuffer(iSize), iLen+1);
				}
				HX_DELETE(pStringAfterTag);
			    }
			    else
			    {
				memset(ramFile, 0, iLen);
			    }			    
			}
			    
			pCursor = strstr(ramFile, ":");
			if (pCursor)			
			{
			    ulProtocol = pCursor - ramFile;

			    if (ulProtocol > 0)
			    {
				pProtocol = new char[ulProtocol+1];
				memset(pProtocol, 0, ulProtocol+1);
				strncpy(pProtocol, ramFile, ulProtocol); /* Flawfinder: ignore */

				if (ValidateProtocol(pProtocol))
				{
				    bTrackFound = TRUE;
				}

				HX_VECTOR_DELETE(pProtocol);
			    }
			}
		    }
		}

		j = 0;
		HX_DELETE(pString);
	    }
	    else
	    {	
		j++; 
	    }

	    i++;
	}
	HX_VECTOR_DELETE(ramFile);

	if (bTrackFound)
	{	    
	    hr = HXR_OK;
	    goto cleanup;
	}
	else
	{
	    hr = HXR_INVALID_METAFILE;
	    goto cleanup;
	}
    }
    // determine if it is a RAM
    else
    {
	// Trim off any leading spaces/EOLs/tabs before adding on the pnm protocol...
	pLine = strtok(pContent, tokenset);

	while (pLine != NULL)
	{	
	    size_t pos = strspn(pLine,charset);
	    pLine += pos;

	    // handle RAM3.0 syntax
	    if (strncasecmp(pLine, HX_RAM30_START_TAG, HX_RAM30_START_TAGSIZE) == 0)
	    {
		ulMajorVersion = 3;
		pLine = strtok(NULL, tokenset);
		continue;
	    }
	    // handle RAM2.0 syntax
	    else if (strncasecmp(pLine, HX_RAM20_START_TAG, HX_RAM20_START_TAGSIZE) == 0)
	    {
		ulMajorVersion = 2;
		pLine = strtok(NULL, tokenset);
		continue;
	    }

	    if (ulMajorVersion == 2 && ulMajorVersion == 3)
	    {
		if (strncasecmp(pLine, HX_RAM_ENTRY_TAG, HX_RAM_ENTRY_TAGSIZE) == 0 &&
		    (pCursor = strstr(pLine, ":")))
		{
		    pLine += 2;		    
		    // trim off the leading spaces
		    while (pLine[0] == ' ')
		    {
			pLine++;
		    }
		}
		else
		{
		    pLine = strtok(NULL, tokenset);
		    continue;
		}			    
	    }
	    else
	    {
		pString = new CHXString(pLine);

		pString->TrimLeft();
		pString->TrimRight();

		if ((pString->GetLength() == 0)	    ||
		    (strncasecmp(pLine, "//", 2) == 0)  ||
		    (strncasecmp(pLine, "#", 1) == 0))
		{
		    // comments, skip to next line
		    pLine = strtok(NULL, tokenset);
		    HX_DELETE(pString);
		    continue;
		}
		HX_DELETE(pString);

		pCursor = strstr(pLine, ":");
		if (!pCursor)
		{
		    bIsRAM = FALSE;
		    break;
		}
	    }

	    ulProtocol = pCursor - pLine;
	    if (ulProtocol > 0)
	    {
		pProtocol = new char[ulProtocol+1];
		memset(pProtocol, 0, ulProtocol+1);
		strncpy(pProtocol, pLine, ulProtocol); /* Flawfinder: ignore */

		bIsRAM = ValidateProtocol(pProtocol);

		HX_VECTOR_DELETE(pProtocol);
		break;
	    }
	    else
	    {
		bIsRAM = FALSE;
		break;
	    }
	}

	if (bIsRAM)
	{
	    hr = HXR_OK;
	    goto cleanup;
	}
	else
	{
	    hr = HXR_FAILED;
	    goto cleanup;
	}
    }
    
cleanup:

    // set the RAMVersion to response header, it will be retrieved by the 
    // RAM file format
    if (pRequest && bIsRAM)
    {
	if (!pResponseHeaders)
	{
	    CreateValuesCCF(pResponseHeaders, m_pContext);
	    bHeaderToBeSet = TRUE;
	}

	UINT32 ulPersistentVersion = HX_ENCODE_PROD_VERSION(ulMajorVersion, 0, 0, 0);
	pResponseHeaders->SetPropertyULONG32("PersistentVersion", ulPersistentVersion);

	if (bHeaderToBeSet)
	{
	    pRequest->SetResponseHeaders(pResponseHeaders); 
	}
    }

    HX_VECTOR_DELETE(pURL);
    HX_VECTOR_DELETE(pContent);

    HX_RELEASE(pResponseHeaders);
    HX_RELEASE(pValue);
    HX_RELEASE(pRequest);
    HX_RELEASE(pData);

    return hr;
}

HX_RESULT
HXValidator::BuildProtocolList(void)
{
    HX_RESULT	hr = HXR_OK;
    UINT32	j = 0;
    UINT32	ulPlugins = 0;
    IHXValues*	pValues = NULL;
    IHXBuffer*	pBuffer = NULL;
    IHXCommonClassFactory* pCommonClassFactory = NULL;
    IHXPluginQuery* pPluginQuery = NULL;
    IHXPluginGroupEnumerator* pPluginEnum = NULL;
    CHXString*	pProtocol = NULL;
    CHXSimpleList::Iterator i;

    if (HXR_OK != m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&pCommonClassFactory))
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    // reset the list
    i = m_ProtocolList.Begin();
    for (; i != m_ProtocolList.End(); ++i)
    {
	pProtocol = (CHXString*) (*i);
	HX_DELETE(pProtocol);
    }
    m_ProtocolList.RemoveAll();

    // we always add "pnm", "rtsp" and "lice"
    pProtocol = new CHXString("pnm");
    m_ProtocolList.AddTail(pProtocol);

    pProtocol = new CHXString("rtsp");
    m_ProtocolList.AddTail(pProtocol);

    pProtocol = new CHXString("lice");
    m_ProtocolList.AddTail(pProtocol);

    // collect protocol info. from the pluginhandler
    if (HXR_OK != pCommonClassFactory->CreateInstance(IID_IHXPluginGroupEnumerator, 
							(void**)&pPluginEnum))
    {
	goto cleanup;
    }

    if (HXR_OK != pPluginEnum->QueryInterface(IID_IHXPluginQuery, (void**)&pPluginQuery))
    {
	goto cleanup;
    }

    if (HXR_OK != pPluginQuery->GetNumPluginsGivenGroup(IID_IHXFileSystemObject, ulPlugins))
    {
	goto cleanup;
    }

    for (j = 0; j < ulPlugins; j++)
    {
	HX_RELEASE(pBuffer);
	HX_RELEASE(pValues);

	if (HXR_OK != pPluginQuery->GetPluginInfo(IID_IHXFileSystemObject, j, pValues) ||
	    !pValues)
	{
	    continue;
	}

	if (HXR_OK != pValues->GetPropertyCString("FileProtocol", pBuffer) ||
	    !pBuffer)
	{
	    continue;
	}

	pProtocol = new CHXString((const char*) pBuffer->GetBuffer());
	int nPos = pProtocol->Find('|');
	while (nPos > 0)
	{
	    CHXString* pTemp = new CHXString(pProtocol->Left(nPos));
	    *pProtocol = pProtocol->Mid(nPos + 1);
	    m_ProtocolList.AddTail(pTemp);
	    nPos = pProtocol->Find('|');
	}
	m_ProtocolList.AddTail(pProtocol);
	if (pProtocol->CompareNoCase("lice") == 0)
	{
	    CHXString *pAnother = new CHXString("rnba");
	    m_ProtocolList.AddTail(pAnother);
	}
    }

cleanup:

    HX_RELEASE(pBuffer);
    HX_RELEASE(pValues);
    HX_RELEASE(pPluginQuery);
    HX_RELEASE(pPluginEnum);
    HX_RELEASE(pCommonClassFactory);

    return hr;
}

void
HXValidator::RefreshProtocols(void)
{
    m_bRefresh = TRUE;
}

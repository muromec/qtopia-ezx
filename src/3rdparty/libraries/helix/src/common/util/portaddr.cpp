/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: portaddr.cpp,v 1.13 2005/05/03 16:14:29 albertofloyd Exp $
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

//XXXLCM ipv6 here

#include "hxcom.h"

#if defined (_UNIX) || defined (_WIN16)
#include <stdlib.h>
#endif

#include "hxcomm.h"

#include "hxslist.h"
#include "hxstring.h"
#include "hxstrutl.h"
#include "hxbuffer.h"
#include "hxprefs.h"
#include "netbyte.h"
#include "portaddr.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

HX_RESULT
ReadUDPPorts(IHXBuffer* pBuffer, REF(CHXSimpleList*) pUDPPortList)
{
    HX_RESULT	hr = HXR_OK;

    int		i = 0;
    UINT16	iPort = 0;
    HXBOOL	bNewEntry = TRUE;
    char*	pUDPPorts = NULL;
    char*	pToken = NULL;
    UDP_PORTS*	pEntry = NULL;
    UDP_PORTS*	pUDPPort = NULL;
    CHXString	szToken;

    if (!pBuffer || !pUDPPortList)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    pUDPPorts = (char*)pBuffer->GetBuffer();

    pToken = strtok(pUDPPorts, ",");
    while (pToken)
    {
	bNewEntry = TRUE;
	szToken = pToken;
	pEntry = new UDP_PORTS;

	// trim off the spaces
	szToken.TrimLeft();
	szToken.TrimRight();

	i = szToken.Find('-');

	if (-1 == i)
	{
	    // single port
	    iPort = atoi(szToken);

	    CHXSimpleList::Iterator lIterator = pUDPPortList->Begin();

	    for (; lIterator != pUDPPortList->End(); ++lIterator)
	    {
		 pUDPPort = (UDP_PORTS*) (*lIterator);

		 if (pUDPPort->uFrom == iPort + 1)
		 {
		     pUDPPort->uFrom = iPort;
		     bNewEntry = FALSE;
		 }
		 else if (pUDPPort->uTo + 1 == iPort)
		 {
		     pUDPPort->uTo = iPort;
		     bNewEntry = FALSE;
		 }
	    }

	    if (bNewEntry)
	    {
		pEntry->uFrom = atoi(szToken);
		pEntry->uTo = atoi(szToken);

		pUDPPortList->AddTail((void*)pEntry);
	    }
	    else
	    {
		HX_DELETE(pEntry);
	    }
	}
	// the entry is a range
	else
	{
	    CHXString szFrom = szToken.Left(i);
	    CHXString szTo = szToken.Right(szToken.GetLength() - (i + 1));

	    szFrom.TrimRight();
	    szTo.TrimLeft();

	    pEntry->uFrom = atoi(szFrom);
	    pEntry->uTo = atoi(szTo);

	    pUDPPortList->AddTail((void*)pEntry);
	}

	pToken = strtok(NULL, ",");
    }

cleanup:

    return hr;
}

HX_RESULT   
ReadListEntries(IHXBuffer* pValue, REF(CHXSimpleList*) pEntryList)
{
    HX_RESULT		hr = HXR_OK;
    char*		pszValue = NULL;
    char*		pszToken = NULL;
    CHXString*		pToken = NULL;
    CommonEntry*	pEntry = NULL;

    if (!pValue)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    // collect the # of entries with/without wildcard
    pszValue = new char[pValue->GetSize() + 1];
    SafeStrCpy(pszValue, (char*)pValue->GetBuffer(), pValue->GetSize());

    pszToken = strtok(pszValue, ",\n");
    while (pszToken)
    {
	pToken = new CHXString(pszToken);

	// trim off the spaces
	pToken->TrimLeft();
	pToken->TrimRight();

	if (IsValidSubnetEntry(*pToken))
	{
	    pEntry = new SubnetEntry(*pToken);
	}
	else if (IsValidWildcardEntry(*pToken))
	{
	    pEntry = new WideCardEntry(*pToken);
	}
	else
	{
	    pEntry = new NonWideCardEntry(*pToken);
	}

	if (!pEntryList)
	{
	    pEntryList = new CHXSimpleList();
	}

	pEntryList->AddTail(pEntry);

	HX_DELETE(pToken);
	pszToken = strtok(NULL, ",\n");
    }

cleanup:

    HX_VECTOR_DELETE(pszValue);
    
    return hr;
}

HXBOOL	    
IsValidWildcardEntry(const char* pszValue)
{
    HXBOOL	bResult = FALSE;
    CHXString	string = pszValue;

    // there are only 3 cases
    // *.XXX
    // XXX.*
    // XXX.*.XXX
    if (!string.Left(2).Compare("*.")	||
	!string.Right(2).Compare(".*")	||
	string.Find(".*.") != -1)
    {
	if (string.Find('*') == string.ReverseFind('*'))
	{
	    bResult = TRUE;
	}
    }

    return bResult;
}

HXBOOL
IsValidSubnetEntry(const char* pszValue)
{
    HXBOOL	bResult = FALSE;
    CHXString	inString = pszValue;
    CHXString	outString;

    // XXX.XXX.XXX.XXX:XXX.XXX.XXX.XXX
    if (2 == inString.CountFields(':'))
    {
	outString = inString.NthField(':', 1);
	if (4 != outString.CountFields('.'))
	{
	    goto cleanup;
	}

	outString = inString.NthField(':', 2);
	if (4 != outString.CountFields('.'))
	{
	    goto cleanup;
	}

	bResult = TRUE;
    }

cleanup:

    return bResult;
}

CommonEntry::CommonEntry(const char* pszValue)
{
    UINT8   size = 0;

    if (pszValue)
    {
	size = strlen(pszValue) + 1;
	
	m_pszValue = new char[size];
	memset(m_pszValue, 0, size);

	strcpy(m_pszValue, pszValue); /* Flawfinder: ignore */
    }

    m_nChunks = 0;
    m_pChunks = 0;
}

CommonEntry::~CommonEntry()
{
    int i = 0;
    for (i = 0; i < m_nChunks; i++)
    {
	HX_VECTOR_DELETE(m_pChunks[i]);
    }

    HX_VECTOR_DELETE(m_pChunks);
    HX_VECTOR_DELETE(m_pszValue);
}

NonWideCardEntry::NonWideCardEntry(const char* pszValue)
		: CommonEntry(pszValue)
{
}

HXBOOL
NonWideCardEntry::IsEqual(const char* pszValue)
{
    HXBOOL bResult = FALSE;

    if (!m_pszValue || !pszValue)
    {
	goto cleanup;
    }

    if (0 == strcasecmp(m_pszValue, pszValue))
    {
	bResult = TRUE;
    }

cleanup:

    return bResult;
}

WideCardEntry::WideCardEntry(const char* pszValue)
	     : CommonEntry(pszValue)
{
    char*	pHead = NULL;
    char*	pTail = NULL;
    UINT8	i = 0;
    UINT8	nLength = 0;
    CHXString	string;

    if (pszValue)
    {
	// caculate the # of chunks
	pHead = pTail = (char*)pszValue;
	string = pszValue;

	m_pChunks = new char*[string.CountFields('.')];

	nLength = (UINT8)(string.GetLength());
	for (i = 0; i < nLength; i++)
	{
	    if (*pTail == '.')
	    {
		m_pChunks[m_nChunks] = new char[(pTail - pHead) + 1];
		strncpy(m_pChunks[m_nChunks], pHead, pTail - pHead); /* Flawfinder: ignore */
		m_pChunks[m_nChunks][pTail - pHead] = '\0';

		m_nChunks++;
		
		pHead = pTail + 1;
	    }

	    pTail++;
	}

	m_pChunks[m_nChunks] = new char[(pTail - pHead) + 1];
	strncpy(m_pChunks[m_nChunks], pHead, pTail - pHead); /* Flawfinder: ignore */
	m_pChunks[m_nChunks][pTail - pHead] = '\0';

	m_nChunks++;
    }
}

HXBOOL
WideCardEntry::IsEqual(const char* pszValue)
{
    HXBOOL	    bResult = FALSE;
    int 	    i = 0;
    int 	    j = 0;
    WideCardEntry*  pTargetEntry = NULL;

    if (!m_pszValue || !pszValue)
    {
	goto cleanup;
    }

    pTargetEntry = new WideCardEntry(pszValue);

    if (pTargetEntry->m_nChunks == 1)
    {
        goto cleanup;
    }

    bResult = TRUE;
	    
    // start from left to right till either hit '*' or done with pTargetInfo
    for (i = 0, j = 0; strcasecmp(m_pChunks[i], "*") != 0 && j < pTargetEntry->m_nChunks; i++, j++)
    {
	if (strcasecmp(m_pChunks[i], pTargetEntry->m_pChunks[j]) != 0)
	{
	    bResult = FALSE;
	    break;
	}
    }

    // start from right to left
    for (i = m_nChunks-1, j = pTargetEntry->m_nChunks-1; 
	 bResult && strcasecmp(m_pChunks[i], "*") != 0 && j >= 0; i--, j--)
    {
	if (strcasecmp(m_pChunks[i], pTargetEntry->m_pChunks[j]) != 0)
	{
	    bResult = FALSE;
	    break;
	}
    }

cleanup:

    HX_DELETE(pTargetEntry);

    return bResult;
}

SubnetEntry::SubnetEntry(const char* pszValue)
	   : CommonEntry(pszValue)
{
    UINT32  ulIAddress = 0;
    char*   pColumn = NULL;

    if (pszValue)
    {
	pColumn = (char*) strchr(pszValue, ':');

	*pColumn = '\0';
	pColumn++;

	ulIAddress = HXinet_addr(pszValue);
	m_ulSubnet = DwToHost(ulIAddress);

	ulIAddress = HXinet_addr(pColumn);
	m_ulSubnetMask = DwToHost(ulIAddress);
    }
}

HXBOOL
SubnetEntry::IsEqual(const char* pszValue)
{
    HXBOOL    bResult = FALSE;
    UINT32  ulIAddress = 0;
    UINT32  ulHostAddress = 0;
    
    if(IsNumericAddr(pszValue, strlen(pszValue)))
    {
	ulIAddress = HXinet_addr(pszValue);
	ulHostAddress = DwToHost(ulIAddress);

	if (m_ulSubnet == (ulHostAddress & m_ulSubnetMask))
	{
	    bResult = TRUE;
	}
    }

    return bResult;
}
 
HXSubnetManager::HXSubnetManager(IUnknown* pContext)
	: m_pContext(pContext)
	, m_pEntryList(NULL)
	, m_pPreferences(NULL)
	, m_pPrevSubnet(NULL)
{
    if (m_pContext)
    {
	m_pContext->AddRef();
	m_pContext->QueryInterface(IID_IHXPreferences, (void**)&m_pPreferences);

	Initialize();
    }
}

HXSubnetManager::~HXSubnetManager()
{
    Close();
}

void    
HXSubnetManager::Initialize(void)
{
    IHXBuffer* pBuffer = NULL;

    if (m_pPreferences && m_pPreferences->ReadPref("SubnetList", pBuffer) == HXR_OK)
    {
	if (!m_pPrevSubnet ||
	    strcasecmp((const char*)m_pPrevSubnet->GetBuffer(), (const char*)pBuffer->GetBuffer()))
	{
	    ResetEntryList();

	    ReadListEntries(pBuffer, m_pEntryList);

	    HX_RELEASE(m_pPrevSubnet);

	    m_pPrevSubnet = pBuffer;
	    m_pPrevSubnet->AddRef();
	}
    }
    HX_RELEASE(pBuffer);
}

HXBOOL
HXSubnetManager::IsSubnet(const char* pszDomain)
{
    HXBOOL		bResult = FALSE;
    CommonEntry*	pEntry = NULL;
    CHXSimpleList::Iterator lIterator;

    if (m_pEntryList)
    {
	lIterator = m_pEntryList->Begin();	
	for (; lIterator != m_pEntryList->End(); ++lIterator)
	{
	    pEntry = (CommonEntry*)(*lIterator);

	    if (pEntry->IsEqual(pszDomain))
	    {
		bResult = TRUE;
		break;
	    }
	}
    }

    return bResult;
}

void
HXSubnetManager::Close(void)
{
    ResetEntryList();
    HX_DELETE(m_pEntryList);

    HX_RELEASE(m_pPrevSubnet);
    HX_RELEASE(m_pPreferences);
    HX_RELEASE(m_pContext);
}

void
HXSubnetManager::ResetEntryList(void)
{
    while (m_pEntryList && m_pEntryList->GetCount() > 0)
    {
	CommonEntry* pEntry = (CommonEntry*)m_pEntryList->RemoveHead();
	HX_DELETE(pEntry);
    }
}

#if defined(HELIX_FEATURE_PROXYMGR)
HXProxyManager::HXProxyManager(void)
	: m_lRefCount(0)
	, m_pContext(NULL)
	, m_pPreferences(NULL)
	, m_pEntryList(NULL)
	, m_pPrevNoProxyFor(NULL)
{
}

HXProxyManager::~HXProxyManager()
{
    Close();
}

STDMETHODIMP
HXProxyManager::QueryInterface(REFIID riid, void**ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), this },
		{ GET_IIDHANDLE(IID_IHXProxyManager), (IHXProxyManager*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
HXProxyManager::AddRef()
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
HXProxyManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
HXProxyManager::Initialize(IUnknown* pContext)
{
    HX_RESULT		hr = HXR_OK;
    IHXBuffer*		pBuffer = NULL;

    if (!m_pPreferences)
    {
        HX_RELEASE(m_pContext);

	m_pContext = pContext;
	if (!m_pContext)
	{
	    hr = HXR_FAILED;
	    goto cleanup;
	}
	m_pContext->AddRef();

	if (HXR_OK != m_pContext->QueryInterface(IID_IHXPreferences, (void**)&m_pPreferences))
	{
	    m_pPreferences = NULL;
	}
    }

    if (m_pPreferences && m_pPreferences->ReadPref("NoProxyFor", pBuffer) == HXR_OK)
    {
	if (!m_pPrevNoProxyFor ||
	    strcasecmp((const char*)m_pPrevNoProxyFor->GetBuffer(), (const char*)pBuffer->GetBuffer()))
	{
	    ResetEntryList();

	    ReadListEntries(pBuffer, m_pEntryList);

	    HX_RELEASE(m_pPrevNoProxyFor);

	    m_pPrevNoProxyFor = pBuffer;
	    m_pPrevNoProxyFor->AddRef();
	}
    }
    HX_RELEASE(pBuffer);

cleanup:

    return hr;
}

STDMETHODIMP_(HXBOOL)
HXProxyManager::IsExemptionHost(char* pszDomain)
{
    HXBOOL		bResult = FALSE;
    CommonEntry*	pEntry = NULL;
    CHXSimpleList::Iterator lIterator;

    if (m_pEntryList)
    {
	lIterator = m_pEntryList->Begin();	
	for (; lIterator != m_pEntryList->End(); ++lIterator)
	{
	    pEntry = (CommonEntry*)(*lIterator);

	    if (pEntry->IsEqual(pszDomain))
	    {
		bResult = TRUE;
		break;
	    }
	}
    }

    return bResult;
}

void
HXProxyManager::Close()
{
    ResetEntryList();
    HX_DELETE(m_pEntryList);

    HX_RELEASE(m_pPrevNoProxyFor);
    HX_RELEASE(m_pPreferences);
    HX_RELEASE(m_pContext);
}

void
HXProxyManager::ResetEntryList(void)
{
    while (m_pEntryList && m_pEntryList->GetCount() > 0)
    {
	CommonEntry* pEntry = (CommonEntry*)m_pEntryList->RemoveHead();
	HX_DELETE(pEntry);
    }
}
#endif /* HELIX_FEATURE_PROXYMGR */    

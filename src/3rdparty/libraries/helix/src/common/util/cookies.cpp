/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cookies.cpp,v 1.59 2008/11/03 18:45:43 ping Exp $
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
 * ***** END LICENSE BLOCK ***** */

#include "hxcom.h"
#include "hxtypes.h"
#include "hlxclib/string.h"
#include "safestring.h"

#if defined(HELIX_FEATURE_SQLITE)
// common/import/sqlite
#include "sqlite3.h"
#endif /* HELIX_FEATURE_SQLITE */

#ifdef _MACINTOSH
#ifdef _CARBON
#ifdef _MAC_MACHO
#include <sys/stat.h>
#else
#include <stat.h>
#endif
#else /* _CARBON */
#include <stat.mac.h>
#endif /* _CARBON */
#else /* _MACINTOSH */
#include "hlxclib/sys/types.h"
#include "hlxclib/sys/stat.h"
#endif /* _MACINTOSH */

#include "hlxclib/time.h"

#if defined(_AIX)
#include <ctype.h>
#endif

#ifdef _WINDOWS
#include "hlxclib/windows.h"
#include <wininet.h>
#include "hlxclib/io.h"
#include <shlobj.h>
typedef HRESULT (HXEXPORT_PTR FPSHGetFolderPath) (HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPTSTR lpszPath);  
#endif /* _WINDOWS */

#if defined(_CARBON) || defined(_MAC_MACHO)
#include "hxver.h" // for HXVER_SDK_PRODUCT
#endif

#if defined(_MAC_UNIX) && defined(HELIX_FEATURE_SAFARI_COOKIES)
#include "platform/mac/WebKitPrefs.h"
#endif

#if defined(_CARBON)
#include "cfwrappers.h"
#endif

#include "hxresult.h"
#include "hxslist.h"
#include "chxpckts.h"
#include "hxstrutl.h"
#include "dbcs.h"
#include "dllpath.h"
#include "chxinifreader.h"
#include "hxprefs.h"
#include "hxprefutil.h"
#include "hxthread.h"
#include "ihxcookies.h"
#include "cookhlpr.h"
#include "ihxcookies2.h"
#include "hxdate.h"
#include "cookies.h"
#include "md5.h"
#include "filespecutils.h"
#include "pckunpck.h"
#include "hxtlogutil.h"

#ifdef _UNIX
#include <sys/types.h>
#include <sys/stat.h>
#endif

#if defined (_UNIX) && !defined(_SUN) && !defined(_HPUX) && !defined(_IRIX) && !defined(_OSF1)
#include <sys/file.h>
#endif /* UNIX */

#define IE_COOKIE_URLSTRING     "http://%s%s/"
#define RM_COOKIE_CAPTION       "# Helix Cookie File\n# http://www.ietf.org/rfc/rfc2109.txt\n# This is a generated file!  Do not edit.\n\n"

#if defined(_UNIX) && !defined(_MAC_UNIX)
#  define RM_COOKIE_FILE_NAME "Cookies_6_0"
#else
#  define RM_COOKIE_FILE_NAME "cookies.txt"
#endif

/* We should really define it in a common header file */
#if defined (_WINDOWS ) || defined (WIN32) || defined(_SYMBIAN)
#define OS_SEPARATOR_CHAR	'\\'
#define OS_SEPARATOR_STRING	"\\"
#elif defined (_UNIX) || defined (_OPENWAVE) || defined (_BREW)
#define OS_SEPARATOR_CHAR	'/'
#define OS_SEPARATOR_STRING	"/"
#elif defined (_MACINTOSH)
#define OS_SEPARATOR_CHAR	':'
#define OS_SEPARATOR_STRING	":"
#endif /* defined (_WINDOWS ) || defined (WIN32) */

HXCookies::HXCookies(IUnknown* pContext, HXBOOL bMemoryOnly)
	: m_lRefCount(0)	
	, m_pContext(NULL)
        , m_ulExternalCookies(0xFF)
	, m_bInitialized(FALSE)
	, m_bSaveCookies(FALSE)
	, m_bMemoryOnly(bMemoryOnly)
	, m_pNSCookiesPath(NULL)
        , m_pFFTextCookiesPath(NULL)
        , m_pFFSQLiteCookiesPath(NULL)
        , m_pFFProfilePath(NULL)
	, m_pRMCookiesPath(NULL)
	, m_lastRMCookiesFileModification(0)
        , m_lastFFTextCookiesFileModification(0)
        , m_lastFFSQLiteCookiesFileModification(0)
	, m_pNSCookies(NULL)
        , m_pFFCookies(NULL)
	, m_pRMCookies(NULL)
        , m_pSFCookies(NULL)
	, m_pPreferences(NULL)
	, m_pCookiesHelper(NULL)
#ifdef _WINDOWS
	, m_hLib(NULL)
	, m_pLock(NULL)
	, _pInternetSetCookie(NULL)
	, _pInternetGetCookie(NULL)
#elif _UNIX
	, m_fileID(0)
#endif /* _WINDOWS */
{    
    if (pContext)
    {
	m_pContext = pContext;
	m_pContext->AddRef();
    }
}

HXCookies::~HXCookies()
{
    Close();
}

STDMETHODIMP
HXCookies::QueryInterface(REFIID riid, void**ppvObj)
{
    QInterfaceList qiList[] =
    {
	{ GET_IIDHANDLE(IID_IUnknown), this },
	{ GET_IIDHANDLE(IID_IHXCookies), (IHXCookies*) this },
	{ GET_IIDHANDLE(IID_IHXCookies2), (IHXCookies2*) this },
	{ GET_IIDHANDLE(IID_IHXCookies3), (IHXCookies3*) this }
    };	

    if (QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXCookiesHelper) && m_pCookiesHelper)
    {
	return m_pCookiesHelper->QueryInterface(riid, ppvObj);
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
STDMETHODIMP_(ULONG32) 
HXCookies::AddRef()
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
HXCookies::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT    
HXCookies::Initialize(void)
{
    HX_RESULT	hr = HXR_OK;
    
    if (!IsCookieEnabled())
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    if (m_pContext)
    {
        UINT32      ulExternalCookies = 0xFF;

        if (SUCCEEDED(ReadPrefUINT32(m_pContext, "ExternalCookies", ulExternalCookies)))
        {
            m_ulExternalCookies = ulExternalCookies;
        }

        m_pContext->QueryInterface(IID_IHXPreferences, (void**)&m_pPreferences);
        m_pContext->QueryInterface(IID_IHXCookiesHelper, (void**) &m_pCookiesHelper);
    }

    if (!m_pCookiesHelper)
    {
        HXCookiesHelper* pHelper = new HXCookiesHelper(m_pContext);
        if (pHelper)
	{
            pHelper->QueryInterface(IID_IHXCookiesHelper, (void**) &m_pCookiesHelper);
	}
    }

    if (!m_bMemoryOnly)
    {
        PrepareCookies();
    }

    m_bInitialized = TRUE;

cleanup:

    return hr;
}

void	
HXCookies::Close(void)
{
    if (m_bSaveCookies && !m_bMemoryOnly)
    {
	SyncRMCookies(FALSE);
	SaveCookies();
    }

    ResetCookies(m_pNSCookies);
    ResetCookies(m_pFFCookies);
    ResetCookies(m_pRMCookies);
    ResetCookies(m_pSFCookies);

    HX_DELETE(m_pNSCookies);
    HX_DELETE(m_pFFCookies);
    HX_DELETE(m_pRMCookies);
    HX_DELETE(m_pSFCookies);

    HX_VECTOR_DELETE(m_pNSCookiesPath);
    HX_VECTOR_DELETE(m_pFFTextCookiesPath);
    HX_VECTOR_DELETE(m_pFFSQLiteCookiesPath);
    HX_VECTOR_DELETE(m_pFFProfilePath);
    HX_VECTOR_DELETE(m_pRMCookiesPath);

    HX_RELEASE(m_pPreferences);
    HX_RELEASE(m_pCookiesHelper);
    HX_RELEASE(m_pContext);

    m_bInitialized = FALSE;

#ifdef _WINDOWS
    if (m_hLib)
    {
	FreeLibrary(m_hLib);
	m_hLib = NULL;
    }

    HX_RELEASE(m_pLock);
#endif /* _WINDOWS */
}

STDMETHODIMP 
HXCookies::SetCookies(const char* pHost, const char* pPath, IHXBuffer* pCookies)
{
    HX_RESULT	    hr = HXR_OK;
    int		    host_length = 0;
    int		    domain_length = 0;
    char*           pPathNew = NULL;
    char*	    pURL = NULL;
    char*	    path_from_header = NULL;
    char*	    domain_from_header = NULL;
    char*	    name_from_header = NULL;
    char*	    cookie_from_header = NULL;
    char*	    dot = NULL;
    char*           domain_from_header_without_leading_dot = NULL;
    time_t	    expires=0;
    IHXBuffer*	    pBuffer = NULL;
    IHXValues*	    pValues = NULL;    
    CookieStruct*   pNewCookie = NULL;
#if defined(_WINDOWS)
    int		    path_length = 0;
#endif
    
    if (!IsCookieEnabled())
    {
	goto cleanup;
    }

    if (!m_bInitialized)
    {
	hr = Initialize();

	if (HXR_OK != hr)
	{
	    goto cleanup;
	}
    }

    if (!m_pCookiesHelper || !pCookies || !pHost)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    if (HXR_OK != m_pCookiesHelper->Pack(pCookies, pValues))
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    if (HXR_OK == pValues->GetPropertyBuffer("path", pBuffer) && pBuffer)
    {
	::StrAllocCopy(path_from_header, (char*)pBuffer->GetBuffer());
    }
    HX_RELEASE(pBuffer);

    if (HXR_OK == pValues->GetPropertyBuffer("domain", pBuffer) && pBuffer)
    {
        // verify that this host has the authority to set for
        // this domain.   We do this by making sure that the
        // host is in the domain
        // We also require that a domain have at least two
        // periods to prevent domains of the form ".com"
        // and ".edu"
	//
	// also make sure that there is more stuff after
	// the second dot to prevent ".com."
	::StrAllocCopy(domain_from_header, (char*)pBuffer->GetBuffer());

        dot = strchr(domain_from_header, '.');
    
	if(dot)
	{
            dot = strchr(dot+1, '.');
	}

	if(!dot || *(dot+1) == '\0') 
	{
	    // did not pass two dot test. FAIL
	    hr = HXR_FAILED;
	    goto cleanup;
	}

        // Trimming off the leading dot prior to checking domain for
        // eligibility of setting cookies, i.e.
        // foo.bar.com is allowed to set cookies for .foo.bar.com but NOT
        // xxx.foo.bar.com
        if (domain_from_header[0] == '.')
        {
            domain_from_header_without_leading_dot = &domain_from_header[1];
        }
        else
        {
            domain_from_header_without_leading_dot = domain_from_header;
        }

	domain_length = strlen(domain_from_header_without_leading_dot);
	host_length = strlen(pHost);

	// check to see if the host is in the domain
	if(domain_length > host_length || 
	   strcasecmp(domain_from_header_without_leading_dot, &pHost[host_length-domain_length]))
	{
	    hr = HXR_FAILED;		
	    goto cleanup;
	}
    }
    HX_RELEASE(pBuffer);

    if (HXR_OK == pValues->GetPropertyBuffer("name", pBuffer) && pBuffer)
    {
	::StrAllocCopy(name_from_header, (char*)pBuffer->GetBuffer());
    }
    HX_RELEASE(pBuffer);

    if (HXR_OK == pValues->GetPropertyBuffer("value", pBuffer) && pBuffer)
    {
	::StrAllocCopy(cookie_from_header, (char*)pBuffer->GetBuffer());
    }
    HX_RELEASE(pBuffer);

    if (HXR_OK == pValues->GetPropertyBuffer("expires", pBuffer) && pBuffer)
    {
	expires = ::ParseDate((char*)pBuffer->GetBuffer());
    }
    HX_RELEASE(pBuffer);

    if(!path_from_header && pPath)
    {
        // strip down everything after the last slash
        // to get the path.
	CHXString strPath = pPath;
#if 0   // Disable it, this prevents the caller from passing
        // "/foo" as pPath.
        // It should be caller's responsibility to ensure pPath
        // is properly formatted.
	INT32 nIndex = strPath.ReverseFind('/');

	if (nIndex != -1)
	{
	    strPath = strPath.Left(nIndex + 1);
	}
#endif
	::StrAllocCopy(path_from_header, (char*) (const char*) strPath);
    }

    if(!domain_from_header && pHost)
    {
	::StrAllocCopy(domain_from_header, (char*)pHost); 
    }
    
    m_bSaveCookies = TRUE;

    // We have decided we are going to insert a cookie into the list
    // Get the cookie lock so that we can munge on the list

    pNewCookie = CheckForPrevCookie(path_from_header, 
				    domain_from_header, 
				    name_from_header);

    if(pNewCookie) 
    {
	HX_DELETE(pNewCookie->pCookieValue);
	HX_DELETE(pNewCookie->pCookieName);
	HX_DELETE(pNewCookie->pPath);
        HX_DELETE(pNewCookie->pHost);
        
	pNewCookie->pCookieValue = new CHXString(cookie_from_header);
	pNewCookie->pCookieName = new CHXString(name_from_header);
        pNewCookie->pPath = new CHXString(path_from_header);
        pNewCookie->pHost = new CHXString(domain_from_header);
        pNewCookie->expires = expires;
        pNewCookie->bIsDomain = TRUE;
	pNewCookie->bMemoryOnly = TRUE;
    }	
    else 
    {
	pNewCookie = new CookieStruct;

        // copy
	pNewCookie->pCookieValue = new CHXString(cookie_from_header);
	pNewCookie->pCookieName = new CHXString(name_from_header);
        pNewCookie->pPath = new CHXString(path_from_header);
        pNewCookie->pHost = new CHXString(domain_from_header);
        pNewCookie->expires = expires;
        pNewCookie->bIsDomain = TRUE;
	pNewCookie->bMemoryOnly = TRUE;

	if (!m_pRMCookies)
	{
	    m_pRMCookies = new CHXSimpleList();
	}

	hr = AddCookie(pNewCookie, m_pRMCookies);
    }

    HXLOGL2(HXLOG_COOK, "SetCookies:\n\t\t\t\tHost:%s\n\t\t\t\tPath:%s\n\t\t\t\tCookieName:%s\n\t\t\t\tCookieValue:%s\n\t\t\t\tExpires:%lu\n\t\t\t\tIsDomain:%lu\n\t\t\t\tMemoryOnly:%lu\n",
                         (const char*)*(pNewCookie->pHost),
                         (const char*)*(pNewCookie->pPath),
                         (const char*)*(pNewCookie->pCookieName),
                         (const char*)*(pNewCookie->pCookieValue),
                         pNewCookie->expires,
                         pNewCookie->bIsDomain,
                         pNewCookie->bMemoryOnly);

#ifdef _WINDOWS
    if (HX_EXTERNAL_COOKIES_IE & m_ulExternalCookies)
    {
        if (!m_bMemoryOnly)
        {
            if (!_pInternetSetCookie)
	    {
	        goto cleanup;
	    }

	    host_length = strlen(pHost);

            FixPath(pPath, pPathNew);
            if (pPathNew)
            {
	        path_length = strlen(pPathNew);
            }

	    pURL = new char[host_length + path_length + strlen(IE_COOKIE_URLSTRING)];
	    sprintf(pURL, IE_COOKIE_URLSTRING, pHost, pPathNew); /* Flawfinder: ignore */

	    _pInternetSetCookie(pURL, NULL, (const char*)pCookies->GetBuffer());
        }
    }
#endif /* _WINDOWS */
    
#if defined(_MAC_UNIX) && defined(HELIX_FEATURE_SAFARI_COOKIES)
    if (HX_EXTERNAL_COOKIES_SF & m_ulExternalCookies)
    {
        if (!m_bMemoryOnly)
        {
            (void)SetWebKitCookieInformation(pNewCookie);
        }
    }
#endif

cleanup:

    HX_RELEASE(pBuffer);
    HX_RELEASE(pValues);

    HX_VECTOR_DELETE(pPathNew);
    HX_VECTOR_DELETE(pURL);
    HX_VECTOR_DELETE(path_from_header);
    HX_VECTOR_DELETE(domain_from_header);
    HX_VECTOR_DELETE(name_from_header);
    HX_VECTOR_DELETE(cookie_from_header);
    
    return hr;
}

STDMETHODIMP 
HXCookies::GetCookies(const char* pHost, const char* pPath, REF(IHXBuffer*) pCookies)
{
    IHXBuffer* pPlayerCookies = NULL;
    HX_RESULT res = GetCookiesInternal(pHost, pPath, pCookies, pPlayerCookies);
    HX_RELEASE(pPlayerCookies);
    return res;
}

STDMETHODIMP HXCookies::GetCookies(const char*	    pHost,
		      const char*	    pPath,
		      REF(IHXBuffer*)   pCookies,
		      REF(IHXBuffer*)   pPlayerCookies)
{
    return GetCookiesInternal(pHost, pPath, pCookies, pPlayerCookies);
}


#if defined(_CARBON) || defined(_MAC_MACHO)
#define REPORT_COOKIES 1
#endif

HX_RESULT HXCookies::GetCookiesInternal(const char* pHost,
				 const char*	    pPath,
				 REF(IHXBuffer*)   pCookies,
				 REF(IHXBuffer*)   pPlayerCookies)
{
    HX_RESULT	    hr = HXR_OK;
#ifdef _WINDOWS
    char*	    cp = NULL;
    char*	    pComma = NULL;
    char*	    pEqualSign = NULL;
    int		    host_length = 0;
    int		    path_length = 0;
    UINT32	    dwSize = 0;    
    HXBOOL	    bAdded = FALSE;
    CookieStruct*   pNewCookie = NULL;
#endif /* _WINDOWS */

    char*           pPathWithSlash = NULL;
    char*	    pData = NULL;
    char*	    pURL = NULL;
    time_t	    cur_time = time(NULL);
    UINT32          ulCookieType = 0;
    CookieStruct*   pTempCookie = NULL;
    CHXSimpleList*  pCookiesFound1 = NULL;
    CHXSimpleList*  pCookiesFound2 = NULL;
    CHXSimpleList*  pCookiesList = NULL;
    IHXValues*	    pValues = NULL;
    CHXSimpleList::Iterator  iterator;
    CHXString       cHostCopy;
    INT32           lColon;
    HXBOOL	    bIsPlayerCookieList = FALSE;
    int	            l = 0;
    CHXString	    strPlayerCookies;
#ifdef REPORT_COOKIES
    CHXString		strCookieReport;
#endif
    
    pCookies = NULL;
    pPlayerCookies = NULL;

    if (!IsCookieEnabled())
    {
	goto cleanup;
    }

    if (!m_bInitialized)
    {
	hr = Initialize();

	if (HXR_OK != hr)
	{
	    goto cleanup;
	}
    }
    else
    {
	SyncRMCookies(FALSE);
    }

    if (!pHost || !m_pCookiesHelper)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    FixPath(pPath, pPathWithSlash);

    // return string to build    
    hr = CreateValuesCCF(pValues, m_pContext);
    if (HXR_OK != hr)
    {
	goto cleanup;
    }

    cHostCopy = pHost;
    lColon    = cHostCopy.Find(':');
    if (lColon >= 0)
    {
	cHostCopy = cHostCopy.Left(lColon);
    }
    cHostCopy.MakeLower();

    // search for all cookies(Netscape/Firefox only for now)
    for (l = 0; l < 4; l++)
    {
        pCookiesList = NULL;

	switch (l)
	{
	case 0:
            ulCookieType = HX_HELIX_COOKIES;
	    pCookiesList = m_pRMCookies;
	    bIsPlayerCookieList = TRUE;
	    break;
	case 1:
            ulCookieType = HX_EXTERNAL_COOKIES_NS;
            if (HX_EXTERNAL_COOKIES_NS & m_ulExternalCookies)
            {
	        pCookiesList = m_pNSCookies;
	        bIsPlayerCookieList = FALSE;
            }
	    break;
	case 2:            
            if (HX_EXTERNAL_COOKIES_FF_TEXT & m_ulExternalCookies)
            {
                ulCookieType = HX_EXTERNAL_COOKIES_FF_TEXT;
                SyncFFCookies(HX_EXTERNAL_COOKIES_FF_TEXT);
	        pCookiesList = m_pFFCookies;
	        bIsPlayerCookieList = FALSE;
            }
            if (HX_EXTERNAL_COOKIES_FF_SQLITE & m_ulExternalCookies)
            {
                ulCookieType = HX_EXTERNAL_COOKIES_FF_SQLITE;
                SyncFFCookies(HX_EXTERNAL_COOKIES_FF_SQLITE);
                pCookiesList = m_pFFCookies;
                bIsPlayerCookieList = FALSE;
            }
	    break;
        case 3:
            if (HX_EXTERNAL_COOKIES_SF & m_ulExternalCookies)
            {
                pCookiesList = m_pSFCookies;
                bIsPlayerCookieList = FALSE;
            }
            break;
	default:
	    break;
	}

	if (!pCookiesList)
	{
	    continue;
	}

	for (iterator = pCookiesList->Begin(); iterator != pCookiesList->End(); ++iterator)
	{
	    pTempCookie = (CookieStruct*) (*iterator);
    
	    if (!pTempCookie->pHost)
	    {
		continue;
	    }
	    
	    // check the host or domain first
	    if(pTempCookie->bIsDomain)
	    {
		if (!DoesDomainMatch(*pTempCookie->pHost, cHostCopy))
		{
		    continue;
		}
	    }
	    else if(strcasecmp((const char*)*(pTempCookie->pHost), pHost))
	    {
		// hostname matchup failed.
		continue;
	    }

	    // shorter strings always come last so there can be no
	    // ambiguity
	    if (DoPathsMatch((const char*)(*pTempCookie->pPath), pPath) ||
                DoPathsMatch((const char*)(*pTempCookie->pPath), pPathWithSlash))
	    {
		// check for expired cookies
		if(pTempCookie->expires && (pTempCookie->expires < cur_time))
		{
		    m_bSaveCookies = TRUE;
		    continue;
		}

		if (!pCookiesFound1)
		{
		    pCookiesFound1 = new CHXSimpleList();
		    pCookiesFound1->AddTail(pTempCookie);
		}
		else if (!WasCookieAdded(pCookiesFound1, pTempCookie))
		{
		    pCookiesFound1->AddTail(pTempCookie);
		}

                HXLOGL2(HXLOG_COOK, "%s:\n\t\t\t\tHost:%s\n\t\t\t\tPath:%s\n\t\t\t\tCookieName:%s\n\t\t\t\tCookieValue:%s\n\t\t\t\tExpires:%lu\n\t\t\t\tIsDomain:%lu\n\t\t\t\tMemoryOnly:%lu\n",
                                    (HX_HELIX_COOKIES == ulCookieType)?"GetRMCookies":((HX_EXTERNAL_COOKIES_FF_TEXT == ulCookieType || HX_EXTERNAL_COOKIES_FF_SQLITE == ulCookieType)?"GetFFCookies":"GetUnknownCookies"),
                                    (const char*)*(pTempCookie->pHost),
                                    (const char*)*(pTempCookie->pPath),
                                    (const char*)*(pTempCookie->pCookieName),
                                    (const char*)*(pTempCookie->pCookieValue),
                                    pTempCookie->expires,
                                    pTempCookie->bIsDomain,
                                    pTempCookie->bMemoryOnly);

		if(bIsPlayerCookieList)
		{
		    if(!strPlayerCookies.IsEmpty())
		    {
		        strPlayerCookies += "|";
		    }
		    strPlayerCookies += *pTempCookie->pCookieName;
#ifdef REPORT_COOKIES
		    strCookieReport += (strCookieReport.IsEmpty() ? "" : ", ");
		    strCookieReport += *pTempCookie->pCookieName;
		    strCookieReport += "=";
		    strCookieReport += *pTempCookie->pCookieValue;
#endif
		}
	    }
	}
    }

#ifdef _WINDOWS
    if (HX_EXTERNAL_COOKIES_IE & m_ulExternalCookies)
    {
        if (!_pInternetGetCookie || m_bMemoryOnly)
        {
	    goto cleanup;
        }

        host_length = strlen(pHost);

        if (pPathWithSlash)
        {
	    path_length = strlen(pPathWithSlash);
        }

        pURL = new char[host_length + path_length + strlen(IE_COOKIE_URLSTRING)];
        sprintf(pURL, IE_COOKIE_URLSTRING, pHost, pPathWithSlash); /* Flawfinder: ignore */

        if (_pInternetGetCookie(pURL, NULL, pData, &dwSize) && !pData && dwSize)
        {
	    pData = new char[dwSize+1];

	    if (!_pInternetGetCookie(pURL, NULL, pData, &dwSize))
	    {
	        goto cleanup;
	    }

	    cp = pData;

	    while (pComma = (char*) ::HXFindChar(cp, ';'))
	    {
	        *pComma = '\0';
	        pComma++;

	        if (pEqualSign = (char*) ::HXFindChar(cp, '='))
	        {
		    *pEqualSign = '\0';
		    pEqualSign++;

        	    pNewCookie = new CookieStruct;
		    bAdded = FALSE;

		    // copy
		    pNewCookie->pCookieValue = new CHXString(pEqualSign);
		    pNewCookie->pCookieName = new CHXString(cp);
		    pNewCookie->pPath = new CHXString(pPathWithSlash);
		    pNewCookie->pHost = new CHXString(pHost);
		    pNewCookie->expires = 0;
		    pNewCookie->bIsDomain = FALSE;
		    pNewCookie->bMemoryOnly = FALSE;

		    if(pNewCookie->pCookieName)
		    {
		        pNewCookie->pCookieName->TrimLeft();
		        pNewCookie->pCookieName->TrimRight();
		    }

		    if (!WasCookieAdded(pCookiesFound1, pNewCookie))
		    {
		        if (!pCookiesFound2)
		        {
			    pCookiesFound2 = new CHXSimpleList();
			    pCookiesFound2->AddTail(pNewCookie);
			    bAdded = TRUE;
		        }
		        else if (!WasCookieAdded(pCookiesFound2, pNewCookie))
		        {
			    pCookiesFound2->AddTail(pNewCookie);
			    bAdded = TRUE;
		        }
		    }
    		
                    if (bAdded)
                    {
                        HXLOGL2(HXLOG_COOK, "%s:\n\t\t\t\tHost:%s\n\t\t\t\tPath:%s\n\t\t\t\tCookieName:%s\n\t\t\t\tCookieValue:%s\n\t\t\t\tExpires:%lu\n\t\t\t\tIsDomain:%lu\n\t\t\t\tMemoryOnly:%lu\n",
                                            "GetIECookies",
                                            (const char*)*(pNewCookie->pHost),
                                            (const char*)*(pNewCookie->pPath),
                                            (const char*)*(pNewCookie->pCookieName),
                                            (const char*)*(pNewCookie->pCookieValue),
                                            pNewCookie->expires,
                                            pNewCookie->bIsDomain,
                                            pNewCookie->bMemoryOnly);
                    }
                    else
		    {
		        HX_DELETE(pNewCookie);
		    }
	        }

	        cp = pComma;
	    }

	    if (pEqualSign = (char*) ::HXFindChar(cp, '='))
	    {
	        *pEqualSign = '\0';
	        pEqualSign++;

	        pNewCookie = new CookieStruct;
	        bAdded = FALSE;

	        // copy
	        pNewCookie->pCookieValue = new CHXString(pEqualSign);
	        pNewCookie->pCookieName = new CHXString(cp);
		pNewCookie->pPath = new CHXString(pPathWithSlash);
		pNewCookie->pHost = new CHXString(pHost);
	        pNewCookie->expires = 0;
	        pNewCookie->bIsDomain = FALSE;
	        pNewCookie->bMemoryOnly = FALSE;

	        if(pNewCookie->pCookieName)
	        {
	            pNewCookie->pCookieName->TrimLeft();
	            pNewCookie->pCookieName->TrimRight();
	        }

	        if (!WasCookieAdded(pCookiesFound1, pNewCookie))
	        {
		    if (!pCookiesFound2)
		    {
		        pCookiesFound2 = new CHXSimpleList();
		        pCookiesFound2->AddTail(pNewCookie);
		        bAdded = TRUE;
		    }
		    else if (!WasCookieAdded(pCookiesFound2, pNewCookie))
		    {
		        pCookiesFound2->AddTail(pNewCookie);
		        bAdded = TRUE;
		    }
	        }

                if (bAdded)
                {
                    HXLOGL2(HXLOG_COOK, "%s:\n\t\t\t\tHost:%s\n\t\t\t\tPath:%s\n\t\t\t\tCookieName:%s\n\t\t\t\tCookieValue:%s\n\t\t\t\tExpires:%lu\n\t\t\t\tIsDomain:%lu\n\t\t\t\tMemoryOnly:%lu\n",
                                        "GetIECookies",
                                        (const char*)*(pNewCookie->pHost),
                                        (const char*)*(pNewCookie->pPath),
                                        (const char*)*(pNewCookie->pCookieName),
                                        (const char*)*(pNewCookie->pCookieValue),
                                        pNewCookie->expires,
                                        pNewCookie->bIsDomain,
                                        pNewCookie->bMemoryOnly);
                }
                else
	        {
		    HX_DELETE(pNewCookie);
	        }
	    }
        }
    }
#endif /* _WINDOWS */

cleanup:

    if (pCookiesFound1)
    {
	for (iterator = pCookiesFound1->Begin(); iterator != pCookiesFound1->End(); ++iterator)
	{
	    pTempCookie = (CookieStruct*) (*iterator);

	    if(pTempCookie->pCookieName && pTempCookie->pCookieValue)
	    {
		::SaveStringToHeader(pValues,
				     (const char*)*(pTempCookie->pCookieName),
				     (char*)(const char*)*(pTempCookie->pCookieValue),
				     m_pContext);
	    }
	}
    }

    if (pCookiesFound2)
    {
	for (iterator = pCookiesFound2->Begin(); iterator != pCookiesFound2->End(); ++iterator)
	{
	    pTempCookie = (CookieStruct*) (*iterator);

	    if(pTempCookie->pCookieName && pTempCookie->pCookieValue)
	    {
		::SaveStringToHeader(pValues,
				     (const char*)*(pTempCookie->pCookieName),
				     (char*)(const char*)*(pTempCookie->pCookieValue),
				     m_pContext);
	    }

	    HX_DELETE(pTempCookie);
	}
    }

    if (m_pCookiesHelper)
    {
	hr = m_pCookiesHelper->UnPack(pValues, pCookies);
    }

#ifdef REPORT_COOKIES
	// if the preference "CookieData" is set to "console" then write the cookie requests to the console
    IHXBuffer* pBuff = NULL;
    if (m_pPreferences
		&& m_pPreferences->ReadPref("CookieData", pBuff) == HXR_OK
		&& 0 == stricmp((const char*)pBuff->GetBuffer(), "console"))
    {
	CHXString str;
	str.Format("Cookies requested, getting host: %s  path: %s  cookies: %s",
			pHost, pPathWithSlash, 
			strCookieReport.IsEmpty() ? "<no cookies>" : (const char *) strCookieReport);
#if defined(_CARBON) || defined(_MAC_MACHO)
	CFStringRef stringRef = ::CFStringCreateWithCString( kCFAllocatorDefault, (const char*) str, kCFStringEncodingUTF8 );
	::CFShow(stringRef);
	::CFRelease(stringRef);
#endif
    }
    HX_RELEASE(pBuff);
#endif

    if(!strPlayerCookies.IsEmpty())
    {
	CreateAndSetBufferCCF(pPlayerCookies, (UCHAR*)(const char*)strPlayerCookies, 
			      strPlayerCookies.GetLength() + 1, m_pContext);
    }

    HX_DELETE(pCookiesFound1);
    HX_DELETE(pCookiesFound2);
    HX_VECTOR_DELETE(pData);
    HX_VECTOR_DELETE(pURL);
    HX_VECTOR_DELETE(pPathWithSlash);

    HX_RELEASE(pValues);
   
    return hr;
}

HX_RESULT	    
HXCookies::PrepareCookies(void)
{
    HX_RESULT   hr = HXR_OK;

    hr = _prepareRMCookies();

    if (HX_EXTERNAL_COOKIES_IE & m_ulExternalCookies)
    {
        _prepareIECookies();
    }

    if (HX_EXTERNAL_COOKIES_NS & m_ulExternalCookies)
    {
        _prepareNSCookies();
    }

    if (HX_EXTERNAL_COOKIES_FF_TEXT & m_ulExternalCookies)
    {
        _prepareFFTextCookies();
    }

    if (HX_EXTERNAL_COOKIES_FF_SQLITE & m_ulExternalCookies)
    {
        _prepareFFSQLiteCookies();
    }

    if (HX_EXTERNAL_COOKIES_SF & m_ulExternalCookies)
    {
        _prepareSFCookies();
    }

    return hr;
}

HX_RESULT
HXCookies::_prepareRMCookies(void)
{
    HX_RESULT		hr = HXR_OK;
    const char*		pRMCookiesPath = NULL;
    IHXBuffer*		pBuffer = NULL;

#if defined(_VXWORKS)
    return hr;
#endif /* _VXWORKS */

    if (m_pRMCookiesPath)
    {
        return HXR_OK;
    }

#if !defined(_CARBON) && !defined(_MAC_MACHO) /* figured out every time at runtime on the Mac since paths are not stable between runs */
    if (m_pPreferences && (m_pPreferences->ReadPref("CookiesPath", pBuffer) == HXR_OK))
    {
	m_pRMCookiesPath = new char[pBuffer->GetSize() + 1];
	strcpy(m_pRMCookiesPath, (const char*)pBuffer->GetBuffer()); /* Flawfinder: ignore */
    }
    HX_RELEASE(pBuffer);
#endif

    if (!m_pRMCookiesPath)
    {
	if (m_pPreferences && (m_pPreferences->ReadPref("UserSDKDataPath", pBuffer) == HXR_OK))
	{
	    pRMCookiesPath = (char*) pBuffer->GetBuffer();
	}
	else
	{
#if defined(_CARBON) || defined(_MAC_MACHO)
	    CHXString strSDKPath = CHXFileSpecUtils::GetAppDataDir(HXVER_SDK_PRODUCT).GetPathName();
#elif defined(_UNIX)
	    pRMCookiesPath = getenv("HOME");
	    HX_ASSERT( pRMCookiesPath );	    
#else
	    pRMCookiesPath = GetDLLAccessPath()->GetPath(DLLTYPE_COMMON);
#endif        
	}
	
        if (pRMCookiesPath)
        {        
            m_pRMCookiesPath = new char[strlen(pRMCookiesPath) + strlen(RM_COOKIE_FILE_NAME)+2];
	    ::strcpy(m_pRMCookiesPath, pRMCookiesPath); /* Flawfinder: ignore */
	    if (m_pRMCookiesPath[::strlen(m_pRMCookiesPath)-1] != OS_SEPARATOR_CHAR)
	    {
		strcat(m_pRMCookiesPath, OS_SEPARATOR_STRING); /* Flawfinder: ignore */
	    }

	    strcat(m_pRMCookiesPath, RM_COOKIE_FILE_NAME); /* Flawfinder: ignore */
            
	    HX_RELEASE(pBuffer);
	    if (HXR_OK == CreateAndSetBufferCCF(pBuffer, (UCHAR*)m_pRMCookiesPath, 
						strlen(m_pRMCookiesPath) + 1, m_pContext))
	    {
		if (m_pPreferences)
		{
		    m_pPreferences->WritePref("CookiesPath", pBuffer);
		}    
		HX_RELEASE(pBuffer);
	    }
        }
    }

    if (m_pRMCookiesPath)
    {
        hr = OpenCookies(m_pRMCookiesPath, TRUE, m_pRMCookies);
        if (SUCCEEDED(hr))
        {
            UpdateModificationTime(m_pRMCookiesPath, m_lastRMCookiesFileModification);
        }
    }

    return hr;
}

HX_RESULT
HXCookies::_prepareIECookies(void)
{
    HX_RESULT   hr = HXR_FAIL;

#ifdef _WINDOWS
    if (m_hLib)
    {
        if (_pInternetSetCookie && _pInternetGetCookie)
        {
            hr = HXR_OK;
        }
    }
    else if (m_hLib = LoadLibrary(OS_STRING("wininet.dll")))
    {
       	_pInternetSetCookie = (INTERNETSETCOOKIE)GetProcAddress(m_hLib, OS_STRING("InternetSetCookieA"));
	_pInternetGetCookie = (INTERNETGETCOOKIE)GetProcAddress(m_hLib, OS_STRING("InternetGetCookieA"));
        hr = HXR_OK;
    }
#endif /* _WINDOWS */
    
    return hr;
}

HX_RESULT
HXCookies::_prepareNSCookies(void)
{
    HX_RESULT   hr = HXR_FAIL;

#ifdef _WINDOWS
    if (m_pNSCookiesPath)
    {
        return HXR_OK;
    }

    HKEY    hMainAppKey = NULL;
    HKEY    hBiffKey = NULL;
    HKEY    hUsersKey = NULL;
    HKEY    hServersKey = NULL;
    HKEY    hCookiesKey = NULL;
    INT32   lSize = _MAX_PATH - 1;
    UINT32  ulSize = _MAX_PATH - 1;
    UINT32  ulType = 0;
    char    regKey[_MAX_PATH] = {0}; /* Flawfinder: ignore */
    char*   pPath = NULL;
    char*   pUser = NULL;
    char*   pValue = NULL;
    char*   pCursor = NULL;

    // Netscape Browser 4.0+
    if (RegOpenKeyEx(HKEY_CURRENT_USER, 
		     OS_STRING("Software\\Netscape\\Netscape Navigator\\biff"), 
		     0,
		     KEY_READ, &hBiffKey) == ERROR_SUCCESS) 
    {
	pUser = new char[_MAX_PATH];
	memset(pUser, 0, _MAX_PATH);

	if (RegQueryValueEx(hBiffKey, OS_STRING("CurrentUser"), 0, &ulType, (LPBYTE)pUser, &ulSize) == ERROR_SUCCESS) 
	{
	    SafeSprintf(regKey, _MAX_PATH, "Software\\Netscape\\Netscape Navigator\\biff\\users\\%s", pUser);
	    
	    if (RegOpenKeyEx(HKEY_CURRENT_USER, OS_STRING(regKey), 0, KEY_READ, &hUsersKey) == ERROR_SUCCESS)
	    {
		pValue = new char[_MAX_PATH];
		memset(pValue, 0, _MAX_PATH);

		ulSize = _MAX_PATH - 1;
		if (RegQueryValueEx(hUsersKey, OS_STRING("defaultServer"), 0, &ulType, (LPBYTE)pValue, &ulSize) == ERROR_SUCCESS)
		{
		    SafeSprintf(regKey, _MAX_PATH, "Software\\Netscape\\Netscape Navigator\\biff\\users\\%s\\servers\\%s",
			    pUser, pValue);

		    if (RegOpenKeyEx(HKEY_CURRENT_USER, OS_STRING(regKey), 0, KEY_READ, &hServersKey) == ERROR_SUCCESS)
		    {
			pPath = new char[_MAX_PATH];
			memset(pPath, 0, _MAX_PATH);

			ulSize = _MAX_PATH - 1;
			if (RegQueryValueEx(hServersKey, OS_STRING("popstatePath"), 0, &ulType, (LPBYTE)pPath, &ulSize) == ERROR_SUCCESS)
			{
			    // pPath ="..\mail\popstate.dat"
			    // the actual location of cookies is 1 level up
			    if (pCursor = ::HXReverseFindChar(pPath, '\\'))
			    {
				*pCursor = '\0';
			    
				if (pCursor = ::HXReverseFindChar(pPath, '\\'))
				{
				    *pCursor = '\0';

				    m_pNSCookiesPath = new char[strlen(pPath) + 13];
				    sprintf(m_pNSCookiesPath, "%s\\%s", pPath, RM_COOKIE_FILE_NAME); /* Flawfinder: ignore */
				    goto cleanup;
				}
			    }
			}
		    }
		}
	    }
	}
    }

    // Netscape Browser 3.0+
    if (RegOpenKeyEx(HKEY_CURRENT_USER, OS_STRING("Software\\Netscape\\Netscape Navigator\\Cookies"), 0,
	KEY_READ, &hCookiesKey) == ERROR_SUCCESS) 
    {
	HX_VECTOR_DELETE(pPath);
	pPath = new char[_MAX_PATH];
	memset(pPath, 0, _MAX_PATH);

	ulSize = _MAX_PATH - 1;
	if (RegQueryValueEx(hCookiesKey, OS_STRING("Cookie File"), 0, &ulType, (LPBYTE)pPath, &ulSize) == ERROR_SUCCESS)
	{
	    m_pNSCookiesPath = new char[strlen(pPath) + 1];
	    strcpy(m_pNSCookiesPath, pPath); /* Flawfinder: ignore */
	}
    }

cleanup:

    if (hMainAppKey)
    {
	RegCloseKey(hMainAppKey);
    }

    if (hBiffKey)
    {
	RegCloseKey(hBiffKey);
    }

    if (hUsersKey)
    {
	RegCloseKey(hUsersKey);
    }

    if (hServersKey)
    {
	RegCloseKey(hServersKey);
    }

    if (hCookiesKey)
    {
	RegCloseKey(hCookiesKey);
    }

    HX_VECTOR_DELETE(pUser);
    HX_VECTOR_DELETE(pPath);
    HX_VECTOR_DELETE(pValue);    

    if (m_pNSCookiesPath)
    {
        hr = OpenCookies(m_pNSCookiesPath, FALSE, m_pNSCookies);
    }
#endif /* _WINDOWS */

    return hr;
}

HX_RESULT
HXCookies::_prepareFFTextCookies(void)
{
    HX_RESULT   hr = HXR_FAIL;

    if (!m_pFFProfilePath)
    {
        HX_ASSERT(!m_pFFTextCookiesPath);
        hr = GetFFPaths(HX_EXTERNAL_COOKIES_FF_TEXT, m_pFFTextCookiesPath, m_pFFProfilePath);
    }

    if (_isFFCookiesEnabled(m_pFFProfilePath) && m_pFFTextCookiesPath)
    {
        hr = OpenCookies(m_pFFTextCookiesPath, FALSE, m_pFFCookies);
        if (SUCCEEDED(hr))
        {
            UpdateModificationTime(m_pFFTextCookiesPath, m_lastFFTextCookiesFileModification);
        }
    }

    return hr;
}

HX_RESULT
HXCookies::_prepareFFSQLiteCookies()
{
    HX_RESULT   hr = HXR_FAIL;

    if (!m_pFFProfilePath)
    {
        HX_ASSERT(!m_pFFSQLiteCookiesPath);
        hr = GetFFPaths(HX_EXTERNAL_COOKIES_FF_SQLITE, m_pFFSQLiteCookiesPath, m_pFFProfilePath);
    }

    if (_isFFCookiesEnabled(m_pFFProfilePath) && m_pFFSQLiteCookiesPath)
    {
        hr = OpenCookiesSQLite(m_pFFSQLiteCookiesPath, m_pFFCookies);
        if (SUCCEEDED(hr))
        {
            UpdateModificationTime(m_pFFSQLiteCookiesPath, m_lastFFSQLiteCookiesFileModification);
        }
    }

    return hr;
}

HX_RESULT
HXCookies::_prepareSFCookies(void)
{
#if defined(_MAC_UNIX) && defined(HELIX_FEATURE_SAFARI_COOKIES)
    HX_RESULT retval = HXR_FAIL;
    if (!m_pSFCookies)
    {
        m_pSFCookies = new CHXSimpleList();
        void* pWebKitCookiesToken = InitializeWebKitCookies();
        if (pWebKitCookiesToken)
        {
            int numberOfWebKitCookies = GetNumberOfWebKitCookies(pWebKitCookiesToken);
            int whichCookie;
            for (whichCookie = 0; whichCookie < numberOfWebKitCookies; whichCookie++)
            {
                CookieStruct* pNewCookie = new CookieStruct;
                HX_RESULT getInformationResult = GetWebKitCookieInformation(pWebKitCookiesToken, whichCookie, pNewCookie);
                if (SUCCEEDED(getInformationResult))
                {
                    m_pSFCookies->AddTail(pNewCookie);
                }
                else
                {
                    delete pNewCookie;
                }
            }
            retval = HXR_OK;
        }
    }
    return retval;
#else
    return HXR_NOTIMPL;
#endif
}

HXBOOL
HXCookies::_isFFCookiesEnabled(const char* pszPrefsPath)
{
    HXBOOL      bResult = TRUE;
    char	szLineBuffer[LINE_BUFFER_SIZE] = {0}; /* Flawfinder: ignore */
    UINT32	ulBytesRead = 0;
    char*       result = NULL;
    FILE*	fp = NULL;

    if (pszPrefsPath)
    {    
        if (!(fp = fopen(pszPrefsPath, "r+b")))
        {
	    goto cleanup;
        }

        while (HXR_OK == FileReadLine(fp, &szLineBuffer[0], LINE_BUFFER_SIZE, &ulBytesRead))
        {
            result = strstr(szLineBuffer, "network.cookie.cookieBehavior");
            if (result)
            {
                // 2 - don't use cookies
                result = strchr(result, '2');
                if (result)
                {
                    bResult = FALSE;
                }
                break;
            }
            ZeroInit(&szLineBuffer);
        }
    }

cleanup:

    return bResult;
}

HX_RESULT
HXCookies::SyncFFCookies(UINT32 ulCookieType)
{
    HX_RESULT	    hr = HXR_OK;
    CHXSimpleList*  pNewFFCookies = NULL;
    CookieStruct*   pCookie = NULL;
    HXBOOL          bCookieFileModified = FALSE;

    if (!m_bInitialized)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    if (HX_EXTERNAL_COOKIES_FF_TEXT == ulCookieType)
    {
        bCookieFileModified = IsCookieFileModified(m_pFFTextCookiesPath, m_lastFFTextCookiesFileModification);
        if (bCookieFileModified)
        {
	    if (HXR_OK == OpenCookies(m_pFFTextCookiesPath, FALSE, pNewFFCookies))
	    {
                UpdateModificationTime(m_pFFTextCookiesPath, m_lastFFTextCookiesFileModification);

                ResetCookies(m_pFFCookies);
	        HX_DELETE(m_pFFCookies);

	        m_pFFCookies = pNewFFCookies;
	    }	    	    
        }
    }
    else if (HX_EXTERNAL_COOKIES_FF_SQLITE == ulCookieType)
    {
        bCookieFileModified = IsCookieFileModified(m_pFFSQLiteCookiesPath, m_lastFFSQLiteCookiesFileModification);
        if (bCookieFileModified)
        {
            ResetCookies(m_pFFCookies);
            HX_DELETE(m_pFFCookies);

	    if (HXR_OK == OpenCookiesSQLite(m_pFFSQLiteCookiesPath, m_pFFCookies))
	    {
                UpdateModificationTime(m_pFFSQLiteCookiesPath, m_lastFFSQLiteCookiesFileModification);
	    }	    	    
        }
    }

cleanup:

    return hr;
}

HX_RESULT
HXCookies::GetFFPaths(UINT32 ulCookieType, char*& pszCookiesPath, char*& pszProfilePath)
{
    HX_RESULT   hr = HXR_FAIL;
    char  szProfilesDirPath[_MAX_PATH] = {0};
    char  szProfilesIniPath[_MAX_PATH] = {0};	
    char  szIniSection[20] = {0};
    char  szProfilePath[_MAX_PATH] = {0};
    int   nCurProfile = 0;
    HXBOOL bIsRelativePath = FALSE;
	
#if defined(_WINDOWS) && !defined(WINCE)
    if (pszCookiesPath || pszProfilePath)
    {
        return HXR_INVALID_PARAMETER;
    }

    // Grabbing Firefox Cookies Path. See http://www.mozilla.org/support/firefox/profile
    // for information on finding the path to profiles.ini on various platforms.

    // first find the profiles.ini file:
    // on Windows XP/2000, %AppData%\Mozilla\Firefox\
    // on Windows 95/98/Me, C:\WINDOWS\Application Data\Mozilla\Firefox\
    // on Linux, ~/.mozilla/firefox
    // on Mac OS X, ~/Library/Application Support/Firefox/
    char  szAppDataPath[_MAX_PATH] = {0};
    HXBOOL bFoundAppDataDir  = FALSE;
    HINSTANCE hShell         = NULL;
    
    FPSHGetFolderPath fpSHGetFolderPath = NULL;

    hShell = ::LoadLibrary(OS_STRING("shell32.dll"));
    if (hShell)
    {
	fpSHGetFolderPath = (FPSHGetFolderPath) GetProcAddress(hShell,OS_STRING("SHGetFolderPathA"));
        if (fpSHGetFolderPath)
        {
            // SHGetSpecialFolderPath dox say buffer must be at least MAX_PATH in size.
            bFoundAppDataDir = SUCCEEDED(fpSHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, &szAppDataPath[0]));
        }
    }

    if (bFoundAppDataDir)
    {
        SafeSprintf(szProfilesDirPath, _MAX_PATH, "%s\\Mozilla\\Firefox\\", szAppDataPath);
        SafeSprintf(szProfilesIniPath, _MAX_PATH, "%sprofiles.ini", szProfilesDirPath);
    }
    else
    {
        // the environment variable didn't exist; we're likely on Windows 95/98/ME, so
        // try the 95/98/ME path.
        SafeSprintf(szProfilesIniPath, _MAX_PATH, "C:\\WINDOWS\\Application Data\\Mozilla\\Firefox\\profiles.ini");
    }

    // Once we've found profiles.ini, iterate through [profile0]-[profileN] looking for
    // the default profile. This will include the path to the profile directory, which
    // will include a file cookies.txt.
    DWORD bytesRead = 0;
    do
    {
        DWORD profilepathSize = _MAX_PATH - 1;
        SafeSprintf(szIniSection, 20, "profile%d", nCurProfile);
        bytesRead = ::GetPrivateProfileString(szIniSection, "Path", NULL, szProfilePath, _MAX_PATH - 1, szProfilesIniPath);
        if (bytesRead)
        {
            // see (1) whether this is the default and (2) whether it's a relative or absolute path
            char defaultBuf[256];

	    // "Default=1" is not set by FireFox(bug?) when there is only 1 profile 
	    // so we look for both "Default=1" and "Name=default"
            DWORD appBytesRead = ::GetPrivateProfileString(szIniSection, "Default", NULL, defaultBuf, 256, szProfilesIniPath);
            if (!appBytesRead)
            {
    	        appBytesRead = ::GetPrivateProfileString(szIniSection, "Name", NULL, defaultBuf, 256, szProfilesIniPath);
	    }

	    if (appBytesRead)
	    {
                if (!strcasecmp(defaultBuf, "1") || !strcasecmp(defaultBuf, "default"))
		{
                    // a ha! this is the default!
                    char relativeBuf[256] = {0};
                    DWORD relBytesRead = ::GetPrivateProfileString(szIniSection, "IsRelative", NULL, relativeBuf, 256, szProfilesIniPath);
                    if (relBytesRead && !strcmp(relativeBuf, "1"))
                    {
                        bIsRelativePath = TRUE;
                    }

		    hr = HXR_OK;
                    break; // done iterating through profile
                }
            }
        }
        nCurProfile++;
    } while (bytesRead);

    if (hShell)
    {
        FreeLibrary(hShell);
    }
#elif defined (_MAC_UNIX)
    CHXDirSpecifier prefsDir = CHXFileSpecUtils::MacFindFolder(kUserDomain, kApplicationSupportFolderType);
    CHXDirSpecifier ffDir = prefsDir.SpecifyChildDirectory("Firefox");
    if (CHXFileSpecUtils::DirectoryExists(ffDir))
    {
        SafeSprintf(szProfilesDirPath, _MAX_PATH, "%s/", (const char*)ffDir.GetPathName());
	SafeSprintf(szProfilesIniPath, _MAX_PATH, "%s/profiles.ini", (const char*)ffDir.GetPathName());
 
	int nNameValue = 0;
	CHXString* pPathValue = NULL;
	CHXString* pNameValue = NULL;
	CHXIniFileReader iniFileReader;
		
	if (iniFileReader.LoadFile(szProfilesIniPath))
	{
	    do
	    {
		SafeSprintf(szIniSection, 20, "Profile%d", nCurProfile);
		
		if (SUCCEEDED(iniFileReader.GetStrVal(szIniSection, "Path", pPathValue)))
		{
		    if (SUCCEEDED(iniFileReader.GetStrVal(szIniSection, "Name", pNameValue)) ||
			SUCCEEDED(iniFileReader.GetIntVal(szIniSection, "Name", nNameValue)))
		    {
			if ((pNameValue && !strcasecmp(*pNameValue, "default")) || nNameValue)
			{
			    iniFileReader.GetIntVal(szIniSection, "IsRelative", bIsRelativePath);
			}
						
			SafeSprintf(szProfilePath, _MAX_PATH, "%s", (const char*)*pPathValue); 
			hr = HXR_OK;
			break;
		    }
		}
		else
		{
		    break;
		}
				
		nCurProfile++;
	    } while (TRUE);
	}			
    }
#endif /* (_WINDOWS) && !(WINCE) */

    if (SUCCEEDED(hr))
    {
	pszCookiesPath = new char[_MAX_PATH];
        pszProfilePath = new char[_MAX_PATH];
        memset(pszCookiesPath, 0, _MAX_PATH);
        memset(pszProfilePath, 0, _MAX_PATH);

        if (bIsRelativePath)
        {
	    if (HX_EXTERNAL_COOKIES_FF_TEXT == ulCookieType)
            {
		SafeSprintf(pszCookiesPath, _MAX_PATH, "%s%s/cookies.txt", szProfilesDirPath, szProfilePath);
            }
            else if (HX_EXTERNAL_COOKIES_FF_SQLITE == ulCookieType)
            {
		SafeSprintf(pszCookiesPath, _MAX_PATH, "%s%s/cookies.sqlite", szProfilesDirPath, szProfilePath);
            }
            else
            {
		HX_ASSERT(FALSE);
            }
            SafeSprintf(pszProfilePath, _MAX_PATH, "%s%s/prefs.js", szProfilesDirPath, szProfilePath);
	}
        else
        {
	    if (HX_EXTERNAL_COOKIES_FF_TEXT == ulCookieType)
            {
	        SafeSprintf(pszCookiesPath, _MAX_PATH, "%s/cookies.txt", szProfilePath);
            }
            else if (HX_EXTERNAL_COOKIES_FF_SQLITE == ulCookieType)
            {
		SafeSprintf(pszCookiesPath, _MAX_PATH, "%s/cookies.sqlite", szProfilePath);
            }
            else
            {
		HX_ASSERT(FALSE);
            }
            SafeSprintf(pszProfilePath, _MAX_PATH, "%s/prefs.js", szProfilePath);
        }
    }

    return hr;
}

// prefix '/' to pPathIn if it's not present
void
HXCookies::FixPath(const char* pPathIn, char*& pPathOut)
{
    HXBOOL bPrefixSlash = FALSE;
    UINT32 ulLen = 0;

    pPathOut = NULL;

    if (pPathIn)
    {
        if (*pPathIn != '/')
        {
            bPrefixSlash = TRUE;
            ulLen = strlen(pPathIn)+2;
        }
        else
        {
            bPrefixSlash = FALSE;
            ulLen = strlen(pPathIn)+1;
        }

        pPathOut = new char[ulLen];
        if (pPathOut)
        {
            if (bPrefixSlash)
            {
                SafeSprintf(pPathOut, ulLen, "/%s", pPathIn);
            }
            else
            {
                SafeSprintf(pPathOut, ulLen, "%s", pPathIn);
            }
        }
    }
}

void
HXCookies::ResetCookies(CHXSimpleList* pCookieList)
{
    CookieStruct*   pCookie = NULL;

    while (pCookieList && pCookieList->GetCount() > 0)
    {
	pCookie = (CookieStruct*) pCookieList->RemoveHead();
	HX_DELETE(pCookie);
    }
}

HX_RESULT	
HXCookies::FileReadLine(FILE* fp, char* pLine, UINT32 ulLineBuf, UINT32* pBytesRead)
{
#ifdef _OPENWAVE
    return HXR_NOTIMPL;
#else
    HX_RESULT	hr = HXR_OK;
    UINT32	i = 0;
    UINT32	ulBytesRead = 0;
    char*	pTmpBuf = NULL;

    if (!fp)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    if (ulLineBuf < 1) 
    {
        *pBytesRead = 0;
        goto cleanup;
    }

    ulBytesRead = fread(pLine, sizeof(char), ulLineBuf, fp);
    pTmpBuf = pLine;

    if (ulBytesRead)
    {
	while (i < ulBytesRead) 
	{
#ifdef _MACINTOSH
	    if (pTmpBuf[i] == 10 || pTmpBuf[i] == 13)
#else
	    if (pTmpBuf[i] == 10)
#endif
	    {   // LF
		if (pTmpBuf[i+1])
		{
		    pTmpBuf[i+1] = '\0';
		}

		// Back the file pointer up.
		fseek(fp, (long)((i + 1) - ulBytesRead), SEEK_CUR);
		*pBytesRead = i + 1;
		break;
	    }
	    i++;
	}
    }
    else
    {
	hr = HXR_FAILED;
    }
    
cleanup:

    return hr;
#endif /* _OPENWAVE */
}

CookieStruct*
HXCookies::CheckForPrevCookie(char * path,
			       char * hostname,
			       char * name)
{
    HXBOOL	    bFound = FALSE;
    CookieStruct*   pCookie = NULL;
    CHXSimpleList::Iterator  i;

    if (!m_pRMCookies)
    {	
	goto cleanup;
    }

    for (i = m_pRMCookies->Begin(); i != m_pRMCookies->End(); ++i)
    {
	pCookie = (CookieStruct*) (*i);
	
        if(path && hostname			&& 
	   pCookie->pPath			&& 
	   pCookie->pHost			&& 
	   pCookie->pCookieName			&&
	   !pCookie->pCookieName->Compare(name)	&&
	   !pCookie->pPath->Compare(path)	&&	    
	   !pCookie->pHost->CompareNoCase(hostname))
	{
            bFound = TRUE;
	    break;
	}
    }

cleanup:    
    return bFound?pCookie:NULL;
}

HX_RESULT
HXCookies::OpenCookies(char* pCookieFile, HXBOOL bRMCookies, CHXSimpleList*& pCookiesList)
{
#ifdef _OPENWAVE
    return HXR_NOTIMPL;
#else
    HX_RESULT	    hr = HXR_OK;
    char*	    pHost = NULL;
    char*	    pPath = NULL;
    char*	    pCookieName = NULL; 
    char*	    pCookieValue = NULL;
    char*	    pIsDomain = NULL;
    char*	    pXXX = NULL;
    char*	    pExpires = NULL;
    char*	    pBuffer = new char[LINE_BUFFER_SIZE]; /* Flawfinder: ignore */
    UINT32	    ulBytesRead = 0;
    FILE*	    fp = NULL;
    CookieStruct*   pNewCookie = NULL;
 
    pCookiesList = NULL;

    if (!pBuffer)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    pBuffer[0] = '\0';

    if (!pCookieFile)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

#ifdef _WINDOWS
    if (bRMCookies)
    {
	if (!m_pLock)
	{
	    CreateEventCCF((void**)&m_pLock, m_pContext, "CookieFileLock", FALSE);
	}
	else
	{
	    m_pLock->Wait(ALLFS);
	}
    }
#endif /* _WINDOWS */

    if (bRMCookies)
    {
	if(CheckCookies() == HXR_FAIL)
	{
     	    hr = HXR_FAILED;
	    goto cleanup;
	}
    }

    if (!(fp = fopen(pCookieFile, "r+b")))
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

#if defined (_UNIX) && !defined(_SUN) && !defined(_HPUX) && !defined(_IRIX) && !defined(_AIX) && !defined(_OSF1)
    if (bRMCookies)
    {
	m_fileID = fileno(fp);    
	flock(m_fileID, LOCK_EX);
    }
#endif /* _UNIX */

    /* format is:
     *
     * host \t is_domain \t path \t xxx \t expires \t name \t cookie
     *
     * if this format isn't respected we move onto the next line in the file.
     *
     * is_domain is TRUE or FALSE   -- defaulting to FALSE
     * xxx is TRUE or FALSE	    -- should default to TRUE
     * expires is a time_t integer
     * cookie can have tabs
     */
    while(HXR_OK == FileReadLine(fp, &pBuffer[0], LINE_BUFFER_SIZE, &ulBytesRead))
    {
	if (*pBuffer == '#' || *pBuffer == CR || *pBuffer == LF || *pBuffer == 0)
	{
	    continue;
	}

	pHost = pBuffer;
	    
	if(!(pIsDomain = strchr(pHost, '\t')))
	{
	    continue;
	}
	*pIsDomain++ = '\0';
	if(*pIsDomain == CR || *pIsDomain == LF || *pIsDomain == 0)
	{
	    continue;
	}
	
	if(!(pPath = strchr(pIsDomain, '\t')))
	{
	    continue;
	}
	*pPath++ = '\0';
	if(*pPath == CR || *pPath == LF || *pPath == 0)
	{
	    continue;
	}

	if(!(pXXX = strchr(pPath, '\t')))
	{
	    continue;
	}
	*pXXX++ = '\0';
	if(*pXXX == CR || *pXXX == LF || *pXXX == 0)
	{
	    continue;
	}

	if(!(pExpires = strchr(pXXX, '\t')))
	{
	    continue;
	}
	*pExpires++ = '\0';
	if(*pExpires == CR || *pExpires == LF || *pExpires == 0)
	{
	    continue;
	}

	if(!(pCookieName = strchr(pExpires, '\t')))
	{
	    continue;
	}
	*pCookieName++ = '\0';
	if(*pCookieName == CR || *pCookieName == LF || *pCookieName == 0)
	{
	    continue;
	}

	if(!(pCookieValue = strchr(pCookieName, '\t')))
	{
	    continue;
	}
	*pCookieValue++ = '\0';
	if(*pCookieValue == CR || *pCookieValue == LF || *pCookieValue == 0)
	{
	    continue;
	}

	// remove the '\n' from the end of the cookie
	pCookieValue = ::StripLine(pCookieValue);

	// construct a new cookie_struct
	pNewCookie = new CookieStruct;
	if(!pNewCookie)
	{
	    hr = HXR_OUTOFMEMORY;
	    goto cleanup;
	}
	
	memset(pNewCookie, 0, sizeof(CookieStruct));

	/* copy
	 */
	pNewCookie->pCookieValue = new CHXString(pCookieValue);
	pNewCookie->pCookieName = new CHXString(pCookieName);
	pNewCookie->pPath = new CHXString(pPath);
	pNewCookie->pHost = new CHXString(pHost);

#ifdef _MACINTOSH
	pNewCookie->expires = (time_t)atoi64(pExpires);
#else
	pNewCookie->expires = atol(pExpires);
#endif

	if(!strcasecmp(pIsDomain, "TRUE"))
	{
	    pNewCookie->bIsDomain = TRUE;
	}
	else
	{
	    pNewCookie->bIsDomain = FALSE;
	}
	
	pNewCookie->bMemoryOnly = FALSE;

	if (!pCookiesList)
	{
	    pCookiesList = new CHXSimpleList();
	}

	hr = AddCookie(pNewCookie, pCookiesList);
    }

cleanup:

#if defined (_UNIX) && !defined(_SUN) && !defined(_HPUX) && !defined(_IRIX) && !defined(_AIX) && !defined(_OSF1)
    if (bRMCookies)
    {
	flock(m_fileID, LOCK_UN);
    }
#endif /* _UNIX */

    if (fp)
    {
	fclose(fp);
    }

#ifdef _WINDOWS
    if (bRMCookies && m_pLock)
    {
	m_pLock->SignalEvent();
    }
#endif /* _WINDOWS */

    delete [] pBuffer;
    
    return hr;
#endif /* _OPENWAVE */
}

int select_callback(void *p_data, int num_fields, char **p_fields, char **p_col_names) 
{
  HXCookies* pCookies = (HXCookies*)p_data;
  if (pCookies)
  {
      pCookies->ProcessSelectResultPerRow(num_fields, p_fields, p_col_names);
  }

  return 0;
}

void
HXCookies::ProcessSelectResultPerRow(int num_fields, char **p_fields, char **p_col_names)
{
    int i = 0;
    CookieStruct* pNewCookie = new CookieStruct;

    if (pNewCookie)
    {
        for (i = 0; i < num_fields; i++)
        {
            if (0 == strcasecmp(p_col_names[i], "name"))
            {
    	        pNewCookie->pCookieName = new CHXString(p_fields[i]);
            }
            else if (0 == strcasecmp(p_col_names[i], "value"))
            {
	        pNewCookie->pCookieValue = new CHXString(p_fields[i]);
            }
            else if (0 == strcasecmp(p_col_names[i], "host"))
            {
	        pNewCookie->pHost = new CHXString(p_fields[i]);
            }
            else if (0 == strcasecmp(p_col_names[i], "path"))
            {
	        pNewCookie->pPath = new CHXString(p_fields[i]);
            }
            else if (0 == strcasecmp(p_col_names[i], "expiry"))
            {
                pNewCookie->expires = atol(p_fields[i]);
            }
        }

        pNewCookie->bIsDomain = TRUE;
	pNewCookie->bMemoryOnly = FALSE;

	if (!m_pFFCookies)
	{
	    m_pFFCookies = new CHXSimpleList();
	}

	AddCookie(pNewCookie, m_pFFCookies);
    }
}

HX_RESULT
HXCookies::OpenCookiesSQLite(char* pCookieFile, CHXSimpleList*& pCookiesList)
{
    HX_RESULT   rc = HXR_FAIL;

#if defined(HELIX_FEATURE_SQLITE)
    int         status = 0;
    sqlite3*    pDB = NULL;

    if (!pCookieFile)
    {
        rc = HXR_INVALID_PARAMETER;
        goto exit;
    }

    status = sqlite3_open(pCookieFile, &pDB);
    if (SQLITE_OK == status && pDB)
    {
        char *errmsg;

        status = sqlite3_exec(pDB, "SELECT * FROM moz_cookies", select_callback, this, &errmsg);
        if (SQLITE_OK == status)
        {
            rc = HXR_OK;
        }

        sqlite3_close(pDB);
    }
#endif /* HELIX_FEATURE_SQLITE */

exit:

    return rc;
}

HX_RESULT	
HXCookies::SaveCookies(void)
{
#ifdef _OPENWAVE
    return HXR_NOTIMPL;
#else
    HX_RESULT	    hr = HXR_OK;
    FILE*	    fp = NULL;
    INT32	    len = 0;
    char	    date_string[36] = {0}; /* Flawfinder: ignore */
    time_t	    cur_date = time(NULL);
    CookieStruct*   pTempCookie = NULL;
    CHXSimpleList::Iterator  i;

    if (!m_pRMCookies || !m_pRMCookiesPath)
    {
	goto cleanup;
    }

#ifdef _WINDOWS
    if (!m_pLock)
    {
	CreateEventCCF((void**)&m_pLock, m_pContext, "CookieFileLock", FALSE);
    }
    else
    {
	m_pLock->Wait(ALLFS);
    }
#endif /* _WINDOWS */

    if (!(fp = fopen(m_pRMCookiesPath, "w")))
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

#ifdef _UNIX
    //Make the permisions on the cookies file User read/write only.
    if( chmod( m_pRMCookiesPath, S_IRUSR | S_IWUSR ) != 0 )
    {
        HX_ASSERT( "Can't change permision on cookies file." == NULL );
    }
    
#endif    

#if defined (_UNIX) && !defined(_SUN) && !defined(_HPUX) && !defined(_IRIX) && !defined(_AIX) && !defined(_OSF1)
    m_fileID = fileno(fp);    
    flock(m_fileID, LOCK_EX);
#endif /* _UNIX */

    fwrite(RM_COOKIE_CAPTION, sizeof(char), strlen(RM_COOKIE_CAPTION), fp);

    for (i = m_pRMCookies->Begin(); i != m_pRMCookies->End(); ++i)
    {
	pTempCookie = (CookieStruct*) (*i);

	/* format shall be:
 	 *
	 * host \t is_domain \t path \t secure \t expires \t name \t cookie
	 *
	 * is_domain is TRUE or FALSE
	 * secure is TRUE or FALSE  
	 * expires is a time_t integer
	 * cookie can have tabs
	 */
	if(pTempCookie->expires < cur_date)
	{
	    continue;  /* don't write entry if cookie has expired 
			* or has no expiration date
			*/
	}
		
	len = fwrite((const char*)*(pTempCookie->pHost), sizeof(char), pTempCookie->pHost->GetLength(), fp);
	if (len < 0)
	{
	    hr = HXR_FAILED;
	    goto cleanup;
	}
	fwrite("\t", sizeof(char), 1, fp);

	if(pTempCookie->bIsDomain)
	{
	    fwrite("TRUE", sizeof(char), 4, fp);
	}
	else
	{
	    fwrite("FALSE", sizeof(char), 5, fp);
	}	    
	fwrite("\t", sizeof(char), 1, fp);

	fwrite((const char*)*(pTempCookie->pPath), sizeof(char), pTempCookie->pPath->GetLength(), fp);
	fwrite("\t", sizeof(char), 1, fp);

	fwrite("FALSE", sizeof(char), 5, fp);
	fwrite("\t", sizeof(char), 1, fp);

	sprintf(date_string, "%u", pTempCookie->expires); /* Flawfinder: ignore */
	fwrite(date_string, sizeof(char), strlen(date_string), fp);
	fwrite("\t", sizeof(char), 1, fp);

	fwrite((const char*)*(pTempCookie->pCookieName), sizeof(char), 
		pTempCookie->pCookieName->GetLength(), fp);
	fwrite("\t", sizeof(char), 1, fp);

	fwrite((const char*)*(pTempCookie->pCookieValue), sizeof(char), 
		pTempCookie->pCookieValue->GetLength(), fp);
	fwrite(LINEBREAK, sizeof(char), LINEBREAK_LEN, fp);

	//once saved to disk, make sure the memory only flag is false
	pTempCookie->bMemoryOnly = FALSE;
    }

    UpdateModificationTime(m_pRMCookiesPath, m_lastRMCookiesFileModification);    
    m_bSaveCookies = FALSE;

cleanup:

#if defined (_UNIX) && !defined(_SUN) && !defined(_HPUX) && !defined(_IRIX) && !defined(_AIX) && !defined(_OSF1)
    flock(m_fileID, LOCK_UN);
#endif /* _UNIX */

    if (fp)
    {
	fclose(fp);
    }

#ifdef _WINDOWS
    if (m_pLock)
    {
	m_pLock->SignalEvent();
    }
#endif /* _WINDOWS */

    SecureCookies();

    return(hr);
#endif /* _OPENWAVE */
}

HX_RESULT
HXCookies::AddCookie(CookieStruct* pCookie, CHXSimpleList*& pCookiesList)
{
    HX_RESULT	    hr = HXR_OK;
    HXBOOL	    bShorterPathFound = FALSE;
    CookieStruct*   pTempCookie = NULL;
    LISTPOSITION    position;

    if (!pCookie || !pCookiesList)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    /* add it to the list so that it is before any strings of
     * smaller length
     */
    position = pCookiesList->GetHeadPosition();
    while (position != NULL)
    {
	pTempCookie = (CookieStruct*) pCookiesList->GetNext(position);

	if (strlen(*pCookie->pPath) > strlen(*pTempCookie->pPath))
	{
	    // Remember that we found a shorter path
	    bShorterPathFound = TRUE;

	    // If the position is null, then event was the first
	    // item in the list, and we need to do some fancy footwork...
	    if (!position)
	    {
		POSITION theTail = pCookiesList->GetTailPosition();
		pCookiesList->InsertBefore(theTail,(void*) pCookie);
	    }
	    // otherwise, roll after one...
	    else
	    {
		pCookiesList->GetPrev(position);

		if (!position)
		{
		    pCookiesList->AddHead((void*) pCookie);
		}
		else
		{
		    pCookiesList->InsertBefore(position,(void*) pCookie);
		}
	    }

	    break;
	}
    }

    // If we didn't find an earlier packet, then we should insert at
    // the head of the list...
    if (!bShorterPathFound)
    {
	pCookiesList->AddTail((void*) pCookie);
    }

cleanup:

    return hr;
}

HXBOOL
HXCookies::WasCookieAdded(CHXSimpleList* pCookiesList, CookieStruct* pCookie)
{
    HXBOOL bResult = FALSE;
    CookieStruct*  pTempCookie = NULL;
    CHXSimpleList::Iterator  i;

    if (!pCookiesList || !pCookie)
    {
	goto cleanup;
    }

    for (i = pCookiesList->Begin(); i != pCookiesList->End(); ++i)
    {
	pTempCookie = (CookieStruct*) (*i);
	
	if (pTempCookie->pCookieName && pCookie->pCookieName 
		&& *(pTempCookie->pCookieName) == *(pCookie->pCookieName) 
		&& pTempCookie->pHost && pCookie->pHost)
	{
	    if(DoesDomainMatch(*pTempCookie->pHost, *pCookie->pHost))
	    {
		bResult = TRUE;
		break;
	    }
	}
    }

cleanup:

    return bResult;
}


HXBOOL HXCookies::DoesDomainMatch(const char* szDomain, const char* szDomainToParse)
{
    HXBOOL bMatches = FALSE;
    CHXString cHostCopy;
    CHXString cDomainCopy;
    CHXString cHostRight;
    CHXString cDomainRight;

    if(!szDomain || !szDomainToParse || !strlen(szDomain) || !strlen(szDomainToParse))
    {
	goto cleanup;
    }

    cHostCopy = szDomainToParse;
    cDomainCopy = szDomain;
    cDomainCopy.MakeLower();

    // Now we compare the domain (from the cookie itself) with
    // the rightmost characters of the host. 
    if (cDomainCopy.GetAt(0) != '.')
    {
        // exact match
        if (cHostCopy == cDomainCopy)
        {
            bMatches = TRUE;
            goto cleanup;
        }
    }
    else
    {
        // excluding leading '.'
        cDomainRight = cDomainCopy.Right(cDomainCopy.GetLength() - 1);
        if (cHostCopy == cDomainRight)
        {
            bMatches = TRUE;
            goto cleanup;
        }

        // allow partial match(right -> left)
        // For instance, a domain of ".bar.com" would match with a host(passed in)
        // of "foo.bar.com", "www.bar.com" and "bar.com" etc. but would NOT match
        // a host of "foobar.com" or "foo.com"
        cHostRight = cHostCopy.Right(cDomainCopy.GetLength());
        if (cHostRight == cDomainCopy)
        {
            bMatches = TRUE;
            goto cleanup;
        }

        bMatches = FALSE;
    }

cleanup:

    return bMatches;
}

HXBOOL HXCookies::DoPathsMatch(const char* szCookiesPath, const char* szUrlPath)
{
    // Normalize the paths, skipping over leading and trailing slashes
    // The url path typically has the leading & trailing / stripped,
    // the cookie path typically contains them.
    
    const char* szCookiesPathPos = NULL;
    const char* szUrlPathPos = NULL;
    HXBOOL bResult = FALSE;

    if(!szCookiesPath || !szUrlPath)
    {
        return FALSE;
    }
        
    szCookiesPathPos = szCookiesPath;
    szUrlPathPos = szUrlPath;

    // always matches root path
    if (szCookiesPath &&
        !strcasecmp(szCookiesPath, "/"))
    {
        return TRUE;
    }

    // Skip past the leading /'s
    while(*szCookiesPathPos)
    {
        if(*szCookiesPathPos != '/')
        {
            break;
        }
        szCookiesPathPos++;
    }

    while(*szUrlPathPos)
    {
        if(*szUrlPathPos != '/')
        {
            break;
        }
        szUrlPathPos++;
    }

    // Compare until we find a difference
    while(*szCookiesPathPos && *szUrlPathPos)
    {
        if(*szCookiesPathPos != *szUrlPathPos)
        {
            break;
        }
        szCookiesPathPos++;
        szUrlPathPos++;        
    }

    if(*szUrlPathPos == '\0')
    {
        // Skip past any trailing slashes in the cookie path
        while(*szCookiesPathPos)
        {
            if(*szCookiesPathPos != '/')
            {
                break;
            }
            szCookiesPathPos++;
        }
    }

    // If we've made it to the end of the cookie path,
    // and if we're at the end of the url path or
    // on a path separator, we match

    if(*szCookiesPathPos == '\0' &&
       (*szUrlPathPos == '\0' || *szUrlPathPos == '/'))
    {
        bResult = TRUE;
    }
                                          
    return bResult;
}


HXBOOL
HXCookies::IsCookieEnabled()
{
    HXBOOL	bResult = TRUE;

    ReadPrefBOOL(m_pContext, "CookiesEnabled", bResult);
    
    return bResult;
}

void	    
HXCookies::UpdateModificationTime(const char* pFile, time_t& lastModification)
{
#ifdef _OPENWAVE
    // XXXSAB implement this!!!
#else
    struct stat status;

    if (!pFile)
    {
	goto cleanup;
    }

#ifndef _VXWORKS
    if (0 == stat(pFile, &status))
    {
	lastModification = status.st_mtime;
    }
#endif /* _VXWORKS */

cleanup:

    return;
#endif
}

HXBOOL	    
HXCookies::IsCookieFileModified(const char* pFile, time_t lastModification)
{
#ifdef _OPENWAVE
    // XXXSAB implement this!!!
    return FALSE;
#else
    HXBOOL	bResult = FALSE;
    struct stat	status;

    if (!pFile)
    {
	goto cleanup;
    }

#ifndef _VXWORKS
    if (0 == stat(pFile, &status) &&
	status.st_mtime != lastModification)
    {
	bResult = TRUE;
    }
#endif /* _VXWORKS */

cleanup:

    return bResult;
#endif
}

HX_RESULT
HXCookies::MergeCookieList(CHXSimpleList* pFromList, CHXSimpleList* pToList)
{
    HX_RESULT	    hr = HXR_OK;
    CookieStruct*   pCookie = NULL;
    CHXSimpleList::Iterator  i;

    if (!pFromList)
    {
	goto cleanup;
    }

    for (i = pFromList->Begin(); HXR_OK == hr && i != pFromList->End(); ++i)
    {
	pCookie = (CookieStruct*) (*i);
	
	if (pCookie->bMemoryOnly && !WasCookieAdded(pToList, pCookie))
	{
	    CookieStruct* pNewCookie = new CookieStruct;
	    pNewCookie->pPath = new CHXString(*pCookie->pPath);
	    pNewCookie->pHost = new CHXString(*pCookie->pHost);
	    pNewCookie->pCookieName = new CHXString(*pCookie->pCookieName);
	    pNewCookie->pCookieValue = new CHXString(*pCookie->pCookieValue);
	    pNewCookie->expires = pCookie->expires;
	    pNewCookie->bIsDomain = pCookie->bIsDomain;
	    pNewCookie->bMemoryOnly = pCookie->bMemoryOnly;
	    hr = AddCookie(pNewCookie, pToList);
	}
    }

cleanup:

    return hr;
}

STDMETHODIMP	
HXCookies::SyncRMCookies(HXBOOL bSave)
{
    HX_RESULT	    hr = HXR_OK;
    CHXSimpleList*  pNewRMCookies = NULL;
    CookieStruct* pCookie = NULL;
    HXBOOL bCookieFileModified = FALSE;

    if (!m_bInitialized)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    bCookieFileModified = IsCookieFileModified(m_pRMCookiesPath, m_lastRMCookiesFileModification);
    if (bCookieFileModified)
    {
	if (HXR_OK == OpenCookies(m_pRMCookiesPath, TRUE, pNewRMCookies))
	{
            UpdateModificationTime(m_pRMCookiesPath, m_lastRMCookiesFileModification);

	    if (m_bSaveCookies)
	    {
		MergeCookieList(m_pRMCookies, pNewRMCookies);
	    }		
	     
	    ResetCookies(m_pRMCookies);
	    HX_DELETE(m_pRMCookies);

	    m_pRMCookies = pNewRMCookies;
	}	    	    
    }

    if (bSave && m_bSaveCookies)
    {
	if(m_pRMCookies && bCookieFileModified && pNewRMCookies == NULL)
	{
	    //this means there are no more cookies on disk, and we need to look at what's in memory, and keep
	    //only cookies that have bMemoryOnly set
	    pNewRMCookies = new CHXSimpleList;

	    while (m_pRMCookies && m_pRMCookies->GetCount() > 0 && pNewRMCookies)
	    {
	    	pCookie = (CookieStruct*) m_pRMCookies->RemoveHead();
		if(pCookie->bMemoryOnly)
		{
		    CookieStruct* pNewCookie = new CookieStruct;
		    pNewCookie->pPath = new CHXString(*pCookie->pPath);
		    pNewCookie->pHost = new CHXString(*pCookie->pHost);
		    pNewCookie->pCookieName = new CHXString(*pCookie->pCookieName);
		    pNewCookie->pCookieValue = new CHXString(*pCookie->pCookieValue);
		    pNewCookie->expires = pCookie->expires;
		    pNewCookie->bIsDomain = pCookie->bIsDomain;
		    pNewCookie->bMemoryOnly = pCookie->bMemoryOnly;
		    AddCookie(pNewCookie, pNewRMCookies);
		}
	    	HX_DELETE(pCookie);
	    }
	    HX_DELETE(m_pRMCookies);
	    m_pRMCookies = pNewRMCookies;
	}
	hr = SaveCookies();
    }

cleanup:

    return hr;
}

#ifdef _TEST
void
HXCookies::DumpCookies(void)
{
    CHXSimpleList* pCookies = NULL;
    CookieStruct*  pCookie = NULL;
    CHXSimpleList::Iterator  i;

    for (int l = 0; l < 3; l++)
    {
	switch (l)
	{
	case 0:
	    pCookies = m_pNSCookies;
	    break;
	case 1:
	    pCookies = m_pFFCookies;
	    break;
	case 2:
	    pCookies = m_pRMCookies;
	    break;
	default:
	    break;
	}

	if (pCookies)
	{
	    printf("Total cookies: %lu\n", pCookies->GetCount());
    
	    for (i = pCookies->Begin(); i != pCookies->End(); ++i)
	    {
		pCookie = (CookieStruct*) (*i);
		
		printf("%s\t%s\t%s\n",
		       (const char*)*(pCookie->pHost),
		       (const char*)*(pCookie->pCookieName),
		       (const char*)*(pCookie->pCookieValue));
	    }
	}
    }

    return;
}
#endif // _TEST




//IRPCookies
STDMETHODIMP HXCookies::GetExpiredCookies(const char* pHost,
    				    const char*	       pPath,
    				    REF(IHXBuffer*)   pCookies)
{
    HX_RESULT	    hr = HXR_OK;
#ifdef _WINDOWS
    char*	    cp = NULL;
    char*	    pComma = NULL;
    char*	    pEqualSign = NULL;
    int		    host_length = 0;
    int		    path_length = 0;
    HXBOOL	    bAdded = FALSE;
    UINT32	    dwSize = 0;    
    CookieStruct*   pNewCookie = NULL;
    char*	    pURL = NULL;
#endif
    int	            l = 0;
    char*	    pData = NULL;
    char*           pPathNew = NULL;
    int		    domain_length = 0;
    time_t	    cur_time = time(NULL);
    CookieStruct*   pTempCookie = NULL;
    CHXSimpleList*  pCookiesFound1 = NULL;
    CHXSimpleList*  pCookiesList = NULL;
    IHXValues*	    pValues = NULL;
    CHXSimpleList::Iterator  i;
    CHXString       cHostCopy;
    INT32           lColon;
    
    pCookies = NULL;

    if (!IsCookieEnabled())
    {
	goto cleanup;
    }

    if (!m_bInitialized)
    {
	hr = Initialize();

	if (HXR_OK != hr)
	{
	    goto cleanup;
	}
    }
    else
    {
	SyncRMCookies(FALSE);
    }

    if (!pHost || !m_pCookiesHelper)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    // return string to build    
    hr = CreateValuesCCF(pValues, m_pContext);
    if (HXR_OK != hr)
    {
	goto cleanup;
    }

    cHostCopy = pHost;
    lColon    = cHostCopy.Find(':');
    if (lColon >= 0)
    {
	cHostCopy = cHostCopy.Left(lColon);
    }
    cHostCopy.MakeLower();

    // search for all cookies(Netscape/Firefox only for now)
    for (l = 0; l < 3; l++)
    {
	switch (l)
	{
	case 0:
	    pCookiesList = m_pRMCookies;
	    break;
	case 1:
	    pCookiesList = m_pNSCookies;
	    break;
	case 2:
	    pCookiesList = m_pFFCookies;
	    break;
	default:
	    break;
	}

	if (!pCookiesList)
	{
	    continue;
	}

	for (i = pCookiesList->Begin(); i != pCookiesList->End(); ++i)
	{
	    pTempCookie = (CookieStruct*) (*i);
    
	    if (!pTempCookie->pHost)
	    {
		continue;
	    }

	    // check the host or domain first
	    if(pTempCookie->bIsDomain)
	    {
		domain_length = pTempCookie->pHost->GetLength();

		CHXString cDomainCopy(*(pTempCookie->pHost));
		cDomainCopy.MakeLower();

		// Now we compare the domain (from the cookie itself) with
		// the rightmost characters of the host. For instance,
		// a domain of ".bar.com" would match with a host (passed in)
		// of "foo.bar.com", "www.bar.com", etc. but would NOT match
		// a host of "bar.com".
		CHXString cHostRight = cHostCopy.Right(cDomainCopy.GetLength());
		if (cHostRight != cDomainCopy)
		{
		    // no match
		    continue;
		}
	    }
	    else if(strcasecmp((const char*)*(pTempCookie->pHost), pHost))
	    {
		// hostname matchup failed.
		continue;
	    }

	    // shorter strings always come last so there can be no
	    // ambiquity						     	      
	    if(pTempCookie->pPath && 
	       !strncmp(pPath, (const char*)*(pTempCookie->pPath), pTempCookie->pPath->GetLength()))
	    {
		// check for expired cookies
		if(pTempCookie->expires && (pTempCookie->expires < cur_time))
		{
		    if (!pCookiesFound1)
		    {
		        pCookiesFound1 = new CHXSimpleList();
		    }
		    pCookiesFound1->AddTail(pTempCookie);
		}
	    }
	}
    }

#ifdef _WINDOWS
    if (!_pInternetGetCookie || m_bMemoryOnly)
    {
	goto cleanup;
    }

    host_length = strlen(pHost);

    FixPath(pPath, pPathNew);
    if (pPathNew)
    {
	path_length = strlen(pPathNew);
    }

    pURL = new char[host_length + path_length + strlen(IE_COOKIE_URLSTRING)];
    sprintf(pURL, IE_COOKIE_URLSTRING, pHost, pPathNew); /* Flawfinder: ignore */

    if (_pInternetGetCookie(pURL, NULL, pData, &dwSize) && !pData && dwSize)
    {
	pData = new char[dwSize+1];

	if (!_pInternetGetCookie(pURL, NULL, pData, &dwSize))
	{
	    goto cleanup;
	}

	cp = pData;

	while (pComma = (char*) ::HXFindChar(cp, ';'))
	{
	    *pComma = '\0';
	    pComma++;

	    if (pEqualSign = (char*) ::HXFindChar(cp, '='))
	    {
		*pEqualSign = '\0';
		pEqualSign++;

		pNewCookie = new CookieStruct;
		bAdded = FALSE;

		// copy
		pNewCookie->pCookieValue = new CHXString(pEqualSign);
		pNewCookie->pCookieName = new CHXString(cp);
		pNewCookie->pPath = NULL;
		pNewCookie->pHost = NULL;
		pNewCookie->expires = 0;
		pNewCookie->bIsDomain = FALSE;
		pNewCookie->bMemoryOnly = FALSE;

		if (!WasCookieAdded(pCookiesFound1, pNewCookie))
		{
		    //if (!pCookiesFound2)
		    //{
		    //	pCookiesFound2 = new CHXSimpleList();
		    //	pCookiesFound2->AddTail(pNewCookie);
		    //	bAdded = TRUE;
		    //}
		    //else if (!WasCookieAdded(pCookiesFound2, pNewCookie))
		    //{
		    //	pCookiesFound2->AddTail(pNewCookie);
		    //	bAdded = TRUE;
		    //}
		}
		
		if (!bAdded)
		{
		    HX_DELETE(pNewCookie);
		}
	    }

	    cp = pComma;
	}

	if (pEqualSign = (char*) ::HXFindChar(cp, '='))
	{
	    *pEqualSign = '\0';
	    pEqualSign++;

	    pNewCookie = new CookieStruct;
	    bAdded = FALSE;

	    // copy
	    pNewCookie->pCookieValue = new CHXString(pEqualSign);
	    pNewCookie->pCookieName = new CHXString(cp);
	    pNewCookie->pPath = NULL;
	    pNewCookie->pHost = NULL;
	    pNewCookie->expires = 0;
	    pNewCookie->bIsDomain = FALSE;
	    pNewCookie->bMemoryOnly = FALSE;

	    if (!WasCookieAdded(pCookiesFound1, pNewCookie))
	    {
		//if (!pCookiesFound2)
		//{
		//    pCookiesFound2 = new CHXSimpleList();
		//    pCookiesFound2->AddTail(pNewCookie);
		//    bAdded = TRUE;
		//}
		//else if (!WasCookieAdded(pCookiesFound2, pNewCookie))
		//{
		//    pCookiesFound2->AddTail(pNewCookie);
		//    bAdded = TRUE;
		//}
	    }

	    if (!bAdded)
	    {
		HX_DELETE(pNewCookie);
	    }
	}
    }
#endif /* _WINDOWS */

cleanup:

    if (pCookiesFound1)
    {
	for (i = pCookiesFound1->Begin(); i != pCookiesFound1->End(); ++i)
	{
	    pTempCookie = (CookieStruct*) (*i);

	    if(pTempCookie->pCookieName && pTempCookie->pCookieValue)
	    {
		::SaveStringToHeader(pValues,
				     (const char*)*(pTempCookie->pCookieName),
				     (char*)(const char*)*(pTempCookie->pCookieValue),
				     m_pContext);
	    }
	}
    }

    //if (pCookiesFound2)
    //{
    //	for (i = pCookiesFound2->Begin(); i != pCookiesFound2->End(); ++i)
    //	{
    //	    pTempCookie = (CookieStruct*) (*i);
    //
    //	    if(pTempCookie->pCookieName && pTempCookie->pCookieValue)
    //	    {
    //		::SaveStringToHeader(pValues,
    //				     (const char*)*(pTempCookie->pCookieName),
    //				     (char*)(const char*)*(pTempCookie->pCookieValue));
    //	    }
    //
    //	    HX_DELETE(pTempCookie);
    //	}
    //}

    if (m_pCookiesHelper)
    {
	hr = m_pCookiesHelper->UnPack(pValues, pCookies);
    }

    HX_DELETE(pCookiesFound1);
    //HX_DELETE(pCookiesFound2);
    HX_VECTOR_DELETE(pPathNew);
    HX_VECTOR_DELETE(pData);

#ifdef _WINDOWS
    HX_VECTOR_DELETE(pURL);
#endif

    HX_RELEASE(pValues);
   
    return hr;
}

HX_RESULT HXCookies::SecureCookies()
{
    return HXR_OK;
}

HX_RESULT HXCookies::CheckCookies()
{
    return HXR_OK;
}

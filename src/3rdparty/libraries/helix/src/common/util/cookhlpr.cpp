/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cookhlpr.cpp,v 1.11 2008/01/18 04:54:26 vkathuria Exp $
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

#include "hlxclib/string.h"
#include "hlxclib/ctype.h"
//#include <stdio.h>

#include "hxcom.h"
#include "hxtypes.h"
#include "hxresult.h"
#include "chxpckts.h"
#include "hxstrutl.h"
#include "ihxcookies.h"
#include "cookhlpr.h"
#include "pckunpck.h"

#if defined(_AIX )
#include <ctype.h>      // for isspace()
#endif

HXCookiesHelper::HXCookiesHelper(IUnknown* pContext)
	: m_lRefCount(0)
        , m_pContext(pContext)
{
    if (m_pContext) m_pContext->AddRef();
}

HXCookiesHelper::~HXCookiesHelper()
{
    HX_RELEASE(m_pContext);
}

STDMETHODIMP
HXCookiesHelper::QueryInterface(REFIID riid, void**ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), this },
		{ GET_IIDHANDLE(IID_IHXCookiesHelper), (IHXCookiesHelper*) this },
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
HXCookiesHelper::AddRef()
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
HXCookiesHelper::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
HXCookiesHelper::Pack(IHXBuffer*	pCookies,
		       REF(IHXValues*)	pCookiesHeader)
{
    HX_RESULT	    hr = HXR_OK;
    char*	    ptr = NULL;
    char*	    semi_colon = NULL;
    char*	    set_cookie_header = NULL;
    char*	    equal = NULL;    
    char*	    date = NULL;
    char*	    path_from_header = NULL;
    char*	    domain_from_header = NULL;
    char*	    name_from_header = NULL;
    char*	    cookie_from_header = NULL;

    if (!pCookies)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    CreateValuesCCF(pCookiesHeader, m_pContext);

    if (!pCookiesHeader)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    set_cookie_header = (char*)pCookies->GetBuffer();
    	
    // terminate at any carriage return or linefeed
    for(ptr = set_cookie_header; *ptr; ptr++)
    {
	if(*ptr == LF || *ptr == CR) 
	{
	    *ptr = '\0';
	    break;
	}
    }

    // parse path and expires attributes from header if
    // present
    semi_colon = strchr(set_cookie_header, ';');

    if(semi_colon)
    {
	// truncate at semi-colon and advance 	 
	*semi_colon++ = '\0';

	// look for the path attribute	 
	ptr = StrStrCaseInsensitive(semi_colon, "path=");

	if(ptr) 
	{	    	    
	    // allocate more than we need
	    ::StrAllocCopy(path_from_header, ::StripLine(ptr+5));

	    // terminate at first space or semi-colon	     
	    for(ptr = path_from_header; *ptr != '\0'; ptr++)
	    {
		if(IS_SPACE(*ptr) || *ptr == ';' || *ptr == ',') 
		{
		    *ptr = '\0';
		    break;
		}
	    }
	    
	    hr = ::SaveStringToHeader(pCookiesHeader, "path", path_from_header, m_pContext);
	    if (HXR_OK != hr)
	    {
		goto cleanup;
	    }	  
    	}

	// look for a domain attribute
        ptr = StrStrCaseInsensitive(semi_colon, "domain=");

        if(ptr) 
	{
	    // allocate more than we need
	    ::StrAllocCopy(domain_from_header, ::StripLine(ptr+7));

            // terminate at first space or semi-colon
            for(ptr = domain_from_header; *ptr != '\0'; ptr++)
	    {
                if(IS_SPACE(*ptr) || *ptr == ';' || *ptr == ',') 
		{
                    *ptr = '\0';
                    break;
		}
	    }

	    hr = ::SaveStringToHeader(pCookiesHeader, "domain", domain_from_header, m_pContext);
	    if (HXR_OK != hr)
	    {
		goto cleanup;
	    }	  
	}

	// now search for the expires header 
	// NOTE: that this part of the parsing
	// destroys the original part of the string
	ptr = StrStrCaseInsensitive(semi_colon, "expires=");

	if(ptr) 
	{
	    date = ptr+8;

	    // terminate the string at the next semi-colon
	    for(ptr = date; *ptr != '\0'; ptr++)
	    {
		if(*ptr == ';') 
		{
		    *ptr = '\0';
		    break;
		}
	    }

	    hr = ::SaveStringToHeader(pCookiesHeader, "expires", date, m_pContext);
	    if (HXR_OK != hr)
	    {
		goto cleanup;
	    }	  
	}
    }
    
    // keep cookies under the max bytes limit
    if(strlen(set_cookie_header) > MAX_BYTES_PER_COOKIE)
    {
	set_cookie_header[MAX_BYTES_PER_COOKIE-1] = '\0';
    }

    // separate the name from the cookie
    equal = strchr(set_cookie_header, '=');

    if(equal) 
    {
	*equal = '\0';
	::StrAllocCopy(name_from_header, ::StripLine(set_cookie_header));
	::StrAllocCopy(cookie_from_header, ::StripLine(equal+1));
    } 
    else 
    {
	::StrAllocCopy(name_from_header, "");
	::StrAllocCopy(cookie_from_header, ::StripLine(set_cookie_header));  
    }

    hr = ::SaveStringToHeader(pCookiesHeader, "name", name_from_header, m_pContext);
    if (HXR_OK != hr)
    {
	goto cleanup;
    }	  

    hr = ::SaveStringToHeader(pCookiesHeader, "value", cookie_from_header, m_pContext);
    if (HXR_OK != hr)
    {
	goto cleanup;
    }	  

cleanup:

    if (HXR_OK != hr)
    {
	HX_RELEASE(pCookiesHeader);
    }

    HX_VECTOR_DELETE(path_from_header);
    HX_VECTOR_DELETE(domain_from_header);
    HX_VECTOR_DELETE(name_from_header);
    HX_VECTOR_DELETE(cookie_from_header);

    return hr;
}

STDMETHODIMP
HXCookiesHelper::UnPack(IHXValues*	    pCookiesHeader,
			 REF(IHXBuffer*)   pCookies)
{
    HX_RESULT	hr = HXR_OK;
    const char*	pPropName=NULL;
    CHXString*	pCookiesValue = NULL;
    IHXBuffer* pBuffer=NULL;

    if (!pCookiesHeader)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    if (!(pCookiesValue = new CHXString()))
    {
	hr = HXR_OUTOFMEMORY;
	goto cleanup;
    }

    if (HXR_OK == pCookiesHeader->GetFirstPropertyBuffer(pPropName, pBuffer) &&
	pPropName && pBuffer)
    {
	*pCookiesValue += pPropName;	
	*pCookiesValue += "=";
	*pCookiesValue += pBuffer->GetBuffer();
    }    
    HX_RELEASE(pBuffer);
        
    while (HXR_OK == pCookiesHeader->GetNextPropertyBuffer(pPropName, pBuffer) &&
	   pPropName && pBuffer)
    {
	*pCookiesValue += "; ";
	*pCookiesValue += pPropName;	
	*pCookiesValue += "=";
	*pCookiesValue += pBuffer->GetBuffer();

	HX_RELEASE(pBuffer);
    }

    if (pCookiesValue && pCookiesValue->GetLength())
    {
        CreateBufferCCF(pCookies, m_pContext);
        if (pCookies)
        {
            pCookies->Set((const unsigned char*)(const char*)*(pCookiesValue), pCookiesValue->GetLength() + 1);
        }
    }

cleanup:

    if (HXR_OK != hr)
    {
	HX_RELEASE(pCookies);
    }

    HX_DELETE(pCookiesValue);

    return hr;
}

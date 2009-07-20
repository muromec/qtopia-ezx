/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxurl.cpp,v 1.54 2009/03/13 19:33:56 gvalverde Exp $
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
#include "hlxclib/stdlib.h"
#include "hlxclib/ctype.h"

#include "hxcom.h"
#include "hxtypes.h"
#include "hxresult.h"
#include "hxcomm.h"
#include "tparse.h"
#include "dbcs.h"
#include "protdefs.h"
#include "hxstrutl.h"
#include "pathutil.h"
#include "hxslist.h"
#include "hxurl.h"
#include "ihxpckts.h"
#include "chxminiccf.h"
#include "hxdir.h" // OS_SEPARATOR_CHAR
#include "hxurlutil.h"
#include "hxescapeutil.h"

#if defined(HELIX_FEATURE_PREFERENCES)
#include "hxprefutil.h"
#endif // HELIX_FEATURE_PREFERENCES 
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

// Client should use the other constructor which takes pContext
#ifndef HELIX_FEATURE_CLIENT
// this prevents linking in CHXMiniCCF when the first ctor is not used
static
IHXCommonClassFactory* CreateCCF() { return new CHXMiniCCF(); }

CHXURL::CHXURL (const char* pszURL)
: m_LastError (HXR_OK)
, m_unProtocol(fileProtocol)
, m_unDefaultPort(0)
, m_pProperties (NULL)
, m_pOptions (NULL)
, m_pCCF(CreateCCF())
, m_pContext(NULL)
{
    if (m_pCCF)
    {
        m_pCCF->AddRef();
    }

    ConstructURL(pszURL);
}
#endif

CHXURL::CHXURL (const char* pszURL, IUnknown* pContext)
: m_LastError (HXR_OK)
, m_unProtocol(fileProtocol)
, m_unDefaultPort(0)
, m_pProperties (NULL)
, m_pOptions (NULL)
, m_pCCF(0)
, m_pContext(pContext)
{
    if (pContext)
    {
        pContext->AddRef();    
        pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCCF);
    }

    ConstructURL(pszURL);
}

CHXURL::~CHXURL ()
{
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pProperties);
    HX_RELEASE(m_pOptions);
    HX_RELEASE(m_pCCF);
}

CHXURL::CHXURL(const CHXURL& rhs)
: m_LastError (HXR_OK)
, m_unProtocol (fileProtocol)
, m_pProperties (NULL)
, m_pOptions (NULL)
, m_pCCF(rhs.m_pCCF)
, m_pContext(rhs.m_pContext)
{
    if (m_pCCF)
    {
        m_pCCF->AddRef();
    }

    if (m_pContext)
    {
        m_pContext->AddRef();
    }
    
    ConstructURL(rhs.GetURL());
}

CHXURL& CHXURL::operator=(const CHXURL& rhs)
{
    if (&rhs != this)
    {
        // Clean out old data
        HX_RELEASE(m_pProperties);
        HX_RELEASE(m_pOptions);
        HX_RELEASE(m_pCCF);
        HX_RELEASE(m_pContext);
        
        m_LastError = HXR_OK;
        m_unProtocol = fileProtocol;

        ConstructURL(rhs.GetURL());
        m_pCCF = rhs.m_pCCF;

        if (m_pCCF)
        {
            m_pCCF->AddRef();
        }
        
        m_pContext = rhs.m_pContext;
        if (m_pContext)
        {
            m_pContext->AddRef();
        }
    }

    return *this;
}



// return TRUE if scheme requires handling via protocol 
// code, as opposed to handling via a filesystem plugin
HXBOOL CHXURL::IsNetworkProtocol()
{
    switch(m_unProtocol)
    {
    case pnmProtocol:
    case rtspProtocol:
    case helixSDPProtocol:
        // these are not handled via filesystem plugin
        return TRUE;
        break;
    default:
        break;
    }

    return FALSE;
}

HXBOOL CHXURL::IsRelativeURL()
{
    return (m_url.GetType() == HXURLRep::TYPE_RELPATH ? TRUE : FALSE);
}

// return TRUE if we know for sure that the
// protocol require a valid host component
HXBOOL CHXURL::ProtocolRequiresHost()
{
    switch(m_unProtocol)
    {
    case httpProtocol:
    case pnmProtocol:
    case rtspProtocol:
        return TRUE;
        break;
    default:
        break;
    }

    // can't be sure
    return FALSE;
}


void CHXURL::ParseProtocolInfo()
{
    HX_ASSERT(m_pProperties);

    HXURLUtil::ProtocolInfo info = HXURLUtil::GetProtocolInfo(m_url.Scheme());
    m_unDefaultPort = info.defaultPort;

    switch(info.type)
    {
    case HXURLUtil::ProtocolInfo::SCHEME_HTTP:
    case HXURLUtil::ProtocolInfo::SCHEME_HTTPS:
        m_unProtocol = httpProtocol;
        break;
    case HXURLUtil::ProtocolInfo::SCHEME_PNM:
        m_unProtocol = pnmProtocol;
        break;
    case HXURLUtil::ProtocolInfo::SCHEME_RTSP:
    case HXURLUtil::ProtocolInfo::SCHEME_RTSPT:
    case HXURLUtil::ProtocolInfo::SCHEME_RTSPU:
        m_unProtocol = rtspProtocol;
        break;
    case HXURLUtil::ProtocolInfo::SCHEME_HELIX_SDP:
        m_unProtocol = helixSDPProtocol;
        break;
    case HXURLUtil::ProtocolInfo::SCHEME_FILE:
        m_unProtocol = fileProtocol;
        break;
    case HXURLUtil::ProtocolInfo::SCHEME_PROD:
        m_unProtocol = prodProtocol;
        break;
    case HXURLUtil::ProtocolInfo::SCHEME_MMS:
        m_unProtocol = mmsProtocol;
        break;
    default:
        m_unProtocol = unknownProtocol;
        break;
    }

    // save scheme/protocol info to header
    m_pProperties->SetPropertyULONG32(PROPERTY_PROTOCOL, (ULONG32)m_unProtocol);

    HX_ASSERT(!m_url.Scheme().IsEmpty());
    if (!m_url.Scheme().IsEmpty())
    {
        ::SaveStringToHeader(m_pProperties, PROPERTY_SCHEME, m_url.Scheme(), m_pCCF);
    }

}



//
// 'pszURL' assumed to be properly escaped per RFC 2396
//
void CHXURL::ConstructURL(const char* pszURL)
{
    HX_ASSERT(pszURL != NULL);

    m_actualURL = pszURL;

    if (!pszURL)
    {
        HX_ASSERT(false);
        m_LastError = HXR_INVALID_PATH;
        return;
    }

    if (!m_pCCF)
    {
        m_LastError = HXR_UNEXPECTED;
        return;
    }

    HX_ASSERT(!m_pProperties && !m_pOptions);
    m_pCCF->CreateInstance(CLSID_IHXValues, (void**)&m_pProperties);
    m_pCCF->CreateInstance(CLSID_IHXValues, (void**)&m_pOptions);

    if( !m_pProperties || !m_pOptions )
    {
        m_LastError = HXR_UNEXPECTED;
        return;
    }

    // make copy so that we can possibly fixup bad url input before parsing
    CHXString strURL(pszURL);

    HXURLUtil::DoDollarFixup(strURL);
    
    // parse the url
    m_url = HXURLRep(strURL);

    // quit if scheme (minimum required) not parsed
    if (m_url.GetParseFlags() & HXURLRep::PF_INVALID_SCHEME)
    {
        m_LastError = HXR_INVALID_PROTOCOL;
        return;
    }

    if (m_url.Scheme().IsEmpty())
    {
        // url must be absolute
        m_LastError = HXR_INVALID_PROTOCOL;
        return;
    }

    // determine whether protocol is network or local
    ParseProtocolInfo();
    if (fileProtocol == m_unProtocol)
    {
        HXURLUtil::DoFileURLFixup(m_url);
        if(m_url.Path().IsEmpty())
        {
            m_LastError = HXR_INVALID_URL_PATH;
            return;
        }
    }
    else
    {
        // in case we have weird path ("foo/bar/../file.rm")
        m_url.Normalize();

        // ensure path and query components are properly escaped
        HXEscapeUtil::EnsureEscapedURL(m_url);
    }

    // save fixed-up url to properties
    HX_ASSERT(!m_url.String().IsEmpty());
    ::SaveStringToHeader(m_pProperties, PROPERTY_URL, m_url.String(), m_pCCF);

    // no need to further parse opaque form url
    if (m_url.GetType() != HXURLRep::TYPE_OPAQUE)
    {
        HX_ASSERT(helixSDPProtocol != m_unProtocol);
        ParseBody (m_url.String());
    }

}



HX_RESULT CHXURL::ParseBody (const char* pszURL)
{
    // note: url components are saved based on post-fixup URL (pszURL)
    HX_ASSERT(m_LastError == HXR_OK);
    bool ignoreQuery = false;

    if (!m_url.Fragment().IsEmpty())
    {
        ::SaveStringToHeader(m_pProperties, PROPERTY_FRAGMENT, m_url.Fragment(), m_pCCF);
    }

    if (fileProtocol != m_unProtocol && unknownProtocol != m_unProtocol)
    {
        // user:pw@
        if (!m_url.UserInfo().IsEmpty())
        {
            CHXString userInfo = m_url.UserInfo();

            CHXString user, password;

            INT32 idxColon = userInfo.Find(':');
            if( -1 != idxColon)
            {
                user = userInfo.Left(idxColon);
                password = userInfo.Mid(idxColon + 1);
            }
            else
            {
                user = userInfo;
            }


            if (!user.IsEmpty())
            {
                ::SaveStringToHeader(m_pProperties, PROPERTY_USERNAME, user, m_pCCF);
            }

            if (!password.IsEmpty())
            {
                ::SaveStringToHeader(m_pProperties, PROPERTY_PASSWORD, password, m_pCCF);
            }
        }


        // host
        if (m_url.GetType() == HXURLRep::TYPE_NETPATH)
        {
            // error only if bad ipv6 or empty host (let other bad hosts slip)
            if ( ProtocolRequiresHost() && (m_url.Host().IsEmpty() || m_url.GetParseFlags() & HXURLRep::PF_INVALID_IPV6) )
            {
                m_LastError = HXR_INVALID_URL_HOST;
                return m_LastError;
            }
            if (!m_url.Host().IsEmpty())
            {
                ::SaveStringToHeader(m_pProperties, PROPERTY_HOST, m_url.Host(), m_pCCF);
            }
 
            // port
            UINT16 port = (UINT16) m_url.Port();
            if(port == 0)
            {
                port = m_unDefaultPort;
            }

            if( port != 0)
            {
                m_pProperties->SetPropertyULONG32(PROPERTY_PORT, port);
            }
        }

    }

    HXBOOL bIgnoreOptions = FALSE;
#if defined(HELIX_FEATURE_PREFERENCES)
    if(m_pContext)
    {    
        ReadPrefBOOL(m_pContext, "IgnoreURLOptions", bIgnoreOptions);
    }
#endif //HELIX_FEATURE_PREFERENCES
    
    // collect other options info if it has and if allowed by preferences
    if (!m_url.Query().IsEmpty() && !bIgnoreOptions)
    {
        HX_RELEASE(m_pOptions);
        HX_RESULT hr = HXURLUtil::GetOptions(m_pCCF, m_url, m_pOptions);
        if(FAILED(hr))
        {
            if(hr != HXR_OUTOFMEMORY)
            {
                // bad query - just ignore query
                ignoreQuery = true;
            }
            else
            {
                m_LastError = hr;
            }
        }
    }

    if (!m_url.Path().IsEmpty()     || 
        !m_url.Query().IsEmpty()    ||
        !m_url.Fragment().IsEmpty())
    {
        // XXXLCM is PROPERTY_RESOURCE stores everything after host
        CHXString strResource = m_url.Path();
        if (!ignoreQuery && !m_url.Query().IsEmpty())
        {
            strResource += "?";
            strResource += m_url.Query();
        }
        if (!m_url.Fragment().IsEmpty())
        {
            strResource += "#";
            strResource += m_url.Fragment();
        }
        ::SaveStringToHeader(m_pProperties, PROPERTY_RESOURCE, strResource, m_pCCF);

        if (!m_url.Path().IsEmpty())
        {
            // note: paths are stored *unescaped* for convenience (very likely use/need)

            // PROPERTY_FULLPATH is the full filename including path, e.g., "the/path/foo.rm"
            CHXString strFullPath = HXEscapeUtil::UnEscape(m_url.Path());
            ::SaveStringToHeader(m_pProperties, PROPERTY_FULLPATH, strFullPath, m_pCCF);

            // Get the extension from the full path. Get the last '.'
	    INT32 lPeriod = strFullPath.ReverseFind('.');
	    if (lPeriod >= 0)
	    {
                // Get the extension by itself
	        CHXString cExtension = strFullPath.Right(strFullPath.GetLength() - lPeriod - 1);
                if (cExtension.GetLength() > 0)
                {
                    // Convert to lower case
                    cExtension.MakeLower();
                    // Set the PROPERTY_EXTENSION
                    ::SaveStringToHeader(m_pProperties, PROPERTY_EXTENSION, cExtension, m_pCCF);
                }
            }

            // PROPERTY_PATH is the directory containing the file, e.g., "the/path/"
            CHXString folder = HXEscapeUtil::UnEscape(HXPathUtil::GetNetPathDirectory(m_url.Path()));
            ::SaveStringToHeader(m_pProperties, PROPERTY_PATH, folder, m_pCCF);
        }
        else
        {
            ::SaveStringToHeader(m_pProperties, PROPERTY_FULLPATH, "/", m_pCCF);
            ::SaveStringToHeader(m_pProperties, PROPERTY_PATH, "/", m_pCCF);
        }
    }
    else if (m_unProtocol == rtspProtocol)
    {
        ::SaveStringToHeader(m_pProperties, PROPERTY_RESOURCE, "\0", m_pCCF);
        ::SaveStringToHeader(m_pProperties, PROPERTY_FULLPATH, "\0", m_pCCF);
        ::SaveStringToHeader(m_pProperties, PROPERTY_PATH, "\0", m_pCCF);
    }

    return m_LastError;
}


static HXBOOL
IsNumber(const char* pszValue)
{
    while (*pszValue != '\0')
    {
        if (!isdigit(*pszValue))
        {
            return FALSE;
        }

        pszValue++;
    }

    return TRUE;
}


void
CHXURL::AddOption(char* pKey, char* pValue)
{
    // trim off leading/tailing spaces
    CHXString key(pKey);
    CHXString value(pValue);

    key.TrimLeft();
    key.TrimRight();
    value.TrimLeft();
    value.TrimRight();

    // save to the header
    if (IsNumber(value))
    {
        m_pOptions->SetPropertyULONG32(key, (ULONG32) atol(value));
    }
    else
    {
        IHXBuffer*  pBuffer = NULL;

        if (m_pCCF)
        {
            m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);
        }

        if (pBuffer)
        {
            pBuffer->Set((UCHAR*)(const char*)value, value.GetLength() + 1);

            m_pOptions->SetPropertyBuffer(key, (IHXBuffer*) pBuffer);

            pBuffer->Release();
        }
    }
}

void
CHXURL::AddOption(char* pKey, UINT32 ulValue)
{
    CHXString key(pKey);
    key.TrimLeft();
    key.TrimRight();
    m_pOptions->SetPropertyULONG32(key, ulValue);
}

IHXValues*
CHXURL::GetProperties(void)
{
    if (m_pProperties)
    {
        m_pProperties->AddRef();
    }

    return m_pProperties;
}

IHXValues*
CHXURL::GetOptions(void)
{
    if (m_pOptions)
    {
        m_pOptions->AddRef();
    }

    return m_pOptions;
}

char*
CHXURL::GetAltURL(HXBOOL& bDefault)
{
    char*       pAltURL = NULL;

    bDefault = FALSE;

    if (HXR_OK == m_LastError)
    {
        IHXBuffer* pBuf = 0;
        // retrieve Alt-URL if it exists in the option list
        if (HXR_OK == m_pOptions->GetPropertyBuffer("altURL", pBuf) && pBuf)
        {
            // allocate mem. for m_pszAltURL
            pAltURL = new char[pBuf->GetSize()];
            SafeStrCpy(pAltURL, (const char*)pBuf->GetBuffer(), pBuf->GetSize());
            HX_RELEASE(pBuf);
        }
        else if (!m_url.Host().IsEmpty())
        {
            if (m_unProtocol == pnmProtocol ||
                m_unProtocol == rtspProtocol)
            {
                // use default alt-url form
                bDefault = TRUE;

                HXURLRep url(HXURLRep::TYPE_NETPATH, "http", "", // no user info
                    m_url.Host(), -1, m_url.Path(), m_url.Query(), m_url.Fragment());

                HX_ASSERT(url.IsValid());
                ::StrAllocCopy(pAltURL, url.String());
            }
        }

    }

    return pAltURL;
}



HX_RESULT
CHXURL::GeneratePrefixRootFragment(const char* pURL, CHXString& urlPrefix,
                                   CHXString& urlRoot, char*& pURLFragment, IUnknown* pContext)
{
    HXBOOL bHasScheme = FALSE;
    HXBOOL bHasHost = FALSE;

    CHXURL urlObj(pURL, pContext);
    IHXValues* pHeader = urlObj.GetProperties();
    if(!pHeader)
    {
        return HXR_FAIL;
    }

    IHXBuffer* pBuffer = 0;
    ULONG32 ulTemp;

    if(HXR_OK == pHeader->GetPropertyBuffer(PROPERTY_SCHEME, pBuffer))
    {
        HX_ASSERT(pBuffer->GetSize() != 0);
        urlPrefix = (const char*)pBuffer->GetBuffer();
        urlPrefix += "://";
        pBuffer->Release();
		bHasScheme = TRUE;
    }
    if(HXR_OK == pHeader->GetPropertyBuffer(PROPERTY_HOST, pBuffer))
    {
        urlPrefix += (const char*)pBuffer->GetBuffer();
        pBuffer->Release();
        bHasHost = TRUE;
    }
    if(HXR_OK == pHeader->GetPropertyULONG32(PROPERTY_PORT, ulTemp))
    {
        char szTemp[10]; /* Flawfinder: ignore */
        SafeSprintf(szTemp, sizeof(szTemp), ":%d", (UINT16)ulTemp);
        urlPrefix += szTemp;
    }

    // set root
    urlRoot = urlPrefix;

    if(bHasHost || bHasScheme)
    {
        urlPrefix += "/";
    }

    if(HXR_OK == pHeader->GetPropertyBuffer(PROPERTY_RESOURCE, pBuffer))
    {
        const char* pResource = (const char*)pBuffer->GetBuffer();
        const char  cDelimiter1  = '/';
        const char  cDelimiter2  = '\\';
        const char  cOSDelimiter = OS_SEPARATOR_CHAR;

        CHXString   strURLWork = pResource;

        char* pFirstChar    = strURLWork.GetBuffer(strURLWork.GetLength());
        char* pLastChar     = NULL;
        char* pOptions      = NULL;

        pOptions = strchr(pFirstChar, '?');

        if (pOptions)
        {
            pLastChar = pOptions -1;
        }
        else
        {
            pLastChar = pFirstChar + strlen(pFirstChar)-1;
        }

        while ((pLastChar > pFirstChar) &&
           (*pLastChar != cDelimiter1) && (*pLastChar != cDelimiter2) &&
           (*pLastChar != cOSDelimiter))
        {
                pLastChar--;
        }
        // If we hit a delimiter before hitting the end, back up one character!
        if(pLastChar > pFirstChar)
        {
            *(++pLastChar) = '\0';

            urlPrefix += pFirstChar;
        }
        pBuffer->Release();
    }

    if(HXR_OK == pHeader->GetPropertyBuffer(PROPERTY_FRAGMENT, pBuffer))
    {
        const char* pFragment = (const char*)pBuffer->GetBuffer();
        pURLFragment = new_string(pFragment);
        pBuffer->Release();
    }

    HX_RELEASE(pHeader);

    return HXR_OK;
}




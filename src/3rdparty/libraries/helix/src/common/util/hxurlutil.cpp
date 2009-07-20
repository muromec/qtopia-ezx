/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#include "hlxclib/ctype.h"
#include "hxcom.h"
#include "hxccf.h"
#include "ihxpckts.h"
#include "hxstring.h"
#include "dbcs.h" //HXFindChar
#include "tparse.h"
#include "hxstrutl.h"
#include "hxstringutil.h"
#include "hxurlrep.h"
#include "pathutil.h"
#include "hxescapeutil.h"
#include "hxurlutil.h"



#define ARRAY_COUNT(a) (sizeof(a)/sizeof(a[0]))
#include "protdefs.h"
#include "hlxclib/string.h"
HXURLUtil::ProtocolInfo HXURLUtil::GetProtocolInfo(const char* pszScheme)
{
    const struct
    {
        const char* psz;
        ProtocolInfo info;
    }
    table[] =
    {
        {"http",        {ProtocolInfo::SCHEME_HTTP,        DEFAULT_HTTP_PORT}  },
        {"chttp",       {ProtocolInfo::SCHEME_HTTP,        DEFAULT_HTTP_PORT}  },
        {"pnm",         {ProtocolInfo::SCHEME_PNM,         DEFAULT_PNA_PORT}   },
        {"rtsp",        {ProtocolInfo::SCHEME_RTSP,        DEFAULT_RTSP_PORT}  },
        {"rtspt",       {ProtocolInfo::SCHEME_RTSPT,       DEFAULT_RTSP_PORT}  },
        {"rtspu",       {ProtocolInfo::SCHEME_RTSPU,       DEFAULT_RTSP_PORT}  },
        {"mms",         {ProtocolInfo::SCHEME_MMS,         DEFAULT_RTSP_PORT}  },
        {"helix-sdp",   {ProtocolInfo::SCHEME_HELIX_SDP,   DEFAULT_RTSP_PORT}  },
        {"https",       {ProtocolInfo::SCHEME_HTTPS,       DEFAULT_HTTPS_PORT} },
        {"file",        {ProtocolInfo::SCHEME_FILE,                         0} },
        {"fd",          {ProtocolInfo::SCHEME_FILE,                         0} },
	{"prod",        {ProtocolInfo::SCHEME_PROD,                         0} },
    };

    ProtocolInfo info = {ProtocolInfo::SCHEME_UNKNOWN, 0};
    for (UINT16 idx = 0; idx < ARRAY_COUNT(table); ++idx)
    {
        if (0 == strcasecmp(pszScheme, table[idx].psz))
        {
            info = table[idx].info;
            break;
        }
    }

    return info;

}


HX_RESULT HXURLUtil::GetOptions(IUnknown* pContext, const HXURLRep& url, IHXValues*& pVal /* out*/,
                                HXBOOL bAllBufferProperties)
{
    HX_RESULT hr = HXR_FAIL;

    HX_ASSERT(pContext);
    HX_ASSERT(url.IsFullyParsed());

    pVal = 0;

    if (!url.Query().IsEmpty())
    {
        IHXCommonClassFactory* pFactory = 0;
        hr = pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&pFactory);
        if (HXR_OK == hr)
        {
            hr = ParseOptions(url.Query(), pFactory, pVal, bAllBufferProperties);
            HX_RELEASE(pFactory);
        }
    }
    return hr;

}


//XXXLCM move
inline
bool IsNumber(const char* pszValue)
{
    for ( ; *pszValue != '\0'; ++pszValue)
    {
	if (!isdigit(*pszValue))
	{
	    return false;
	}
    }
    return true;
}


HX_RESULT HXURLUtil::ParseOptions(const char* pszEscapedQuery, 
                         IHXCommonClassFactory* pFactory,
                         IHXValues*& pVal /*out*/,
                         HXBOOL bAllBufferProperties)
{
    HX_ASSERT(pFactory);

    HX_RESULT hr = pFactory->CreateInstance(IID_IHXValues, (void**)&pVal);
    if(FAILED(hr))
    {
        return hr;
    }

    // look for <key>="<value>" or <key>=<value>

    CHXString strEscapedQuery = pszEscapedQuery;
    UINT32 nNumOptions = strEscapedQuery.CountFields('&');
    for(UINT32 nIndex = 1; nIndex <= nNumOptions; nIndex++)
    {
	CHXString strOption = strEscapedQuery.NthField('&', nIndex);
	INT32 nPos = strOption.Find('=');
	if(nPos == -1)
	{
            // error: expected key termination '='
            hr = HXR_FAIL;
            break;
	}

	CHXString key = strOption.Left(nPos);
	key.TrimRight();
	key.TrimLeft();

        CHXString value = strOption.Mid(nPos + 1);
	value.TrimRight();
	value.TrimLeft();

        // escape possibly URL-encoded query string values
        key = HXEscapeUtil::UnEscape(key);
        value = HXEscapeUtil::UnEscape(value);

	HXBOOL valueIsQuoted = FALSE;
	// remove surrounding ""
	UINT32 nValueLength = value.GetLength();
	if(nValueLength >= 2 && value.GetAt(0) == '"' && value.GetAt(nValueLength - 1) == '"')
	{
	    valueIsQuoted = TRUE;
	    value = value.Mid(1, nValueLength - 2);
	    value.TrimRight();
	    value.TrimLeft();
	}

	// add key value to collection
        if (!key.CompareNoCase("Start")
	    || !key.CompareNoCase("End")
	    || !key.CompareNoCase("Delay")
	    || !key.CompareNoCase("Duration"))
	{
            hr = pVal->SetPropertyULONG32(key, (ULONG32) ::TimeParse(value) * 100);
        }
        else
        {
            if(valueIsQuoted || !IsNumber(value) || bAllBufferProperties)
            {
                IHXBuffer* pBuffer = 0;
                hr = pFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);
                if(HXR_OK == hr)
                {
                    HX_ASSERT(pBuffer);

                    hr = pBuffer->Set((UCHAR*)(const char*)value, value.GetLength() + 1);
                    if(HXR_OK == hr)
                    {
	                hr = pVal->SetPropertyBuffer(key, pBuffer);
                    }
                    HX_RELEASE(pBuffer);
                }

            }
            else
            {
                hr = pVal->SetPropertyULONG32(key, (ULONG32) atol(value));
            }
        }

        if(FAILED(hr))
        {
            // error: failed to add key value
            break;
        } 

    } // for
        
    return hr;
}

void HXURLUtil::DoDollarFixup(CHXString& url /*modified*/)
{
    //
    // Check for obsolete $ sign option used to indicate time in
    // old legacy urls, e.g.:
    //
    // rtsp://moe.cr.prognet.com/ambush.rm$1:00
    //
    // Time after the $ in above url is the start time. However, both
    // the $ and : are valid URL path characters per rfc 2396.
    //
    // Solution: compare the string following the $ to
    // a properly formed time. If the string is a time and only
    // a time, then we assume its the old-style start-time option
    // (and that nothing follows).
    //
    // Otherwise, '$' we assume the '$' is part of the directory/file
    // name and we keep it.
    //


    const char* pszDollarSign = (const char*) ::HXFindChar(url, '$');
    while (pszDollarSign)
    {
        const char* pszTemp = pszDollarSign + 1;

        if (::TimeParse(pszTemp))
        {
            // we successfully parsed a time following the $
            const char* pszBegin = url;
            INT32 cch = pszDollarSign - pszBegin;

            // re-format time as 'start' query parameter
            CHXString fixed(pszBegin, cch);
            fixed += "?start=";
            fixed += pszTemp;

            url = fixed;
            break;
        }

        pszDollarSign = (char*) ::HXFindChar(pszTemp, '$');
    }
}




static
void FixUpFileURLPrefix(HXURLRep& url /*modified*/)
{
    // Ensure we have valid file scheme prefix: 
    //
    //      file://<host>/<path>
    //
    // It is extrememely rare that a <host> is present or intended.
    // Therefore the normal-case correct form is
    //
    //      file:///<path>
    //
    // It is common practice to omit the third slash (and in some cases
    // even the first or second). We now ensure that the the file URL has
    // the correct three-slash form.
    //
    if (url.GetType() != HXURLRep::TYPE_NETPATH)
    {
        // wrong number of slashes after scheme.
        url.SetType(HXURLRep::TYPE_NETPATH);
        url.Update();
    }
    else
    {
        // correct form; possible mis-interpretation of path as <host> component
        if (!url.Host().IsEmpty())
        {
            // The <host> part of the URL is not empty.
            //
            // Since <host> is rarely or ever used or intended for a file URL
            // it is safe to assume that a host is really intended to be part
            // of the path. We make an exception for localhost, even though 
            // that is highly unlikely.
            //
            if(0 != url.Host().Compare("localhost"))
            {
                // re-create url string
                CHXString str = url.Scheme();
                str += ":///";
                UINT32 idxAuthority = url.Scheme().GetLength() + 3; // 'scheme' + '://'
                str += url.String().Mid(idxAuthority);
                url = HXURLRep(str);
            }
        }
    }
}


static
void FixUpPath(CHXString& path /*modified*/)
{
    // ensure path is unescaped
    if (HXEscapeUtil::IsValidEscapedPath(path))
    {
        path = HXEscapeUtil::UnEscape(path);
    } // else, assume already unescaped in all cases

    // convert to forward-slashes (note: other OS path delimiters not converted)
    HXStringUtil::Replace(path, '\\', '/');

#if defined(_WINDOWS)
    // Handle net-share path ("//netshare/path/to/file") case.
    //
    // Both "file:////netshare/path" and "file://///netshare/path"
    // are treated as equivalent. In the first case there will be
    // one slash at the front of the path; in the second there will
    // be two. In the first case we assume the path is not really 
    // intended to be a rooted path relative to '/' (since it  does
    // not make sense on Windows).
    bool isNetSharePath;
    UINT32 cchSlash = HXStringUtil::FindFirstMisMatch(path, '/');
    if (1 == cchSlash || 2 == cchSlash)
    {
        isNetSharePath = true;
    }
    else
    {
        isNetSharePath = false;
    }
#endif

    // ensure that path is in normalized net-path form and compressed.
    path = HXPathUtil::NormalizeNetPath(path, true);

#if defined(_WINDOWS)
    if (isNetSharePath)
    {
        // normalize operation above will have compressed the double-slash
        HX_ASSERT(1 == HXStringUtil::FindFirstMisMatch(path, '/'));
        // add another slash back to the path
        path = CHXString("/") + path;
    }
#endif
    
    // ensure that path is escaped
    path = HXEscapeUtil::EscapePath(path);

}


void HXURLUtil::DoFileURLFixup(HXURLRep& url /*modified*/)
{
    if (!url.IsFullyParsed())
    {
        HX_ASSERT(false);
        return;
    }

    FixUpFileURLPrefix(url);

    // Aattempt to fix up possible '#' chars in the filename.
    //
    // Unescaped '#' chars in the filename will be interpreted as
    // signifying the start of a fragment by HXURLRep.
    // 
    // Fragments don't makes sense for file URLs so it should
    // be safe to assume that any '#' found before '?' is an
    // unescaped filename character. 
    //
    // We manually extract the path by interpreting
    // everything after scheme prefix up to EOS or the first '?' 
    // as the path. We also need to update the query and fragment
    // of the URL which may have parsed out unintended results. The
    // location of the query is not foolproof since '?' is a valid
    // filename character on some systems, as well as a valid scheme
    // or fragment character.
    //
    // Other ambiguous cases:
    //
    // file:///path#with#frag/bar?query=foo?#oth?erdata
    // file:///blah?foo=bar?something=else
    //
    // On Windows '=' is a valid filename character; '?' is illegal.
    // On Unix filesystems '?' and '=' are both valid.
    //

    CHXString query;
    INT32 idxBegin = url.GetPathOffset();
    INT32 idxEnd = url.String().Find('?');
    if (-1 != idxEnd)
    {
        // has query...
        query = url.String().Mid(idxEnd + 1);
       
        // another '?' is ambiguous
        HX_ASSERT(-1 == query.Find('?'));
        
        if (!HXEscapeUtil::IsValidEscapedQuery(query))
        {
            // '#' ambiguous; assume '#' chars are unescaped query chars
            HX_ASSERT(-1 == query.Find('#'));
            query = HXEscapeUtil::EscapeQuery(query);
        }
    }
    else
    {
        // no query
        idxEnd = url.String().GetLength();
    }

    // update fragment and query 
    url.SetFragment("");
    url.SetQuery(query);
 
    HX_ASSERT(idxEnd >= idxBegin);
    CHXString path = url.String().Mid(idxBegin, idxEnd - idxBegin);  

    if (!path.IsEmpty())
    {
        FixUpPath(path);
    }

    url.SetPath(path);
    url.Update();

    HX_ASSERT(url.IsFullyParsed());
}


/*============================================================================*
 *
 * (c) 1995-2002 RealNetworks, Inc. Patents pending. All rights reserved.
 *
 *============================================================================*/


#include "hxurlrep.h"
#include "hxescapeutil.h"
#include "pathutil.h"

inline
bool IsAlpha(char ch)
{
    return (((ch >= 'a') && (ch <= 'z')) ||
            ((ch >= 'A') && (ch <= 'Z')));
}

inline
bool IsDigit(char ch)
{
    return ((ch >= '0') && (ch <= '9'));
}

inline
bool IsHexDigit(char ch)
{
    return (((ch >= 'a') && (ch <= 'f')) ||
        ((ch >= 'A') && (ch <= 'F'))) || IsDigit(ch);
}

// Simple test to decide if we have an IPv6 address 
// or not. Address may still be invalid.
static
bool IsValidIPv6Literal(const char* p)
{
    if (!p || '\0' == *p)
    {
        // empty is invalid
        return false;
    }
    
    bool hasColon = false;
    for(/**/; *p != '\0'; ++p)
    {
        if (!IsHexDigit(*p))
        {
            if (*p == ':')
            {
                hasColon = true;
            }
            else if (*p != '.')
            {
                // invalid IPv6 character
                return false;
            } 
        }
    }
    
    //  at least one colon, 0 or more hex and/or dots => close enough
    return hasColon;
}

inline
bool IsAlnum(char ch)
{
    return (IsAlpha(ch) || IsDigit(ch));
}

inline
bool StringToInt(const char* pBuf, int& value)
{
    char* pEnd = 0;
    value = (int)strtol(pBuf, &pEnd, 10);
    return (*pBuf && *pEnd == '\0');
}

inline
bool IsAllDigits(const char* pBuf)
{
    while (*pBuf != '\0')
    {
        if (!IsDigit(*pBuf))
        {
            return false;
        }
        ++pBuf;
    }
    return true;
}

// first char may be alpha only
inline
bool IsSchemeChar(char ch)
{
    return (IsAlnum(ch) || strchr("+-.", ch));
}

static
bool IsValidScheme(const char* p)
{
    if (!p || '\0' == *p)
    {
        // empty is invalid
        return false;
    }
    
    if (!IsAlpha(*p++))
    {
        return false;
    }

    for( /**/; *p; ++p)
    {
        // RFC 2068 'scheme' production
        if (!IsSchemeChar(*p))
        {
            // bad character
            return false;
        }
    }

    return true;
    
}

//XXXLCM use ipv4 validation/parser when available
// Checks for valid hostname as specified by RFC 952 and RFC 1123
static
bool IsValidIPv4OrNameHost(const CHXString& host)
{
    if (host.IsEmpty())
    {
        return true;
    }

    bool isValid = true;

    const char* pTmp = host;

    if (*pTmp != '\0')
    {
        int len = 0;

        if (IsAlnum(*pTmp))
        {
            for(; *pTmp; pTmp++, len++)
            {
                if (!IsAlnum(*pTmp) && (*pTmp != '.') && (*pTmp != '-'))
                {
                    isValid = false;
                    break;
                }
            }
        }
        else
        {
            // RFC 1123 Sec 2.1 specifies that it MUST start with a letter OR
            // a number.
            isValid = false;
        }

        if (isValid)
        {
            if (len > 1)
            {
                pTmp--; // move back to the last character

                if ((*pTmp == '-') || (*pTmp == '.'))
                {
                    // a hostname cannot end with a '-' or '.'
                    isValid = false;
                }
            }
        }
    }
    
    return isValid;
}



inline
void CollectChars(const char*& pch, const char* pExtraDelim, CHXString& token)
{
    for(; *pch && !strchr(pExtraDelim, *pch); pch++)
    {
        token += *pch;
    }
}

//
// url assumed escaped
//
HXURLRep::HXURLRep(const CHXString& url)
: m_type(TYPE_OPAQUE)
, m_hasIPv6Host(false)
, m_port(-1)
, m_idxPath(0)
, m_parseFlags(0)
{
    Parse(url);
}


// components should be escaped prior to construction
HXURLRep::HXURLRep(Type type,
                   const CHXString& scheme,
                   const CHXString& userInfo,
                   const CHXString& host,
                   INT32 port,
                   const CHXString& path,
                   const CHXString& query,
                   const CHXString& fragment)
: m_type(type)
, m_scheme(scheme)
, m_userInfo(userInfo)
, m_host(host)
, m_hasIPv6Host(false)
, m_port(port)
, m_path(path)
, m_idxPath(0)
, m_query(query)
, m_fragment(fragment)
, m_parseFlags(0)
{
    Update();
}

// components should be escaped prior to construction
HXURLRep::HXURLRep(Type type,
                   const CHXString& scheme,
                   const CHXString& host,
                   INT32 port,
                   const CHXString& path)
: m_type(type)
, m_scheme(scheme)
, m_host(host)
, m_hasIPv6Host(false)
, m_port(port)
, m_path(path)
, m_idxPath(0)
, m_parseFlags(0)
{
    Update();
}

// Fixes the path
//
//  1) Compresses relative paths
//   
//  /foo/bar/../wam.rm => /foo/wam.rm
//
//  2) Translates platform path form into valid url path form
//
//  c:\foo\bar.rm" => c:/foo/bar.rm
//
// Also gets rid of unnecessary ':', '?' or '#' symbols
//
// Example:
//
//    foo://host:/path? => foo://host/path
//
void HXURLRep::Normalize()
{
    if (!m_path.IsEmpty() && m_type != TYPE_OPAQUE)
    {
        m_path = HXPathUtil::NormalizeNetPath(m_path);
    }

    Update();
}



// Assemble an URL string from current state and re-parse.
// This is called after updating url component state.
//
bool HXURLRep::Update()
{
    CHXString string;

    bool hasAuth = false;

    if (m_scheme.GetLength())
    {
        string += m_scheme;
        string += ":";
    }

    m_hasIPv6Host = IsValidIPv6Literal(m_host);

    if (TYPE_NETPATH == m_type)
    {
        string += "//";

        CHXString auth;

        if (!m_userInfo.IsEmpty())
        {
            auth += m_userInfo;
            auth += "@";
        }
        
        if (m_hasIPv6Host)
        {
            auth += '[';
        }
        auth += m_host;

        if (m_hasIPv6Host)
        {
            auth += ']';
        }

        if (m_port >= 0)
        {
            auth += ':';
            auth.AppendULONG(m_port);
        }

        hasAuth = !auth.IsEmpty();
        string += auth;
        
    }

    // 'path' is eiter 
    //   a) a full rel-path relative url path
    //   b) the actual path after the abs-path delimiter '/' in net-path and abs-path forms
    //   c) all opaque data after  <scheme>:

    if (m_type != TYPE_OPAQUE)
    {

        if (TYPE_ABSPATH == m_type)
        {
            // always add / for TYPE_ABSPATH
            string += "/";
        }
        else if (TYPE_NETPATH == m_type)
        {
            if (hasAuth || !m_path.IsEmpty())
            {
                string += "/";
            }
        }

        string += m_path;
        
        if (!m_query.IsEmpty())
        {
            string += "?";
            string += m_query;
        }

        if (!m_fragment.IsEmpty())
        {
            string += "#";
            string += m_fragment;
        }
    } 
    else
    {
        // note: opaque part of url stored as path
        string += m_path;
    }

    return Parse(string);

}

CHXString HXURLRep::GetSchemeAuthorityString()
{
    // [<scheme>:]//<authority>
    CHXString string;
    CHXString auth;

    if (m_scheme.GetLength())
    {
        string += m_scheme;
        string += ":";
    }

    string += "//";

    if (m_hasIPv6Host)
    {
        auth += '[';
    }
    auth += m_host;

    if (m_hasIPv6Host)
    {
        auth += ']';
    }

    if (m_port >= 0)
    {
        auth += ':';
        auth.AppendULONG(m_port);
    }

    if (!auth.IsEmpty())
       string += auth;
    return string;
}

// Parse helper.
//
// We have determined we have a net- or abs-path form URL:
//
//  [<scheme>:]//<authority>/  (net-path)
//  [<scheme>:]/               (abs-path)
//
// If net-path we are just past the slash after <authority>.
// Otherwise we are just past the abs-path slash.
//
bool HXURLRep::ParsePathQueryFragment(const char*& pch, CHXString& token)
{
    // safe offset to path (token may have part of path in rel-path case)
    m_idxPath = pch - m_string - token.GetLength();

    // Collect the rest of the path
    CollectChars(pch, "?#", token);
    m_path = token;
    token = ""; 

    if (!HXEscapeUtil::IsValidEscapedPath(m_path))
    {
        // bad path segments; continue parsing
        m_parseFlags |= PF_INVALID_PATH_SEGMENTS;
    }

    if (*pch == '?')
    {
        ++pch;
        // Collect the rest the query part
        CollectChars(pch, "#", token);
        m_query = token;
        token = "";

        if (!HXEscapeUtil::IsValidEscapedQuery(m_query))
        {
            // bad query; continue parsing
            m_parseFlags |= PF_INVALID_QUERY;
        }
    }
    
    if (*pch == '#')
    {
        ++pch;
        m_fragment = pch;
    }
    
    return true;

}

// ParseAuthority helper
static const char* ScanUserInfo(const char* pch)
{
    HX_ASSERT(pch);

    const char* pchEnd = 0;

    //
    // scan ahead and look for '@' or '/' to determine if we
    // have "user:pw@" or "user@"
    //
    for(;;)
    {
        if( *pch == '@')
        {
            // has user info
            pchEnd = pch;
            break;
        }
        else if('/' == *pch || '\0' == *pch)
        {
            // no user info
            break;
        }

        ++pch;
    }

    return pchEnd;

}


// Parse helper
//
// We have encountered a net-path form URL: 
//
//  [<scheme>:]//
//
// We are just past the two net-path slashes.
//
bool HXURLRep::ParseAuthority(const char*& pch, CHXString& token)
{
    const char* pEndUserInfo = ScanUserInfo(pch);
    if (pEndUserInfo != 0)
    {
        HX_ASSERT(pEndUserInfo >= pch);
        m_userInfo = CHXString(pch, pEndUserInfo - pch);
        HX_ASSERT(*pEndUserInfo == '@');
        pch = pEndUserInfo + 1; // past '@'
    }

    // look for possible IPv6 literal per rfc 2732
    if(*pch == '[')
    {
        m_hasIPv6Host = true;

        // Collect IPv6 host rfc2732
        ++pch;
        CollectChars(pch, "]", token);
        
        if( *pch == ']' )
        {
            // Found full IPv6 literal
            ++pch;
            m_host = token;
            if (!IsValidIPv6Literal(m_host))
            {
                // bad IPv6 literal; continue parsing
                m_parseFlags |= PF_INVALID_IPV6;
                m_parseFlags |= PF_INVALID_HOST;
            }
            
        }
        else
        {
            // invalid host (missing closing ']'); stop parsingxlh
            m_parseFlags |= PF_INVALID_IPV6;
            m_parseFlags |= PF_INVALID_HOST;
            return false;
        }
        

    }
    else
    {
        // Collect host name or IPv4 address
        CollectChars(pch, ":/?#", token);
        m_host = token;
        if (!IsValidIPv4OrNameHost(m_host))
        {
            // bad IPv4/name literal; continue parse
            m_parseFlags |= PF_INVALID_HOST;
        }
    }
    token = "";

    // maybe port: scan to '/' or EOS
    if (*pch == ':')
    {
        ++pch;

        // Collect port number; empty port is ok (ignored)
        CollectChars(pch, "/?#", token);
        if (!token.IsEmpty())
        {
            // rfc does not specify a limit; we base limit on our storage limit (INT_MAX)
            const unsigned int PORT_MAX_DIGIT_COUNT  = 10;
            const unsigned int PORT_MAX_VALUE        = INT_MAX;
            if (token.GetLength() <= PORT_MAX_DIGIT_COUNT && IsAllDigits(token))
            {
                UINT32 val = strtoul(token, NULL, 10);
                if (val <= PORT_MAX_VALUE)
                {
                    m_port = (int)val;
                }
                else
                {
                    m_parseFlags |= PF_INVALID_PORT;
                    m_port = -1;
                }  
            }
            else
            {
                // invalid port *digit see 3.2.2.; continue parse
                m_parseFlags |= PF_INVALID_PORT;
            }
            token = "";
        }
    }
    
    if (*pch == '/')
    {
        // skip the '/' that separates "<authority>/<path>"
        ++pch;
    }
    
    return true;
}

// Parse helper
//
// We have determined we have a generic-form URL:
//
//  [<scheme>:]/[unknown]
//
// We are just past the first slash.
//
bool HXURLRep::ParseGenericBody(const char*& pch, CHXString& token)
{
    // look for a second slash to determine form
    if(*pch == '/')
    {
        // two slash; net-path
        ++pch;
        m_type = TYPE_NETPATH;
        if (!ParseAuthority(pch, token))
        {
            return false;
        }
    }
    else
    {
        // one slash; abs-path
        m_type = TYPE_ABSPATH;
    }
   
    // parse path/query/fragment
    return ParsePathQueryFragment(pch, token);

}


// Parse helper
//
// We have just determined this an absolute URL:
// 
//   <scheme>:[unknown]
//
// We are just past the colon.
//
bool HXURLRep::ParseAbsoluteBody(const char*& pch, CHXString& token)
{
    bool isParsed = true;

    // determine which form we are parsing:
    //  1) abs-path or net-path (generic URI form)
    //  2) opaque
    if ((*pch == '/'))
    {
        // generic URI (abs-path or net-path)
        ++pch;
        isParsed = ParseGenericBody(pch, token);
    } 
    else
    {
        // opaque URI form "<scheme>:<opaque-scheme-specific-part>"
        m_type = TYPE_OPAQUE;

        // safe offset to opaque data
        m_idxPath = pch - m_string;

        // stick <opaque-part> in path
        CollectChars(pch, "", token);
        m_path = token;
        token = "";
        if (m_path.IsEmpty())
        {
            // opaque_part   = uric_no_slash *uric
            m_parseFlags |= PF_INVALID_OPAQUE_PART;
        }
    }

    return isParsed;

}

// parse URI according to "generic URI" syntax (see rfc 2396)
bool HXURLRep::Parse(const CHXString& url)
{
    m_string = url;
    m_parseFlags = 0;

    // reset component info to defaults
    m_scheme          = "";
    m_host              = "";
    m_port              = -1;
    m_path              = "";
    m_idxPath           = 0;
    m_query             = "";
    m_fragment          = "";
    m_userInfo          = "";
    m_type              = TYPE_OPAQUE;
    m_hasIPv6Host       = false;

    const char* pch = m_string;
    CHXString token;
    
    // scheme: scan to first ':', '/', '?', '#' or EOS
    CollectChars(pch, ":/?#", token);

    if (*pch == ':')
    {
        // has protocol/scheme
        m_scheme = token;
        token = ""; 
        ++pch;
        if (!IsValidScheme(m_scheme))
        {
            // bad scheme
            m_parseFlags |= PF_INVALID_SCHEME;
        }
 
        // continue parsing absolute URL
        if (!ParseAbsoluteBody(pch, token))
        {
            m_parseFlags |= PF_ABORTED;
        }
    }
    else
    {
        // continue parsing relative URL
        if (token.IsEmpty() && *pch == '/')
        {
            // net- or abs-path form (first char is '/')
            ++pch;
            if (!ParseGenericBody(pch, token))
            {
                m_parseFlags |= PF_ABORTED;
            }
        }
        else
        {
            // relative path form
            m_type = TYPE_RELPATH;
            if (!ParsePathQueryFragment(pch, token))
            {
                m_parseFlags |= PF_ABORTED;
            }
        }
       
    }    

    return (IsFullyParsed());
}

//
// see RFC 2396 Section 5.2 for algorithm upon which this is based
//
bool HXURLRep::ApplyRelative(const HXURLRep& other)
{
    // we should be absolute form (have scheme)
    HX_ASSERT(!m_scheme.IsEmpty());

    // both urls must be valid net-path or abs-path or rel-path forms
    if (!IsValid() || !other.IsValid())
    {
        return false;
    }

    if (other.m_scheme.IsEmpty())
    {
        if (other.m_type == TYPE_RELPATH)
        {
            HX_ASSERT(other.m_scheme.IsEmpty());
            
            if (!other.m_path.IsEmpty() || !other.m_query.IsEmpty())
            {
                // apply relative path
                bool isRooted = (!m_path.IsEmpty() && m_path[0] == '/');
                m_path = HXPathUtil::GetNetPathRelativePath(m_path, other.m_path, isRooted);
                if (m_path == "./")
                {
                    m_path = "";
                }
            }
        }
        else 
        {

            HX_ASSERT(other.m_type == TYPE_NETPATH || other.m_type == TYPE_ABSPATH);
        
            m_path = other.m_path;
            if (other.m_type == TYPE_NETPATH)
            {
                // inherit net path (<authority>)
                m_host = other.m_host;
                m_port = other.m_port;
                m_userInfo = other.m_userInfo;
                m_hasIPv6Host = other.m_hasIPv6Host;
            }
        }

        // inherit the query and frag
        m_query = other.m_query;
        m_fragment = other.m_fragment;

        Update();
    }
    else
    {
        // other is absolute url (has scheme) -> take other url as is
        *this = other;
    }

    return IsFullyParsed();
}



















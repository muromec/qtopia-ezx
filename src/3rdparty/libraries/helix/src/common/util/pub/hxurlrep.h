/*============================================================================*
 *
 * (c) 1995-2002 RealNetworks, Inc. Patents pending. All rights reserved.
 *
 *============================================================================*/
 
// -*-c++-*-

#ifndef HX_URLREP_H__
#define HX_URLREP_H__

#include "hxstring.h"

// generic URI syntax (see rfc 2396) URI parser

class HXURLRep 
{
public:
     // flags indicating status of parse attempt
    enum ParseFlags
    {
        PF_INVALID_SCHEME           = 0x0001,
        PF_INVALID_PORT             = 0x0002,
        PF_INVALID_PATH_SEGMENTS    = 0x0004, // not a generic abs path
        PF_INVALID_HOST             = 0x0008,
        PF_INVALID_IPV6             = 0x0010,
        PF_INVALID_QUERY            = 0x0020,
        PF_INVALID_OPAQUE_PART      = 0x0040,
        PF_ABORTED                  = 0x1000
    };

    // type indicates which of several general URL form types
    enum Type
    {
        TYPE_OPAQUE,     // [<scheme>:]<opaque part>
        TYPE_NETPATH,    // [<scheme>:]//
        TYPE_ABSPATH,    // [<scheme>:]/
        TYPE_RELPATH     // <path>?<query><frag>
    };

    HXURLRep(const CHXString& url = "");

    HXURLRep(Type type,
               const CHXString& scheme,
               const CHXString& host,
               INT32 port, // -1 = no port
               const CHXString& path);

    HXURLRep(Type type,
             const CHXString& scheme,
             const CHXString& userInfo,
             const CHXString& host,
	     INT32 port, // -1 = no port
	     const CHXString& path,
	     const CHXString& query,
             const CHXString& fragment);

    // set/alter methods (implicit update)
    bool ApplyRelative(const HXURLRep& other);
    void Normalize();

    // Update() must be called after set operations
    void SetType(Type type);
    void SetScheme(const char* pScheme);
    void SetUserInfo(const char* pUserInfo);
    void SetHost(const char* pHost);
    void SetPort(INT32 port);
    void SetPath(const char* pPath);
    void SetQuery(const char* pQuery);
    void SetFragment(const char* pFragment);
    bool Update();

    // get/query methods
    UINT32 GetParseFlags() const;
    Type GetType() const;
    bool HasIPv6Host() const;
    bool HasPort() const;
    bool IsFullyParsed() const;
    bool IsValid() const;

    // url components
    const CHXString&    String() const;
    const CHXString&    Scheme() const;
    const CHXString&    Host() const;
    const CHXString&    UserInfo() const;
    UINT32              Port() const;
    const CHXString&    Path() const;
    const CHXString&    Query() const;
    const CHXString&    Fragment() const;
    CHXString           GetSchemeAuthorityString();

    UINT32 GetPathOffset() const;
private: 
    int operator==(const HXURLRep&) const;
    int operator!=(const HXURLRep&) const;

    // parse helpers
    bool Parse(const CHXString& url);
    bool ParseAbsoluteBody(const char*& pch, CHXString& token);
    bool ParseGenericBody(const char*& pch, CHXString& token);
    bool ParseAuthority(const char*& pch, CHXString& token);
    bool ParsePathQueryFragment(const char*& pch, CHXString& token);

    CHXString   m_string;
    
    Type        m_type;
    CHXString   m_scheme;	
    CHXString   m_userInfo;
    CHXString   m_host;
    bool        m_hasIPv6Host;
    INT32       m_port;
    CHXString   m_path;  
    UINT32      m_idxPath;
    CHXString   m_query;
    CHXString   m_fragment;

    UINT32      m_parseFlags;
};

inline
void HXURLRep::SetType(Type type)
{
    m_type = type;
}

inline
void HXURLRep::SetScheme(const char* pScheme)
{
    m_scheme = pScheme;
}

inline
void HXURLRep::SetUserInfo(const char* pUserInfo)
{
    m_userInfo = pUserInfo;
}

inline
void HXURLRep::SetHost(const char* pHost)
{
    m_host = pHost;
}

inline
void HXURLRep::SetPort(INT32 port)
{
    m_port = port;
}

inline
void HXURLRep::SetPath(const char* pPath)
{
    m_path = pPath;
}

inline
void HXURLRep::SetQuery(const char* pQuery)
{
    m_query = pQuery;
}

inline
void HXURLRep::SetFragment(const char* pFragment)
{
    m_fragment = pFragment;
}

inline
const CHXString& HXURLRep::String() const
{
    return m_string;
}

inline
const CHXString& HXURLRep::Scheme() const
{
    return m_scheme;
}

inline
const CHXString& HXURLRep::Host() const
{
    return m_host;
}

inline
const CHXString& HXURLRep::UserInfo() const
{
    return m_userInfo;
}

inline
UINT32 HXURLRep::Port() const
{
    return (m_port >= 0) ? m_port : 0;
}

inline
const CHXString& HXURLRep::Path() const
{
    return m_path;
}
 
inline
const CHXString& HXURLRep::Query() const
{
    return m_query;
}
 
inline
const CHXString& HXURLRep::Fragment() const
{
    return m_fragment;
}

// return true if last build-and-parse
// resulted in a full parse
inline
bool HXURLRep::IsFullyParsed() const
{
    return !(PF_ABORTED & m_parseFlags);
}

// return true if all components are correct per rfc;
// NB: url may still be useable even if invalid 
// (e.g., path component in Window path format)
inline
bool HXURLRep::IsValid() const
{
    return (0 == m_parseFlags);
}

inline
bool HXURLRep::HasIPv6Host() const
{
    return m_hasIPv6Host;
}

inline
bool HXURLRep::HasPort() const
{
    return (m_port >= 0);
}

inline
HXURLRep::Type HXURLRep::GetType() const
{
    return m_type;
}

inline
UINT32 HXURLRep::GetParseFlags() const
{
    return m_parseFlags;
}


// TYPE_ABSPATH, TYPE_NETPATH:  
//      offset to character just past abs-path delimeter ('/'); points to path 
// TYPE_OPAQUE:                 
//      offset to character just past scheme delimiter (':') points to data
// TYPE_RELPATH:
//      offset to path (always 0 by definition)
inline
UINT32 HXURLRep::GetPathOffset() const
{
    HX_ASSERT(IsFullyParsed());
    return m_idxPath;
}

#endif // HX_URLREP_H__

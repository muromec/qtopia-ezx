/************************************************************************
 * hx_escapeutil.h
 * --------------------
 *
 * Synopsis:
 * URL escaping/unescaping utility helpers
 *
 * Target:
 * Helix
 *
 *
 * (c) 1995-2003 RealNetworks, Inc. Patents pending. All rights reserved.
 *
 *****************************************************************************/

#ifndef HX_ESCAPED_STRING_H__
#define HX_ESCAPED_STRING_H__

class HXURLRep;
class CHXString;

// escaping per rfc 2396
struct HXEscapeUtil
{

// decode
static CHXString UnEscape(const CHXString& escapedStr);

// encode (used when assembling url components into a full URL)
static CHXString EscapeGeneric(const CHXString& unescapedStr, const HXBOOL doubleEscape = FALSE);
static CHXString EscapePath(const CHXString& unescapedPath, bool bForcePlusEscape = false);
static CHXString EscapeQuery(const CHXString& unescapedQuery);

// url fixup for urls originating from user input 
static bool EnsureEscapedURL(CHXString& rep /*modified*/);
static bool EnsureEscapedURL(HXURLRep& url /*modified*/);


// used to determine if string needs escaping (has reserved/illegal chars)
static bool IsValidEscapedPath(const CHXString& str);
static bool IsValidEscapedQuery(const CHXString& str);

// escape special character (e.g., escape '%' characters for printf)
static CHXString EscapeSymbol(const CHXString& in, char symbol);

// escape string data to be legal XML element, currently '<' to '&lt' and '>' to '&gt' only
static CHXString EscapeStringDataForXML(const CHXString& unescapedData);

// unescape data from escaped XML string element
static CHXString UnEscapeXMLStringData(const CHXString& escapedData);
};

#endif // HX_ESCAPED_STRING_H__

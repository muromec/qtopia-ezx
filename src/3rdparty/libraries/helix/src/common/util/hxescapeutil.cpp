/************************************************************************
 * hx_escapeutil.cpp
 * ----------------------
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
 
// System includes
#include <ctype.h>

// Helix includes
#include "hxstring.h"
#include "char_stack.h"
#include "hxassert.h"
#include "pathutil.h"
#include "hxurlrep.h"
#include "hxescapeutil.h"

#define ARRAY_COUNT(a) (sizeof(a)/sizeof(a[0]))


//
// set of chars (in addition to alphanum) that need not
// be escaped when escaping URI components
//
// rfc 2396 2.3.
//
static const char k_unreservedChars[] = 
{ 
    '-', '_', '.', '!', '~', '*', '\'', '(', ')', '\0' 
};

// unwise = { } | \ ^ [ ] ' //should be escaped

// legal userinfo = ; : & = + $ , (3.2.2.) //need not be escaped 

//
// this is set of chars that should not or need not be escaped
// when escaping path component of a URL
//
// some of these are reserved (hence should not), some are just
// legal pchars (hence need not)
//
// rfc 2396 3.3.
//
static const char k_pathChars[] = 
{ 
    ':', '@', '&', '=', '+', '$', ',', '/', ';', '\0'
};


//
// this is set of chars that should not be escaped
// when escaping query component of a URL
//
// these are all reserved (note: query key value data must escape to prevent conflict)
//
// rfc 2396 3.4
//
static const char k_queryChars[] = 
{
    ';', '/', '?', ':', '@', '&', '=', '+', '$', ',', '\0'
};


inline
bool IsEscaped(const char* pBuf, int len)
{
    bool ret = false;

    if ((len >= 3) &&
	(pBuf[0] == '%') &&
	(isxdigit(pBuf[1])) &&
	(isxdigit(pBuf[2])))
	ret = true;	

    return ret;
}

inline 
int Hex2Char(char ch)
{
    int ret = 0;

    if ((ch >= 'a') && (ch <= 'f'))
	ret =  10 + ch - 'a';
    else if ((ch >= 'A') && (ch <= 'F'))
	ret =  10 + ch - 'A';
    else
	ret = ch - '0';
    
    return ret;
}

inline
bool IsAlphaNum(unsigned char ch)
{
    bool ret = false;
    
    if (((ch >= 0x30) && (ch < 0x3a)) ||  // 0-9
	((ch >= 0x41) && (ch < 0x5b)) ||  // A-Z
	((ch >= 0x61) && (ch < 0x7b)))    // a-z
	ret = true;
    
    return ret;
}

// is this a char that should be escaped?
inline
bool IsSpecialChar(char ch, const char* pExtra)
{
    bool ret = true;

    if ((IsAlphaNum(ch)) ||
	(strchr(k_unreservedChars, ch))||
	(pExtra && strchr(pExtra, ch)))
    {
	ret = false;
    }
       
    return ret;
}

inline
char MakeHex(unsigned char ch)
{
    static const char z_hexBuf[] = "0123456789ABCDEF";
    HX_ASSERT(ch >= 0 && ch < ARRAY_COUNT(z_hexBuf));
    return z_hexBuf[ch];
}


CHXString HXEscapeUtil::UnEscape(const CHXString& escapedStr)
{
    int len = escapedStr.GetLength();
    const char* pCur = escapedStr;

    CharStack newStr;

    while(len)
    {
	if (IsEscaped(pCur,len))
	{
	    // Contruct the character from the escape sequence and
	    // copy it into the new string
	    *newStr = (Hex2Char(pCur[1]) << 4) | Hex2Char(pCur[2]);
	    newStr++;

	    pCur += 2;
	    len -= 2;
	}
	else
	{
	    // Just copy the character
	    *newStr = *pCur;
	    newStr++;
	}

	pCur++;
	len--;
    }

    return newStr.Finish();
}

static CHXString Escape(const CHXString& unescapedStr, const char* pExtra = 0)
{
    int len = unescapedStr.GetLength();
    const char* pCur = unescapedStr;
    
    CharStack newPath;

    while (len)
    {
	if (IsSpecialChar(*pCur, pExtra) &&
	    !IsEscaped(pCur,len))
	{
	    // Escape this character
	    *newPath = '%';
	    newPath++;
	    
	    *newPath = MakeHex((*pCur >> 4) & 0x0f);
	    newPath++;
	    
	    *newPath = MakeHex(*pCur & 0xf);
	    newPath++;
	}
	else
	{
	    *newPath = *pCur;
	    newPath++;
	}

	pCur++;
	len--; 
    }

    return newPath.Finish();
}

// return 'true' if string contains no chars that need escaping
static bool IsValidEscapedString(const CHXString& str, const char* pExtra)
{
    bool ret = true;

    int len = str.GetLength();
    const char* pCur = str;

    while(len)
    {
	if (IsSpecialChar(*pCur, pExtra) &&
	    !IsEscaped(pCur,len))
	{
	    // The escaped string has an illegal character in it
	    ret = false;
	    break;
	}

	pCur++;
	len--;
    }

    return ret;
}


bool HXEscapeUtil::IsValidEscapedPath(const CHXString& str)
{
    return IsValidEscapedString(str, k_pathChars);
}

bool HXEscapeUtil::IsValidEscapedQuery(const CHXString& str)
{
    return IsValidEscapedString(str, k_queryChars);
}

bool HXEscapeUtil::EnsureEscapedURL(HXURLRep& rep /*modified*/)
{
    if (rep.IsFullyParsed())
    {
        if(!rep.IsValid())
        {
            CHXString path = rep.Path();
            if(!IsValidEscapedPath(path))
            {
                // try escaping path
                rep.SetPath(EscapePath(path));
            }

            CHXString query = rep.Query();
            if(!IsValidEscapedQuery(rep.Query()))
            {
                // try escaping path
                rep.SetQuery(EscapeQuery(query));
            }

            // re-build URL
            rep.Update();
        }
    }
    return rep.IsValid();
}

//
// URLs originating from user input may have unescaped characters
// (spaces, colons, etc.) that should be escaped. This does best
// effort attempt to fix up the url so it is properly escaped.
//
bool HXEscapeUtil::EnsureEscapedURL(CHXString& url /*modified*/)
{

    //
    // This is not fool-proof
    //
    // for example, this logic will incorrectly interpret the path after
    // the '#' as a fragment
    //
    //       http://server/the path+/file#name.txt
    //
    HXURLRep rep(url);
    EnsureEscapedURL(rep);
    if (rep.IsValid())
    {
        url = rep.String();
        return true;
    }
    return false;
}


// for escaping a path component before assembly into full URL
CHXString HXEscapeUtil::EscapePath(const CHXString& unescapedPath, bool bForcePlusEscape)
{
    CHXString strPathChars = k_pathChars;
    if(bForcePlusEscape)
    {
        // replace '+' with arbitrary unreserved character so '+' will be escaped

        // force '+' to be escaped so other unescaping logic doesn't mis-intepret it as a space
        INT32 idxPlus = strPathChars.Find('+');
        HX_ASSERT(idxPlus != -1);
        strPathChars.SetAt(idxPlus, k_unreservedChars[0]);
    }
    
    return Escape(unescapedPath, strPathChars);
}

// for escaping a query component before assembly into full URL
CHXString HXEscapeUtil::EscapeQuery(const CHXString& unescapedQuery)
{
    return Escape(unescapedQuery, k_queryChars);
}

// escape everything but unreserved characters
CHXString HXEscapeUtil::EscapeGeneric(const CHXString& unescapedQuery)
{
    return Escape(unescapedQuery);
}

// escape symbol by doubling, e.g., '%' -> '%%'
CHXString HXEscapeUtil::EscapeSymbol(const CHXString& in, char symbol)
{
    if (-1 == in.Find(symbol))
    {
        // nothing to escape
        return in;
    }

    CHXString out;
    UINT32 cch = in.GetLength();
    for (UINT32 idx = 0; idx < cch; ++idx)
    {
        char ch = in[idx];
        out += ch;
        if (symbol == ch)
        {
            // escape
            out += symbol;
        }
    }
    return out;
}


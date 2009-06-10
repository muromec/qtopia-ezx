/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hlxosstr.h,v 1.14 2009/01/19 22:52:02 sfu Exp $
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

#ifndef HLXOSSTR_H
#define HLXOSSTR_H

#ifdef __cplusplus

#include "hxtypes.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/string.h"


//----------------------------------------------------------------------------
// New utilities
//----------------------------------------------------------------------------


// Returns the number of characters in the string excluding the terminator.
// If <psz> is NULL, 0 is returned.
//
size_t StringLengthA(const char* psz);


// Returns the number of characters in the wide-character string excluding the terminator.
// If <pszw> is NULL, 0 is returned.
//
size_t StringLengthW(const wchar_t* pszw);


// Copies the string <psz2> including the terminating character
// into the memory area <psz1>. If copying takes place between objects that overlap,
// behavior is undefined.
// If <psz1> is NULL or <psz2> is NULL, the function does nothing.
// Returns <psz1>.
//
char* StringCopyA(char* psz1, const char* psz2);


// Copies the wide-character string <pszw2> including the terminating character
// into the memory area <pszw1>. If copying takes place between objects that overlap,
// behavior is undefined.
// If <pszw1> is NULL or <pszw2> is NULL, the function does nothing.
// Returns <pszw1>.
//
wchar_t* StringCopyW(wchar_t* pszw1, const wchar_t* pszw2);


// Returns 1 if the terminated strings are byte-by-byte equal, otherwise it returns 0.
//
int StringEqualA(const char* psz1, const char* psz2);


// Returns 1 if the terminated wide-character strings are byte-by-byte equal, otherwise it returns 0.
//
int StringEqualW(const wchar_t* pszw1, const wchar_t* pszw2);


// Allocates memory and concatenates strings in the new buffer including the terminating character.
// If <releaseOldBuffer> is 1, <pBuffer> is released before the functions exits, but only if the new
// buffer was sucessfully allocated. That means: if NULL was returned, <pBuffer> was not released.
// If <allocType> is 0, memory is allocated by using malloc, and released by using free.
// If <allocType> is 1, memory is allocated by using new[] operator, and released by using delete[] operator.
// If <pBuffer> is NULL and <pAppend> is NULL, NULL is returned.
//
char* StringAppendA(char* pBuffer, const char* pAppend, int releaseOldBuffer = 1, int allocType = 1);


// Allocates memory and concatenates strings in the new buffer including the terminating character.
// If <releaseOldBuffer> is 1, <pBuffer> is released before the functions exits, but only if the new
// buffer was sucessfully allocated. That means: if NULL was returned, <pBuffer> was not released.
// If <allocType> is 0, memory is allocated by using malloc, and released by using free.
// If <allocType> is 1, memory is allocated by using new[] operator, and released by using delete[] operator.
// If <pBuffer> is NULL and <pAppend> is NULL, NULL is returned.
//
wchar_t* StringAppendW(wchar_t* pBuffer, const wchar_t* pAppend, int releaseOldBuffer = 1, int allocType = 1);


// Base buffer classes.
class HXConvBufferBaseA
{
public:
    virtual ~HXConvBufferBaseA()
    {
        if(m_data && m_owner)
        {
            free(m_data);
        }
    }
    char* Release()
    {
        m_owner = FALSE;
        return m_data;
    }
    operator const char*() const { return m_data; }
    operator char*() { return m_data; }
    UINT32 Length() const { return m_length; }

protected:
    HXConvBufferBaseA()
        : m_data(0)
        , m_length(0)
        , m_owner(TRUE)
    {;}

    char* m_data;
    UINT32 m_length;
    HXBOOL m_owner;
};

class HXConvBufferBaseW
{
public:
    virtual ~HXConvBufferBaseW()
    {
        if(m_data && m_owner)
        {
            free(m_data);
        }
    }
    wchar_t* Release()
    {
        m_owner = FALSE;
        return m_data;
    }
    operator const wchar_t*() const { return m_data; }
    operator wchar_t*() { return m_data; }
    UINT32 Length() const { return m_length; }

protected:
    HXConvBufferBaseW()
        : m_data(0)
        , m_length(0)
        , m_owner(TRUE)
    {;}

    wchar_t* m_data;
    UINT32 m_length;
    HXBOOL m_owner;
};


// Inline wchar_t -> UTF8 conversion. Uses system-independent implementation.
// Class constructs UTF8 encoded string and keeps it in the temporary buffer
// which is released by the destructor.
// Usage example:
//       wchar_t* pw = ...
//       CopyUTF8SomewhereElse((const char*)UTF8FromWCharT(pw));
//
class UTF8FromWCharT : public HXConvBufferBaseA
{
public:
    // If (size_t)-1 is passed as <length>, constructor scans <pszw> for terminator and the resulting
    // string includes the terminator; otherwise, constructor converts exactly <length> units and so
    // depending on the input, the resulting string may not be terminated (in this case use Length()
    // to obtain the resulting length).
    explicit UTF8FromWCharT(const wchar_t* pszw, size_t length = (size_t)-1);
};


// Inline UTF8 -> wchar_t conversion. Uses system-independent implementation.
// Class constructs wchar_t encoded string and keeps it in the temporary buffer
// which is released by the destructor.
// Usage example:
//       char* p = ...
//       CopyWCharTSomewhereElse((const wchar_t*)WCharTFromUTF8(p));
//
class WCharTFromUTF8 : public HXConvBufferBaseW
{
public:
    // If (size_t)-1 is passed as <length>, constructor scans <psz> for terminator and the resulting
    // string includes the terminator; otherwise, constructor converts exactly <length> units and so
    // depending on the input, the resulting string may not be terminated (in this case use Length()
    // to obtain the resulting length).
    explicit WCharTFromUTF8(const char* psz, size_t length = (size_t)-1);
};


// Inline wchar_t -> CurrentCodePage conversion. Uses system-dependent implementation.
// Class constructs CurrentCodePage encoded string and keeps it in the temporary buffer
// which is released by the destructor. On systems which do not support native current-code-page
// translation only pure ascii input strings are accepted, otherwise translation fails.
// Usage example:
//       wchar_t* pw = ...
//       CopyCCPSomewhereElse((const char*)CCPFromWCharT(pw));
//
class CCPFromWCharT : public HXConvBufferBaseA
{
public:
    // If (size_t)-1 is passed as <length>, constructor scans <pszw> for terminator and the resulting
    // string includes the terminator; otherwise, constructor converts exactly <length> units and so
    // depending on the input, the resulting string may not be terminated (in this case use Length()
    // to obtain the resulting length).
    explicit CCPFromWCharT(const wchar_t* pszw, size_t length = (size_t)-1);
};


// Inline CurrentCodePage -> wchar_t conversion. Uses system-dependent implementation.
// Class constructs wchar_t encoded string and keeps it in the temporary buffer
// which is released by the destructor. On systems which do not support native current-code-page
// translation only pure ascii input strings are accepted, otherwise translation fails.
// Usage example:
//       char* p = ...
//       CopyWCharTSomewhereElse((const wchar_t*)WCharTFromCCP(p));
//
class WCharTFromCCP : public HXConvBufferBaseW
{
public:
    // If (size_t)-1 is passed as <length>, constructor scans <psz> for terminator and the resulting
    // string includes the terminator; otherwise, constructor converts exactly <length> units and so
    // depending on the input, the resulting string may not be terminated (in this case use Length()
    // to obtain the resulting length).
    explicit WCharTFromCCP(const char* psz, size_t length = (size_t)-1);
};


// Inline CurrentCodePage -> UTF8 conversion. Uses system-dependent implementation.
// Class constructs UTF8 encoded string and keeps it in the temporary buffer
// which is released by the destructor. On systems which do not support native current-code-page
// translation only pure ascii input strings are accepted, otherwise translation fails.
// Usage example:
//       char* p = ...
//       CopyUTF8SomewhereElse((const char*)UTF8FromCCP(p));
//
class UTF8FromCCP : public HXConvBufferBaseA
{
public:
    // If (size_t)-1 is passed as <length>, constructor scans <psz> for terminator and the resulting
    // string includes the terminator; otherwise, constructor converts exactly <length> units and so
    // depending on the input, the resulting string may not be terminated (in this case use Length()
    // to obtain the resulting length).
    explicit UTF8FromCCP(const char* psz, size_t length = (size_t)-1);
};


// Inline UTF8 -> CurrentCodePage conversion. Uses system-dependent implementation.
// Class constructs CurrentCodePage encoded string and keeps it in the temporary buffer
// which is released by the destructor. On systems which do not support native current-code-page
// translation only pure ascii input strings are accepted, otherwise translation fails.
// Usage example:
//       char* p = ...
//       CopyCCPSomewhereElse((const char*)CCPFromUTF8(p));
//
class CCPFromUTF8 : public HXConvBufferBaseA
{
public:
    // If (size_t)-1 is passed as <length>, constructor scans <psz> for terminator and the resulting
    // string includes the terminator; otherwise, constructor converts exactly <length> units and so
    // depending on the input, the resulting string may not be terminated (in this case use Length()
    // to obtain the resulting length).
    explicit CCPFromUTF8(const char* psz, size_t length = (size_t)-1);
};


//----------------------------------------------------------------------------
// Legacy utilities
//----------------------------------------------------------------------------
/*
 * XXX
 * IMPORTANT:
 * HLXOsStrW class actually interprets <ascii> parameter as UTF8 encoded string.
 * Keep this behaviour for now, but original intention should be verified.
 * From now on, use new classes: WCharTFromUTF8/UTF8FromWCharT for UTF8 <-> wchar_t conversions,
 * and WCharTFromCCP/CCPFromWCharT for CurrentCodePage <-> wchar_t conversions.
 * :ENDIMPORTANT
 *
 * Simple class for simple in-line Unicode conversion
 * Handles conversions in both directions as well as handling mutable 
 * conversions as follows:
 *
 * void f()
 * {
 *      char s[100];
 *      UnicodeFunctionWithSideEffect(HLXOsStrW(s));
 *
 *		if (strcmp(s, "Desired output")
 *		{
 *			...
 *		}
 * }
 * Since HLXOsStrW() is constructed as char* rather than const char*.
 * ~HLXOsStrW() will convert the Unicode back to ascii
 *
 */
class HLXOsStrW
{
public:
    HLXOsStrW() : m_isMutable(FALSE), m_toAscii(FALSE), m_size(0), m_uni(0), m_ascii(0) { }
    
    HLXOsStrW(char* ascii, size_t length = (size_t)-1);
    HLXOsStrW(const char* ascii, size_t length = (size_t)-1);
    HLXOsStrW(const unsigned char* ascii, size_t length = (size_t)-1);
#ifndef ANDROID
    HLXOsStrW(const wchar_t* uni, size_t length = (size_t)-1);
#endif

    operator wchar_t*() { return m_uni; } 
    operator const char*() { return (const char*) m_ascii; } 
    operator const unsigned char*() { return (const unsigned char*) m_ascii; } 
    operator char*() { return m_ascii; } 
    operator unsigned char*() { return (unsigned char*) m_ascii; } 
     ~HLXOsStrW();
    int GetOutSize() { return m_outsize; }

    HLXOsStrW(const HLXOsStrW& rhs);
    HLXOsStrW& operator=(const HLXOsStrW& rhs);

protected:
    inline void Copy(HLXOsStrW& lhs, const HLXOsStrW& rhs);
    inline void Init(const char* ascii, size_t length);

    HXBOOL m_isMutable;
    HXBOOL m_toAscii;
    int m_size;
    int m_outsize;
    wchar_t* m_uni;
    char* m_ascii;
};

/* 
 * Pass through object for ascii strings. This allows
 * us to bypass type checking for situations where the
 * parameter we are handed is const and the OS call takes
 * something non-const.
 */
class HLXOsStrA
{
public:
    HLXOsStrA() : m_str(0) {}
    HLXOsStrA(const char* str, size_t length = (size_t)-1) : m_str((char*)str) {}
    HLXOsStrA(const unsigned char* str, size_t length = (size_t)-1) : m_str((char*)str) {}

    operator char* () const { return m_str; }
    operator unsigned char* () const { return (unsigned char*)m_str; }
    operator const char* () const { return (const char*)m_str; }
    operator const unsigned char* () const { return (const unsigned char*)m_str; }
private:
    char* m_str;
};

#if defined(WIN32_PLATFORM_PSPC)
#define OS_STRING_TYPE HLXOsStrW
#define OS_TEXT_PTR wchar_t*

#elif defined(_SYMBIAN) && defined(_UNICODE)
#define OS_STRING_TYPE HLXOsStrW
#define OS_TEXT_PTR wchar_t*

#else	// default
#define OS_STRING_TYPE HLXOsStrA
#define OS_TEXT_PTR char*

#endif /* defined(WIN32_PLATFORM_PSPC) */

#define OS_STRING(str) OS_STRING_TYPE(str)
#define OS_STRING2(str, size) OS_STRING_TYPE(str, size)

#else /* __cplusplus */
/* No C++?!? You're on your own. Have fun with the type checker */
#define OS_STRING_TYPE char*
#define OS_STRING(str) str
#define OS_STRING2(str, size) str
#define OS_TEXT_PTR char*

#endif /* __cplusplus */


#endif /* HLXOSSTR_H */

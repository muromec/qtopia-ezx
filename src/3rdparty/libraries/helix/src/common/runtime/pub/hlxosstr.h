/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hlxosstr.h,v 1.9 2005/03/14 19:36:38 bobclark Exp $
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

#ifndef HLXOSSTR_H
#define HLXOSSTR_H

#ifdef __cplusplus

#include "hxtypes.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/string.h"

/*
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
    HLXOsStrW(const wchar_t* uni, size_t length = (size_t)-1);

    operator wchar_t*() { return m_uni; } 
    operator const char*() { return (const char*) m_ascii; } 
    operator const unsigned char*() { return (const unsigned char*) m_ascii; } 
    operator char*() { return m_ascii; } 
    operator unsigned char*() { return (unsigned char*) m_ascii; } 
     ~HLXOsStrW();

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


// UTF16 <-> Unicode conversion functions
#ifdef __cplusplus
extern "C" 
{
#endif
typedef enum	
{
	UTF16,
	UTF16BE,
	UTF16LE,
} UTF16_TYPE;

int ConvertUTF16ToUTF8(const UINT16* pwIn,int iInSize,UINT8* pbOut,int iOutSize,UTF16_TYPE Utf16Type);

#ifdef __cplusplus
}
#endif

#endif /* HLXOSSTR_H */

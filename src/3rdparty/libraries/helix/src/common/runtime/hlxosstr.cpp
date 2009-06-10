/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hlxosstr.cpp,v 1.30 2009/01/19 23:13:53 sfu Exp $
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


#include "hlxosstr.h"
#include "hlxclib/windows.h"
#include "hlxclib/assert.h"
#include "hlxclib/string.h"
#include "hxassert.h"
#include "uniconv.h"


//----------------------------------------------------------------------------
// New utilities
//----------------------------------------------------------------------------
// Keep this flag for testing (at least for compile-time syntax checking).
// By uncommenting it, it's possible to test on MS-Windows how the code would behave
// without native WideCharToMultiByte/MultiByteWideChar.
//
//#define FORCE_NO_NATIVE_MULTIBYTE_WIDECHAR 1


#ifdef _BIG_ENDIAN
const HXBOOL g_bBigEndianMachine = TRUE;
#else
const HXBOOL g_bBigEndianMachine = FALSE;
#endif //_BIG_ENDIAN


size_t StringLengthA(const char* psz)
{
    if(psz)
    {
        const char* pszStart = psz;
        while(*psz++);
        return size_t((psz - pszStart) - 1);
    }
    return size_t(0);
}

size_t StringLengthW(const wchar_t* pszw)
{
    if(pszw)
    {
        const wchar_t* pszwStart = pszw;
        while(*pszw++);
        return size_t((pszw - pszwStart) - 1);
    }
    return size_t(0);
}

char* StringCopyA(char* psz1, const char* psz2)
{
    char* psz1Start = psz1;
    if(psz1 && psz2)
    {
        while((*psz1++ = *psz2++));
    }
    return psz1Start;
}

wchar_t* StringCopyW(wchar_t* pszw1, const wchar_t* pszw2)
{
    wchar_t* pszw1Start = pszw1;
    if(pszw1 && pszw2)
    {
        while((*pszw1++ = *pszw2++));
    }
    return pszw1Start;
}

int StringEqualA(const char* psz1, const char* psz2)
{
    if(psz1 && psz2)
    {
        while(*psz1 && *psz2)
        {
            if(*psz1++ != *psz2++) return 0; //FALSE
        }
        // if both point to the terminator, return TRUE
        return (*psz1 == *psz2) ? 1 : 0;
    }
    // if both are NULL, return TRUE
    return (psz1 == psz2) ? 1 : 0;
}

int StringEqualW(const wchar_t* pszw1, const wchar_t* pszw2)
{
    if(pszw1 && pszw2)
    {
        while(*pszw1 && *pszw2)
        {
            if(*pszw1++ != *pszw2++) return 0; //FALSE
        }
        // if both point to the terminator, return TRUE
        return (*pszw1 == *pszw2) ? 1 : 0;
    }
    // if both are NULL, return TRUE
    return (pszw1 == pszw2) ? 1 : 0;
}

char* StringAppendA(char* pBuffer, const char* pAppend, int releaseOldBuffer, int allocType)
{
    if(!pBuffer && !pAppend)
    {
        return 0;
    }
    // calc new buffer length
    size_t len = 1; //for null-terminator
    size_t lenFirst = 0;
    size_t lenSecond = 0;
    if(pBuffer)
    {
        lenFirst = StringLengthA(pBuffer);
        len += lenFirst;
    }
    if(pAppend)
    {
        lenSecond = StringLengthA(pAppend);
        len += lenSecond;
    }
    // allocate buffer
    char* pOut = 0;
    if(allocType == 0)
    {
        pOut = (char*)malloc(len);
    }
    else if(allocType == 1)
    {
        pOut = new char[len];
    }
    if(pOut)
    {
        // concatenate
        char* pWrite = pOut;
        if(pBuffer)
        {
            StringCopyA(pWrite, pBuffer);
            pWrite += lenFirst;
        }
        if(pAppend)
        {
            StringCopyA(pWrite, pAppend);
            pWrite += lenSecond;
        }
        *pWrite = 0;

        // release old buffer
        if(pBuffer && releaseOldBuffer == 1)
        {
            if(allocType == 0)
            {
                free(pBuffer);
            }
            else if(allocType == 1)
            {
                delete [] pBuffer;
            }
        }
    }
    return pOut;
}

wchar_t* StringAppendW(wchar_t* pBuffer, const wchar_t* pAppend, int releaseOldBuffer, int allocType)
{
    // calc new buffer length
    if(!pBuffer && !pAppend)
    {
        return 0;
    }
    size_t len = 1; //for null-terminator
    size_t lenFirst = 0;
    size_t lenSecond = 0;
    if(pBuffer)
    {
        lenFirst = StringLengthW(pBuffer);
        len += lenFirst;
    }
    if(pAppend)
    {
        lenSecond = StringLengthW(pAppend);
        len += lenSecond;
    }
    // allocate buffer
    wchar_t* pOut = 0;
    if(allocType == 0)
    {
        pOut = (wchar_t*)malloc(len * sizeof(wchar_t));
    }
    else if(allocType == 1)
    {
        pOut = new wchar_t[len];
    }
    if(pOut)
    {
        wchar_t* pWrite = pOut;
        if(pBuffer)
        {
            StringCopyW(pWrite, pBuffer);
            pWrite += lenFirst;
        }
        if(pAppend)
        {
            StringCopyW(pWrite, pAppend);
            pWrite += lenSecond;
        }
        *pWrite = 0;

        // release old buffer
        if(pBuffer && releaseOldBuffer == 1)
        {
            if(allocType == 0)
            {
                free(pBuffer);
            }
            else if(allocType == 1)
            {
                delete [] pBuffer;
            }
        }
    }
    return pOut;
}


UTF8FromWCharT::UTF8FromWCharT(const wchar_t* pszw, size_t length)
{
    if(!pszw)
    {
        return;
    }
    UINT32 numInputUnits = (length == (size_t)-1) ? UINT32(MAX_UINT32) : UINT32(length);
    UINT32 numConsumed = 0;
    UINT32 numOutputed = 0;

    // platform uses UTF16 encoding
    if(sizeof(wchar_t) == 2)
    {
        if(ConvertUTF16StringToUTF8String((const UINT16*)pszw, numInputUnits, numConsumed,
                        g_bBigEndianMachine, 0, 0, numOutputed) >= 0)
        {
            m_data = (char*)malloc(numOutputed);
            if(ConvertUTF16StringToUTF8String((const UINT16*)pszw, numInputUnits, numConsumed,
                        g_bBigEndianMachine, (UINT8*)m_data, numOutputed, numOutputed) >= 0)
            {
                m_length = numOutputed; //success
            }
            else if(m_data)
            {
                free(m_data); //failed
            }
        }
        return;
    }
    // platform uses UTF32 encoding
    if(sizeof(wchar_t) == 4)
    {
        if(ConvertUTF32StringToUTF8String((const UINT32*)pszw, numInputUnits, numConsumed,
                        g_bBigEndianMachine, 0, 0, numOutputed) >= 0)
        {
            m_data = (char*)malloc(numOutputed);
            if(ConvertUTF32StringToUTF8String((const UINT32*)pszw, numInputUnits, numConsumed,
                        g_bBigEndianMachine, (UINT8*)m_data, numOutputed, numOutputed) >= 0)
            {
                m_length = numOutputed; //success
            }
            else if(m_data)
            {
                free(m_data); //failed
            }
        }
        return;
    }
    HX_ASSERT(!"Unexpected size of wchar_t");
}


WCharTFromUTF8::WCharTFromUTF8(const char* psz, size_t length)
{
    if(!psz)
    {
        return;
    }
    UINT32 numInputUnits = (length == (size_t)-1) ? UINT32(MAX_UINT32) : UINT32(length);
    UINT32 numConsumed = 0;
    UINT32 numOutputed = 0;

    // platform uses UTF16 encoding
    if(sizeof(wchar_t) == 2)
    {
        if(ConvertUTF8StringToUTF16String((const UINT8*)psz, numInputUnits, numConsumed,
                        0, 0, numOutputed, g_bBigEndianMachine) >= 0)
        {
            m_data = (wchar_t*)malloc(numOutputed * 2);
            if(ConvertUTF8StringToUTF16String((const UINT8*)psz, numInputUnits, numConsumed,
                        (UINT16*)m_data, numOutputed, numOutputed, g_bBigEndianMachine) >= 0)
            {
                m_length = numOutputed; //success
            }
            else if(m_data)
            {
                free(m_data); //failed
            }
        }
        return;
    }
    // platform uses UTF32 encoding
    if(sizeof(wchar_t) == 4)
    {
        if(ConvertUTF8StringToUTF32String((const UINT8*)psz, numInputUnits, numConsumed,
                        0, 0, numOutputed, g_bBigEndianMachine) >= 0)
        {
            m_data = (wchar_t*)malloc(numOutputed * 4);
            if(ConvertUTF8StringToUTF32String((const UINT8*)psz, numInputUnits, numConsumed,
                        (UINT32*)m_data, numOutputed, numOutputed, g_bBigEndianMachine) >= 0)
            {
                m_length = numOutputed; //success
            }
            else if(m_data)
            {
                free(m_data); //failed
            }
        }
        return;
    }
    HX_ASSERT(!"Unexpected size of wchar_t");
}


CCPFromWCharT::CCPFromWCharT(const wchar_t* pszw, size_t length)
{
    if(!pszw)
    {
        return;
    }
#if defined(_WINDOWS) && !defined(FORCE_NO_NATIVE_MULTIBYTE_WIDECHAR)
    int numInputUnits = (length == (size_t)-1) ? -1 : int(length);
    UINT codePage = CP_ACP;
    int iBufferLen = WideCharToMultiByte(codePage, 0, pszw, numInputUnits, 0, 0, 0, 0);
    if(!iBufferLen)
    {
        return;
    }
    m_data = (char*)malloc(iBufferLen);
    if(WideCharToMultiByte(codePage, 0, pszw, numInputUnits, (LPSTR)m_data, iBufferLen, 0, 0) == 0)
    {
        free(m_data);
    }
    else
    {
        m_length = UINT32(iBufferLen);
    }

#else
    // fail for non-ascii strings
    UINT32 numInputUnits = (length == (size_t)-1) ? UINT32(MAX_UINT32) : UINT32(length);
    UINT32 numConsumed = 0;
    UINT32 numOutputed = 0;

    // platform uses UTF16 encoding
    if(sizeof(wchar_t) == 2)
    {
        if(IsUTF16StringAsciiOnly((const UINT16*)pszw, numInputUnits, g_bBigEndianMachine, numConsumed))
        {
            m_data = (char*)malloc(numConsumed);
            if(ConvertUTF16StringToUTF8String((const UINT16*)pszw, numInputUnits, numConsumed, g_bBigEndianMachine,
                                (UINT8*)m_data, numConsumed, numOutputed) < 0)
            {
                free(m_data);
            }
            else
            {
                m_length = numOutputed;
            }
        }
        return;
    }
    // platform uses UTF32 encoding
    if(sizeof(wchar_t) == 4)
    {
        if(IsUTF32StringAsciiOnly((const UINT32*)pszw, numInputUnits, g_bBigEndianMachine, numConsumed))
        {
            m_data = (char*)malloc(numConsumed);
            if(ConvertUTF32StringToUTF8String((const UINT32*)pszw, numInputUnits, numConsumed, g_bBigEndianMachine,
                                (UINT8*)m_data, numConsumed, numOutputed) < 0)
            {
                free(m_data);
            }
            else
            {
                m_length = numOutputed;
            }
        }
        return;
    }
#endif
}


WCharTFromCCP::WCharTFromCCP(const char* psz, size_t length)
{
    if(!psz)
    {
        return;
    }
#if defined(_WINDOWS) && !defined(FORCE_NO_NATIVE_MULTIBYTE_WIDECHAR)
    int numInputUnits = (length == (size_t)-1) ? -1 : int(length);
    UINT codePage = CP_ACP;
    int iBufferLen = MultiByteToWideChar(codePage, MB_PRECOMPOSED, psz, numInputUnits, 0, 0);
    if(!iBufferLen)
    {
        return;
    }
    m_data = (wchar_t*)malloc(iBufferLen * 2);
    if(MultiByteToWideChar(codePage, MB_PRECOMPOSED, psz, numInputUnits, (LPWSTR)m_data, iBufferLen) == 0)
    {
        free(m_data);
    }
    else
    {
        m_length = UINT32(iBufferLen);
    }

#else
    // fail for non-ascii strings
    UINT32 numInputUnits = (length == (size_t)-1) ? UINT32(MAX_UINT32) : UINT32(length);
    UINT32 numConsumed = 0;
    UINT32 numOutputed = 0;

    if(IsUTF8StringAsciiOnly((const UINT8*)psz, numInputUnits, numConsumed))
    {
        // platform uses UTF16 encoding
        if(sizeof(wchar_t) == 2)
        {
            m_data = (wchar_t*)malloc(numConsumed * 2);
            if(ConvertUTF8StringToUTF16String((const UINT8*)psz, numInputUnits, numConsumed,
                                (UINT16*)m_data, numConsumed, numOutputed, g_bBigEndianMachine) < 0)
            {
                free(m_data);
            }
            else
            {
                m_length = numOutputed;
            }
            return;
        }
        // platform uses UTF32 encoding
        if(sizeof(wchar_t) == 4)
        {
            m_data = (wchar_t*)malloc(numConsumed * 4);
            if(ConvertUTF8StringToUTF32String((const UINT8*)psz, numInputUnits, numConsumed,
                                (UINT32*)m_data, numConsumed, numOutputed, g_bBigEndianMachine) < 0)
            {
                free(m_data);
            }
            else
            {
                m_length = numOutputed;
            }
        }
        return;
    }
#endif
}


UTF8FromCCP::UTF8FromCCP(const char* psz, size_t length)
{
    WCharTFromCCP wide(psz, length);
    UTF8FromWCharT utf8((const wchar_t*)wide, wide.Length());
    m_data = utf8.Release();
    m_length = utf8.Length();
}


CCPFromUTF8::CCPFromUTF8(const char* psz, size_t length)
{
    WCharTFromUTF8 wide(psz, length);
    CCPFromWCharT ccp((const wchar_t*)wide, wide.Length());
    m_data = ccp.Release();
    m_length = ccp.Length();
}


//----------------------------------------------------------------------------
// Legacy utilities
//----------------------------------------------------------------------------
// XXX
// This should be reviewed.
// With current logic we're using ActiveCodePage on CE Windows and UTF8 on non-CE Windows.
#ifdef _WINCE
#  if !defined(CP_UTF8)
#     define CP_UTF8 CP_ACP
#  else //!CP_UTF8
#     if (defined(_X86_) || (WINVER >= 0x500)) && (CP_UTF8==65001)
#       undef CP_UTF8
#       define CP_UTF8 CP_ACP
#     endif //_X86_ && (CP_UTF8==65001)
#  endif //!CP_UTF8
#endif //_WINCE

#ifndef _WINDOWS
#define CP_UTF8 65001
#endif //_WINDOWS


static int MultiByteToWideChar_Dispatch(
                        UINT32 CodePage,            // code page
                        ULONG32 dwFlags,            // character-type options
                        const char* lpMultiByteStr, // string to map
                        int cchMultiByte,           // number of bytes in string
                        wchar_t* lpWideCharStr,     // wide-character buffer
                        int cchWideChar)            // size of buffer
{
#if defined(_WINDOWS) && !defined(FORCE_NO_NATIVE_MULTIBYTE_WIDECHAR)
    return MultiByteToWideChar(CodePage, dwFlags, lpMultiByteStr, cchMultiByte,
                    lpWideCharStr, cchWideChar);
#else
    // check arguments
    if(!cchMultiByte || (CodePage != CP_UTF8))
    {
        return 0;
    }
    // translate arguments
    UINT32 numInputUnits = UINT32(cchMultiByte);
    if(cchMultiByte == -1)
    {
        numInputUnits = MAX_UINT32;
    }
    UINT32 numConsumed = 0;
    UINT32 numOutputed = 0;

    // platform uses UTF16 encoding
    if(sizeof(wchar_t) == 2)
    {
        if(ConvertUTF8StringToUTF16String((const UINT8*)lpMultiByteStr, numInputUnits, numConsumed,
                        (UINT16*)lpWideCharStr, (UINT32)cchWideChar, numOutputed, g_bBigEndianMachine) < 0)
        {
            return 0; //failure
        }
        return int(numOutputed);
    }
    // platform uses UTF32 encoding
    if(sizeof(wchar_t) == 4)
    {
        if(ConvertUTF8StringToUTF32String((const UINT8*)lpMultiByteStr, numInputUnits, numConsumed,
                        (UINT32*)lpWideCharStr, (UINT32)cchWideChar, numOutputed, g_bBigEndianMachine) < 0)
        {
            return 0; //failure
        }
        return int(numOutputed);
    }
    HX_ASSERT(!"Unexpected size of wchar_t");
    return 0; //failure

#endif
}


static int WideCharToMultiByte_Dispatch(
                        UINT32 CodePage,                // code page
                        ULONG32 dwFlags,                // performance and mapping flags
                        const wchar_t* lpWideCharStr,   // wide-character string
                        int cchWideChar,                // number of characters
                        char* lpMultiByteStr,           // buffer for new string
                        int cchMultiByte,               // size of buffer
                        char* lpDefaultChar,            // default for unmappable characters
                        HXBOOL* lpUsedDefaultChar)      // flag set when default char used
{
#if defined(_WINDOWS) && !defined(FORCE_NO_NATIVE_MULTIBYTE_WIDECHAR)
    return WideCharToMultiByte(CodePage, dwFlags, lpWideCharStr, cchWideChar,
                    lpMultiByteStr, cchMultiByte, lpDefaultChar, lpUsedDefaultChar);
#else
    // check arguments
    if(!cchWideChar || (CodePage != CP_UTF8))
    {
        return 0;
    }
    // translate arguments
    UINT32 numInputUnits = UINT32(cchWideChar);
    if(cchWideChar == -1)
    {
        numInputUnits = MAX_UINT32;
    }
    UINT32 numConsumed = 0;
    UINT32 numOutputed = 0;

    // platform uses UTF16 encoding
    if(sizeof(wchar_t) == 2)
    {
        if(ConvertUTF16StringToUTF8String((const UINT16*)lpWideCharStr, numInputUnits, numConsumed,
                        g_bBigEndianMachine, (UINT8*)lpMultiByteStr, (UINT32)cchMultiByte, numOutputed) < 0)
        {
            return 0; //failure
        }
        return int(numOutputed);
    }
    // platform uses UTF32 encoding
    if(sizeof(wchar_t) == 4)
    {
        if(ConvertUTF32StringToUTF8String((const UINT32*)lpWideCharStr, numInputUnits, numConsumed,
                        g_bBigEndianMachine, (UINT8*)lpMultiByteStr, (UINT32)cchMultiByte, numOutputed) < 0)
        {
            return 0; //failure
        }
        return int(numOutputed);
    }
    HX_ASSERT(!"Unexpected size of wchar_t");
    return 0; //failure

#endif
}


HLXOsStrW::HLXOsStrW(const char* ascii, size_t length) : 
    m_isMutable(FALSE), 
    m_toAscii(TRUE),
    m_size(0),
    m_outsize(0),
    m_uni(0),
    m_ascii(0)
{
    Init(ascii, length);
}

HLXOsStrW::HLXOsStrW(char* ascii, size_t length) : 
    m_isMutable(TRUE), 
    m_toAscii(TRUE),
    m_size(0),
    m_outsize(0),
    m_uni(0),
    m_ascii(ascii)
{ 
    Init(ascii, length);
}

HLXOsStrW::HLXOsStrW(const unsigned char* ascii, size_t length) : 
    m_isMutable(FALSE), 
    m_toAscii(TRUE),
    m_size(0),
    m_outsize(0),
    m_uni(0),
    m_ascii(0)
{ 
    Init((const char*) ascii, length);
}

void HLXOsStrW::Init(const char* ascii, size_t length)
{
    m_size = ((length != (size_t)-1) ? length : ((ascii) ? StringLengthA((const char*) ascii) + 1 : 0));

    if (ascii)
    {
	m_outsize = MultiByteToWideChar_Dispatch(CP_UTF8, 0, (const char*) ascii, length, NULL, 0);
	if ((length != (size_t)-1) && (length != 0))
	  m_outsize = ((size_t)m_outsize < length)?length:m_outsize;
	m_uni = ((wchar_t*) malloc(m_outsize * sizeof(wchar_t)));
	if (m_uni)
	{
	    m_outsize = MultiByteToWideChar_Dispatch(CP_UTF8, 0, (const char*) ascii, length, m_uni, m_outsize);
	}
	else
	{
	    m_outsize = 0;
	}
    }
}

#ifndef ANDROID
HLXOsStrW::HLXOsStrW(const wchar_t* uni, size_t length) : 
    m_isMutable(FALSE), 
    m_toAscii(FALSE),
    m_size((length != (size_t)-1) ? length : ((uni) ? StringLengthW(uni) + 1 : 0)),
    m_uni(0),
    m_ascii(0)
{ 
    if (uni)
    {
	m_outsize = WideCharToMultiByte_Dispatch(CP_UTF8, 0, uni, length, NULL, 0, NULL, NULL);
	if ((length != (size_t)-1) && (length != 0))
	  m_outsize = ((size_t)m_outsize < length)?length:m_outsize;
	m_ascii = ((char*) malloc(m_outsize));
	if (m_ascii)
	{
	    m_outsize = WideCharToMultiByte_Dispatch(CP_UTF8, 0, uni, length, m_ascii, m_outsize, NULL, NULL); 
	}
	else
	{
	    m_outsize = 0;
	}
    }
}
#endif

HLXOsStrW::~HLXOsStrW() 
{ 
    if (m_isMutable) 
    {
	if (m_toAscii && m_ascii && m_uni)
	{
	    WideCharToMultiByte_Dispatch(CP_UTF8, 0, m_uni, -1, m_ascii, m_size, NULL, NULL);
	}
    }
    if (m_toAscii) 
    {
	if (m_uni)
	{
	    free(m_uni);
	}
    } 
    else 
    {
	if (m_ascii)
	{
	    free(m_ascii);
	}
    }
}

HLXOsStrW::HLXOsStrW(const HLXOsStrW& rhs) :
    m_isMutable(FALSE),
    m_toAscii(TRUE),
    m_size(0),
    m_outsize(0),
    m_uni(0),
    m_ascii(0)
{    
    Copy(*this, rhs);
}

HLXOsStrW& HLXOsStrW::operator=(const HLXOsStrW& rhs)
{
    if (&rhs != this)
    {
	Copy(*this, rhs);
    }

    return *this;
}

void HLXOsStrW::Copy(HLXOsStrW& lhs, const HLXOsStrW& rhs)
{
    lhs.m_isMutable = rhs.m_isMutable;
    lhs.m_toAscii = rhs.m_toAscii;
    lhs.m_size = rhs.m_size;

    if (lhs.m_toAscii)
    {
	if (rhs.m_uni)
	{
	    int bufSize = rhs.m_outsize * sizeof(wchar_t);
	    
	    if (lhs.m_uni)
	    {
		free(lhs.m_uni);
	    }
	    lhs.m_uni = (wchar_t*) malloc(bufSize);
	    if (lhs.m_uni)
	    {
		lhs.m_outsize = rhs.m_outsize;
		memcpy(lhs.m_uni, rhs.m_uni, bufSize); /* Flawfinder: ignore */
	    }
	}
	lhs.m_ascii = rhs.m_ascii;
    }
    else
    {
	if (rhs.m_ascii)
	{
	    if (lhs.m_ascii)
	    {
		free(lhs.m_ascii);
	    }
	    lhs.m_ascii = (char*) malloc(rhs.m_outsize);
	    if (lhs.m_ascii)
	    {
		lhs.m_outsize = rhs.m_outsize;
		memcpy(lhs.m_ascii, rhs.m_ascii, lhs.m_outsize); /* Flawfinder: ignore */
	    }
	}
	lhs.m_uni = rhs.m_uni;
    }
}


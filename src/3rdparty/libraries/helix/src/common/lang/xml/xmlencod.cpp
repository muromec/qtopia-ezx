/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: xmlencod.cpp,v 1.11 2005/04/04 17:48:46 ehyche Exp $
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


#include "hxstrutl.h"
#include "xmlencod.h"
#include "xmlvalid.h"

#include "hxresult.h"
#include "hxassert.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE             
static const char HX_THIS_FILE[] = __FILE__;
#endif

struct xmlEncodingInfo
{
    char*   m_pIANAName;                // name of charset, case-insensitive
    HXBOOL    m_bDoubleByte;            // true if DBCS charset
    BYTE    m_szLeadingByteRange[12];   // up to 5 zero-terminated
                                        // pairs of lead byte ranges
} ;

// Removed static here because it exercises a bug in gcc.  pgodman, 3/2/2000
const xmlEncodingInfo XMLEncodingInfo[] =
{
    { "Default", TRUE,      // assume DBCS; any non-ASCII char is a lead byte
      {0x81, 0xFF, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },

    { "US-ASCII", FALSE, 
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },

    { "Shift-JIS", TRUE, 
      {0x81, 0x9F, 0xE0, 0xFC, 0x00, 0x00, 
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },

    { "Big5", TRUE,
      {0x81, 0xFE, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },

    { "GB2312", TRUE,
      {0xA1, 0xFE, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },

    { "EUC-KR", TRUE,
      {0x84, 0xD3, 0xD8, 0xD8, 0xD9, 0xDE,
       0xE0, 0xF9, 0x00, 0x00, 0x00, 0x00} },

    { "ISO-2022-KR", TRUE,
      {0x81, 0xFE, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },

    { "ISO-8859-1", FALSE,
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },

};

CHXXMLEncode::CHXXMLEncode(const char* pEncoding,
                           BYTE* pBuffer,
                           UINT32 ulLength):
    m_pBuffer(pBuffer),
    m_pCurrent(pBuffer),
    m_ulBufferLength(ulLength)
{
    m_lEncodingIndex = GetEncodingIndex(pEncoding);
}

CHXXMLEncode::CHXXMLEncode(const char* pEncoding,
                           BYTE* pStr):
    m_pBuffer(pStr),
    m_pCurrent(pStr)
{
    m_ulBufferLength = strlen((char*)pStr) + 1;
    m_lEncodingIndex = GetEncodingIndex(pEncoding);
}

CHXXMLEncode::CHXXMLEncode(const CHXXMLEncode& lhs)
{
    m_ulBufferLength = lhs.m_ulBufferLength;
    m_pBuffer = lhs.m_pBuffer;
    m_pCurrent = m_pBuffer;
    m_lEncodingIndex = lhs.m_lEncodingIndex;
}

CHXXMLEncode::~CHXXMLEncode()
{
}

const CHXXMLEncode&
CHXXMLEncode::operator=(const CHXXMLEncode& lhs)
{
    m_ulBufferLength = lhs.m_ulBufferLength;
    m_pBuffer = lhs.m_pBuffer;
    m_pCurrent = m_pBuffer;     // start at 0
    m_lEncodingIndex = lhs.m_lEncodingIndex;

    return *this;
}

CHXXMLEncode::operator const BYTE*() const
{
    return (const BYTE*)m_pBuffer;
}

BYTE*
CHXXMLEncode::operator+(int offset)
{
    BYTE* pCh = m_pCurrent;
    for(int nCount = 0; nCount < offset; ++nCount)
    {
        UINT16 uLen = 0;
        GetNextChar(uLen);
    }
    BYTE* pOffset = m_pCurrent;
    m_pCurrent = pCh;
    return pOffset;
}

BYTE*
CHXXMLEncode::operator+=(int offset)
{
    BYTE* pCh = m_pCurrent;
    for(int nCount = 0; nCount < offset; ++nCount)
    {
        UINT16 uLen = 0;
        pCh = GetNextChar(uLen);
    }
    return pCh;
}

BYTE*
CHXXMLEncode::operator++(int)
{
    UINT16 uLen = 0;
    return GetNextChar(uLen);
}

BYTE*
CHXXMLEncode::operator--(int)
{
    UINT16 uLen = 0;
    return GetPrevChar(uLen);
}

HXBOOL
CHXXMLEncode::IsLeadByte(BYTE ch)
{
    int bRange = 0;

    const xmlEncodingInfo* pEncInfo = &(XMLEncodingInfo[m_lEncodingIndex]);
    if(pEncInfo->m_bDoubleByte)
    {
        while((bRange < 12) &&
              (pEncInfo->m_szLeadingByteRange[bRange] != 0))
        {
            if(ch >= pEncInfo->m_szLeadingByteRange[bRange] &&
               ch <= pEncInfo->m_szLeadingByteRange[bRange + 1])
            {
                return TRUE;
            }
            bRange += 2;
        }
    }
    return FALSE;
}

BYTE*
CHXXMLEncode::GetNextChar(UINT16& uLen)
{
    BYTE* pCh = m_pCurrent;
    if(IsLeadByte(*m_pCurrent))
    {
        m_pCurrent += 2;    // skip 2 bytes
        uLen = 2;
        return pCh;
    }
    m_pCurrent++;   // skip 1 byte
    uLen = 1;
    return pCh;
}

BYTE*
CHXXMLEncode::GetPrevChar(UINT16& uLen)
{
    BYTE* pCh = m_pCurrent;
    // assumes the m_pCurrent is on a boundary, i.e. it doesn't
    // point to the middle of a DBCS character

    // if current is at beginning or the previous byte is
    // at the beginning, return current
    if(m_pCurrent - 1 <= m_pBuffer)
    {
        return pCh;
    }

    // point to the previous byte
    BYTE* pTemp = m_pCurrent - 1;

    // if pTemp points to a lead byte, then it must be a trail byte;
    // decrement current 2 bytes to lead byte
    if(IsLeadByte(*pTemp))
    {
        m_pCurrent -= 2;
        pCh = m_pCurrent;
        uLen = 2;
        return pCh;
    }

    // otherwise, step back unil a non-lead byte is found
    while(m_pCurrent <= --pTemp && IsLeadByte(*pTemp))
    {
        ;
    }

    // now pTemp + 1 must point to the beginning of a character,
    // so figure out whether we went back to an even or an odd
    // number of bytes and go back 1 or 2 bytes, respectively.
    m_pCurrent = m_pCurrent - 1 - ((m_pCurrent - pTemp) & 1);

    pCh = m_pCurrent;
    if(IsLeadByte(*pCh))
    {
        uLen = 2;
    }
    else
    {
        uLen = 1;
    }
    return pCh;
}

BYTE*
CHXXMLEncode::SetCurrent(BYTE* pCh)
{
    // no checking other than boundary;
    // if it is set on the trail byte
    // of a DBCS character, bad things
    // will happen...
    HX_ASSERT(pCh >= m_pBuffer &&
              pCh <= m_pBuffer + m_ulBufferLength);
    m_pCurrent = pCh;
    return m_pCurrent;
}

UINT32
CHXXMLEncode::CharCount()
{
    UINT32 ulLength = 0;
    BYTE* pCh = m_pBuffer;

    if(XMLEncodingInfo[m_lEncodingIndex].m_bDoubleByte)
    {
        UINT16 uLen = 0;
        for(; *pCh; pCh = GetNextChar(uLen))
        {
            ++ulLength;
        }
    }
    else
    {
        for(; *pCh; pCh++)
        {
            ++ulLength;
        }
    }
    return ulLength;
}

///@todo sizeof returns size_t which can be larger than INT32 (signed) and cause an infinite loop
INT32
CHXXMLEncode::GetEncodingIndex(const char* pEncoding)
{
    INT32 lIndex = 0;   // default
    for(UINT32 lCount = 0; 
        lCount < sizeof(XMLEncodingInfo) / sizeof(XMLEncodingInfo[0]);
        ++lCount)
    {
        if(strcasecmp(pEncoding, XMLEncodingInfo[lCount].m_pIANAName) == 0)
        {
            lIndex = lCount;
            break;
        }
    }
    return lIndex;
}

HXBOOL
CHXXMLEncode::IsRefValid(const BYTE* p, UINT32 len)
{
    if (!XMLEncodingInfo[m_lEncodingIndex].m_bDoubleByte)
    {
        return ISO8859Valid::IsRefValid(p, len);
    }
    else
    {
        // TODO: Double byte Validation
        return TRUE;
    }
}

HXBOOL
CHXXMLEncode::IsNameValid(const BYTE* p, UINT32 len)
{
    if (!XMLEncodingInfo[m_lEncodingIndex].m_bDoubleByte)
    {
        return ISO8859Valid::IsNameValid(p, len);
    }
    else
    {
        // TODO: Double byte Validation.
        return TRUE;
    }
}

HXBOOL
CHXXMLEncode::IsNmtokenValid(const BYTE*p, UINT32 len)
{
    if (!XMLEncodingInfo[m_lEncodingIndex].m_bDoubleByte)
    {
        return ISO8859Valid::IsNmtokenValid(p, len);
    }
    else
    {
        // TODO: Double byte Validation.
        return TRUE;
    }
}

HXBOOL
CHXXMLEncode::IsEntityValueValid(const BYTE* p, UINT32 len)
{
    if (!XMLEncodingInfo[m_lEncodingIndex].m_bDoubleByte)
    {
        return ISO8859Valid::IsEntityValueValid(p, len);
    }
    else
    {
        return TRUE;
    }
}

HXBOOL 
CHXXMLEncode::IsAttValueValid(const BYTE* p, UINT32 len)
{
    if (!XMLEncodingInfo[m_lEncodingIndex].m_bDoubleByte)
    {
        return ISO8859Valid::IsAttValueValid(p, len);
    }
    else
    {
        // TODO: Double byte Validation.
        return TRUE;
    }
}

HXBOOL 
CHXXMLEncode::IsSystemLiteralValid(const BYTE* p, UINT32 len)
{
    if (!XMLEncodingInfo[m_lEncodingIndex].m_bDoubleByte)
    {
        return ISO8859Valid::IsSystemLiteralValid(p, len);
    }
    else
    {
        // TODO: Double byte Validation.
        return TRUE;
    }
}

HXBOOL
CHXXMLEncode::IsPubidLiteralValid(const BYTE* p, UINT32 len)
{
    if (!XMLEncodingInfo[m_lEncodingIndex].m_bDoubleByte)
    {
        return ISO8859Valid::IsPubidLiteralValid(p, len);
    }
    else
    {
        // TODO: Double byte Validation.
        return TRUE;
    }
}

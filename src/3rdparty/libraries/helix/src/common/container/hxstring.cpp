/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxstring.cpp,v 1.22 2007/11/08 10:43:26 atin Exp $
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

#include "hxstring.h"

#ifdef HELIX_FEATURE_SERVER_CHXSTRING_ATOMIC_REFCOUNT
#include "atomicbase.h"
#endif

#include "hlxclib/string.h"
#include "hlxclib/ctype.h"
#include "hxassert.h"

#ifdef HELIX_FEATURE_STR_2X_GROWTH
#define DEFAULT_GROWTH_FUNC CHXString::DoublingGrowth
#else
#define DEFAULT_GROWTH_FUNC CHXString::MinimalGrowth
#endif /* HELIX_FEATURE_STR_2X_GROWTH */


#if !defined(HELIX_CONFIG_NOSTATICS)
const CHXString HXEmptyString;
#else
const char* const _g_emptyString = NULL;
#endif


CHXStringRep::CHXStringRep(INT32 strSize, bool bSetLength) :
    m_refCount(1),
    m_strSize(0),
    m_bufSize((strSize > 0) ? strSize + 1 : 1),
    m_pData(new char[m_bufSize]) // Depends on m_bufSize being initialized
{
    if( m_pData )
    {
        m_pData[0] = '\0';

        if (bSetLength)
        {
	    m_strSize = strSize;
	    m_pData[m_strSize] = '\0';
        }
    }
}

CHXStringRep::CHXStringRep(const char* pStr) :
    m_refCount(1),
    m_strSize((pStr) ? strlen(pStr) : 0),
    m_bufSize(m_strSize + 1),    // Depends on m_strSize being initialized
    m_pData(new char[m_bufSize]) // Depends on m_bufSize being initialized
{
    if( m_pData )
    {
        if (pStr)
            strcpy(m_pData, pStr); /* Flawfinder: ignore */
        else
            m_pData[0] = '\0';
    }
}

CHXStringRep::CHXStringRep(const char* pStr, INT32 strSize) :
    m_refCount(1),
    m_strSize(strSize),
    m_bufSize((strSize > 0) ? strSize + 1: 1),
    m_pData(new char[m_bufSize]) // Depends on m_bufSize being initialized
{
    if( m_pData )
    {
        if (pStr)
	    strncpy(m_pData, pStr, m_strSize); /* Flawfinder: ignore */

        m_pData[m_strSize] = '\0';
    
        m_strSize = strlen(m_pData);
    }
}

CHXStringRep::CHXStringRep(char ch, INT32 count) :
    m_refCount(1),
    m_strSize((ch) ? count : 0),
    m_bufSize(count + 1),
    m_pData(new char[m_bufSize]) // Depends on m_bufSize being initialized
{
    if( m_pData )
    {
        memset(m_pData, ch, count);
        m_pData[m_strSize] = '\0';
    }
}

CHXStringRep::~CHXStringRep()
{
    HX_VECTOR_DELETE(m_pData);
    m_pData = 0;
}

void CHXStringRep::AddRef()
{
#ifndef HELIX_FEATURE_SERVER_CHXSTRING_ATOMIC_REFCOUNT
    m_refCount++;
#else
    HXAtomicIncRetINT32(&m_refCount);
#endif
}

void CHXStringRep::Release()
{
#ifndef HELIX_FEATURE_SERVER_CHXSTRING_ATOMIC_REFCOUNT
    if ((--m_refCount) == 0)
#else
    if (HXAtomicDecRetINT32(&m_refCount) <= 0)
#endif
	delete this;
}

void CHXStringRep::Resize(INT32 newStrSize)
{
    HX_ASSERT(newStrSize >= 0);

    INT32 newBufSize = newStrSize + 1;

    if (newBufSize != m_bufSize)
    {
	delete [] m_pData;
	m_pData = new char[newBufSize];
	m_bufSize = newBufSize;
    }
}

void CHXStringRep::ResizeAndCopy(INT32 newStrSize, bool bSetLength)
{
    HX_ASSERT(newStrSize >= 0);

    INT32 newBufSize = newStrSize + 1;

    if (newBufSize != m_bufSize)
    {
	char* pNewBuf = new char[newBufSize];
        if( !pNewBuf )
        {
            // It would be swell to be able to notify the caller that we are
            // out of memory.
            return;
        }

	if (newStrSize < m_strSize)
	    m_strSize = newStrSize;

	if (m_pData)
	    strncpy(pNewBuf, m_pData, m_strSize); /* Flawfinder: ignore */

	pNewBuf[m_strSize] = '\0';

	if (bSetLength)
	{
	    m_strSize = newStrSize;
	    pNewBuf[m_strSize] = '\0';
	}

	delete [] m_pData;
	m_pData = pNewBuf;
	m_bufSize = newBufSize;
    }
}

void CHXStringRep::Copy(const char* pStr, INT32 strSize)
{
    HX_ASSERT(strSize >= 0);
    
    if (m_bufSize < (strSize + 1))
	Resize(strSize);

    if( m_pData )
    {
    strncpy(m_pData, pStr, strSize); /* Flawfinder: ignore */
    m_pData[strSize] = '\0';
    m_strSize = strSize;
    }
}

CHXString::CHXString(StringGrowthFunc pGrowthFunc) :
    m_pRep(0),
    m_pGrowthFunc((pGrowthFunc) ? pGrowthFunc : DEFAULT_GROWTH_FUNC)
{}

CHXString::CHXString(const CHXString& rhs) :
    m_pRep(rhs.m_pRep),
    m_pGrowthFunc(rhs.m_pGrowthFunc)
{
    if (m_pRep)
	m_pRep->AddRef();
}

CHXString::CHXString(char ch, int length,
		     StringGrowthFunc pGrowthFunc) :
    m_pRep(new CHXStringRep(ch, length)),
    m_pGrowthFunc((pGrowthFunc) ? pGrowthFunc : DEFAULT_GROWTH_FUNC)
{}

CHXString::CHXString(const char* pStr, 
		     StringGrowthFunc pGrowthFunc) :
    m_pRep(NULL),
    m_pGrowthFunc((pGrowthFunc) ? pGrowthFunc : DEFAULT_GROWTH_FUNC)
{
    if (pStr && *pStr)
    {
	/* Only create a CHXStringRep if the string
	 * is not empty
	 */
	m_pRep = new CHXStringRep(pStr);
    }
}

CHXString::CHXString(const char* pStr, int length,
		     StringGrowthFunc pGrowthFunc) :
    m_pRep(NULL),
    m_pGrowthFunc((pGrowthFunc) ? pGrowthFunc : DEFAULT_GROWTH_FUNC)
{
    if (pStr && (length > 0) && *pStr)
    {
	/* Only create a CHXStringRep if the string
	 * is not empty
	 */
	m_pRep = new CHXStringRep(pStr, length);
    }
}

CHXString::CHXString(const unsigned char* pStr,
		       StringGrowthFunc pGrowthFunc) :
    m_pRep(NULL),
    m_pGrowthFunc((pGrowthFunc) ? pGrowthFunc : DEFAULT_GROWTH_FUNC)
{
    if (pStr && *pStr)
    {
	/* Only create a CHXStringRep if the string
	 * is not empty
	 */
	m_pRep = new CHXStringRep((const char*)pStr);
    }
}

CHXString::~CHXString()
{
    if (m_pRep)
    {
        m_pRep->Release();
        m_pRep = NULL;
    }
}

void CHXString::Empty()
{
    if (m_pRep)
    {
        m_pRep->Release();
        m_pRep = NULL;
    }
}

void CHXString::SetAt(INT32 i, char ch)
{
    HX_ASSERT(m_pRep && (i < m_pRep->GetBufferSize()));

    if (m_pRep)
    {
	EnsureUnique();
	m_pRep->GetBuffer()[i] = ch;
    }
}

const CHXString& CHXString::operator=(const CHXString& rhs)
{
    if (&rhs != this)
    {
	if (m_pRep)
	    m_pRep->Release();

	m_pRep = rhs.m_pRep;
	
	if (m_pRep)
	    m_pRep->AddRef();

	m_pGrowthFunc = rhs.m_pGrowthFunc;
    }

    return *this;
}

const CHXString& CHXString::operator=(char ch)
{
    if (m_pRep)
    {
	EnsureUnique();
	if (m_pRep->GetBufferSize() < 2)
	    m_pRep->Resize(1);
	
	m_pRep->GetBuffer()[0] = ch;
	m_pRep->GetBuffer()[1] = '\0';
	
	if (ch)
	    m_pRep->SetStringSize(1);
	else
	    m_pRep->SetStringSize(0);
    }
    else
	m_pRep = new CHXStringRep(ch, 1);

    return *this;
}

const CHXString& CHXString::operator=(const char* pStr)
{
    if (m_pRep)
    {
	EnsureUnique();
	m_pRep->Copy(pStr, SafeStrlen(pStr));
    }
    else if (pStr && *pStr)
	m_pRep = new CHXStringRep(pStr);

    return *this;
}

const CHXString& CHXString::operator=(const unsigned char* pStr)
{
    if (m_pRep)
    {
	EnsureUnique();
	m_pRep->Copy((const char*)pStr, SafeStrlen((const char*)pStr));
    }
    else if (pStr && *pStr)
	m_pRep = new CHXStringRep((const char*)pStr);

    return *this;
}

const CHXString& CHXString::operator+=(const CHXString& rhs)
{
    // Be careful here. You must make sure that this implementation
    // handles the case where (&rhs == this)

    if (rhs.m_pRep)
	Append(rhs.m_pRep->GetBuffer(), rhs.m_pRep->GetStringSize());
    
    return *this;
}

const CHXString& CHXString::operator+=(char ch)
{
    if (ch)
	Append(&ch, 1);

    return *this;
}

const CHXString& CHXString::operator+=(const char* pStr)
{
    // Make sure that someone is not trying to be tricky and
    // append part of this string to itself.
    HX_ASSERT(!m_pRep ||
	      (pStr < m_pRep->GetBuffer()) ||
	      (pStr > m_pRep->GetBuffer() + m_pRep->GetBufferSize()));

    Append(pStr, SafeStrlen(pStr));

    return *this;
}

CHXString operator+(const CHXString& strA, const CHXString& strB)
{
    CHXString ret(strA);
    ret += strB;
    return ret;
}

CHXString operator+ (const CHXString& str, char ch)
{
    CHXString ret(str);
    ret += ch;
    return ret;
}

CHXString operator+ (char ch , const CHXString& str)
{
    CHXString ret(ch);
    ret += str;
    return ret;
}

CHXString operator+ (const CHXString& strA, const char* pStrB)
{
    CHXString ret(strA);
    ret += pStrB;
    return ret;
}

CHXString operator+ (const char* pStrA, const CHXString& strB)
{
    CHXString ret(pStrA);
    ret += strB;
    return ret;
}

char* CHXString::GetBuffer(INT32 minSize)
{
    // NOTE: minSize is string length, not including ending zero byte...

    HX_ASSERT(minSize >= 0);

    if (m_pRep)
    {
	if (m_pRep->GetBufferSize() < (minSize + 1))
	{
	    EnsureUnique();
	    m_pRep->ResizeAndCopy(minSize);
	}
    }
    else
	m_pRep = new CHXStringRep(minSize);

    return m_pRep->GetBuffer();
}

void CHXString::ReleaseBuffer(INT32 newSize)
{
    // NOTE: newSize is string length, not including ending zero byte...

    if (m_pRep)
    {
        // Update the string size since the caller could've changed the
        // internal data (the whole point of this GetBuffer()/ReleaseBuffer()
        // stuff).

        char* pBuf = m_pRep->GetBuffer();

        if (newSize >= m_pRep->GetBufferSize())
        {
            HX_ASSERT(newSize < m_pRep->GetBufferSize());
            // ...so if it's too big, clamp it to the max available.
            newSize = m_pRep->GetBufferSize() - 1;
        }

        if (newSize >= 0)
            pBuf[newSize] = '\0';
        else
            newSize = strlen(pBuf);

        if (newSize > 0)
        {
            m_pRep->SetStringSize(newSize);
            m_pRep->ResizeAndCopy(newSize);
        }
        else
        {
            if (m_pRep)
            {
                m_pRep->Release();
                m_pRep = NULL;
            }
        }
    }
    else
    {
        HX_ASSERT(!"Shouldn't call ReleaseBuffer() without GetBuffer()");

        if (newSize > 0)
            m_pRep = new CHXStringRep(newSize);
    }
}

char* CHXString::GetBufferSetLength(INT32 newSize)
{
    // NOTE : newSize is a string length, not including ending zero byte...

    HX_ASSERT(newSize >= 0);

    if (m_pRep)
    {
	EnsureUnique();
        m_pRep->ResizeAndCopy(newSize, true);
    }
    else if (newSize > 0)
	m_pRep = new CHXStringRep(newSize, true);

    return m_pRep->GetBuffer();
}

void CHXString::FreeExtra()
{
    if (m_pRep)
    {
        INT32 newSize = GetLength();
        if (newSize > 0)
	{
	    EnsureUnique();
            m_pRep->ResizeAndCopy(newSize);
	}
        else
        {
            if (m_pRep)
            {
                m_pRep->Release();
                m_pRep = NULL;
            }
        }
    }
}

INT32 CHXString::GetAllocLength() const
{
    return GetLength();
}

INT32 CHXString::SetMinBufSize(INT32 minSize)
{
    // NOTE: minSize is a string length, not including ending zero byte...

    HX_ASSERT(minSize >= 0);

    INT32 ret = 0;

    if (m_pRep)
    {
	if (minSize >= m_pRep->GetStringSize())
	{
	    if (minSize)
	    {
		EnsureUnique();
		m_pRep->ResizeAndCopy(minSize);
	    }
	    else
            {
                if (m_pRep)
                {
                    m_pRep->Release();
                    m_pRep = NULL;
                }
            }
	}
	
	if (m_pRep)
	    ret = m_pRep->GetBufferSize() - 1;
    }
    else if (minSize > 0)
    {
	m_pRep = new CHXStringRep(minSize);
	ret = minSize;
    }

    return ret;
}

#if defined(_MACINTOSH) || defined(_MAC_UNIX)

#include "platform/mac/fullpathname.h"

#include "platform/mac/cfwrappers.h"

const CHXString& CHXString::SetFromStr255(const Str255 src)
{
    char temp[256]; /* Flawfinder: ignore */
    
    if (src==NULL)
    {
        Empty();
    }
    else
    {
        memcpy (temp, &src[1], src[0]); /* Flawfinder: ignore */
        temp[src[0]] = 0;
        *this = CHXString( temp, src[0]);
    }
    
    return *this;
}

const CHXString& CHXString::AppendFromStr255(const Str255 src)
{
    char temp[256]; /* Flawfinder: ignore */
    
    if (src!=NULL)
    {
        memcpy (temp, &src[1], src[0]); /* Flawfinder: ignore */
        temp[src[0]] = 0;

        CHXString tempS;
        tempS = temp;
        *this = *this + tempS;
    }
    return *this;
}

const CHXString& CHXString::InsertFromStr255(const Str255 src)
{
    char temp[256]; /* Flawfinder: ignore */
    
    if (src!=NULL)
    {
        memcpy (temp, &src[1], src[0]); /* Flawfinder: ignore */
        temp[src[0]] = 0;

	CHXString tempS;
	tempS = temp;
	*this = tempS + *this;
    }
    
    return *this;
}
    
const CHXString& CHXString::SetFromIndString(short strlist, short item)
{
    Str255	theStr;

    GetIndString(theStr,strlist,item);

    SetFromStr255(theStr);

    return *this;
}

const CHXString& CHXString::operator =(FSSpec spec)
{
    CHXString	temp;
    
    PathNameFromFSSpec(&spec, temp);

    *this=temp;
    return *this;
}

CHXString::operator const FSSpec(void)
{
    FSSpec	spec;
    (void) FSSpecFromPathName((const char*)(*this), &spec);
    return spec;
}

HX_RESULT CHXString::MakeStr255(Str255& outPascalString) const
{
    UINT32 len = GetLength();

    if (m_pRep)
	c2pstrcpy(outPascalString, m_pRep->GetBuffer());
    
    return (len <= 255 ? HXR_OK : HXR_FAIL);
}

#if !defined(_CARBON) && !defined(_MAC_UNIX)
CHXString::operator Str255* (void)
{
#error "Not Implemented"
}

CHXString::operator const Str255* (void) const
{
#error "Not Implemented"
}

CHXString::operator ConstStr255Param (void) const
{
#error "Not Implemented"
}

#else

const CHXString& CHXString::operator =(const FSRef& ref)
{
    CHXString	temp;
    
    (void) PathFromFSRef(&ref, temp);

    *this = temp;
    return *this;
}

CHXString::operator const FSRef(void)
{
    FSRef ref;
    ZeroInit(&ref);
    (void) FSRefFromPath((const char*)(*this), &ref);
    
    return ref;
}

const CHXString& CHXString::operator =(CFStringRef ref)
{
#ifdef _MAC_CFM
	CFStringEncoding encoding = CFStringGetSystemEncoding();
#else
	CFStringEncoding encoding = kCFStringEncodingUTF8;
#endif

	// we need the string to be canonically decomposed Unicode in case it'll be used as a path on
	// an HFS disk, so we'll make a mutable copy of the string, normalize it, and then encode that as UTF-8
	
	const CFIndex kNoMaxLength = 0;
	
	CFMutableStringRef mutableRef = CFStringCreateMutableCopy(kCFAllocatorDefault, kNoMaxLength, ref);

#ifndef __MWERKS__
	// our version of CodeWarrior doesn't have CFStringNormalize in the headers since they are pre-10.2 headers, alas
	CFStringNormalize(mutableRef, kCFStringNormalizationFormD);
#endif
	
    (void) SetFromCFString(mutableRef, encoding);

	CFRelease(mutableRef);
	
    return *this;
}

HX_RESULT CHXString::SetFromCFString(CFStringRef ref, CFStringEncoding encoding)
{
    CHXString strTemp;
    char * pBuffer;
    HXBOOL bSuccess;
    CFIndex buffSize;

    bSuccess = FALSE;

    //const CFStringEncoding kEncoding = kCFStringEncodingUTF8;

    buffSize = 1 + CFStringGetMaximumSizeForEncoding(CFStringGetLength(ref), encoding);
    pBuffer = strTemp.GetBuffer(buffSize);
    if (pBuffer)
    {
	bSuccess = CFStringGetCString(ref, pBuffer, buffSize, encoding);
	strTemp.ReleaseBuffer();
    }

    *this = strTemp;
    return bSuccess ? HXR_OK : HXR_FAIL;	
}

HX_RESULT CHXString::SetFromHFSUniStr255(const HFSUniStr255& uniName, 
					 CFStringEncoding encoding)
{
    CHXCFString cfs(uniName);

    if (cfs.IsSet())
    {
	SetFromCFString(cfs, encoding);
	return HXR_OK;
    }
    return HXR_FAIL;
}

HX_RESULT CHXString::MakeHFSUniStr255(HFSUniStr255& outUniName, CFStringEncoding encoding) const
{
    if (GetLength() < 256)
    {
	CHXCFString cfs((const char*)(*this), encoding);

	if (cfs.IsSet())
	{
	    outUniName = (HFSUniStr255) cfs;
	    return HXR_OK;
	}
    }
    return HXR_FAIL;
}

#endif /* _CARBON || _MAC_UNIX */

#endif /* _MACINTOSH || _MAC_UNIX */

void CHXString::Init(const char* pStr, UINT32 size)
{
    if (size == UINT_MAX)
	size = (UINT32)SafeStrlen(pStr);

    if (m_pRep)
    {
	if ((UINT32)m_pRep->GetBufferSize() < (size + 1))
	    m_pRep->Resize(size);
	
	strncpy(m_pRep->GetBuffer(), pStr, size); /* Flawfinder: ignore */
	m_pRep->GetBuffer()[size] = '\0';
	m_pRep->SetStringSize(SafeStrlen(m_pRep->GetBuffer()));
    }
    else
	m_pRep = new CHXStringRep(pStr, size);
}

void CHXString::Nuke()
{
    if (m_pRep)
    {
	m_pRep->Release();
	m_pRep = 0;
    }
}

void CHXString::ConcatInPlace(const char* pStr, const UINT32 size)
{
    Append(pStr, (INT32)size);
}

void CHXString::EnsureUnique()
{
    if (m_pRep && m_pRep->IsShared())
    {
	// m_pRep is being shared. Time to copy it so that we
	// have our own copy.
	CHXStringRep* pOld = m_pRep;
	m_pRep = new CHXStringRep(pOld->GetBuffer(), 
				  pOld->GetStringSize());
	pOld->Release();
    }
}

void CHXString::Release()
{
}

INT32 CHXString::MinimalGrowth(INT32 currentSize, INT32 sizeNeeded)
{
    return sizeNeeded;
}

INT32 CHXString::DoublingGrowth(INT32 currentSize, INT32 sizeNeeded)
{
    INT32 ret = currentSize;

    while (ret < sizeNeeded)
	ret *= 2;

    return ret;
}

void CHXString::Append(const char* pStr, INT32 size)
{
    HX_ASSERT(size >= 0);

    if (size)
    {
	if (m_pRep)
	{
	    EnsureUnique();
	    
	    int newSize = m_pRep->GetStringSize() + size;

	    Grow(newSize + 1);

	    strncpy(m_pRep->GetBuffer() + m_pRep->GetStringSize(), pStr, size); /* Flawfinder: ignore */
	
	    m_pRep->GetBuffer()[newSize] = '\0';
	    m_pRep->SetStringSize(newSize);
	}
	else
	    m_pRep = new CHXStringRep(pStr, size);

    }
}

void CHXString::Grow(INT32 newSize)
{
    HX_ASSERT(m_pRep);
    HX_ASSERT(newSize >= 0);

    if (newSize > m_pRep->GetBufferSize())
    {
	INT32 growSize = m_pGrowthFunc(m_pRep->GetBufferSize(), newSize);
	
	// Protect ourselves from bad grow functions
	if (growSize < newSize)
	    growSize = newSize;
    
	m_pRep->ResizeAndCopy(growSize - 1);
    }
}

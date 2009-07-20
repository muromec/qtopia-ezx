/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsbuffer.h,v 1.7 2007/07/06 20:35:02 jfinnecy Exp $
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

#ifndef _HXSBUFFER_H_
#define _HXSBUFFER_H_

#include "ihxpckts.h"
#include "hxvalue.h"
//#include "hxheap.h"
#include "hxstring.h"
#include "hxcomm.h"    // IHXFastAlloc
#include "baseobj.h"

HX_RESULT CreateStaticBuffer(IHXBuffer* pBuffer, UINT32 ulOffset,
                                UINT32 ulLen, REF(IHXBuffer*) rpBuffer);

/****************************************************************************
 * 
 *        Class:
 *
 *                CHXStaticBuffer
 *
 *        Purpose:
 *
 *                PN implementation of a static buffer.
 *
 */
class CHXBaseCountingObject;
class CHXStaticBuffer : public IHXBuffer
{
public:
    CHXStaticBuffer(void);
    CHXStaticBuffer(UCHAR* pData, UINT32 ulLength);
    CHXStaticBuffer(IHXBuffer* pbuf, UINT32 pos, UINT32 len);

    inline CHXStaticBuffer& operator=(const char* psz);
    inline CHXStaticBuffer& operator=(const unsigned char* psz);

    // IUnknown
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXBuffer
    STDMETHOD(Get)                  (THIS_ REF(UCHAR*) pData, REF(ULONG32) ulLength);
    STDMETHOD(Set)                  (THIS_ const UCHAR* pData, ULONG32 ulLength);
    STDMETHOD(SetSize)              (THIS_ ULONG32 ulLength);
    STDMETHOD_(ULONG32,GetSize)     (THIS);
    STDMETHOD_(UCHAR*,GetBuffer)    (THIS);

protected:
    ULONG32         m_ulRefCount;
    IHXBuffer*     m_pbufRef;
    const UCHAR*    m_pData;
    ULONG32         m_ulLength;

private:
    virtual ~CHXStaticBuffer(void);
};

CHXStaticBuffer& CHXStaticBuffer::operator=(const char* psz)
{
    Set((const UCHAR*)psz, strlen(psz)+1);
    return *this;
}

CHXStaticBuffer& CHXStaticBuffer::operator=(const unsigned char* psz)
{
    Set(psz, strlen((const char*)psz) + 1);
    return *this;
}

#ifdef HELIX_FEATURE_USE_NULL_STATIC_BUFFER
class CHXStaticBufferNULL : public CHXBaseCountingObject, public IHXBuffer
{
public:
    CHXStaticBufferNULL(void);
    CHXStaticBufferNULL(UCHAR* pData, UINT32 ulLength);
    CHXStaticBufferNULL(IHXBuffer* pbuf, UINT32 pos, UINT32 len);

    inline CHXStaticBufferNULL& operator=(const char* psz);
    inline CHXStaticBufferNULL& operator=(const unsigned char* psz);

    // IUnknown
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXBuffer
    STDMETHOD(Get)                  (THIS_ REF(UCHAR*) pData, REF(ULONG32) ulLength);
    STDMETHOD(Set)                  (THIS_ const UCHAR* pData, ULONG32 ulLength);
    STDMETHOD(SetSize)              (THIS_ ULONG32 ulLength);
    STDMETHOD_(ULONG32,GetSize)     (THIS);
    STDMETHOD_(UCHAR*,GetBuffer)    (THIS);

#ifdef HELIX_FEATURE_SERVER
    FAST_CACHE_MEM
#endif

protected:
    ULONG32         m_ulRefCount;
    IHXBuffer*     m_pbufRef;
    const UCHAR*    m_pData;
    ULONG32         m_ulLength;

private:
    virtual ~CHXStaticBufferNULL(void);
};

CHXStaticBufferNULL& CHXStaticBufferNULL::operator=(const char* psz)
{
    Set((const UCHAR*)psz, strlen(psz)+1);
    return *this;
}

CHXStaticBufferNULL& CHXStaticBufferNULL::operator=(const unsigned char* psz)
{
    Set(psz, strlen((const char*)psz) + 1);
    return *this;
}
#endif  //HELIX_FEATURE_USE_NULL_STATIC_BUFFER



#endif

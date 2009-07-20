/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxbuffer.h,v 1.16 2007/07/06 20:35:02 jfinnecy Exp $
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

#ifndef _HXBUFFER_H_
#define _HXBUFFER_H_

#include "ihxpckts.h"
#include "hxvalue.h"
//#include "hxheap.h"
#include "hxstring.h"

typedef struct 
{
    UCHAR*		m_pData;
    ULONG32		m_ulLength;
    unsigned char	m_FreeWithMallocInterfaceIfAvail;
} _BigData;

// This determines the length of the built in buffer that is used if the
// data length is small enough, to save us from allocating so many little
// pieces of data.
#define PnBufferShort
#ifdef PnBufferShort
// XXXNH: This value was originally 15 and was chosen after some research as
// an optimal size for bulk of the small strings we deal with in our buffers.
// However, since the size of the structure is larger than 15 bytes on
// 64-bit systems we are now using this compile-time size calculation to
// ensure that the structure is of sufficient size.
const UINT32 MaxPnbufShortDataLen = HX_MAX(sizeof(_BigData), 15);
#endif

#define NUM_ALLOCATION_EACH_TIME	25
/****************************************************************************
 * 
 *	Class:
 *
 *		CHXBuffer
 *
 *	Purpose:
 *
 *		PN implementation of a basic buffer.
 *
 */
class CHXBuffer : public IHXBuffer
		, public IHXBuffer2
{
protected:

    LONG32  m_lRefCount;
    ULONG32 m_ulAllocLength;
    HXBOOL  m_bJustPointToExistingData;

#if !defined(HELIX_CONFIG_NOSTATICS)
    // Interface for optional allocator
    static IMalloc* m_zMallocInterface;
#endif

    // number of CHXBuffer allocated at a time to be placed in freeStore
    static 	CHXBuffer*		s_pFreeStore;
    static 	const int		s_iBufferChunk;	

    virtual ~CHXBuffer();

    HXBOOL FreeWithMallocInterface() const;

#ifdef PnBufferShort
    // buffer for small amounts of data
    //UCHAR m_ShortData[MaxPnbufShortDataLen + 1];
#endif

    enum { BigDataTag = 0xEE };

    union
    {
	_BigData m_BigData;

	UCHAR m_ShortData[MaxPnbufShortDataLen + 1];
    };

    HXBOOL IsShort() const;
    HX_RESULT SetSize(ULONG32 ulLength, HXBOOL copyExistingData);

    UCHAR* Allocate(UINT32 size) const;
    UCHAR* Reallocate(UCHAR*, UINT32 oldSize, UINT32 newSize) const;
    void Deallocate(UCHAR*) const;

    
public:
    CHXBuffer();
    CHXBuffer(UCHAR* pData, UINT32 ulLength, HXBOOL bOwnBuffer = TRUE);

    inline CHXBuffer& operator=(const char* psz);
    inline CHXBuffer& operator=(const unsigned char* psz);
    inline CHXBuffer& operator=(const CHXString &str);

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
                                REFIID riid,
                                void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	IHXBuffer methods
     */
    STDMETHOD(Get)		(THIS_
                                REF(UCHAR*)	pData, 
				REF(ULONG32)	ulLength);

    STDMETHOD(Set)		(THIS_
    				const UCHAR*	pData, 
                                ULONG32		ulLength);

    STDMETHOD(SetSize)		(THIS_
                                ULONG32		ulLength);

    STDMETHOD_(ULONG32,GetSize)	(THIS);

    STDMETHOD_(UCHAR*,GetBuffer)(THIS);

    /*
     *	IHXBuffer2 methods
     */
    STDMETHOD(SetWithoutAlloc) (THIS_
				UCHAR*		pData, 
				ULONG32		ulLength);

public:
    static HX_RESULT FromCharArray
    (
        const char* szIn, 
        IHXBuffer** ppbufOut
    );
    static HX_RESULT FromCharArray
    (
        const char* szIn, 
        UINT32 ulLength, 
        IHXBuffer** ppbufOut
    );
    static void SetAllocator(IMalloc* pMalloc);
    static void ReleaseAllocator();
};

CHXBuffer& CHXBuffer::operator=(const char* psz)
{
        Set((const unsigned char*)psz, strlen(psz)+1);
        return(*this);
}

CHXBuffer& CHXBuffer::operator=(const unsigned char* psz)
{
        Set(psz, strlen((const char*)psz)+1);
        return(*this);
}

CHXBuffer& CHXBuffer::operator=(const CHXString& str)
{
        Set((const unsigned char*)(const char *)str, str.GetLength()+1);
        return(*this);
}


// This class was created in order to be able to have a buffer that consists of
// a subset of another existing buffer without allocating any new data or 
// copying data over. The way to use this class is to instantiate it with 3 
// parameters: 
// 1) A pointer to the superset buffer, 
// 2) The pointer to the point in the the buffer that represents the start of 
//    the subset buffer, and 
// 3) The length of the subset buffer.
//
class CHXBufferFragment : public CHXBuffer
{
  public:
    CHXBufferFragment( IHXBuffer * pWrappedBuffer,
                       UCHAR* pModFrameStart,
                       ULONG32 ulFragLen)
        : CHXBuffer( pModFrameStart, ulFragLen, FALSE ),
        m_pHXBufferPointedTo(pWrappedBuffer)
    {
        if(pWrappedBuffer)
        {
            pWrappedBuffer->AddRef();
        }
    };
    ~CHXBufferFragment()
    {
        HX_RELEASE(m_pHXBufferPointedTo);
    }
    
  protected:
    IHXBuffer * m_pHXBufferPointedTo;
    
  private:
    CHXBufferFragment();
    CHXBufferFragment(const CHXBufferFragment& it);
    const CHXBufferFragment& operator=(const CHXBufferFragment& rhs);
    
};

#endif

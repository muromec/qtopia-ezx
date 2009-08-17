/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxfgbuf.h,v 1.4 2006/02/07 19:21:09 ping Exp $
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

#ifndef _HXFGBUF_H_
#define _HXFGBUF_H_

/*
 * IHXFragmentedBuffer
 *     STDMETHOD(GetEnumerator)(REF(IEnumHXFragmentedBuffer *));
 *     STDMETHOD(Prepend)(IHXBuffer *pBufferFrom, UINT32 ulStartFrom, UINT32 ulLengthFrom);
 *     STDMETHOD(Append)(IHXBuffer *pBufferFrom, UINT32 ulStartFrom, UINT32 ulLengthFrom);
 *     STDMETHOD(Insert)(IHXBuffer *pBufferFrom, UINT32 ulStartFrom, UINT32 ulLengthFrom, UINT32 ulStartTo);
 *     STDMETHOD(Replace)(IHXBuffer *pBufferFrom, UINT32 ulStartFrom, UINT32 ulLengthFrom, UINT32 ulStartTo);
 *     STDMETHOD(Get)(UINT32 ulStartFrom, UINT32 ulLengthFrom, REF(UCHAR*) pData, REF(UINT32) ulLength);
 *     STDMETHOD_(UCHAR*, GetBuffer)(UINT32 ulStartFrom, UINT32 ulLengthFrom);
 *
 * IEnumHXFragmentedBuffer
 *     STDMETHOD(Reset)();
 *     STDMETHOD(Next)(UINT32 ulRequested, IHXBuffer *[]arrpbufReturned, REF(UINT32) ulReturned);
 *     STDMETHOD(Skip)(UINT32 ulNumToSkip);
 *     STDMETHOD(Clone)(REF(IEnumHXFragmentedBuffer *)ppefbufClone);
 */

#include "hxtypes.h"
#include "unkimp.h"
#include "ihxfgbuf.h"
#include "hxbuffer.h"
#include "arrenum.h"

DECLARE_INTERFACE_ARRAY_ENUMERATOR(IHXBuffer, IHXEnumFragmentedBuffer);


    /*
     * This class is to allow fragment(s) to be
     * inserted into the middle of existing buffers
     * without re-copying the bytes.
     *
     *	Before:
     *	    ...
     *	    IHXBuffer[100]
     *	    ...
     *
     *	After:
     *	    ...
     *	    (_CBufferFragment(0,20))IHXBuffer[20]----\
     *	    (New)IHXBuffer[43]			       -->IHXBuffer[100]
     *	    (_CBufferFragment(21,80)IHXBuffer[80]----/
     *	    ...
     *
     * XXX Optimize by overriding the new operator.
     *
     * The local implementation should maintain a 
     * static array of BufferFragment arrays 
     * (BufferFragment[n][32]), and return the next 
     * free one.
     * When all BFs are in use, the first dimension
     * should be re-alloced to twice it's size
     * (BufferFragment*[n*2]). The existing pointers 
     * are then copied into it.  A new block
     * of memory to cover the second dimension of 
     * each of the new locations in the first dimension
     * is alloced, (BufferFragment[n*32])
     * then its contents are distrubuted to the first 
     * array.
     * 
     * This has several benefits: 
     *	- not copying the data around when growing.
     *	- Balancing allocs with wasted space.
     *	- Not allocing for every new object.
     *	- Fixed second dimension keeps management simple.
     */
class _CBufferFragment
    : public IHXBuffer
    , public CUnknownIMP
{
    DECLARE_UNKNOWN(_CBufferFragment)
public:
    _CBufferFragment()
	: m_pData(NULL)
	, m_ulStart(0)
	, m_ulLength(0)
	, m_pContext(NULL)
    {}
    _CBufferFragment(IUnknown* pContext)
	: m_pData(NULL)
	, m_ulStart(0)
	, m_ulLength(0)
	, m_pContext(NULL)
    {
	m_pContext = pContext;
	HX_ADDREF(m_pContext);
    }

    ~_CBufferFragment()
    {HX_RELEASE(m_pData); HX_RELEASE(m_pContext); m_ulStart=0; m_ulLength=0;}

    _CBufferFragment* _SetBuffer(IHXBuffer* pData, UINT32 ulStart, UINT32 ulLength);
    STDMETHOD(Set)(const UCHAR* pData, UINT32 ulLength);
    STDMETHOD(Get)(REF(UCHAR*) pData, REF(UINT32) ulLength);
    STDMETHOD(SetSize)(UINT32 ulLength);
    STDMETHOD_(UINT32, GetSize)();
    STDMETHOD_(UCHAR*, GetBuffer)();

private:
    IHXBuffer* m_pData;
    IUnknown* m_pContext;
    UINT32 m_ulStart, m_ulLength;
};

class CHXFragmentedBuffer
    : public IHXFragmentedBuffer
    , public IHXBuffer
    , public CUnknownIMP
{

    DECLARE_UNKNOWN(CHXFragmentedBuffer)

private:

    class _CFragment;
    class _CFragmentList;
    friend class _CFragment;
    friend class _CFragmentList;

    /* This class implements the listnodes of fragments
     */
    class _CFragment
    {
    public:
	inline _CFragment* Prev(){return m_pPrev;}
	inline _CFragment* Next(){return m_pNext;}
	
	_CFragment* SetData(IHXBuffer* pData);
	_CFragment* SetData(IHXBuffer* pData, UINT32 ulStartFrom, UINT32 ulLengthFrom);

	inline IHXBuffer* GetData(){return m_pData;}
	
	_CFragment* Insert(_CFragment* pNewPrev);
	_CFragment* Append(_CFragment* pNewNext);
	_CFragment* Remove();
	
	_CFragment(IUnknown* pContext):m_pData(NULL),m_pPrev(NULL),m_pNext(NULL),m_pContext(NULL)
	{
	    m_pContext = pContext;
	    HX_ADDREF(m_pContext);
	}
	~_CFragment()
	{
	    HX_RELEASE(m_pData); 
	    if(m_pNext)
	    {
		m_pNext->_SetPrev(m_pPrev);
	    }
	    if(m_pPrev)
	    {
		m_pPrev->_SetNext(m_pNext);
	    }
	    HX_RELEASE(m_pContext);
	}
    private:
	inline void _SetPrev(_CFragment* pfrgNew){m_pPrev = pfrgNew;}
	inline void _SetNext(_CFragment* pfrgNew){m_pNext = pfrgNew;}

	IUnknown* m_pContext;
	IHXBuffer* m_pData;
	_CFragment* m_pPrev;
	_CFragment* m_pNext;
    };

    /* This class maintains the First and Last pointers
     * for a list of fragments
     */
    class _CFragmentList
    {
    public:
	_CFragmentList()
	    : m_pfrgListStart(NULL)
	    , m_pfrgListEnd(NULL)
	    , m_ulTotal(0)
	{}
	~_CFragmentList()
	{
	    m_pfrgListEnd = NULL; 
	    while(m_pfrgListStart)
	    {
		m_pfrgListStart=m_pfrgListStart->Remove();
	    }
	}

	inline _CFragment* First(){return m_pfrgListStart;}
	inline _CFragment* Last(){return m_pfrgListEnd;}

	void Remove(_CFragment* pfrgObsolete);
	void Insert(_CFragment* pfrgNew, _CFragment* pfrgRelative = NULL);
	void Append(_CFragment* pfrgNew, _CFragment* pfrgRelative = NULL);

	inline UINT32 GetTotal(){return m_ulTotal;}

    private:
	_CFragment* m_pfrgListStart;
	_CFragment* m_pfrgListEnd;
	UINT32      m_ulTotal;
    };

public:
    CHXFragmentedBuffer(){};
    CHXFragmentedBuffer(IUnknown* pContext){m_pContext = pContext; HX_ADDREF(m_pContext);};
    ~CHXFragmentedBuffer(){HX_RELEASE(m_pContext);};

    /* IHXFragmentedBuffer Methods
     */
    STDMETHOD(GetEnumerator)
    (
	THIS_ 
	IHXEnumFragmentedBuffer** ppefbNewEnum
    );

    STDMETHOD(Prepend)
    (
	THIS_ 
	IHXBuffer* pBufferFrom, 
	ULONG32 ulStartFrom, 
	ULONG32 ulLengthFrom
    );
    STDMETHOD(Append)
    (
	THIS_ 
	IHXBuffer* pBufferFrom, 
	ULONG32 ulStartFrom, 
	ULONG32 ulLengthFrom
    );
    STDMETHOD(Insert)
    (
	THIS_ 
	IHXBuffer* pBufferFrom, 
	ULONG32 ulStartFrom, 
	ULONG32 ulLengthFrom, 
	ULONG32 ulStartTo
    );
    STDMETHOD(Replace)
    (
	THIS_ 
	IHXBuffer* pBufferFrom, 
	ULONG32 ulStartFrom, 
	ULONG32 ulLengthFrom, 
	ULONG32 ulStartTo
    );

    STDMETHOD(Get)
    (
	THIS_ 
	ULONG32 ulStartFrom, 
	ULONG32 ulLengthFrom, 
	REF(UCHAR*) pData, 
	REF(ULONG32) ulLength
    );
    STDMETHOD_(UCHAR*, GetBuffer)
    (
	THIS_ 
	ULONG32 ulStartFrom, 
	ULONG32 ulLengthFrom
    );


    /* IHXBuffer Methods
     */ 
    STDMETHOD(Set)(const UCHAR* pData, ULONG32 ulLength);
    STDMETHOD(Get)(REF(UCHAR*) pData, REF(ULONG32) ulLength);
    STDMETHOD(SetSize)(ULONG32 ulLength);
    STDMETHOD_(ULONG32, GetSize)();
    STDMETHOD_(UCHAR*, GetBuffer)();

private:
    STDMETHOD(_FindFragment)(UINT32 ulFindIndex, REF(_CFragment*) pfrgCurrent, REF(UINT32) ulCurrentSize, REF(UINT32) ulCurrentStart);
    void _RecursiveBufferCopy
    (
	UCHAR* pucDestBuffer,
	IHXBuffer* pbufSource,
	UINT32 ulStartIndex,
	UINT32 ulSize
    );

    _CFragmentList m_frglstThis;
    IUnknown* m_pContext;
};

#endif //!_HXFGBUF_H_



/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#ifndef _QTBATOM_H
#define _QTBATOM_H

/****************************************************************************
 *  Defines
 */
#define QT_HEADER_SIZE	    8
#define QT_NEW_HEADER_SIZE  20
#define QT_EXTENDED_SIZE    8
#define QT_NEW_HEADER_GAP   4

#define QT_ENCODE_TYPE(b1, b2, b3, b4)	((b1 << 24) |	\
					 (b2 << 16) |	\
					 (b3 << 8)  |	\
					 b4)

/****************************************************************************
 *  Includes
 */
#include "hxcom.h"
#include "hxtypes.h"
#include "hxslist.h"
#include "ihxpckts.h"


class CMemPager;


/****************************************************************************
 *  Atom Type definition
 */
typedef LONG32	QTAtomType;


/****************************************************************************
 *  Abstract Atom Class
 */
class CQTAtom : public IUnknown
{
public:
    /*
     *	IUnknown methods
     */
    STDMETHOD (QueryInterface )	    (THIS_ 
				    REFIID ID, 
				    void** ppInterfaceObj);

    STDMETHOD_(UINT32, AddRef )	    (THIS);

    STDMETHOD_(UINT32, Release)	    (THIS);
    
    /*
     *	Utilities
     */
    static UINT64   GetUL64	(UINT8 *pData)
    {
	return	(((UINT64) *pData)	    << 56) |
		(((UINT64) *(pData + 1))    << 48) |
		(((UINT64) *(pData + 2))    << 40) |
		(((UINT64) *(pData + 3))    << 32) |
		(*(pData + 4)		    << 24) |
		(*(pData + 5)		    << 16) |
		(*(pData + 6)		    << 8 ) |
		*(pData + 7);
    }
    static ULONG32  GetUL32	(UINT8 *pData)
    {
	return	(*pData		<< 24) |
		(*(pData + 1)	<< 16) |
		(*(pData + 2)	<< 8 ) |
		*(pData + 3);
    }
    static LONG32   GetL32	(UINT8 *pData)
    {
	return (LONG32) GetUL32(pData);
    }
    static UINT16   GetUI16	(UINT8 *pData)
    {
	return	(*pData	    << 8) |
		*(pData + 1);
    }
    static INT16    GetI16	(UINT8 *pData)
    {
	return (INT16) GetUI16(pData);
    }
    static double    GetFixed32	(UINT8 *pData)
    {
	return	((*pData	<< 8) |
		 *(pData + 1)) +
		(((*(pData + 2)	<< 8) |
		  *(pData + 3)) / 65536.0);
    }
    static double    GetFixed16	(UINT8 *pData)
    {
	return	*pData +
		(*(pData + 1) / 256.0);
    }
    static ULONG32  GetFlags	(UINT8 *pData)
    {
	return	(*pData		<< 16) |
		(*(pData + 1)	<< 8 ) |
		*(pData + 2);
    }
    
    /*
     *	Main Interface
     */
    virtual HXBOOL	IsLeafType(void) = 0;
    virtual QTAtomType	GetType(void) = 0;
    virtual HXBOOL	IsNewQTAtom(void)	{ return FALSE; }
    virtual HXBOOL	IsPagingAtom(void)	{ return FALSE; }
    virtual HXBOOL	IsPagingEnabled(void)	{ return FALSE; }
    virtual void	SetMemPager(CMemPager* pMemPager)   { ; }
    virtual HXBOOL	IsFaulted(void)		{ return FALSE; }

    UINT16  GetPresentChildCount(void)
    {
	return m_pChildList ? m_pChildList->GetCount() : 0;
    }
    HX_RESULT	AddPresentChild(CQTAtom *pChild);
    CQTAtom*	GetPresentChild(UINT16 uChildIndex);
    CQTAtom*	FindPresentChild(QTAtomType AtomType);
    CQTAtom*	GetParent(void) { return m_pParent; }

    ULONG32	GetOffset(void)	{ return m_ulOffset; }
    ULONG32	GetSize(void)	{ return m_ulSize; }
    IHXBuffer*	GetBuffer(void)	{ return m_pBuffer; }
    UINT8*	GetData(void)	{ return m_pData; }
    ULONG32	GetDataSize(void) { return m_pBuffer ? m_pBuffer->GetSize() : 0; }

    void	SetOffset(ULONG32 ulOffset) { m_ulOffset = ulOffset; }   
    void	SetSize(ULONG32 ulSize)	{ m_ulSize = ulSize; }
    virtual void SetBuffer(IHXBuffer *pBuffer);

    /*
     *	Child Iterator
     */
    class ChildIterator
    {
    public:
	friend class CQTAtom;

	ChildIterator(void)
	    : m_pChildList(NULL)
	    , m_NextChildPos(NULL)
	    , m_pChild(NULL) {;}

	ChildIterator& operator ++(void)
	{
	    HX_ASSERT(m_NextChildPos);
	    m_pChild = (CQTAtom*) m_pChildList->GetNext(m_NextChildPos);
	    return *this;
	}

	CQTAtom* operator *(void) { return m_pChild; }

    private:
	ChildIterator(CHXSimpleList* pSimpleList, LISTPOSITION ChildPos)
	    : m_pChildList(pSimpleList)
	    , m_NextChildPos(ChildPos)
	{
	    m_pChild = (CQTAtom*) m_pChildList->GetNext(m_NextChildPos);
	}

	CHXSimpleList *m_pChildList;
	LISTPOSITION m_NextChildPos;
	CQTAtom* m_pChild;
    };

    ChildIterator BeginChildren(void)
    {
	HX_ASSERT(m_pChildList);
	HX_ASSERT(m_pChildList->GetHeadPosition());
	return ChildIterator(m_pChildList, m_pChildList->GetHeadPosition());
    }

protected:
    UINT8* FindArrayEntry(UINT8* pStartEntry, ULONG32 ulArrayIdx)
    {
	while (ulArrayIdx != 0)
	{
	    HX_ASSERT(((ULONG32) (pStartEntry - m_pData))
		      < m_pBuffer->GetSize());
	    pStartEntry += GetUL32(pStartEntry);
	    ulArrayIdx--;
	}

	return pStartEntry;
    }

    CQTAtom(void);

    CQTAtom(ULONG32 ulOffset,
	    ULONG32 ulSize, 
	    CQTAtom *pParent);

    virtual ~CQTAtom();

    CQTAtom*	    m_pParent;
    CHXSimpleList*  m_pChildList;
    ULONG32	    m_ulOffset;

    ULONG32	    m_ulSize;
    IHXBuffer*	    m_pBuffer;
    UINT8*	    m_pData;

private:
    LONG32	    m_lRefCount;
};


/****************************************************************************
 *  Abstract Atom Class for new QT Atoms
 */
class CQTNewAtom : public CQTAtom
{
public:
    virtual HXBOOL	IsLeafType(void)    { return (m_uChildCount == 0); }
    virtual QTAtomType	GetType(void) = 0;
    virtual HXBOOL	IsNewQTAtom(void)   { return TRUE; }

    UINT16  GetChildCount(void)	{ return m_uChildCount; }

protected:
    CQTNewAtom(	ULONG32 ulOffset,
		ULONG32 ulSize, 
		CQTAtom *pParent, 
		ULONG32 AtomID, 
		UINT16 uChildCount)
		: CQTAtom(ulOffset, ulSize, pParent)
		, m_AtomID(AtomID)
		, m_uChildCount(uChildCount) {;}

    ULONG32 m_AtomID;
    UINT16  m_uChildCount;
};

class CQTPagingAtom : public CQTAtom
{
public:
    virtual HXBOOL	IsLeafType(void) = 0;
    virtual QTAtomType	GetType(void) = 0;
    virtual HXBOOL	IsPagingAtom(void)	{ return TRUE; }
    virtual HXBOOL	IsPagingEnabled(void)	{ return m_pMemPager ? TRUE : FALSE; }
    virtual void	SetMemPager(CMemPager* pMemPager);
    virtual HXBOOL	IsFaulted(void);
    virtual void	SetBuffer(IHXBuffer *pBuffer);

    HX_RESULT GetLastStatus(void)   { return m_lastStatus; }

protected:
    CQTPagingAtom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent)
		  : CQTAtom(ulOffset, ulSize, pParent)
		  , m_pMemPager(NULL)
		  , m_lastStatus(HXR_OK) {;}

    virtual ~CQTPagingAtom();

    CMemPager* m_pMemPager;
    HX_RESULT m_lastStatus;
};

#endif  // QTBATOM_H


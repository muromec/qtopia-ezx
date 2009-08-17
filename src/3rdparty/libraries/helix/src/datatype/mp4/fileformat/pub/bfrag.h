/****************************************************************************
 * 
 *  $Id: bfrag.h,v 1.4 2004/05/03 19:00:52 tmarshall Exp $
 *
 *  Copyright (C) 1995-1999 RealNetworks, Inc. All rights reserved.
 *  
 *  http://www.real.com/devzone
 *
 *  This program contains proprietary 
 *  information of Progressive Networks, Inc, and is licensed
 *  subject to restrictions on use and distribution.
 *
 *
 *  Buffer Fragment Class
 *
 */

#ifndef _BFRAG_H_
#define _BFRAG_H_

/****************************************************************************
 *  Defines
 */

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "qtffrefcounter.h"


class CBufferFragmentFactory;


/****************************************************************************
 * 
 *  Class:
 *	CBufferFragment
 *
 *  Purpose:
 *	Provides for containment of buffer fragments under IHXBuffer API
 *
 */
class CBufferFragment : public IHXBuffer
{
public:
    /*
     *	Constructor/Destructor
     */
#ifdef QTCONFIG_BFRAG_FACTORY
    CBufferFragment(void)
	: m_pBuffer(NULL)
	, m_pData(NULL)
	, m_ulSize(0)
	, m_lRefCount(0)
	, m_pBufferFragmentFactory(NULL)
    {
    }
#else	// QTCONFIG_BFRAG_FACTORY
    CBufferFragment(IHXBuffer* pBuffer,
		    ULONG32 ulOffset,
		    ULONG32 ulSize)
	: m_pBuffer(pBuffer)
	, m_ulSize(ulSize)
	, m_lRefCount(0)
    {

	pBuffer->AddRef();
	m_pData = pBuffer->GetBuffer() + ulOffset;
	m_ulSize = ulSize;
    }
#endif	// QTCONFIG_BFRAG_FACTORY

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	CBufferFragment methods
     */
#ifdef QTCONFIG_BFRAG_FACTORY
    void Link	(CBufferFragmentFactory* pBufferFragmentFactory);

    inline void Unlink(void);

    void Init	(IHXBuffer* pBuffer,
		 ULONG32 ulOffset,
		 ULONG32 ulSize, 
		 CBufferFragmentFactory* pBufferFragmentFactory)
    {
	if (m_pBuffer)
	{
	    m_pBuffer->Release();
	}
	Link(pBufferFragmentFactory);
	m_pBuffer = pBuffer;
	pBuffer->AddRef();
	m_pData = pBuffer->GetBuffer() + ulOffset;
	m_ulSize = ulSize;
    }
#endif	// QTCONFIG_BFRAG_FACTORY

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


private:
    IHXBuffer* m_pBuffer;
    UINT8* m_pData;
    UINT32 m_ulSize;
#ifdef QTCONFIG_BFRAG_FACTORY
    CBufferFragmentFactory* m_pBufferFragmentFactory;
#endif	// QTCONFIG_BFRAG_FACTORY

    LONG32 m_lRefCount;

    ~CBufferFragment();

};

#endif  // _BFRAG_H_

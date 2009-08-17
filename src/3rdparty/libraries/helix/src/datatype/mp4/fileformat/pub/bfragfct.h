/****************************************************************************
 * 
 *  $Id: bfragfct.h,v 1.5 2005/03/14 19:17:44 bobclark Exp $
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
 *  Buffer Fragment Factory class
 *
 */

#ifndef _BFRAGFCT_H_
#define _BFRAGFCT_H_

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

#include "cringbuf.h"
#include "bfrag.h"

#include "qtffrefcounter.h"


/****************************************************************************
 * 
 *  Class:
 *	CQTBufferFragmentFactory
 *
 *  Purpose:
 *	Enables paging of file sections into memory
 *
 */
class CBufferFragmentFactory
{
public:
    /*
     *	Constructor/Destructor
     */
    CBufferFragmentFactory(ULONG32 ulMaxSize, ULONG32 ulInitialOccupancy = 0);


    /*
     *	CBufferFragment methods
     */
    IHXBuffer* WrapFragment(IHXBuffer* pBuffer,
			     ULONG32 ulOffset,
			     ULONG32 ulSize)
    {
	CBufferFragment* pBufferFragment = NULL;

	pBufferFragment = (CBufferFragment*) m_pRingBuffer->Get();
	if (!pBufferFragment)
	{
	    pBufferFragment = new CBufferFragment();
	    if (pBufferFragment)
	    {
		pBufferFragment->AddRef();
	    }
	}
	if (pBufferFragment)
	{
	    pBufferFragment->Init(pBuffer,
				  ulOffset,
				  ulSize,
				  this);
	}

	return pBufferFragment;
    }

    HXBOOL Put(CBufferFragment* pBufferFragment)
    {
	if (m_pRingBuffer->Put(pBufferFragment))
	{
	    pBufferFragment->AddRef();
	    return TRUE;
	}

	return FALSE;
    }

    ULONG32 AddRef(void)
    {
	return InterlockedIncrement(&m_lRefCount);
    }

    ULONG32 Release(void)
    {
	if (InterlockedDecrement(&m_lRefCount) > 0)
	{
	    return m_lRefCount;
	}

	delete this;
	return 0;
    }

private:
    CRingBuffer* m_pRingBuffer;

    LONG32 m_lRefCount;

    ~CBufferFragmentFactory()
    {
	CBufferFragment* pDeadFragment;

	while (pDeadFragment = ((CBufferFragment*) m_pRingBuffer->Get()))
	{
	    pDeadFragment->Release();
	}
	delete m_pRingBuffer;
    }

};

#endif  // _BFRAGFCT_H_

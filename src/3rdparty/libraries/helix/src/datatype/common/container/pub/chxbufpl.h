/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxbufpl.h,v 1.4 2006/02/16 23:05:38 ping Exp $
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

#ifndef _HXBUFPL_H_
#define _HXBUFPL_H_

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxresult.h"
#include "hxassert.h"
#include "hxslist.h"
#include "pckunpck.h"

typedef void (*BufferKillerFunc) (void* pBuffer);
typedef ULONG32 (*BufferSizeFunc) (void* pBuffer);


/****************************************************************************
 *  CHXBufferPool
 */
class CHXBufferPool
{
public:
    CHXBufferPool(IUnknown* pContext,
		  BufferSizeFunc fpSize,
		  BufferKillerFunc fpKiller = NULL)
	: m_pMutex(NULL)
	, m_fpSize(fpSize)
	, m_fpKiller(fpKiller)
    {
	CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, pContext);

	HX_ASSERT(m_fpSize);
	HX_ASSERT(m_pMutex);
    }

    ~CHXBufferPool()
    {
	Flush();
	HX_RELEASE(m_pMutex);
    }


    void* Get(ULONG32 ulSize)
    {
	void* pBuffer;

	m_pMutex->Lock();

	while (!m_FreeList.IsEmpty())
	{
	    pBuffer = m_FreeList.RemoveHead();
	    if (m_fpSize(pBuffer) >= ulSize)
	    {
		m_pMutex->Unlock();
		
		return pBuffer;
	    }
	    
	    if (m_fpKiller)
	    {
		m_fpKiller(pBuffer);
	    }
	}

	m_pMutex->Unlock();
	
	return NULL;
    }

    void* GetBestMatch(ULONG32 ulSize)
    {
        void* pBuffer = NULL;

        m_pMutex->Lock();

        if (!m_FreeList.IsEmpty())
        {
            UINT32  ulBufSize = 0,
                    ulSmallest = 0xffffffff,
                    ulBest = 0xffffffff;
            
            LISTPOSITION pos = m_FreeList.GetHeadPosition(),
                         best = NULL,
                         smallest = NULL;

            for (int i=0; i<m_FreeList.GetCount(); i++)
            {
                pBuffer = m_FreeList.GetAt(pos);
                ulBufSize = m_fpSize(pBuffer);

                if (ulBufSize >= ulSize)
                {
                    if (ulBufSize - ulSize < ulBest)
                    {
                        ulBest = ulBufSize - ulSize;
                        best = pos;
                    }
                }

                if (ulBufSize < ulSmallest)
                {
                    ulSmallest = ulBufSize;
                    smallest = pos;
                }
                m_FreeList.GetNext(pos);
            } 

            // Return the buffer closest to the requested size
            if (best)
            {
                pBuffer = m_FreeList.GetAt(best);
                m_FreeList.RemoveAt(best);
            }
            // If we could not find a buffer to match the size,
            // destroy the smallest and return NULL.
            else
            {
                pBuffer = m_FreeList.GetAt(smallest);
                m_FreeList.RemoveAt(smallest);

                if (m_fpKiller)
                {
                    m_fpKiller(pBuffer);
                }

                pBuffer = NULL;
            }
        }

        m_pMutex->Unlock();

        return pBuffer;
    }
    
    void Put(void* pBuffer)
    {
	m_pMutex->Lock();
	m_FreeList.AddHead(pBuffer);
	m_pMutex->Unlock();

	return;
    }

    void Flush(void)
    {
	void* pBuffer;

	m_pMutex->Lock();

	while (!m_FreeList.IsEmpty())
	{
	    pBuffer = m_FreeList.RemoveHead();

	    if (m_fpKiller)
	    {
		m_fpKiller(pBuffer);
	    }
	}
	
	m_pMutex->Unlock();
    }

private:
    IHXMutex* m_pMutex;
    CHXSimpleList m_FreeList;

    BufferSizeFunc m_fpSize;
    BufferKillerFunc m_fpKiller;
};

#endif	// _HXBUFPL_H_

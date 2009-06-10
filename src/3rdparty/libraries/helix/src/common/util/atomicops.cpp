/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: atomicops.cpp,v 1.4 2007/07/06 20:39:16 jfinnecy Exp $
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

#if !defined (HXATOMIC_MUTEX_POOL_SIZE)
#define HXATOMIC_MUTEX_POOL_SIZE 256 // must  be a power of 2
#endif

#include "hxtypes.h"
#include "atomicbase.h"
#include "microsleep.h"
#include "hxcom.h"
#include "hxmutexlock.h"
#include "hxassert.h"

HXAtomic g_AtomicOps;

HXAtomic::HXAtomic()
    : m_pLocks(0)
{
}

HXAtomic::~HXAtomic()
{
    HX_DELETE(m_pLocks);
}

void
HXAtomic::Lock(HX_MUTEX pLock)
{
    while (_HXMutexSetBit((pLock)))
    {
        /*spin - this is fine since the locks are held so briefly */
    }
};

void
HXAtomic::Unlock(HX_MUTEX pLock)
{
    _HXMutexClearBit((pLock));
};

void
HXAtomic::InitLockPool()
{
    HX_ASSERT(!m_pLocks);
    HX_MUTEX* pLocks = new HX_MUTEX[HXATOMIC_MUTEX_POOL_SIZE];
    for (int i=0; i < HXATOMIC_MUTEX_POOL_SIZE; ++i)
    {
        pLocks[i] = HXMutexCreate();
    }
    m_pLocks = pLocks;
};

INT32
HXAtomic::_AddRetINT32(INT32* pNum, INT32 nNum)
{
    register INT32 nRet;

    if (m_pLocks)
    {
        register int nIndex = ((UINT32)(PTR_INT)pNum >> 4) & (HXATOMIC_MUTEX_POOL_SIZE - 1);
        register HX_MUTEX pLock = (HX_MUTEX) m_pLocks[nIndex];
        Lock(pLock);
        nRet = *pNum;
        nRet += nNum;
        *pNum = nRet;
        Unlock(pLock);
        return nRet;
    }

    // locks are not defined, so just do it -- (non-threadsafe!)
    nRet = *pNum;
    nRet += nNum;
    *pNum = nRet;
    return nRet;
};

UINT32
HXAtomic::_AddRetUINT32(UINT32* pNum, UINT32 ulNum)
{
    register UINT32 ulRet;

    if (m_pLocks)
    {
        register int nIndex = ((UINT32)(PTR_INT)pNum >> 4) & (HXATOMIC_MUTEX_POOL_SIZE - 1);
        register HX_MUTEX pLock = (HX_MUTEX) m_pLocks[nIndex];
        Lock(pLock);
        ulRet = *pNum;
        ulRet += ulNum;
        *pNum = ulRet;
        Unlock(pLock);
        return ulRet;
    }

    // locks are not defined, so just do it -- (non-threadsafe!)
    ulRet = *pNum;
    ulRet += ulNum;
    *pNum = ulRet;
    return ulRet;
};

INT32
HXAtomic::_SubRetINT32(INT32* pNum, INT32 nNum)
{
    register INT32 nRet;

    if (m_pLocks)
    {
        register int nIndex = ((UINT32)(PTR_INT)pNum >> 4) & (HXATOMIC_MUTEX_POOL_SIZE - 1);
        register HX_MUTEX pLock = (HX_MUTEX) m_pLocks[nIndex];
        Lock(pLock);
        nRet = *pNum;
        nRet -= nNum;
        *pNum = nRet;
        Unlock(pLock);
        return nRet;
    }

    // locks are not defined, so just do it -- (non-threadsafe!)
    nRet = *pNum;
    nRet -= nNum;
    *pNum = nRet;
    return nRet;
};

UINT32
HXAtomic::_SubRetUINT32(UINT32* pNum, UINT32 ulNum)
{
    register UINT32 ulRet;

    if (m_pLocks)
    {
        register int nIndex = ((UINT32)(PTR_INT)pNum >> 4) & (HXATOMIC_MUTEX_POOL_SIZE - 1);
        register HX_MUTEX pLock = (HX_MUTEX) m_pLocks[nIndex];
        Lock(pLock);
        ulRet = *pNum;
        ulRet -= ulNum;
        *pNum = ulRet;
        Unlock(pLock);
        return ulRet;
    }

    // locks are not defined, so just do it -- (non-threadsafe!)
    ulRet = *pNum;
    ulRet -= ulNum;
    *pNum = ulRet;
    return ulRet;
};


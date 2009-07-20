/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: mutexlist.cpp,v 1.3 2003/09/04 22:35:34 dcollins Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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
/*
 *  This implements a list of locks that will be unlocked in the
 *  case of a crash-avoid event in this process/thread.
 */

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxassert.h"

#include "mutex.h"
#include "mutexlist.h"


#define MUTEX_LIST_MAX 1024


/************************************************************************/

#include "proc.h"
#include "proc_container.h"
#include "server_engine.h"

ServerEngine* get_my_engine();

HX_RESULT
AddManagedMutex(HX_MUTEX pMutex)
{
    ServerEngine* pEngine = NULL;
    Process* pProc = NULL;
    ProcessContainer* pc = NULL;
    ManagedMutexList* pMutexList = NULL;

    pEngine = get_my_engine();

    if (pEngine)
    {
        pProc = (Process*)pEngine->get_proc();
    }

    if (pProc)
    {
        pc = pProc->pc;
    }

    if (pc)
    {
        pMutexList = pc->managed_mutex_list;
    }

    if (pMutexList)
    {
        return pMutexList->AddMutex(pMutex);
    }
    return HXR_FAIL;
}

HX_RESULT
RemoveManagedMutex(HX_MUTEX pMutex)
{
    ServerEngine* pEngine=NULL;
    Process* pProc=NULL;
    ProcessContainer* pc = NULL;
    ManagedMutexList* pMutexList = NULL;

    pEngine = get_my_engine();

    if (pEngine)
    {
        pProc = (Process*)pEngine->get_proc();
    }

    if (pProc)
    {
        pc = pProc->pc;
    }

    if (pc)
    {
        pMutexList = pc->managed_mutex_list;
    }

    if (pMutexList)
    {
        pMutexList->RemoveMutex(pMutex);
    }
    return HXR_FAIL;
}


/************************************************************************/

ManagedMutexList::ManagedMutexList()
    : m_nCount(0)
{
    m_pMutexList = new char*[MUTEX_LIST_MAX];
    memset((char*)m_pMutexList, 0, sizeof(char*) * MUTEX_LIST_MAX);
}


ManagedMutexList::~ManagedMutexList()
{
    HX_ASSERT(0); //should never be released
    HX_VECTOR_DELETE(m_pMutexList);
}


HX_RESULT
ManagedMutexList::AddMutex(HX_MUTEX pMutex)
{
    ++m_nCount;

    HX_ASSERT(m_nCount < MUTEX_LIST_MAX);
    if (m_nCount >= MUTEX_LIST_MAX)
    {
        return HXR_FAIL;
    }

    m_pMutexList[m_nCount-1] = (char*)pMutex;
    return HXR_OK;
}


HX_RESULT
ManagedMutexList::RemoveMutex(HX_MUTEX pMutex)
{
    HX_ASSERT(m_nCount > 0);
    if (m_nCount <= 0)
    {
        return HXR_FAIL;
    }

    // The locks are not refcounted so we just pop the most recent
    // one off the top.
    --m_nCount;

    // Check whether we're popping them off in the reverse order
    // they were added.
    HX_ASSERT((char*)pMutex == m_pMutexList[m_nCount]);
    //useful for debugging:
    //if ((char*)pMutex != m_pMutexList[m_nCount])
    //{
    //    printf("ML::RemoveMutex: pMutex != m_pMutexList[m_nCount] "
    //           "(%p, %p, %d)\n", pMutex, m_pMutexList[m_nCount], m_nCount);
    //}

    return HXR_OK;
}


//
// This is to be used during crash-recovery to prevent deadlock
// and not anywhere else!
//
INT32
ManagedMutexList::UnlockAll()
{
    int i = 0;

    for (i=0; i < m_nCount && i < MUTEX_LIST_MAX; ++i)
    {
        if (m_pMutexList[i])
        {
            HXMutexUnlock((HX_MUTEX)(m_pMutexList[i]));
        }
    }

    HX_ASSERT(i >= m_nCount);

    return i;
}

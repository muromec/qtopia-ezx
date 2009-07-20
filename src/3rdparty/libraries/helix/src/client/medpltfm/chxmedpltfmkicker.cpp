/* ***** BEGIN LICENSE BLOCK *****
* Source last modified: $Id: chxmedpltfmkicker.cpp,v 1.9 2009/05/06 20:11:57 sfu Exp $
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

#include "hxcom.h"
#include "hxresult.h"
#include "hxtypes.h"
#include "hxtick.h"
#include "hxslist.h"
#include "pckunpck.h"
#include "chxmedpltfmsched.h"
#include "chxmedpltfmkicker.h"

#if defined(_WINDOWS) && !defined(_WIN32)   /* WIN16 */
#define MINIMUM_GRANULARITY   55
#elif defined (_MACINTOSH)		    /* MACINTOSH */
#define MINIMUM_GRANULARITY   20
#elif defined (ANDROID)
#define MINIMUM_GRANULARITY   20 
#elif defined (_UNIX)
#define MINIMUM_GRANULARITY   5
#else					    /* ELSE */
#define MINIMUM_GRANULARITY   5
#endif 


IMPLEMENT_COM_CREATE_FUNCS(CHXMediaPlatformKicker)

BEGIN_INTERFACE_LIST_NOCREATE(CHXMediaPlatformKicker)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXContextUser)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXMediaPlatformKicker)
END_INTERFACE_LIST

CHXMediaPlatformKicker::CHXMediaPlatformKicker(void)
		       :m_pSchedulerMap(NULL)
		       ,m_pContext(NULL)
		       ,m_pMutex(NULL)
		       ,m_bClosed(FALSE)
{
}

CHXMediaPlatformKicker::~CHXMediaPlatformKicker(void)
{
    if (!m_bClosed)
    {
	Close();
    }
}

HX_RESULT
CHXMediaPlatformKicker::Close(void)
{
    CHXMapLongToObj::Iterator i;
    CHXSimpleList::Iterator j;

    HX_LOCK(m_pMutex);
    
    if (m_pSchedulerMap)
    {
	for (i = m_pSchedulerMap->Begin(); i != m_pSchedulerMap->End(); ++i)
	{
	    CHXSimpleList* pSchedulerPerThreadList = (CHXSimpleList*) (*i);
	    for (j = pSchedulerPerThreadList->Begin(); j != pSchedulerPerThreadList->End(); ++j)
	    {
		CHXMediaPlatformScheduler* pScheduler = (CHXMediaPlatformScheduler*) (*j);
		pScheduler->DetachKicker(this);
	    }
	    HX_DELETE(pSchedulerPerThreadList);
	}
	HX_DELETE(m_pSchedulerMap);
    }

    for( i=m_SchedTimerMap.Begin() ; i!=m_SchedTimerMap.End() ; ++i )
    {
        HXSchedulerTimer* pTmp = (HXSchedulerTimer*)(*i);
        HX_DELETE(pTmp);
    }
    
    m_bClosed = TRUE;

    HX_UNLOCK(m_pMutex);

    HX_RELEASE(m_pMutex);
    HX_RELEASE(m_pContext);    

    return HXR_OK;
}

HX_RESULT
CHXMediaPlatformKicker::AddScheduler(CHXMediaPlatformScheduler* pScheduler)
{
    HX_RESULT	      rc                      = HXR_OK;
    UINT32	      ulSchedulerThreadID     = 0;
    CHXSimpleList*    pSchedulerPerThreadList = NULL;
    HXSchedulerTimer* pSchedTimer             = NULL;

    HX_LOCK(m_pMutex);

    if (!pScheduler)
    {
	rc = HXR_INVALID_PARAMETER;
	goto exit;
    }

    if (!m_pSchedulerMap)
    {
	m_pSchedulerMap = new CHXMapLongToObj();
	if (!m_pSchedulerMap)
	{
	    rc = HXR_OUTOFMEMORY;
	    goto exit;
	}
    }

    ulSchedulerThreadID = pScheduler->GetThreadID();
    if (!ulSchedulerThreadID)
    {
	HX_ASSERT(FALSE);
	rc = HXR_FAILED;
	goto exit;
    }

    //Check to see if we need to create the list
    if (!m_pSchedulerMap->Lookup(ulSchedulerThreadID, (void*&)pSchedulerPerThreadList))
    {
	pSchedulerPerThreadList = new CHXSimpleList();
	if (!pSchedulerPerThreadList)
	{
	    rc = HXR_OUTOFMEMORY;
	    goto exit;
	}

	(*m_pSchedulerMap)[ulSchedulerThreadID] = pSchedulerPerThreadList;
    }

    //Add the scheduler onto this list
    pSchedulerPerThreadList->AddTail(pScheduler);

    //Create a timer scheduler for this thread ID.
    if( !m_SchedTimerMap.Lookup(ulSchedulerThreadID, (void*&)pSchedTimer) )
    {
        pSchedTimer = new HXSchedulerTimer();
        if( !pSchedTimer )
        {
            rc = HXR_OUTOFMEMORY;
            goto exit;
        }
        
        pSchedTimer->Init(MINIMUM_GRANULARITY);
        m_SchedTimerMap[ulSchedulerThreadID] = pSchedTimer;
    }
    

exit:

    HX_UNLOCK(m_pMutex);

    return rc;
}

HX_RESULT
CHXMediaPlatformKicker::RemoveScheduer(CHXMediaPlatformScheduler* pScheduler)
{
    HX_RESULT	    rc = HXR_OK;
    UINT32	    ulSchedulerThreadID = 0;
    CHXSimpleList*  pSchedulerPerThreadList = NULL;
    LISTPOSITION    lPos = 0;

    HX_LOCK(m_pMutex);

    if (!pScheduler)
    {
	rc = HXR_INVALID_PARAMETER;
	goto exit;
    }
    
    if (!m_pSchedulerMap)
    {
	goto exit;
    }

    ulSchedulerThreadID = pScheduler->GetThreadID();
    if (!m_pSchedulerMap->Lookup(ulSchedulerThreadID, (void*&)pSchedulerPerThreadList))
    {
	goto exit;
    }

    lPos = pSchedulerPerThreadList->Find(pScheduler);
    if (lPos)
    {
	pSchedulerPerThreadList->RemoveAt(lPos);
    }

    HX_ASSERT(!pSchedulerPerThreadList->Find(pScheduler));

exit:

    HX_UNLOCK(m_pMutex);

    return rc;
}

STDMETHODIMP
CHXMediaPlatformKicker::RegisterContext(IUnknown* pContext)
{
    HX_RESULT retVal = HXR_FAILED;

    if (!m_pContext && pContext)
    {
	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	m_pContext = pContext;
	HX_ADDREF(m_pContext);
    
	// Platform kicker should not use platform mutex as
	// this unnecesarily makes Kick() on all threads mutually
	// exclusive with execution of scheduler callbacks.
	// Additionally, using a platform mutex here would be
	// problematic as it would prevent proper kicker 
	// initialization for schedulers created prior to the
	// context initialization.  The mutex used would not
	// necessarily be the intended platform mutex since
	// platform initialization may override it via custom
	// platform scheduler.  This in turn can create unexpected 
	// behavior such as deadlocks.
	retVal = CreateInstanceCCF(CLSID_IHXMutex, 
				   (void**) &m_pMutex, 
				   m_pContext);
    }

    return retVal;
}
STDMETHODIMP
CHXMediaPlatformKicker::Kick(UINT32 ulThreadID, UINT32* pulSuggestedSleep)
{
    HX_RESULT	      rc = HXR_OK;
    CHXSimpleList*    pSchedulerPerThreadList = NULL;
    HXSchedulerTimer* pSchedTimer = NULL;
    UINT32            ulTS1;
    UINT32            ulTS2;
    CHXSimpleList::Iterator i;
    
    HX_LOCK(m_pMutex);

    if (!ulThreadID)
    {
	rc = HXR_INVALID_PARAMETER;
	goto exit;
    }

    if (!m_pSchedulerMap)
    {
	goto exit;
    }

    if (!m_pSchedulerMap->Lookup(ulThreadID, (void*&)pSchedulerPerThreadList))
    {
	goto exit;
    }

    if( !m_SchedTimerMap.Lookup(ulThreadID, (void*&)pSchedTimer ) )
    {
        goto exit;
    }
    
    //We will take timestamps for the entire loop of schedulers.
    ulTS1 = HX_GET_TICKCOUNT();
    for (i = pSchedulerPerThreadList->Begin();
         i != pSchedulerPerThreadList->End();
         ++i) 
    {
	CHXMediaPlatformScheduler* pScheduler = (CHXMediaPlatformScheduler*) (*i);
	pScheduler->OnTimeSync(FALSE);
    }
    ulTS2 = HX_GET_TICKCOUNT();

    if( pulSuggestedSleep )
    {
        *pulSuggestedSleep = pSchedTimer->AddSample(ulTS1, ulTS2);
    }
    
exit:

    HX_UNLOCK(m_pMutex);

    return rc;
}



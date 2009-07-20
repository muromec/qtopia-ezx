/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: pq.cpp,v 1.9 2008/09/07 10:42:03 pbasic Exp $
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

#include "hxtypes.h"

#include "hxassert.h"
#include "debug.h"

#include "timeval.h"

#include "hxcom.h"
#include "hxengin.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "id.h"
#include "pq.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#ifdef WIN32
/*
 * XXX This is the 3.0 bug that Glenn found. 
 * We need to fix this correctly!
 */
#define float double
#endif

PQ::PQ(CHXID* pIds)
{
    m_pHead = 0;
    m_pNextZeroInsertion = 0;
    m_lElementCount = 0;
    m_HeadTime.tv_sec = PQ_UNINITIALIZED;

    gettimeofday(&m_Bucket0Time, 0);
    m_Bucket0Time.tv_usec = 0;

    memset (m_pBuckets, 0, NUM_BUCKETS * sizeof (PQElem*));

    if (pIds)
    {
	m_pIds = pIds;
	m_bOwnID = FALSE;
    }
    else
    {
	m_pIds = new CHXID;
	m_bOwnID = TRUE;
    }
}

PQ::~PQ()
{
    destruct();
}

void
PQ::destruct()
{
    // check if we're already destructed
    if (!m_pIds)
    {
        return;
    }

    Timeval now;

    now.tv_sec = 0x7fffffff;
    now.tv_usec = 0x7fffffff;
    PQElem*	pElem = _remove_head(now);

    while (pElem) 
    {
	PQElem* pElemNext = pElem->m_pNext;
	m_pIds->destroy(pElem->m_Id);
	if (!pElem->m_bDefunct)
	    free_callback(pElem->m_pCallback);
	free_elem(pElem);
	if (!pElemNext)
	    break;
	pElem = pElemNext; 
    }

    for (int i = 0; i < NUM_BUCKETS; i++)
    {
	PQElem*	pElem = m_pBuckets[i];

	while (pElem) 
	{
	    PQElem* pElemNext = pElem->m_pNext;
	    m_pIds->destroy(pElem->m_Id);
	    if (!pElem->m_bDefunct)
		free_callback(pElem->m_pCallback);
	    free_elem(pElem);
	    if (!pElemNext)
		break;
	    pElem = pElemNext; 
	}
    }

    if (m_bOwnID)
    {
	HX_DELETE(m_pIds);
    }

    // mark as destructed
    m_pIds = 0;
}

UINT32
PQ::enter(Timeval t, IHXCallback* pCallback)
{
#ifdef WIN32
    /*
     * Normalize t for NT
     */

    t.tv_usec = (t.tv_usec / 1000) * 1000;
#endif

    PQElem** pq;
    PQElem* q;

    PQElem* i = new_elem();
    HX_ASSERT (i);

    pCallback->AddRef();
    i->m_pCallback  = pCallback;
    i->m_bRemoved = 0;

#ifdef SMP_PQ_DEBUG
    if (pCallback < 0x8000)
    {
	printf ("Bad Callback!! BAD BAD BAD!\n");

	volatile int* pCrashMe = 0;
	*pCrashMe = 5;
    }
#endif

    int bucket = (int)((t.tv_sec  - m_Bucket0Time.tv_sec)  * RESOLUTION +
    	    	 (t.tv_usec - m_Bucket0Time.tv_usec) / (float)USEC_PER_BUCKET);

    if ((bucket < NUM_BUCKETS) && (bucket >= 0))
    {
	i->m_pNext = m_pBuckets[bucket];
    	i->m_Time = t;
    	m_pBuckets[bucket] = i;
    }
    else
    {
	if (t.tv_sec == 0)
	{
	    if (m_pNextZeroInsertion)
	    {
		i->m_pNext = m_pNextZeroInsertion->m_pNext;
		m_pNextZeroInsertion->m_pNext = i;
		m_pNextZeroInsertion = i;
		i->m_Time = t;

		goto inserted;
	    }
	    else
	    {
		m_pNextZeroInsertion = i;
	    }
	}

	for (pq = &m_pHead; (q = *pq) != 0; pq = &q->m_pNext)
	{
	    if (q->m_Time >= t)
		break;
	}
	*pq = i;
	i->m_pNext = q;
	i->m_Time = t;
    }

inserted:
    if ((m_HeadTime.tv_sec == PQ_UNINITIALIZED) || (m_HeadTime > t))
    	m_HeadTime = t;

    m_lElementCount++;

    i->m_Id = m_pIds->create((void *)i);
//{FILE* f1 = ::fopen("c:\\temp\\pq.txt", "a+"); ::fprintf(f1, "enter: %lu\n", i->m_Id);::fclose(f1);}

    return i->m_Id;
}

void
PQ::remove(UINT32 id)
{
//{FILE* f1 = ::fopen("c:\\temp\\pq.txt", "a+"); ::fprintf(f1, "remove: %lu\n", id);::fclose(f1);}
    PQElem* i = (PQElem*)m_pIds->get(id);
    HX_ASSERT(i);
    if (i)
    {
	i->m_bDefunct = 1;
	free_callback(i->m_pCallback);
    }
}

HXBOOL
PQ::removeifexists(UINT32 id)
{
    PQElem* i = (PQElem*)m_pIds->get(id);
    
    if (i)
    {
//{FILE* f1 = ::fopen("c:\\temp\\pq.txt", "a+"); ::fprintf(f1, "removeifexists: %lu\n", id);::fclose(f1);}
	i->m_bDefunct = 1;
	free_callback(i->m_pCallback);
	return TRUE;
    }

    return FALSE;
}

HXBOOL		
PQ::exists(UINT32 id)
{
    return ((PQElem*)m_pIds->get(id) != 0);
}

int
PQ::execute(Timeval now)
{
    PQElem*	pElem = _remove_head(now);
    int ulCount = 0;

    /* Make sure head time is uninitialized ONLY if element count is zero */
    HX_ASSERT(m_HeadTime.tv_sec != PQ_UNINITIALIZED || m_lElementCount == 0);

    while (pElem) 
    {
        PQElem* pElemNext = dispatch_element(pElem);
        if (!pElem->m_bDefunct)
            ulCount++;
        destroy_element(pElem);
        if (!pElemNext)
            break;
        pElem = pElemNext; 
    }

    return ulCount;
}

PQElem*
PQ::get_execute_list(Timeval now)
{
    return _remove_head(now);
}

PQElem*
PQ::execute_element(PQElem* pElem)
{
    PQElem* pElemNext = dispatch_element(pElem);
    destroy_element(pElem);
    return pElemNext;
}


PQElem*
PQ::_remove_head(Timeval now)
{
    int i;
    PQElem* out_list	= 0;
    PQElem* current	= 0;

    m_pNextZeroInsertion = NULL;

    int bucket = (int)((now.tv_sec  - m_Bucket0Time.tv_sec)  * RESOLUTION +
    	    	 (now.tv_usec - m_Bucket0Time.tv_usec) / (float)USEC_PER_BUCKET);

    // We are not yet ready to empty the first bucket
    if (bucket < 0)
    {
    	m_HeadTime.tv_sec = PQ_UNINITIALIZED;

    	for (i = 0; i < NUM_BUCKETS; i++)
    	    if (m_pBuckets[i])
    	    {
    	    	m_HeadTime = m_pBuckets[i]->m_Time;
    	    	break;
    	    }
    }
    else
    {
    	if (bucket >= NUM_BUCKETS)
    	    bucket = NUM_BUCKETS - 1;

    	for (i = 0; i <= bucket; i++)
    	{
    	    if (!m_pBuckets[i])
    	    	continue;

    	    if (!out_list)
    	    	out_list = m_pBuckets[i];

    	    if (current)
    	    {
    	    	// Concatenate with previous bucket's list
    	    	current->m_pNext = m_pBuckets[i];
    	    }

    	    current = m_pBuckets[i];

    	    while (current->m_pNext)
    	    {
		current = current->m_pNext;
    	    	m_lElementCount--;
    	    }
	    m_lElementCount--;
    	}

    	// Update the first bucket's time stamp.
    	Timeval t;
    	t.tv_sec = 0;
    	t.tv_usec = (int)((float)USEC_PER_BUCKET * (bucket + 1));
    	m_Bucket0Time += t;

#ifdef WIN32
	/*
	 * Adjust Bucket0Time.tv_usec for NT
	 */

	m_Bucket0Time.tv_usec = (m_Bucket0Time.tv_usec / 1000) * 1000;
#endif

    	// Move the buckets up
	memmove (m_pBuckets, &m_pBuckets[bucket + 1], (NUM_BUCKETS - bucket - 1) *
    	    sizeof (void *));

    	// Zero all new buckets
	for (i = NUM_BUCKETS - 1;; i--, bucket--)
    	{
    	    m_pBuckets[i] = 0;
    	    if (!bucket)
    	    	break;
    	}

    	// Recalculate the m_HeadTime
    	m_HeadTime.tv_sec = PQ_UNINITIALIZED;
    	for (i = 0; i < NUM_BUCKETS; i++)
    	    if (m_pBuckets[i])
    	    {
    	    	m_HeadTime = m_pBuckets[i]->m_Time;
    	    	break;
    	    }
    }

    PQElem* h;

    if (!m_pHead)
	return out_list;

    h = m_pHead;

    /*
     * Keep track of the head time from the buckets
     */

    Timeval TempHeadTime = m_HeadTime;
    
    if ((m_HeadTime.tv_sec == PQ_UNINITIALIZED) || (h->m_Time < m_HeadTime))
    	m_HeadTime = h->m_Time;

    if (h->m_Time != (LONG32)0 && now < h->m_Time)
	return out_list;

    PQElem* last;
    for (;;)
    {
	last = m_pHead;
	m_lElementCount--;
	m_pHead = last->m_pNext;

	if (!m_pHead)
	    break;

	if (m_pHead->m_Time != (LONG32)0 && now < m_pHead->m_Time)
	{
	    last->m_pNext = 0;
	    break;
	}
    }

    /*
     * If the head time from the alternate queue is earlier than the head
     * time from the buckets, keep the head time from the alternate queue
     */

    if (m_pHead &&
        (TempHeadTime.tv_sec == PQ_UNINITIALIZED || m_pHead->m_Time < TempHeadTime))
	TempHeadTime = m_pHead->m_Time;

    /*
     * Now reset m_HeadTime to the time of the queue which will be ready
     * first
     */

    if (m_HeadTime == (LONG32)0 || TempHeadTime < m_HeadTime)
    	m_HeadTime = TempHeadTime;

    if (current)
    {
	last->m_pNext = out_list;
	out_list = h;
    }
    else
    	return h;

    return out_list;
}

PQElem* PQ::dispatch_element(PQElem* pElem)
{
    PQElem* pRet = NULL;
    
    if (pElem)
    {
        pRet = pElem->m_pNext;
        if (!pElem->m_bDefunct && pElem->m_pCallback)
        {
            pElem->m_pCallback->Func();
            free_callback(pElem->m_pCallback);
        }
    }
    
    return pRet;
}

void PQ::destroy_element(PQElem* pElem)
{
    if (pElem && m_pIds)
    {
        m_pIds->destroy(pElem->m_Id);
        free_elem(pElem);
    }
}


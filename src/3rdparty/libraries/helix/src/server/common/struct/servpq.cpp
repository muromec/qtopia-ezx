/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: servpq.cpp,v 1.10 2007/09/20 14:18:05 rdolas Exp $ 
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

#include "hxtypes.h"
#include "hxcom.h"

#include "hxassert.h"
#include "debug.h"
#include "trycatch.h"

#include "timeval.h"

#include "hxengin.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "id.h"
#include "proc.h"
#include "servpq.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static char HX_THIS_FILE[] = __FILE__;
#endif


ServPQ::ServPQ(CHXID* pIds)
    : m_pFastAlloc(NULL)
    , m_bCleanFinish(TRUE)
    , m_pNextZeroInsertion(NULL)
    , m_pHead(0)
    , m_pOutlist(0)
    , m_pCurrent(0)
    , m_pRestoreHead(0)
    , m_pPrevHead(0)
    , m_lBucketElementCount(0)
    , m_lZeroElementCount(0)
    , m_lListElementCount(0)
    , m_pProc(0)
    , m_uBucket0Index(0)
    , m_uCrashCount(0)
    , m_uCurrentBucketNum(0)
{
    m_HeadTime.tv_sec = SERVPQ_UNINITIALIZED;

    gettimeofday(&m_Bucket0Time, 0);
    m_Bucket0Time.tv_usec = 0;

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

    memset(m_pBuckets, 0, SERVPQ_NUM_BUCKETS * sizeof(ServPQElem*));
    m_CrashState = NONE;
}

ServPQ::~ServPQ()
{
    Timeval now;

    now.tv_sec = 0x7fffffff;
    now.tv_usec = 0x7fffffff;
    ServPQElem*	pElem = _remove_head(now);

    while (pElem) 
    {
	ServPQElem* pElemNext = pElem->m_pNext;
	m_pIds->destroy(pElem->m_Id);
	if (!pElem->m_bDefunct)
	    free_callback(pElem->m_pCallback);
	free_elem(pElem);
	if (!pElemNext)
	    break;
	pElem = pElemNext; 
    }

    for (int i = 0; i < SERVPQ_NUM_BUCKETS; i++)
    {
	ServPQElem* pElem = m_pBuckets[i];

	while (pElem) 
	{
	    ServPQElem* pElemNext = pElem->m_pNext;
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
}

void
ServPQ::init(Process* p)
{
    if (m_pProc)
	fprintf(stderr, "ServPQ for PID(%d, %d) ---> PID(%d, %d)\n",
	    m_pProc->procid(m_pProc->procnum()), m_pProc->procnum(),
	    p->procid(p->procnum()), p->procnum());

    m_pProc = p;
}

UINT32
ServPQ::enter(Timeval t, IHXCallback* pCallback)
{
#ifdef WIN32
    /*
     * Normalize t for NT
     */

    t.tv_usec = (t.tv_usec / 1000) * 1000;
#endif

    ServPQElem** pq;
    ServPQElem* q;

    ServPQElem* i = new_elem();
    HX_ASSERT (i);

    pCallback->AddRef();
    i->m_pCallback  = pCallback;

#ifdef SMP_PQ_DEBUG
    if (pCallback < 0x8000)
    {
	printf ("Bad Callback!! BAD BAD BAD!\n");

	volatile int* pCrashMe = 0;
	*pCrashMe = 5;
    }
#endif

    // convert everthing into microseconds and then divide by the us per
    // bucket. everything is INT64 so that there is no wrap around with
    // negative values... for a long time.
    INT64 bucket = (((INT64)(t.tv_sec - m_Bucket0Time.tv_sec) * 1000000) + 
	(INT64)(t.tv_usec - m_Bucket0Time.tv_usec)) / SERVPQ_USEC_PER_BUCKET;

    if ((bucket < SERVPQ_NUM_BUCKETS) && (bucket >= 0))
    {
	i->m_pNext = m_pBuckets[(UINT16)(m_uBucket0Index+bucket)];
    	i->m_Time = t;
    	m_pBuckets[(UINT16)(m_uBucket0Index+bucket)] = i;
	m_lBucketElementCount++;
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
		m_lZeroElementCount++;

		goto inserted;
	    }
	    else
	    {
		m_pNextZeroInsertion = i;
		m_lZeroElementCount++;
	    }
	}
	else
	    m_lListElementCount++;

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
    if (m_HeadTime.tv_sec == SERVPQ_UNINITIALIZED || m_HeadTime > t)
    	m_HeadTime = t;

    i->m_Id = m_pIds->create((void *)i);

    return i->m_Id;
}

void
ServPQ::remove(UINT32 id)
{
    ServPQElem* i = (ServPQElem*)m_pIds->get(id);
    HX_ASSERT(i);
    if (i)
    {
	i->m_bDefunct = 1;
	free_callback(i->m_pCallback);
    }
}

BOOL
ServPQ::removeifexists(UINT32 id)
{
    ServPQElem* i = (ServPQElem*)m_pIds->get(id);
    
    if (i)
    {
	i->m_bDefunct = 1;
	free_callback(i->m_pCallback);
	return TRUE;
    }

    return FALSE;
}

BOOL		
ServPQ::exists(UINT32 id)
{
    return ((ServPQElem*)m_pIds->get(id) != 0);
}

int
ServPQ::execute(Timeval now)
{
    ServPQElem*	pElem = _remove_head(now);
    int ulCount = 0;

    while (pElem) 
    {
	ServPQElem* pElemNext = pElem->m_pNext;
	if (!pElem->m_bDefunct)
	{
            pElem->m_pCallback->Func();
            ulCount++;
            if (pElem->m_bDefunct)
            {
                HX_ASSERT(0 && "Someone removed me while I was executing!!!");
            }
            else
            {
                pElem->m_pCallback->Release();
            }
	}
	m_pIds->destroy(pElem->m_Id);
	delete pElem;
	if (!pElemNext)
	    break;
	pElem = pElemNext; 
    }

    return ulCount;
}

ServPQElem*
ServPQ::get_execute_list(Timeval now)
{
    return _remove_head(now);
}

ServPQElem*     
ServPQ::execute_locked_element(ServPQElem* pElem) 
{
    if (m_bCleanFinish && pElem)
    {
        ServPQElem* pElemNext = pElem->m_pNext;
        if (!pElem->m_bDefunct)
        {
	    m_bCleanFinish = FALSE;

            pElem->m_pCallback->Func();
            m_bCleanFinish = TRUE;
            if (pElem->m_bDefunct)
            {
                HX_ASSERT(0 && "Someone removed me while I was executing!!!");
            }
            else
            {
                free_callback(pElem->m_pCallback);
            }
        }
        m_pIds->destroy(pElem->m_Id);
        free_elem(pElem);
        pElem = pElemNext;
    }
    else if (m_bCleanFinish == FALSE)
    {
	/* Previous Func() must have caused a GPF */

	m_bCleanFinish = TRUE;
	return pElem->m_pNext;
    }

    return pElem;
}

ServPQElem*     
ServPQ::execute_element(ServPQElem* pElem) 
{
    if (m_bCleanFinish && pElem)
    {
        ServPQElem* pElemNext = pElem->m_pNext;
        if (!pElem->m_bDefunct)
        {
	    m_bCleanFinish = FALSE;

            pElem->m_pCallback->Func();
            m_bCleanFinish = TRUE;
            if (pElem->m_bDefunct)
            {
                HX_ASSERT(0 && "Someone removed me while I was executing!!!");
            }
            else
            {
                free_callback(pElem->m_pCallback);
            }
        }
        m_pIds->destroy(pElem->m_Id);
        free_elem(pElem);
        pElem = pElemNext;
    }
    else if (m_bCleanFinish == FALSE)
    {
	/* Previous Func() must have caused a GPF */

	m_bCleanFinish = TRUE;
	return pElem->m_pNext;
    }

    return pElem;
}

/*
 * diff time = now - bucket 0 time <==> now > bucket 0 time
 *
 * xxxaak -- Tue Sep 14 2004
 *
 * XXX NOTE:
 *     THE m_uBucket0Index RELIES ON THE FACT THAT ITS UINT16 VALUE WILL
 * OVERFLOW AND WRAPAROUND!!!
 *
 * m_pBuckets has 65536 elements which is the upper limit of a UINT16.
 * the m_pBuckets array is maintained as a circular array of buckets via the
 * m_uBucket0Index (UINT16) variable. 
 *
 * for example is "now" is 1 second, then we would have to concatenate all the
 * servpq elements from 128 buckets starting from m_uBucket0Index. once that
 * is done, m_uBucket0Index is incremented to bucket+1. 
 *
 * due to the fact that m_uBucket0Index is a UINT16 even if bucket is 65535
 * adding 1 to it will cause an overflow and bring back the m_uBicket0Index to
 * 0 (zero). 
 *
 * this property of using overflow and wrapping around of the UINT16 index
 * variable eliminates the memmove() that was present in the earlier design.
 */
ServPQElem*
ServPQ::_remove_head(Timeval now)
{
    UINT16 i;

    if (m_CrashState != SPQ_HANDLING_CRASH)
    {
        m_pOutlist = 0;
        m_pCurrent = 0;
        m_pPrevHead = 0;
    }

    UINT16 uMaxBucket = 0;

    INT64 bucket = (((INT64)(now.tv_sec - m_Bucket0Time.tv_sec) * 1000000) + 
	(INT64)(now.tv_usec - m_Bucket0Time.tv_usec)) / SERVPQ_USEC_PER_BUCKET;

    m_CrashState = SPQ_BUCKETS;
    // We are not yet ready to empty the first bucket
    if (bucket < 0)
    {
    	m_HeadTime.tv_sec = m_HeadTime.tv_usec = SERVPQ_UNINITIALIZED;

	uMaxBucket = m_uBucket0Index + SERVPQ_NUM_BUCKETS - 1;
	if (m_lBucketElementCount > 0)
	{
	    for (i = m_uBucket0Index; i != uMaxBucket; i++)
	    {
		if (m_pBuckets[i])
		{
		    m_HeadTime = m_pBuckets[i]->m_Time;
		    break;
		}
	    }
	}
    }
    else
    {
    	if (bucket >= SERVPQ_NUM_BUCKETS_MINUS_ONE)
	{
	    /*
	     * if the bucket value is for the last bucket
	     * (SERVPQ_NUM_BUCKETS_MINUS_ONE) don't change its
	     * value because in the following for() loop the stopping
	     * condition will allow it to search the entire array.
	     * if on the other hand the bucket is >= SERVPQ_NUM_BUCKETS set it
	     * to SERVPQ_NUM_BUCKETS_MINUS_ONE.
	     */
	    if (bucket >= SERVPQ_NUM_BUCKETS)
		bucket = SERVPQ_NUM_BUCKETS_MINUS_ONE;
	    uMaxBucket = m_uBucket0Index + (UINT16)(bucket);
	}
	else
	{
	    uMaxBucket = m_uBucket0Index + (UINT16)(bucket + 1);
	}
	/*
	 * make sure that m_uBucket0Index != uMaxBucket or else instead of
	 * going thru the entire m_pBuckets it will drop out immediately. the
	 * above code makes sure that that doesn't happen.
	 */
    	for (i = m_uBucket0Index; m_lBucketElementCount > 0 && i != uMaxBucket; i++)
    	{
            m_uCurrentBucketNum = i;

    	    if (!m_pBuckets[i])
    	    	continue;

    	    if (!m_pOutlist)
    	    	m_pOutlist = m_pBuckets[i];

    	    if (m_pCurrent)
    	    {
    	    	// Concatenate with previous bucket's list
    	    	m_pCurrent->m_pNext = m_pBuckets[i];
    	    }
            
    	    m_pCurrent = m_pBuckets[i];

    	    while (m_pCurrent->m_pNext)
    	    {
		m_pCurrent = m_pCurrent->m_pNext;
    	    	m_lBucketElementCount--;
    	    }
	    m_lBucketElementCount--;
	    m_pBuckets[i] = 0;
    	}

    	// Update the first bucket's time stamp.
    	Timeval t;
    	t.tv_sec = 0;
    	t.tv_usec = (int)(SERVPQ_USEC_PER_BUCKET * (bucket + 1));
    	m_Bucket0Time += t;
	/*
	 * increment m_uBucket0Index to the next index past bucket because
	 * we've already serviced all elements from the old m_uBucket0Index
	 * upto bucket.
	 */
	m_uBucket0Index += (UINT16)(bucket+1);

#ifdef WIN32
	/*
	 * Adjust Bucket0Time.tv_usec for NT
	 */

	m_Bucket0Time.tv_usec = (m_Bucket0Time.tv_usec / 1000) * 1000;
#endif

    	// Recalculate the m_HeadTime
    	m_HeadTime.tv_sec = m_HeadTime.tv_usec = SERVPQ_UNINITIALIZED;
	uMaxBucket = m_uBucket0Index + SERVPQ_NUM_BUCKETS - 1;
	if (m_lBucketElementCount > 0)
	{
	    for (i = m_uBucket0Index; i != uMaxBucket; i++)
	    {
		if (m_pBuckets[i])
		{
		    m_HeadTime = m_pBuckets[i]->m_Time;
		    break;
		}
	    }
	}
    }

    if (!m_pHead)
        return m_pOutlist;

    m_pRestoreHead = m_pHead;

    /*
     * Keep track of the head time from the buckets
     */

    Timeval TempHeadTime = m_HeadTime;
    
    if (m_HeadTime.tv_sec == SERVPQ_UNINITIALIZED || m_pRestoreHead->m_Time < m_HeadTime)
        m_HeadTime = m_pRestoreHead->m_Time;

    if (m_pRestoreHead->m_Time != (LONG32)0 && now < m_pRestoreHead->m_Time) 
        return m_pOutlist;

    ServPQElem* last;
    if (m_pNextZeroInsertion)
    {
        m_CrashState = SPQ_ZERO_LIST;
	last = m_pNextZeroInsertion;
	m_pHead = last->m_pNext;
        m_pPrevHead = m_pHead;
	m_pNextZeroInsertion = NULL;
	m_lZeroElementCount = 0;
    }
    else
    {
        m_CrashState = SPQ_OVERFLOW_LIST;
	last = m_pHead;
	m_lListElementCount--;
        m_pPrevHead = m_pHead;
	m_pHead = last->m_pNext;
    }

    for (;m_pHead;)
    {
	if (now < m_pHead->m_Time)
	{
	    last->m_pNext = 0;
	    break;
	}
	last = m_pHead;
	m_lListElementCount--;
        m_pPrevHead = m_pHead;
	m_pHead = last->m_pNext;
    }

    /*
     * If the head time from the alternate queue is earlier than the head
     * time from the buckets, keep the head time from the alternate queue
     */

    if (m_pHead && 
        (TempHeadTime.tv_sec == SERVPQ_UNINITIALIZED || m_pHead->m_Time < TempHeadTime))
	TempHeadTime = m_pHead->m_Time;

    /*
     * Now reset m_HeadTime to the time of the queue which will be ready
     * first
     */

    if (m_HeadTime == (LONG32)0 || TempHeadTime < m_HeadTime)
    	m_HeadTime = TempHeadTime;

    m_CrashState = NONE;
    m_uCrashCount = 0;

    if (m_pCurrent)
    {
        last->m_pNext = m_pOutlist;
        m_pOutlist = m_pRestoreHead;
    }
    else
        return m_pRestoreHead;

    return m_pOutlist;

}


void
ServPQ::crash_recovery()
{
    m_uCrashCount++;

    switch (m_CrashState)
    {
        case SPQ_ZERO_LIST:
        {
            if (!m_pPrevHead)
            {
                ServPQElem *zeroList = m_pRestoreHead;
                while (zeroList->m_pNext != m_pNextZeroInsertion)
                {
                    zeroList = zeroList->m_pNext;
                }
                zeroList->m_pNext = 0;
                m_pHead = m_pRestoreHead;
                m_pRestoreHead = 0;
            }
            else
            {
                if (m_uCrashCount < 3)
                {
                    if ((m_uCrashCount < 2) && (m_pPrevHead->m_pNext != 0))
                    {
                        //Skip the overflow list node having corrupted data members
                        m_pPrevHead->m_pNext = m_pPrevHead->m_pNext->m_pNext;
                    }
                    else
                    { 
                        /*
                         * Truncate the overflow list if not recoverable
                         * by skipping the corrupted node.
                         */
                        m_pPrevHead->m_pNext = 0;
                    }

                    m_pHead = m_pRestoreHead;
                    m_lListElementCount--;
                    m_pRestoreHead = 0;
                }
                else 
                {
                    if (m_pNextZeroInsertion)
                    {
                        m_pNextZeroInsertion = NULL;
                        m_lZeroElementCount = 0;
                    }
                    m_pHead = 0;
                    m_lListElementCount = 0;
                    m_pRestoreHead = 0;
                    m_uCrashCount = 0;
                }
            }
        }
        break;

        case SPQ_OVERFLOW_LIST:
        {
            if (!m_pPrevHead)
            {
                m_pHead = 0;
                m_lListElementCount = 0;
            }
            else
            {
                if (m_uCrashCount < 3)
                {
                    if ((m_uCrashCount < 2) && (m_pPrevHead->m_pNext != 0))
                    {
                        //Skip the overflow list node having corrupted data members
                        m_pPrevHead->m_pNext = m_pPrevHead->m_pNext->m_pNext;
                    }
                    else
                    {
                        /*
                         * Truncate the overflow list if not recoverable
                         * by skipping the corrupted node.
                         */
                        m_pPrevHead->m_pNext = 0;
                    }

                    m_pHead = m_pRestoreHead;
                    m_lListElementCount--;
                    m_pRestoreHead = 0;
                }
                else
                {
                    m_pHead = 0;
                    m_lListElementCount = 0;
                    m_pRestoreHead = 0;
                    m_uCrashCount = 0;
                }
            }
        }
        break;

        case SPQ_BUCKETS:
        {
            if (m_pOutlist)
            {
                ServPQElem * getPrev = m_pOutlist;

                while (getPrev->m_pNext != m_pCurrent)
                {
                    getPrev = getPrev->m_pNext;
                }

                m_pCurrent = getPrev;
                getPrev->m_pNext = 0;

                m_lBucketElementCount--;
                m_pBuckets[m_uCurrentBucketNum] = 0;
            }
        }
        break;

        case NONE:
            break;

        default:
            break;
    } 

    m_CrashState = SPQ_HANDLING_CRASH;
    m_HeadTime.tv_sec = m_HeadTime.tv_usec = SERVPQ_UNINITIALIZED;
}


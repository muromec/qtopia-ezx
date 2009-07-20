/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: servpq.h,v 1.9 2007/09/20 14:18:06 rdolas Exp $ 
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

#ifndef _SERVPQ_H_
#define _SERVPQ_H_

#include "id.h"
#include "hxassert.h"
#include "hxcom.h"
#include "hxtypes.h"
#include "hxcomm.h"

/*
 * SERVPQ_NUM_BUCKETS is the number of buckets that we will allocate.
 * SERVPQ_USEC_PER_BUCKET is the number of microseconds per bucket.
 * they r powers of 2 so that the calculation will b faster
 */
#define SERVPQ_NUM_BUCKETS		65536 
#define SERVPQ_NUM_BUCKETS_MINUS_ONE	65535
#define SERVPQ_USEC_PER_BUCKET		8192 
#define SERVPQ_UNINITIALIZED 		1

class Process;

class ServPQElem 
{
public:
    FAST_CACHE_MEM
    ServPQElem() 
	: m_pCallback(0)
	, m_pNext(0)
	, m_bDefunct(0)
	, m_Id(0)
    {
	m_Time.tv_sec = m_Time.tv_usec = 0;
    }
    IHXCallback*	m_pCallback;
    ServPQElem*	    	m_pNext;
    Timeval	    	m_Time;
    BOOL	    	m_bDefunct;
    UINT32		m_Id;
};

class ServPQ
{
public:
    ServPQ(CHXID* pIds = NULL);
    ~ServPQ();

    void	init(Process* p);
    UINT32	enter(Timeval t, IHXCallback* pCallback);

    void	remove(UINT32 handle);
    BOOL	removeifexists(UINT32 handle);

    int		execute(Timeval now);
    ServPQElem*	get_execute_list(Timeval now);
    ServPQElem*	execute_locked_element(ServPQElem* pElem);
    ServPQElem*	execute_element(ServPQElem* pElem);

    ServPQElem*	new_elem(void)  { return (new(m_pFastAlloc) ServPQElem); }
    void	free_elem(ServPQElem*& pElem) { delete pElem; }
    void	free_callback(IHXCallback* pCb) { pCb->Release(); }
    void	free_mem() { }

    int		bucket_empty() { return m_lBucketElementCount == 0; }
    LONG32	bucket_count() { return m_lBucketElementCount; }
    int		zero_list_empty() { return m_lZeroElementCount == 0; }
    LONG32	zero_list_count() { return m_lZeroElementCount; }
    int		list_empty() { return m_lListElementCount == 0; }
    LONG32	list_count() { return m_lListElementCount; }
    int		empty() { return m_lListElementCount == 0 && m_lZeroElementCount == 0 && m_lBucketElementCount == 0; }
    int		immediate() { return (!empty()) && (m_HeadTime.tv_sec == 0 && m_HeadTime.tv_usec == 0); }

    Timeval	head_time() { return m_HeadTime; }
    BOOL	exists(UINT32 handle);
    void        crash_recovery();

    IHXFastAlloc*	m_pFastAlloc;

private:
    virtual ServPQElem*	_remove_head(Timeval now);

    ServPQElem*		m_pBuckets[SERVPQ_NUM_BUCKETS];
    ServPQElem*		m_pHead;
    ServPQElem*	        m_pOutlist;
    ServPQElem*	        m_pCurrent;
    ServPQElem*	        m_pRestoreHead;
    ServPQElem*	        m_pPrevHead;
    ServPQElem*		m_pNextZeroInsertion;
    LONG32		m_lListElementCount;
    LONG32		m_lZeroElementCount;
    LONG32              m_lBucketElementCount;
    Timeval		m_Bucket0Time;
    Timeval		m_HeadTime;
    CHXID*		m_pIds;
    BOOL		m_bOwnID;
    Process*		m_pProc;
    Timeval		m_DiffTime;
    UINT16		m_uBucket0Index;

    BOOL		m_bCleanFinish;
    UINT16              m_uCrashCount;
    UINT16              m_uCurrentBucketNum;

    enum _CrashState { SPQ_BUCKETS, SPQ_ZERO_LIST, SPQ_OVERFLOW_LIST, SPQ_HANDLING_CRASH, NONE };
        _CrashState m_CrashState;
};

#endif

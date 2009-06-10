/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: pq.h,v 1.9 2008/09/07 10:41:44 pbasic Exp $
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

#ifndef _PQ_H_
#define _PQ_H_

#include "id.h"
#include "hxcom.h"
#include "hxtypes.h"
#include "hxcomm.h"
#include "hxengin.h"
#include "hxassert.h"
#include "timeval.h"

class CHXID;

/*
 * Timing resolution is equal to 1/RESOLUTION seconds.
 * BUCKET_TIME is the amount of time that we would like buckets allocated for.
 * NUM_BUCKETS is the number of buckets that we will allocate.
 * USEC_PER_BUCKET is the number of microseconds per bucket.
 */
#define RESOLUTION	64
#define BUCKET_TIME	8
#define NUM_BUCKETS	(RESOLUTION * BUCKET_TIME)
#define USEC_PER_BUCKET	(1000000 / RESOLUTION)
#define PQ_UNINITIALIZED 1

class PQElem 
{
public:
    FAST_CACHE_MEM
    PQElem() : m_pCallback(0), m_pNext(0), m_bDefunct(0), m_bRemoved(0), m_Id(0)
    {
	m_Time.tv_sec   = 0;
	m_Time.tv_usec  = 0;
    }
    IHXCallback*	m_pCallback;
    PQElem*	    	m_pNext;
    Timeval	    	m_Time;
    HXBOOL	    	m_bDefunct;
    HXBOOL	    	m_bRemoved;
    UINT32		m_Id;
};

class PQ
{
protected:
    PQElem*		_remove_head(Timeval now);
    PQElem*     dispatch_element(PQElem* pElem);
    void        destroy_element(PQElem* pElem);
    void        destruct();

    PQElem*	m_pBuckets[NUM_BUCKETS];
    PQElem*	m_pHead;
    PQElem*	m_pNextZeroInsertion;
    LONG32	m_lElementCount;
    Timeval	m_Bucket0Time;
    Timeval	m_HeadTime;
    CHXID*	m_pIds;
    HXBOOL	m_bOwnID;

public:
			PQ(CHXID* pIds = NULL);
    virtual		~PQ();
    virtual int		execute(Timeval now);
    virtual UINT32	enter(Timeval t, IHXCallback* i);

    virtual PQElem*	get_execute_list(Timeval now);
    virtual PQElem*	execute_element(PQElem* pElem);

    virtual void	free_callback(IHXCallback* pCb) { pCb->Release(); }
    virtual PQElem*	new_elem(void)  { return (new PQElem); }
    virtual void	free_elem(PQElem*& pElem) { delete pElem; }
    virtual void	free_mem() { }

    virtual void	remove(UINT32 handle);
    virtual HXBOOL	removeifexists(UINT32 handle);

    int		empty() { return m_lElementCount == 0; }
    int		immediate() { return (!empty()) && (m_HeadTime.tv_sec == 0 && m_HeadTime.tv_usec == 0); }

    Timeval	head_time();
    HXBOOL	exists(UINT32 handle);

};

inline Timeval
PQ::head_time()
{
    //ASSERT (m_HeadTime.tv_sec > 0);XXXSMP It should be ok that HeadTime == 0
    return m_HeadTime;
}

#endif

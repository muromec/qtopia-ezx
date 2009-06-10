/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: dist_lic_requester.h,v 1.3 2003/09/04 22:39:08 dcollins Exp $ 
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

#ifndef _DIST_LIC_REQUESTER_H_
#define _DIST_LIC_REQUESTER_H_

#include <limits.h>

#include "hxtypes.h"
#include "hxcom.h"

#include "hxengin.h"

#include "hxmap.h"
#include "simple_callback.h"
#include "mutex.h"
#include "proc_container.h"
#include "dispatchq.h"
#include "proc.h"
#include "servlist.h"
#include "simple_callback.h"
#include "id.h"
#include "hxdist_lic_requester.h"

class LicenseRequesterCallback : public SimpleCallback
{
public:
    LicenseRequesterCallback(IHXCallback* pCb);

    INT32 AddRef();
    INT32 Release();
    void SetStatus(DistLicReqStatus eLicStatus, Process* pProc);
    void func(Process* pProc);

    IHXCallback* m_pCb;
    IHXDistributedLicenseRequestStatus* m_pDLRS;
    DistLicReqStatus m_eLicStatus;
    INT32 m_lRefCount;

    inline void
    MutexUnlock(Process* pProc)
    {
        HX_ASSERT(m_nLockedBy == pProc->procnum());
        m_nLockedBy = -1;
        HXMutexUnlock(m_pMutex);
    };

    inline BOOL
    MutexLockIfNeeded(Process* pProc)
    {
        if (m_nLockedBy != pProc->procnum())
        {
            HXMutexLock(m_pMutex, TRUE);
            m_nLockedBy = (UINT32)(pProc->procnum());
            m_pProc = pProc;
            return TRUE;
        }
        return FALSE;
    };

#define LRC_ISLOCKED() HX_ASSERT(m_nLockedBy != -1)

#ifdef VERBOSE_REGISTRY_LOCKING
//for debugging
#define LRC_LOCK(p) \
BOOL bLocked = MutexLockIfNeeded(p); \
bLocked ? \
  printf("PendLic: lock at line %d pid=%d\n", __LINE__, (p)->procnum()) : \
  printf("PendLic: already locked at line %d (pid=%d, %s)\n", \
          __LINE__, (p)->procnum(), (p)->pc->ProcessType())

#define LRC_UNLOCK(p) \
(bLocked ? (MutexUnlock(p), \
    printf("PendLic: unlock at line %d pid=%d\n", \
            __LINE__, (p)->procnum())) : \
    (printf("PendLic: skipped unlock at line %d (pid=%d, %s)\n", \
            __LINE__, (p)->procnum(), (p)->pc->ProcessType())))
#else
#define LRC_LOCK(p)     BOOL bLocked = MutexLockIfNeeded(p)
#define LRC_UNLOCK(p)   if (bLocked) MutexUnlock(p)
#endif

protected:
    HX_MUTEX m_pMutex;
    int m_nLockedBy;
    Process* m_pProc;

    ~LicenseRequesterCallback();
};

class LicenseRequesterListElem : public HXListElem
{
public:
    LicenseRequesterListElem()
	: HXListElem()
    {
    };

    LicenseRequesterListElem(UINT32 ulCID, LicenseRequesterCallback* pCb, 
	IHXCallback* pParentCb, DistLicReqStatus eDLRS, Process* pProc) 
	: HXListElem()
	, m_pCb(pCb)
	, m_pParentCb(pParentCb)
	, m_bCbDispatched(FALSE)
	, m_bDone(FALSE)
	, m_ulCID(ulCID)
	, m_eDLRS(eDLRS)
	, m_pProc(pProc)
    {
	if (m_pCb)
	    m_pCb->AddRef();
	if (m_pParentCb)
	    m_pParentCb->AddRef();
	// fprintf(stderr, "LRLE(%p)::LRLE() -- cid(%lu)\n", this, m_ulCID);
    };

    ~LicenseRequesterListElem()
    {
	// fprintf(stderr, "LRLE(%p)::~LRLE() -- cid(%lu) start\n", this, m_ulCID);
	HX_RELEASE(m_pCb);
	HX_RELEASE(m_pParentCb);
	// fprintf(stderr, "LRLE(%p)::~LRLE() -- cid(%lu) end\n", this, m_ulCID);
    };

#if 0
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	IHXDistributedLicenseRequestListElement methods
     */
    STDMETHOD_(DistLicReqStatus,SetLicenseRequestStatus)(THIS);
    STDMETHOD_(DistLicReqStatus,GetLicenseRequestStatus)(THIS);
#endif

    LicenseRequesterCallback* m_pCb;
    IHXCallback* m_pParentCb;
    BOOL m_bCbDispatched;
    BOOL m_bDone;
    UINT32 m_ulCID;
    DistLicReqStatus m_eDLRS;
    Process* m_pProc;
};

class DistributedLicenseRequester 
{
public:
    // XXX need to decide how to correctly implment the copy constructor
			DistributedLicenseRequester();
		        ~DistributedLicenseRequester();

    DistLicReqStatus 	LicenseRequestStatus(ULONG32 ulClientID, Process* p);
    DistLicReqStatus 	Insert(ULONG32 ulClientID, IHXCallback* pCb, 
			    IHXCallback*& pParentCb, Process* p);
    INT32		Execute(INT32 nNumLicenses, Process* p);
    HX_RESULT		Remove(ULONG32 ulClientID, IHXCallback* pCb,
			    Process* p);
    HX_RESULT		RemoveAll(Process* p);
    UINT32		Count(Process* p);

    inline void
    MutexUnlock(Process* pProc)
    {
        HX_ASSERT(m_nLockedBy == pProc->procnum());
        m_nLockedBy = -1;
        HXMutexUnlock(m_pMutex);
    };

    inline BOOL
    MutexLockIfNeeded(Process* pProc)
    {
        if (m_nLockedBy != pProc->procnum())
        {
            HXMutexLock(m_pMutex, TRUE);
            m_nLockedBy = (UINT32)(pProc->procnum());
            m_pProc = pProc;
            return TRUE;
        }
        return FALSE;
    };

#define PL_ISLOCKED() HX_ASSERT(m_nLockedBy != -1)

#ifdef VERBOSE_REGISTRY_LOCKING
//for debugging
#define PL_LOCK(p) \
BOOL bLocked = MutexLockIfNeeded(p); \
bLocked ? \
  printf("PendLic: lock at line %d pid=%d\n", __LINE__, (p)->procnum()) : \
  printf("PendLic: already locked at line %d (pid=%d, %s)\n", \
          __LINE__, (p)->procnum(), (p)->pc->ProcessType())

#define PL_UNLOCK(p) \
(bLocked ? (MutexUnlock(p), \
    printf("PendLic: unlock at line %d pid=%d\n", \
            __LINE__, (p)->procnum())) : \
    (printf("PendLic: skipped unlock at line %d (pid=%d, %s)\n", \
            __LINE__, (p)->procnum(), (p)->pc->ProcessType())))
#else
#define PL_LOCK(p)     BOOL bLocked = MutexLockIfNeeded(p)
#define PL_UNLOCK(p)   if (bLocked) MutexUnlock(p)
#endif

protected:
    HX_MUTEX            	m_pMutex;
    int                 	m_nLockedBy;
    HXList*			m_pLicenseRequesterList;
    CHXID*			m_pIDs;
    CHXMapLongToObj* 		m_pCIDMap;
    CHXMapPtrToPtr* 		m_pCbMap;
    UINT32                 	m_ulCount;
    Process*			m_pProc;
};

#endif /* _DIST_LIC_REQUESTER_H_ */

/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: servreg.h,v 1.4 2009/05/30 19:11:00 atin Exp $ 
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

#ifndef _SERV_REG_H_
#define _SERV_REG_H_

#include <limits.h>

#include "hxtypes.h"
#include "hxcom.h"

#include "hxmon.h"
#include "regdb_misc.h"
#include "regprop.h"
#include "regkey.h"
#include "hxmap.h"
#include "simple_callback.h"
#include "ihxpckts.h"
#include "hxslist.h"
#include "mutex.h"
#include "server_resource.h"
#include "streamer_info.h"
#include "proc_container.h"
#include "server_engine.h"
#include "proc.h"

class ServRegProperty;
class ServRegDB_node;
class ServRegDB_dict;
class Process;
class WatchList;
class ServRegKey;
class ServerPropWatch;

struct IHXPropWatchResponse;

/*
 * 
 *  Class:
 *
 *	ServerRegistry
 *
 *  Purpose:
 *
 *     this is the server's registry which is going to be in shared memory.
 *  this class acts like a bridge between the interface and the 
 *  implementation. it is a model that i have taken from the book 
 *  "design patterns" by gamma, helm, johnson and vlissides.
 * 
 *     the implementation of this class can now be anything, list, vector, ...
 *  even the formerly known as "server group"'s Dictionary class with some
 *  modifications can be used.
 *
 */
class ServerRegistry : public IHXActivePropUserResponse
{
public:
    // XXX need to decide how to correctly implment the copy constructor
			ServerRegistry(Process* p);
		        ~ServerRegistry();

    // XXX most of these have versions with and without Process pointer arg. 
    // The ones without are typically not properly mutex protected and normally 
    // shouldn't be used except perhaps at server startup.  We need the Process 
    // pointer for registry locking among other things.

    UINT32 		AddComp(const char* szPropName, Process* pProc);

    UINT32 		AddInt(const char* szPropName, const INT32 iValue, Process* pProc);
    HX_RESULT 		GetInt(const char* szPropName, INT32* iValue, Process* pProc);
    HX_RESULT 		GetInt(const UINT32 ulId, INT32* iValue, Process* pProc);
    HX_RESULT 		SetInt(const char* szPropName, const INT32 iValue, Process* pProc);
    HX_RESULT 		SetInt(const UINT32 ulId, const INT32 iValue, Process* pProc);

    UINT32 		AddStr(const char* szPropName, IHXBuffer* pValue, Process* pProc);
    HX_RESULT 		SetStr(const char* szPropName, IHXBuffer* pValue, Process* pProc);
    HX_RESULT 		SetStr(const UINT32 ulId, IHXBuffer* pValue, Process* pProc);
    HX_RESULT 		GetStr(const char* szPropName, REF(IHXBuffer*) pValue, Process* pProc);
    HX_RESULT 		GetStr(const UINT32 ulId,   REF(IHXBuffer*) pValue, Process* pProc);

    UINT32 		AddBuf(const char* szPropName, IHXBuffer* pBuffer, Process* pProc);
    HX_RESULT 		SetBuf(const char* szPropName, IHXBuffer* pBuffer, Process* pProc);
    HX_RESULT 		SetBuf(const UINT32 ulId, IHXBuffer* pBuffer, Process* pProc);
    HX_RESULT 		GetBuf(const char* szPropName, IHXBuffer** ppBuffer, Process* pProc);
    HX_RESULT 		GetBuf(const UINT32 ulId,   IHXBuffer** ppBuffer, Process* pProc);

    UINT32 		AddIntRef(const char* szPropName, INT32* pValue, Process* pProc);

    HX_RESULT 		SetReadOnly(const char* szPropName, BOOL bValue, Process* pProc);
    HX_RESULT 		SetReadOnly(UINT32 ulId, BOOL bValue, Process* pProc);

    UINT32 		Del(const char* szPropName, Process* pProc);
    UINT32 		Del(const UINT32 ulId, Process* pProc);

    HXPropType 	GetType(const char* szPropName, Process* pProc); //XXX
    HXPropType 	GetType(const UINT32 ulId, Process* pProc); //XXX

    HX_RESULT		GetPropList(REF(IHXValues*) pValues, Process* pProc); //XXX
    HX_RESULT		GetPropList(const char* szPropName, REF(IHXValues*) pValues, Process* pProc); //XXX
    HX_RESULT		GetPropList(const UINT32 ulId, REF(IHXValues*) pValues, Process* pProc); //XXX

    HX_RESULT		GetChildIdList(const char* szPropName, REF(UINT32*) pChildIds, REF(UINT32) ulCount, Process* pProc); //XXX
    HX_RESULT		GetChildIdList(const UINT32 ulId, REF(UINT32*) pChildIds, REF(UINT32) ulCount, Process* pProc); //XXX

    HX_RESULT		Copy(const char* szFrom, const char* szTo, Process* pProc); //XXX
    HX_RESULT		SetStringAccessAsBufferById(UINT32 ulId, Process* p); //XXX

    UINT32		FindParentKey(const char* pName, Process* p); //XXX
    UINT32		FindParentKey(const UINT32 id, Process* p); //XXX
    INT32 		Count(Process* pProc); //XXX
    INT32 		Count(const char* pName, Process* p); //XXX
    INT32 		Count(const UINT32 id, Process* p); //XXX

    HX_RESULT           GetPropStatus(const UINT32 ulId,
	                              Process* pProc);
    HX_RESULT           GetPropStatus(const char* szPropName,
	                              Process* pProc);

    HX_RESULT		GetPropName(const UINT32 ulId, 
				    REF(IHXBuffer*) pPropName,
				    Process* pProc); //XXX
    UINT32		GetId(const char* szPropName, Process* pProc); //XXX

    /*
     * Response methods
     */
    HX_RESULT     	AddDone(ServRegDB_dict* pParentDB, ServRegDB_node* pNode);
    HX_RESULT     	SetDone(ServRegDB_node* pNode, ServRegProperty* pProp);
    HX_RESULT     	DeleteDone(ServRegDB_dict* pParentDB, ServRegDB_node* pNode,
				   ServRegProperty* pProp);
    /*
     * Watch specific methods
     */
    UINT32		SetWatch(ServerPropWatch* cb);
    UINT32		SetWatch(const char* pName, ServerPropWatch* cb);
    UINT32		SetWatch(const UINT32 id, ServerPropWatch* cb);

    HX_RESULT		ClearWatch(IHXPropWatchResponse* response,
				   Process* proc);
    HX_RESULT		ClearWatch(const char* pName, 
				   IHXPropWatchResponse* response,
				   Process* proc);
    HX_RESULT		ClearWatch(const UINT32 id, 
				   IHXPropWatchResponse* response,
				   Process* proc);

    HX_RESULT		DeleteWatch(ServRegProperty* p, WListElem* wle);

    /*
     * IHXRegistry2 methods
     */
    HX_RESULT           ModifyInt      (const char*     szName,
                                        INT32           nDelta,
                                        INT32*          pValue,
                                        Process*        proc);
    HX_RESULT           ModifyInt      (const UINT32    id,
                                        INT32           nDelta,
                                        INT32*          pValue,
                                        Process*        proc);
    HX_RESULT           BoundedModifyInt(const char*     szName,
                                        INT32           nDelta,
                                        INT32*          pValue,
                                        Process*        proc,
                                        INT32           nMin=INT_MIN,
                                        INT32           nMax=INT_MAX);
    HX_RESULT           BoundedModifyInt(const UINT32    id,
                                        INT32           nDelta,
                                        INT32*          pValue,
                                        Process*        proc,
                                        INT32           nMin=INT_MIN,
                                        INT32           nMax=INT_MAX);
    HX_RESULT           SetAndReturnInt(const char*     szName,
					INT32           nValue,
                                        INT32*          pOldValue,
                                        Process*        proc);
    HX_RESULT           SetAndReturnInt(const UINT32    id,
					INT32           nValue,
                                        INT32*          pOldValue,
                                        Process*        proc);
    HX_RESULT           GetIntRef      (const char*     szPropName,
                                        INT32**         ppValue,
                                        Process*        pProc);
    HX_RESULT           GetIntRef      (const UINT32    ulId,
                                        INT32**         ppValue,
                                        Process*        pProc);
    UINT32              AddInt64       (const char*     szName,
                                        const INT64     nValue,
                                        Process*        proc);
    HX_RESULT           GetInt64       (const char*     szName,
                                        INT64*          pValue,
                                        Process*        proc);
    HX_RESULT           GetInt64       (const UINT32    ulId,
                                        INT64*          pValue,
                                        Process*        proc);
    HX_RESULT           SetInt64       (const char*     szName,
                                        const INT64     nValue,
                                        Process*        proc);
    HX_RESULT           SetInt64       (const UINT32    id,
                                        const INT64     nValue,
                                        Process*        proc);
    HX_RESULT           ModifyInt64    (const char*     szName,
					INT64           nDelta,
                                        INT64*          pValue,
                                        Process*        proc);
    HX_RESULT           ModifyInt64    (const UINT32    id,
					INT64           nDelta,
                                        INT64*          pValue,
                                        Process*        proc);
    HX_RESULT           SetAndReturnInt64(
                                        const char*     szName,
					INT64           nValue,
                                        INT64*          pOldValue,
                                        Process*        proc);
    HX_RESULT           SetAndReturnInt64(
                                        const UINT32    id,
					INT64           nValue,
                                        INT64*          pOldValue,
                                        Process*        proc);
    UINT32              AddInt64Ref    (const char*     szName,
                                        INT64*          pValue,
                                        Process*        proc);
    HX_RESULT           GetInt64Ref    (const char*     szName,
                                        INT64**         ppValue,
                                        Process*        proc);
    HX_RESULT           GetInt64Ref    (const UINT32    id,
                                        INT64**         ppValue,
                                        Process*        proc);


protected:
    /*
     * methods which are called from inside same name methods with the
     * additional Process* parameter.
     */
    UINT32 		AddComp(const char* szPropName);

    UINT32 		AddInt(const char* szPropName, const INT32 iValue);
    HX_RESULT 		GetInt(const char* szPropName, INT32* pValue);
    HX_RESULT 		GetInt(const UINT32 ulId, INT32* pValue);
    HX_RESULT 		SetInt(const char* szPropName, const INT32 iValue);
    HX_RESULT 		SetInt(const UINT32 ulId, const INT32 iValue);

    UINT32 		AddStr(const char* szPropName, IHXBuffer* pValue);
    HX_RESULT 		SetStr(const char* szPropName, IHXBuffer* pValue);
    HX_RESULT 		SetStr(const UINT32 ulId, IHXBuffer* pValue);
    HX_RESULT 		GetStr(const char* szPropName, REF(IHXBuffer*) pcValue);
    HX_RESULT 		GetStr(const UINT32 ulId,   REF(IHXBuffer*) pcValue);


    UINT32 		AddBuf(const char* szPropName, IHXBuffer* pBuffer);
    HX_RESULT 		SetBuf(const char* szPropName, IHXBuffer* pBuffer);
    HX_RESULT 		SetBuf(const UINT32 ulId, IHXBuffer* pBuffer);
    HX_RESULT 		GetBuf(const char* szPropName, IHXBuffer** ppBuffer);
    HX_RESULT 		GetBuf(const UINT32 ulId,   IHXBuffer** ppBuffer);

    UINT32 		AddIntRef(const char* szPropName, INT32* pValue);
    HX_RESULT           GetIntRef(const char* szPropName, INT32** ppValue);
    HX_RESULT           GetIntRef(const UINT32 ulId, INT32** ppValue);

    HX_RESULT 		SetReadOnly(const char* szPropName, BOOL bValue);
    HX_RESULT 		SetReadOnly(UINT32 ulId, BOOL bValue);

    UINT32 		Del(const char* szPropName);
    UINT32 		Del(const UINT32 ulId);


    HXPropType 	GetType(const char* szPropName); //XXX
    HXPropType 	GetType(const UINT32 ulId); //XXX

    HX_RESULT		GetPropList(REF(IHXValues*) pValues); //XXX
    HX_RESULT		GetPropList(const char* szPropName, REF(IHXValues*) pValues); //XXX
    HX_RESULT		GetPropList(const UINT32 ulId, REF(IHXValues*) pValues); //XXX

    HX_RESULT		GetChildIdList(const char* szPropName, REF(UINT32*) pChildIds, REF(UINT32) ulCount); //XXX
    HX_RESULT		GetChildIdList(const UINT32 ulId, REF(UINT32*) pChildIds, REF(UINT32) ulCount); //XXX

    HX_RESULT		Copy(const char* szFrom, const char* szTo); //XXX
    HX_RESULT		SetStringAccessAsBufferById(UINT32 ulId); //XXX

    UINT32		FindParentKey(const char* pName); //XXX
    UINT32		FindParentKey(const UINT32 id); //XXX
    INT32 		Count() const { return m_iPropCount; } //XXX
    INT32 		Count(const char* pName); //XXX
    INT32 		Count(const UINT32 id); //XXX

    HX_RESULT           GetPropStatus(const UINT32 ulId);
    HX_RESULT           GetPropStatus(const char* szPropName);

    HX_RESULT		GetPropName(const UINT32 ulId, 
				    REF(IHXBuffer*) pPropName); //XXX
    UINT32		GetId(const char* szPropName); //XXX

     // this is so we can unlock the registry if there's a CA inside it
     // while we have it locked
    friend class ServerEngine;

    // so the callback can ensure the registy is locked before accessing it
    class PendingActiveRegChangesCallback;
    friend class PendingActiveRegChangesCallback;

    HX_MUTEX            m_pMutex;
    volatile int        m_nLockedBy;


//#define VERBOSE_REGISTRY_LOCKING

public:
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
            _proc = pProc;
            return TRUE;
        }
        return FALSE;
    };


#define ISLOCKED() HX_ASSERT(m_nLockedBy != -1)


#ifdef VERBOSE_REGISTRY_LOCKING
//for debugging
#define LOCK(p) \
BOOL bLocked = MutexLockIfNeeded(p); \
bLocked ? \
  printf("ServReg: lock at line %d pid=%d\n", __LINE__, (p)->procnum()) : \
  printf("ServReg: already locked at line %d (pid=%d, %s)\n", \
          __LINE__, (p)->procnum(), (p)->pc->ProcessType())

#define UNLOCK(p) \
(bLocked ? (MutexUnlock(p), \
    printf("ServReg: unlock at line %d pid=%d\n", \
            __LINE__, (p)->procnum())) : \
    (printf("ServReg: skipped unlock at line %d (pid=%d, %s)\n", \
            __LINE__, (p)->procnum(), (p)->pc->ProcessType())))
#else
#define LOCK(p)     BOOL bLocked = MutexLockIfNeeded(p)
#define UNLOCK(p)   if (bLocked) MutexUnlock(p)
#endif

protected:
    class ActivePropUserInfo
    {
    public:
	ActivePropUserInfo(IHXActivePropUser* pUser, Process* p)
	    : m_pUser(pUser)
	    , m_pProc(p)
	{
	    pUser->AddRef();
	}

	~ActivePropUserInfo()
	{
	    HX_RELEASE(m_pUser);
	    m_pProc = 0;
	}
	IHXActivePropUser* m_pUser;
	Process* m_pProc;
    };

    class PendingActiveRegChangesCallback : public SimpleCallback
    {
    public:
	typedef enum
	{
	    PACT_STR,
	    PACT_INT,
	    PACT_BUF,
	    PACT_DELETE
	} PAC_TYPE;

	typedef enum
	{
	    PACR_NOTIFY,
	    PACR_RESPONSE
	} PAC_REASON;

	PendingActiveRegChangesCallback()
	    : m_pResponse(0)
	    , m_pPendingBuffer(0)
	    , m_pProc(0)
	    , m_keyname(0)
	    , m_pUser(0)
	    , m_res(HXR_FAIL)
	    , m_ulNumResInfo(0)
	{
	}
	~PendingActiveRegChangesCallback()
	{
	    HX_RELEASE(m_pPendingBuffer);
	    HX_RELEASE(m_pResponse);
	    HX_RELEASE(m_pUser);
	    delete[] m_keyname;
	}

	static PendingActiveRegChangesCallback* CreateResponse(
	    IHXActivePropUserResponse* presp,
	    PAC_TYPE pt,
	    UINT32 ul, IHXBuffer* pBuffer,
	    const char* keyname,
	    UINT32 num_outstanding,
	    Process* p)
	{
	    PendingActiveRegChangesCallback* cb =
		new PendingActiveRegChangesCallback;
	    cb->m_PacReason = PACR_RESPONSE;
	    cb->m_PacType = pt;
	    cb->m_pResponse = presp;
	    cb->m_pResponse->AddRef();
	    cb->m_ulPendingInt = ul;
	    cb->m_pPendingBuffer = pBuffer;
	    if(pBuffer)
	    {
		pBuffer->AddRef();
	    }
	    cb->m_keyname = new char[strlen(keyname) + 1];
	    strcpy(cb->m_keyname, keyname);
	    cb->m_ulSetsOutstanding = num_outstanding;
	    cb->m_pProc = p;
	    return cb;
	}

	static PendingActiveRegChangesCallback* CreateNotify(
	    IHXActivePropUserResponse* presp,
	    IHXActivePropUser* pusr,
	    PAC_TYPE pt,
	    UINT32 ul,
	    IHXBuffer* pBuffer,
	    const char* keyname)
	{
	    PendingActiveRegChangesCallback* cb =
		new PendingActiveRegChangesCallback;
	    cb->m_PacReason = PACR_NOTIFY;
	    cb->m_PacType = pt;
	    cb->m_pResponse = presp;
	    cb->m_pResponse->AddRef();
	    cb->m_pUser = pusr;
	    cb->m_pUser->AddRef();
	    cb->m_ulPendingInt = ul;
	    cb->m_pPendingBuffer = pBuffer;
	    if(pBuffer)
	    {
		pBuffer->AddRef();
	    }
	    cb->m_keyname = new char[strlen(keyname) + 1];
	    strcpy(cb->m_keyname, keyname);
	    return cb;
	}

	void 			func(Process* p);
	void			AddResInfo(IHXBuffer* pInfo[],
				UINT32 ulNumInfo)
	{
	    UINT32 i;
	    for (i = 0; i < ulNumInfo; i++)
	    {
		m_res_info[m_ulNumResInfo] =
		    pInfo[i];
		m_res_info[m_ulNumResInfo]->AddRef();
	    }
	    m_ulNumResInfo += ulNumInfo;
	}

	IHXActivePropUserResponse*	m_pResponse;
	IHXActivePropUser*		m_pUser;
	PAC_TYPE		m_PacType;
	PAC_REASON		m_PacReason;
	UINT32		m_ulPendingInt;
	IHXBuffer*		m_pPendingBuffer;
	char*		m_keyname;
	Process*		m_pProc;
	HX_RESULT		m_res;
	UINT32			m_ulSetsOutstanding;
	IHXBuffer*		m_res_info[256];
	UINT32			m_ulNumResInfo;
    };

    ServRegDB_node*		_addComp(ServRegKey* pKey, 
	                                 char* szPropName, 
					 ServRegDB_dict* pOwnerDB);

    ServRegDB_node*		_addInt(ServRegKey* pKey, 
	                                char* szPropName, 
					INT32 iValue, 
				        ServRegDB_dict* pOwnerDB);

    ServRegDB_node*		_addBuf(ServRegKey* pKey, 
	                                char* szPropName, 
					IHXBuffer* pBuffer, 
				        ServRegDB_dict* pOwnerDB, 
				        HXPropType type = PT_BUFFER);

    ServRegDB_node*		_addIntRef(ServRegKey* pKey, 
	                                   char* szPropName, INT32* pValue, 
				           ServRegDB_dict* pOwnerDB);

    HX_RESULT		_find(ServRegDB_node** ppNode, ServRegProperty** ppProp, const char* szPropName);

    void    		_FillListList(CHXSimpleList*, const char*);
    void    		_AppendListList(CHXSimpleList*, const char*);

    UINT32		_Del(ServRegDB_dict* pOwnerDB, ServRegDB_node* pNode, ServRegProperty* pProp, UINT32 ulId);
    HX_RESULT		_del(ServRegDB_dict* pOwnerDB);
    HX_RESULT		_getPropList(ServRegDB_dict* pOwnerDB, REF(IHXValues*) pValues);
    HX_RESULT		_getChildIdList(ServRegDB_dict* pOwnerDB, REF(UINT32*) ulChildIds, REF(UINT32) ulCount);
    HX_RESULT		_setReadOnly(ServRegProperty* pProp, BOOL bValue);
    UINT32		_buildSubstructure4Prop(const char* pFailurePoint,
			    			const char* pProp);

    void 		_dispatchParentCallbacks(ServRegDB_dict*, ServRegDB_node*,
						 ServRegDB_Event);
    void		_dispatchCallbacks(ServRegDB_node*, ServRegProperty*, ServRegDB_Event);
    HX_RESULT		_clearWatch(IHXPropWatchResponse*);
    HX_RESULT		_clearWatch(ServRegProperty*, IHXPropWatchResponse*);

    ServRegDB_dict*  		m_pRootDB;
    CHXID*			m_pIdTable;
    INT32			m_iPropCount;		// num of elems in the DB
    
    Process*                    _proc;
    WatchList*			m_pRootWatchList;

public:
    /*
     * Stuff for IHXActiveRegistry.
     */
    CHXMapStringToOb			m_ActiveMap2;

    CHXMapStringToOb			m_PendingMap2;

    /*
     * Alas, we have to introduce com for the active reg stuff.
     * I would just like to say that I am very sorry.
     */

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * pseudo IHXActiveRegistry methods.
     */

    /************************************************************************
    * IHXActiveRegistry::SetAsActive
    *
    *     Method to set prop pName to active and register pUser as
    *   the active prop user.
    */
    STDMETHOD(SetAsActive)    (const char* pName,
			       IHXActivePropUser* pUser,
			       Process* proc);

    /************************************************************************
    * IHXActiveRegistry::SetAsInactive
    *
    *	Method to remove an IHXActiveUser from Prop activation.
    */ //XXXPM may need proc here.
    STDMETHOD(SetAsInactive)  (const char* pName,
			       IHXActivePropUser* pUser,
			       Process* pProc);

    /************************************************************************
    * IHXActiveRegistry::IsActive
    *
    *     Tells if prop pName has an active user that must be queried to
    *   change the value, or if it can just be set.
    */
    STDMETHOD_(BOOL, IsActive)	(const char* szPropName, Process* pProc);
    BOOL IsDeleteActive(const char* szPropName, Process* pProc);

    /*
     * These are like IMRAActiveUserMethods, but they take a Process too.
     */

    /************************************************************************
    *    Async request to set int pName to ul.
    */
    STDMETHOD(SetActiveInt) (const char* pName,
			    UINT32 ul,
			    IHXActivePropUserResponse* pResponse,
			    Process* p);

    /************************************************************************
    *    Async request to set string pName to string in pBuffer.
    */
    STDMETHOD(SetActiveStr) (const char* pName,
			    IHXBuffer* pBuffer,
			    IHXActivePropUserResponse* pResponse,
			    Process* p);

    /************************************************************************
    *    Async request to set buffer pName to buffer in pBuffer.
    */
    STDMETHOD(SetActiveBuf)	(const char* pName,
				IHXBuffer* pBuffer,
				IHXActivePropUserResponse* pResponse,
				Process* p);

    /************************************************************************
    * IHXActivePropUser::DeleteActiveProp
    *
    *	Async request to delete the active property.
    */
    STDMETHOD(DeleteActiveProp)	(const char* pName,
				IHXActivePropUserResponse* pResponse,
				Process* p);

    /*
     * IHXActivePropUserResponse methods.
     */

    /************************************************************************
    * Called with status result on completion of set request.
    */
    STDMETHOD(SetActiveIntDone)   (THIS_ HX_RESULT res, const char* pName, UINT32 ul,
	IHXBuffer* pInfo[], UINT32 ulNumInfo);
    STDMETHOD(SetActiveStrDone)	  (THIS_ HX_RESULT res, const char* pName, IHXBuffer* pBuffer,
	IHXBuffer* pInfo[], UINT32 ulNumInfo);
    STDMETHOD(SetActiveBufDone)	  (THIS_ HX_RESULT res, const char* pName, IHXBuffer* pBuffer,
	IHXBuffer* pInfo[], UINT32 ulNumInfo);
    STDMETHOD(DeleteActivePropDone) (THIS_ HX_RESULT res, const char* pName,
	IHXBuffer* pInfo[], UINT32 ulNumInfo);
};




#endif // _SERV_REG_H_

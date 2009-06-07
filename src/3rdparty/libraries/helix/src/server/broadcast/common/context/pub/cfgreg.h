/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: cfgreg.h,v 1.2 2003/01/23 23:42:48 damonlan Exp $ 
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

#ifndef _CFG_REG_H_
#define _CFG_REG_H_

#include "hxtypes.h"
#include "hxcom.h"
#include "hxmon.h"
#include "hxengin.h"
#include "db_misc.h"
#include "commreg.h"
#include "hxmap.h"
#include "ihxpckts.h"
#include "hxslist.h"

class Property;
class DB_node;
class DB_implem;
class Process;
class WatchList;
class Key;
class ServerPropWatch;

struct IHXPropWatchResponse;
struct IHXRegistry;
class  CommonRegistry;

class ConfigRegistry : public IHXRegistry,
		       public IHXActiveRegistry,
		       public IHXActivePropUserResponse
{
 protected:
    
    LONG32		m_lRefCount;
    CommonRegistry*	m_pPropDB;
    IUnknown*		m_pContext;

 public:

    CHXMapStringToOb     m_ActiveMap;
    CHXMapStringToOb     m_PendingMap;

    ConfigRegistry();
    ~ConfigRegistry();

    void Init    (IUnknown* pContext);
    void Close   (void);

    BOOL IsDeleteActive(const char* pName);

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
			         REFIID riid,
				 void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	IHXRegistry methods
     */

  
    STDMETHOD(CreatePropWatch)		(THIS_
					 REF(IHXPropWatch*)	pPropWatch);
  
    STDMETHOD_(UINT32, AddComp)		(THIS_
					 const char*	pName);

    STDMETHOD_(UINT32, AddInt)		(THIS_
					 const char*	pName, 
					 const INT32	iValue);

    STDMETHOD(GetIntByName)		(THIS_
					 const char*	pName,
					 REF(INT32)	nValue) const;

    STDMETHOD(GetIntById)		(THIS_
					 const UINT32	id,
					 REF(INT32)	nValue) const;

    STDMETHOD(SetIntByName)		(THIS_
					 const char*	pName, 
					 const INT32	iValue);
    STDMETHOD(SetIntById)		(THIS_
					 const UINT32	id,
					 const INT32	iValue);

    STDMETHOD_(UINT32, AddStr)		(THIS_
					 const char*	pName, 
					 IHXBuffer*	pValue);

    STDMETHOD(GetStrByName)		(THIS_
					 const char*	    pName,
					 REF(IHXBuffer*)    pValue) const;
    STDMETHOD(GetStrById)		(THIS_
					 const UINT32	    id,
					 REF(IHXBuffer*)    pValue) const;

    STDMETHOD(SetStrByName)		(THIS_
					 const char*	pName, 
					 IHXBuffer*	pValue);
    STDMETHOD(SetStrById)		(THIS_
					 const UINT32	id,
					 IHXBuffer*	pValue);

    STDMETHOD_(UINT32, AddBuf)		(THIS_
					 const char*	pName, 
					 IHXBuffer*	pValue);

    STDMETHOD(GetBufByName)		(THIS_
					 const char*	pName,
					 REF(IHXBuffer*)	ppValue) const;

    STDMETHOD(GetBufById)		(THIS_
					 const UINT32	id,
					 REF(IHXBuffer*)	ppValue) const;

    STDMETHOD(SetBufByName)		(THIS_
					 const char*	pName, 
					 IHXBuffer*	pValue);
    STDMETHOD(SetBufById)		(THIS_
					 const UINT32	id,
					 IHXBuffer*	pValue);
 
    STDMETHOD_(UINT32, AddIntRef)	(THIS_
					 const char*	pName, 
					 INT32*	 	piValue);
 
    STDMETHOD_(UINT32, DeleteByName)	(THIS_
					 const char*	pName);
    STDMETHOD_(UINT32, DeleteById)	(THIS_
					 const UINT32	id);

    STDMETHOD_(HXPropType, GetTypeByName)	(THIS_
						 const char* pName) const;
    STDMETHOD_(HXPropType, GetTypeById)	(THIS_
						 const UINT32 id) const;

    STDMETHOD_(UINT32, FindParentIdByName)	(THIS_
						 const char* pName) const;
    STDMETHOD_(UINT32, FindParentIdById)	(THIS_
						 const UINT32 id) const;

    STDMETHOD(GetPropName)			(THIS_
						 const UINT32 ulId,
						 REF(IHXBuffer*) pName) const;

    STDMETHOD_(UINT32, GetId)			(THIS_
						 const char* pName) const;

    STDMETHOD(GetPropListOfRoot) 	(THIS_
					 REF(IHXValues*) pValues) const;
  
    STDMETHOD(GetPropListByName) 	(THIS_
					 const char* pName,
					 REF(IHXValues*) pValues) const;
    STDMETHOD(GetPropListById) 	 	(THIS_
					 const UINT32 id,
					 REF(IHXValues*) pValues) const;

    STDMETHOD_(INT32, GetNumPropsAtRoot)	(THIS) const;

    STDMETHOD_(INT32, GetNumPropsByName)(THIS_
					 const char*	pName) const;
    STDMETHOD_(INT32, GetNumPropsById)	(THIS_
					 const UINT32	id) const;
    
    /*
     * IHXActiveRegistry methods.
     */

    STDMETHOD(SetAsActive)    (THIS_
			       const char* pName,
			       IHXActivePropUser* pUser);

    STDMETHOD(SetAsInactive)  (THIS_
			       const char* pName,
			       IHXActivePropUser* pUser);

    STDMETHOD_(BOOL, IsActive)	(THIS_
				 const char* pName);

    STDMETHOD(SetActiveInt) (THIS_
			     const char* pName,
			     UINT32 ul,
			     IHXActivePropUserResponse* pResponse);
     
    STDMETHOD(SetActiveStr) (THIS_
			     const char* pName,
			     IHXBuffer* pBuffer,
			     IHXActivePropUserResponse* pResponse);
     
    STDMETHOD(SetActiveBuf)	(THIS_
				 const char* pName,
				 IHXBuffer* pBuffer,
				 IHXActivePropUserResponse* pResponse);
     
    STDMETHOD(DeleteActiveProp)	(THIS_
				 const char* pName,
				 IHXActivePropUserResponse* pResponse);

    /*
     * IHXActivePropUserResponse methods.
     */

     STDMETHOD(SetActiveIntDone)   (THIS_
				    HX_RESULT res,
				    const char* pName,
				    UINT32 ul,
				    IHXBuffer* pInfo[],
				    UINT32 ulNumInfo);

    STDMETHOD(SetActiveStrDone)	  (THIS_
				    HX_RESULT res,
				    const char* pName,
				    IHXBuffer* pBuffer,
				    IHXBuffer* pInfo[],
				    UINT32 ulNumInfo);

    STDMETHOD(SetActiveBufDone)	  (THIS_
				    HX_RESULT res,
				    const char* pName,
				    IHXBuffer* pBuffer,
				    IHXBuffer* pInfo[],
				    UINT32 ulNumInfo);

    STDMETHOD(DeleteActivePropDone) (THIS_
				    HX_RESULT res,
				    const char* pName,
				    IHXBuffer* pInfo[],
				    UINT32 ulNumInfo);

 protected:
    class ActivePropUserInfo
    {
    public:
	ActivePropUserInfo(IHXActivePropUser* pUser)
	    : m_pUser(pUser)
	{
	    pUser->AddRef();
	}

	~ActivePropUserInfo()
	{
	    HX_RELEASE(m_pUser);
	}
	IHXActivePropUser* m_pUser;
    };

    class PendingActiveRegChangesCallback : public IHXCallback
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
	    , m_keyname(0)
	    , m_pUser(0)
	    , m_res(HXR_FAIL)
	    , m_ulNumResInfo(0)
	    , m_lRefCount(0)
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
	    UINT32 num_outstanding)
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

	/*
	  *	IUnknown methods
	  */
	STDMETHOD(QueryInterface)	(THIS_
					 REFIID riid,
					 void** ppvObj);
	
	STDMETHOD_(ULONG32,AddRef)	(THIS);
	
	STDMETHOD_(ULONG32,Release)	(THIS);

	/*    IHXCallback    */
	STDMETHODIMP Func();
	
	LONG32                          m_lRefCount;
	IHXActivePropUserResponse*	m_pResponse;
	IHXActivePropUser*		m_pUser;
	PAC_TYPE		m_PacType;
	PAC_REASON		m_PacReason;
	UINT32		m_ulPendingInt;
	IHXBuffer*		m_pPendingBuffer;
	char*		m_keyname;
	HX_RESULT		m_res;
	UINT32			m_ulSetsOutstanding;
	IHXBuffer*		m_res_info[256];
	UINT32			m_ulNumResInfo;
    };

    WatchList*			_watchList;
    int				wCount;		// count num of Watches
    void    _FillListList(CHXSimpleList*, const char*);
    void    _AppendListList(CHXSimpleList*, const char*);
};

#endif // _CFG_REG_H_




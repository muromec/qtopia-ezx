/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: altserverproxycfg.h,v 1.3 2004/05/03 19:02:48 tmarshall Exp $ 
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
 *  altserverproxycfg.h
 *  
 *  Player reconnect config var handler
 */

#ifndef _ALTSERVERPROXYCFG_H_
#define _ALTSERVERPROXYCFG_H_

#include "altserv.h"
#include "simple_callback.h"

class Alternates;
class CHXSimpleList;

enum ServerProxyEnum
{
    ALT_NONE,
    ALT_SERVER,
    ALT_PROXY,
    ALT_DPATH
};


class OnWrapper : public SimpleCallback
{
public:
    /*
     * SimpleCallback methods
     */
    void		func(Process* proc);

    AltServerProxy*		m_pResp; 
    HX_ALTERNATES_MOD_FLAG	m_type;
};



class AltServerProxyConfigHandler : public IHXCallback
{
public:
    AltServerProxyConfigHandler
	(Process* pProc, ServerRegistry* pRegistry);

    /************************************************************************
     *  IUnknown COM Interface Methods                          ref:  hxcom.h
     */
    STDMETHOD (QueryInterface ) (REFIID ID, void** ppInterfaceObj);
    STDMETHOD_(UINT32, AddRef ) (THIS);
    STDMETHOD_(UINT32, Release) (THIS);

    /************************************************************************
     *  IHXCallback COM Interface Methods	              ref: hxengin.h
     */
    STDMETHOD(Func)		(THIS);

    /**************************************************************************
     *	called from AltServerProxy
     */
    HX_RESULT Init		(AltServerProxy* pResp);
    HX_RESULT ClearResponse	(AltServerProxy* pResp);
    HX_RESULT GetAllAltServers	(REF(IHXValues*) pAlt);
    HX_RESULT GetAllAltProxies	(REF(IHXValues*) pAlt);
    HX_RESULT GetAllDynamicPaths(REF(IHXValues*) pDPath);

    HX_RESULT			SetResponse(AltServerProxy* pResp);
    void			GetReadyToReload(void);
    HX_RESULT			Init(void);
private:
    ~AltServerProxyConfigHandler();

    HX_RESULT			InitServer(void);
    HX_RESULT			InitProxy(void);
    HX_RESULT			InitDynamicPath(void);
    

    HX_RESULT	LoadAll(ServerProxyEnum type);
    void	DispatchResponse(HX_ALTERNATES_MOD_FLAG type);


    HX_RESULT	CreateAlternates(const char* pName, UINT32 ulRegID, ServerProxyEnum type);
    HX_RESULT	CreateAlternateString(const char* pKey, IHXValues* pAltNames, ServerProxyEnum type);
    HX_RESULT	AccumulateAltStr
	(UINT32 ulRegID, CHXSimpleList* pList, REF(UINT32) ulTotalSize, ServerProxyEnum type);


private:
    UINT32		m_ulRefCount;
    
    Process*		m_pProc;
    ServerRegistry*	m_pRegistry;

    Alternates*		m_pServer;
    Alternates*		m_pProxy;
    Alternates*		m_pDPath;

    UINT32		m_hCB;


    class AltResp : public HXListElem
    {
    public:
	AltServerProxy*	    m_pResp; 
	int		    m_iProcNum;	
    };
    HXList*		m_pRespList;
};



/*
 * holds Alternates for a given prefix/rule
 */
class Alternates : public IHXPropWatchResponse
{
public:
    Alternates(Process* pProc, 
	       ServerRegistry* pRegistry, 
	       AltServerProxyConfigHandler* pCfgHandler,
	       ServerProxyEnum type);

    HX_RESULT	GetAlternate(const char* pName, REF(IHXBuffer*)pAltStr);
    HX_RESULT   SetProxyDefaultAltStrVals();
    UINT32	SetWatch(UINT32 ulRegID);
    UINT32	SetWatch(const char* pName);
    
    /************************************************************************
     *  IUnknown COM Interface Methods                          ref:  hxcom.h
     */
    STDMETHOD (QueryInterface ) (REFIID ID, void** ppInterfaceObj);
    STDMETHOD_(UINT32, AddRef ) (THIS);
    STDMETHOD_(UINT32, Release) (THIS);


    /************************************************************************
     *  IHXPropWatchResponse COM Interface Methods            ref:  hxmon.h
     */
    STDMETHOD(AddedProp)	(const UINT32		id, 
				 const HXPropType	propType, 
				 const UINT32		ulParentID);
    STDMETHOD(ModifiedProp)	(const UINT32		id,
				 const HXPropType   	propType,
				 const UINT32		ulParentID);
    STDMETHOD(DeletedProp)	(const UINT32		id,
				 const UINT32		ulParentID);

private:	
    ~Alternates();
   
    HX_RESULT LoadAll(void);    
    HX_RESULT CreateAltString(const char* pName, UINT32 ulRegID);
    HX_RESULT CreateDPathVals(const char* pName, UINT32 ulRegID);
    
private:
    UINT32		m_ulRefCount;	

    Process*		m_pProc;
    ServerRegistry*	m_pRegistry;
    ServerProxyEnum	m_type;

    CHXMapStringToOb*   m_pAlternateMap;

    AltServerProxyConfigHandler* m_pCfgHandler;

    class WatchListElm : public HXListElem
    {
    public:	
	UINT32	    m_ulHandle;	
    };
    HXList*		m_pWatchList;

public:
    IHXValues*		m_pAltStrVals;
    BOOL		m_bIsValid;
};

#endif 

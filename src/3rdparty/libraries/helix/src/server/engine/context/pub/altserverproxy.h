/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: altserverproxy.h,v 1.3 2004/05/03 19:02:48 tmarshall Exp $ 
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

#ifndef _ALTSERVERPROXY_H_
#define _ALTSERVERPROXY_H_
 
#include "altserv.h"

class AltServerProxy : public IHXAlternateServerProxy
{
public:
    AltServerProxy(Process* pProc);
    HX_RESULT SetCfgHandler(AltServerProxyConfigHandler* pCfgHandler);

    STDMETHOD (QueryInterface)	(THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);


    /**************************************************************************
     *	IHXAlternateServerProxy methods			 ref: altserv.h
     */
    STDMETHOD(Init)		(THIS_
				 IHXAlternateServerProxyResponse* pResp);				 
    STDMETHOD(ClearResponse)	(THIS_
				 IHXAlternateServerProxyResponse* pResp);    
    STDMETHOD_(BOOL,IsEnabled)	(THIS_
				 const char* pURL);
    STDMETHOD(GetAltServerProxy)(THIS_
				 const char* pURL,
				 REF(IHXBuffer*) pAltServ,
				 REF(IHXBuffer*) pAltProx);				 
    STDMETHOD(GetAltServers)	(THIS_
				 const char* pURL, 
				 REF(IHXBuffer*) pAlt);
    STDMETHOD(GetAltProxies)	(THIS_
				 const char* pURL, 
				 REF(IHXBuffer*) pAlt);

    /**************************************************************************
     *	Called from AltServerProxyConfigHandler
     */
    HX_RESULT OnModifiedEntry	(HX_ALTERNATES_MOD_FLAG type);
    

private:
    ~AltServerProxy();

#if 0
    void TestServerProxy();
    void TestServer();
    void TestProxy();
#endif

    HX_RESULT FindAltServers(const char* pRsrc, REF(IHXBuffer*)pAlt);
    HX_RESULT FindAltProxies(const char* pAddr, REF(IHXBuffer*)pAlt);

    void CreateMap(IHXValues* pVal, CHXMapStringToOb* pMap);
    void CleanMap(CHXMapStringToOb* pMap);
    

    void AddResp(HXList* pList, IHXAlternateServerProxyResponse* pResp);
    
    UINT32			    m_ulRefCount;
    Process*			    m_pProc;
    AltServerProxyConfigHandler*    m_pCfgHandler;


    CHXMapStringToOb*		    m_pServerAltMap;     
    CHXMapStringToOb*		    m_pProxyAltMap; 
    IHXValues*			    m_pDPath;

    CHXMapPtrToPtr*		    m_pRespMap;


    
};


#endif

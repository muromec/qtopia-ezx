/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: allowance_wrap.h,v 1.7 2007/05/10 18:44:48 seansmith Exp $ 
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

#ifndef _ALLOWANCE_WRAP_H_
#define _ALLOWANCE_WRAP_H_

class AllowanceWrapper;

typedef enum
{
    SINK_RELEASE,
    SINK_ON_URL,
    SINK_ON_BEGIN,
    SINK_ON_STOP,
    SINK_ON_PAUSE
} SinkCallbackType;

typedef enum
{
    RESPONSE_ON_URL_DONE,
    RESPONSE_ON_BEGIN_DONE,
    RESPONSE_ON_STOP_DONE,
    RESPONSE_ON_PAUSE_DONE
} ResponseCallbackType;

typedef enum
{
    CONTROLLER_PAUSE,
    CONTROLLER_RESUME,
    CONTROLLER_DISCONNECT,
    CONTROLLER_ALERT_AND_DISCONNECT,
    CONTROLLER_REDIRECT,
    CONTROLLER_HOST_REDIRECT,
    CONTROLLER_NETWORK_REDIRECT,
    CONTROLLER_PROXY_REDIRECT
} ControllerCallbackType;

class AllowanceNetworkRedirectWrapperParameters
{
public:
    IHXBuffer* m_pBuffer;
    UINT32 	m_ulSecsFromNow;
};

class AllowanceHostRedirectWrapperParameters
{
public:
    IHXBuffer* m_pHost;
    UINT32 	m_unPort;
};

class AllowanceWrapperSinkCreateCallback : public SimpleCallback
{
public:
    AllowanceWrapperSinkCreateCallback(AllowanceWrapper* pWrapper,
	IHXPlayerConnectionAdviseSinkManager* pPCAdviseSinkManager)
    : m_pWrapper(pWrapper)
    , m_pPCAdviseSinkManager(pPCAdviseSinkManager)
    {};

    void func(Process* p);

    AllowanceWrapper*			m_pWrapper;
    IHXPlayerConnectionAdviseSinkManager*
					m_pPCAdviseSinkManager;
};

class AllowanceWrapperSinkCallback : public SimpleCallback
{
public:
    AllowanceWrapperSinkCallback(AllowanceWrapper* pWrapper,
                                 SinkCallbackType Type)
    : m_pWrapper(pWrapper)
    , m_Type(Type)
    {};

    void func(Process* p);

    AllowanceWrapper*			m_pWrapper;
    SinkCallbackType			m_Type;
};

class AllowanceWrapperResponseCallback : public SimpleCallback
{
public:
    AllowanceWrapperResponseCallback(AllowanceWrapper* pWrapper,
                                     ResponseCallbackType Type,
                                     HX_RESULT status)
    : m_pWrapper(pWrapper)
    , m_Type(Type)
    , m_status(status)
    {};

    void func(Process* p);

    AllowanceWrapper*			m_pWrapper;
    ResponseCallbackType		m_Type;
    HX_RESULT				m_status;
};

class AllowanceWrapperControllerCallback : public SimpleCallback
{
public:
    AllowanceWrapperControllerCallback(IHXPlayerController* pPlayerController,
                                       ControllerCallbackType Type,
                                       void* pParameter);
    ~AllowanceWrapperControllerCallback();

    void func(Process* p);

    IHXPlayerController*		m_pPlayerController;
    ControllerCallbackType		m_Type;
    void*				m_pParameter;
};

class AllowanceWrapper : public AllowanceMgr,
                         public IHXPlayerController,
                         public IHXPlayerControllerProxyRedirect
{
public:
    AllowanceWrapper(ClientSession*,
                     Process*,
                     Process*,
                     const char*,
                     IHXPlayerConnectionAdviseSinkManager*);

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    STDMETHOD(AllowanceMgrDone) (THIS);

    // *** IHXPlayerConnectionAdviseSink methods ***

    STDMETHOD (OnConnection)	(THIS_
				IHXPlayerConnectionResponse* pResponse);
    STDMETHOD (SetPlayerController)
				(THIS_
				IHXPlayerController* pPlayerController);
    STDMETHOD (SetRegistryID)   (THIS_
				UINT32 ulPlayerRegistryID);
    STDMETHOD (OnURL)		(THIS_
				IHXRequest* pRequest);
    STDMETHOD (OnBegin)		(THIS);
    STDMETHOD (OnPause)		(THIS);
    STDMETHOD (OnStop)		(THIS);
    STDMETHOD (OnDone)		(THIS);


    // *** IHXPlayerConnectionResponse methods ***

    STDMETHOD (OnConnectionDone)    (HX_RESULT status);
    STDMETHOD (OnURLDone)	    (HX_RESULT status);
    STDMETHOD (OnBeginDone)	    (HX_RESULT status);
    STDMETHOD (OnStopDone)	    (HX_RESULT status);
    STDMETHOD (OnPauseDone)	    (HX_RESULT status);

    // *** IHXPlayerController methods ***

    STDMETHOD(Pause)		(THIS);
    STDMETHOD(Resume)		(THIS);
    STDMETHOD(Disconnect)	(THIS);
    STDMETHOD(AlertAndDisconnect)
				(THIS_
				IHXBuffer* pAlert);
    STDMETHOD(HostRedirect)     (THIS_ 
				IHXBuffer* pHost, 
				UINT16 nPort);
    STDMETHOD(NetworkRedirect)	(THIS_ 
				IHXBuffer* pURL,
				UINT32 ulSecsFromNow);
    STDMETHOD(NetworkProxyRedirect)	(THIS_ 
				IHXBuffer* pURL);
    STDMETHOD(Redirect)		(THIS_ 
				IHXBuffer* pPartialURL);

    void DispatchSinkCallback	(SinkCallbackType	Type);
    void DispatchResponseCallback
				(ResponseCallbackType	Type,
				HX_RESULT		status);
    void DispatchControllerCallback
				(ControllerCallbackType	Type,
				void*			pParameter);
    HX_RESULT HandleOnURL	();
    void HandleOnURLDone	(HX_RESULT		status);

    // added as part of Startup Optimization work
    STDMETHOD_ (void, PrintDebugInfo)  (THIS);
    STDMETHOD_ (BOOL, AcceptMountPoint)  (THIS_ const char* pszMountPoint,
                                                INT32 uLen);

private:
    Process*			m_pPluginProc;
    ClientSession*		m_pSession;
    IHXRequest*		m_pRequest;
    UINT32			m_ulRegistryID;
    IHXPlayerConnectionAdviseSinkManager*
				m_pPCAdviseSinkManager;

    char*                       m_pszMountPoint;

    virtual ~AllowanceWrapper();


    friend class AllowanceWrapperSinkCreateCallback;
    friend class AllowanceWrapperSinkCallback;
};

#endif

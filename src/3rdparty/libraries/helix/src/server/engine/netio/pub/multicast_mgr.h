/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: multicast_mgr.h,v 1.11 2007/03/08 00:17:43 tknox Exp $ 
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

#ifndef _MULTICAST_MGR_H_
#define _MULTICAST_MGR_H_

#include "sink.h"
#include "hxmutexlock.h"
#include "base_callback.h"

class MulticastManager;
class CHXMapStringToOb;
struct CHXMapLongToObj;
class LiveSource;
class PacketFlowWrapper;
class ASMRuleBook;
class Transport;
class IHXRegistry;
struct IHXValues;
struct IHXPSourceControl;
struct IHXPacketFlowControl;
class Config;
class UDPIO;
class Transport;
class RTSPServerMulticastTransport;
class MulticastAddressAllocator;
struct IHXSapManager;
struct IHXStreamDescription;
struct IHXSessionStats;
class IHXDataConvertResponse;
class TransportStreamHandler;

enum MMProtocols
{
    MMP_PNA_AUDIO, //depreciated
    MMP_PNA_RAW,   //depreciated
    MMP_RTSP_RDT
};

class MulticastSession : public IHXPSinkControl
{
public:
    	    	    	    MulticastSession(
						IUnknown* pContext,
						const char* pFileName,
						enum MMProtocols eProtocol,
						MulticastManager* pMgr
					    );
    	    	    	    ~MulticastSession();
    HX_RESULT		    GetMulticastInfo(
						UINT16  ulRuleNumber,
						UINT32& ulAddress,
						UINT16&	unPort,
						UINT32& ulFECAddress,
						UINT16&	unFECPort
					    );

    void  MulticastSessionSetup(PacketFlowWrapper* pPacketFlowWrap,
                                IHXPSourceControl* pSourceControl,
                                IHXSessionStats* pSessionStats,
				UINT16 unNumStreams,
				REF(IHXPacketFlowControl*) pSessionControl,
				IHXDataConvertResponse* pDataConvert);

    Transport*	GetRTSPTransport(UINT32 ulStreamCount,
				Transport* pTrans);

    HX_RESULT		    SubscriptionDone(BYTE*       bRuleOn,
                                             REF(UINT32) ulSourcePort,
                                             REF(UINT32) ulPort,
                                             REF(UINT32) ulAddr,
                                             REF(TransportStreamHandler*)pHandler);
    STDMETHOD(QueryInterface)   (THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32, AddRef) (THIS);

    STDMETHOD_(ULONG32, Release)(THIS);

    STDMETHOD(InitDone)         (THIS_
				HX_RESULT               ulStatus);

    STDMETHOD(FileHeaderReady)  (THIS_
				HX_RESULT               ulStatus,
				IHXValues*             pHeader);

    STDMETHOD(StreamHeaderReady)(THIS_
				HX_RESULT               ulStatus,
				IHXValues*             pHeader);

    STDMETHOD(StreamDone)       (THIS_
				UINT16                  unStreamNumber);

    STDMETHOD(SeekDone)         (THIS_
				HX_RESULT               ulStatus);


    void	AddCount();
    void	ReleaseCount();
    HX_RESULT	GetFileName(REF(char*) pFileName);
    IHXDataConvertResponse* GetTransportConverter() { return m_pTransportDataConvert; }
    IHXPacketFlowControl* GetSessionCtrl() { return m_pSessionControl; }
private:
    class SockCloseCallback : public BaseCallback
    {
    public:
     	STDMETHOD(Func) (THIS);
	IUnknown* m_pContext;
	UDPIO*	 m_pUDP;	
    };

    IUnknown*	    	m_pContext;
    MulticastManager*	m_pMgr;
    INT32	    	m_lRefCount;
    INT32	    	m_lCount;
    LiveSource*	    	m_pLiveSource;
    char*		m_pFileName;
    enum MMProtocols	m_eProtocol;
    PacketFlowWrapper*	m_pPacketFlowWrap;
    Transport*        m_pTransport;
    IHXPSourceControl* m_pSourceControl;
    UINT32		m_ulAddress;
    UDPIO*		m_pUDP;
    IHXPacketFlowControl* m_pSessionControl;
    IHXDataConvertResponse*	m_pTransportDataConvert;
    UINT32              m_ulStreamCount;
    UINT32              m_ulStreamHeadersReceived;
    IHXValues**        m_ppStreamHeaders;
    BOOL                m_bSAPStarted; 
};

class MulticastManager
{
public:
    	    	    	    MulticastManager(IUnknown* pContext);
    	    	    	    ~MulticastManager();
    BOOL		    RegisterMulticast(
						const char* pFileName,
						enum MMProtocols eProtocol,
						MulticastSession*& pMSession,
						IUnknown*    pContext
					     );
    void		    UnRegisterMulticast (
						    const char* pFileName,
						    enum MMProtocols eProtocol
						);
    BOOL		    SetupOK();

    void		    Init();
    
    BOOL		m_bMulticastOnly;
    BOOL		m_bMulticastResend;
private:
    HX_RESULT	    	GetNextAddress(REF(UINT32) ulAddr, UINT32 ulPort);
    void		ReleaseAddress(UINT32 ulAddr);
    void		StartSap(UINT32 ulAddr, UINT32 ulPort, IHXValues** ppStreamHeaders, UINT32 ulStreamCount);
    void		StopSap(UINT32 ulAddr);
    
    /* maps and their mutex */
    CHXMapStringToOb*	m_pRDTMap;
    HX_MUTEX		m_pRDTMutex;
    CHXMapLongToObj*	m_pSapHandleMap;
    HX_MUTEX		m_pSapHandleMutex;
    
    UINT8		m_ucMulticastTTL;
    BOOL		m_bSetupOK;
    UINT32		m_ulRTSPPort;
    IUnknown*	    	m_pContext;
    IHXRegistry*	m_pRegistry;

    IHXStreamDescription*	m_pStreamDesc;
    
    MulticastAddressAllocator*	m_pAddrAllocator;
    IHXSapManager*		m_pSapMgr;
    friend class MulticastSession;
    friend class RTSPServerMulticastTransport;
#ifdef _DEBUG
    BOOL		m_bInitDone;
#endif
    BOOL		m_bLicenseCheckDone;
};

#endif

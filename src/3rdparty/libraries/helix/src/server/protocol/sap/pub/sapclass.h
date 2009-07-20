/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: sapclass.h,v 1.3 2004/05/03 19:02:50 tmarshall Exp $ 
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
/*  class to deal with SAP
 */

#ifndef _SAPCLASS_H_
#define _SAPCLASS_H_

#include "hxrand.h" // CMultiplePrimeRandom
#include "sapmgr.h" // IHXSapManager
/*
   // Here is the formula taken from draft-ietf-mmusic-sap-00.txt
    limits:
	TTL		BW
	1   - 15	2   kbps    = 2 * 2 ^ 10 bps = 2048 bps
	16  - 63	1   kbps		     = 1024 bps
	64  - 127	1   kbps		     = 1024 bps
	128 - 255	200 bps			     = 200  bps
*/
#define SAP_BW_LIMIT_1 2048
#define SAP_BW_LIMIT_2 1024
#define SAP_BW_LIMIT_3 1024
#define SAP_BW_LIMIT_4  200

#define SAP_RESET_TIME_INTERVAL 1800000 // 30 min in msec

#define SAP_VERSION 1
#define SDP_MULTICAST_ADDR 0xE0027FFE  // "224.2.127.254"
#define SDP_MULTICAST_PORT 9875
#define SAP_HEADER_SIZE 8 // b byets

#define PROP_MULTI_ADDRESS		"MulticastAddress"

class SapPacket;
class CHXPtrArray;
class Process;
class Config;
//class MulticastAddressPool;
_INTERFACE IHXMulticastAddressPool;

class CSapManager : public IHXSapManager
{
    class CSapInfo;

public:
    CSapManager(void);

    /* IUnknown */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);

    /* IHXSap */
    STDMETHOD_(UINT32, GetSAPInterval)	    (THIS_ 
					    UINT8	    uchTTL, 
					    UINT32	    ulPktSize);
    STDMETHOD_(SapHandle, StartAnnouncement) (THIS_ 
					    IHXBuffer*     pTextPayload, 
					    UINT8	    uchTTL);
    STDMETHOD(StopAnnouncement)		    (THIS_ 
					    SapHandle	    hSap);

    STDMETHOD(ChangeTTL)		    (THIS_
					    SapHandle hSap,
					    UINT8 uchTTL);
    STDMETHOD_(BOOL, IsAnnouncerEnabled)    (THIS_);
					    

    /* public func's */
    HX_RESULT	Init			    (IUnknown* pContext);
    HX_RESULT	StartDirectory		    (UINT32 ulAddr, UINT16 unPort);
    //    BOOL	IsListenEnabled		    (void) { return m_bListenEnabled; }
private:
    ~CSapManager();

    HX_RESULT	SendSAP(CSapInfo* pSapInfo);
    HX_RESULT	CreateSocket(UINT32 ulAddr, UINT16 uPort, UINT8  chTTL,
			     REF(IHXUDPSocket*) pSock, IHXUDPResponse* pResp);
    HX_RESULT	CompressSDP(IHXBuffer* pData, REF(IHXBuffer*) pCompBuf);
    HX_RESULT	MakeSap(IHXBuffer* pTextPayload,
			CSapInfo*   pSapInfo,
			SapType	    msgType);
    void	SendDeletionPkt(CSapInfo* pSapInfo);
    CallbackHandle ScheduleAnnouncement(CSapInfo* pSapInfo);
    void	RemoveAnnouncement(REF(CallbackHandle) hHandle);
    void	HandleSapCallback(SapHandle hSap);

    /*
     * To deal with sdp types
     * We might need to send 2 SAPs if the config var is "both".  If so, we need 
     * to have two sap handles but a user only cares about 1, so create a dammy 
     * CSapInfo class just to handle this...
     */
    HX_RESULT   DealWithSDPType		    (IHXBuffer* pTextPayload, 
					     REF(IHXBuffer*) pNewType, 
					     REF(IHXBuffer*) pOldType);
    void	ReallyStopAnnouncement(CSapInfo* pSapInfo);
    SapHandle	ReallyStartAnnouncement(IHXBuffer* pTextPayload, UINT8 uchTTL);    



    HX_RESULT   HandleSAP	(REF(SapPacket) pkt);
    void	Reset		(void);				 
    void	HandleResetCallback (void);    	      	    	    	 

    HX_RESULT	GetSAPSock	(void);
    UINT16	GetBWLimit(UINT8 uchTTL);
    BOOL 	GetAndMarkMultiAddr(REF(SapPacket) pkt, REF(CHXPtrArray*) prgAddrs);
    void	ReleaseAddrs(CHXPtrArray* prgAddrs);

    class CSapCallback : public IHXCallback
    {
    public:
	CSapCallback()
	    : m_lRefCount(0)
	    , m_hSap(0)
	    , m_pSapMgr(NULL)
	{}	    
	
	CSapCallback(CSapManager* pSapMgr, SapHandle hSap)
	    : m_lRefCount(0)
	    , m_hSap(hSap)
	    , m_pSapMgr(pSapMgr)
	{}   

	STDMETHOD (QueryInterface ) (THIS_ REFIID ID, void** ppInterfaceObj);
	STDMETHOD_(UINT32, AddRef ) (THIS);
	STDMETHOD_(UINT32, Release) (THIS);

	STDMETHOD(Func)		(THIS_);
	
    private:
	~CSapCallback() {};

    	INT32	    m_lRefCount;
    	SapHandle   m_hSap;
    	CSapManager*    m_pSapMgr;
    };
    friend class CSapCallback;

    /*
     *	If m_phSap is not NULL, this class is a DAMMY.  Get two real SapInfo obj
     *  by using m_phSap, which will be an array of two SapHandle elements.
     */
    class CSapInfo
    {
    public:
	CSapInfo();
    	~CSapInfo();

	IHXBuffer*	m_pSapPkt;
	UINT8		m_uchTTL;
	UINT16		m_unMsgIdHash;
	CallbackHandle	m_hCallbackID;
	SapHandle	m_hSap;
	// man...we need a single handle to return to the user, but we might have
	// to keep track of two handles if SdpFileType is both.
	SapHandle*	m_phSap;
    };
    friend class CSapInfo;
    
    /**************************************************************************
    * 	class to receive SDP file
    *   To figure out a SAP multicasting interval, we have to keep track of a
    *	number of other SAP msg on the trafic.
    */
    class CSDPResponseHandler : public IHXUDPResponse
                              , public IHXThreadSafeMethods
    {
    	~CSDPResponseHandler() {}
    	
    public:
    	CSDPResponseHandler(CSapManager* pParent)
	    : m_lRefCount(0)
	    , m_pSapMgr(pParent)
    	    {}
    	
    	/*******************************************************************
    	*  IUnknown COM Interface Methods                      ref:  hxcom.h
    	*/
    	STDMETHOD (QueryInterface ) (THIS_ REFIID ID, void** ppInterfaceObj);
    	STDMETHOD_(UINT32, AddRef ) (THIS);
    	STDMETHOD_(UINT32, Release) (THIS);   
    	/*
     	*	IHXUDPPResponse methods
     	*/   
    	STDMETHOD(ReadDone)	(THIS_
			    	HX_RESULT	status,
			    	IHXBuffer*	pBuffer,
			    	ULONG32		ulAddr,
			    	UINT16		nPort);  

        /* IHXThreadSafeMethods */
        STDMETHOD_(UINT32,IsThreadSafe)         (THIS);

    private:
	BOOL			ValidSapDatagram(BYTE* off, UINT32 len);

    	INT32			m_lRefCount;
    	CSapManager*		m_pSapMgr;
    };
    friend class CSDPResponseHandler;

    /**************************************************************************
    * 	class to reset the map and m_ulNumAds
    */
    class CResetCallback : public IHXCallback
    {
    public:
	CResetCallback(CSapManager* pParent)
	    : m_lRefCount(0)
	    , m_pSapMgr(pParent)
	    {}
	~CResetCallback() {}

	STDMETHOD (QueryInterface ) (THIS_ REFIID ID, void** ppInterfaceObj);
	STDMETHOD_(UINT32, AddRef ) (THIS);
	STDMETHOD_(UINT32, Release) (THIS);

	STDMETHOD(Func)		(THIS_);
	
    private:
	INT32			m_lRefCount;
	CSapManager*		m_pSapMgr;
    };
    friend class CResetCallback;



private:
    INT32			m_lRefCount;
    IHXUDPSocket*		m_pSAPSock;
    CSDPResponseHandler*	m_pSAPResp;
    UINT32			m_ulNumAds;
    CHXMapLongToObj		m_SourceIdMap;
    CallbackHandle		m_hResetCBHandle;
    IHXUDPMulticastInit*	m_pMulticast;
    CMultiplePrimeRandom	m_randGen;

    // keeps all SAP pkts, accessed by SapHandle
    CHXSimpleList		m_sapHandles;

    IHXStreamDescription*	m_pStreamDesc;
    IHXMulticastAddressPool*   m_pAddrPool;
    IHXScheduler*		m_pScheduler;
    IUnknown*			m_pContext;
    IHXCommonClassFactory*	m_pClassFactory;
    UINT32			m_ulOrigSrc;
    BOOL			m_bDirectoryStarted;
    BOOL			m_bListenEnabled;
    BOOL			m_bSendEnabled;
    BOOL			m_bNoCompression;
    UINT32                      m_ulCompressionSize;
    
    enum 
    {
	OLD_SDP,
	NEW_SDP,
	BOTH_SDP
    } m_sdpFileType;   
};

#endif // _SAPCLASS_H_


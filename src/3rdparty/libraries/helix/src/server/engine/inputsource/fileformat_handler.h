/* ***** BEGIN LICENSE BLOCK *****  
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

#ifndef _FILEFORMAT_HANDLER_H_
#define _FILEFORMAT_HANDLER_H_

_INTERFACE IHXBuffer;
_INTERFACE IHXClientProfileInfo;
_INTERFACE IHXSyncHeaderSource;

class CHXMapLongToObj;
class Process;
class CHXTimestampConverter;


#include "hxformt.h"
#include "hxasm.h"
#include "source.h"
#include "ispifs.h"

/*
 * A generic IHXFileFormatObject to IHXPSourceControl/IHXServerSource 
 * conversion class.
 * CHeaderHandler handles IHXPSourceControl
 * CPacketHandler handles IHXServerSource
 * CPacketHandler::CSingleStreamPush demultiplex GetPacket/PacketReady into a single
 * stream unit
 */
class CHeaderHandler : public IHXPSourceControl
		     , public IHXFormatResponse
		     , public IHXSyncHeaderSource
		     , public IHXFileFormatObject
		     , public IHXASMSource
		     , public IHXPSourcePackets
{
public:
    CHeaderHandler(Process* pProc, 
		   IHXFileFormatObject* pFF,
		   IUnknown* pFileObject,
		   IHXRequest* pRequest);

    /* IHXPSourceControl */
    STDMETHOD(Init)             (THIS_ IHXPSinkControl* pSink);
    STDMETHOD(Done)		(THIS);
    STDMETHOD(GetFileHeader)    (THIS_ IHXPSinkControl* pSink);
    STDMETHOD(GetStreamHeader)  (THIS_ IHXPSinkControl* pSink, UINT16 unStreamNumber);
    STDMETHOD(Seek)             (THIS_ UINT32 ulSeekTime);
    STDMETHOD_(BOOL,IsLive)     (THIS) { return FALSE; }
    STDMETHOD(SetLatencyParams) (THIS_
	UINT32 ulLatency, BOOL bStartAtTail, BOOL bStartAtHead) { return HXR_OK; }
        
    /* IHXFormatResponse */
    STDMETHOD(InitDone)		(THIS_ HX_RESULT status);
    STDMETHOD(FileHeaderReady)	(THIS_ HX_RESULT status, IHXValues* pHeader);
    STDMETHOD(StreamHeaderReady)(THIS_ HX_RESULT status, IHXValues* pHeader);
    STDMETHOD(PacketReady)	(THIS_ HX_RESULT status, IHXPacket* pPacket);
    STDMETHOD(SeekDone)		(THIS_ HX_RESULT status);
    STDMETHOD(StreamDone)	(THIS_ UINT16 unStreamNumber);    

    /* IHXSyncHeaderSource */
    STDMETHOD(GetFileHeader)	(THIS_ REF(IHXValues*)pHeader);
    STDMETHOD(GetStreamHeader)	(THIS_ UINT32 ulStreamNo, REF(IHXValues*)pHeader);

    // IHXFileFormat methods
    STDMETHOD(GetFileFormatInfo)(THIS_ REF(const char**) /*OUT*/ pFileMimeTypes,
				REF(const char**) /*OUT*/ pFileExtensions,
				REF(const char**) /*OUT*/ pFileOpenNames);
    STDMETHOD(InitFileFormat)(THIS_ IHXRequest*		/*IN*/	pRequest, 
			IHXFormatResponse*	/*IN*/	pFormatResponse,
			IHXFileObject*		/*IN*/  pFileObject);
    STDMETHOD(GetFileHeader)(THIS);
    STDMETHOD(GetStreamHeader)(THIS_ UINT16 unStreamNumber);
    STDMETHOD(GetPacket)(THIS_ UINT16 unStreamNumber);
    STDMETHOD(Close)(THIS);

    // IHXASMSource methods
    STDMETHOD(Subscribe)	(THIS_ UINT16 uStreamNumber, UINT16 uRuleNumber);
    STDMETHOD(Unsubscribe)	(THIS_ UINT16 uStreamNumber, UINT16 uRuleNumber);

    /* PPM Support */
    /* IHXPSourcePackets */
    STDMETHOD(Init)             (THIS_ IHXPSinkPackets* pSink);
    IHXPSinkPackets*		m_pPullPacketSink;

protected:
    ~CHeaderHandler();

    void _Done(void);
    void ReportInitFileFormat(HX_RESULT status, const char* pc);
    HX_RESULT SimulateStreamGroups();
    HX_RESULT CloneHeader(IHXValues* pSourceHeader, IHXValues** ppClonedHeader);
    HX_RESULT GetPhysicalStreams(UINT32 ulLogicalStreamNum, IHXValues* pHeader, REF(UINT32)ulNumPhysicalStreams, REF(UINT32*)aulAvgBandwidth);
    UINT16 GetActualLogicalStreamNumber(UINT16 unLogicalStreamNum);
    HX_RESULT UpdateStreamHeader(IHXValues* pHeader);
    inline HX_RESULT InitialStartup(void);

protected:
    UINT32		    m_ulRefCount;
    Process*		    m_pProc;
    
    IHXPSinkControl*	    m_pHeaderSink;

    IHXFileFormatObject*    m_pFileFormat;
    IHXASMSource*	    m_pAsmSource;
    IUnknown*		    m_pFileObject;
    IHXRequest*		    m_pRequest;

    IHXFormatResponse*	    m_pFormatResponse;

    BOOL		    m_bDone;

//    IHXClientProfileInfo*   m_pProfile;
    
    IHXValues*		    m_pFileHeader;
    CHXMapLongToObj*	    m_pStreamHeaderMap;    
    UINT32		    m_ulNumLogicalStreams;    
    UINT32		    m_ulNumStreamGroups;    
    UINT32*		    m_aulActualLogicalStream;

    BOOL		    m_bSimulateStreamGroups;
    BOOL            m_bStreamErrorReported;

    /*
     * InputSource performs multiple Subscribe/Unsubscribe in order to initialize 
     * a presenstation with an appropriate set of bitrates.  However, FF does not
     * support back to back Subscribe() call (it requires a packet to go out before
     * another Subscribe() succeeds). 
     * This class keeps a record of initial subscriptions and calls FF right before
     * packet fetching routine begins.  
     */
    class InitialSubscription
    {
    public:
	InitialSubscription();
	~InitialSubscription();
	HX_RESULT Init(UINT16 unStreamCount);
	void Update(BOOL bSubscribe, UINT16 unStreamNum, UINT16 unRuleNum);
	HX_RESULT Subscribe(IHXASMSource* pASMSource);

	// [stream#][rule#]
	BOOL** m_ppSubscription;
	UINT16 m_unStreamCount;
	UINT16 m_unActualHiRuleNum;
    };
    InitialSubscription*    m_pInitialSubscription;
};

class CPacketHandler : public CHeaderHandler
		     , public IHXServerPacketSource
{
public:
    CPacketHandler(Process* pProc, 
		   IHXFileFormatObject* pFF,
		   IUnknown* pFileObject,
		   IHXRequest* pRequest);

    /* IHXPSourceControl */
    STDMETHOD(Init)             (THIS_ IHXPSinkControl* pSink);
    STDMETHOD(Seek)             (THIS_ UINT32 ulSeekTime);
    STDMETHOD(Done)		(THIS);    

    /* IHXServerPacketSource */
    STDMETHOD(SetSink)(THIS_ IHXServerPacketSink* pSink);
    STDMETHOD(StartPackets) (THIS);
    STDMETHOD(GetPacket) (THIS);
    STDMETHOD(SinkBlockCleared)(THIS_ UINT32 ulStream);
    STDMETHOD(EnableTCPMode)    (THIS);

    /* IHXFormatResponse */
    STDMETHOD(PacketReady)	(THIS_ HX_RESULT status, IHXPacket* pPacket);
    STDMETHOD(SeekDone)		(THIS_ HX_RESULT status);
    STDMETHOD(StreamDone)	(THIS_ UINT16 unStreamNumber);    

    /* IUnknown Interfaces */
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

protected:
    /*
     * private class - represents a single stream that interacts with
     * IHXServerPacketSink
     */
    class CSingleStreamPush : public IUnknown
    {
    public:
    	CSingleStreamPush(CPacketHandler* pParent, UINT32 ulStreamNo);
    	~CSingleStreamPush();

    	/* IUnknown Interfaces */
    	STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    	STDMETHOD_(ULONG32,AddRef)  (THIS);
    	STDMETHOD_(ULONG32,Release) (THIS);

	HX_RESULT   Init(IHXServerPacketSink* pPacketSink);
	void	    Close(void);// close immediately - IHXPSourceControl::Done()
	void	    Done(void);	// done flush packet - IHXFormatResponse::StreamDone()
	
	HX_RESULT   Start(void);
	HX_RESULT   Restart(void);
	HX_RESULT   PacketReady(IHXPacket* pPacket);
	void	    Reset(void);

    private:	
	HX_RESULT   TransmitPacket(ServerPacket* pPacket);

	// Packet conversion routine
	typedef ServerPacket* (CSingleStreamPush::*PacketConverter)(IHXPacket*); 
	inline ServerPacket* PacketConvert(IHXPacket* pPkt);
	inline ServerPacket* PacketTSConvert(IHXPacket* pPkt);
	HX_RESULT FindPktConverter(IHXPacket* pPacket);

	// TXState handling in Start()
	HX_RESULT   HandleStreamReady(void);
	HX_RESULT   HandleStreamBlocked(void);
	HX_RESULT   HandleStreamFlush(void);
	HX_RESULT   FlushPacket(void);

	void	    DoneStream(void);
	
	CPacketHandler*	    m_pParent;
	UINT32		    m_ulRefCount;
    	UINT32		    m_ulStreamNo;

	enum TxState
	{
	    STREAM_READY,   // Init()ed, waiting for Start() -> STARTED
	    STREAM_STARTED, // Start()ed	    
	    STREAM_BLOCKED, // blocked, waiting for Start() -> STARTED
	    STREAM_FLUSH,   // Done(), waiting for Start() -> READY & call StreamDone on HeaderSink 
	    STREAM_CLOSED   // Closed(), no further action taken until Init()
	}		    m_state;
    	ServerPacket*	    m_pPacket; 
    
    	IHXServerPacketSink*    m_pPacketSink;
    	CHXTimestampConverter*	m_pTSConverter;
    	PacketConverter		m_pPktConverter;
    };
    friend class CSingleStreamPush;
    
    ~CPacketHandler();
    

    inline HX_RESULT	    Start(UINT32 ulStreamNo);    
    inline HX_RESULT	    Fetch(UINT32 ulStreamNo);

    void   DoneStream(UINT32 ulStreamNo);
    HX_RESULT CreateSinglePushStreams(void);

    CSingleStreamPush**	    m_ppPacketSinks;
    IHXServerPacketSink*    m_pPacketSink;
    BOOL		    m_bSuspended;
};
#endif /* _FILEFORMAT_HANDLER_H_ */

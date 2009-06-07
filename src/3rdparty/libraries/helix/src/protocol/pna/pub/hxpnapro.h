/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0 and Exhibits. 
 * REALNETWORKS CONFIDENTIAL--NOT FOR DISTRIBUTION IN SOURCE CODE FORM 
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. 
 * All Rights Reserved. 
 * 
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Community Source 
 * License Version 1.0 (the "RCSL"), including Attachments A though H, 
 * all available at http://www.helixcommunity.org/content/rcsl. 
 * You may also obtain the license terms directly from RealNetworks. 
 * You may not use this file except in compliance with the RCSL and 
 * its Attachments. There are no redistribution rights for the source 
 * code of this file. Please see the applicable RCSL for the rights, 
 * obligations and limitations governing use of the contents of the file. 
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 * 
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 * https://rarvcode-tck.helixcommunity.org 
 * 
 * Contributor(s): 
 * 
 * ***** END LICENSE BLOCK ***** */ 

#ifndef _PNAPROTOCOL__
#define _PNAPROTOCOL__

struct  IHXASMSource;
struct  Challenge;
class   ResendManager;

#include "hxtypes.h"
#include "hxresult.h"
#include "conn.h"
#include "hxnet.h"
#include "hxprotocol.h"
#include "growingq.h"

#define NETSTATE_MAX_NAME_LEN  28 /* name[20] was being used, but "authentication_len_state" is 24 chars long... */

// macro to set a state table entry
#define TABLE_INIT(t,x,y,z) 	\
	(t).required = (x);	\
	(t).fun = (y);		\
	strncpy((t).name,(z), NETSTATE_MAX_NAME_LEN); /* Flawfinder: ignore */

#define MAX_STATES 	30

#define MAX_AUTHENTICATION_VERSION	1   // the highest authenrication version we support
#define AUTHENTICATION_KEY_SIZE		16

//////////////////////////////////////////
// Servers with protocol's greater than this
// must send a challenge response, or else we
// will refuse to work with them...
#define MIN_CHALLENGE_PROTOCOL	8

enum
{
	PN_STATE_CLOSED = 0
};

#ifndef XXX //...Came from pnpro.h

#define ENQUEUE_DATA(x,y,z) (x)->EnQueue((y),(UINT16)(z))
#define ENQUEUE_STRING(x,y) (x)->EnQueue((const char*)y, (UINT16)y.GetLength())

#define ENQUEUE_BYTE(x,y) {UCHAR uChar = (UCHAR)(y); \
	(x)->EnQueue(&uChar,sizeof(uChar));}

#define ENQUEUE_WORD(x,y) {UINT16 wTemp = (UINT16)(y); \
	wTemp = WToNet(wTemp); \
	(x)->EnQueue(&wTemp,sizeof(UINT16));}

#define ENQUEUE_DWORD(x,y) {ULONG32 dTemp = (ULONG32)(y); \
	dTemp = DwToNet(dTemp); \
	(x)->EnQueue(&dTemp,sizeof(ULONG32));}

// need to be removed from here once all common defines are moved
// to a separate file..

#define PN_5_PORT_OFFSET 	9

// macro to reset server timeout
#define RESET_TIMEOUT(x)	{ x = HX_GET_TICKCOUNT(); }

#define PING_DELAY 		60000	// 60 secs * 1000 ms/sec
#define SHORT_PING_DELAY 	2000	// 5 secs * 1000 ms/sec
#define TCP_PING_DELAY 		10000	// 10 secs * 1000 ms/sec

#ifndef TCP_BUF_SIZE
#define TCP_BUF_SIZE 		32678
#endif

#define QUEUE_START_SIZE	512

#define MIN_DATA_BLOCKS 	12

#endif //XXX

// in order to support multiple streams, we need to store the sequence number
// info for each stream.

#define MAX_PNA_STREAMS	32	// Note: we can actually have up to 128 streams
				// we need to make this dynamic. Question: how
				// do we know how many streams are active?

typedef struct _PNA_SEQUENCE_INFO
{
    UINT16	mStreamNumber;
    ULONG32	mSequenceDecade;
    ULONG32	mNextDecade;
    ULONG32	mReadSequence;
    ULONG32	m_ulStreamBandwidth;
    HXBOOL	mIsSynchronized;
} PNA_SEQUENCE_INFO;

struct IHXPacket;
struct IHXValues;
class CHXFile;
class CHXEvent;

struct IHXInterruptSafe;

class PNAProtocol : public HXProtocol, 
		    public IHXASMSource,
		    public IHXAuthenticationManagerResponse,
                    public IHXResolveResponse
{
public:
    PNAProtocol(HXNetSource *owner, ULONG32 ulPlatformSpecific = 0 );
    ~PNAProtocol();

    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /* IHXResolveResponse methods */
    STDMETHOD(GetAddrInfoDone)      (THIS_ HX_RESULT status, UINT32 nVecLen, IHXSockAddr** ppAddrVec);
    STDMETHOD(GetNameInfoDone)      (THIS_ HX_RESULT status, const char* pNode, const char* pService);


    // IHXPendingStatus methods

    /************************************************************************
     *	Method:
     *	    IHXPendingStatus::GetStatus
     *	Purpose:
     *	    Called by the user to get the current pending status from an object
     */
    STDMETHOD(GetStatus)	(THIS_
				REF(UINT16) uStatusCode, 
				REF(IHXBuffer*) pStatusDesc, 
				REF(UINT16) ulPercentDone);
    /*
     *	IHXStatistics methods
     */

    /************************************************************************
     *  Method:
     *      IHXStatistics::InitializeStatistics
     *  Purpose:
     *      Pass registry ID to the caller
     */
    STDMETHOD (InitializeStatistics)	(THIS_
					UINT32	/*IN*/ ulRegistryID);

    /************************************************************************
     *  Method:
     *      IHXStatistics::UpdateStatistics
     *  Purpose:
     *      Notify the client to update its statistics stored in the registry
     */
    STDMETHOD (UpdateStatistics)	(THIS);

    /*
     * IHXASMSource methods
     */

    STDMETHOD (Subscribe)	(THIS_
				UINT16	uStreamNumber,
				UINT16	uRuleNumber);

    STDMETHOD (Unsubscribe)	(THIS_
				UINT16	uStreamNumber,
				UINT16	uRuleNumber);

    /*
     * IHXAuthenticationManagerResponse methods
     */
    STDMETHOD(AuthenticationRequestDone) (THIS_ HX_RESULT,
					  const char*,
					  const char*);

    virtual HX_RESULT	GetStreamStatistics	(ULONG32 ulStreamNumber,
						 STREAM_STATS** ppStreamStats);

    virtual HX_RESULT	UpdateRegistry		(UINT32 ulStreamNumber,
						 UINT32 ulRegistryID);

    virtual HX_RESULT	server_hello		(void);

    virtual HX_RESULT	proxy_hello		(void);

    virtual HX_RESULT	process			(void);

    virtual HX_RESULT	abort	 		(void);

    virtual HX_RESULT   GetEvent		(UINT16 usStreamNumber,
						 CHXEvent*& pEvent);

    virtual HX_RESULT	GetCurrentBuffering(UINT16  uStreamNumber,
					    UINT32& ulLowestTimestamp, 
					    UINT32& ulHighestTimestamp,
					    UINT32& ulNumBytes,
					    HXBOOL& bDone);

    virtual HX_RESULT	setup			(const char* host,
						 const char* path,
						 UINT16 port,
						 HXBOOL	LossCorrection,
						 HXBOOL	bHTTPCloak,
                                                 HXBOOL   bSDPInitiated,
						 UINT16	cloakPort);

    virtual void	initialize_members	(void);

    virtual HX_RESULT	seek			(ULONG32 posArg,
						 ULONG32 posArg2 = 0,
						 UINT16  seekFrom = 0);

    virtual HX_RESULT	pause			(void);

    virtual HX_RESULT	resume			(UINT32 ulEndTime = 0);

    virtual HX_RESULT	stop 			(void);

    virtual void	send_statistics		(UINT32	ulStatsMask);

    virtual void	set_transport		(TransportMode mode, UINT32 ulTransportPrefMask);

    HX_RESULT		resend_request		(ULONG32 sequenceNum,
						 UINT16  stream_number,
						 ULONG32 numPackets);

    HX_RESULT		ReadDataSocket		(void);
    void                HandleDataReadEvent(HX_RESULT status);

    HX_RESULT		ReadControlSocket	(void);
    void                HandleControlReadEvent(HX_RESULT status);
    void                HandleControlConnectDone(HX_RESULT status);

    // bandwidth caculation
    UINT32		GetSourceStartTime	(void)
						{
						    return m_ulSourceStartTime;
						};

    UINT32		GetTotalPauseTime	(void)
						{
						    return m_ulTotalPauseTime;
						};

    // stream thinning
    virtual void	process_bandwidth_report(char* data, UINT16 len,HXBOOL bUDPFormat);
    virtual void	start_bandwidth_report	(void);
    virtual void	bandwidth_report	(UINT16 timeInterval, ULONG32 numBytes);
    virtual void	reset_bandwidth_report	(void);

    virtual void	DoFrameControl		(void);

    virtual HX_RESULT 	frame_control		(UINT16 stream, 
						 UCHAR	interframes_enabled, 
						 UCHAR	frameRate,
						 HXBOOL	bRestart = FALSE);
    virtual HX_RESULT 	_frame_control		(UINT16 stream, 
						 UCHAR	interframes_enabled, 
						 UCHAR	frameRate);

	    HXBOOL	IsSourceDone(void);

    const char*		get_protocol_name	(void)
						{return "PNM";}

private:

    // helpers
    HX_RESULT SetMulticastAddr(UINT32 mcastAddr, UINT16 mcastPort);
    HX_RESULT CreateMulticastSocket();
    HX_RESULT ResolveConnectAddr(const char* pHost);
    HX_RESULT CreateCloakedControlSocket(HXBOOL bUseProxy);
    HX_RESULT SendHello();



    HX_RESULT		_proxy_hello		(void);

    HX_RESULT		control_write		(void);

    HX_RESULT		control_read		(void);

    HX_RESULT		data_read		(void);

    HX_RESULT		data_write		(IHXBuffer* pBuffer = NULL);

    HXBOOL		data_available		(void);

    HX_RESULT		process_control		(void);

    HX_RESULT		process_data		(UCHAR* mTempBuf,
						 UINT16 length);

    char*		parse_data		(char*   data,
						 UINT16  &length,
						 HXBOOL    &valid,
						 ULONG32 &sequence,
						 ULONG32 &timestamp,
						 UINT16  &subsequence,
						 UINT16  &stream_number,
						 UINT16  &flags);

    HX_RESULT		server_timeout		(void);

    HX_RESULT		AttemptRetry	        (void);
    void                HandleSocketError(HX_RESULT code);


    HX_RESULT		retry			(ULONG32 playpos,
						 UINT16  resume);

    HX_RESULT           send_authentication_version1(const char *user, 
                                                     const char *password);
    HX_RESULT           send_authentication_version0(const char *user, 
                                                     const char *password);

    HX_RESULT		flow_control		(HXBOOL flow_val);

    HX_RESULT		create_event_header	(IHXValues*& pHeader, 
						 ULONG32 ulDuaration);

    HX_RESULT		create_header		(IHXValues*& header,
						 UCHAR*       data,
						 UINT16       length);

    HX_RESULT		create_packet		(IHXPacket*& packet,
						 UCHAR*       data,
						 UINT16       len,
						 ULONG32      sequence,
						 ULONG32      timestamp,
						 UINT16       stream_number,
						 UINT16       flags = 0);

    HX_RESULT		AddStreamInfo		(UINT16 ulStreamNumber, ULONG32 ulStreamBandwidth);

    PNA_SEQUENCE_INFO*	GetSequenceInfo		(UINT16 ulStreamNumber);

    void		DeleteSequenceInfo	(void);

    HX_RESULT		allocate_buffers	(void);

    void		empty_buffers		(void);

    void		delete_buffers		(void);

    HX_RESULT		SetupControlSocket	(const char* host,
						 UINT16     port,
						 HXBOOL	    bHTTPCloaking = FALSE,
						 UINT16		cloakPort = 80);

    HX_RESULT		SetupDataSocket		(UINT16 port);
    void                FinishSetup(HX_RESULT hr);

    HX_RESULT		convert_network_error	(HX_RESULT theErr);

    char*               GetBandwidthData(UINT32* len);

    void		AddRuleToFlagMap	(IHXValues* pHeader);

    void		TransportSucceeded	(void);

    // state table
    struct NetState 
    {
	UINT16   required;
	HX_RESULT (PNAProtocol::*fun)(char *, UINT16);
	char     name[NETSTATE_MAX_NAME_LEN]; /* Flawfinder: ignore */
    };

    NetState state_table[MAX_STATES];

    HX_RESULT	hello_state			(char *data,UINT16 len);
    HX_RESULT	interleave_state		(char *data,UINT16 len);
    HX_RESULT	audio_len_state			(char *data,UINT16 len);
    HX_RESULT	audio_state			(char *data,UINT16 len);
    HX_RESULT	rawdata_len_state		(char *data,UINT16 len);
    HX_RESULT	rawdata_state			(char *data,UINT16 len);
    HX_RESULT	format_len_state		(char *data,UINT16 len);
    HX_RESULT	format_state			(char *data,UINT16 len);
    HX_RESULT	length_state			(char *data,UINT16 len);
    HX_RESULT	event_len_state			(char *data,UINT16 len);
    HX_RESULT	event_state			(char *data,UINT16 len);
    HX_RESULT	time_state			(char *data,UINT16 len);
    HX_RESULT	pna_time_offset_state		(char *data,UINT16 len);
    HX_RESULT	alert_len_state			(char *data,UINT16 len);
    HX_RESULT	alert_state			(char *data,UINT16 len);
    HX_RESULT	ready_state			(char *data,UINT16 len);
    HX_RESULT	seek_ack_state			(char *data,UINT16 len);
    HX_RESULT	ping_request			(char *data,UINT16 len);
    HX_RESULT	ping				(void);
    HX_RESULT	redirect_len_state		(char *data,UINT16 len);
    HX_RESULT	redirect_state			(char *data,UINT16 len);
    HX_RESULT	proxy_status_len		(char *data,UINT16 len);
    HX_RESULT	proxy_status			(char *data,UINT16 len);
    HX_RESULT	proxy_version			(char *data,UINT16 len);
    HX_RESULT	challenge_len_state		(char *data,UINT16 len);
    HX_RESULT	challenge_state			(char *data,UINT16 len);
    HX_RESULT	option_response_len_state	(char *data,UINT16 len);
    HX_RESULT	option_response_state		(char *data,UINT16 len);
    HX_RESULT	bandwidth_report_state		(char *data,UINT16 len);
    HX_RESULT	authentication_len_state	(char *data, UINT16 len);
    HX_RESULT	authentication_state		(char *data, UINT16 len);
 
    ULONG32	mSecurityKey;		// used for challenge
    UINT16	mTCPState;		// current state
    ULONG32	mResendID;		// resend id for player authentication
    UINT16	mTimeExtant;		// # of time states required after seek
    UINT16	mSeeksExtant;		// keep track of number of seeks
    UINT16	mFormatHeaderLength; 	// used in challenge
    ULONG32	mEventBegin;		// event start time
    ULONG32	mEventEnd;		// event end time
    UINT16	mEventLength;		// event length
    UCHAR	*mEventBuffer;		// buffer to store event
    ULONG32	m_ulEventSequence;
    ULONG32	mStartTime;		// used for tcp audio timestamps
    ULONG32	mOldStartTime;
    ULONG32	mActualBandwidth; 	// bandwidth preference of the user
    UINT16	mDatagramBase;		// always set to 4
    UINT16	mBlockHeaderSize;	// always set to 11     

    // ads. insertion
    UINT32	m_ulMaxStreamDuration;

    // avg. bandwidth caculation
    UINT32	m_ulSourceStartTime;

    // authentication
    UINT16	m_AuthenticateVersion;
    UINT16	m_RealmLength;
    char*	m_pRealmData;
    IHXBuffer*	m_pRealm;
    ULONG32	m_Nonce1;
    ULONG32	m_Nonce2;


    ULONG32	m_TimeToCheckBW;
    ULONG32	mTimeToResumeInterframes;
    ULONG32	m_VideoBandwidth;
    ULONG32	m_AudioBandwidth;
    ULONG32	m_KeyframeRate;
    ULONG32	m_FirstBWFlowBWPercent;
    ULONG32	m_TargetVideoPercent;
    ULONG32	m_PrevVideoPercent;
    ULONG32	m_TimeFlowControlOff;
    ULONG32	m_TimeToResumeInterframes;
    ULONG32	mClipBandwidth;

    INT16	m_VideoStreamNum;
    UINT32	m_ulLastVideoTimeStamp;
    INT16	m_AudioStreamNum;
    UINT16	m_usLastAudioRule;
  
    UCHAR	m_LastBWReportSequence;
    ULONG32	m_StartBWReportTime;
    ULONG32	m_LastBWReportTime;
    ULONG32	m_BWReportBytesReceived;
    ULONG32	m_totalServerTime;
    ULONG32	m_totalPlayerTime;
    ULONG32	m_totalServerBytes;
    ULONG32	m_totalPlayerBytes;

    // used to calculate timestamp for tcp_audio
    float	mMsecsPerByte;		// this info taken from owner
    ULONG32	mTotalBytes;		// total tcp audio bytes received 

    UINT16	mGeneration;		// seek generation (for UDP data )
    UINT16	mPrevGeneration;	// previous UDP data seek generation    
    
    UINT16	mServerResendPort;	// port to send server resend requests
    CByteGrowingQueue*	mSendTCP;		// queue for sending tcp data to net
    CByteGrowingQueue*	mReceiveTCP;		// queue for receiving tcp data from net			
    CHXSimpleList* mPendingUDPWriteList;
    char*	mTempInBuf;		// temp buffer to hold input data
    UINT16	mCurrentTempInBufSize;

    // pending seek info when we have to start from offset...
    UINT16	mPendingSeekfrom;
    ULONG32	mPendingPosArg;
    ULONG32	mPendingPosArg2;

    UINT16	mSubSequence;		// packet subsequencing in protocol 10	
    ULONG32	mReadAudioSequence;	// sequence number for TCP audio
    ULONG32	mPrevAudioSequence;	// prev sequence number for TCP audio

    CHXMapLongToObj	mSequenceMap;

    UINT16	mBlockSize;

    UINT16	mInterleaveFactor;

    ULONG32	m_ulAvgBandwidth;

    Challenge*	mClientChallenge;	// for client-server authentication
    Challenge*	mServerChallenge;	// for client-server authentication
    UINT16	mChallengeVersion;	// for client-server authentication
    ULONG32	mChallengeLength;	// for client-server authentication
    ULONG32	mServerTime;		// time last received data from server 
    ULONG32	mPingTime;		// time for next ping to the server

    UINT16	mResendPort;		// client port to send resend requests
    UINT16	m_uNumHeadersExpected;
    UINT16	m_uNumHeadersReceived;	// count for data!

    /* These are for backwards compatibility for a bug in realvideo server */
    INT16	m_sRealAudioStream;
    ULONG32	m_ulAudioSkewTime;
    
    IHXNetServices*     m_pNetServices;
    IHXResolve*         m_pResolver;
    IHXSockAddr*        m_pServerAddr;  // should match connect addr
    IHXSockAddr*        m_pConnectAddr; // resolved addr we connect to
    IHXSockAddr*        m_pMulticastAddr;

    // for pending resolver during setup
    struct SETUPDATA
    {
        SETUPDATA() : m_port(0), m_cloakPort(80), m_bHTTPCloaking(FALSE) {}
        UINT16 m_port;
        UINT16 m_cloakPort;
        HXBOOL   m_bHTTPCloaking;
    } m_setupData;

    IHXSocket*  m_pControlSocket;
    IHXSocket*  m_pDataSocket;
    IHXSocket*  m_pMulticastSocket;
    IHXBuffer*  m_pMostRecentControlBuffer; // facilitates using byte-growing queue in combo with new net api
    

    // bandwidth caculation
    UINT32	m_ulLastPauseTime;
    UINT32	m_ulLastPingTimeInPause;
    UINT32	m_ulTotalPauseTime;
    UINT16	m_ulBandwidthReportLen;

    LONG32	m_lRefCount;

    class  PNADataSockResponse;
    class  PNAControlSockResponse;		

    PNAControlSockResponse*	m_pControlSocketResponse;
    PNADataSockResponse*	m_pDataSocketResponse;
    PNADataSockResponse*	m_pMulticastSocketResponse;

    CHXSimpleList*	m_pUDPDataList;
    ResendManager*	m_pResendManager;

    HX_BITFIELD	m_bStarTimeToBeSet : 1;
    HX_BITFIELD	mInitialized : 1;		// == 1 owner is ready to receive data
    HX_BITFIELD	m_bFirstAuthAttempt : 1;
    // stream thinning
    HX_BITFIELD	m_bFrameControlSupport : 1;
    HX_BITFIELD	m_bHasBWReportSequence : 1;
    HX_BITFIELD	m_bFirstBWSequence : 1;
    HX_BITFIELD	m_bSkipFirstBWReport : 1;
    HX_BITFIELD	m_bRestartBWReport : 1;

    HX_BITFIELD	m_bVideoHasInterframes : 1;
    HX_BITFIELD	m_bFirstBWFlowControl : 1;
    HX_BITFIELD	mVideoFlowControl : 1;
    HX_BITFIELD	m_bInterframes : 1;
    HX_BITFIELD	m_bServerHasResend : 1;	// == 1 server supports resend
    HX_BITFIELD	mIsVersion3_0Server : 1;
    HX_BITFIELD	mReceivedInterleave : 1;
    HX_BITFIELD	mReceivedControl : 1;	// received some data on "control" link
    HX_BITFIELD	mReceivedData : 1;		// received some data on "data" link
    
    HX_BITFIELD	m_bResumeSent : 1;
    HX_BITFIELD	m_bDataWaitStarted : 1;	// have we started the wait
    HX_BITFIELD	mSynchronized : 1;

    HX_BITFIELD	m_bMulticastReadPending : 1;

    HX_BITFIELD	m_bLivePauseSupport : 1;
    HX_BITFIELD	m_bCompositeUDPSupport : 1;
    HX_BITFIELD	m_bBandwidthReportSupport : 1;
    HX_BITFIELD	m_bProblemDetected : 1;
    HX_BITFIELD	m_bProblemExists : 1;
    HX_BITFIELD	m_bIsRAFile : 1;
    HX_BITFIELD	m_bIsNonInterleavedRAStream : 1;
    HX_BITFIELD	m_bNeedLoadTestResponse : 1;


    class ResendPendingStatus : public IHXPendingStatus
    {
    private:
	LONG32	     m_lRefCount;
	PNAProtocol* m_pProtocol;

    public:
	ResendPendingStatus(PNAProtocol* pProtocol)
	{
	    m_pProtocol = pProtocol;
	};

	~ResendPendingStatus() {};

	/*
	 * IUnknown methods
	 */
	STDMETHOD(QueryInterface)	(THIS_
					REFIID riid,
					void** ppvObj) {return HXR_UNEXPECTED;};

	STDMETHOD_(ULONG32,AddRef)	(THIS) {return 1;};

	STDMETHOD_(ULONG32,Release)	(THIS) {return 1;};

	/*
	 * IHXPendingStatus methods
	 */

	/*******************************************************************
	 *	Method:
	 *	    IHXPendingStatus::GetStatus
	 *	Purpose:
	 *	    Called by the user to get the current pending status from
	 *          an object
	 */
	STDMETHOD(GetStatus)		(THIS_
					REF(UINT16) uStatusCode, 
					REF(IHXBuffer*) pStatusDesc, 
					REF(UINT16) ulPercentDone) 
					{if (m_pProtocol)
					 {return m_pProtocol->GetStatus(
					     uStatusCode, 
					     pStatusDesc, 
					     ulPercentDone);
					 }
					 return HXR_OK;
					};
    };

    friend class ResendPendingStatus;

    ResendPendingStatus*	m_pPendingStatus;

    class RAHeaderParser
    {
        struct RaChunk;
        friend struct RaChunk;

	typedef ULONG32 RaID;
	typedef ULONG32 RaChunkSize;

	// The RaChunk header must precede every chunk in the file
	struct RaChunk
	{
	    RaID	ckID;		// chunk ID
	    RaChunkSize	ckSize;		// size of chunk
	};

	// copy byte values

#define RA_SAVE_BIT		1
#define RA_PERFECT_PLAY_BIT	2

	// Stream Types
	enum
	{
	    FILE_STREAM = 0,
	    NET_STREAM,
	    LIVE_STREAM
	};

    public:
	RAHeaderParser(PNAProtocol* pOwner);
	~RAHeaderParser();

	/* Just parse header and get info */
	HX_RESULT ParseHeader(UCHAR* buf, UINT16 len);

	/* parse header and form an IHXValues */
	HX_RESULT ParseHeader(UCHAR* buf, UINT16 len, IHXValues*& pHeader);
	    
	HXBOOL    IsLiveStream();
	HXBOOL    IsSaveAsAllowed();
	HXBOOL    IsPerfectPlayAllowed();
	HXBOOL    IsInterleaved() { return (HXBOOL) m_cIsInterleaved;};
	UINT16	GetInterleaveFactor() { return m_usInterleaveFactor;};
	ULONG32 GetBytesPerMinute() {return m_ulBytesPerMinute;};
	ULONG32 GetSuperBlockTime() {return m_ulSuperBlockTime;};

    protected:
	HX_RESULT ParseV3Header(UCHAR* buf);
	HX_RESULT ParseV4Header(UCHAR* buf);

//	UINT32		m_ulMagicNumber;
	UINT16		m_usVersion;
	UINT16		m_usV3Len;
	UINT32		m_ulDuration;
	UINT32		m_ulSuperBlockTime;

	/* Header block of an RA file holds all this: */
	UINT32		m_ulBytesTotal;
	UINT32		m_ulBytesPerMinute;
	UINT16		m_usInterleaveFactor;
	UINT16		m_usBlockSize;
	UINT32		m_ulSampleRate;
	UINT8		m_cIsInterleaved;
	UINT8		m_cCopyByte;
	UINT8		m_cStreamType;
	UINT8		m_cTitleLen;
	char*		m_cpTitle;
	UINT8		m_cAuthorLen;
	char*		m_cpAuthor;
	UINT8		m_cCopyrightLen;
	char*		m_cpCopyright;
	UINT8		m_cAppLen;
	char*		m_cpApp;
	PNAProtocol*	m_pOwner;
    };

    friend class RAHeaderParser;

    class PNAControlSockResponse : public IHXSocketResponse,
			   public IHXInterruptSafe 
    {
    public:

	PNAControlSockResponse(PNAProtocol* pOwner);
	~PNAControlSockResponse();
	/*
	 *  IUnknown methods
	 */
	STDMETHOD(QueryInterface)		(THIS_
						REFIID riid,
						void** ppvObj);

	STDMETHOD_(ULONG32,AddRef)		(THIS);

	STDMETHOD_(ULONG32,Release)		(THIS);


        /* IHXSocketResponse methods */
        STDMETHOD(EventPending)         (THIS_ UINT32 uEvent, HX_RESULT status);

    	/*
     	 * IHXInterruptSafe methods 
     	 */
	STDMETHOD_(HXBOOL,IsInterruptSafe)	(THIS) {return TRUE;};


    private:
	LONG32			m_lRefCount;
	PNAProtocol*		m_pOwner;
    };

    friend class PNAControlSockResponse;

    class PNADataSockResponse : public IHXSocketResponse,
			   public IHXInterruptSafe 
    {
    public:

	PNADataSockResponse(PNAProtocol* pOwner);
	~PNADataSockResponse();
	/*
	 *  IUnknown methods
	 */
	STDMETHOD(QueryInterface)		(THIS_
					    REFIID riid,
					    void** ppvObj);

	STDMETHOD_(ULONG32,AddRef)		(THIS);

	STDMETHOD_(ULONG32,Release)		(THIS);

        /* IHXSocketResponse methods */
        STDMETHOD(EventPending)         (THIS_ UINT32 uEvent, HX_RESULT status);


    	/*
     	 * IHXInterruptSafe methods 
     	 */
	STDMETHOD_(HXBOOL,IsInterruptSafe)	(THIS) {return TRUE;};

    private:
 
	LONG32			m_lRefCount;
	PNAProtocol*		m_pOwner;
	HXBOOL			m_bMulticast;
	HXBOOL			m_bReceivedData;
    };

    friend class PNADataSockResponse;
};


#endif //_PNAPROTOCOL__

#ifndef CBR_RATEMGR_H
#define CBR_RATEMGR_H

class PacketFlowManager;

class CCBRRateMgr : public IHXQoSRateManager,
                    public IHXPacketFlowControl
{
public:
    // Constructors
    CCBRRateMgr(Process* proc);
    CCBRRateMgr(Process* proc, UINT16 unStreamCount, PacketFlowManager* pFlowMgr);
    
    // Destructors
    virtual ~CCBRRateMgr();

    // IUnknown methods
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXQoSRateManager methods
     */
    STDMETHOD(Init) (THIS_ IHXSessionStats* pSessionStats, IHXBuffer* pbufsessionId, UINT16 unStreamCount, IHXServerPacketSource*  pSource);
    STDMETHOD(VerifyDelivery) (THIS_ ServerPacket* pPacket, Timeval& ulTime);
    STDMETHOD(TimeLineStateChanged) (THIS_ BOOL bStart);
    STDMETHOD_(ULONG32, GetActualDeliveryRate) (THIS) const;
    STDMETHOD(RegisterStream) (THIS_ PacketStream* pStreamData, UINT16 ulStreamNumber);    
    STDMETHOD(Done) (THIS);

    /*
     * IHXPacketFlowControl methods
     */
    STDMETHOD(Play) (THIS);

    STDMETHOD(StartSeek) (THIS_
                          UINT32 ulTime);

    STDMETHOD(Activate)(THIS);

    STDMETHOD(WantWouldBlock)(THIS);

    STDMETHOD(SeekDone)(THIS);

    STDMETHOD(SetEndPoint)(THIS_
                           UINT32 ulEndPoint, 
                           BOOL bPause);

    STDMETHOD(SetStartingTimestamp)(THIS_
                                    UINT32 ulStartingTimestamp);

    STDMETHOD(RegisterStream)(THIS_
                              Transport* pTransport,
                              UINT16 uStreamNumber,
                              ASMRuleBook* pRuleBook,
                              IHXValues* pHeader);

    STDMETHOD(RegisterStream)(THIS_
                              Transport* pTransport,
			      UINT16 uStreamGroupNumber,
                              UINT16 uStreamNumber,
                              ASMRuleBook* pRuleBook,
                              IHXValues* pHeader)
    { return HXR_OK;}

    STDMETHOD(GetSequenceNumber)(THIS_
                                 UINT16 uStreamNumber,
                                 UINT16& uSequenceNumber);

    STDMETHOD(Pause)(THIS_
                     UINT32 ulPausePoint);

    STDMETHOD(StreamDone)(THIS_
                          UINT16 uStreamNumber, BOOL bForce = FALSE);

    STDMETHOD(SetDropRate)(THIS_
                           UINT16 uStreamNumber, 
                           UINT32 uDropRate);

    STDMETHOD(SetDropToBandwidthLimit)(THIS_
                                       UINT16 uStreamNumber,
                                       UINT32 ulBandwidthLimit);

    STDMETHOD(SetDeliveryBandwidth)(THIS_
                                    UINT32 ulBackOff,
                                    UINT32 ulBandwidth);

    STDMETHOD(SetBandwidth)(UINT32 ulBandwidth) { return HXR_NOTIMPL; }

    STDMETHOD(HandleSubscribe)(THIS_
                               INT32 lRuleNumber,
                               UINT16 unStreamNumber);

    STDMETHOD(HandleUnSubscribe)(THIS_
                                 INT32 lRuleNumber,
                                 UINT16 unStreamNumber);

    STDMETHOD_(ULONG32, GetDeliveryRate)(THIS) { return m_ulSubscribedRate;};

    STDMETHOD(ControlDone)(THIS);

    STDMETHOD_(float, SetSpeed)(THIS_
                                float fSpeed);

// Methods
	UINT16 GetStreamCount() const { return m_unStreamCount; }

protected:
    Process*      m_pProc;
    UINT16        m_unStreamCount;
    PacketFlowManager* m_pFlowMgr;
    IHXSessionStats* m_pStats;
    IHXBuffer* m_pbufSessionId;
    PacketStream** m_ppStreams;

private:
    ULONG32       m_ulRefCount;
    
    UINT32	  m_ulActualDeliveryRate;
    Timeval       m_tNextSendTime;
    BOOL          m_bDeliveryBandwidthSet;
    UINT32        m_ulSubscribedRate;
    float         m_fDeliveryRatio;
    UINT32        m_ulDeliveryRate;
};

#endif

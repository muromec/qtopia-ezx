#ifndef _HXQOSSESS_H_
#define _HXQOSSESS_H_

_INTERFACE IHXSessionStats;
_INTERFACE IHXBuffer;
_INTERFACE IHXServerPacketSource;

// {0A7A1AB2-6E3E-4ed4-800F-97D54E67EE3C}
DEFINE_GUID(IID_IHXClientBufferInfo, 
0xa7a1ab2, 0x6e3e, 0x4ed4, 0x80, 0xf, 0x97, 0xd5, 0x4e, 0x67, 0xee, 0x3c);

#define CLSID_IHXClientBufferInfo	IID_IHXClientBufferInfo

#undef  INTERFACE
#define INTERFACE   IHXClientBufferInfo

DECLARE_INTERFACE_(IHXClientBufferInfo, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

   /*
    *   IHXClientBufferInfo methods
    */
    STDMETHOD_(BOOL, IsBlocked)() const PURE;
    STDMETHOD(GetBufferDepth) (REF(ULONG32) ulfirstPTS, REF(ULONG32) ulduration) const PURE;
    STDMETHOD(GetBytesInUse) (REF(UINT32) ulbufusage) const PURE;
};

// {51D547A0-F019-49e8-B498-50DF22F7EB6B}
DEFINE_GUID(IID_IHXQoSClientBufferVerifier, 
0x51d547a0, 0xf019, 0x49e8, 0xb4, 0x98, 0x50, 0xdf, 0x22, 0xf7, 0xeb, 0x6b);

#define CLSID_IHXQoSClientBufferVerifier	IID_IHXQoSClientBufferVerifier

#undef  INTERFACE
#define INTERFACE   IHXQoSClientBufferVerifier

class ServerPacket;

DECLARE_INTERFACE_(IHXQoSClientBufferVerifier, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;
   /*
    *   IHXQosClientBufferVerifier methods
    */
    STDMETHOD(Verify) (THIS_ ServerPacket* pPacket) PURE;
    STDMETHOD(Reset) (THIS) PURE;
    STDMETHOD(TimeLineSuspended) (BOOL bSuspend) PURE;
    STDMETHOD(VerifyResend) (THIS_ ServerPacket* pPacket) PURE;
};

// {8FCE6B44-6067-4060-8675-D8B56D059592}
DEFINE_GUID(IID_IHXQoSRateManager, 
0x8fce6b44, 0x6067, 0x4060, 0x86, 0x75, 0xd8, 0xb5, 0x6d, 0x5, 0x95, 0x92);

#define CLSID_IHXQoSRateManager   IID_IHXQoSRateManager

#undef INTERFACE
#define INTERFACE IHXQoSRateManager

class PacketStream;

DECLARE_INTERFACE_(IHXQoSRateManager, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXQoSRateManager methods
     */
    STDMETHOD(Init) (THIS_ IHXSessionStats* pStats, IHXBuffer* pbufsessionId, UINT16 unStreamCount, IHXServerPacketSource*  pSource) PURE;
    STDMETHOD(VerifyDelivery) (THIS_ ServerPacket* pPacket, Timeval& ulTime) PURE;
    STDMETHOD_(ULONG32, GetActualDeliveryRate) (THIS) const PURE;
    STDMETHOD(TimeLineStateChanged) (THIS_ BOOL bStart) PURE;
    STDMETHOD(RegisterStream) (THIS_ PacketStream* pStreamData, UINT16 ulStreamNumber) PURE;    
    STDMETHOD(Done) (THIS) PURE;
};

#endif // _HXQOSSESS_H_

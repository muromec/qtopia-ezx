///////////////////////////////////////////////////////////////////////////////
// INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "servlist.h"
#include "ihxpckts.h"
#include "id.h"

#include "hxqos.h"
#include "hxqosinfo.h"
#include "hxqossig.h"
#include "servsked.h"
#include "servbuffer.h"
#include "dispatchq.h"
#include "proc.h"
#include "proc_container.h"
#include "hxclientprofile.h"
#include "hxtick.h"

#include "mutex.h"
#include "hxstats.h"

#define INITGUID
#include "hxprivstats.h"
#include "server_stats.h"


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::ClipStats()
///////////////////////////////////////////////////////////////////////////////

ClipStats::ClipStats()
: m_RefCount(0)
, m_ulStatus(0)
, m_pSession(NULL)
, m_pURL(NULL)
, m_pLogURL(NULL)
, m_pPlayerRequestedURL(NULL)
, m_ulBytesSent(0)
, m_ulSuccessfulResends(0)
, m_ulFailedResends(0)
, m_ulPacketsSent(0)
, m_ulPacketLoss(0)
, m_ulFileSize(0)
, m_pAbsStartTime(NULL)
, m_ulNPTStartTime(0)
, m_ulNPTEndTime(0)
, m_ulDuration(0)
, m_ulSendingTime(0)
, m_ulPlayTime(0)
, m_ulAvgBitrate(0)
, m_ulEstPlayerBufferUnderruns(0)
, m_ulSwitchCount(0)
, m_bEndFlag(FALSE)
, m_pOriginPlaylist(NULL)
{
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::ClipStats()
///////////////////////////////////////////////////////////////////////////////

ClipStats::~ClipStats()
{
    HX_RELEASE(m_pURL);
    HX_RELEASE(m_pLogURL);
    HX_RELEASE(m_pPlayerRequestedURL);
    HX_RELEASE(m_pAbsStartTime);
    HX_RELEASE(m_pOriginPlaylist);
}


///////////////////////////////////////////////////////////////////////////////
// IUnknown::AddRef()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClipStats::AddRef(void)
{
    return InterlockedIncrement(&m_RefCount);
}


///////////////////////////////////////////////////////////////////////////////
// IUnknown::Release()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClipStats::Release(void)
{
    if (InterlockedDecrement(&m_RefCount) > 0)
    {
        return m_RefCount;
    }

    delete this;
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// IUnknown::QueryInterface()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::QueryInterface(REFIID riid,
                             void** ppInterfaceObj)
{
// By definition all COM objects support the IUnknown interface
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)(IHXClipStats*)this;
        return HXR_OK;
    }

// IHXClipStats interface is supported
    else if (IsEqualIID(riid, IID_IHXClipStats))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)(IHXClipStats*)this;
        return HXR_OK;
    }
    // IHXClipStats2 interface is supported
    else if (IsEqualIID(riid, IID_IHXClipStats2))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)(IHXClipStats2*)this;
        return HXR_OK;
    }
// No other interfaces are supported
    *ppInterfaceObj = NULL;
    return HXR_NOINTERFACE;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetSession()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXSessionStats*)
ClipStats::GetSession()
{
    if (m_pSession)
    {
        m_pSession->AddRef();
    }
    return m_pSession;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetSession()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetSession(IHXSessionStats* pSession)
{
    //to avoid circular reference counting do not AddRef the Session*
    m_pSession = pSession;

    //Hack: This implementation is just for the FCS bubbler release, 
    //we need to come up with some better solution. Right now the problem is that QoSSignalBus doest have a way to acess ClipStats.
    //So all transport level variables in clipstats can not be updated by QosSignalBus or any of its caller.
    //So the temporary solution is to have clipstats save the state to transport variables when initialized, 
    //and return the diffrence from the actual tranport values by subtracting the saved values. 
    //The better solution would be to extend the SessionStats to do a job of QosSignalBus.

    IHXQoSTransportAdaptationInfo* pQosTransportAdaptationInfo = NULL;
    pQosTransportAdaptationInfo = m_pSession->GetQoSTransportAdaptationInfo();
    
    IHXQoSSessionAdaptationInfo*   pQosSessionAdaptationInfo   = NULL;
    pQosSessionAdaptationInfo = m_pSession->GetQoSSessionAdaptationInfo();

    if (pQosTransportAdaptationInfo)
    {
        m_ulBytesSent         = pQosTransportAdaptationInfo->GetBytesSent();
        m_ulSuccessfulResends = pQosTransportAdaptationInfo->GetSuccessfulResends();
        m_ulFailedResends     = pQosTransportAdaptationInfo->GetFailedResends();
        m_ulPacketsSent       = pQosTransportAdaptationInfo->GetPacketsSent();
        m_ulPacketLoss        = pQosTransportAdaptationInfo->GetPacketLoss();
    }

    //In case of HTTPQoSAdaptation there is no QoSSessionAdaptation, so adding gaurds here.
    if (pQosSessionAdaptationInfo)
    {
        m_ulEstPlayerBufferUnderruns = pQosSessionAdaptationInfo->GetEstimatedPlayerBufferUnderruns();
    }

    HX_RELEASE(pQosTransportAdaptationInfo);
    HX_RELEASE(pQosSessionAdaptationInfo);

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetStatus()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClipStats::GetStatus()
{
    return m_ulStatus;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetStatus()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetStatus(UINT32 ulStatus)
{
    m_ulStatus = ulStatus;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetURL()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClipStats::GetURL()
{
    if (m_pURL)
    {
        m_pURL->AddRef();
    }
    return m_pURL;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetURL()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetURL(IHXBuffer* pURL)
{
    IHXBuffer* pTemp = m_pURL;
    m_pURL = pURL;
    HX_RELEASE(pTemp);
    if (m_pURL)
    {
        m_pURL->AddRef();
    }

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetLogURL()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClipStats::GetLogURL()
{
    if (m_pLogURL)
    {
        m_pLogURL->AddRef();
    }
    return m_pLogURL;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetLogURL()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetLogURL(IHXBuffer* pLogURL)
{
    IHXBuffer* pTemp = m_pLogURL;
    m_pLogURL = pLogURL;
    HX_RELEASE(pTemp);
    if (m_pLogURL)
    {
        m_pLogURL->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetPlayerRequestedURL()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClipStats::GetPlayerRequestedURL()
{
    if (m_pPlayerRequestedURL)
    {
        m_pPlayerRequestedURL->AddRef();
    }
    return m_pPlayerRequestedURL;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetPlayerRequestedURL()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetPlayerRequestedURL(IHXBuffer* pPlayerRequestedURL)
{
    IHXBuffer* pTemp = m_pPlayerRequestedURL;
    m_pPlayerRequestedURL = pPlayerRequestedURL;
    HX_RELEASE(pTemp);
    if (m_pPlayerRequestedURL)
    {
        m_pPlayerRequestedURL->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetBytesSent()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT64)
ClipStats::GetBytesSent()
{
    IHXQoSTransportAdaptationInfo* pQosTransportAdaptationInfo = m_pSession->GetQoSTransportAdaptationInfo();
    HX_ASSERT(pQosTransportAdaptationInfo);
    UINT64 ulActualBytesSent = pQosTransportAdaptationInfo->GetBytesSent();
    HX_RELEASE(pQosTransportAdaptationInfo);

    return ulActualBytesSent - m_ulBytesSent;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetBytesSent()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetBytesSent(UINT64 ulBytesSent)
{
    m_ulBytesSent = ulBytesSent;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetSuccessfulResends()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClipStats::GetSuccessfulResends()
{
    IHXQoSTransportAdaptationInfo* pQosTransportAdaptationInfo = m_pSession->GetQoSTransportAdaptationInfo();
    HX_ASSERT(pQosTransportAdaptationInfo);
    UINT32 ulActualSuccessfulResends = pQosTransportAdaptationInfo->GetSuccessfulResends();
    HX_RELEASE(pQosTransportAdaptationInfo);

    return ulActualSuccessfulResends - m_ulSuccessfulResends;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetSuccessfulResends()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetSuccessfulResends(UINT32 ulSuccessfulResends)
{
    m_ulSuccessfulResends = ulSuccessfulResends;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetFailedResends()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClipStats::GetFailedResends()
{
    IHXQoSTransportAdaptationInfo* pQosTransportAdaptationInfo = m_pSession->GetQoSTransportAdaptationInfo();
    HX_ASSERT(pQosTransportAdaptationInfo);
    UINT32 ulActualFailedResends = pQosTransportAdaptationInfo->GetFailedResends();
    HX_RELEASE(pQosTransportAdaptationInfo);

    return ulActualFailedResends - m_ulFailedResends;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetFailedResends()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetFailedResends(UINT32 ulFailedResends)
{
    m_ulFailedResends = ulFailedResends;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetPacketsSent()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClipStats::GetPacketsSent()
{
    IHXQoSTransportAdaptationInfo* pQosTransportAdaptationInfo = m_pSession->GetQoSTransportAdaptationInfo();
    HX_ASSERT(pQosTransportAdaptationInfo);
    UINT32 ulActualPacketsSent = pQosTransportAdaptationInfo->GetPacketsSent();
    HX_RELEASE(pQosTransportAdaptationInfo);

    return ulActualPacketsSent - m_ulPacketsSent;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetPacketsSent()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetPacketsSent(UINT32 ulPacketsSent)
{
    m_ulPacketsSent = ulPacketsSent;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetPacketLoss()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClipStats::GetPacketLoss()
{
    IHXQoSTransportAdaptationInfo* pQosTransportAdaptationInfo = m_pSession->GetQoSTransportAdaptationInfo();
    HX_ASSERT(pQosTransportAdaptationInfo);
    UINT32 ulActualPacketLoss = pQosTransportAdaptationInfo->GetPacketLoss();
    HX_RELEASE(pQosTransportAdaptationInfo);

    return ulActualPacketLoss - m_ulPacketLoss;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetPacketsLoss()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetPacketLoss(UINT32 ulPacketLoss)
{
    m_ulPacketLoss = ulPacketLoss;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetFileSize()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT64)
ClipStats::GetFileSize()
{
    return m_ulFileSize;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetFileSize()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetFileSize(UINT64 ulFileSize)
{
    m_ulFileSize = ulFileSize;
    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetStartTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClipStats::GetStartTime()
{
    if (m_pAbsStartTime)
    {
        m_pAbsStartTime->AddRef();
    }
    return m_pAbsStartTime;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetStartTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetStartTime(IHXBuffer* pStartTime)
{
    IHXBuffer* pTemp = m_pAbsStartTime;
    m_pAbsStartTime = pStartTime;
    HX_RELEASE(pTemp);
    if (m_pAbsStartTime)
    {
        m_pAbsStartTime->AddRef();
    }
    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetNPTStartTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClipStats::GetNPTStartTime()
{
    return m_ulNPTStartTime;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetNPTStartTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetNPTStartTime(UINT32 ulStartTime)
{
    m_ulNPTStartTime = ulStartTime;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetNPTEndTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClipStats::GetNPTEndTime()
{
    return m_ulNPTEndTime;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetNPTEndTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetNPTEndTime(UINT32 ulEndTime)
{
    m_ulNPTEndTime = ulEndTime;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetDuration()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClipStats::GetDuration()
{
    return m_ulDuration;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetDuration()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetDuration(UINT32 ulDuration)
{
    m_ulDuration = ulDuration;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetSendingTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClipStats::GetSendingTime()
{
    return m_ulSendingTime;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetSendingTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetSendingTime(UINT32 ulSendingTime)
{
    m_ulSendingTime = ulSendingTime;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetPlayTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClipStats::GetPlayTime()
{
    return m_ulPlayTime;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetPlayTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetPlayTime(UINT32 ulPlayTime)
{
    m_ulPlayTime = ulPlayTime;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetAvgBitrate()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClipStats::GetAvgBitrate()
{
    return m_ulAvgBitrate;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetAvgBitrate()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetAvgBitrate(UINT32 ulAvgBitrate)
{
    m_ulAvgBitrate = ulAvgBitrate;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetEstPlayerBufferUnderruns()
///////////////////////////////////////////////////////////////////////////////

//In case of HTTPQoSAdaptation there is no QoSSessionAdaptation, so adding gaurds to this method.
STDMETHODIMP_(UINT32)
ClipStats::GetEstPlayerBufferUnderruns()
{
    IHXQoSSessionAdaptationInfo* pQosSessionAdaptationInfo = m_pSession->GetQoSSessionAdaptationInfo();
    UINT32 ulActualEstPlayerBufferUnderruns = 0;

    if (pQosSessionAdaptationInfo)
    {
        ulActualEstPlayerBufferUnderruns = pQosSessionAdaptationInfo->GetEstimatedPlayerBufferUnderruns();
        HX_RELEASE(pQosSessionAdaptationInfo);
        return ulActualEstPlayerBufferUnderruns - m_ulEstPlayerBufferUnderruns;
    }
    else
    {
        return m_ulEstPlayerBufferUnderruns;
    }
   
}

///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetSwitchCount()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClipStats::GetSwitchCount()
{
    return m_ulSwitchCount;
}

///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetSwitchCount()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetSwitchCount(UINT32 ulSwitchCount)
{
    m_ulSwitchCount = ulSwitchCount;
    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetEstPlayerBufferUnderruns()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetEstPlayerBufferUnderruns(UINT32 ulEstPlayerBufferUnderruns)
{
    m_ulEstPlayerBufferUnderruns = ulEstPlayerBufferUnderruns;
    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
// IHXClipStats2::GetOriginPlaylist()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClipStats::GetOriginPlaylist()
{
    if (m_pOriginPlaylist)
    {
        m_pOriginPlaylist->AddRef();
    }
    return m_pOriginPlaylist;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClipStats2::SetOriginPlaylist()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetOriginPlaylist(IHXBuffer* pOriginPlaylist)
{
    IHXBuffer* pTemp = m_pOriginPlaylist;
    m_pOriginPlaylist = pOriginPlaylist;
    HX_RELEASE(pTemp);
    if (m_pOriginPlaylist)
    {
        m_pOriginPlaylist->AddRef();
    }

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// SessionStats::SessionStats()
///////////////////////////////////////////////////////////////////////////////

SessionStats::SessionStats()
: m_RefCount(0)
, m_ulID(0)
, m_pClient(NULL)
, m_pHost(NULL)
, m_pSessionStartTime(NULL)
, m_pURL(NULL)
, m_pLogStats(NULL)
, m_pPlayerRequestedURL(NULL)
, m_pSalt(NULL)
, m_pAuth(NULL)
, m_pProxyConnectionType(NULL)
, m_pInterfaceAddr(NULL)
, m_pClientProfileURIs(NULL)
, m_ulEndStatus(SSES_NOT_ENDED)
, m_ulDuration(0)
, m_ulSendingTime(0)
, m_ulPlayTime(0)
, m_bIsUDP(FALSE)
, m_bUseMDP(FALSE)
, m_bIsRVStreamFound(FALSE)
, m_bIsRAStreamFound(FALSE)
, m_bIsREStreamFound(FALSE)
, m_bIsRIStreamFound(FALSE)
, m_bIsMulticastUsed(FALSE)
, m_bUpdateRegistryForLive(FALSE)
, m_uiXWapProfileStatus(0)
, m_pClientProfileInfo(NULL)
, m_pQoSTransportAdaptationInfo(NULL)
, m_pQoSSessionAdaptationInfo(NULL)
, m_pQoSApplicationAdaptationInfo(NULL)
, m_ulConnectTime(0)
, m_ulSessionEstablishmentTime(0)
, m_ulSessionSetupTime(0)
, m_ulFirstPacketTime(0)
, m_ulPreDataTime(0)
, m_ulPreDataBytes(0)
, m_ulPrerollInMsec(0)
, m_pSessionControlId(NULL)
, m_pClip(NULL)
, m_pSessionStartTicks(0)
, m_ulSwitchCount(0)
, m_ulClipCount(0)
{
    IHXBuffer* pUnknown = NULL;
    char* szLogStats = new char[BRACKETED_UNKNOWN_LENGTH + 1];
    strncpy(szLogStats, BRACKETED_UNKNOWN, BRACKETED_UNKNOWN_LENGTH);
    szLogStats[BRACKETED_UNKNOWN_LENGTH] = '\0';

// Just in case we need this string anywhere else.
    pUnknown = new ServerBuffer((UCHAR*)szLogStats, BRACKETED_UNKNOWN_LENGTH + 1);
    m_pLogStats = pUnknown;
    m_pLogStats->AddRef();
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SessionStats()
///////////////////////////////////////////////////////////////////////////////

SessionStats::~SessionStats()
{
    HX_RELEASE(m_pHost);
    HX_RELEASE(m_pSessionStartTime);
    HX_RELEASE(m_pURL);
    HX_RELEASE(m_pLogStats);
    HX_RELEASE(m_pPlayerRequestedURL);
    HX_RELEASE(m_pSalt);
    HX_RELEASE(m_pAuth);
    HX_RELEASE(m_pInterfaceAddr);
    HX_RELEASE(m_pProxyConnectionType);
    HX_RELEASE(m_pClientProfileURIs);
    HX_RELEASE(m_pClientProfileInfo);
    HX_RELEASE(m_pQoSTransportAdaptationInfo);
    HX_RELEASE(m_pQoSSessionAdaptationInfo);
    HX_RELEASE(m_pQoSApplicationAdaptationInfo);
    HX_RELEASE(m_pSessionControlId);
    HX_RELEASE(m_pClip);
}


///////////////////////////////////////////////////////////////////////////////
// IUnknown::AddRef()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
SessionStats::AddRef(void)
{
    return InterlockedIncrement(&m_RefCount);
}


///////////////////////////////////////////////////////////////////////////////
// IUnknown::Release()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
SessionStats::Release(void)
{
    if (InterlockedDecrement(&m_RefCount) > 0)
    {
        return m_RefCount;
    }

    delete this;
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// IUnknown::QueryInterface()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::QueryInterface(REFIID riid,
                             void** ppInterfaceObj)
{

// By definition all COM objects support the IUnknown interface
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)(IHXSessionStats*)this;
        return HXR_OK;
    }

// IHXSessionStats interface is supported
    else if (IsEqualIID(riid, IID_IHXSessionStats))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)(IHXSessionStats*)this;
        return HXR_OK;
    }

// IHXSessionStats2 interface is supported
    else if (IsEqualIID(riid, IID_IHXSessionStats2))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)(IHXSessionStats2*)this;
        return HXR_OK;
    }
// IHXSessionStats3 interface is supported
    else if (IsEqualIID(riid, IID_IHXSessionStats3))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)(IHXSessionStats3*)this;
        return HXR_OK;
    }
// IHXSessionStats4 interface is supported
    else if (IsEqualIID(riid, IID_IHXSessionStats4))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)(IHXSessionStats4*)this;
        return HXR_OK;
    }

// IHXCheckRetainEntityForSetup interface is supported
    else if (IsEqualIID(riid, IID_IHXCheckRetainEntityForSetup))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)(IHXCheckRetainEntityForSetup*)this;
        return HXR_OK;
    }

// No other interfaces are supported
    *ppInterfaceObj = NULL;
    return HXR_NOINTERFACE;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetClient()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXClientStats*)
SessionStats::GetClient()
{
    if (m_pClient)
    {
        m_pClient->AddRef();
    }
    return m_pClient;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetClient()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetClient(IHXClientStats* pClient)
{
    //to avoid circular reference counting do not AddRef the Client*
    m_pClient = pClient;

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetHost()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
SessionStats::GetHost()
{
    if (m_pHost)
    {
        m_pHost->AddRef();
    }
    return m_pHost;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetHost()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetHost(IHXBuffer* pHost)
{
    IHXBuffer* pTemp = m_pHost;
    m_pHost = pHost;
    HX_RELEASE(pTemp);
    if (m_pHost)
    {
        m_pHost->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetSessionStartTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
SessionStats::GetSessionStartTime()
{
    if (m_pSessionStartTime)
    {
        m_pSessionStartTime->AddRef();
    }
    return m_pSessionStartTime;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetSessionStartTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetSessionStartTime(IHXBuffer* pSessionStartTime)
{
    IHXBuffer* pTemp = m_pSessionStartTime;
    m_pSessionStartTime = pSessionStartTime;
    HX_RELEASE(pTemp);
    if (m_pSessionStartTime)
    {
        m_pSessionStartTime->AddRef();
    }
    
    m_pSessionStartTicks = HX_GET_TICKCOUNT();

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetURL()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
SessionStats::GetURL()
{
    if (m_pURL)
    {
        m_pURL->AddRef();
    }
    return m_pURL;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetURL()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetURL(IHXBuffer* pURL)
{
    IHXBuffer* pTemp = m_pURL;
    m_pURL = pURL;
    HX_RELEASE(pTemp);
    if (m_pURL)
    {
        m_pURL->AddRef();
    }

    //notify sinks
    IHXClientStatsManager* pStatsMgr = NULL;
    if (m_pClient && (pStatsMgr = m_pClient->GetStatsManager()))
    {
        pStatsMgr->ScheduleSinkNotifications(m_pClient, this,
                                            CSEVENT_SESSION_SETURL);
    }
    HX_RELEASE(pStatsMgr);

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetLogURL()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
SessionStats::GetLogURL()
{
    return (m_pClip) ? m_pClip->GetLogURL() : NULL;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetLogURL()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetLogURL(IHXBuffer* pLogURL)
{
    if (m_pClip)
    {
        m_pClip->SetLogURL(pLogURL);
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetLogStats()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
SessionStats::GetLogStats()
{
    if (m_pLogStats)
    {
        m_pLogStats->AddRef();
    }
    return m_pLogStats;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetLogStats()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetLogStats(IHXBuffer* pLogStats)
{
    IHXBuffer* pTemp = m_pLogStats;
    m_pLogStats = pLogStats;
    HX_RELEASE(pTemp);
    if (m_pLogStats)
    {
        m_pLogStats->AddRef();
    }

//XXXDPL Might need to notify on update of this property!

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetPlayerRequestedURL()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
SessionStats::GetPlayerRequestedURL()
{
    if (m_pPlayerRequestedURL)
    {
        m_pPlayerRequestedURL->AddRef();
    }
    return m_pPlayerRequestedURL;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetPlayerRequestedURL()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetPlayerRequestedURL(IHXBuffer* pPlayerRequestedURL)
{
    IHXBuffer* pTemp = m_pPlayerRequestedURL;
    m_pPlayerRequestedURL = pPlayerRequestedURL;
    HX_RELEASE(pTemp);
    if (m_pPlayerRequestedURL)
    {
        m_pPlayerRequestedURL->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetSalt()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
SessionStats::GetSalt()
{
    if (m_pSalt)
    {
        m_pSalt->AddRef();
    }
    return m_pSalt;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetSalt()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetSalt(IHXBuffer* pSalt)
{
    IHXBuffer* pTemp = m_pSalt;
    m_pSalt = pSalt;
    HX_RELEASE(pTemp);
    if (m_pSalt)
    {
        m_pSalt->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetAuth()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
SessionStats::GetAuth()
{
    if (m_pAuth)
    {
        m_pAuth->AddRef();
    }
    return m_pAuth;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetAuth()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetAuth(IHXBuffer* pAuth)
{
    IHXBuffer* pTemp = m_pAuth;
    m_pAuth = pAuth;
    HX_RELEASE(pTemp);
    if (m_pAuth)
    {
        m_pAuth->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetProxyConnectionType()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
SessionStats::GetProxyConnectionType()
{
    if (m_pProxyConnectionType)
    {
        m_pProxyConnectionType->AddRef();
    }
    return m_pProxyConnectionType;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetProxyConnectionType()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetProxyConnectionType(IHXBuffer* pProxyConnectionType)
{
    IHXBuffer* pTemp = m_pProxyConnectionType;
    m_pProxyConnectionType = pProxyConnectionType;
    HX_RELEASE(pTemp);
    if (m_pProxyConnectionType)
    {
        m_pProxyConnectionType->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetInterfaceAddr()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
SessionStats::GetInterfaceAddr()
{
    if (m_pInterfaceAddr)
    {
        m_pInterfaceAddr->AddRef();
    }
    return m_pInterfaceAddr;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetInterfaceAddr()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetInterfaceAddr(IHXBuffer* pInterfaceAddr)
{
    IHXBuffer* pTemp = m_pInterfaceAddr;
    m_pInterfaceAddr = pInterfaceAddr;
    HX_RELEASE(pTemp);
    if (m_pInterfaceAddr)
    {
        m_pInterfaceAddr->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetID()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
SessionStats::GetID()
{
    return m_ulID;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetID()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetID(UINT32 ulID)
{
    m_ulID = ulID;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetFileSize()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT64)
SessionStats::GetFileSize()
{
    return (m_pClip) ? m_pClip->GetFileSize() : 0;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetFileSize()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetFileSize(UINT64 ulFileSize)
{
    if (m_pClip)
    {
        m_pClip->SetFileSize(ulFileSize);
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetStatus()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
SessionStats::GetStatus()
{
    return (m_pClip) ? m_pClip->GetStatus() : 0;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetStatus()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetStatus(UINT32 ulStatus)
{
    if (m_pClip)
    {
        m_pClip->SetStatus(ulStatus);
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetEndStatus()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(SessionStatsEndStatus)
SessionStats::GetEndStatus()
{
    return m_ulEndStatus;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetEndStatus()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetEndStatus(SessionStatsEndStatus ulStatus)
{
    // This should only be set once.
    HX_ASSERT(m_ulEndStatus == SSES_NOT_ENDED);

    m_ulEndStatus = ulStatus;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetDuration()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
SessionStats::GetDuration()
{
    return m_ulDuration;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetDuration()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetDuration(UINT32 ulDuration)
{
    m_ulDuration = ulDuration;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetAvgBitrate()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
SessionStats::GetAvgBitrate()
{
    return (m_pClip) ? m_pClip->GetAvgBitrate() : 0;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetAvgBitrate()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetAvgBitrate(UINT32 ulAvgBitrate)
{
    if (m_pClip)
    {
        m_pClip->SetAvgBitrate(ulAvgBitrate);
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetSendingTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
SessionStats::GetSendingTime()
{
    if (m_pSessionStartTicks)
    {
        m_ulSendingTime = CALCULATE_ELAPSED_TICKS(m_pSessionStartTicks, HX_GET_TICKCOUNT()) / 1000;
    }
    return m_ulSendingTime;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetSendingTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetSendingTime(UINT32 ulSendingTime)
{
    m_ulSendingTime = ulSendingTime;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetPlayTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
SessionStats::GetPlayTime()
{
    return m_ulPlayTime;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetPlayTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetPlayTime(UINT32 ulPlayTime)
{
    m_ulPlayTime = ulPlayTime;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::IsUDP()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL)
SessionStats::IsUDP()
{
    return m_bIsUDP;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetUDP()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetUDP(BOOL bIsUDP)
{
    m_bIsUDP = bIsUDP;
    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::IsUseMDP()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL)
SessionStats::IsUseMDP()
{
    return m_bUseMDP;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetUDP()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetUseMDP(BOOL bUseMDP)
{
    m_bUseMDP = bUseMDP;
    return HXR_OK;
}



///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::IsRVStreamFound()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL)
SessionStats::IsRVStreamFound()
{
    return m_bIsRVStreamFound;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetRVStreamFound()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetRVStreamFound(BOOL bIsRVStreamFound)
{
    m_bIsRVStreamFound = bIsRVStreamFound;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::IsRAStreamFound()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL)
SessionStats::IsRAStreamFound()
{
    return m_bIsRAStreamFound;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetRAStreamFound()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetRAStreamFound(BOOL bIsRAStreamFound)
{
    m_bIsRAStreamFound = bIsRAStreamFound;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::IsREStreamFound()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL)
SessionStats::IsREStreamFound()
{
    return m_bIsREStreamFound;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetREStreamFound()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetREStreamFound(BOOL bIsREStreamFound)
{
    m_bIsREStreamFound = bIsREStreamFound;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::IsRIStreamFound()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL)
SessionStats::IsRIStreamFound()
{
    return m_bIsRIStreamFound;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetRIStreamFound()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetRIStreamFound(BOOL bIsRIStreamFound)
{
    m_bIsRIStreamFound = bIsRIStreamFound;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::IsMulticastUsed()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL)
SessionStats::IsMulticastUsed()
{
    return m_bIsMulticastUsed;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetMulticastUsed()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetMulticastUsed(BOOL bIsMulticastUsed)
{
    m_bIsMulticastUsed = bIsMulticastUsed;
    return HXR_OK;
}





///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetXWapProfileStatus()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT16)
SessionStats::GetXWapProfileStatus()
{
    return m_uiXWapProfileStatus;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetXWapProfileStatus()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetXWapProfileStatus(UINT16 uiXWapProfileStatus)
{
    m_uiXWapProfileStatus = uiXWapProfileStatus;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetClientProfileInfo()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::GetClientProfileInfo(REF(IHXClientProfileInfo*) pInfo)
{
    if (m_pClientProfileInfo)
    {
        m_pClientProfileInfo->AddRef();
        pInfo = m_pClientProfileInfo;
        return HXR_OK;
    }

    return HXR_FAIL;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetClientProfileInfo()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetClientProfileInfo(IHXClientProfileInfo* pClientProfileInfo)
{
    IHXClientProfileInfo* pTemp = m_pClientProfileInfo;
    m_pClientProfileInfo = pClientProfileInfo;
    HX_RELEASE(pTemp);
    if (m_pClientProfileInfo)
    {
        m_pClientProfileInfo->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetClientProfileURIs()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
SessionStats::GetClientProfileURIs()
{
    if (m_pClientProfileURIs)
    {
        m_pClientProfileURIs->AddRef();
    }
    return m_pClientProfileURIs;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetClientProfileURIs()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetClientProfileURIs(IHXBuffer* pClientProfileURIs)
{
    IHXBuffer* pTemp = m_pClientProfileURIs;
    m_pClientProfileURIs = pClientProfileURIs;
    HX_RELEASE(pTemp);
    if (m_pClientProfileURIs)
    {
        m_pClientProfileURIs->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetQoSTransportAdaptationInfo()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXQoSTransportAdaptationInfo*)
SessionStats::GetQoSTransportAdaptationInfo()
{
    if (m_pQoSTransportAdaptationInfo)
    {
        m_pQoSTransportAdaptationInfo->AddRef();
    }
    return m_pQoSTransportAdaptationInfo;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetQoSTransportAdaptationInfo()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetQoSTransportAdaptationInfo(IHXQoSTransportAdaptationInfo* pInfo)
{
    IHXQoSTransportAdaptationInfo* pTemp = m_pQoSTransportAdaptationInfo;
    m_pQoSTransportAdaptationInfo = pInfo;
    HX_RELEASE(pTemp);
    if (m_pQoSTransportAdaptationInfo)
    {
        m_pQoSTransportAdaptationInfo->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetQoSSessionAdaptationInfo()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXQoSSessionAdaptationInfo*)
SessionStats::GetQoSSessionAdaptationInfo()
{
    if (m_pQoSSessionAdaptationInfo)
    {
        m_pQoSSessionAdaptationInfo->AddRef();
    }
    return m_pQoSSessionAdaptationInfo;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetQoSSessionAdaptationInfo()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetQoSSessionAdaptationInfo(IHXQoSSessionAdaptationInfo* pInfo)
{
    IHXQoSSessionAdaptationInfo* pTemp = m_pQoSSessionAdaptationInfo;
    m_pQoSSessionAdaptationInfo = pInfo;
    HX_RELEASE(pTemp);
    if (m_pQoSSessionAdaptationInfo)
    {
        m_pQoSSessionAdaptationInfo->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::GetQoSApplicationAdaptationInfo()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXQoSApplicationAdaptationInfo*)
SessionStats::GetQoSApplicationAdaptationInfo()
{
    if (m_pQoSApplicationAdaptationInfo)
    {
        m_pQoSApplicationAdaptationInfo->AddRef();
    }
    return m_pQoSApplicationAdaptationInfo;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats::SetQoSApplicationAdaptationInfo()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetQoSApplicationAdaptationInfo(IHXQoSApplicationAdaptationInfo* pInfo)
{
    IHXQoSApplicationAdaptationInfo* pTemp = m_pQoSApplicationAdaptationInfo;
    m_pQoSApplicationAdaptationInfo = pInfo;
    HX_RELEASE(pTemp);
    if (m_pQoSApplicationAdaptationInfo)
    {
        m_pQoSApplicationAdaptationInfo->AddRef();
    }
    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
//  IHXSessionStats4::GetSwitchCount()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
SessionStats::GetSwitchCount()
{
    return m_ulSwitchCount;
}

///////////////////////////////////////////////////////////////////////////////
//  IHXSessionStats4::SetSwitchCount()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetSwitchCount(UINT32 ulSwitchCount)
{
    m_ulSwitchCount = ulSwitchCount;
    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
//  IHXSessionStats4::GetClipCount()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
SessionStats::GetClipCount()
{
    return m_ulClipCount;
}

///////////////////////////////////////////////////////////////////////////////
//  IHXSessionStats4::SetClipCount()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetClipCount(UINT32 ulClipCount)
{
    m_ulClipCount = ulClipCount;
    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats2::GetConnectTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
SessionStats::GetConnectTime()
{
    return m_ulConnectTime;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats2::SetConnectTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetConnectTime(UINT32 ulConnectTime)
{
    m_ulConnectTime = ulConnectTime;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats2::GetSessionEstablishmentTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
SessionStats::GetSessionEstablishmentTime()
{
    return m_ulSessionEstablishmentTime;
}

///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats2::SetSessionEstablishmentTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetSessionEstablishmentTime(UINT32 ulSessionEstablishmentTime)
{
    m_ulSessionEstablishmentTime = ulSessionEstablishmentTime;
    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats2::GetSessionSetupTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
SessionStats::GetSessionSetupTime()
{
    return m_ulSessionSetupTime;
}

///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats2::SetSessionSetupTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetSessionSetupTime(UINT32 ulSessionSetupTime)
{
    m_ulSessionSetupTime = ulSessionSetupTime;
    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats2::GetFirstPacketTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
SessionStats::GetFirstPacketTime()
{
    return m_ulFirstPacketTime;
}

///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats2::SetFirstPacketTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetFirstPacketTime(UINT32 ulFirstPacketTime)
{
    m_ulFirstPacketTime = ulFirstPacketTime;
    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats2::GetPreDataTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
SessionStats::GetPreDataTime()
{
    return m_ulPreDataTime;
}

///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats2::SetPreDataTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetPreDataTime(UINT32 ulPreDataTime)
{
    m_ulPreDataTime = ulPreDataTime;
    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats2::GetPreDataBytes()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
SessionStats::GetPreDataBytes()
{
    return m_ulPreDataBytes;
}

///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats2::SetPreDataBytes()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetPreDataBytes(UINT32 ulPreDataBytes)
{
    m_ulPreDataBytes = ulPreDataBytes;
    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats2::GetPrerollInMsec()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
SessionStats::GetPrerollInMsec()
{
    return m_ulPrerollInMsec;
}

///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats2::SetPrerollInMsec()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetPrerollInMsec(UINT32 ulPrerollInMsec)
{
    m_ulPrerollInMsec = ulPrerollInMsec;
    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats2::DumpStartupInfo()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::DumpStartupInfo()
{
    UINT32 ulStart = m_ulConnectTime;

    if (!ulStart)
    {
        printf("\n\nServer Startup : Warning, Connection time not logged\n");
        ulStart = m_ulSessionEstablishmentTime;
    }

    printf("\n\nServer Startup Delays :\n");
    printf("    Session Establishment : %d ms\n", 
                                 (int)(m_ulSessionEstablishmentTime - ulStart));
    printf("    Session Setup : %d ms\n", (int)(m_ulSessionSetupTime - ulStart));
    printf("    First PacketReady: %d ms\n", 
                  m_ulFirstPacketTime ? (int)(m_ulFirstPacketTime - ulStart) : 0);

    if (m_ulPreDataTime)
    {
        printf("    PreData fulfilled %d ms after 1st PacketReady\n", (int)(m_ulPreDataTime - m_ulFirstPacketTime));
        printf("        PreData bytes : %d\n", (int)m_ulPreDataBytes);
        printf("        Preroll in Msec : %d\n\n", (int)m_ulPrerollInMsec);
    }
    else
    {
        printf("    Preroll information not available for this session\n");
    }

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXCheckRetainEntityForSetup::GetUpdateRegistryForLive()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL)
SessionStats::GetUpdateRegistryForLive()
{
    return m_bUpdateRegistryForLive;
}

///////////////////////////////////////////////////////////////////////////////
// IHXCheckRetainEntityForSetup::SetUpdateRegistryForLive()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetUpdateRegistryForLive()
{
    m_bUpdateRegistryForLive = TRUE;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats3::GetClip()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXClipStats*)
SessionStats::GetClip()
{
    if (m_pClip)
    {
        m_pClip->AddRef();
    }
    return m_pClip;
}

///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats3::SetClip()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetClip(IHXClipStats* pClip)
{
    HX_RELEASE(m_pClip);
    m_pClip = pClip;
    m_pClip->AddRef();

    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats3::GetSessionControlId()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
SessionStats::GetSessionControlId()
{
    if (m_pSessionControlId)
    {
        m_pSessionControlId->AddRef();
    }
    return m_pSessionControlId;
}


///////////////////////////////////////////////////////////////////////////////
// IHXSessionStats3::SetSessionControlId()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SessionStats::SetSessionControlId(IHXBuffer* pSessionControlId)
{
    IHXBuffer* pTemp = m_pSessionControlId;
    m_pSessionControlId = pSessionControlId;
    HX_RELEASE(pTemp);
    if (m_pSessionControlId)
    {
        m_pSessionControlId->AddRef();
    }
    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::GetEndFlag()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL)
ClipStats::GetEndFlag()
{
    return m_bEndFlag;
}

///////////////////////////////////////////////////////////////////////////////
// IHXClipStats::SetEndFlag()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClipStats::SetEndFlag(BOOL bEndFlag)
{
    m_bEndFlag = bEndFlag;
    return HXR_OK;
}



///////////////////////////////////////////////////////////////////////////////
// ClientStats::ClientStats()
///////////////////////////////////////////////////////////////////////////////

ClientStats::ClientStats(Process* pProc)
: m_pSessionTable(NULL)
, m_pProc(pProc)
, m_RefCount(0)
, m_pStatsMgr(NULL)
, m_ulID(0)
, m_lSessionCount(0)
, m_lSessionIndex(0)
, m_pIPAddress(NULL)
, m_pCBID(NULL)
, m_pGUID(NULL)
, m_pClientID(NULL)
, m_pPNAClientID(NULL)
, m_pCompanyID(NULL)
, m_pClientChallenge(NULL)
, m_pLanguage(NULL)
, m_pPlayerStartTime(NULL)
, m_pProtocol(NULL)
, m_pStartTime(NULL)
, m_pRequestMethod(NULL)
, m_pUserAgent(NULL)
, m_pVersion(NULL)
, m_pLoadTestPassword(NULL)
, m_pRTSPEvents(NULL)
, m_pStreamSelectionInfo(NULL)
, m_ulControlBytesSent(0)
, m_ulPort(0)
, m_ulTotalMediaAdaptations(0)
, m_ulReasonForTermination(0)
, m_bIsCloaked(FALSE)
, m_bIsRDT(FALSE)
, m_bSupportsMaximumASMBandwidth(FALSE)
, m_bSupportsMulticast(FALSE)
, m_bIsPrevAuth(FALSE)
, m_pMutex(NULL)
{
    m_pSessionTable = new CHXID(10);
    m_pTimerList = new HXList();
}


////////////////////////////////////////////////////////zz///////////////////////
// ClientStats::~ClientStats()
///////////////////////////////////////////////////////////////////////////////

ClientStats::~ClientStats()
{
    UINT32 i = 0;
    IHXSessionStats* pSession = NULL;

    for ( ; i < m_pSessionTable->get_size(); i++)
    {
        pSession = (IHXSessionStats*)m_pSessionTable->get(i);
        HX_RELEASE(pSession);
    }

    if (m_pTimerList)
    {
        ClientStatsTimer* pTemp = NULL;

        while (!m_pTimerList->empty())
        {
            pTemp = (ClientStatsTimer*)m_pTimerList->remove_head();
            HX_RELEASE(pTemp);
        }
    }


    HX_DELETE(m_pSessionTable);
    HX_DELETE(m_pTimerList);

    HX_RELEASE(m_pIPAddress);
    HX_RELEASE(m_pCBID);
    HX_RELEASE(m_pGUID);
    HX_RELEASE(m_pClientID);
    HX_RELEASE(m_pPNAClientID);
    HX_RELEASE(m_pCompanyID);
    HX_RELEASE(m_pClientChallenge);
    HX_RELEASE(m_pLanguage);
    HX_RELEASE(m_pPlayerStartTime);
    HX_RELEASE(m_pProtocol);
    HX_RELEASE(m_pStartTime);
    HX_RELEASE(m_pRequestMethod);
    HX_RELEASE(m_pUserAgent);
    HX_RELEASE(m_pVersion);
    HX_RELEASE(m_pLoadTestPassword);
    HX_RELEASE(m_pRTSPEvents);
    HX_RELEASE(m_pStreamSelectionInfo);
    HX_RELEASE(m_pStatsMgr);
}



///////////////////////////////////////////////////////////////////////////////
// IUnknown::AddRef()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClientStats::AddRef(void)
{
    return InterlockedIncrement(&m_RefCount);
}


///////////////////////////////////////////////////////////////////////////////
// IUnknown::Release()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClientStats::Release(void)
{
    if (InterlockedDecrement(&m_RefCount) > 0)
    {
        return m_RefCount;
    }

    delete this;
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// IUnknown::QueryInterface()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::QueryInterface(REFIID riid,
                            void** ppInterfaceObj)
{

// By definition all COM objects support the IUnknown interface
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)(IHXClientStats*)this;
        return HXR_OK;
    }

// IHXClientStats interface is supported
    else if (IsEqualIID(riid, IID_IHXClientStats))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)(IHXClientStats*)this;
        return HXR_OK;
    }

// IHXClientStats2 interface is supported
    else if (IsEqualIID(riid, IID_IHXClientStats2))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)(IHXClientStats2*)this;
        return HXR_OK;
    }

// IHXClientStatsTimerControl interface is supported
    else if (IsEqualIID(riid, IID_IHXClientStatsTimerControl))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)(IHXClientStatsTimerControl*)this;
        return HXR_OK;
    }

// IHXPrivateClientStats interface is supported
    else if (IsEqualIID(riid, IID_IHXPrivateClientStats))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)(IHXPrivateClientStats*)this;
        return HXR_OK;
    }

// No other interfaces are supported
    *ppInterfaceObj = NULL;
    return HXR_NOINTERFACE;

}

STDMETHODIMP_(IHXClientStatsManager*)
ClientStats::GetStatsManager()
{
    if (m_pStatsMgr)
    {
        m_pStatsMgr->AddRef();
    }
    return m_pStatsMgr;
}

STDMETHODIMP
ClientStats::SetStatsManager(IHXClientStatsManager* pMgr)
{
    m_pStatsMgr = pMgr;
    if (m_pStatsMgr)
    {
        m_pStatsMgr->AddRef();
    }

    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetID()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClientStats::GetID()
{
    return m_ulID;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetID()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetID(UINT32 ulID)
{
    m_ulID = ulID;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetIPAddress()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClientStats::GetIPAddress()
{
    if (m_pIPAddress)
    {
        m_pIPAddress->AddRef();
    }
    return m_pIPAddress;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetIPAddress()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetIPAddress(IHXBuffer* pIPAddress)
{
    IHXBuffer* pTemp = m_pIPAddress;
    m_pIPAddress = pIPAddress;
    HX_RELEASE(pTemp);
    if (m_pIPAddress)
    {
        m_pIPAddress->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetCBID()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClientStats::GetCBID()
{
    if (m_pCBID)
    {
        m_pCBID->AddRef();
    }
    return m_pCBID;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetCBID()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetCBID(IHXBuffer* pCBID)
{
    IHXBuffer* pTemp = m_pCBID;
    m_pCBID = pCBID;
    HX_RELEASE(pTemp);
    if (m_pCBID)
    {
        m_pCBID->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetGUID()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClientStats::GetGUID()
{
    if (m_pGUID)
    {
        m_pGUID->AddRef();
    }
    return m_pGUID;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetGUID()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetGUID(IHXBuffer* pGUID)
{
    IHXBuffer* pTemp = m_pGUID;
    m_pGUID = pGUID;
    HX_RELEASE(pTemp);
    if (m_pGUID)
    {
        m_pGUID->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetClientID()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClientStats::GetClientID()
{
    if (m_pClientID)
    {
        m_pClientID->AddRef();
    }
    return m_pClientID;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetClientID()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetClientID(IHXBuffer* pClientID)
{
    IHXBuffer* pTemp = m_pClientID;
    m_pClientID = pClientID;
    HX_RELEASE(pTemp);
    if (m_pClientID)
    {
        m_pClientID->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetPNAClientID()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClientStats::GetPNAClientID()
{
    if (m_pPNAClientID)
    {
        m_pPNAClientID->AddRef();
    }
    return m_pPNAClientID;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetPNAClientID()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetPNAClientID(IHXBuffer* pPNAClientID)
{
    IHXBuffer* pTemp = m_pPNAClientID;
    m_pPNAClientID = pPNAClientID;
    HX_RELEASE(pTemp);
    if (m_pPNAClientID)
    {
        m_pPNAClientID->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetCompanyID()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClientStats::GetCompanyID()
{
    if (m_pCompanyID)
    {
        m_pCompanyID->AddRef();
    }
    return m_pCompanyID;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetCompanyID()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetCompanyID(IHXBuffer* pCompanyID)
{
    IHXBuffer* pTemp = m_pCompanyID;
    m_pCompanyID = pCompanyID;
    HX_RELEASE(pTemp);
    if (m_pCompanyID)
    {
        m_pCompanyID->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetClientChallenge()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClientStats::GetClientChallenge()
{
    if (m_pClientChallenge)
    {
        m_pClientChallenge->AddRef();
    }
    return m_pClientChallenge;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetClientChallenge()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetClientChallenge(IHXBuffer* pClientChallenge)
{
    IHXBuffer* pTemp = m_pClientChallenge;
    m_pClientChallenge = pClientChallenge;
    HX_RELEASE(pTemp);
    if (m_pClientChallenge)
    {
        m_pClientChallenge->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetLanguage()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClientStats::GetLanguage()
{
    if (m_pLanguage)
    {
        m_pLanguage->AddRef();
    }
    return m_pLanguage;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetLanguage()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetLanguage(IHXBuffer* pLanguage)
{
    IHXBuffer* pTemp = m_pLanguage;
    m_pLanguage = pLanguage;
    HX_RELEASE(pTemp);
    if (m_pLanguage)
    {
        m_pLanguage->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetPlayerStartTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClientStats::GetPlayerStartTime()
{
    if (m_pPlayerStartTime)
    {
        m_pPlayerStartTime->AddRef();
    }
    return m_pPlayerStartTime;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetPlayerStartTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetPlayerStartTime(IHXBuffer* pPlayerStartTime)
{
    IHXBuffer* pTemp = m_pPlayerStartTime;
    m_pPlayerStartTime = pPlayerStartTime;
    HX_RELEASE(pTemp);
    if (m_pPlayerStartTime)
    {
        m_pPlayerStartTime->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetProtocol()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClientStats::GetProtocol()
{
    if (m_pProtocol)
    {
        m_pProtocol->AddRef();
    }
    return m_pProtocol;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetProtocol()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetProtocol(IHXBuffer* pProtocol)
{
    IHXBuffer* pTemp = m_pProtocol;
    m_pProtocol = pProtocol;
    HX_RELEASE(pTemp);
    if (m_pProtocol)
    {
        m_pProtocol->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetStartTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClientStats::GetStartTime()
{
    if (m_pStartTime)
    {
        m_pStartTime->AddRef();
    }
    return m_pStartTime;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetStartTime()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetStartTime(IHXBuffer* pStartTime)
{
    IHXBuffer* pTemp = m_pStartTime;
    m_pStartTime = pStartTime;
    HX_RELEASE(pTemp);
    if (m_pStartTime)
    {
        m_pStartTime->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetRequestMethod()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClientStats::GetRequestMethod()
{
    if (m_pRequestMethod)
    {
        m_pRequestMethod->AddRef();
    }
    return m_pRequestMethod;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetRequestMethod()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetRequestMethod(IHXBuffer* pRequestMethod)
{
    IHXBuffer* pTemp = m_pRequestMethod;
    m_pRequestMethod = pRequestMethod;
    HX_RELEASE(pTemp);
    if (m_pRequestMethod)
    {
        m_pRequestMethod->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetUserAgent()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClientStats::GetUserAgent()
{
    if (m_pUserAgent)
    {
        m_pUserAgent->AddRef();
    }
    return m_pUserAgent;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetUserAgent()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetUserAgent(IHXBuffer* pUserAgent)
{
    IHXBuffer* pTemp = m_pUserAgent;
    m_pUserAgent = pUserAgent;
    HX_RELEASE(pTemp);
    if (m_pUserAgent)
    {
        m_pUserAgent->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetVersion()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClientStats::GetVersion()
{
    if (m_pVersion)
    {
        m_pVersion->AddRef();
    }
    return m_pVersion;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetVersion()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetVersion(IHXBuffer* pVersion)
{
    IHXBuffer* pTemp = m_pVersion;
    m_pVersion = pVersion;
    HX_RELEASE(pTemp);
    if (m_pVersion)
    {
        m_pVersion->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetLoadTestPassword()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClientStats::GetLoadTestPassword()
{
    if (m_pLoadTestPassword)
    {
        m_pLoadTestPassword->AddRef();
    }
    return m_pLoadTestPassword;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetLoadTestPassword()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetLoadTestPassword(IHXBuffer* pLoadTestPassword)
{
    IHXBuffer* pTemp = m_pLoadTestPassword;
    m_pLoadTestPassword = pLoadTestPassword;
    HX_RELEASE(pTemp);
    if (m_pLoadTestPassword)
    {
        m_pLoadTestPassword->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetRTSPEvents()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClientStats::GetRTSPEvents()
{
    if (m_pRTSPEvents)
    {
        m_pRTSPEvents->AddRef();
    }
    return m_pRTSPEvents;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetRTSPEvents()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetRTSPEvents(IHXBuffer* pRTSPEvents)
{
    IHXBuffer* pTemp = m_pRTSPEvents;
    m_pRTSPEvents = pRTSPEvents;
    HX_RELEASE(pTemp);
    if (m_pRTSPEvents)
    {
        m_pRTSPEvents->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetStreamSelectionInfo()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXBuffer*)
ClientStats::GetStreamSelectionInfo()
{
    if (m_pStreamSelectionInfo)
    {
        m_pStreamSelectionInfo->AddRef();
    }
    return m_pStreamSelectionInfo;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetStreamSelectionInfo()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetStreamSelectionInfo(IHXBuffer* pStreamSelectionInfo)
{
    IHXBuffer* pTemp = m_pStreamSelectionInfo;
    m_pStreamSelectionInfo = pStreamSelectionInfo;
    HX_RELEASE(pTemp);
    if (m_pStreamSelectionInfo)
    {
        m_pStreamSelectionInfo->AddRef();
    }
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetControlBytesSent()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT64)
ClientStats::GetControlBytesSent()
{
    return m_ulControlBytesSent;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetControlBytesSent()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetControlBytesSent(UINT64 ulControlBytesSent)
{
    m_ulControlBytesSent = ulControlBytesSent;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetPort()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClientStats::GetPort()
{
    return m_ulPort;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetPort()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetPort(UINT32 ulPort)
{
    m_ulPort = ulPort;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetTotalMediaAdaptations()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClientStats::GetTotalMediaAdaptations()
{
    return m_ulTotalMediaAdaptations;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetTotalMediaAdaptations()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetTotalMediaAdaptations(UINT32 ulTotalMediaAdaptations)
{
    m_ulTotalMediaAdaptations = ulTotalMediaAdaptations;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetSessionCount()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClientStats::GetSessionCount()
{
    return m_lSessionCount;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetSessionCount()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetSessionCount(UINT32 ulSessionCount)
{
    m_lSessionCount = (INT32)ulSessionCount;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetSessionIndex()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClientStats::GetSessionIndex()
{
    return m_lSessionIndex;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetSessionIndex()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetSessionIndex(UINT32 ulSessionIndex)
{
    m_lSessionIndex = (INT32)ulSessionIndex;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::IsCloaked()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL)
ClientStats::IsCloaked()
{
    return m_bIsCloaked;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetCloaked()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetCloaked(BOOL bIsCloaked)
{
    m_bIsCloaked = bIsCloaked;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::IsRDT()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL)
ClientStats::IsRDT()
{
    return m_bIsRDT;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetRDT()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetRDT(BOOL bIsRDT)
{
    m_bIsRDT = bIsRDT;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SupportsMaximumASMBandwidth()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL)
ClientStats::SupportsMaximumASMBandwidth()
{
    return m_bSupportsMaximumASMBandwidth;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetSupportsASMBandwidth()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetSupportsMaximumASMBandwidth(BOOL bSupportsMaximumASMBandwidth)
{
    m_bSupportsMaximumASMBandwidth = bSupportsMaximumASMBandwidth;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SupportsMulticast()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL)
ClientStats::SupportsMulticast()
{
    return m_bSupportsMulticast;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::SetSupportsMulticast()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetSupportsMulticast(BOOL bSupportsMulticast)
{
    m_bSupportsMulticast = bSupportsMulticast;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::GetSession()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXSessionStats*)
ClientStats::GetSession(UINT32 ulID)
{
    IHXSessionStats* pSessionStats = (IHXSessionStats*)m_pSessionTable->get(ulID);
    if (pSessionStats)
    {
        pSessionStats->AddRef();
        return pSessionStats;
    }
    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::AddSession()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::AddSession(IHXSessionStats* pSession)
{
    if (!pSession)
    {
        return HXR_FAIL;
    }

    pSession->AddRef();

    m_lSessionCount++;
    m_lSessionIndex++;

    UINT32 ulSessionNumber = m_pSessionTable->create((void*)pSession);
    HX_ASSERT(ulSessionNumber == (UINT32)m_lSessionIndex);

    pSession->SetID(ulSessionNumber);
    pSession->SetClient(this);

    return HXR_OK;
}

/*
///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::AddSession()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClientStats::AddSession(IHXSessionStats* pSession)
{
    pSession->AddRef();

    m_ulSessionCount++;

    UINT32 ulNewSessionId = m_pSessionTable->create((void*)pSession);

    HX_ASSERT(ulNewSessionId == m_ulSessionCount);

    return ulNewSessionId;
}
*/

///////////////////////////////////////////////////////////////////////////////
// IHXClientStats::RemoveSession()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::RemoveSession(UINT32 ulID)
{
    IHXSessionStats* pSession = (IHXSessionStats*)m_pSessionTable->destroy(ulID);

    if (pSession == NULL)
    {
    // Session number does not exist in table.
        HX_ASSERT(!"ClientStatsObject: Tried to remove non-existent session!");
        return HXR_FAIL;
    }

    m_lSessionCount--;

    HX_ASSERT(m_lSessionCount >= 0);

    HX_RELEASE(pSession);

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// ClientStats::SetTimer()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetTimer(ClientStatsTimer* pTimer)
{
    pTimer->AddRef();
    m_pTimerList->insert(pTimer);
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// ClientStats::ClearTimers()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::ClearTimers()
{
    if (m_pTimerList)
    {
        ClientStatsTimer* pTemp = NULL;

        while (!m_pTimerList->empty())
        {
            pTemp = (ClientStatsTimer*)m_pTimerList->remove_head();
            pTemp->Cleanup();
            HX_RELEASE(pTemp);
        }
    }

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats2::GetReasonForTermination()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClientStats::GetReasonForTermination()
{
    return m_ulReasonForTermination;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStats2::SetReasonForTermination()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetReasonForTermination(UINT32 ulReasonForTermination)
{
    m_ulReasonForTermination = ulReasonForTermination;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXPrivateClientStats::IsPrevAuth()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL)
ClientStats::IsPrevAuth()
{
    return m_bIsPrevAuth;
}


///////////////////////////////////////////////////////////////////////////////
// IHXPrivateClientStats::SetPrevAuth()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStats::SetPrevAuth(BOOL bIsPrevAuth)
{
    m_bIsPrevAuth = bIsPrevAuth;
    return HXR_OK;
}






///////////////////////////////////////////////////////////////////////////////
// ClientStatsManagerPerProcessWrapper::ClientStatsManagerPerProcessWrapper()
///////////////////////////////////////////////////////////////////////////////


ClientStatsManagerPerProcessWrapper::ClientStatsManagerPerProcessWrapper(ClientStatsManager* pClientStatsMgr,
                                                                         Process* pProc)
: m_pMgr(pClientStatsMgr)
, m_pProc(pProc)
, m_RefCount(0)
{
}



///////////////////////////////////////////////////////////////////////////////
// ClientStatsManagerPerProcessWrapper::~ClientStatsManagerPerProcessWrapper()
///////////////////////////////////////////////////////////////////////////////

ClientStatsManagerPerProcessWrapper::~ClientStatsManagerPerProcessWrapper()
{
}


///////////////////////////////////////////////////////////////////////////////
// IUnknown::AddRef()
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(UINT32)
ClientStatsManagerPerProcessWrapper::AddRef(void)
{
    return InterlockedIncrement(&m_RefCount);
}


///////////////////////////////////////////////////////////////////////////////
// IUnknown::Release()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClientStatsManagerPerProcessWrapper::Release(void)
{
    if (InterlockedDecrement(&m_RefCount) > 0)
    {
        return m_RefCount;
    }

    delete this;
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// IUnknown::QueryInterface()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStatsManagerPerProcessWrapper::QueryInterface(REFIID riid,
                                                    void** ppInterfaceObj)
{

// By definition all COM objects support the IUnknown interface
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)(IHXClientStatsManager*)this;
        return HXR_OK;
    }

// IHXClientStatsManager interface is supported
    else if (IsEqualIID(riid, IID_IHXClientStatsManager))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)(IHXClientStatsManager*)this;
        return HXR_OK;
    }

// No other interfaces are supported
    *ppInterfaceObj = NULL;
    return HXR_NOINTERFACE;

}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStatsManagerPerProcessWrapper::RegisterSink()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStatsManagerPerProcessWrapper::RegisterSink(IHXClientStatsSink* pSink)
{
    return m_pMgr->RegisterSink(pSink, m_pProc);
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStatsManagerPerProcessWrapper::RemoveSink()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStatsManagerPerProcessWrapper::RemoveSink(IHXClientStatsSink* pSink)
{
    return m_pMgr->RemoveSink(pSink);
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStatsManagerPerProcessWrapper::AddClient()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStatsManagerPerProcessWrapper::AddClient(IHXClientStats* pClient)
{
    if (pClient)
    {
        pClient->SetStatsManager(this);
    }
    return m_pMgr->AddClient(pClient, m_pProc);
}

///////////////////////////////////////////////////////////////////////////////
// IHXClientStatsManagerPerProcessWrapper::RemoveClient()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStatsManagerPerProcessWrapper::RemoveClient(UINT32 ulClientId)
{
    return m_pMgr->RemoveClient(ulClientId, m_pProc);
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStatsManagerPerProcessWrapper::GetClient()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(IHXClientStats*)
ClientStatsManagerPerProcessWrapper::GetClient(UINT32 ulID)
{
    return m_pMgr->GetClient(ulID);
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStatsManagerPerProcessWrapper::GetClientCount()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClientStatsManagerPerProcessWrapper::GetClientCount()
{
    return m_pMgr->GetClientCount();
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStatsManagerPerProcessWrapper::UseRegistryForStats()
///////////////////////////////////////////////////////////////////////////////

BOOL
ClientStatsManagerPerProcessWrapper::UseRegistryForStats()
{
    return m_pMgr->UseRegistryForStats();
}

///////////////////////////////////////////////////////////////////////////////
// IHXClientStatsManagerPerProcessWrapper::ScheduleSinkNotifications()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStatsManagerPerProcessWrapper::ScheduleSinkNotifications(
                                 IHXClientStats* pClient,
                                 IHXSessionStats* pSession,
                                 ClientStatsEvent nEvent)
{
    return m_pMgr->ScheduleSinkNotifications(pClient, pSession, NULL, nEvent, m_pProc);
}






///////////////////////////////////////////////////////////////////////////////
// ClientStatsManager::ClientStatsManager()
///////////////////////////////////////////////////////////////////////////////

ClientStatsManager::ClientStatsManager()
: m_pClientList(NULL)
, m_pClientTable(NULL)
, m_pSinkList(NULL)
, m_RefCount(0)
, m_ulCurrentId(0)
, m_bUseRegistryForStats(FALSE)
, m_pMutex(NULL)
{
    m_pClientList = new HXList();
    m_pClientTable = new CHXID(MAX_CLIENT_STATS_ID);
    m_pSinkList = new HXList();

    m_pMutex = HXMutexCreate();
    HXMutexInit(m_pMutex);
}


///////////////////////////////////////////////////////////////////////////////
// ClientStatsManager::~ClientStatsManager()
///////////////////////////////////////////////////////////////////////////////

ClientStatsManager::~ClientStatsManager()
{
    if (m_pClientList)
    {
        IHXClientStats* pTemp = NULL;

        while (!m_pClientList->empty())
        {
            pTemp = (IHXClientStats*)m_pClientList->remove_head();
            HX_RELEASE(pTemp);
        }
    }

    HX_DELETE(m_pClientList);

    if (m_pSinkList)
    {
        SinkListElem* pTemp = NULL;

        while (!m_pSinkList->empty())
        {
            pTemp = (SinkListElem*)m_pSinkList->remove_head();
            HX_RELEASE(pTemp->m_pSink);
            HX_DELETE(pTemp);
        }
    }

    HX_DELETE(m_pSinkList);
    HX_DELETE(m_pClientTable);

    HXMutexDestroy(m_pMutex);
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStatsManager::RegisterSink()
///////////////////////////////////////////////////////////////////////////////

HX_RESULT
ClientStatsManager::RegisterSink(IHXClientStatsSink* pSink, Process* pProc)
{
    HXMutexLock(m_pMutex);
    SinkListElem* pElem = new SinkListElem;

    pElem->m_pSink = pSink;
    pSink->AddRef();
    pElem->m_pProc = pProc;

    m_pSinkList->insert(pElem);
    HXMutexUnlock(m_pMutex);

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStatsManager::RemoveSink()
///////////////////////////////////////////////////////////////////////////////

HX_RESULT
ClientStatsManager::RemoveSink(IHXClientStatsSink* pSink)
{
    SinkListElem* pElem = NULL;
    HXList_iterator i(m_pSinkList);

// Can't use the usual HXList->remove() because we're not
// getting the SinkListElem object-- we're getting the
// IHXClientStatsSink object instead!

    for (; *i != NULL; ++i)
    {
        pElem = (SinkListElem*)*i;

        if (pElem->m_pSink == pSink)
        {
            HXMutexLock(m_pMutex);
            pElem->prev->next = pElem->next;
            pElem->next->prev = pElem->prev;
            pElem->prev = pElem->next = pElem;

            m_pSinkList->size--;

            HX_RELEASE(pElem->m_pSink);
            HX_DELETE(pElem);
            HXMutexUnlock(m_pMutex);

            return HXR_OK;
        }
    }

    return HXR_FAIL;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStatsManager::AddClient()
///////////////////////////////////////////////////////////////////////////////

HX_RESULT
ClientStatsManager::AddClient(IHXClientStats* pClient, Process* pProc)
{
    if (!pClient)
    {
        return HXR_FAIL;
    }
    pClient->AddRef();

    IHXClientStatsManager* pStatsManager = pClient->GetStatsManager();
    if (!pStatsManager)
    {
        pClient->SetStatsManager(
            new ClientStatsManagerPerProcessWrapper(this,pProc));
    }
    else
    {
        pStatsManager->Release();
    }

    HXMutexLock(m_pMutex);
    UINT32 ulID = m_pClientTable->create((void*)pClient);
    m_pClientList->insert((ClientStats*)pClient);
    HXMutexUnlock(m_pMutex);

    pClient->SetID(ulID);

    ScheduleSinkNotifications(pClient, NULL, NULL, CSEVENT_CLIENT_CONNECT, pProc);

    HXList_iterator i(m_pSinkList);

    for (; *i != NULL; ++i)
    {
        SinkListElem* pSinkInfo = (SinkListElem*)*i;

        if (pSinkInfo->m_pSink->GetStatsTimerInterval() > 0)
        {
            ClientStatsTimer* pTimer             = new ClientStatsTimer;
            IHXClientStatsTimerControl* pControl = NULL;

            pTimer->Init(pClient,
                         pSinkInfo->m_pSink,
                         pProc,
                         pSinkInfo->m_pProc);

            if (FAILED(pClient->QueryInterface(IID_IHXClientStatsTimerControl,
                                               (void**)&pControl)))
            {
                HX_ASSERT(!"Unable to QI for stats timer control from client stats obj!");
                HX_DELETE(pTimer);
                return HXR_FAIL;
            }

            pControl->SetTimer(pTimer);
            HX_RELEASE(pControl);
        }
    }

    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
// IHXClientStatsManager::RemoveClient()
///////////////////////////////////////////////////////////////////////////////

HX_RESULT
ClientStatsManager::RemoveClient(UINT32 ulClientId, Process* pProc)
{
    ClientStats* pClient = NULL;

    HXMutexLock(m_pMutex);
    pClient = (ClientStats*)m_pClientTable->destroy(ulClientId);

    if (pClient == NULL)
    {
    // Client did not exist in table.
        HX_ASSERT(!"ClientStatsManager: Tried to remove non-existant client!");
        HXMutexUnlock(m_pMutex);
        return HXR_FAIL;
    }

#ifdef _DEBUG

    BOOL bFound = FALSE;

    HXList_iterator i(m_pClientList);

    for ( ; *i != NULL; ++i)
    {
        if (pClient == (ClientStats*)*i)
        {
            bFound = TRUE;
        }
    }

    if (!bFound)
    {
        HX_ASSERT(!"ClientStatsManager: ID-table/list inconsistency!");
    }

#endif

// This code assumes that the clientstats object is in the clientlist.
// We only verify this in debug builds!
    pClient->prev->next = pClient->next;
    pClient->next->prev = pClient->prev;
    pClient->prev = pClient->next = pClient;

    m_pClientList->size--;
    HXMutexUnlock(m_pMutex);

    pClient->ClearTimers();

    ScheduleSinkNotifications(pClient, NULL, NULL, CSEVENT_CLIENT_DISCONNECT, pProc);

    HX_RELEASE(pClient);

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStatsManager::GetClient()
///////////////////////////////////////////////////////////////////////////////

IHXClientStats*
ClientStatsManager::GetClient(UINT32 ulID)
{
    IHXClientStats* pClientStats = (IHXClientStats*)m_pClientTable->get(ulID);

    if (pClientStats)
    {
        pClientStats->AddRef();
        return pClientStats;
    }

    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStatsManager::GetClientCount()
///////////////////////////////////////////////////////////////////////////////

UINT32
ClientStatsManager::GetClientCount()
{
    //HX_ASSERT(m_pClientList->size == m_pClientTable->get_size());
    return m_pClientList->size;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStatsManager::SetUseRegistryForStats()
///////////////////////////////////////////////////////////////////////////////

void ClientStatsManager::SetUseRegistryForStats(BOOL bUseRegistryForStats)
{
    m_bUseRegistryForStats = bUseRegistryForStats;
}


///////////////////////////////////////////////////////////////////////////////
// IHXClientStatsManager::UseRegistryForStats()
///////////////////////////////////////////////////////////////////////////////

BOOL ClientStatsManager::UseRegistryForStats()
{
    return m_bUseRegistryForStats;
}


///////////////////////////////////////////////////////////////////////////////
// ClientStatsManager::SessionDone
///////////////////////////////////////////////////////////////////////////////

HX_RESULT
ClientStatsManager::SessionDone(IHXClientStats* pClient,
                                IHXSessionStats* pSession,
                                Process* pProc)
{
    ScheduleSinkNotifications(pClient, pSession, NULL, CSEVENT_SESSION_DONE, pProc);

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// ClientStatsManager::ClipDone
///////////////////////////////////////////////////////////////////////////////

HX_RESULT
ClientStatsManager::ClipDone(IHXClientStats* pClient,
                             IHXSessionStats* pSession,
                             IHXClipStats* pClip,
                             Process* pProc)
{
    ScheduleSinkNotifications(pClient, pSession, pClip, CSEVENT_CLIP_DONE, pProc);

    return HXR_OK;
}


HX_RESULT
ClientStatsManager::ScheduleSinkNotifications(IHXClientStats* pClient,
                                 IHXSessionStats* pSession,
                                 IHXClipStats* pClip,
                                 ClientStatsEvent nEvent,
                                 Process* pProc)
{
    HXList_iterator i(m_pSinkList);
    for (; *i != NULL; ++i)
    {
        SinkListElem* pSinkInfo = (SinkListElem*)*i;
        ClientStatsSinkNotifyCallback* pCB = new ClientStatsSinkNotifyCallback();
        pCB->Init(pClient,
                  pSession,
                  pClip,
                  pSinkInfo->m_pSink,
                  pSinkInfo->m_pProc,
                  nEvent);
        pProc->pc->dispatchq->send(pProc, pCB, pSinkInfo->m_pProc->procnum());
    }

    return HXR_OK;
}








///////////////////////////////////////////////////////////////////////////////
// ClientStatsTimer::ClientStatsTimer()
///////////////////////////////////////////////////////////////////////////////

ClientStatsTimer::ClientStatsTimer()
: m_pTimerProc(NULL)
, m_pSinkProc(NULL)
, m_pSink(NULL)
, m_pStats(NULL)
, m_CBH(0)
, m_RefCount(0)
{
}


///////////////////////////////////////////////////////////////////////////////
// ClientStatsTimer::~ClientStatsTimer()
///////////////////////////////////////////////////////////////////////////////

ClientStatsTimer::~ClientStatsTimer()
{
    Cleanup();
}


///////////////////////////////////////////////////////////////////////////////
// IUnknown::AddRef()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClientStatsTimer::AddRef(void)
{
    return InterlockedIncrement(&m_RefCount);
}


///////////////////////////////////////////////////////////////////////////////
// IUnknown::Release()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
ClientStatsTimer::Release(void)
{
    if (InterlockedDecrement(&m_RefCount) > 0)
    {
        return m_RefCount;
    }

    delete this;
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// IUnknown::QueryInterface()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStatsTimer::QueryInterface(REFIID riid,
                            void** ppInterfaceObj)
{

// By definition all COM objects support the IUnknown interface
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)(IHXCallback*)this;
        return HXR_OK;
    }

// IHXClientStatsTimer interface is supported
    else if (IsEqualIID(riid, IID_IHXCallback))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)(IHXCallback*)this;
        return HXR_OK;
    }

// No other interfaces are supported
    *ppInterfaceObj = NULL;
    return HXR_NOINTERFACE;

}


///////////////////////////////////////////////////////////////////////////////
// ClientStatsTimer::Init()
///////////////////////////////////////////////////////////////////////////////

HX_RESULT
ClientStatsTimer::Init(IHXClientStats* pClientStats,
                       IHXClientStatsSink* pSink,
                       Process* pTimerProc,
                       Process* pSinkProc)
{
    if (!pSink || !pTimerProc || !pSinkProc || !pClientStats)
    {
        return HXR_FAIL;
    }

    HX_ASSERT(pSink && pTimerProc && pSinkProc && pClientStats);

    m_pStats = pClientStats;
    m_pStats->AddRef();

    m_pSink = pSink;
    m_pSink->AddRef();

    m_pTimerProc = pTimerProc;
    m_pSinkProc = pSinkProc;

//XXXDPL should we enforce a minimum here?
    m_CBH = m_pTimerProc->pc->scheduler->RelativeEnter(this,
                                                       m_pSink->GetStatsTimerInterval());

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// ClientStatsTimer::Cleanup()
///////////////////////////////////////////////////////////////////////////////

void
ClientStatsTimer::Cleanup()
{
    HX_RELEASE(m_pSink);
    HX_RELEASE(m_pStats);

    if (m_CBH)
    {
        m_pTimerProc->pc->scheduler->Remove(m_CBH);
        m_CBH = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
// ClientStatsTimer::Func()
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
ClientStatsTimer::Func()
{
    ClientStatsSinkNotifyCallback* pCB = new ClientStatsSinkNotifyCallback();

    if (FAILED(pCB->Init(m_pStats, NULL, NULL, m_pSink, m_pSinkProc, CSEVENT_TIMER)))
    {
        HX_DELETE(pCB);
        return HXR_FAIL;
    }

//XXXDPL should we enforce a minimum here?
    m_pTimerProc->pc->dispatchq->send(m_pTimerProc,
                                      pCB,
                                      m_pSinkProc->procnum());
    m_CBH = m_pTimerProc->pc->scheduler->RelativeEnter(this,
                                                       m_pSink->GetStatsTimerInterval());

    return HXR_OK;
}








///////////////////////////////////////////////////////////////////////////////
// ClientStatsSinkNotifyCallback::ClientStatsSinkNotifyCallback()
///////////////////////////////////////////////////////////////////////////////

ClientStatsSinkNotifyCallback::ClientStatsSinkNotifyCallback()
: m_pClientStats(NULL)
, m_pSessionStats(NULL)
, m_pClipStats(NULL)
, m_pSink(NULL)
, m_pSink2(NULL)
, m_pProc(NULL)
, m_Event(CSEVENT_UNKNOWN)
{
}


///////////////////////////////////////////////////////////////////////////////
// ClientStatsSinkNotifyCallback::~ClientStatsSinkNotifyCallback()
///////////////////////////////////////////////////////////////////////////////

ClientStatsSinkNotifyCallback::~ClientStatsSinkNotifyCallback()
{
    HX_RELEASE(m_pClientStats);
    HX_RELEASE(m_pSessionStats);
    HX_RELEASE(m_pClipStats);
    HX_RELEASE(m_pSink);
    HX_RELEASE(m_pSink2);
}


///////////////////////////////////////////////////////////////////////////////
// ClientStatsSinkNotifyCallback::Init()
///////////////////////////////////////////////////////////////////////////////

HX_RESULT
ClientStatsSinkNotifyCallback::Init(IHXClientStats* pClientStats,
                                    IHXSessionStats* pSessionStats,
                                    IHXClipStats* pClipStats,
                                    IHXClientStatsSink* pSink,
                                    Process* pProc,
                                    ClientStatsEvent Event)
{
    if (!pClientStats && !pSessionStats)
    {
        return HXR_FAIL;
    }

    if (!pSink || !pProc || Event == CSEVENT_UNKNOWN)
    {
        return HXR_FAIL;
    }


    HX_ASSERT(pClientStats || pSessionStats);

    HX_ASSERT(pClientStats && pSink && pProc && Event != CSEVENT_UNKNOWN);

    m_pClientStats = pClientStats;

    if (m_pClientStats)
    {
        m_pClientStats->AddRef();
    }

    m_pSessionStats = pSessionStats;

    if (m_pSessionStats)
    {
        m_pSessionStats->AddRef();
    }

    m_pClipStats = pClipStats;

    if (m_pClipStats)
    {
        m_pClipStats->AddRef();
    }

    m_pProc = pProc;

    m_pSink = pSink;
    m_pSink->AddRef();

    m_pSink->QueryInterface(IID_IHXClientStatsSink2, (void**)&m_pSink2);

    m_Event = Event;

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
// ClientStatsSinkNotifyCallback::func()
///////////////////////////////////////////////////////////////////////////////

void
ClientStatsSinkNotifyCallback::func(Process* pProc)
{
    switch (m_Event)
    {
        case CSEVENT_CLIP_DONE:
            HX_ASSERT(m_pClipStats);
            // fall through

        case CSEVENT_SESSION_SETURL:
        case CSEVENT_SESSION_DONE:
            HX_ASSERT(m_pSessionStats);
            //fall through

        case CSEVENT_TIMER:
        case CSEVENT_CLIENT_CONNECT:
        case CSEVENT_CLIENT_DISCONNECT:
            HX_ASSERT(m_pClientStats);

            if (m_pSink2)
            {
                m_pSink2->OnStatsEvent(m_Event, m_pClientStats, m_pSessionStats, m_pClipStats);
            }
            else
            {
            m_pSink->OnStatsEvent(m_Event, m_pClientStats, m_pSessionStats);
            }
            break;

        default:
            HX_ASSERT(m_Event == CSEVENT_UNKNOWN);
            HX_ASSERT(!"Bad client stats notify event!");
            return;
    }
    delete this;
}

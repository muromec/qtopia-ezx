
#if !defined(SESSION_LINGER_TIMEOUT_H__)
#define SESSION_LINGER_TIMEOUT_H__

#include "unkimp.h"
#include "hxengin.h"

class RTSPClientSessionManager;
class RTSPClientSession;

class SessionLingerTimeout
: public CUnknownIMP
, public IHXCallback
{
    // IUnknown
    DECLARE_UNKNOWN_NOCREATE(SessionLingerTimeout)
public:
    static HX_RESULT Create(IUnknown* pContext, RTSPClientSessionManager* pMgr, SessionLingerTimeout*& pTimeout);
    ~SessionLingerTimeout();
private:
    SessionLingerTimeout();

    // IHXCallback
    STDMETHOD(Func)();

public:
    HX_RESULT Init(IUnknown* pContext, RTSPClientSessionManager* pMgr);
    void Start(RTSPClientSession* pSession, UINT32 msLinger);
    void Cancel();
    HXBOOL IsPending() const;


private:
    RTSPClientSessionManager* m_pSessionMgr;
    IHXScheduler* m_pScheduler;
    RTSPClientSession* m_pLingerSession;
    CallbackHandle m_hCallback;
};

inline
HXBOOL
SessionLingerTimeout::IsPending() const
{
    return m_pLingerSession != NULL;
}

#endif //SESSION_LINGER_TIMEOUT_H__


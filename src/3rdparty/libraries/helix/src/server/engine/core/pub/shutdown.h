#ifndef _SHUTDOWN_H_
#define _SHUTDOWN_H_

#include "hxtypes.h"
#include "hxcom.h"
#include "hxtypes.h"
#include "hxengin.h"
#include "hxmon.h"
#include "simple_callback.h"
#include "base_callback.h"

struct IHXServerShutdownResponse;
class CHXMapLongToObj;

class ShutdownCallback : public SimpleCallback
{
public:
    ShutdownCallback() {}
    void                func(Process* proc);
    BOOL                m_bRestart;
private:
                        ~ShutdownCallback() {};
};

class DelayedShutdownCallback: public BaseCallback
{
public:
    STDMETHOD(Func) (THIS);
    BOOL m_bRestart;
};

class CServerShutdown: public IHXCallback,
                        public IHXPropWatchResponse
{
public:
	CServerShutdown();
    ~CServerShutdown();

    void Init(Process* proc);
    void StartShutdown(BOOL bRestart);

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXCallback methods
     */

    /************************************************************************
     *	Method:
     *	    IHXCallback::Func
     *	Purpose:
     *	    This is the function that will be called when a callback is
     *	    to be executed.
     */
    STDMETHOD(Func)			(THIS);

    /*
     * IHXPropWatchResponse methods
     */

    /************************************************************************
     *  Method:
     *      IHXPropWatchResponse::AddedProp
     *  Purpose:
     *      Gets called when a watched Property gets modified. It passes
     *  the id of the Property just modified, its datatype and the
     *  id of its immediate parent COMPOSITE property.
     */
    STDMETHOD(AddedProp)	(THIS_
				const UINT32		id,
				const HXPropType   	propType,
				const UINT32		ulParentID);

    /************************************************************************
     *  Method:
     *      IHXPropWatchResponse::ModifiedProp
     *  Purpose:
     *      Gets called when a watched Property gets modified. It passes
     *  the id of the Property just modified, its datatype and the
     *  id of its immediate parent COMPOSITE property.
     */
    STDMETHOD(ModifiedProp)	(THIS_
				const UINT32		id,
				const HXPropType   	propType,
				const UINT32		ulParentID);

    /************************************************************************
     *  Method:
     *      IHXPropWatchResponse::DeletedProp
     *  Purpose:
     *      Gets called when a watched Property gets deleted. As can be
     *  seen, it returns the id of the Property just deleted and
     *  its immediate parent COMPOSITE property.
     */
    STDMETHOD(DeletedProp)	(THIS_
				const UINT32		id,
				const UINT32		ulParentID);


	HX_RESULT Register(LONG32 connId, IHXServerShutdownResponse * pSserverShutdownResponse);
	HX_RESULT Unregister(LONG32 connId);

private:
    Process*            m_proc;
    CHXMapLongToObj*    m_pClientTable;
    IHXScheduler*       m_pScheduler;
    IHXRegistry*        m_pRegistry;
    LONG32              m_lRefCount;
    IHXPropWatch*	m_pPropWatch;

    UINT32              m_DelayedShutdownEnabledPropID;
    UINT32              m_ClientDisconnectIntervalPropID;
    UINT32              m_ShutdownProceedTimePropID;
    UINT32              m_NewClientConnectionAllowedPropID;
    UINT32              m_logPlayerTerminationPropID;

    BOOL                m_bDelayedShutdownEnabled;
    INT32               m_ulClientDisconnectInterval;
    INT32               m_ulShutdownProceedTime;
    BOOL                m_bNewClientConnAllowed;
    BOOL                m_bLogPlayerTermination;

    BOOL                m_bRestart;
    
    enum
    {
        NORMAL_STATE,
        SHUTDOWN_START,
        SHUTDOWN_PROCEEDING,
        SHUTDOWN_FINISHING,
    } m_state;


    void ProcessConfig();


};

#endif //_SHUTDOWN_H_

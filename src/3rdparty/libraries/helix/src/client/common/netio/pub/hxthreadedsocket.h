/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxthreadedsocket.h,v 1.8 2006/02/16 23:07:05 ping Exp $ 
 *   
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.  
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
 * Alternatively, the contents of this file may be used under the 
 * terms of the GNU General Public License Version 2 or later (the 
 * "GPL") in which case the provisions of the GPL are applicable 
 * instead of those above. If you wish to allow use of your version of 
 * this file only under the terms of the GPL, and not to allow others  
 * to use your version of this file under the terms of either the RPSL 
 * or RCSL, indicate your decision by deleting the provisions above 
 * and replace them with the notice and other provisions required by 
 * the GPL. If you do not delete the provisions above, a recipient may 
 * use your version of this file under the terms of any one of the 
 * RPSL, the RCSL or the GPL. 
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


#if !defined(HXTHREADEDSOCKET_H__)
#define HXTHREADEDSOCKET_H__

#include "hxnet.h"
#include "unkimp.h"
#include "hxclassdispatchtasks.h"
#include "hxsocketdata.h"
#include "hxsockeventdata.h"
#include "hxengin.h"

class HXThreadTaskDriver;
class HXThreadTask;
class HXMutex;

class HXThreadedSocket
: public CUnknownIMP
, public IHXSocket
, public IHXMulticastSocket
, public IHXSocketResponse
, public IHXCallback
, public IHXInterruptSafe
{
public:
    DECLARE_UNKNOWN(HXThreadedSocket)

public:
    static 
    HX_RESULT Create(IUnknown* pContext,
                     IHXSocket* pActualSock, 
                     IHXSocket*& pThreadedSock);

// IHXSocket
    STDMETHOD_(HXSockFamily,GetFamily)      (THIS);
    STDMETHOD_(HXSockType,GetType)          (THIS);
    STDMETHOD_(HXSockProtocol,GetProtocol)  (THIS);

    STDMETHOD(SetResponse)          (THIS_ IHXSocketResponse* pResponse);
    STDMETHOD(SetAccessControl)     (THIS_ IHXSocketAccessControl* pControl);

    STDMETHOD(Init)                 (THIS_ HXSockFamily f, HXSockType t, HXSockProtocol p);
    STDMETHOD(CreateSockAddr)       (THIS_ IHXSockAddr** ppAddr);

    STDMETHOD(Bind)                 (THIS_ IHXSockAddr* pAddr);
    STDMETHOD(ConnectToOne)         (THIS_ IHXSockAddr* pAddr);
    STDMETHOD(ConnectToAny)         (THIS_ UINT32 nVecLen, IHXSockAddr** ppAddrVec);
    STDMETHOD(GetLocalAddr)         (THIS_ IHXSockAddr** ppAddr);
    STDMETHOD(GetPeerAddr)          (THIS_ IHXSockAddr** ppAddr);

    STDMETHOD(SelectEvents)         (THIS_ UINT32 uEventMask);
    STDMETHOD(Peek)                 (THIS_ IHXBuffer** pBuf);
    STDMETHOD(Read)                 (THIS_ IHXBuffer** pBuf);
    STDMETHOD(Write)                (THIS_ IHXBuffer* pBuf);
    STDMETHOD(Close)                (THIS);

    STDMETHOD(Listen)               (THIS_ UINT32 uBackLog);
    STDMETHOD(Accept)               (THIS_ IHXSocket** ppNewSock,
                                           IHXSockAddr** ppSource);

    STDMETHOD(GetOption)            (THIS_ HXSockOpt name, UINT32* pval);
    STDMETHOD(SetOption)            (THIS_ HXSockOpt name, UINT32 val);

    STDMETHOD(PeekFrom)             (THIS_ IHXBuffer** ppBuf,
                                           IHXSockAddr** ppAddr);
    STDMETHOD(ReadFrom)             (THIS_ IHXBuffer** ppBuf,
                                           IHXSockAddr** ppAddr);
    STDMETHOD(WriteTo)              (THIS_ IHXBuffer* pBuf,
                                           IHXSockAddr* pAddr);


    STDMETHOD(ReadV)                (THIS_ UINT32 nVecLen, UINT32* puLenVec,
                                           IHXBuffer** ppBufVec);
    STDMETHOD(ReadFromV)            (THIS_ UINT32 nVecLen, UINT32* puLenVec,
                                           IHXBuffer** ppBufVec,
                                           IHXSockAddr** ppAddr);

    STDMETHOD(WriteV)               (THIS_ UINT32 nVecLen,
                                           IHXBuffer** ppBufVec);
    STDMETHOD(WriteToV)             (THIS_ UINT32 nVecLen,
                                           IHXBuffer** ppBufVec,
                                           IHXSockAddr* pAddr);

// IHXMulticastSocket
    STDMETHOD(JoinGroup)            (THIS_ IHXSockAddr* pGroupAddr,
                                           IHXSockAddr* pInterface);
    STDMETHOD(LeaveGroup)           (THIS_ IHXSockAddr* pGroupAddr,

                                           IHXSockAddr* pInterface);

    STDMETHOD(SetSourceOption)      (THIS_ HXMulticastSourceOption flag,
                                           IHXSockAddr* pSourceAddr,
                                           IHXSockAddr* pGroupAddr,
                                           IHXSockAddr* pInterface);

// IHXSocketResponse
    STDMETHOD(EventPending) (THIS_ UINT32 uEvent, HX_RESULT status);

// IHXCallback 
    STDMETHOD(Func)();

// IHXInterruptSafe
    STDMETHOD_(HXBOOL,IsInterruptSafe)();

private:
    HXThreadedSocket();
    virtual ~HXThreadedSocket();

    // implementation
    STDMETHOD(HandleRead)();
    STDMETHOD(HandleWrite)();

    void AddPendingEvent(UINT32 event, HX_RESULT status);
    void ScheduleCallback();

    HX_RESULT DoConstructInit(IUnknown* pContext, IHXSocket* pSock);
    HX_RESULT SendWaitHelper(HXThreadTask* pTask);
    HX_RESULT SendWait(HXClassDispatchTask<HX_RESULT>* pTask);
    HX_RESULT Send(HXClassDispatchTask<HX_RESULT>* pTask);
    
    void InitBufferLimits(HXSockType type);
    HXBOOL IsReadable() const;
    void DestroyScheduler();
    void EnsureReadEventPosted();

private:
    volatile enum State
    {
        TS_OPEN,
        TS_ENDSTREAM,
        TS_ERROR,
        TS_CLOSED
    } m_state;

    IHXSocket*              m_pSock;
    IHXSocketResponse*      m_pResponse;
    HXThreadTaskDriver*     m_pDriver; 
    
    HXBOOL                    m_inReadEventPostingLoop;

    // tracks outstanding user event requests
    UINT32                  m_selectedEvents;

    HXSocketData*           m_pInbound;  
    HXSocketData*           m_pOutbound;

    // events queued on parent thread for dispatch under scheduler callback
    HXSockEventData         m_events;

    UINT32                  m_cbMaxInbound;

    IHXScheduler*           m_pScheduler;
    IUnknown*               m_pContext;
    volatile UINT32         m_hCallback;

    // can response handle calls (e.g. to EventPending) on either core or app thread
    HXBOOL                  m_isResponseSafe;

    IHXMutex*               m_pSchedulerMutex;

};
#endif //HXTHREADEDSOCKET_H__






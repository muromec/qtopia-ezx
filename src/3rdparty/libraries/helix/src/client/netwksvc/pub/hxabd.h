/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxabd.h,v 1.10 2005/08/15 17:30:08 gwright Exp $
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

#ifndef _HXABD_H_
#define _HXABD_H_

#include "protdefs.h"
#include "hxabdutil.h"
#include "hxnet.h"

#define DEFAULT_ABD_SERVER_PORT 80

struct ABD_SERVER_INFO
{
    CHXString*  pServer;
    UINT32      ulPort;

    ABD_SERVER_INFO()
    {
        pServer = NULL;
        ulPort = 0;
    };

    ~ABD_SERVER_INFO()
    {
        HX_DELETE(pServer);
    };
};

class CHXABDResponse;
class CByteGrowingQueue;

class CHXABDCalibrator : public IHXAutoBWCalibration
{
public:
    CHXABDCalibrator(IUnknown* pContext);

    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)               (THIS_
                                            REFIID riid,
                                            void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)              (THIS);

    STDMETHOD_(ULONG32,Release)             (THIS);

    /*
     *  IHXAutoBWCalibration methods
     */
    STDMETHOD(InitAutoBWCalibration)        (THIS_
                                             IHXValues* pValues);

    STDMETHOD(StartAutoBWCalibration)       (THIS);

    STDMETHOD(StopAutoBWCalibration)        (THIS);

    STDMETHOD(AddAutoBWCalibrationSink)     (THIS_
                                             IHXAutoBWCalibrationAdviseSink* pSink);

    STDMETHOD(RemoveAutoBWCalibrationSink)  (THIS_
                                             IHXAutoBWCalibrationAdviseSink* pSink);

    void Close(void);

private:

    ~CHXABDCalibrator();

    HX_RESULT GetAddrInfoDone(HX_RESULT status,
                              UINT32 nVecLen,
                              IHXSockAddr** ppAddrVec);

    HX_RESULT GetNameInfoDone(HX_RESULT status,
                              const char* pszNode,
                              const char* pszService);

    HX_RESULT EventPending(UINT32 uEvent, HX_RESULT status);

    void Reset(void);

    class TimeoutCallback : public IHXCallback
    {
    public:
        TimeoutCallback(CHXABDCalibrator* pOwner);
        ~TimeoutCallback();

        STDMETHOD(QueryInterface)       (THIS_
                                        REFIID riid,
                                        void** ppvObj);

        STDMETHOD_(ULONG32,AddRef)      (THIS);

        STDMETHOD_(ULONG32,Release)     (THIS);

        STDMETHOD(Func)                 (THIS);

    private:
        LONG32                          m_lRefCount;
        CHXABDCalibrator*               m_pOwner;
    };

    LONG32                          m_lRefCount;
    IUnknown*                       m_pContext;
    IHXSocket*                      m_pSocket;
    IHXResolve*                     m_pResolve;
    IHXScheduler*                   m_pScheduler;
    IHXErrorMessages*               m_pErrMsg;
    IHXNetServices*                 m_pNetServices;

    HXBOOL                            m_bInitialized;
    HXBOOL                            m_bABDPending;
    HXBOOL                            m_bReadHeaderDone;
    UINT8                           m_nABDServers;
    UINT8                           m_nCurrentABDServer;
    ABD_SERVER_INFO*                m_pABDServers[MAX_ABD_SERVERS];
    ABD_PROBPKT_INFO*               m_pABDProbPktInfo[MAX_ABD_PROBPKT];
    CHXABDResponse*                 m_pABDResponse;
    CHXSimpleList*                  m_pAutoBWCalibrationSinkList;
    UINT32                          m_ulABDTimeoutCBHandle;
    TimeoutCallback*                m_pABDTimeoutCB;
    CByteGrowingQueue*              m_pInQueue;

    UINT8                           m_nProbingPacketsReceived;
    UINT32                          m_ulProbingPacketsGap[MAX_ABD_PROBPKT];

    UINT8                           m_nProbingPacketsRequested;
    UINT32                          m_ulProbingPacketSize;
    UINT8                           m_nABDMode;

    HX_RESULT       HandleSocketWrite();
    HX_RESULT       HandleSocketRead(HX_RESULT status, IHXBuffer* Buffer);
    HX_RESULT       HandleABDTimeout();
    HX_RESULT       ReportABDStatus(HX_RESULT rc, UINT32 ulBW);

    HX_RESULT       DeleteServers(void);
    HX_RESULT       ParseServers(const char* pszServers);
    void            AddABDHeader(CHXString& mesg, 
                                 const char* pName, ULONG32 ulValue);

    friend class TimeoutCallback;
    friend class CHXABDResponse;
};

class CHXABDResponse : public CUnknownIMP,
                       public IHXSocketResponse,
                       public IHXResolveResponse
{
    DECLARE_UNKNOWN(CHXABDResponse)

public:

    CHXABDResponse();
    ~CHXABDResponse();

    void InitObject(CHXABDCalibrator* pOwner);

    // IHXResolveResponse methods
    STDMETHOD(GetAddrInfoDone)  (THIS_
                                 HX_RESULT status,
                                 UINT32 nVecLen,
                                 IHXSockAddr** ppAddrVec);

    STDMETHOD(GetNameInfoDone)  (THIS_
                                 HX_RESULT status,
                                 const char* pszNode,
                                 const char* pszService);

    // IHXSocketResponse methods
    STDMETHOD(EventPending)     (THIS_ UINT32 uEvent, HX_RESULT status);

protected:
    LONG32                      m_lRefCount;
    CHXABDCalibrator*           m_pOwner;

    friend class CHXABDCalibrator;
};

#endif /* _HXABD_H_ */

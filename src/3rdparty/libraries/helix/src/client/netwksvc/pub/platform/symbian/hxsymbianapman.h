/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsymbianapman.h,v 1.9 2006/07/27 15:58:16 junhliu Exp $
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
#ifndef HXSYMBIANAPMAN_H
#define HXSYMBIANAPMAN_H

#include "hxcom.h"
#include "ihxaccesspoint.h"
#include "unkimp.h"
#include "hxslist.h"
#include "hxccf.h"
#include "hxsymbianbearermonitor.h"

#include <es_sock.h>

class HXSymbianApManAO;

class HXSymbianAPConnector;

class HXSymbianAccessPointManager : public CUnknownIMP,
                    public IHXAccessPointManager,
                    public IHXAccessPointSelectorResponse,
                    public HXConnectionMonitorObserver
{
public:

    DECLARE_UNKNOWN(HXSymbianAccessPointManager);

    /*
     * IHXAccessPointManager methods
     */

    /************************************************************************
     *  Method:
     *      IHXAccessPointManager::Connect
     *  Purpose:
     *      Notifies the access point manager that an object wants the access
     *      point to connect to it's ISP.
     *
     */
    STDMETHOD(Connect) (THIS_ IHXAccessPointConnectResponse* pResp);

    /************************************************************************
     *  Method:
     *      IHXAccessPointManager::RegisterSelector
     *  Purpose:
     *      Provides the IHXAccessPointManager with an IHXAccessPointSelector
     *      to use when it needs information about the desired access point.
     *
     */
    STDMETHOD(RegisterSelector)(THIS_ IHXAccessPointSelector* pSelector);

    /************************************************************************
     *  Method:
     *      IHXAccessPointManager::UnregisterSelector
     *  Purpose:
     *      Unregisters a previously registered IHXAccessPointSelector
     *
     */
    STDMETHOD(UnregisterSelector)(THIS_ IHXAccessPointSelector* pSelector);

    /************************************************************************
     *  Method:
     *      IHXAccessPointManager::GetActiveAccessPointInfo
     *  Purpose:
     *      Returns information about the access point we are currently
     *      connected to. This function returns an error if we are
     *      not connected to an access point.
     *
     */
    STDMETHOD(GetActiveAccessPointInfo)(THIS_ REF(IHXValues*) pInfo);

    /************************************************************************
     *  Method:
     *      IHXAccessPointManager::GetPreferredAccessPointInfo
     *  Purpose:
     *      Returns information about the access point we want to connect to.
     */
    STDMETHOD(GetPreferredAccessPointInfo)(THIS_ REF(IHXValues*) pInfo);

    /************************************************************************
     *  Method:
     *      IHXAccessPointManager::SetPreferredAccessPointInfo
     *  Purpose:
     *      Tells the access point manager about the access
     *      point we would like it to connect to.
     */
    STDMETHOD(SetPreferredAccessPointInfo)(THIS_ IHXValues* pInfo);

    /*
     * IHXAccessPointSelectorResponse methods
     */

    /************************************************************************
     *  Method:
     *      IHXAccessPointSelectorResponse::SelectAccessPointDone
     *  Purpose:
     *      Returns the selected access point info
     */
    STDMETHOD(SelectAccessPointDone)(THIS_ HX_RESULT status,
                     IHXValues* pInfo);

    void SetContext(IUnknown* pContext);

    // HXConnectionMonitorObserver implementation.
    virtual void BearerChanged(const HXBearerType bearer,
                               const HXNetworkGeneration generation,
                               TUint dlBandwidth, TUint ulBandwidth);

    virtual void OnBearerError(HX_RESULT hxr);

    /************************************************************************
     * It is required to Connect to access point before using GetRConnection
     * and GetRSocketServ.
     ************************************************************************/
    RConnection&    GetRConnection();
    RSocketServ&    GetRSocketServ();

protected:
    friend class HXSymbianApManAO;

    void ConnectDone(TInt aoStatus);
    void DisconnectDone(HX_RESULT status);

private:
    HXSymbianAccessPointManager();
    ~HXSymbianAccessPointManager();

    HXBOOL DispatchConnectDones(HX_RESULT status);

    HX_RESULT DoInit();
    HX_RESULT DoClose();

    HX_RESULT DoConnect(IHXAccessPointConnectResponse* pResp);
    HX_RESULT StartConnection();
    HX_RESULT StartDisconnect();

    HX_RESULT GetPreferredID(REF(ULONG32) ulID);
    HX_RESULT GetActiveID(REF(ULONG32) ulID);

    typedef enum {apError, apIdle, apConnecting, apConnected,
          apDisconnecting} APState;

    CHXSimpleList           m_respList;

    IUnknown*               m_pContext;
    APState                 m_state;
    RSocketServ             m_sockServ;
    RConnection             m_rconn;

    HXSymbianApManAO*       m_pConnector;
    IHXValues*              m_pPreferredInfo;
    IHXCommonClassFactory*  m_pCCF;

    IHXAccessPointSelector* m_pAPSelector;
    HXBOOL                  m_bSelectAPPending;
    HXBOOL                  m_bInitialized;
    HXConnectionMonitor*    m_pMonitor;
};
#endif /* HXSYMBIANAPMAN_H */


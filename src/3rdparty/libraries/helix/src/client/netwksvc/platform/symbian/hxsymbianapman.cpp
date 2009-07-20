/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsymbianapman.cpp,v 1.22 2008/05/08 19:14:11 shivnani Exp $
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
 * terms of the GNU General Public License Version 2 (the
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


#include "platform/symbian/hxsymbianapman.h"
#include "hxsymbianapmanao.h"
#include "ihxpckts.h"
#include "hxprefutil.h"
#include "hxtlogutil.h"
#include "hxerror.h"
#include <exterror.h>//just for error codes

#if defined(HELIX_FEATURE_SYMBIAN_MMF) && defined(HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY)
#include <etelpckt.h>//just for error codes
#include <gsmerror.h>//just for error codes
#include <wlanerrorcodes.h> //just for error codes
#endif

const UINT32 DefaultNumOfRetries = 1; /* Number of connect retries before
                       * reporting the error. The retry
                       * mechanism is needed because switching
                       * access points can cause the first
                       * connect to fail. This is a documented
                       * Symbian bug.
                       */


BEGIN_INTERFACE_LIST(HXSymbianAccessPointManager)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXAccessPointManager)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXAccessPointSelectorResponse)
END_INTERFACE_LIST

HXSymbianAccessPointManager::HXSymbianAccessPointManager():
    m_pContext(NULL),
    m_state(apError),
    m_pConnector(0),
    m_pPreferredInfo(NULL),
    m_pCCF(NULL),
    m_pAPSelector(NULL),
    m_bSelectAPPending(FALSE),
    m_bInitialized(FALSE),
    m_pMonitor(NULL)
{
    HXLOGL4(HXLOG_NETW, "HXSymbianAccessPointManager::HXSymbianAccessPointManager");
}

HXSymbianAccessPointManager::~HXSymbianAccessPointManager()
{
    HXLOGL4(HXLOG_NETW, "HXSymbianAccessPointManager::~HXSymbianAccessPointManager()");
    DoClose();
    HX_RELEASE(m_pPreferredInfo);
    HX_RELEASE(m_pCCF);
    HX_RELEASE(m_pAPSelector);
    HX_DELETE(m_pMonitor);
    HX_RELEASE(m_pContext);

    DispatchConnectDones(HXR_FAILED);
}

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
STDMETHODIMP
HXSymbianAccessPointManager::Connect(THIS_ IHXAccessPointConnectResponse* pResp)
{
    HXLOGL2(HXLOG_NETW, "HXSymbianAccessPointManager::Connect()");

    HX_RESULT res = HXR_FAILED;

    if (pResp)
    {
    res = DoConnect(pResp);
    }

    return res;
}

/************************************************************************
 *  Method:
 *      IHXAccessPointManager::RegisterSelector
 *  Purpose:
 *      Provides the IHXAccessPointManager with an IHXAccessPointSelector
 *      to use when it needs information about the desired access point.
 *
 */
STDMETHODIMP
HXSymbianAccessPointManager::RegisterSelector(THIS_ IHXAccessPointSelector* pSelector)
{
    HX_RESULT res = HXR_FAILED;

    // Currently we only support 1 registered selector so
    // we only allow this call to succeed if m_pAPSelector
    // is not set.

    if (pSelector && !m_pAPSelector)
    {
    m_pAPSelector = pSelector;
    m_pAPSelector->AddRef();

    res = HXR_OK;
    }

    return res;
}

/************************************************************************
 *  Method:
 *      IHXAccessPointManager::UnregisterSelector
 *  Purpose:
 *      Unregisters a previously registered IHXAccessPointSelector
 *
 */
STDMETHODIMP
HXSymbianAccessPointManager::UnregisterSelector(THIS_ IHXAccessPointSelector* pSelector)
{
    HX_RESULT res = HXR_FAILED;

    // We only allow one registered selector so this call
    // will only succeed if the pSelector is a valid pointer
    // and matches the selector that was registered.

    if (pSelector && (pSelector == m_pAPSelector))
    {
    HX_RELEASE(m_pAPSelector);
    }

    return res;
}

/************************************************************************
 *  Method:
 *      IHXAccessPointManager::GetActiveAccessPointInfo
 *  Purpose:
 *      Returns information about the access point we are currently
 *      connected to. This function returns an error if we are
 *      not connected to an access point.
 *
 */
STDMETHODIMP
HXSymbianAccessPointManager::GetActiveAccessPointInfo(THIS_ REF(IHXValues*) pInfo)
{
    HX_RESULT res = HXR_UNEXPECTED;

    pInfo = NULL;

    if (m_pCCF)
    {
    ULONG32 ulActiveID = 0;

    res = GetActiveID(ulActiveID);

    if (HXR_OK == res)
    {
        res = m_pCCF->CreateInstance(CLSID_IHXValues, (void**)&pInfo);

        if (HXR_OK == res)
        {
        res = pInfo->SetPropertyULONG32("ID", ulActiveID);

        if (HXR_OK != res)
        {
            HX_RELEASE(pInfo);
        }
        }
    }
    }


    HXLOGL2(HXLOG_NETW,
        "HXSymbianAccessPointManager::GetActiveAccessPointInfo():%08x", res);

    return res;

}

/************************************************************************
 *  Method:
 *      IHXAccessPointManager::GetPreferredAccessPointInfo
 *  Purpose:
 *      Returns information about the access point we want to connect to.
 */
STDMETHODIMP
HXSymbianAccessPointManager::GetPreferredAccessPointInfo(THIS_ REF(IHXValues*) pInfo)
{
    HX_RESULT res = HXR_FAILED;

    pInfo = m_pPreferredInfo;

    if (pInfo)
    {
    res = HXR_OK;
    }

    HXLOGL2(HXLOG_NETW,
        "HXSymbianAccessPointManager::GetPreferredAccessPointInfo() : %08x",
        res);

    return res;
}

/************************************************************************
 *  Method:
 *      IHXAccessPointManager::SetPreferredAccessPointInfo
 *  Purpose:
 *      Tells the access point manager about the access
 *      point we would like it to connect to.
 */
STDMETHODIMP
HXSymbianAccessPointManager::SetPreferredAccessPointInfo(THIS_ IHXValues* pInfo)
{
    HX_RESULT res = HXR_FAILED;
    ULONG32 ulAccessPointID = 0;

    HX_RELEASE(m_pPreferredInfo);
    
    if (pInfo && (HXR_OK == pInfo->GetPropertyULONG32("ID", ulAccessPointID)))
    {
        HXLOGL2( HXLOG_NETW, 
            "HXSymbianAccessPointManager::SetPreferredAccessPointInfo(%lu)", 
            ulAccessPointID );

        if ( ulAccessPointID <= 0 )
        {
            res = HXR_ACCESSPOINT_NOT_FOUND;
        }
        else
        {
            m_pPreferredInfo = pInfo;
            m_pPreferredInfo->AddRef();
            
            res = HXR_OK;
        }
    }
    
    HXLOGL2( HXLOG_NETW, 
        "HXSymbianAccessPointManager::SetPreferredAccessPointInfo(%x)", res );
    return res;
}

/*
 * IHXAccessPointSelectorResponse methods
 */

/************************************************************************
 *  Method:
 *      IHXAccessPointSelectorResponse::SelectAccessPointDone
 *  Purpose:
 *      Returns the selected access point info
 */
STDMETHODIMP
HXSymbianAccessPointManager::SelectAccessPointDone(THIS_ HX_RESULT status,
                           IHXValues* pInfo)
{
    HX_RESULT res = HXR_UNEXPECTED;
    HXLOGL2( HXLOG_NETW, 
        "HXSymbianAccessPointManager::SelectAccessPointDone(%x)", status);

    if (m_bSelectAPPending)
    {
        m_bSelectAPPending = FALSE;

        if ( (HXR_OK == status) && 
             (HXR_OK == (status = SetPreferredAccessPointInfo(pInfo)) ) )
        {    
            status = DoConnect(0);
        }

        if ( HXR_OK != status )
        {
            if ( (HXR_OUTOFMEMORY != status) && (HXR_ACCESSPOINT_NOT_FOUND != status) )
            {
                status = HXR_NET_CONNECT;
            }

            HXLOGL1( HXLOG_NETW, 
                "HXSymbianAccessPointManager::SelectAccessPointDone() failure %x", 
                status);

            DispatchConnectDones(status);
        }

        res = HXR_OK;
    }

    return res;
}

void
HXSymbianAccessPointManager::SetContext(IUnknown* pContext)
{
    if (pContext)
    {
        HX_RELEASE(m_pContext);
        m_pContext = pContext;
        HX_ADDREF(m_pContext);
        HX_RELEASE(m_pCCF);
        m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCCF);
    }
}

/*
Initialize RConnection, SocketServ.
This should be done before any network connection starts.
*/

HX_RESULT
HXSymbianAccessPointManager::DoInit()
{
    HX_RESULT hxr=HXR_OK;
    if(!m_bInitialized)
    {
        m_pConnector = new HXSymbianApManAO(this);

        if (m_pConnector == NULL)
        {
            hxr = HXR_OUTOFMEMORY;
        }
        else if(m_sockServ.Connect() != KErrNone)
        {
            HX_DELETE(m_pConnector);
            hxr = HXR_FAILED;
        }
        else if(m_rconn.Open(m_sockServ) != KErrNone)
        {
            HX_DELETE(m_pConnector);
            m_sockServ.Close();
            hxr = HXR_FAILED;
        }
        else
        {
            m_state = apIdle;
            m_bInitialized = TRUE;
        }
    }
    return hxr;
}

/*
de-initialization.
*/

HX_RESULT
HXSymbianAccessPointManager::DoClose()
{
    if(m_bInitialized)
    {
        HX_DELETE(m_pConnector);
        m_rconn.Close();
        m_sockServ.Close();
        m_bInitialized = FALSE;
    }
    return HXR_OK;
}

HX_RESULT
HXSymbianAccessPointManager::DoConnect(IHXAccessPointConnectResponse* pResp)
{
    HXLOGL2(HXLOG_NETW, "HXSymbianAccessPointManager::DoConnect(%p)", pResp);

    HX_RESULT res = HXR_FAILED;

    ULONG32 ulActiveId = 0;
    ULONG32 ulPreferredId = 0;

    HXBOOL bQueueResponse = (pResp) ? TRUE : FALSE;

    // make sure that RConnection, RSocketServ are ready for the connection.
    if((res = DoInit()) != HXR_OK)
    {
        if (pResp)
        {
            pResp->ConnectDone(res);
        }

        m_state = apIdle;
        DispatchConnectDones(res);
        return res;
    }

    if (HXR_OK == GetPreferredID(ulPreferredId))
    {
        // We have a preferred access point set

        if (HXR_OK != GetActiveID(ulActiveId))
        {
            // We don't have an active access point

            if (apConnected == m_state)
            {
                // The access point disconnected without
                // our knowledge. Update our state.
                m_state = apIdle;
            }

            // Start to connect
            res = StartConnection();
        }
        else if (ulActiveId == ulPreferredId)
        {
            // We have a preferred access point and we are connected
            // to it.

            // Dispatch the callbacks now.
            if (pResp)
            {
                pResp->ConnectDone(HXR_OK);
                bQueueResponse = FALSE;
            }

            if (apIdle == m_state)
            {
                m_state = apConnected;

                // Dispatch any other pending callbacks
                DispatchConnectDones(HXR_OK);
            }

            res = HXR_OK;
        }
        else
        {
            // We need to disconnect from the current access
            // point
            res = StartDisconnect();
        }
    }
    else
    {
        // We don't have a preferred access point
        if (m_pAPSelector)
        {
            if (!m_bSelectAPPending)
            {
                // Use the Selector to get the preferred access point
                m_bSelectAPPending = TRUE;
                res = m_pAPSelector->SelectAccessPoint(this);

                if (HXR_OK != res)
                {
                    m_bSelectAPPending = FALSE;
                }
            }
            else
            {
                // A SelectAccessPoint() request is currently
                // pending
                res = HXR_OK;
            }
        }
        else
        {
            // We have no way to get access point information.
            res = HXR_NET_CONNECT;
        }
    }

    if ((HXR_OK == res) && bQueueResponse)
    {
        pResp->AddRef();
        m_respList.AddTail(pResp);
    }

    return res;
}

HX_RESULT
HXSymbianAccessPointManager::StartConnection()
{
    HXLOGL2(HXLOG_NETW, "HXSymbianAccessPointManager::StartConnection() m_state:%d", m_state);

    HX_RESULT res = HXR_UNEXPECTED;
    HXBOOL bConnect = TRUE;
    
    if (apConnecting == m_state)
    {
        res = HXR_OK;
        bConnect = FALSE;
    }
    
    if (bConnect && m_pPreferredInfo)
    {
    ULONG32 ulAccessPointID = 0;
    res = m_pPreferredInfo->GetPropertyULONG32("ID", ulAccessPointID);

    if (HXR_OK == res)
    {
        res = m_pConnector->Connect(ulAccessPointID);

        if (HXR_OK == res)
        {
        m_state = apConnecting;
        }
    }
    }

    HXLOGL2(HXLOG_NETW,
        "HXSymbianAccessPointManager::StartConnection() : res %08x\n",
         res);

    return res;
}

void
HXSymbianAccessPointManager::ConnectDone(TInt aoStatus)
{
    HXLOGL2(HXLOG_NETW, "HXSymbianAccessPointManager::ConnectDone aoStatus(%d)\n", aoStatus);
    HX_RESULT status = HXR_OK;

    if (aoStatus == KErrCancel)
    {
        status = HXR_CANCELLED;
    }
    else if ( (aoStatus == KErrNotFound) 
#if defined(HELIX_FEATURE_SYMBIAN_MMF) && defined(HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY)		
             ||  (aoStatus == KErrGprsMissingorUnknownAPN)
#endif              
            )
    {
        status = HXR_ACCESSPOINT_NOT_FOUND;
    }
    #if defined(HELIX_FEATURE_SYMBIAN_MMF) && defined(HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY)
    
      else if(aoStatus ==  KErrWlanNetworkNotFound) 
      { 
       status = HXR_INVALID_ACCESSPOINT; 
      } 
	#endif  
    else if (aoStatus != KErrNone) // all other errors
    {
        if ( aoStatus == KErrGsmMMServiceOptionNotSubscribed         || // -4161
#if defined(HELIX_FEATURE_SYMBIAN_MMF) && defined(HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY)
             aoStatus == KErrGprsInsufficientResources               || // -4154
             aoStatus == KErrGprsActivationRejected                  || // -4159
             aoStatus == KErrGprsRequestedServiceOptionNotSubscribed || // -4161
             aoStatus == KErrGprsServiceOptionTemporarilyOutOfOrder  || // -4162
             aoStatus == KErrGprsServicesNotAllowed                  || // -4135
             aoStatus == KErrUmtsMaxNumOfContextExceededByPhone      || // -4178
             aoStatus == KErrUmtsMaxNumOfContextExceededByNetwork    || // -4179
             aoStatus == KErrPacketDataTsyMaxPdpContextsReached      || // -6000
#endif
             aoStatus == KErrGsmMMServiceOptionTemporaryOutOfOrder   || // -4162
             aoStatus == KErrGsmMMNetworkFailure                      ) // -4145
        {
            status = HXR_NET_RESOURCES_IN_USE;
        }
        else
        {
            status = HXR_NET_CONNECT;
        }
    }
    else
    {
        ULONG32 ulAccessPointID = 0;
		//get ActiveID from RConnection
		//Not using GetActiveID as connection status is not yet updated to connected.
		//Since  this callback method is invoked and no error is found it can be safely assumed to be in 
		//connected state. State is updated at the end of this function
		if(m_rconn.GetIntSetting(_L("IAP\\Id"), ulAccessPointID) != KErrNone)
		{	
			HXLOGL2(HXLOG_NETW, "HXSymbianAccessPointManager::ConnectDone"
                " No Active IAP Id found in RConnection\n");
        if(m_pPreferredInfo &&
           m_pPreferredInfo->GetPropertyULONG32("ID", ulAccessPointID) == HXR_OK)
			{
				HXLOGL2(HXLOG_NETW, "HXSymbianAccessPointManager::ConnectDone No Preferred Access Point Found\n");
				
			    status = HXR_ACCESSPOINT_NOT_FOUND;    	 	
			}
		}
		else //cant get active so fetch the preferred AP ID
		{
		
			HXLOGL2(HXLOG_NETW, "HXSymbianAccessPointManager::ConnectDone"
				" Active IAP Id (%d) found.  Updating as Preferred IAP Id\n", ulAccessPointID);
			if(m_pPreferredInfo) 
				m_pPreferredInfo->SetPropertyULONG32("ID", ulAccessPointID); //No error checks - failure ignored
		}
		//If non-zero Access point ID then proceed
        if(ulAccessPointID)
        {
            // Start the bearer monitor.
            HX_DELETE(m_pMonitor);
            m_pMonitor = new HXConnectionMonitor();
            if(m_pMonitor)
            {
                HXLOGL2(HXLOG_NETW, "HXSymbianAccessPointManager::ConnectDone"
                " m_pMonitor->Open(%d)\n", ulAccessPointID);

                m_pMonitor->AddBearerObserver(this);
                status = m_pMonitor->Open(ulAccessPointID);
            }
            else
            {
                status = HXR_OUTOFMEMORY;
            }
        }
        else
        {
            status = HXR_ACCESSPOINT_NOT_FOUND;
        }

        if (HXR_OK == status)
        {
            m_state = apConnected;
        }
        else
        {
            m_state = apIdle;
        }

    }

    if( status != HXR_OK )
    {
        DispatchConnectDones(status);
    }
}

HX_RESULT
HXSymbianAccessPointManager::StartDisconnect()
{
    HXLOGL2(HXLOG_NETW, "HXSymbianAccessPointManager::StartDisconnect()");

    HX_RESULT res = HXR_OK;

    m_state = apDisconnecting;
    m_pConnector->Disconnect();
    HXLOGL2(HXLOG_NETW,
        "HXSymbianAccessPointManager::StartDisconnect() : res %08x", res);

    return res;
}

void
HXSymbianAccessPointManager::DisconnectDone(HX_RESULT status)
{
    HXLOGL2(HXLOG_NETW,
        "HXSymbianAccessPointManager::DisconnectDone(%08x)", status);

    HX_ASSERT(apDisconnecting == m_state);

    DoClose();
    m_state = apIdle;

    if (HXR_OK == status)
    {
    // Try to connect now
    status = DoConnect(0);
    }

    if (HXR_OK != status)
    {
    // We failed to reconnect. Send the
    // failure code to the response objects
    DispatchConnectDones(status);
    }
}

HXBOOL
HXSymbianAccessPointManager::DispatchConnectDones(HX_RESULT status)
{
    HXLOGL2(HXLOG_NETW,
        "HXSymbianAccessPointManager::DispatchConnectDones(%08x)", status);

    HXBOOL ret = FALSE;

    // Handle any pending connect requests
    while(!m_respList.IsEmpty())
    {
    IHXAccessPointConnectResponse* pResp =
        (IHXAccessPointConnectResponse*)m_respList.RemoveHead();

    pResp->ConnectDone(status);
    HX_RELEASE(pResp);
    ret = TRUE;
    }

    return ret;
}

HX_RESULT
HXSymbianAccessPointManager::GetPreferredID(REF(ULONG32) ulID)
{
    HX_RESULT res = HXR_FAILED;

    if (m_pPreferredInfo)
    {
    res = m_pPreferredInfo->GetPropertyULONG32("ID", ulID);
	HXLOGL3(HXLOG_NETW, "HXSymbianAccessPointManager::GetPreferredID"
		" Preferred IAP Id: %d\n", ulID);
    }

    return res;
}

HX_RESULT
HXSymbianAccessPointManager::GetActiveID(REF(ULONG32) ulID)
{
    HX_RESULT res = HXR_FAILED;
    TUint32 iapId = 0;
    if ( (m_state != apError) && m_state == apConnected &&
         (m_rconn.GetIntSetting(_L("IAP\\Id"), iapId) == KErrNone)
       )
    {
        ulID = iapId;
        res = HXR_OK;
    }
    return res;
}

RConnection&
HXSymbianAccessPointManager::GetRConnection()
{
    return m_rconn;
}

RSocketServ&
HXSymbianAccessPointManager::GetRSocketServ()
{
    return m_sockServ;
}

void
HXSymbianAccessPointManager::BearerChanged(const HXBearerType bearer,
                                     const HXNetworkGeneration generation,
                                     TUint dlBandwidth, TUint ulBandwidth)
{

    if(bearer == BearerUnknown)
    {
        HXLOGL2(HXLOG_NETW, "HXSymbianAccessPointManager::BearerChanged Error");
        DispatchConnectDones(HXR_NET_CONNECT);
        return;
    }

    const char *pStrBearer      = m_pMonitor->GetBearer();

    HXLOGL2(HXLOG_NETW, "HXSymbianAccessPointManager::BearerChanged"
         "bearer=%d gen=%d bearer=%s gen=%s\n", bearer, generation,
          pStrBearer, m_pMonitor->GetGeneration());

    HXLOGL2(HXLOG_NETW, "HXSymbianAccessPointManager::BearerChanged"
        "dlBandwidth=%d ulBandwidth=%d\n", dlBandwidth, ulBandwidth);

#if defined(HELIX_FEATURE_PREFERENCES)

    // update the XWapProfileURI pref depending on the generation.
    CHXString pUAProfURL;
    if(generation == ThirdG)
    {
        ReadPrefCSTRING(m_pContext, "3GUAProfURL", pUAProfURL);
    }
    else
    {
        ReadPrefCSTRING(m_pContext, "2GUAProfURL", pUAProfURL);
    }

    if(!pUAProfURL.IsEmpty())
    {
        WritePrefCSTRING(m_pContext, "XWapProfileURI", pUAProfURL);
    }

    // Get the maximum bandwidth for the current bearer.
    CHXString maxBandwidthStr = pStrBearer;
    maxBandwidthStr += "MaximumBandwidth";
    UINT32 maxBandwidth = 0;
    ReadPrefUINT32(m_pContext, maxBandwidthStr, maxBandwidth);

    // Set the maxBandwidth to current Bandwidth prefs.
    if(maxBandwidth != 0)
    {
        WritePrefUINT32(m_pContext, "Bandwidth", maxBandwidth);
        WritePrefUINT32(m_pContext, "MaxBandwidth", maxBandwidth);
    }

    DispatchConnectDones(HXR_OK);

#else
#error "HELIX_FEATURE_PREFERENCES Must be defined"
#endif

}

void
HXSymbianAccessPointManager::OnBearerError(HX_RESULT hxr)
{
    HXLOGL1(HXLOG_NETW, "HXSymbianAccessPointManager::OnBearerError %x\n",hxr);

    if (!DispatchConnectDones(hxr))
    {
        IHXErrorMessages* pErrorMessage = NULL;
        m_pContext->QueryInterface(IID_IHXErrorMessages, (void**) &pErrorMessage);
        if (pErrorMessage)
        {
            pErrorMessage->Report(HXLOG_ERR, hxr, 0, NULL, NULL);
            HX_RELEASE(pErrorMessage);
        }
    }
}


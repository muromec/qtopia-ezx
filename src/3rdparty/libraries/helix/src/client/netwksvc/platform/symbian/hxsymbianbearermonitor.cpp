
#include "platform/symbian/hxsymbianbearermonitor.h"

// Bearer Information

struct HXBearerNameType {
    const char* m_name;
    HXBearerType m_bearer;
    HXNetworkGeneration m_generation;
};

// Table of bearers
static HXBearerNameType const HXBearerTable[] = {
    { "Default",    BearerDefault,  SecondG },
    { "CSD",        BearerCSD,      SecondG },
    { "HSCSD",      BearerHSCSD,    SecondG },
    { "GPRS",       BearerGPRS,     SecondG },
    { "EGPRS",      BearerEGPRS,    SecondG },
    { "WCDMA",      BearerWCDMA,    ThirdG },
    { "CDMA2000",   BearerCDMA2000, ThirdG },
    { "WLAN",       BearerWLAN,     ThirdG },
    { "Bluetooth",  BearerBlueTooth,ThirdG },
    { "LAN",        BearerLAN,      ThirdG },
    { "HSDPA",      BearerHSDPA,    ThirdG }
};

static const int NumOfBearers = sizeof(HXBearerTable)/sizeof(HXBearerNameType);

// Network Generation
struct HXGenNameType {
    const char* m_name;
    HXNetworkGeneration m_generation;
};


// Table of generation
static HXGenNameType const HXGenTable[] = {
    { "2G", SecondG},
    { "3G", ThirdG},
    { "Undefined", NotDefined}
};

static const int NumOfGenerations = sizeof(HXGenTable)/sizeof(HXGenNameType);

struct HXConnBearerInfo{
    TConnMonBearerType m_connBearer;
    TConnMonBearerId   m_connBearerId;
    HXBearerType m_bearer;
};


// Connection Bearer Table maps the system bearer to Helix bearer
static HXConnBearerInfo const HXConnBearerTable[] = {
    { EBearerCSD,               EBearerIdCSD,   BearerCSD       },
    { EBearerExternalCSD,       EBearerIdCSD,   BearerCSD       },
    { EBearerWCDMA,             EBearerIdWCDMA, BearerWCDMA     },
    { EBearerExternalWCDMA,     EBearerIdWCDMA, BearerWCDMA     },
#if defined(HELIX_FEATURE_HSDPA_BEARER_SUPPORT)
    { EBearerHSDPA,             EBearerIdWCDMA, BearerHSDPA     },
    { EBearerExternalHSDPA,     EBearerIdWCDMA, BearerHSDPA     },
#endif
    { EBearerLAN,               EBearerIdAll,   BearerLAN       },
    { EBearerExternalLAN,       EBearerIdAll,   BearerLAN       },
    { EBearerCDMA2000,          EBearerIdAll,   BearerCDMA2000  },
    { EBearerExternalCDMA2000,  EBearerIdAll,   BearerCDMA2000  },
    { EBearerGPRS,              EBearerIdGPRS,  BearerGPRS      },
    { EBearerExternalGPRS,      EBearerIdGPRS,  BearerGPRS      },
    { EBearerHSCSD,             EBearerIdCSD,   BearerHSCSD     },
    { EBearerExternalHSCSD,     EBearerIdCSD,   BearerHSCSD     },
    { EBearerEdgeGPRS,          EBearerIdGPRS,  BearerEGPRS     },
    { EBearerExternalEdgeGPRS,  EBearerIdGPRS,  BearerEGPRS     },
    { EBearerWLAN,              EBearerIdAll,   BearerWLAN      },
    { EBearerExternalWLAN,      EBearerIdAll,   BearerWLAN      },
    { EBearerBluetooth,         EBearerIdAll,   BearerBlueTooth },
    { EBearerExternalBluetooth, EBearerIdAll,   BearerBlueTooth }
};

static const int NumofConnBearers = sizeof(HXConnBearerTable)/sizeof(HXConnBearerInfo);

// Converts the system bearer to the Helix bearer
static
HXBearerType ConntoBearer(int connBearer)
{

    HXBearerType Bearer = BearerUnknown;

        for (int i=0 ; i < NumofConnBearers; ++i)
        {
            if (connBearer == HXConnBearerTable[i].m_connBearer)
            {
                Bearer = HXConnBearerTable[i].m_bearer;
                break;
            }
        }
    return Bearer;
}


//Converts system bearer to bearer id
static
TConnMonBearerId ConnBearerId(int connBearer)
{
    TConnMonBearerId connBearerId = EBearerIdAll;

    for (int i=0; i < NumofConnBearers; ++i)
    {
        if (connBearer == HXConnBearerTable[i].m_connBearer)
        {
            connBearerId = HXConnBearerTable[i].m_connBearerId;
            break;
        }
    }
    return connBearerId;
}


//Gets the network generation of the current Helix bearer
static
HXNetworkGeneration ConnNetworkGen(HXBearerType bearer)
{
    HXNetworkGeneration networkgen = SecondG;
     for (int i=0; i < NumOfBearers; ++i)
     {
            if (bearer == HXBearerTable[i].m_bearer)
            {
                networkgen = HXBearerTable[i].m_generation;
                break;
            }
    }
    return networkgen;
}


HXConnectionMonitor::HXConnectionMonitor()
    :CActive(EPriorityNormal),
    m_state(Closed),
    m_monitoring(FALSE),
    m_queryIAPid(0),
    m_connCount(0),
    m_connNum(0),
    m_connId(0),
    m_subConnCount(0),
    m_connIAPid(0),
    m_connBearer(0),
    m_ulBandwidth(0),
    m_dlBandwidth(0)
{}


HXConnectionMonitor::~HXConnectionMonitor()
{
    Close();
}

/*
 * HXConnectionMonitor methods
 */

/****************************************************************************************************
 *  Method:
        HXConnectionMonitor::Open
    Purpose:
        Check that the bearer monitor is not already open
        Open Bearer Monitor. Start the bearer monitor state machine running off an Active Object
        using the IAP id supplied by the client
 *****************************************************************************************************/

HX_RESULT
HXConnectionMonitor::Open(int IAPId)
{
	HX_RESULT res = HXR_OK;

	if (m_state == Closed)
    {
        TInt err2 = KErrNone;
        // Connect to the Connection Monitor
        TRAPD( err1, err2 = m_monitor.ConnectL());

        if (err1 == KErrNone && err2 == KErrNone)
        {
            m_queryIAPid = IAPId;
            m_monitor.GetConnectionCount(m_connCount, iStatus);
            CActiveScheduler::Add(this);
            SetActive();
            m_state = GettingCount;
        }
        else
        {
        res = HXR_NET_CONNECT;
        }
    }
    else
    {
        res = HXR_NET_CONNECT;
    }

    return res;
}

/****************************************************************************************************
 *  Method:
        HXConnectionMonitor::Close
    Purpose:
        Close observers on the Connection Monitor, cancel all notifications

 *****************************************************************************************************/

void
HXConnectionMonitor::Close()
{
    if (m_state != Closed)
    {
        if (IsActive())
        {
            Cancel();
        }

        if (m_monitoring)
        {
            m_monitor.CancelNotifications();
            m_monitoring = FALSE;
        }
        m_monitor.Close();
        m_observers.RemoveAll();
        }
}

/****************************************************************************************************
 *  Method:
        HXConnectionMonitor::AddBearerObserver
    Purpose:
        Add observer to the list of the observers on the BearerMonitor

 *****************************************************************************************************/

void
HXConnectionMonitor::AddBearerObserver(const HXConnectionMonitorObserver* pObs)
{
    if(pObs)
        m_observers.AddTail((void *)pObs);
}

/****************************************************************************************************
 *  Method:
        HXConnectionMonitor::RemoveBearerObserver
    Purpose:
        Remove an observer from the list of the observers on the BearerMonitor

 *****************************************************************************************************/

void
HXConnectionMonitor::RemoveBearerObserver(const HXConnectionMonitorObserver* pObs)
{
    LISTPOSITION lPosition = NULL;

    lPosition = m_observers.Find((void *)pObs);
            if (lPosition)
            {
                m_observers.RemoveAt(lPosition);
            }
}



/****************************************************************************************************
 *  Method:
        HXConnectionMonitor::GetBearer
    Purpose:
        Returns the bearer string of the current system bearer

 *****************************************************************************************************/

const char*
HXConnectionMonitor::GetBearer( ) const
{
    HXBearerType bearer = ConntoBearer(m_connBearer);
  int i;
    for(i = 0; i < NumOfBearers; i++)
    {
        if (bearer == HXBearerTable[i].m_bearer)
            break;
    }


    return HXBearerTable[i].m_name;
}


/****************************************************************************************************
 *  Method:
        HXConnectionMonitor::GetGeneration
    Purpose:
        Returns the generation string of the current system bearer

 *****************************************************************************************************/

const char*
HXConnectionMonitor::GetGeneration( ) const
{
    HXBearerType bearer = ConntoBearer(m_connBearer);
    HXNetworkGeneration gen = ConnNetworkGen(bearer);
  int i;
    for(i = 0; i < NumOfGenerations; i++)
    {
        if (gen == HXGenTable[i].m_generation)
            break;
    }

    return HXGenTable[i].m_name;

}


/****************************************************************************************************
 *  Method:
        HXConnectionMonitor::GetBandwidth
    Purpose:
        Returns the bandwidth of the current system bearer

 *****************************************************************************************************/


HXBOOL
HXConnectionMonitor::GetBandwidth(TUint& ulBandwidth, TUint& dlBandwidth) const
{
    if (m_state == Opened)
    {
        ulBandwidth = m_ulBandwidth;
        dlBandwidth = m_dlBandwidth;
    }

    return (m_state == Opened && (ulBandwidth != 0 || dlBandwidth !=0));

}


/****************************************************************************************************
 *  Method:
        HXConnectionMonitor::RunL
    Purpose:
        Walk through the Bearer Monitor State Machine:
        Get number of connections and request the connection attributes
        Compare the query IAPId with return IAPID. If they match, request the bearer info
        Get Bearer Info. Convert ConnectionMonitor bearer id to external type and notify client.


 *****************************************************************************************************/


void
HXConnectionMonitor::RunL()
{
    if(iStatus.Int() == KErrNone)
    {
        switch (m_state)
        {
            case    GettingCount:
                m_state = Error;
                if(m_connCount >  0)
                {
					m_connNum = 1;
                    m_monitor.GetConnectionInfo(m_connNum, m_connId, m_subConnCount);
                    m_monitor.GetUintAttribute(m_connId, 0, KIAPId, m_connIAPid, iStatus);
                    SetActive();
                    m_state = Enumerating;
                }
                else
                {
                    OnError(HXR_NET_CONNECT);
                }
                break;

            case    Enumerating:
                m_state = Error;
                if(m_connIAPid == m_queryIAPid)
                {
                    m_monitor.GetIntAttribute(m_connId, 0, KBearer, (TInt&)m_connBearer, iStatus);
                    SetActive();
                    m_state = GettingBearer;
                }
                else
                {
                    if (m_connNum < m_connCount)
                    {
                        m_connNum++;
                        m_monitor.GetConnectionInfo(m_connNum, m_connId, m_subConnCount);
                        m_monitor.GetUintAttribute(m_connId, 0, KIAPId, m_connIAPid, iStatus);
                        SetActive();
                        m_state = Enumerating;
                    }
                    else
                    {
                        OnError(HXR_NET_CONNECT);
                    }
                }
                break;



            case    GettingBearer:
                {
                m_state = Opened;
                HXBearerType  bearer = ConntoBearer(m_connBearer);
                HXNetworkGeneration generation = ConnNetworkGen(bearer);
                NotifyObservers(bearer, generation, m_dlBandwidth, m_ulBandwidth);

                if (!m_monitoring)
                m_monitoring = m_monitor.NotifyEventL(*this) == KErrNone;
                break;
                }

            case    Opened:
            break;
            }//end switch
        }
    else
    {
        m_state = Error;
        OnError(HXR_NET_CONNECT);
    }
}


/****************************************************************************************************
 *  Method:
        HXConnectionMonitor::DoCancel
    Purpose:
        Cancel asynch requests to Connection Monitor

 *****************************************************************************************************/


void
HXConnectionMonitor::DoCancel()
{
    switch (m_state)
    {
        case GettingCount:
        m_monitor.CancelAsyncRequest(EConnMonGetConnectionCount);
        break;

        case Enumerating:
        m_monitor.CancelAsyncRequest(EConnMonGetIntAttribute);
        break;

        case GettingBearer:
        m_monitor.CancelAsyncRequest(EConnMonGetIntAttribute);
        break;

        case Opened:
        break;

        default:
        break;
    }
    m_state = Closed;
}


/****************************************************************************************************
 *  Method:
        HXConnectionMonitor::EventL
    Purpose:
        Handler for bearer monitor events

 *****************************************************************************************************/

void
HXConnectionMonitor::EventL(const CConnMonEventBase& event)
{
    if (m_monitoring && event.EventType()== EConnMonBearerChange)
    {
        const CConnMonBearerChange* pBearerEvent
            = (CConnMonBearerChange*)(&event);

        HandleBearerChange(pBearerEvent->ConnectionId(), pBearerEvent->Bearer());
    }
    if(m_monitoring && event.EventType()==EConnMonNetworkStatusChange )
    {
        const CConnMonNetworkStatusChange* pNetStatus
            = (CConnMonNetworkStatusChange*)(&event);
        if(EConnMonStatusSuspended == pNetStatus->NetworkStatus()
        && event.ConnectionId() == m_connId)
        {
            //right now if the network becomes suspended in gprs/2g
            //we need to error out because we have gone out of
            //sync with the network
            //no status change will be sent in 3g, no need to check
            OnError(HXR_NET_SUSPENDED);
        }
    }
    if(m_monitoring && event.EventType()==EConnMonDeleteConnection)
    {
        if( event.ConnectionId()==m_connId)
        {
            OnError(HXR_NET_CONNECT);
        }
    }
}

/****************************************************************************************************
 *  Method:
        HXConnectionMonitor::HandleBearerChange
    Purpose:
        Handles the logic for changing the bearer

 *****************************************************************************************************/


void
HXConnectionMonitor::HandleBearerChange(TUint connId, TUint connBearer)
{
    if (connId == ConnBearerId(m_connBearer))
    {
        HXBearerType prevbearer = ConntoBearer(m_connBearer);
        HXBearerType newbearer = ConntoBearer(connBearer);
        HXNetworkGeneration newgen = ConnNetworkGen(newbearer);

        m_connBearer = connBearer;

        if( newbearer != prevbearer )
        {
            NotifyObservers( newbearer, newgen, m_dlBandwidth, m_ulBandwidth);
        }
    }
}


/****************************************************************************************************
 *  Method:
        HXConnectionMonitor::OnError
    Purpose:
        Iterates through the list of observers and notifies them of error in bearer.

 *****************************************************************************************************/
void
HXConnectionMonitor::OnError(HX_RESULT error)
{
    HXConnectionMonitorObserver* m_Obs;
    CHXSimpleList::Iterator lIter = m_observers.Begin();
    for (; lIter != m_observers.End(); ++lIter)
    {
        m_Obs = (HXConnectionMonitorObserver*) (*lIter);
        m_Obs->OnBearerError(error);
    }
}

/****************************************************************************************************
 *  Method:
        HXConnectionMonitor::NotifyOberservers
    Purpose:
        Iterates through the list of observers and notifies them of the new bearer.

 *****************************************************************************************************/
void
HXConnectionMonitor::NotifyObservers(const HXBearerType& bearer, const HXNetworkGeneration& generation, TUint dlBandwidth, TUint ulBandwidth)
{

    HXConnectionMonitorObserver* m_Obs;
    CHXSimpleList::Iterator lIter = m_observers.Begin();
            for (; lIter != m_observers.End(); ++lIter)
            {
                m_Obs = (HXConnectionMonitorObserver*) (*lIter);
                m_Obs->BearerChanged( bearer, generation, dlBandwidth, ulBandwidth);
            }

}


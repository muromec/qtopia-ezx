/* **** BEGIN LICENSE BLOCK ****
 *TO DO
 * **** END LICENSE BLOCK **** */


#ifndef _BEARER_MONITOR_H__
#define _BEARER_MONITOR_H__

#include <e32base.h>
#include <es_sock.h>
#include <rconnmon.h>
#include "hxslist.h"

enum HXBearerType{
    BearerUnknown = 0,
    BearerDefault,
    BearerCSD,
    BearerHSCSD,
    BearerGPRS,
    BearerEGPRS,
    BearerWCDMA,
    BearerWLAN,
    BearerBlueTooth,
    BearerCDMA2000,
    BearerLAN,
    BearerHSDPA
};

enum HXNetworkGeneration{
    SecondG = 0,
    ThirdG,
    NotDefined
};


// List of observers on the Connection Monitor
class CHXSimpleList;

/*
 * Bearer Monitor Observer
*/

class HXConnectionMonitorObserver
{
public:

    // Pure virtual functions
    // For Bearer Errors
    virtual void OnBearerError(HX_RESULT hxr) = 0;

    // For Bearer Change Events
    virtual void BearerChanged(const HXBearerType bearer, const HXNetworkGeneration generation, TUint dlBandwidth, TUint ulBandwidth) = 0;

};


/*
* Bearer Monitor Implementation

  Symbian implementation of bearer monitoring based on the RConnectionMonitor.
  Provides functions for getting the current bearer and network generation.
  Provides notification of changes in the bearer.
  Hooks are provided for obtaining the downlink and uplink bandwidth, but this data is currently not available.

*/
class HXConnectionMonitor : public CActive,
                        public MConnectionMonitorObserver
{
public:
    HXConnectionMonitor();
    ~HXConnectionMonitor();

    // Open bearer monitor on the IAP provided
    HX_RESULT Open(int IAPId);

    // Closer bearer monitor
    void Close();

    // Add Observer on bearer change
    void AddBearerObserver(const HXConnectionMonitorObserver* pObs);

    // Remove Observer on bearer change
    void RemoveBearerObserver(const HXConnectionMonitorObserver* pObs);

    // Get Current Bearer
    const char* GetBearer( ) const;

    const char* GetGeneration( ) const;

    // Get Current Uplink and Downlink Bandwidth
    HXBOOL GetBandwidth(TUint& ulBandwidth, TUint& dlBandwidth) const;

protected:
    virtual void RunL();
    virtual void DoCancel();

    //Observer on MConnectionMonitorObserver
    virtual void EventL(const CConnMonEventBase& aConnMonEvent);

    //Handles bearer change event from Connection Monitor
    void HandleBearerChange(TUint connId, TUint connBearer);

    //Notifies all observers of bearer error
    void OnError(HX_RESULT error);

    //Notifies all observers of bearer change with network generation
    void NotifyObservers(const HXBearerType& bearer, const HXNetworkGeneration& generation, TUint dlBandwidth, TUint ulBandwidth);


private:
    enum State{
        Closed = 0,
        GettingCount,
        Enumerating,
        GettingBearer,
        Opened,
        Error
    };

    RConnectionMonitor  m_monitor;           //ConnectionManager Handle
    State               m_state;             //Current state
    HXBOOL              m_monitoring;        //TRUE when bearer changes
    TUint               m_queryIAPid;        //IAP Id to find bearer for
    TUint               m_connCount;         //Number of connections
    TUint               m_connNum;           //Connection number
    TUint               m_connId;            //Connection monitor id
    TUint               m_subConnCount;      //Number of subconnections
    TUint               m_connIAPid;         //Connection monitor IAP id
    TUint               m_connBearer;        //Current bearer
    TUint               m_dlBandwidth;       //Current downlink bandwidth
    TUint               m_ulBandwidth;       //Current uplink bandwidth
    CHXSimpleList       m_observers;         //List of observers
    HX_RESULT           m_connErr;
};




#endif // End of #ifndef _BEARER_MONITOR_H__


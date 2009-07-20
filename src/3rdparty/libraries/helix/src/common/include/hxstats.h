/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxstats.h,v 1.19 2006/12/04 18:05:31 gwright Exp $
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

///////////////////////////////////////////////////////////////////////////////
// hxstats.h - Client stats interfaces. 
///////////////////////////////////////////////////////////////////////////////

#ifndef HXSTATS_H
#define HXSTATS_H

///////////////////////////////////////////////////////////////////////////////
// INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include <limits.h>


///////////////////////////////////////////////////////////////////////////////
// ClientStatsEvent
///////////////////////////////////////////////////////////////////////////////

typedef enum 
{
    CSEVENT_UNKNOWN,
    CSEVENT_CLIENT_CONNECT,
    CSEVENT_CLIENT_DISCONNECT,
    CSEVENT_SESSION_DONE,
    CSEVENT_TIMER,
    CSEVENT_SESSION_SETURL
} ClientStatsEvent;

///////////////////////////////////////////////////////////////////////////////

_INTERFACE      IUnknown;

_INTERFACE      IHXBuffer;
_INTERFACE      IHXQoSTransportAdaptationInfo;
_INTERFACE      IHXQoSSessionAdaptationInfo;
_INTERFACE      IHXQoSApplicationAdaptationInfo;
_INTERFACE      IHXClientProfileInfo;

_INTERFACE      IHXClientStats;
_INTERFACE      IHXSessionStats;
_INTERFACE      IHXClientStatsSink;
_INTERFACE      IHXClientStatsManager;


///////////////////////////////////////////////////////////////////////////////
// Interface: 
//
//      IHXSessionStats
//
// Purpose:
//
//      Allows access to session statistics. 
//
// IID_IHXSessionStats:
//
//      {3C02C47F-6F44-47fd-B625-C81A2BF05D4F}
//
///////////////////////////////////////////////////////////////////////////////

// {3C02C47F-6F44-47fd-B625-C81A2BF05D4F}
DEFINE_GUID(IID_IHXSessionStats, 0x3c02c47f, 0x6f44, 0x47fd, 0xb6, 
            0x25, 0xc8, 0x1a, 0x2b, 0xf0, 0x5d, 0x4f);

#define CLSID_IHXSessionStats     IID_IHXSessionStats

#undef  INTERFACE
#define INTERFACE   IHXSessionStats

///////////////////////////////////////////////////////////////////////////////
// End Status Values
///////////////////////////////////////////////////////////////////////////////

/*
 * First bit indicates success/error.
 * Next two bits indicate error source: server, client, + room for expansion
 *
 * 000  No error
 * 100  Client/remote-socket error
 * 101  Server error
 * 110  Undetermined
 * 111  Reserved
 */

#define SSES_ERROR_MASK               0xE0000000
#define SSES_CLIENT_ERROR_VALUE       0x80000000
#define SSES_SERVER_ERROR_VALUE       0xA0000000
#define SSES_UNDETERMINED_ERROR_VALUE 0xC0000000

#define SSES_SUCCESS(x) (!((x) & 0x80000000))
#define SSES_FAILURE(x)   ((x) & 0x80000000)

#define SSES_FAILURE_CLIENT(x)  (((x) & SSES_ERROR_MASK) == SSES_CLIENT_ERROR_VALUE)
#define SSES_FAILURE_SERVER(x)  (((x) & SSES_ERROR_MASK) == SSES_SERVER_ERROR_VALUE)
#define SSES_FAILURE_UNDETERMINED(x)  \
                          (((x) & SSES_ERROR_MASK) == SSES_UNDETERMINED_ERROR_VALUE)

typedef enum
{
    // success codes
    SSES_OK=0,

    // client errors
    SSES_SOCKET_CLOSED        =0x80000000,
    SSES_CLIENT_TIMEOUT       =0x80000001,
    SSES_INVALID_FILE         =0x80000002,

    // server errors
    SSES_SERVER_INTERNAL_ERROR=0xA0000000,
    SSES_SERVER_ALERT         =0xA0000001,
    SSES_SOCKET_BLOCKED       =0xA0000002,
    SSES_REDIRECTED           =0xA0000003,

    // indeterminate errors
    SSES_UNKNOWN_ERROR=0xC0000000,

    SSES_NOT_ENDED=0xffffffff	// session still active, no end status yet
} SessionStatsEndStatus;

DECLARE_INTERFACE_(IHXSessionStats, IUnknown)
{

    // IUnknown methods

    STDMETHOD(QueryInterface)               (THIS_
                                            REFIID riid,
                                            void** ppvObj) PURE;

    STDMETHOD_(UINT32,AddRef)               (THIS) PURE;

    STDMETHOD_(UINT32,Release)              (THIS) PURE;


    STDMETHOD_(UINT32, GetID)               (THIS) PURE;
    STDMETHOD(SetID)                        (THIS_
                                            UINT32 ulID) PURE;


    // IHXSessionStats methods.

    ///////////////////////////////////////////////////////////////////////////
    // Statistics accessor/mutator methods.
    //
    // Purpose:
    //      All used to either acquire a property value or set it.
    //
    // Arguments:
    //      Mutators take an IN parameter, the new value to set the property
    //      to. Accessors take nothing.
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD_(IHXClientStats*, GetClient) (THIS) PURE;
    STDMETHOD(SetClient)                    (THIS_ 
                                            IHXClientStats* pClient) PURE;

    STDMETHOD_(IHXBuffer*, GetHost)         (THIS) PURE;
    STDMETHOD(SetHost)                      (THIS_
                                            IHXBuffer* pHost) PURE;

    STDMETHOD_(IHXBuffer*, GetSessionStartTime) (THIS) PURE;
    STDMETHOD(SetSessionStartTime)              (THIS_
                                                IHXBuffer* pSessionStartTime) PURE;

    STDMETHOD_(IHXBuffer*, GetURL)          (THIS) PURE;
    STDMETHOD(SetURL)                       (THIS_
                                             IHXBuffer* pURL)       PURE;

    STDMETHOD_(IHXBuffer*, GetLogURL)          (THIS) PURE;
    STDMETHOD(SetLogURL)                       (THIS_
                                             IHXBuffer* pLogURL)       PURE;

    STDMETHOD_(IHXBuffer*, GetLogStats)     (THIS) PURE;
    STDMETHOD(SetLogStats)                  (THIS_
                                            IHXBuffer* pLogStats) PURE;

    STDMETHOD_(IHXBuffer*, GetPlayerRequestedURL)   (THIS) PURE;
    STDMETHOD(SetPlayerRequestedURL)                (THIS_
                                                    IHXBuffer* pPlayerRequestedURL) PURE;

    STDMETHOD_(IHXBuffer*, GetSalt)         (THIS) PURE;
    STDMETHOD(SetSalt)                      (THIS_
                                            IHXBuffer* pSalt) PURE;

    STDMETHOD_(IHXBuffer*, GetAuth)         (THIS) PURE;
    STDMETHOD(SetAuth)                      (THIS_
                                            IHXBuffer* pAuth) PURE;

    STDMETHOD_(IHXBuffer*, GetProxyConnectionType)   (THIS) PURE;
    STDMETHOD(SetProxyConnectionType)                (THIS_
                                                     IHXBuffer* pProxyConnectionType) PURE;

    STDMETHOD_(IHXBuffer*, GetInterfaceAddr)    (THIS) PURE;
    STDMETHOD(SetInterfaceAddr)                 (THIS_
                                                IHXBuffer* pInterfaceAddr) PURE;



    STDMETHOD_(UINT64, GetFileSize)         (THIS) PURE;
    STDMETHOD(SetFileSize)                  (THIS_
                                            UINT64 ulFileSize) PURE;

    STDMETHOD_(UINT32, GetStatus)            (THIS) PURE;
    STDMETHOD(SetStatus)                    (THIS_
                                            UINT32 ulHTTPStatus) PURE;

    STDMETHOD_(SessionStatsEndStatus, GetEndStatus)   (THIS) PURE;
    STDMETHOD(SetEndStatus)                 (THIS_
                                            SessionStatsEndStatus ulEndStatus) PURE;

    STDMETHOD_(UINT32, GetDuration)         (THIS) PURE;
    STDMETHOD(SetDuration)                  (THIS_
                                            UINT32 ulDuration) PURE;

    STDMETHOD_(UINT32, GetAvgBitrate)       (THIS) PURE;
    STDMETHOD(SetAvgBitrate)                (THIS_
                                            UINT32 ulAvgBitrate) PURE;

    STDMETHOD_(UINT32, GetSendingTime)      (THIS) PURE;
    STDMETHOD(SetSendingTime)               (THIS_
                                            UINT32 ulSendingTime) PURE;

    STDMETHOD_(UINT32, GetPlayTime)         (THIS) PURE;
    STDMETHOD(SetPlayTime)                  (THIS_
                                            UINT32 ulPlayTime) PURE;

    STDMETHOD_(HXBOOL, IsUDP)                 (THIS) PURE;
    STDMETHOD(SetUDP)                       (THIS_
                                             HXBOOL bIsUDP) PURE;

    STDMETHOD_(HXBOOL, IsRVStreamFound)                 (THIS) PURE;
    STDMETHOD(SetRVStreamFound)                       (THIS_
                                             HXBOOL bIsRVStreamFound) PURE;

    STDMETHOD_(HXBOOL, IsRAStreamFound)                 (THIS) PURE;
    STDMETHOD(SetRAStreamFound)                       (THIS_
                                             HXBOOL bIsRAStreamFound) PURE;

    STDMETHOD_(HXBOOL, IsREStreamFound)                 (THIS) PURE;
    STDMETHOD(SetREStreamFound)                       (THIS_
                                             HXBOOL bIsREStreamFound) PURE;

    STDMETHOD_(HXBOOL, IsRIStreamFound)                 (THIS) PURE;
    STDMETHOD(SetRIStreamFound)                       (THIS_
                                             HXBOOL bIsRIStreamFound) PURE;

    STDMETHOD_(HXBOOL, IsMulticastUsed)       (THIS) PURE;
    STDMETHOD(SetMulticastUsed)             (THIS_
                                            HXBOOL bIsMulticastUsed) PURE;

    STDMETHOD_(UINT16, GetXWapProfileStatus) (THIS) PURE;
    STDMETHOD(SetXWapProfileStatus)         (THIS_ 
                                            UINT16 unXWapProfileStatus) PURE;

    STDMETHOD(GetClientProfileInfo)         (THIS_  
                                            REF(IHXClientProfileInfo*) pInfo) PURE;
    STDMETHOD(SetClientProfileInfo)         (THIS_  
                                            IHXClientProfileInfo* pInfo) PURE;

    STDMETHOD_(IHXBuffer*, GetClientProfileURIs)    (THIS) PURE;
    STDMETHOD(SetClientProfileURIs)                 (THIS_ 
                                                    IHXBuffer* pURIs) PURE;

    STDMETHOD_(IHXQoSTransportAdaptationInfo*, GetQoSTransportAdaptationInfo)       (THIS) PURE;
    STDMETHOD(SetQoSTransportAdaptationInfo)                                        (THIS_
                                                                                     IHXQoSTransportAdaptationInfo* pInfo) PURE;

    STDMETHOD_(IHXQoSSessionAdaptationInfo*, GetQoSSessionAdaptationInfo)       (THIS) PURE;
    STDMETHOD(SetQoSSessionAdaptationInfo)                                      (THIS_
                                                                                 IHXQoSSessionAdaptationInfo* pInfo) PURE;

    STDMETHOD_(IHXQoSApplicationAdaptationInfo*, GetQoSApplicationAdaptationInfo)       (THIS) PURE;
    STDMETHOD(SetQoSApplicationAdaptationInfo)                                          (THIS_
                                                                                         IHXQoSApplicationAdaptationInfo* pInfo) PURE;

};


///////////////////////////////////////////////////////////////////////////////
// Interface: 
//
//      IHXSessionStats2
//
// Purpose:
//      New interface adds start-up statistics. Inherits
//      IHXSessionStats interface.
//
// IID_IHXSessionStats:
//
//      {A5D172EB-47BC-4ee4-AA40-E872C2891A54}
//
///////////////////////////////////////////////////////////////////////////////

// {A5D172EB-47BC-4ee4-AA40-E872C2891A54}
DEFINE_GUID(IID_IHXSessionStats2, 0xa5d172eb, 0x47bc, 0x4ee4,
            0xaa, 0x40, 0xe8, 0x72, 0xc2, 0x89, 0x1a, 0x54);

#define CLSID_IHXSessionStats2     IID_IHXSessionStats2

#undef  INTERFACE
#define INTERFACE   IHXSessionStats2

DECLARE_INTERFACE_(IHXSessionStats2, IHXSessionStats)
{
    // IHXSessionStats2 methods

    STDMETHOD_(UINT32, GetConnectTime)       (THIS) PURE;
    STDMETHOD(SetConnectTime)                (THIS_
                                             UINT32 ulConnectTime) PURE;

    STDMETHOD_(UINT32, GetSessionEstablishmentTime)  (THIS) PURE;
    STDMETHOD(SetSessionEstablishmentTime)           (THIS_
                                             UINT32 ulSessionEstablishmentTime) PURE;

    STDMETHOD_(UINT32, GetSessionSetupTime)  (THIS) PURE;
    STDMETHOD(SetSessionSetupTime)           (THIS_
                                             UINT32 ulSessionSetupTime) PURE;

    STDMETHOD_(UINT32, GetFirstPacketTime)   (THIS) PURE;
    STDMETHOD(SetFirstPacketTime)            (THIS_
                                             UINT32 ulFirstPacketTime) PURE;

    STDMETHOD_(UINT32, GetPreDataTime)       (THIS) PURE;
    STDMETHOD(SetPreDataTime)                (THIS_
                                             UINT32 ulPreDataTime) PURE;

    STDMETHOD_(UINT32, GetPreDataBytes)      (THIS) PURE;
    STDMETHOD(SetPreDataBytes)               (THIS_
                                             UINT32 ulPreDataBytes) PURE;

    STDMETHOD_(UINT32, GetPrerollInMsec)     (THIS) PURE;
    STDMETHOD(SetPrerollInMsec)              (THIS_
                                             UINT32 ulPrerollInMsec) PURE;

    STDMETHOD(DumpStartupInfo)               (THIS) PURE;
};

///////////////////////////////////////////////////////////////////////////////
// Interface: 
//
//      IHXClientStats
//
// Purpose:
//
//      Allows access to client statistics. 
//
// IID_IHXClientStats:
//
//      {83CE47E8-3EBE-450a-BF49-66D88094E516}
//
///////////////////////////////////////////////////////////////////////////////

// {83CE47E8-3EBE-450a-BF49-66D88094E516}
DEFINE_GUID(IID_IHXClientStats, 0x83ce47e8, 0x3ebe, 0x450a, 
            0xbf, 0x49, 0x66, 0xd8, 0x80, 0x94, 0xe5, 0x16);

#define CLSID_IHXClientStats     IID_IHXClientStats

#undef  INTERFACE
#define INTERFACE   IHXClientStats

DECLARE_INTERFACE_(IHXClientStats, IUnknown)
{

    // IUnknown methods

    STDMETHOD(QueryInterface)               (THIS_
                                            REFIID riid,
                                            void** ppvObj) PURE;

    STDMETHOD_(UINT32,AddRef)               (THIS) PURE;
 
    STDMETHOD_(UINT32,Release)              (THIS) PURE;


    // IHXClientStats methods

    STDMETHOD_(IHXClientStatsManager*, GetStatsManager) (THIS) PURE;
    STDMETHOD(SetStatsManager) (THIS_ IHXClientStatsManager* pStatsMgr) PURE;

    ///////////////////////////////////////////////////////////////////////////
    // Statistics accessor/mutator methods.
    //
    // Purpose:
    //      All used to either acquire a property value or set it.
    //
    // Arguments:
    //      Mutators take an IN parameter, the new value to set the property
    //      to. Accessors take nothing.
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD_(IHXBuffer*, GetIPAddress)    (THIS) PURE;
    STDMETHOD(SetIPAddress)                 (THIS_
                                            IHXBuffer* pIPAddress) PURE;

    STDMETHOD_(IHXBuffer*, GetCBID)         (THIS) PURE;
    STDMETHOD(SetCBID)                      (THIS_
                                            IHXBuffer* pCBID) PURE;

    STDMETHOD_(IHXBuffer*, GetGUID)         (THIS) PURE;
    STDMETHOD(SetGUID)                      (THIS_
                                            IHXBuffer* pGUID) PURE;

    STDMETHOD_(IHXBuffer*, GetClientID)     (THIS) PURE;
    STDMETHOD(SetClientID)                  (THIS_
                                            IHXBuffer* pClientID) PURE;

    STDMETHOD_(IHXBuffer*, GetPNAClientID)  (THIS) PURE;
    STDMETHOD(SetPNAClientID)               (THIS_
                                             IHXBuffer* pPNAClientID) PURE;

    STDMETHOD_(IHXBuffer*, GetCompanyID)     (THIS) PURE;
    STDMETHOD(SetCompanyID)                  (THIS_
                                             IHXBuffer* pCompanyID) PURE;

    STDMETHOD_(IHXBuffer*, GetClientChallenge)     (THIS) PURE;
    STDMETHOD(SetClientChallenge)                  (THIS_
                                                   IHXBuffer* pClientChallenge) PURE;

    STDMETHOD_(IHXBuffer*, GetLanguage)     (THIS) PURE;
    STDMETHOD(SetLanguage)                  (THIS_
                                             IHXBuffer* pLanguage) PURE;

    STDMETHOD_(IHXBuffer*, GetPlayerStartTime) (THIS) PURE;
    STDMETHOD(SetPlayerStartTime)              (THIS_
                                               IHXBuffer* pPlayerStartTime) PURE;

    STDMETHOD_(IHXBuffer*, GetProtocol)     (THIS) PURE;
    STDMETHOD(SetProtocol)                  (THIS_
                                            IHXBuffer* pProtocol) PURE;

    STDMETHOD_(IHXBuffer*, GetStartTime)    (THIS) PURE;
    STDMETHOD(SetStartTime)                 (IHXBuffer* pStartTime) PURE;

    STDMETHOD_(IHXBuffer*, GetRequestMethod)    (THIS) PURE;
    STDMETHOD(SetRequestMethod)                 (IHXBuffer* pRequestMethod) PURE;

    STDMETHOD_(IHXBuffer*, GetUserAgent)    (THIS) PURE;
    STDMETHOD(SetUserAgent)                 (THIS_ 
                                            IHXBuffer* pUserAgent) PURE;

    STDMETHOD_(IHXBuffer*, GetVersion)      (THIS) PURE;
    STDMETHOD(SetVersion)                   (THIS_
                                            IHXBuffer* pVersion) PURE;

    STDMETHOD_(IHXBuffer*, GetLoadTestPassword)     (THIS) PURE;
    STDMETHOD(SetLoadTestPassword)                  (THIS_
                                                    IHXBuffer* pLoadTestPassword) PURE;

    STDMETHOD_(IHXBuffer*, GetRTSPEvents)   (THIS) PURE;
    STDMETHOD(SetRTSPEvents)                (THIS_
                                            IHXBuffer* pRTSPEvents) PURE;


    STDMETHOD_(UINT64, GetControlBytesSent) (THIS) PURE;
    STDMETHOD(SetControlBytesSent)          (THIS_
                                            UINT64 ulControlBytesSent) PURE;

    STDMETHOD_(UINT32, GetPort)             (THIS) PURE;
    STDMETHOD(SetPort)                      (THIS_
                                            UINT32 ulPort) PURE;

    STDMETHOD_(UINT32, GetSessionCount)     (THIS) PURE;
    STDMETHOD(SetSessionCount)              (THIS_
                                            UINT32 ulSessionCount) PURE;

    STDMETHOD_(UINT32, GetSessionIndex)     (THIS) PURE;
    STDMETHOD(SetSessionIndex)              (THIS_
                                            UINT32 ulSessionIndex) PURE;

    STDMETHOD_(HXBOOL, IsCloaked)             (THIS) PURE;
    STDMETHOD(SetCloaked)                   (THIS_ 
                                            HXBOOL bIsCloaked) PURE;

    STDMETHOD_(HXBOOL, IsRDT)                 (THIS) PURE;
    STDMETHOD(SetRDT)                       (THIS_
                                            HXBOOL bIsRDT) PURE;

    STDMETHOD_(HXBOOL, SupportsMaximumASMBandwidth)  (THIS) PURE;
    STDMETHOD(SetSupportsMaximumASMBandwidth)      (THIS_
                                                    HXBOOL bSupportsMaximumASMBandwidth) PURE;

    STDMETHOD_(HXBOOL, SupportsMulticast)     (THIS) PURE;
    STDMETHOD(SetSupportsMulticast)         (THIS_
                                            HXBOOL bSupportsMulticast) PURE;

    STDMETHOD_(IHXBuffer*, GetStreamSelectionInfo)  (THIS) PURE;
    STDMETHOD(SetStreamSelectionInfo)               (THIS_ 
                                                    IHXBuffer* pStreamSelectionInfo) PURE;

    STDMETHOD_(UINT32, GetTotalMediaAdaptations)    (THIS) PURE;
    STDMETHOD(SetTotalMediaAdaptations)             (THIS_ 
                                                    UINT32 ulTotalMediaAdaptations) PURE;



    ///////////////////////////////////////////////////////////////////////////
    // Method: 
    //      IHXClientStats::GetID()
    // Purpose:
    //      Get the id (assigned by the client stats mgr) for this client.
    // Arguments:
    //      void
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD_(UINT32, GetID)               (THIS) PURE;

    
    ///////////////////////////////////////////////////////////////////////////
    // Method: 
    //      IHXClientStats::SetID()
    // Purpose:
    //      Set the connection id for this client. To be called by the client
    //      stats manager.
    // Arguments:
    //      ulConnId - conn id of client.
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD(SetID)                        (THIS_
                                            UINT32 ulConnId) PURE;


    ///////////////////////////////////////////////////////////////////////////
    // Method: 
    //      IHXClientStats::GetSession()
    // Purpose:
    //      Get the IHXSessionStats object for the provided session number.
    // Arguments:
    //      ulSessionId - Session number of session to get.
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD_(IHXSessionStats*, GetSession) (THIS_
                                             UINT32 ulSessionNumber) PURE;

    ///////////////////////////////////////////////////////////////////////////
    // Method: 
    //      IHXClientStats::RemoveSession()
    // Purpose:
    //      Remove the IHXSessionStats object associated with the provided 
    //      session number.
    // Arguments:
    //      ulSessionId - Session number of session to remove.
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD(RemoveSession)                 (THIS_
                                             UINT32 ulSessionNumber) PURE;

    ///////////////////////////////////////////////////////////////////////////
    // Method: 
    //      IHXClientStats::AddSession()
    // Purpose:
    //      Add an IHXSessionStats object to list.
    // Arguments:
    //      pSession    - Pointer to IHXSessionStats object.
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD (AddSession)                   (THIS_
                                             IHXSessionStats* pSession) PURE;


};


///////////////////////////////////////////////////////////////////////////////
// Interface: 
//
//      IHXClientStats2
//
// Purpose:
//      New interface adds Get/SetReasonForTermination methods. Inherits
//      IHXClientStats interface.
//
// IID_IHXClientStats:
//
//      {5DAD23DF-A442-4fe9-89CD-39B13DA5CA51}
//
///////////////////////////////////////////////////////////////////////////////

// {5DAD23DF-A442-4fe9-89CD-39B13DA5CA51}
DEFINE_GUID(IID_IHXClientStats2, 0x5dad23df, 0xa442, 0x4fe9,
            0x89, 0xcd, 0x39, 0xb1, 0x3d, 0xa5, 0xca, 0x51);

#define CLSID_IHXClientStats2     IID_IHXClientStats2

#undef  INTERFACE
#define INTERFACE   IHXClientStats2

DECLARE_INTERFACE_(IHXClientStats2, IHXClientStats)
{
    // IHXClientStats2 methods

    ///////////////////////////////////////////////////////////////////////////
    // Statistics accessor/mutator methods.
    //
    // Purpose:
    //      All used to either acquire a property value or set it.
    //
    // Arguments:
    //      Mutators take an IN parameter, the new value to set the property
    //      to. Accessors take nothing.
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD_(UINT32, GetReasonForTermination)  (THIS) PURE;
    STDMETHOD(SetReasonForTermination)           (THIS_
                                                 UINT32 ulReasonForTermination) PURE;
};

///////////////////////////////////////////////////////////////////////////////
// Interface: 
//
//      IHXCheckRetainEntityForSetup
//
// Purpose:
//      New interface adds Get/SetUpdateRegistryForLive methods. Inherits
//      IUnknown interface.
//
// IID_IHXCheckRetainEntityForSetup:
//
//      {7EC159C4-D9ED-4e58-B895-6CBA38D592F4}
//
///////////////////////////////////////////////////////////////////////////////

// {7EC159C4-D9ED-4e58-B895-6CBA38D592F4}

DEFINE_GUID(IID_IHXCheckRetainEntityForSetup, 
        0x7ec159c4, 0xd9ed, 0x4e58, 0xb8, 0x95, 0x6c, 0xba, 0x38, 0xd5, 0x92, 0xf4);

#define CLSID_IHXCheckRetainEntityForSetup     IID_IHXCheckRetainEntityForSetup

#undef  INTERFACE
#define INTERFACE   IHXCheckRetainEntityForSetup

DECLARE_INTERFACE_(IHXCheckRetainEntityForSetup, IUnknown)
{
    // IHXCheckRetainEntityForSetup methods

    ///////////////////////////////////////////////////////////////////////////
    // Statistics accessor/mutator methods.
    //
    // Purpose:
    //      All used to either acquire a property value or set it.
    //
    // Arguments:
    //      Mutators take an IN parameter, the new value to set the property
    //      to. Accessors take nothing.
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD_(HXBOOL, GetUpdateRegistryForLive) (THIS) PURE;
    STDMETHOD(SetUpdateRegistryForLive)          (THIS) PURE;
};


///////////////////////////////////////////////////////////////////////////////
// Interface: 
//
//      IHXClientStatsSink
//
// Purpose:
//
//      Applications wishing to receive information regarding stats from the
//      client stats manager implement this interface.
//
// IID_IHXClientStatsSink:
//
//      {FDC6D1AA-D78E-40b0-A3E6-B7D6DF30383B}
//
///////////////////////////////////////////////////////////////////////////////


// {FDC6D1AA-D78E-40b0-A3E6-B7D6DF30383B}
DEFINE_GUID(IID_IHXClientStatsSink, 0xfdc6d1aa, 0xd78e, 0x40b0, 
            0xa3, 0xe6, 0xb7, 0xd6, 0xdf, 0x30, 0x38, 0x3b);


#define CLSID_IHXClientStatsSink     IID_IHXClientStatsSink

#undef  INTERFACE
#define INTERFACE   IHXClientStatsSink

DECLARE_INTERFACE_(IHXClientStatsSink, IUnknown)
{

    // IUnknown methods

    STDMETHOD(QueryInterface)               (THIS_
                                            REFIID riid,
                                            void** ppvObj) PURE;

    STDMETHOD_(UINT32,AddRef)               (THIS) PURE;

    STDMETHOD_(UINT32,Release)              (THIS) PURE;


    // IHXClientStatsSink methods

    ///////////////////////////////////////////////////////////////////////////
    // Method: 
    //      IHXClientStatsSink::OnStatsEvent()
    // Purpose:
    //      Notify sinks of a Client/Session stats event
    // Arguments:
    //      nEvent      the id of the triggering event 
    //      pClient     IHXStats object 
    //      pSession    IHX stats object (== NULL for client connect/disconnect, 
    //                  and timer events)
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD(OnStatsEvent)             (THIS_
                                        ClientStatsEvent nEvent,
                                        IHXClientStats* pClient,
                                        IHXSessionStats* pSession) PURE;

    ///////////////////////////////////////////////////////////////////////////
    // Method: 
    //      IHXClientStatsSink::GetStatsTimerInterval()
    // Purpose:
    //      Retrieve the desired timer interval from the sink for scheduling
    //      callbacks with connecting clients.
    // Arguments:
    //      pClient - Stats object. 
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD_(UINT32, GetStatsTimerInterval)    (THIS) PURE;

};


///////////////////////////////////////////////////////////////////////////////
// Interface: 
//
//      IHXClientStatsManager
//
// Purpose:
//      
//      Allows plugins to access the server's client stats management system
//      via the IHXContext.      
//
// IID_IHXClientStatsManager:
//
//      {E0ECE2B8-A94B-4eb4-AF0D-177D053B2ED6}
//
///////////////////////////////////////////////////////////////////////////////


DEFINE_GUID(IID_IHXClientStatsManager, 0xe0ece2b8, 0xa94b, 0x4eb4, 
            0xaf, 0xd, 0x17, 0x7d, 0x5, 0x3b, 0x2e, 0xd6);

#define CLSID_IHXClientStatsManager     IID_IHXClientStatsManager

#undef  INTERFACE
#define INTERFACE   IHXClientStatsManager

DECLARE_INTERFACE_(IHXClientStatsManager, IUnknown)
{

    // IUnknown methods

    STDMETHOD(QueryInterface)               (THIS_
                                            REFIID riid,
                                            void** ppvObj) PURE;

    STDMETHOD_(UINT32,AddRef)               (THIS) PURE;

    STDMETHOD_(UINT32,Release)              (THIS) PURE;


    // IHXClientStatsManager methods


    ///////////////////////////////////////////////////////////////////////////
    // Method: 
    //      IHXClientStatsManager::RegisterSink()
    // Purpose:
    //      Registers an IHXClientStatsSink object with the manager.
    //      The sink is then able to receive notifications.
    // Arguments:
    //      pSink - IN - IHXClientStatsSink object to register.   
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD(RegisterSink)                 (THIS_
                                            IHXClientStatsSink* pSink) PURE;

    ///////////////////////////////////////////////////////////////////////////
    // Method: 
    //      IHXClientStatsManager::RemoveSink()
    // Purpose:
    //      Removes an IHXClientStatsSink object from the manager.
    // Arguments:
    //      pSink - IN - IHXClientStatsSink object to remove.   
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD(RemoveSink)                 (THIS_
                                          IHXClientStatsSink* pSink) PURE;

    ///////////////////////////////////////////////////////////////////////////
    // Method: 
    //      IHXClientStatsManager::GetClient()
    // Purpose:
    //      Returns an IHXClientStats object, given a client ID.
    // Arguments:
    //      ulClientId - IN - Conn id of client. 
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD_(IHXClientStats*, GetClient)  (THIS_
                                            UINT32 ulClientId) PURE;   

    ///////////////////////////////////////////////////////////////////////////
    // Method: 
    //      IHXClientStatsManager::AddClient()
    // Purpose:
    //      Adds an IHXClientStats object to the manager list. To be used
    //      when clients connect.
    // Arguments:
    //      ulClientId - IN - Conn id of client. 
    //      pClient    - IN - Pointer to clientstats object to add.
    //      
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD(AddClient)                 (THIS_ 
                                         IHXClientStats* pClient) PURE;

    ///////////////////////////////////////////////////////////////////////////
    // Method: 
    //      IHXClientStatsManager::RemoveClient()
    // Purpose:
    //      Removes an IHXClientStats object from the manager list. To be used
    //      when clients disconnect.
    // Arguments:
    //      ulClientId - Conn id of client. 
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD(RemoveClient)                 (THIS_ 
                                            UINT32 ulClientId) PURE;


    ///////////////////////////////////////////////////////////////////////////
    // Method: 
    //      IHXClientStatsManager::GetClientCount()
    // Purpose:
    //      Calls a sink back, passing info about all IHXClientStats objects 
    //      that the manager currently knows about.
    // Arguments:
    //      pSink - IN - IHXClientStatsSink object to callback. 
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD_(UINT32, GetClientCount)       (THIS) PURE;


    ///////////////////////////////////////////////////////////////////////////
    // Method: 
    //      IHXClientStatsManager::UseRegistryForStats()
    // Purpose:
    //      Returns TRUE if the server is configured to use the registry for
    //      client stats.
    // Arguments:
    //      void
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD_(HXBOOL, UseRegistryForStats)  (THIS) PURE;

    ///////////////////////////////////////////////////////////////////////////
    // Method: 
    //      IHXClientStatsManager::ScheduleSinkNotifications()
    // Purpose:
    //      Allows clients of the StatsMgr to notify stat sinks of important
    //      state changes
    // Arguments:
    //      ulClientId - IN - Conn id of client. 
    //      pClient    - IN - Pointer to clientstats object to add.
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD(ScheduleSinkNotifications)(THIS_
                                     IHXClientStats* pClient, 
                                     IHXSessionStats* pSession,
                                     ClientStatsEvent nEvent) PURE;

};


#endif

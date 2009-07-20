/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxqosinfo.h,v 1.9 2007/05/01 18:18:24 darrick Exp $
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
// hxqosinfo.h - Interfaces for QoS Adaptation Info.
///////////////////////////////////////////////////////////////////////////////

#ifndef HXQOSINFO_H
#define HXQOSINFO_H

///////////////////////////////////////////////////////////////////////////////
// RateSelInfoId
//
// Rate selection info.
//
// Info can be [agg]regate across all streams, per [strm]. or both.
///////////////////////////////////////////////////////////////////////////////

enum RateSelInfoId
{
  RSI_NONE,
  RSI_QUERYPARAM_IR,    // "ir=" from query param.[agg]
  RSI_BANDWIDTH,        // "Bandwidth" [agg]
  RSI_SDB,              // "SetDeliveryBandwidth" [agg]
  RSI_LINKCHAR_MBW,     // 3GPP-Link-Char Max Bandwidth [agg/strm]
  RSI_LINKCHAR_GBW,     // 3GPP-Link-Char Guaranteed Bandwidth [agg/strm]
  RSI_LINKCHAR_MTD,     // 3GPP-Link-Char Max Transfer Delay [agg/strm]
  RSI_DEFAULT_RULE,     // Default ASM rule for stream. [strm]
  RSI_AVGBITRATE,       // Average bitrate for stream. [strm]
  RSI_TRACKID,          // TrackID [strm]
  RSI_ISDEFAULTSTREAM,  // 1 if stream is default stream for 3gp multirate file,
                        // otherwise 0. [strm]
  RSI_STREAMGROUPID,    // Streamgroup id [strm]
  RSI_SUBSCRIBE_RULE,   // Verb: Subscribe to ASM rule [strm]
  RSI_UNSUBSCRIBE_RULE, // Verb: Unsubscribe to ASM rule [strm]
  RSI_MAX
};

///////////////////////////////////////////////////////////////////////////////

_INTERFACE      IUnknown;
_INTERFACE      IHXQoSTransportAdaptationInfo;
_INTERFACE      IHXQoSSessionAdaptationInfo;
_INTERFACE      IHXQoSApplicationAdaptationInfo;


///////////////////////////////////////////////////////////////////////////////
// Interface: 
//
//      IHXQoSTransportAdaptationInfo
//
// Purpose:
//
//      Provides adaptation info for QoS transport layer.
//
// IID_IHXQoSTransportAdaptationInfo:
//
//      {213645C5-3A56-4945-A8A9-6702BB5604B6}
//
///////////////////////////////////////////////////////////////////////////////

// {213645C5-3A56-4945-A8A9-6702BB5604B6}
DEFINE_GUID(IID_IHXQoSTransportAdaptationInfo, 
            0x213645c5, 0x3a56, 0x4945, 0xa8, 0xa9, 0x67, 0x2, 0xbb, 0x56, 0x4, 0xb6);

#define CLSID_IHXQoSTransportAdaptationInfo     IID_IHXQoSTransportAdaptationInfo

#undef  INTERFACE
#define INTERFACE   IHXQoSTransportAdaptationInfo

DECLARE_INTERFACE_(IHXQoSTransportAdaptationInfo, IUnknown)
{
    STDMETHOD_(UINT32, GetRRFrequency) (THIS) PURE;
    STDMETHOD(SetRRFrequency) (THIS_
                               UINT32 ulRRFrequency) PURE;

    STDMETHOD_(UINT32, GetRTT) (THIS) PURE;
    STDMETHOD(SetRTT) (THIS_
                       UINT32 ulRTT) PURE;

    STDMETHOD_(UINT32, GetPacketLoss) (THIS) PURE;
    STDMETHOD(SetPacketLoss) (THIS_
                              UINT32 ulPacketLoss) PURE;

    STDMETHOD_(UINT32, GetReceivedThroughput) (THIS) PURE;
    STDMETHOD(SetReceivedThroughput) (THIS_
                                      UINT32 ulReceivedThroughput) PURE;

    STDMETHOD_(UINT32, GetSuccessfulResends)    (THIS) PURE;
    STDMETHOD(SetSuccessfulResends)             (THIS_
                                                UINT32 ulSuccessfulResends) PURE;

    STDMETHOD_(UINT32, GetFailedResends)    (THIS) PURE;
    STDMETHOD(SetFailedResends)             (THIS_
                                            UINT32 ulFailedResends) PURE;

    STDMETHOD_(IHXBuffer*, GetTxRateRange)    (THIS) PURE;
    STDMETHOD(SetTxRateRange)                 (THIS_
                                              IHXBuffer* pTxRateRange) PURE;

    STDMETHOD_(UINT32, GetPacketsSent)      (THIS) PURE;
    STDMETHOD(SetPacketsSent)               (THIS_
                                            UINT32 ulPacketsSent) PURE;

    STDMETHOD_(UINT64, GetBytesSent)        (THIS) PURE;
    STDMETHOD(SetBytesSent)                 (THIS_
                                            UINT64 ulBytesSent) PURE;

};


///////////////////////////////////////////////////////////////////////////////
// Interface: 
//
//      IHXQoSSessionAdaptationInfo
//
// Purpose:
//
//      Provides adaptation info for QoS Session layer.
//
// IID_IHXQoSSessionAdaptationInfo:
//
//      {AED09295-0A71-4520-9D7F-BFA9B5A97245}
//
///////////////////////////////////////////////////////////////////////////////

// {AED09295-0A71-4520-9D7F-BFA9B5A97245}
DEFINE_GUID(IID_IHXQoSSessionAdaptationInfo, 
            0xaed09295, 0xa71, 0x4520, 0x9d, 0x7f, 0xbf, 0xa9, 0xb5, 0xa9, 0x72, 0x45);

#define CLSID_IHXQoSSessionAdaptationInfo     IID_IHXQoSSessionAdaptationInfo

#undef  INTERFACE
#define INTERFACE   IHXQoSSessionAdaptationInfo

DECLARE_INTERFACE_(IHXQoSSessionAdaptationInfo, IUnknown)
{
    STDMETHOD_(UINT32, GetEstimatedPlayerBufferUnderruns) (THIS) PURE;
    STDMETHOD(SetEstimatedPlayerBufferUnderruns) (THIS_
                                                  UINT32 ulEstimatedPlayerBufferUnderruns) PURE;

    STDMETHOD_(UINT32, GetEstimatedPlayerBufferOverruns) (THIS) PURE;
    STDMETHOD(SetEstimatedPlayerBufferOverruns) (THIS_
                                                 UINT32 ulEstimatedPlayerBufferOverruns) PURE;

    STDMETHOD_(UINT32, GetBufferDepthTime) (THIS) PURE;
    STDMETHOD(SetBufferDepthTime) (THIS_
                                   UINT32 ulBufferDepthTime) PURE;

    STDMETHOD_(UINT32, GetBufferDepthBytes) (THIS) PURE;
    STDMETHOD(SetBufferDepthBytes) (THIS_
                                    UINT32 ulBufferDepthBytes) PURE;
};


///////////////////////////////////////////////////////////////////////////////
// Interface: 
//
//      IHXQoSApplicationAdaptationInfo
//
// Purpose:
//
//      Provides adaptation info for QoS Application layer.
//
// IID_IHXQoSApplicationAdaptationInfo:
//
//      {207E23E5-F71F-4a18-B7D0-F4F865A2058B}
//
///////////////////////////////////////////////////////////////////////////////

// {207E23E5-F71F-4a18-B7D0-F4F865A2058B}
DEFINE_GUID(IID_IHXQoSApplicationAdaptationInfo, 
            0x207e23e5, 0xf71f, 0x4a18, 0xb7, 0xd0, 0xf4, 0xf8, 0x65, 0xa2, 0x5, 0x8b);

#define CLSID_IHXQoSApplicationAdaptationInfo     IID_IHXQoSApplicationAdaptationInfo

#undef  INTERFACE
#define INTERFACE   IHXQoSApplicationAdaptationInfo

DECLARE_INTERFACE_(IHXQoSApplicationAdaptationInfo, IUnknown)
{
    STDMETHOD_(UINT32, GetTotalBitrateAdaptations) (THIS) PURE;
    STDMETHOD(SetTotalBitrateAdaptations) (THIS_
                                           UINT32 ulTotalBitrateAdaptations) PURE;

    STDMETHOD_(UINT32, GetCurrentBitrate) (THIS) PURE;
    STDMETHOD(SetCurrentBitrate) (THIS_
                                  UINT32 ulCurrentBitrate) PURE;

    STDMETHOD_(UINT32, GetTotalUpshifts)    (THIS) PURE;
    STDMETHOD(SetTotalUpshifts)             (THIS_
                                            UINT32 ulTotalUpshifts) PURE;

    STDMETHOD_(UINT32, GetTotalDownshifts)  (THIS) PURE;
    STDMETHOD(SetTotalDownshifts)           (THIS_
                                            UINT32 ulTotalDownshifts) PURE;

    STDMETHOD_(UINT32, GetASMSubscribes)    (THIS) PURE;
    STDMETHOD(SetASMSubscribes)             (THIS_
                                            UINT32 ulASMSubscribes) PURE;

    STDMETHOD_(UINT32, GetASMUnsubscribes)  (THIS) PURE;
    STDMETHOD(SetASMUnsubscribes)           (THIS_
                                            UINT32 ulASMUnsubscribes) PURE;

};


///////////////////////////////////////////////////////////////////////////////
// Interface: 
//
//      IHXRateSelectionInfo
//
// Purpose:
//
//      Provides rate selection info for media session.
//
// IID_IHXRateSelectionInfo:
//
//      {207E23E5-F71F-4a18-B7D0-F4F865A2058B}
//
///////////////////////////////////////////////////////////////////////////////

// {FFACB639-AECF-4111-A1FB-92F0859D4F0F}
DEFINE_GUID(IID_IHXRateSelectionInfo, 
            0xffacb639, 0xaecf, 0x4111, 0xa1, 0xfb, 0x92, 0xf0, 0x85, 0x9d, 0x4f, 0xf);

#define CLSID_IHXRateSelectionInfo IID_IHXRateSelectionInfo

#undef  INTERFACE
#define INTERFACE   IHXRateSelectionInfo

DECLARE_INTERFACE_(IHXRateSelectionInfo, IUnknown)
{
    // IHXRateSelectionInfo 

    STDMETHOD(SetInfo)          (THIS_ 
                                 RateSelInfoId Id, 
                                 UINT32 ulInfo) PURE;

    STDMETHOD(SetInfo)          (THIS_ 
                                 RateSelInfoId Id, 
                                 UINT16 ulLogicalStreamId, 
                                 UINT32 ulInfo) PURE;

    STDMETHOD(GetInfo)          (THIS_ 
                                 RateSelInfoId Id, 
                                 UINT32& ulInfo) PURE;

    STDMETHOD(GetInfo)          (THIS_ 
                                 RateSelInfoId Id, 
                                 UINT16 ulLogicalStreamId, 
                                 UINT32& ulInfo) PURE;

    STDMETHOD_(UINT16, GetNumSubscribedRules) (THIS_ 
                                               UINT16 ulLogicalStreamId) PURE;

    STDMETHOD(GetSubscribedRules)  (THIS_ 
                                    UINT16 ulLogicalStreamId, 
                                    UINT16 ulArraySize, 
                                    UINT16* Rules) PURE;

    STDMETHOD_(UINT16, GetNumRegisteredStreams) (THIS) PURE;

    STDMETHOD(GetRegisteredLogicalStreamIds)  (THIS_ 
                                               UINT16 ulArraySize, 
                                               UINT16* StreamIds) PURE;
};


// Define smart pointers
#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXQoSTransportAdaptationInfo)
DEFINE_SMART_PTR(IHXQoSSessionAdaptationInfo)
DEFINE_SMART_PTR(IHXQoSApplicationAdaptationInfo)
DEFINE_SMART_PTR(IHXRateSelectionInfo)

#endif //defined HXQOSINFO_H

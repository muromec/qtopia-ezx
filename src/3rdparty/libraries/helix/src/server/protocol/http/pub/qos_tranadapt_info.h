/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: 
 *
 * Portions Copyright (c) 1995-2005 RealNetworks, Inc. All Rights Reserved.
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

#ifndef _QOSTAINFO_H_
#define _QOSTAINFO_H_

#include "ihxpckts.h"
#include "hxqosinfo.h"

class HTTPQoSTranAdaptInfo : public IHXQoSTransportAdaptationInfo
{
public:
    HTTPQoSTranAdaptInfo(void);
    virtual ~HTTPQoSTranAdaptInfo(void);

    // IHXUnknown methods
    STDMETHOD(QueryInterface)   (REFIID riid, void** ppvObj);
    STDMETHOD_(UINT32,AddRef)   (THIS);
    STDMETHOD_(UINT32,Release)  (THIS);

    // IHXQoSTransportAdaptationInfo methods
    STDMETHOD_(UINT32, GetRRFrequency) (THIS);
    STDMETHOD(SetRRFrequency) (THIS_
                               UINT32 ulRRFrequency);

    STDMETHOD_(UINT32, GetRTT) (THIS);
    STDMETHOD(SetRTT) (THIS_
                       UINT32 ulRTT);

    STDMETHOD_(UINT32, GetPacketLoss) (THIS);
    STDMETHOD(SetPacketLoss) (THIS_
                              UINT32 ulPacketLoss);

    STDMETHOD_(UINT32, GetReceivedThroughput) (THIS);
    STDMETHOD(SetReceivedThroughput) (THIS_
                                      UINT32 ulReceivedThroughput);

    STDMETHOD_(UINT32, GetSuccessfulResends)    (THIS);
    STDMETHOD(SetSuccessfulResends)             (THIS_
                                                UINT32 ulSuccessfulResends);

    STDMETHOD_(UINT32, GetFailedResends)    (THIS);
    STDMETHOD(SetFailedResends)             (THIS_
                                            UINT32 ulFailedResends);

    STDMETHOD_(IHXBuffer*, GetTxRateRange)    (THIS);
    STDMETHOD(SetTxRateRange)                 (THIS_
                                              IHXBuffer* pTxRateRange);

    STDMETHOD_(UINT32, GetPacketsSent)      (THIS);
    STDMETHOD(SetPacketsSent)               (THIS_
                                            UINT32 ulPacketsSent);

    STDMETHOD_(UINT64, GetBytesSent)        (THIS);
    STDMETHOD(SetBytesSent)                 (THIS_
                                            UINT64 ulBytesSent);
private:
    UINT32                        m_nRefCount;
    UINT32                        m_ulRRFrequency;
    UINT32                        m_ulRTT;
    UINT32                        m_ulPacketLoss;
    UINT32                        m_ulReceivedThroughput;
    UINT32                        m_ulSuccessfulResends;
    UINT32                        m_ulFailedResends;
    IHXBuffer*                    m_pTxRateRange;
    UINT64                        m_ulBytesSent;
    UINT32                        m_ulPacketsSent;
};

#endif  /* _QOSTAINFO_H_ */

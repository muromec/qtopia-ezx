/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rateselinfo.h,v 1.3 2007/05/23 00:43:01 darrick Exp $
 *
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.
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

#ifndef _RATE_SEL_INFO_H_
#define _RATE_SEL_INFO_H_

#include "hxmap.h"
#include "hxslist.h"
#include "hxqosinfo.h"

class StreamRateSelectionInfo
{
public:

    StreamRateSelectionInfo();
    ~StreamRateSelectionInfo();

    HX_RESULT GetInfo(RateSelInfoId Id, UINT32& ulInfo);
    HX_RESULT SetInfo(RateSelInfoId Id, UINT32 ulInfo);

    UINT16 GetNumSubscribedRules();
    HX_RESULT GetSubscribedRules(UINT16 ulNumRules, 
                                 UINT16* Rules);

    void Dump();

protected:
    BOOL            m_bGotTrackId;
    CHXSimpleList   m_RuleSubs;
    UINT32          m_Info[RSI_MAX];
};


class RateSelectionInfo : public IHXRateSelectionInfo
{
public:

    RateSelectionInfo();
    virtual ~RateSelectionInfo();

    // IUnknown 

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);


    // IHXRateSelectionInfo 

    STDMETHOD(SetInfo)          (THIS_ 
                                 RateSelInfoId Id, 
                                 UINT32 ulInfo);

    STDMETHOD(SetInfo)          (THIS_ 
                                 RateSelInfoId Id, 
                                 UINT16 ulLogicalStreamId, 
                                 UINT32 ulInfo);

    STDMETHOD(GetInfo)          (THIS_ 
                                 RateSelInfoId Id, 
                                 UINT32& ulInfo);

    STDMETHOD(GetInfo)          (THIS_ 
                                 RateSelInfoId Id, 
                                 UINT16 ulLogicalStreamId, 
                                 UINT32& ulInfo);

    STDMETHOD_(UINT16, GetNumSubscribedRules) 
                                (THIS_ UINT16 ulLogicalStreamId);

    STDMETHOD(GetSubscribedRules)   (THIS_ 
                                     UINT16 ulLogicalStreamId, 
                                     UINT16 ulNumRules, 
                                     UINT16* Rules);

    STDMETHOD_(UINT16, GetNumRegisteredStreams) (THIS);

    STDMETHOD(GetRegisteredLogicalStreamIds) 
                                (THIS_
                                 UINT16 ulArraySize,
                                 UINT16* StreamIds);

    void Dump();


protected:

    CHXMapLongToObj m_StreamMap;
    UINT32 m_ulRefCount;
    UINT32 m_Info[RSI_MAX];
};


#endif //defined _RATE_SEL_INFO_H_

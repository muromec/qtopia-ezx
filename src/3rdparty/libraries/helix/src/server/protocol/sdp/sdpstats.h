/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: sdpstats.h,v 1.5 2004/04/19 19:09:10 darrick Exp $ 
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

#ifndef _SDPSTATS_H
#define _SDPSTATS_H

#include "ihxlist.h"
#include "hxassert.h"
#include "hxsdp.h"

#if defined _WINDOWS && !defined snprintf
#define snprintf _snprintf
#endif

///////////////////////////////////////////////////////////////////////////////
// SDPAggregateStats - Instantiated one per server.
///////////////////////////////////////////////////////////////////////////////

class SDPAggregateStats : public IHXSDPAggregateStats
{

public:

    SDPAggregateStats();
    virtual ~SDPAggregateStats();

    enum StatsCounterType
    {
        TOTAL_DOWNLOAD_COUNT,
        SDP_RTSP_DOWNLOAD_COUNT,
        SDP_HTTP_DOWNLOAD_COUNT,
        SDP_SESSION_INIT_SUCCESS_COUNT,
        SDP_SESSION_INIT_ERROR_COUNT,
        NUM_COUNTERS
    };

    enum ProtocolType
    {
        UNKNOWN_PROT,
        PNA_PROT,
        RTSP_PROT,
        HTTP_PROT,
        MMS_PROT,
        NUM_PROT
    };

    // IUnknown methods.

    STDMETHODIMP QueryInterface(REFIID ID, void** ppInterfaceObj);     
    STDMETHODIMP_(UINT32) AddRef();     
    STDMETHODIMP_(UINT32) Release();

    // IHXSDPAggregateStats methods.

    STDMETHODIMP IncrementSDPDownloadCount    (UINT32 ulProtocolId, 
                                               BOOL bSuccess);
    STDMETHODIMP IncrementSDPSessionInitCount (UINT32 ulProtocolId, 
                                               BOOL bSuccess);

    HX_RESULT Init(IUnknown* pContext);
    HX_RESULT GetLicense(REF(BOOL) bIsLicensed);

    typedef struct _SDPStatsInfo
    {
        StatsCounterType    ulCounterType;
        const char*         szRegPropName;
    } SDPStatsInfo;

    typedef struct _SDPStatsCounter
    {
        UINT32      ulRegId;
        INT32       nCounter;
        BOOL        bOverflow;
    } SDPStatsCounter;
  

protected:

    // Protected Methods
    inline HX_RESULT UpdateCounter(INT32 lCount, StatsCounterType ulCounterId);

    HX_RESULT SetupRegistryEntries();
    HX_RESULT ReadConfig();

    // Static members
    static SDPStatsInfo zm_StatInfoList[NUM_COUNTERS];

    // Instance members
    UINT32 m_ulRefCount;

    IUnknown* m_pContext;
    IHXRegistry2* m_pRegistry;
    IHXErrorMessages* m_pErrorLog;

    BOOL m_bIsEnabled;

    BOOL m_bIsLicensed;
    BOOL m_bIsLicenseInit;

    BOOL m_bIsProxy;

    const char* m_szStatsRegRoot;
    SDPStatsCounter m_StatsCounterVec[NUM_COUNTERS];
};


inline HX_RESULT
SDPAggregateStats::UpdateCounter(INT32 lCount, StatsCounterType ulCounterId)
{
    HX_RESULT rc = HXR_OK;
    SDPStatsCounter* pStatInfo = &(m_StatsCounterVec[ulCounterId]);

    // These checks should occur further up.
    HX_ASSERT(m_bIsLicensed && !m_bIsProxy);

    if (!pStatInfo->ulRegId)
    {
        return rc;
    }

    if (pStatInfo->bOverflow)
    {
        rc = HXR_FAIL;
        return rc;
    }

    INT32 lNewVal = HXAtomicAddRetINT32(&(pStatInfo->nCounter), lCount);

    if (lNewVal < 0)
    { //Overflow

        HXAtomicSubINT32(&(pStatInfo->nCounter), lCount);
        pStatInfo->bOverflow = TRUE;
        char szErrMsg[128];
        snprintf(szErrMsg, 127,
                 "ERROR: INTEGER OVERFLOW in SDP Stats registry entry (%d)\n",
                 pStatInfo->ulRegId);
        szErrMsg[127] = '\0';
        if (m_pErrorLog)
        {
             m_pErrorLog->Report(HXLOG_ERR, 0, 0, szErrMsg, NULL);
        }
        rc = HXR_FAIL;
    }
    else
    {
        m_pRegistry->SetIntById(pStatInfo->ulRegId, pStatInfo->nCounter);
    }

    return rc;
}

#endif  /* ifndef _SDPSTATS_H */

/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: sdpstats.cpp,v 1.5 2004/04/16 23:49:56 darrick Exp $ 
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


#include <stdlib.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxassert.h"
#include "debug.h"
#include "hxstrutl.h"
#include "hxcomm.h"
#include "hxmon.h"
#include "ihxpckts.h"
#include "hxerror.h"
#include "defslice.h"

#include "sdpstats.h"

#if defined _WINDOWS && !defined snprintf
#define snprintf _snprintf
#endif

#define IfFailGo(x) { if (FAILED(x)) { goto cleanup; } }

static const char* z_szConfigAggregateStatsEnabled = "config.SDP Aggregate Statistics.Enabled";
static const char* z_szServerSDPStatsRegRoot = "Server.AggregateSDPStatistics";

const UINT32 MAX_ERROR_BUFF_SIZE = 256;

SDPAggregateStats::SDPStatsInfo
SDPAggregateStats::zm_StatInfoList[NUM_COUNTERS] = 
                        {TOTAL_DOWNLOAD_COUNT,           ".TotalDownloadCount", 
                         SDP_RTSP_DOWNLOAD_COUNT,        ".RTSPDownloadCount",
                         SDP_HTTP_DOWNLOAD_COUNT,        ".HTTPDownloadCount", 
                         SDP_SESSION_INIT_SUCCESS_COUNT, ".StreamSuccessCount",
                         SDP_SESSION_INIT_ERROR_COUNT,   ".StreamErrorCount"};

/*****************************************************************************
 * 
 * SDPAggregateStats Implementation
 *
 *****************************************************************************/

SDPAggregateStats::SDPAggregateStats()
: m_ulRefCount(0)
, m_pContext(NULL)
, m_pRegistry(NULL)
, m_pErrorLog(NULL)
, m_bIsLicensed(FALSE)
, m_bIsLicenseInit(FALSE)
, m_bIsProxy(FALSE)
, m_szStatsRegRoot(NULL)
{
    for (UINT32 i = 0; i < NUM_COUNTERS; ++i)
    {
        SDPStatsCounter* pStatInfo = &(m_StatsCounterVec[i]);
        pStatInfo->bOverflow = FALSE;
        pStatInfo->ulRegId = 0;
        pStatInfo->nCounter = 0;
    }
}

SDPAggregateStats::~SDPAggregateStats()
{
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pErrorLog);
}

STDMETHODIMP
SDPAggregateStats::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXSDPAggregateStats))
    {
        AddRef();
        *ppvObj = (IHXSDPAggregateStats*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
SDPAggregateStats::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(UINT32)
SDPAggregateStats::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0) 
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}

HX_RESULT
SDPAggregateStats::Init(IUnknown* pContext)
{
    HX_RESULT rc = HXR_FAIL;
    BOOL bDummy = FALSE;

    if (m_pContext != NULL)
    {
        rc = HXR_UNEXPECTED;
        return rc;
    }

    HX_ASSERT(pContext);

    m_pContext = pContext;
    m_pContext->AddRef();

    IfFailGo(rc = m_pContext->QueryInterface(IID_IHXRegistry2, (void**)&m_pRegistry));
    IfFailGo(rc = m_pContext->QueryInterface(IID_IHXErrorMessages, (void**)&m_pErrorLog));

    INT32 nProxy;

    if(SUCCEEDED(m_pRegistry->GetIntByName(REGISTRY_RTSPPROXY_ENABLED, nProxy)) && nProxy)
    {
        m_bIsProxy = TRUE;
        m_bIsEnabled = FALSE;
        m_szStatsRegRoot = ""; 
        rc = HXR_FAIL;
        goto cleanup;
    }

    IfFailGo(rc = ReadConfig());

    if (!m_bIsEnabled)
    {
        rc = HXR_OK;
        goto cleanup;
    }

    IfFailGo(rc = GetLicense(bDummy));

    m_szStatsRegRoot = z_szServerSDPStatsRegRoot;

    IfFailGo(rc = SetupRegistryEntries());

    return rc;

cleanup:

    m_bIsEnabled = FALSE;

    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pErrorLog);

    return rc;
}


HX_RESULT
SDPAggregateStats::ReadConfig()
{
    INT32 nTemp;

    if (SUCCEEDED(m_pRegistry->GetIntByName(z_szConfigAggregateStatsEnabled, nTemp)))
    {
        m_bIsEnabled = nTemp == 1 ? TRUE : FALSE;
    }
    else
    {
        m_bIsEnabled = FALSE;
    }
    
    return HXR_OK;
}

HX_RESULT
SDPAggregateStats::SetupRegistryEntries()
{    
    HX_RESULT rc = HXR_OK; 
    SDPStatsCounter* pStatInfo = NULL;
    const char* szRegPropName = NULL;

    UINT32 ulRegRootLen = strlen(m_szStatsRegRoot);

    HX_ASSERT(!m_bIsProxy);
    HX_ASSERT(m_bIsEnabled);

    if (m_pRegistry->AddComp(m_szStatsRegRoot) > 0)
    {
        DPRINTF(D_INFO,("SDP-Stats: Initializing registry entry %s\n",
                         m_szStatsRegRoot)); 
    }

    for (UINT32 i = 0; i < NUM_COUNTERS; ++i)
    {
        UINT32 ulCounterId = zm_StatInfoList[i].ulCounterType;
        szRegPropName = zm_StatInfoList[i].szRegPropName;

        if (!szRegPropName)
        {
            continue;
        }
        
        UINT32 ulPropNameBufLen = ulRegRootLen + strlen(szRegPropName) + 1;
        UINT32 ulRegId = 0;
        
        NEW_FAST_TEMP_STR(szRegEntry, 512, ulPropNameBufLen);
        snprintf(szRegEntry, ulPropNameBufLen, "%s%s", 
                 m_szStatsRegRoot, szRegPropName);

        pStatInfo = &(m_StatsCounterVec[ulCounterId]);
        pStatInfo->bOverflow = FALSE;

        if ((ulRegId = m_pRegistry->AddInt(szRegEntry, 0)) == 0)
        {
            if ((ulRegId = m_pRegistry->GetId(szRegEntry)) > 0)
            {
                if (m_pRegistry->GetTypeById(ulRegId) != PT_INTEGER)
                {
                    ulRegId = 0;
                }
            }
        }

        if (!ulRegId)
        {
            rc = HXR_FAIL;
            char szErrMsg[MAX_ERROR_BUFF_SIZE];
            snprintf(szErrMsg, MAX_ERROR_BUFF_SIZE - 1,
                     "ERROR: Unable to get/create registry entry %s\n",
                     szRegEntry); 
            szErrMsg[MAX_ERROR_BUFF_SIZE - 1] = '\0';
            m_pErrorLog->Report(HXLOG_ERR, 0, 0, szErrMsg, NULL);
        }
        else
        {
            pStatInfo->ulRegId = ulRegId;
        }
        
        DELETE_FAST_TEMP_STR(szRegEntry);
        
        if (FAILED(rc)) break;
    }

    return rc;
}


HX_RESULT
SDPAggregateStats::GetLicense(REF(BOOL) bIsLicensed)
{
    HX_RESULT rc = HXR_OK;
    if (!m_pRegistry)
    {
        bIsLicensed = FALSE;
        rc = HXR_UNEXPECTED;
        return rc;
    }

    INT32 lLicEnabled = 0;
    if (!m_bIsLicenseInit)
    {
        if (SUCCEEDED(m_pRegistry->GetIntByName(REGISTRY_RTSP_EVENTSTATS_ENABLED,
                                                lLicEnabled)))
        {
            if (lLicEnabled > 0)
            {
                m_bIsLicensed = TRUE;
            }
            else 
            {
                m_bIsLicensed = FALSE;
            }
        }
        else
        {
            m_bIsLicensed = FALSE;
        }
    }
    
    bIsLicensed = m_bIsLicensed;
    if (!m_bIsLicensed)
    {
        rc = HXR_NOT_LICENSED;
        if (m_pErrorLog 
        &&  !m_bIsLicenseInit)
        {
            // If not enabled we should've skipped this step entirely.
            HX_ASSERT(m_bIsEnabled);

            m_pErrorLog->Report(HXLOG_ERR, 
                                rc, 
                                0, 
                                "This server is not licensed to use the "
                                "SDP Aggregate Statistics feature.", 
                                NULL);
            m_bIsEnabled = FALSE;
        }
    }
    m_bIsLicenseInit = TRUE;

    return rc;
}

STDMETHODIMP
SDPAggregateStats::IncrementSDPDownloadCount(UINT32 ulProtocolId,
                                            BOOL bSuccess)
{
    HX_RESULT rc = HXR_OK;
    
    if (!m_bIsEnabled)
    {
        return rc;
    }

    if (!m_bIsLicensed)
    {
        rc = HXR_NOT_LICENSED;
        return rc;
    }

    HX_ASSERT(!m_bIsProxy);

    if (bSuccess)
    {
        UpdateCounter(1, TOTAL_DOWNLOAD_COUNT);

        switch (ulProtocolId)
        {
            case RTSP_PROT:
                UpdateCounter(1, SDP_RTSP_DOWNLOAD_COUNT);
                break;

            case HTTP_PROT:
                UpdateCounter(1, SDP_HTTP_DOWNLOAD_COUNT);
                break;

            default:
                rc = HXR_UNEXPECTED;
                break;
        }
    }

    return rc;
}

STDMETHODIMP
SDPAggregateStats::IncrementSDPSessionInitCount(UINT32 ulProtocolId,
                                                BOOL bSuccess)
{
    HX_RESULT rc = HXR_OK;
    
    if (!m_bIsEnabled)
    {
        return rc;
    }

    if (!m_bIsLicensed)
    {
        rc = HXR_NOT_LICENSED;
        return rc;
    }

    HX_ASSERT(!m_bIsProxy);

    if (bSuccess)
    {
        rc = UpdateCounter(1, SDP_SESSION_INIT_SUCCESS_COUNT);
    }
    else
    {
        rc = UpdateCounter(1, SDP_SESSION_INIT_ERROR_COUNT);
    }

    return rc;
}

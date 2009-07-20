/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: latency_mon_plin.cpp,v 1.5 2007/07/06 21:58:17 jfinnecy Exp $
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
#define INITGUID

#include "latency_mon_plin.h"
#include "hxtlogutil.h"
#include "hxprefs.h"
#include "hxprefutil.h"
#include "hxstring.h"

const char CHXLatencyMonitorPlugin::zm_pDescription[] = 
"Latency Monitor Plugin";

const char CHXLatencyMonitorPlugin::zm_pCopyright[] = "Helix Community 2004";
const char CHXLatencyMonitorPlugin::zm_pMoreInfoURL[] = "";

const UINT32 DefaultReportFrequency = 1000;

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(HXCREATEINSTANCE)(IUnknown** ppIUnknown)   
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (ppIUnknown)
    {
        *ppIUnknown = (IHXPlugin*)new CHXLatencyMonitorPlugin;

        if (*ppIUnknown)
        {
            (*ppIUnknown)->AddRef();
            res = HXR_OK;
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }

    return res;
}   


CHXLatencyMonitorPlugin::CHXLatencyMonitorPlugin() :
    m_ulRefCount(0),
    m_pEngine(NULL),
    m_pSched(NULL),
    m_uInterval(DefaultReportFrequency)
{}

CHXLatencyMonitorPlugin::~CHXLatencyMonitorPlugin()
{
    HX_RELEASE(m_pEngine);
    HX_RELEASE(m_pSched);
}

/*
 *	IUnknown methods
 */
STDMETHODIMP 
CHXLatencyMonitorPlugin::QueryInterface(THIS_
                                        REFIID riid,
                                        void** ppvObj)
{
    QInterfaceList qiList[] = {
        { GET_IIDHANDLE(IID_IHXPlugin), (IHXPlugin*)this },
        { GET_IIDHANDLE(IID_IHXGenericPlugin), (IHXGenericPlugin*)this },
        { GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*)this },
        { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXPlugin*)this }
    };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) 
CHXLatencyMonitorPlugin::AddRef(THIS)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32) 
CHXLatencyMonitorPlugin::Release(THIS)
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    if(m_ulRefCount == 0)
    {
        delete this;
    }
    return 0;
}

/*
 *	IHXPlugin methods
 */
STDMETHODIMP
CHXLatencyMonitorPlugin::GetPluginInfo(THIS_
                                       REF(HXBOOL)	/*OUT*/ bMultipleLoad,
                                       REF(const char*) /*OUT*/ pDescription,
                                       REF(const char*) /*OUT*/ pCopyright,
                                       REF(const char*) /*OUT*/ pMoreInfoURL,
                                       REF(ULONG32)	/*OUT*/ ulVersionNumber)
{
    bMultipleLoad   = FALSE;
    pDescription    = zm_pDescription;
    pCopyright      = zm_pCopyright;
    pMoreInfoURL    = zm_pMoreInfoURL;
    ulVersionNumber = 1;

    return HXR_OK;
}

STDMETHODIMP
CHXLatencyMonitorPlugin::InitPlugin(THIS_
                                    IUnknown*   /*IN*/  pContext)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if(pContext)
    {
        HX_ENABLE_LOGGING(pContext);

        res = pContext->QueryInterface(IID_IHXClientEngine, 
                                       (void**)&m_pEngine);

        if (HXR_OK == res)
        {
            res = pContext->QueryInterface(IID_IHXScheduler,
                                           (void**)&m_pSched);
        }

        if (HXR_OK == res)
        {
            IHXPreferences* pPrefs = NULL;
            UINT32 uTmp = 0;
            if ((HXR_OK == pContext->QueryInterface(IID_IHXPreferences,
                                                    (void**)&pPrefs)) &&
                (HXR_OK == ReadPrefUINT32(pPrefs, "LatencyMonitorInterval",
                                          uTmp)) &&
                (uTmp > 0))
            {
                m_uInterval = uTmp;
            }
            HX_RELEASE(pPrefs);
        }

        if (HXR_OK == res)
        {
            res = m_pSched->RelativeEnter(this, m_uInterval);
        }
    }

    return res;
}

/*
 *	IHXGenericPlugin methods
 */

STDMETHODIMP 
CHXLatencyMonitorPlugin::IsGeneric(THIS_
                                   REF(HXBOOL)	 /*OUT*/ bIsGeneric)
{
    bIsGeneric = TRUE;
    return HXR_OK;
}

/*
 *  IHXCallback methods
 */
STDMETHODIMP
CHXLatencyMonitorPlugin::Func(THIS)
{
    if (m_pEngine)
    {
        for (UINT16 i = 0; i < m_pEngine->GetPlayerCount(); i++)
        {
            IUnknown* pPlayerUnk = NULL;
            IHXPlayer* pPlayer = NULL;

            if ((HXR_OK == m_pEngine->GetPlayer(i, pPlayerUnk)) &&
                (HXR_OK == pPlayerUnk->QueryInterface(IID_IHXPlayer,
                                                      (void**)&pPlayer)))
            {
                for (UINT16 j = 0; j < pPlayer->GetSourceCount(); j++)
                {
                    IUnknown* pSrcUnk = NULL;
                    IHXStreamSource* pStreamSrc = NULL;

                    if ((HXR_OK == pPlayer->GetSource(j, pSrcUnk)) &&
                        (HXR_OK == pSrcUnk->QueryInterface(IID_IHXStreamSource,
                                                          (void**)&pStreamSrc)))
                    {
                        logStats(pSrcUnk, pStreamSrc->GetStreamCount());
                    }
                    HX_RELEASE(pStreamSrc);
                    HX_RELEASE(pSrcUnk);
                }
            }
            HX_RELEASE(pPlayer);
            HX_RELEASE(pPlayerUnk);
        }

        m_pSched->RelativeEnter(this, m_uInterval);
    }
    return HXR_OK;
}

void CHXLatencyMonitorPlugin::logStats(IUnknown* pSrc, UINT16 usStreamCount)
{
    if (pSrc)
    {
        IHXSourceBufferingStats3* pStats = NULL;

        if (HXR_OK == pSrc->QueryInterface(IID_IHXSourceBufferingStats3,
                                           (void**)&pStats))
        {
            CHXString line = " LATENCY ";
            HXBOOL bStatsAdded = FALSE;

            for (UINT16 i = 0; i < usStreamCount; i++)
            {
                UINT32 ulCurrentLow = 0;
                UINT32 ulCurrentHigh = 0;
                UINT32 uCurrentBytes = 0;
            
                UINT32 ulTotalLow = 0;
                UINT32 ulTotalHigh = 0;
                UINT32 uTotalBytes = 0;
                HXBOOL bDone;
            
                if ((HXR_OK == pStats->GetCurrentBuffering(i,
                                                           ulCurrentLow,
                                                           ulCurrentHigh,
                                                           uCurrentBytes,
                                                           bDone)) &&
                    (HXR_OK == pStats->GetTotalBuffering(i,
                                                         ulTotalLow,
                                                         ulTotalHigh,
                                                         uTotalBytes,
                                                         bDone)))
                {
                    
                    UINT32 ulTransportLatency = ulCurrentHigh - ulCurrentLow;
                    UINT32 ulTotalLatency = ulTotalHigh - ulTotalLow;
                    UINT32 ulRendererLatency = uTotalLatency - uTransportLatency;
                
                    char buf[100];
                    sprintf(buf, "%5u %5u %5u %5u ",
                            i,
                            ulTransportLatency,
                            ulRendererLatency,
                            ulTotalLatency);
                
                    line += buf;
                    bStatsAdded = TRUE;
                }
            }

            if (bStatsAdded)
            {
                HXLOGL2(HXLOG_GENE, (const char*)line);
            }
        }

        HX_RELEASE(pStats);
    }
}

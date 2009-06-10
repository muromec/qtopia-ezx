/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxovmgr.cpp,v 1.9 2008/06/04 06:49:33 ping Exp $
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxresult.h"
#include "hxstrutl.h"
#include "hxstring.h"
#include "pckunpck.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxcore.h"
#include "hxmon.h"
#include "hxprefs.h"
#include "hxwin.h"
#include "hxslist.h"
#include "hxthread.h"
#include "hxovmgr.h"
#include "hxtick.h"

#define DEFAULT_THERMOSTAT_FACTOR 2.0

//#define _DEBUG_LOG 

HXOverlayManager::HXOverlayManager(IUnknown* pContext)
    :   m_lRefCount(0)
    ,   m_pContext(pContext)
    ,   m_pCurrentOverlayOwner(NULL)
    ,   m_CallbackHandle(0)
    ,   m_pScheduler(NULL)
    ,   m_pOldOverlaySite(NULL)
    ,   m_pNewOverlaySite(NULL)
    ,   m_fThemoStatFactor(DEFAULT_THERMOSTAT_FACTOR)
    ,   m_bChangingOwner(FALSE)
    ,   m_pMutex(NULL)
{
    m_pContext->AddRef();
    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);  
}

HXOverlayManager::~HXOverlayManager()
{ 
    Close();
}

void HXOverlayManager::Initialize()
{
    m_pContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);

    IHXPreferences*    pPreferences    = NULL;
    IHXBuffer*         pBuffer         = NULL;

    if (HXR_OK == m_pContext->QueryInterface(IID_IHXPreferences,(void**)&pPreferences))
    {   
        if (pPreferences->ReadPref("ThermoStatFactor", pBuffer) == HXR_OK)
        {
            m_fThemoStatFactor = atof((char*)pBuffer->GetBuffer());  /* is this ANSI? if not use the next line */
            //sscanf((char*)pBuffer->GetBuffer(), "%f", &m_fThemoStatFactor); 
        }
        HX_RELEASE(pBuffer);
    }
    HX_RELEASE(pPreferences);
}

void
HXOverlayManager::Close()
{ 
    CSiteStats* pStats;
    CStatPoint* pPoint;

    while (m_ListOfSiteStats.GetCount())
    {
        pStats = (CSiteStats*) m_ListOfSiteStats.RemoveHead();
        while(pStats->samples.GetCount())
        {
            pPoint = (CStatPoint*) pStats->samples.RemoveHead();
            HX_DELETE(pPoint);
        }
        HX_DELETE(pStats);
    }

    if (m_CallbackHandle)
    {
        m_pScheduler->Remove(m_CallbackHandle);
        m_CallbackHandle = 0;
    }

    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pMutex);
}

/*
 * IUnknown methods
 */

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::QueryInterface
//      Purpose:
//              Implement this to export the interfaces supported by your 
//              object.
//
STDMETHODIMP HXOverlayManager::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXOverlayManager), (IHXOverlayManager*)this },
            { GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXOverlayManager*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::AddRef
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) HXOverlayManager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::Release
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) HXOverlayManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP HXOverlayManager::HasOverlay(THIS_
                IHXOverlayResponse* pResp
                ) 
{
    HX_LOCK(m_pMutex);

    HX_RESULT res = HXR_FAIL;
    if (!m_pCurrentOverlayOwner)
    {
        m_pCurrentOverlayOwner = pResp;
        res = HXR_OK;
    }

    HX_UNLOCK(m_pMutex);

    return res;
}

STDMETHODIMP HXOverlayManager::AddStats(THIS_
                                         IHXOverlayResponse* pResp,
                                         UINT32    ulNumPixels) 
{
    if (m_bChangingOwner)
        return HXR_OK;

    HX_LOCK(m_pMutex);

    CSiteStats* pStats;
    CStatPoint* pPoint;
    UINT32 time = HX_GET_TICKCOUNT();
    HXBOOL bExisted = FALSE;

    CHXSimpleList::Iterator i;
    for (i = m_ListOfSiteStats.Begin(); i != m_ListOfSiteStats.End(); ++i)
    {
        pStats = (CSiteStats*)*i;
        if (pStats->pResp == pResp)
        {
#ifdef _DEBUG_LOG
            FILE* f = fopen("c:\\overlay.txt", "a+");
            if (f)
            {
                fprintf(f, "Owner: %p  Pixels: %d\n", pResp, ulNumPixels);
                fclose(f);
            }
#endif
            AddStatPoint(pStats, ulNumPixels, time);
            bExisted = TRUE;
        }

        while(pStats->samples.GetCount())
        {
            pPoint = (CStatPoint*) pStats->samples.GetHead();
            if (pPoint->ulTime + 1000 > time)
            {
                break;
            }
            pStats->ulNumPixelsPerSecond -= pPoint->ulPixels;
            pStats->samples.RemoveHead();
            HX_DELETE(pPoint);
        }
#ifdef _DEBUG_LOG
            FILE* f = fopen("c:\\overlay.txt", "a+");
            if (f)
            {
                fprintf(f, "*Owner: %p  Pixels: %d Count: %d\n", pStats->pResp, pStats->ulNumPixelsPerSecond, pStats->samples.GetCount());
                fclose(f);
            }
#endif
    }

    if (!bExisted)
    {
        pStats                          = new CSiteStats;
        pStats->ulFirstTime             = time;
        pStats->ulNumPixelsPerSecond    = 0;
        pStats->pResp                   = pResp;
        m_ListOfSiteStats.AddTail(pStats);

        AddStatPoint(pStats, ulNumPixels, time);
    }

    ValidateCurrentOwner();

    HX_UNLOCK(m_pMutex);

    return HXR_OK;
}

void HXOverlayManager::AddStatPoint(CSiteStats* pStats, UINT32 ulNumPixels, UINT32 ulTime)
{
    CStatPoint* pPoint;
    pPoint = new CStatPoint;
    pPoint->ulTime = ulTime;
    pPoint->ulPixels = ulNumPixels;
    pStats->ulNumPixelsPerSecond  += pPoint->ulPixels;
    pStats->samples.AddTail(pPoint);
}

void HXOverlayManager::ValidateCurrentOwner()
{
    IHXOverlayResponse* pMaxResp = NULL;
    UINT32  ulMaxNumPixelsPerSecond = 0;
    UINT32  ulCurrentNumPixelsPerSecond = 0;
    CSiteStats* pStats = NULL;

    CHXSimpleList::Iterator i;
    for (i = m_ListOfSiteStats.Begin(); i != m_ListOfSiteStats.End(); ++i)
    {
        pStats = (CSiteStats*)*i;
        if (pStats->pResp == m_pCurrentOverlayOwner)
        {
            ulCurrentNumPixelsPerSecond = pStats->ulNumPixelsPerSecond;
        }
        if (ulMaxNumPixelsPerSecond < pStats->ulNumPixelsPerSecond)
        {
            ulMaxNumPixelsPerSecond = pStats->ulNumPixelsPerSecond;
            pMaxResp = pStats->pResp;
        }
    }

#ifdef _DEBUG_LOG
        FILE* f = fopen("c:\\overlay.txt", "a+");
        fprintf(f, "CurrentOvOwner = %p  Pels: %d \t\t\t Max: %p Pels: %d\n", m_pCurrentOverlayOwner, ulCurrentNumPixelsPerSecond, pMaxResp, ulMaxNumPixelsPerSecond);
        fclose(f);
#endif


    if ((double)ulMaxNumPixelsPerSecond > (double)ulCurrentNumPixelsPerSecond * 2)
    {
#ifdef _DEBUG_LOG
        UINT32 ulTempMaxNumPixelsPerSecond = 0;
        FILE* f = fopen("c:\\overlay.txt", "a+");
        if (f)
        {
            CHXSimpleList::Iterator i;
            for (i = m_ListOfSiteStats.Begin(); i != m_ListOfSiteStats.End(); ++i)
            {
                pStats = (CSiteStats*)*i;
                fprintf(f, "Owner: %p  Pixels: %d NumSamples: %d \n", pStats->pResp , pStats->ulNumPixelsPerSecond, pStats->samples.GetCount());
                if (pStats->pResp == m_pCurrentOverlayOwner)
                {
                    fprintf(f, "** Current Owner: %p Current Num: %d\n", pStats->pResp , pStats->ulNumPixelsPerSecond);
                }
                if (ulTempMaxNumPixelsPerSecond < pStats->ulNumPixelsPerSecond)
                {
                    fprintf(f, "** Current Max Owner: %p Current Num: %d\n", pStats->pResp , pStats->ulNumPixelsPerSecond);
                    ulTempMaxNumPixelsPerSecond = pStats->ulNumPixelsPerSecond;
                }
            }
            fclose(f);
        }
#endif
        ScheduleCallback(m_pCurrentOverlayOwner,   pMaxResp);
    }
}

void HXOverlayManager::ScheduleCallback(IHXOverlayResponse* pOld, IHXOverlayResponse* pNew)
{
    if (m_pScheduler && !m_CallbackHandle)
    {
        m_CallbackHandle = m_pScheduler->RelativeEnter(this, 0);
        m_pOldOverlaySite = pOld;
        m_pNewOverlaySite = pNew;
    }
}

/************************************************************************
 *  Method:
 *    IHXCallback::Func
 */

STDMETHODIMP HXOverlayManager::Func(void)
{
    m_bChangingOwner = TRUE;

    HX_LOCK(m_pMutex);

    AddRef();
    
    if( m_pOldOverlaySite && (HXR_OK == m_pOldOverlaySite->OverlayRevoked()) )
    {
        m_pCurrentOverlayOwner = NULL;
    }
    
    if( m_pNewOverlaySite && (HXR_OK == m_pNewOverlaySite->OverlayGranted()) )
    {
        m_pCurrentOverlayOwner = m_pNewOverlaySite;
    }
    
    
    m_pNewOverlaySite        = NULL;
    m_pOldOverlaySite        = NULL;
    m_CallbackHandle         = 0;

    Release();

    HX_UNLOCK(m_pMutex);

    m_bChangingOwner = FALSE;

    return HXR_OK;
}

STDMETHODIMP HXOverlayManager::RemoveOverlayRequest(THIS_ IHXOverlayResponse* pResp ) 
{
    HX_LOCK(m_pMutex);
    
    HX_RESULT res = HXR_FAIL;

    if (pResp == m_pNewOverlaySite && m_CallbackHandle)
    {
        m_pScheduler->Remove(m_CallbackHandle);
        m_CallbackHandle = 0;
        m_pNewOverlaySite = NULL;
        m_pOldOverlaySite = NULL;    
    }

    if (m_pCurrentOverlayOwner == pResp)
    {
        m_pCurrentOverlayOwner = NULL;
    }

    CSiteStats* pStats;
    CStatPoint* pPoint;

    LISTPOSITION pos = NULL;
    
    pos = m_ListOfSiteStats.GetHeadPosition();
    while (pos)
    {
        pStats = (CSiteStats*)m_ListOfSiteStats.GetAt(pos);
        if (pStats->pResp == pResp)
        {
            res = HXR_OK;
            while(pStats->samples.GetCount())
            {
                pPoint = (CStatPoint*) pStats->samples.RemoveHead();
                HX_DELETE(pPoint);
            }
            HX_DELETE(pStats);
            m_ListOfSiteStats.RemoveAt(pos);
            break;
        }
        m_ListOfSiteStats.GetNext(pos);
    }

    HX_UNLOCK(m_pMutex);
    return res;
}

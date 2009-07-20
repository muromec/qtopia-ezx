/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mediamrk.cpp,v 1.5 2005/03/14 20:31:02 bobclark Exp $
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

#if defined(HELIX_FEATURE_MEDIAMARKER)
// include
#include "hxtypes.h"
#include "hxcom.h"
#include "hxresult.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxgroup.h"
#include "hxmmrkr.h"
// pncont
#include "hxslist.h"
#include "hxmap.h"
// rmacore
#include "hxplay.h"
#include "hxcleng.h"
#include "mediamrk.h"
// pndebug
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE             
static const char HX_THIS_FILE[] = __FILE__;
#endif

CMarkerInfo::CMarkerInfo(IHXBuffer* pURLStr,
                         IHXBuffer* pNameStr,
                         UINT32      ulTime,
                         IHXValues* pOtherParams)
{
    m_pURLStr      = pURLStr;
    m_pNameStr     = pNameStr;
    m_ulTime       = ulTime;
    m_pOtherParams = pOtherParams;
    if (m_pURLStr)      m_pURLStr->AddRef();
    if (m_pNameStr)     m_pNameStr->AddRef();
    if (m_pOtherParams) m_pOtherParams->AddRef();
}

CMarkerInfo::~CMarkerInfo()
{
    HX_RELEASE(m_pURLStr);
    HX_RELEASE(m_pNameStr);
    HX_RELEASE(m_pOtherParams);
}

CHXMediaMarkerManager::CHXMediaMarkerManager(HXPlayer* pPlayer)
{
    m_lRefCount   = 0;
    m_pPlayer     = pPlayer;
    m_pMarkerList = NULL;
    m_pSinkList   = NULL;
    if (m_pPlayer)
    {
        m_pPlayer->AddRef();
    }
}

CHXMediaMarkerManager::~CHXMediaMarkerManager()
{
    Close();
}

STDMETHODIMP CHXMediaMarkerManager::QueryInterface(REFIID riid, void** ppvObj)
{
    HX_RESULT retVal = HXR_OK;

    if (ppvObj)
    {
        QInterfaceList qiList[] =
            {
                { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXMediaMarkerManager*)this },
                { GET_IIDHANDLE(IID_IHXMediaMarkerManager), (IHXMediaMarkerManager*)this },
            };
        
        retVal = ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
    }
    else
    {
        retVal = HXR_FAIL;
    }

    return retVal;
}

STDMETHODIMP_(ULONG32) CHXMediaMarkerManager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXMediaMarkerManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;

    return 0;
}

STDMETHODIMP CHXMediaMarkerManager::Close()
{
    HX_RESULT retVal = HXR_OK;

    HX_RELEASE(m_pPlayer);
    ClearMarkerList();
    ClearSinkList();

    return retVal;
}

STDMETHODIMP CHXMediaMarkerManager::AddMediaMarkerSink(IHXMediaMarkerSink* pSink)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pSink)
    {
        // Do we have a list?
        if (!m_pSinkList)
        {
            m_pSinkList = new CHXSimpleList();
        }
        if (m_pSinkList)
        {
            // Make sure this sink is not already on the list
            if (!WasSinkAdded(pSink))
            {
                // AddRef before we put it on the list
                pSink->AddRef();
                // Now put this sink on the tail of the list
                m_pSinkList->AddTail((void*) pSink);
                // Clear the return value
                retVal = HXR_OK;
            }
        }
    }

    return retVal;
}

STDMETHODIMP CHXMediaMarkerManager::RemoveMediaMarkerSink(IHXMediaMarkerSink* pSink)
{
    HX_RESULT retVal = HXR_OK;

    if (pSink)
    {
        if (m_pSinkList)
        {
            LISTPOSITION pos = m_pSinkList->GetHeadPosition();
            while (pos)
            {
                IHXMediaMarkerSink* pListSink =
                    (IHXMediaMarkerSink*) m_pSinkList->GetAt(pos);
                if (pListSink == pSink)
                {
                    m_pSinkList->RemoveAt(pos);
                    HX_RELEASE(pListSink);
                    break;
                }
                m_pSinkList->GetNext(pos);
            }
        }
    }
    else
    {
        retVal = HXR_FAIL;
    }

    return retVal;
}

STDMETHODIMP CHXMediaMarkerManager::AddMediaMarkerSinkFilter(IHXMediaMarkerSink* pSink,
                                                              IHXValues*          pFilter)
{
    HX_RESULT retVal = HXR_NOTIMPL;

    return retVal;
}

STDMETHODIMP CHXMediaMarkerManager::ResolveMarker(IHXBuffer* pURLStr,
                                                   IHXBuffer* pMarkerNameStr,
                                                   UINT32      ulTime,
                                                   IHXValues* pOtherMarkerParams)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pURLStr && pMarkerNameStr)
    {
        // Create a CMarkerInfo struct
        CMarkerInfo* pInfo = new CMarkerInfo(pURLStr, pMarkerNameStr,
                                             ulTime, pOtherMarkerParams);
        if (pInfo)
        {
            // Add the marker to all our data structures. This method
            // will fail if an identical marker is already present
            retVal = AddMarkerInfo(pInfo);
            if (SUCCEEDED(retVal))
            {
                // Now we need to broadcast this marker to
                // all the sinks who haven't filtered this
                // marker out. 
                BroadcastMarkerToSinks(pInfo);
            }
            else
            {
                HX_DELETE(pInfo);
            }
        }
    }

    return retVal;
}

STDMETHODIMP_(HXBOOL) CHXMediaMarkerManager::IsMarkerResolved(IHXBuffer* pURLStr,
                                                             IHXBuffer* pMarkerNameStr)
{
    HX_RESULT retVal = HXR_NOTIMPL;

    return retVal;
}

STDMETHODIMP CHXMediaMarkerManager::GetFirstMarker(IHXMediaMarkerSink* pSink,
                                                    REF(IHXBuffer*)     rpURLStr,
                                                    REF(IHXBuffer*)     rpMarkerNameStr,
                                                    REF(UINT32)          rulTime,
                                                    REF(IHXValues*)     rpOtherMarkerParams)
{
    HX_RESULT retVal = HXR_NOTIMPL;

    return retVal;
}

STDMETHODIMP CHXMediaMarkerManager::GetNextMarker(IHXMediaMarkerSink* pSink,
                                                   REF(IHXBuffer*)     rpURLStr,
                                                   REF(IHXBuffer*)     rpMarkerNameStr,
                                                   REF(UINT32)          rulTime,
                                                   REF(IHXValues*)     rpOtherMarkerParams)
{
    HX_RESULT retVal = HXR_NOTIMPL;

    return retVal;
}

void CHXMediaMarkerManager::ClearMarkerList()
{
    if (m_pMarkerList)
    {
        // Run through the list, freeing the marker objects
        LISTPOSITION pos = m_pMarkerList->GetHeadPosition();
        while (pos)
        {
            CMarkerInfo* pInfo = (CMarkerInfo*) m_pMarkerList->GetNext(pos);
            HX_DELETE(pInfo);
        }
        m_pMarkerList->RemoveAll();
        HX_DELETE(m_pMarkerList);
    }
}

void CHXMediaMarkerManager::ClearSinkList()
{
    if (m_pSinkList)
    {
        // Run through the list, releasing the sinks
        LISTPOSITION pos = m_pSinkList->GetHeadPosition();
        while (pos)
        {
            IHXMediaMarkerSink* pSink = (IHXMediaMarkerSink*) m_pSinkList->GetNext(pos);
            HX_RELEASE(pSink);
        }
        m_pSinkList->RemoveAll();
        HX_DELETE(m_pSinkList);
    }
}

HXBOOL CHXMediaMarkerManager::WasSinkAdded(IHXMediaMarkerSink* pSink)
{
    HXBOOL bRet = FALSE;

    if (m_pSinkList)
    {
        LISTPOSITION pos = m_pSinkList->GetHeadPosition();
        while (pos)
        {
            IHXMediaMarkerSink* pListSink =
                (IHXMediaMarkerSink*) m_pSinkList->GetNext(pos);
            if (pListSink == pSink)
            {
                bRet = TRUE;
                break;
            }
        }
    }

    return bRet;
}

HX_RESULT CHXMediaMarkerManager::AddMarkerInfo(CMarkerInfo* pInfo)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInfo)
    {
        // First we add the marker to the master list
        //
        // Do we have a master list yet?
        if (!m_pMarkerList)
        {
            // We don't - create one
            m_pMarkerList = new CHXSimpleList();
        }
        if (m_pMarkerList)
        {
            // This master list will be sorted in increasing time. Therefore,
            // since we expect most datatypes to add markers in increasing
            // time, then we search the list from the TAIL for the right
            // place to insert.
            LISTPOSITION pos = m_pMarkerList->GetTailPosition();
            while (pos)
            {
                CMarkerInfo* pLInfo = (CMarkerInfo*) m_pMarkerList->GetAt(pos);
                if (pLInfo && pLInfo->m_ulTime <= pInfo->m_ulTime)
                {
                    if (!AreMarkersIdentical(pLInfo, pInfo))
                    {
                        // Time of the list marker is either less
                        // than or equal to ours, but we know they
                        // are not the same marker, so we can safely
                        // add our marker after the list marker.
                        m_pMarkerList->InsertAfter(pos, (void*) pInfo);
                        // We DID add the marker, so clear the return value
                        retVal = HXR_OK;
                    }
                    // Whether we added or not, we are bustin' outta here.
                    break;
                }
                m_pMarkerList->GetPrev(pos);
            }
            // If we haven't already inserted this marker, then
            // add it at the head and clear the return value
            if (FAILED(retVal))
            {
                m_pMarkerList->AddHead((void*) pInfo);
                retVal = HXR_OK;
            }
            // XXXMEH - TODO - Add this marker to any additional
            // data structures we will keep
        }
    }

    return retVal;
}

HXBOOL CHXMediaMarkerManager::AreMarkersIdentical(CMarkerInfo* pInfo1, CMarkerInfo* pInfo2)
{
    HXBOOL bRet = FALSE;

    if (pInfo1 && pInfo2)
    {
        // Check url
        if (pInfo1->m_pURLStr && pInfo2->m_pURLStr &&
            !strcmp((const char*) pInfo1->m_pURLStr->GetBuffer(),
                    (const char*) pInfo2->m_pURLStr->GetBuffer()))
        {
            // Check marker name
            if (pInfo1->m_pNameStr && pInfo2->m_pNameStr &&
                !strcmp((const char*) pInfo1->m_pNameStr->GetBuffer(),
                        (const char*) pInfo2->m_pNameStr->GetBuffer()))
            {
                // Check time
                if (pInfo1->m_ulTime == pInfo2->m_ulTime)
                {
                    bRet = TRUE;
                }
            }
        }
    }

    return bRet;
}

void CHXMediaMarkerManager::BroadcastMarkerToSinks(CMarkerInfo* pInfo)
{
    if (pInfo && m_pSinkList)
    {
        // Loop through the list of marker sinks
        LISTPOSITION pos = m_pSinkList->GetHeadPosition();
        while (pos)
        {
            // Get the marker sink and check whether this
            // marker should be passed to this sink
            IHXMediaMarkerSink* pSink =
                (IHXMediaMarkerSink*) m_pSinkList->GetNext(pos);
            if (SinkWantsThisMarker(pSink, pInfo))
            {
                // This sink DOES want to be notified of this marker,
                // so call IHXMediaMarkerSink::MarkerResolved()
                pSink->MarkerResolved(pInfo->m_pURLStr,
                                      pInfo->m_pNameStr,
                                      pInfo->m_ulTime,
                                      pInfo->m_pOtherParams);
            }
        }
    }
}

HXBOOL CHXMediaMarkerManager::SinkWantsThisMarker(IHXMediaMarkerSink* pSink,
                                                 CMarkerInfo*         pInfo)
{
    HXBOOL bRet = TRUE;

    return bRet;
}

#endif /* #if defined(HELIX_FEATURE_MEDIAMARKER) */

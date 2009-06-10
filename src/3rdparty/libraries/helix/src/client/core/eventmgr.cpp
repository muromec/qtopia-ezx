/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: eventmgr.cpp,v 1.8 2007/07/06 21:58:11 jfinnecy Exp $
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

#if defined(HELIX_FEATURE_EVENTMANAGER)
// include
#include "hxtypes.h"
#include "hxcom.h"
#include "hxresult.h"
#include "ihxpckts.h"
#include "hxcomm.h"
#include "hxengin.h"
#include "hxinter.h"
// pncont
#include "hxslist.h"
#include "hxmap.h"
#include "hxthread.h"
// rmacore
#include "hxcbobj.h"
#include "eventmgr.h"
// pndebug
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE             
static const char HX_THIS_FILE[] = __FILE__;
#endif

CRendererEventManager::CRendererEventManager(IUnknown* pContext)
{
    m_lRefCount        = 0;
    m_pContext         = pContext;
    m_pScheduler       = NULL;
    m_pSinkList        = NULL;
    m_pSinkToFilterMap = NULL;
    m_pCallback        = NULL;
    m_pEventQueue      = NULL;
    m_pEventQueueMutex = NULL;
    if (m_pContext)
    {
        // AddRef the context
        m_pContext->AddRef();
        // QI for IHXCommonClassFactory
        IHXCommonClassFactory* pFactory = NULL;
        m_pContext->QueryInterface(IID_IHXCommonClassFactory,
                                   (void**) &pFactory);
        if (pFactory)
        {
            // Create a mutex
            pFactory->CreateInstance(CLSID_IHXMutex,
                                     (void**) &m_pEventQueueMutex);
        }
        HX_RELEASE(pFactory);
    }
}

CRendererEventManager::~CRendererEventManager()
{
    Close();
}

STDMETHODIMP CRendererEventManager::QueryInterface(REFIID riid, void** ppvObj)
{
    HX_RESULT retVal = HXR_OK;

    if (ppvObj)
    {
        QInterfaceList qiList[] =
            {
                { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXEventManager*)this },
                { GET_IIDHANDLE(IID_IHXEventManager), (IHXEventManager*)this },
                { GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*)this },
            };
        
        retVal = ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
    }
    else
    {
        retVal = HXR_FAIL;
    }

    return retVal;
}

STDMETHODIMP_(ULONG32) CRendererEventManager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CRendererEventManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;

    return 0;
}

STDMETHODIMP CRendererEventManager::AddEventSink(IHXEventSink* pSink)
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
            // Clear the return value
            retVal = HXR_OK;
            // Make sure this sink is not already on the list
            if (!IsSinkInList(pSink))
            {
                // AddRef before we put it on the list
                pSink->AddRef();
                // Now put this sink on the tail of the list
                m_pSinkList->AddTail((void*) pSink);
                // Add an NULL entry for this sink in the map
                if (!m_pSinkToFilterMap)
                {
                    m_pSinkToFilterMap = new CHXMapPtrToPtr();
                }
                if (m_pSinkToFilterMap)
                {
                    m_pSinkToFilterMap->SetAt((void*) pSink, NULL);
                }
            }
        }
    }

    return retVal;
}

STDMETHODIMP CRendererEventManager::RemoveEventSink(IHXEventSink* pSink)
{
    HX_RESULT retVal = HXR_OK;

    if (pSink && m_pSinkList && m_pSinkList->GetCount() > 0)
    {
        // Remove the sink's entry in the sink-to-filter map and
        // release our ref on the filter
        void* pVoid = NULL;
        if (m_pSinkToFilterMap &&
            m_pSinkToFilterMap->Lookup((void*) pSink, pVoid))
        {
            // Get the key value list
            CHXSimpleList* pList = (CHXSimpleList*) pVoid;
            // Clear this list
            ClearSinkFilterList(pList);
            // Delete the rule list
            HX_DELETE(pList);
            // Remove the entry from the sink-to-filter map
            m_pSinkToFilterMap->RemoveKey((void*) pSink);
        }
        // Run through the sink list and release
        // the list's ref on the sink
        LISTPOSITION pos = m_pSinkList->GetHeadPosition();
        while (pos)
        {
            IHXEventSink* pListSink =
                (IHXEventSink*) m_pSinkList->GetAt(pos);
            if (pListSink && pListSink == pSink)
            {
                m_pSinkList->RemoveAt(pos);
                HX_RELEASE(pListSink);
                break;
            }
            m_pSinkList->GetNext(pos);
        }
    }

    return retVal;
}

STDMETHODIMP CRendererEventManager::AddEventSinkFilterRule(IHXEventSink* pSink,
                                                           IHXValues*    pRule)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pSink && pRule && m_pSinkToFilterMap)
    {
        // Do we have an entry in the filter map? We definitely
        // should, because it was initialized when the sink
        // was added to the sink list. The entry can be NULL,
        // but there has to be an entry.
        void* pVoid = NULL;
        if (m_pSinkToFilterMap->Lookup((void*) pSink, pVoid))
        {
            // Get the filter
            CHXSimpleList* pFilter = (CHXSimpleList*) pVoid;
            // If the filter is NULL, then that means
            // we need to create the list and put it
            // back into the map
            if (!pFilter)
            {
                pFilter = new CHXSimpleList();
                if (pFilter)
                {
                    m_pSinkToFilterMap->SetAt((void*) pSink,
                                              (void*) pFilter);
                }
            }
            if (pFilter)
            {
                // Create the CEventSinkFilterRule object
                CEventSinkFilterRule* pRuleObj =
                    new CEventSinkFilterRule(pRule);
                if (pRuleObj)
                {
                    // Make sure this rule is not already in the list
                    HXBOOL bPresent = FALSE;
                    LISTPOSITION pos = pFilter->GetHeadPosition();
                    while (pos)
                    {
                        CEventSinkFilterRule* pListRuleObj =
                            (CEventSinkFilterRule*) pFilter->GetNext(pos);
                        if (pListRuleObj &&
                            pListRuleObj->Same(pRuleObj))
                        {
                            bPresent = TRUE;
                            break;
                        }
                    }
                    // Was the rule already present?
                    if (!bPresent)
                    {
                        // It was NOT already present, so we
                        // can append it to the list of rules
                        pFilter->AddTail((void*) pRuleObj);
                    }
                    else
                    {
                        // It was already present, so we can
                        // delete this object
                        HX_DELETE(pRuleObj);
                    }
                    // Clear the return value
                    retVal = HXR_OK;
                }
            }
        }
    }

    return retVal;
}

STDMETHODIMP CRendererEventManager::RemoveEventSinkFilterRule(IHXEventSink* pSink,
                                                              IHXValues*    pRule)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pSink && pRule && m_pSinkToFilterMap)
    {
        // Do we have an entry in the filter map? We definitely
        // should, because it was initialized when the sink
        // was added to the sink list. The entry can be NULL,
        // but there has to be an entry.
        void* pVoid = NULL;
        if (m_pSinkToFilterMap->Lookup((void*) pSink, pVoid))
        {
            // Get the filter
            CHXSimpleList* pFilter = (CHXSimpleList*) pVoid;
            // Do we have a filter for this sink yet?
            if (pFilter)
            {
                // Create a temp CEventSinkFilterRule object
                // just so we can use CEventSinkFilterRule::Same()
                CEventSinkFilterRule* pRuleObj =
                    new CEventSinkFilterRule(pRule);
                if (pRuleObj)
                {
                    // Find this rule in the list
                    LISTPOSITION pos = pFilter->GetHeadPosition();
                    while (pos)
                    {
                        CEventSinkFilterRule* pListRuleObj =
                            (CEventSinkFilterRule*) pFilter->GetAt(pos);
                        if (pListRuleObj &&
                            pListRuleObj->Same(pRuleObj))
                        {
                            // Remove the rule from the list
                            pFilter->RemoveAt(pos);
                            // Delete the rule object
                            HX_DELETE(pListRuleObj);
                            // Jump out of the loop
                            break;
                        }
                        else
                        {
                            pFilter->GetNext(pos);
                        }
                    }
                    // Clear the return value
                    retVal = HXR_OK;
                }
                // Delete the temporary rule
                HX_DELETE(pRuleObj);
            }
        }
    }

    return retVal;
}

STDMETHODIMP CRendererEventManager::FireEvent(IHXBuffer* pURLStr,
                                              IHXBuffer* pFragmentStr,
                                              IHXBuffer* pEventNameStr,
                                              IHXValues* pOtherValues)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pURLStr && pEventNameStr)
    {
        // Do we have a scheduler interface?
        if (!m_pScheduler && m_pContext)
        {
            // QI for IHXScheduler
            m_pContext->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler);
        }
        if (m_pScheduler)
        {
            // Do we have a callback?
            if (!m_pCallback)
            {
                // Create a callback object
                m_pCallback = new CHXGenericCallback((void*)this, (fGenericCBFunc)RendererEventCallback);
                if (m_pCallback)
                {
                    m_pCallback->AddRef();
                }
            }
            if (m_pCallback)
            {
                // Create the event object
                CRendererEvent* pEvent = new CRendererEvent(pURLStr,
                                                            pFragmentStr,
                                                            pEventNameStr,
                                                            pOtherValues);
                if (pEvent)
                {
                    // Do we have an event queue?
                    if (!m_pEventQueue)
                    {
                        // Create an event queue
                        m_pEventQueue = new CHXSimpleList();
                    }
                    if (m_pEventQueue)
                    {
                        // Lock the mutex
			HX_LOCK(m_pEventQueueMutex);

                        // Put the event object on the queue
                        m_pEventQueue->AddTail((void*) pEvent);
                        // If we don't already have a callback scheduled
                        // then schedule one
                        if (!m_pCallback->GetPendingCallback())
                        {
                            m_pCallback->CallbackScheduled(m_pScheduler->RelativeEnter(m_pCallback, 0));
                        }
                        // Unlock the mutex
                        HX_UNLOCK(m_pEventQueueMutex);
                        // Clear the return value
                        retVal = HXR_OK;
                    }
                }
                if (FAILED(retVal))
                {
                    HX_DELETE(pEvent);
                }
            }
        }
    }

    return retVal;
}

STDMETHODIMP CRendererEventManager::Func()
{
    HX_RESULT retVal = HXR_OK;

    // Lock the mutex
    HX_LOCK(m_pEventQueueMutex);

    // Fire off all the events in the event queue
    if (m_pEventQueue && m_pEventQueue->GetCount() > 0)
    {
        LISTPOSITION pos = m_pEventQueue->GetHeadPosition();
        while (pos)
        {
            // Get the event
            CRendererEvent* pEvent =
                (CRendererEvent*) m_pEventQueue->GetNext(pos);
            if (pEvent)
            {
                // Loop through our lists and test each sink
                // to see if it should be passed this event
                //
                // XXXMEH - TODO - short-circuit the callback - if
                // we find out that there are no sinks registered for
                // this kind of event up front, then we shouldn't 
                // even fire the callback
                //
                if (m_pSinkList && m_pSinkList->GetCount() > 0)
                {
                    LISTPOSITION pos = m_pSinkList->GetHeadPosition();
                    while (pos)
                    {
                        IHXEventSink* pSink =
                            (IHXEventSink*) m_pSinkList->GetNext(pos);
                        if (ShouldSinkGetEvent(pSink, pEvent))
                        {
                            pEvent->Fire(pSink);
                        }
                    }
                }
            }
            // Now that we've processed the event,
            // we can delete it
            HX_DELETE(pEvent);
        }
        // Now remove everything from the queue
        m_pEventQueue->RemoveAll();
    }

    // Unlock the mutex
    HX_UNLOCK(m_pEventQueueMutex);

    return retVal;
}

STDMETHODIMP CRendererEventManager::Close()
{
    HX_RESULT retVal = HXR_OK;

    // Cleanup all remaining callbacks
    if (m_pCallback && m_pScheduler)
    {
        m_pScheduler->Remove(m_pCallback->GetPendingCallback());
        m_pCallback->CallbackCanceled();
    }

    HX_RELEASE(m_pCallback);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pContext);
    ClearSinks();
    HX_DELETE(m_pSinkList);
    HX_DELETE(m_pSinkToFilterMap);
    ClearEventQueue();
    HX_DELETE(m_pEventQueue);
    HX_RELEASE(m_pEventQueueMutex);

    return retVal;
}

void CRendererEventManager::ClearSinks()
{
    if (m_pSinkToFilterMap)
    {
        // Run through the filter map and release each entry
        POSITION pos = m_pSinkToFilterMap->GetStartPosition();
        while (pos)
        {
            void* pKey   = NULL;
            void* pEntry = NULL;
            m_pSinkToFilterMap->GetNextAssoc(pos, pKey, pEntry);
            if (pEntry)
            {
                // Get the filter
                CHXSimpleList* pFilter = (CHXSimpleList*) pEntry;
                // Clear this list
                ClearSinkFilterList(pFilter);
                // Delete the rule list
                HX_DELETE(pFilter);
            }
        }
        m_pSinkToFilterMap->RemoveAll();
    }
    if (m_pSinkList)
    {
        // Run through the list, releasing the sinks
        LISTPOSITION pos = m_pSinkList->GetHeadPosition();
        while (pos)
        {
            IHXEventSink* pSink = (IHXEventSink*) m_pSinkList->GetNext(pos);
            HX_RELEASE(pSink);
        }
        m_pSinkList->RemoveAll();
    }
}

HXBOOL CRendererEventManager::IsSinkInList(IHXEventSink* pSink)
{
    HXBOOL bRet = FALSE;

    if (pSink && m_pSinkList)
    {
        LISTPOSITION pos = m_pSinkList->GetHeadPosition();
        while (pos)
        {
            IHXEventSink* pListSink =
                (IHXEventSink*) m_pSinkList->GetNext(pos);
            if (pListSink &&
                pListSink == pSink)
            {
                bRet = TRUE;
                break;
            }
        }
    }

    return bRet;
}

void CRendererEventManager::ClearEventQueue()
{
    if (m_pEventQueue && m_pEventQueue->GetCount() > 0)
    {
        LISTPOSITION pos = m_pEventQueue->GetHeadPosition();
        while (pos)
        {
            CRendererEvent* pEvent =
                (CRendererEvent*) m_pEventQueue->GetNext(pos);
            HX_DELETE(pEvent);
        }
        m_pEventQueue->RemoveAll();
    }
}

HXBOOL CRendererEventManager::ShouldSinkGetEvent(IHXEventSink*  pSink,
                                               CRendererEvent* pEvent)
{
    HXBOOL bRet = FALSE;

    if (pSink && pEvent && m_pSinkToFilterMap)
    {
        // Look up the filter for this sink
        void* pVoid = NULL;
        if (m_pSinkToFilterMap->Lookup((void*) pSink, pVoid))
        {
            // Get the filter
            CHXSimpleList* pFilter = (CHXSimpleList*) pVoid;
            // Check if the event passes the filter
            bRet = PassFilter(pFilter, pEvent);
        }
    }

    return bRet;
}

HXBOOL CRendererEventManager::PassFilter(CHXSimpleList*  pFilter,
                                       CRendererEvent* pEvent)
{
    HXBOOL bRet = FALSE;

    // Do we have a non-NULL filter?
    if (pFilter)
    {
        if (pEvent)
        {
            // Loop through the rules until we find a rule
            // that will allow this event to pass through
            LISTPOSITION pos = pFilter->GetHeadPosition();
            while (pos)
            {
                CEventSinkFilterRule* pRule =
                    (CEventSinkFilterRule*) pFilter->GetNext(pos);
                if (PassFilterRule(pRule, pEvent))
                {
                    bRet = TRUE;
                    break;
                }
            }
        }
    }
    else
    {
        // We don't have a filter, so the event passes
        bRet = TRUE;
    }

    return bRet;
}

HXBOOL CRendererEventManager::PassFilterRule(CEventSinkFilterRule* pRule,
                                           CRendererEvent*       pEvent)
{
    HXBOOL bRet = FALSE;

    if (pRule && pEvent)
    {
        // Do we pass all of the properties?
        if (PassFilterRuleString(pRule->GetURL(),       pEvent->GetURL())      &&
            PassFilterRuleString(pRule->GetFragment(),  pEvent->GetFragment()) &&
            PassFilterRuleString(pRule->GetEventName(), pEvent->GetEventName()))
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

HXBOOL CRendererEventManager::PassFilterRuleString(const char* pszRule,
                                                 const char* pszEvent)
{
    HXBOOL bRet = FALSE;

    // If the rule string is NULL, that's an automatic pass,
    // because the rule is not making any statement about this
    // particular property.
    // If the rule string is NOT NULL, then the two
    // strings have to match exactly.
    if (!pszRule || (pszEvent && !strcmp(pszRule, pszEvent)))
    {
        bRet = TRUE;
    }

    return bRet;
}

void CRendererEventManager::ClearSinkFilterList(CHXSimpleList* pList)
{
    if (pList)
    {
        LISTPOSITION pos = pList->GetHeadPosition();
        while (pos)
        {
            CEventSinkFilterRule* pRule =
                (CEventSinkFilterRule*) pList->GetNext(pos);
            HX_DELETE(pRule);
        }
        pList->RemoveAll();
    }
}

CEventSinkFilterRule::CEventSinkFilterRule(IHXValues* pRule)
{
    m_pRule = pRule;
    if (m_pRule)
    {
        m_pRule->AddRef();
    }
}

CEventSinkFilterRule::~CEventSinkFilterRule()
{
    HX_RELEASE(m_pRule);
}

HXBOOL CEventSinkFilterRule::Same(CEventSinkFilterRule* pRule)
{
    HXBOOL bRet = FALSE;

    if (pRule)
    {
        if (SameString(GetURL(),       pRule->GetURL())      &&
            SameString(GetFragment(),  pRule->GetFragment()) &&
            SameString(GetEventName(), pRule->GetEventName()))
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

const char* CEventSinkFilterRule::GetURL() const
{
    return GetString(m_pRule, FILTER_RULE_KEY_URL);
}

const char* CEventSinkFilterRule::GetFragment() const
{
    return GetString(m_pRule, FILTER_RULE_KEY_FRAGMENT);
}

const char* CEventSinkFilterRule::GetEventName() const
{
    return GetString(m_pRule, FILTER_RULE_KEY_EVENTNAME);
}

HXBOOL CEventSinkFilterRule::SameString(const char* pszA,
                                      const char* pszB)
{
    HXBOOL bRet = FALSE;

    // If both of the strings are NULL, that's TRUE.
    // If both of the strings are NOT NULL and the
    // strings match, then that's TRUE.
    // Anything else is FALSE.
    if ((!pszA && !pszB) ||
        (pszA  &&  pszB && !strcmp(pszA, pszB)))
    {
        bRet = TRUE;
    }

    return bRet;
}

const char* CEventSinkFilterRule::GetString(IHXValues* pValues,
                                            const char* pszName)
{
    const char* pRet = NULL;

    if (pValues && pszName)
    {
        IHXBuffer* pBuf = NULL;
        pValues->GetPropertyCString(pszName, pBuf);
        if (pBuf)
        {
            pRet = (const char*) pBuf->GetBuffer();
        }
        HX_RELEASE(pBuf);
    }

    return pRet;
}

void CRendererEventManager::RendererEventCallback(void* pParam)
{
    CRendererEventManager* pObj = (CRendererEventManager*)pParam;

    if (pObj)
    {
        pObj->Func();
    }
}

CRendererEvent::CRendererEvent(IHXBuffer* pURLStr,
                               IHXBuffer* pFragmentStr,
                               IHXBuffer* pEventNameStr,
                               IHXValues* pOtherValues)
{
    m_pURLStr       = pURLStr;
    m_pFragmentStr  = pFragmentStr;
    m_pEventNameStr = pEventNameStr;
    m_pOtherValues  = pOtherValues;
    if (m_pURLStr)       m_pURLStr->AddRef();
    if (m_pFragmentStr)  m_pFragmentStr->AddRef();
    if (m_pEventNameStr) m_pEventNameStr->AddRef();
    if (m_pOtherValues)  m_pOtherValues->AddRef();
}

CRendererEvent::~CRendererEvent()
{
    HX_RELEASE(m_pURLStr);
    HX_RELEASE(m_pFragmentStr);
    HX_RELEASE(m_pEventNameStr);
    HX_RELEASE(m_pOtherValues);
}

HX_RESULT CRendererEvent::Fire(IHXEventSink* pSink)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pSink)
    {
        retVal = pSink->EventFired(m_pURLStr,
                                   m_pFragmentStr,
                                   m_pEventNameStr,
                                   m_pOtherValues);
    }

    return retVal;
}

const char* CRendererEvent::GetURL() const
{
    const char* pRet = NULL;

    if (m_pURLStr)
    {
        pRet = (const char*) m_pURLStr->GetBuffer();
    }

    return pRet;
}

const char* CRendererEvent::GetFragment() const
{
    const char* pRet = NULL;

    if (m_pFragmentStr)
    {
        pRet = (const char*) m_pFragmentStr->GetBuffer();
    }

    return pRet;
}

const char* CRendererEvent::GetEventName() const
{
    const char* pRet = NULL;

    if (m_pEventNameStr)
    {
        pRet = (const char*) m_pEventNameStr->GetBuffer();
    }

    return pRet;
}

#endif /* #if defined(HELIX_FEATURE_EVENTMANAGER) */

/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: pfeventproxy.cpp,v 1.1 2006/03/27 23:49:03 ehodge Exp $
 * 
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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

#if defined(HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION)  &&  defined(HELIX_FEATURE_EVENTMANAGER)
// include
#include <string.h>
#include "hxtypes.h"
#include "hxcom.h"
#include "hxresult.h"
#include "hxcomm.h"
#include "hxinter.h"
#include "hxpfs.h"
// rmacore
#include "eventmgr.h"
#include "pfeventproxy.h"
// pndebug
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE             
static const char HX_THIS_FILE[] = __FILE__;
#endif

CPresentationFeatureEventProxy::CPresentationFeatureEventProxy(
        IHXPresentationFeatureSink* pPresentationFeatureSink,
        IHXPresentationFeatureManager* pPresentationFeatureManager)
        : m_lRefCount(0)
        , m_pPresentationFeatureSink(NULL)
        , m_pPresentationFeatureManager(NULL)
{
    m_pPresentationFeatureSink = pPresentationFeatureSink;
    m_pPresentationFeatureManager = pPresentationFeatureManager;
    HX_ADDREF(m_pPresentationFeatureSink);
    HX_ADDREF(m_pPresentationFeatureManager);
}

CPresentationFeatureEventProxy::~CPresentationFeatureEventProxy()
{
    Close();
}


/////////////////////////////////////////////
//  IUnknown methods:

STDMETHODIMP CPresentationFeatureEventProxy::QueryInterface(REFIID riid, void** ppvObj)
{
    HX_RESULT retVal = HXR_OK;

    if (ppvObj)
    {
        QInterfaceList qiList[] =
            {
                  { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXEventSink*)this }
                , { GET_IIDHANDLE(IID_IHXEventSink), (IHXEventSink*)this }
            };
        
        retVal = ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
    }
    else
    {
        retVal = HXR_FAIL;
    }

    return retVal;
}

STDMETHODIMP_(ULONG32) CPresentationFeatureEventProxy::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CPresentationFeatureEventProxy::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;

    return 0;
}


/////////////////////////////////////////////
//  IHXEventSink methods:

STDMETHODIMP CPresentationFeatureEventProxy::EventFired(IHXBuffer* pURLStr,
                                               IHXBuffer* pFragmentStr,
                                               IHXBuffer* pEventNameStr,
                                               IHXValues* pOtherValues)
{
    HX_RESULT hxr = HXR_OK;

    //  P.F.Sinks only care about pEventNameStr.  Size must be big enough to
    // hold both the prefix as well as the '.' separator and so on:
    if (pEventNameStr  &&  pEventNameStr->GetSize() >
            PRESENTATION_FEATURE_SELECTION_EVENT_MIN_LEN)
    {
        HX_ADDREF(pEventNameStr);

        const char* pszEventName = (const char*)pEventNameStr->GetBuffer();
        if (pszEventName  &&  0 == strncmp(pszEventName,
                PRESENTATION_FEATURE_SELECTION_EVENT_PREFIX_WITH_SEPARATOR,
                PRESENTATION_FEATURE_SELECTION_EVENT_LEN_OF_PREFIX_WITH_SEPARATOR))
        {
            //  This is a PF event so pass it on to the P.F.Sink:
            HX_ASSERT(m_pPresentationFeatureSink);
            HX_ASSERT(m_pPresentationFeatureManager);
            if (!m_pPresentationFeatureSink  ||  !m_pPresentationFeatureManager)
            {
                hxr = HXR_UNEXPECTED;
            }
            else
            {
                PfChangeEventType thePfChangeEvent = PF_EVENT_INVALID_EVENT;
                IHXBuffer* pParsedFeatureName = NULL;
                IHXBuffer* pParsedFeatureCurrentSetting = NULL;
                if (HXR_OK == m_pPresentationFeatureManager->ParsePFEvent(
                        pszEventName, /*OUT*/ thePfChangeEvent,
                        /*OUT*/ pParsedFeatureName,
                        /*OUT*/ pParsedFeatureCurrentSetting)  &&
                        pParsedFeatureName)
                {
                    const char* pszFeatureName = (const char*)
                            pParsedFeatureName->GetBuffer();
                    switch (thePfChangeEvent)
                    {
                      case PF_EVENT_PF_CURRENT_VALUE_CHANGED:
                        {
                            //  We could just pass the cur value passed to us
                            // from ParsePFEvent(), above, but we should pass
                            // the truly-current value in case it doesn't match
                            // or got changed in the mean time:
                            IHXBuffer* pFeatureCurrentSetting = NULL;
                            m_pPresentationFeatureManager->
                                    GetPresentationFeatureValue(pszFeatureName,
                                    pFeatureCurrentSetting);

                            //  Remove this assert if it appears to only happen
                            // when nearly-simultaneous cur-value changes occur
                            HX_ASSERT(!strcmp(
                                    (const char*)pFeatureCurrentSetting->GetBuffer(),
                                    (const char*)pParsedFeatureCurrentSetting->GetBuffer()) );

                            m_pPresentationFeatureSink->
                                    PresentationFeatureCurrentSettingChanged(
                                    pszFeatureName, pFeatureCurrentSetting);

                            HX_RELEASE(pFeatureCurrentSetting);
                        }
                        break;

                      case PF_EVENT_PF_FEATURE_CHANGED:
                      case PF_EVENT_PF_REMOVED:
                        {
                            m_pPresentationFeatureSink->
                                    PresentationFeatureChanged(pszFeatureName);
                        }
                        break;
                      default:
                        HX_ASSERT(0);
                        break;
                    } //  END of: switch (thePfChangeEvent).
                }

                HX_RELEASE(pParsedFeatureName);
                HX_RELEASE(pParsedFeatureCurrentSetting);
            }
        }

    }
    else
    {
        hxr = HXR_INVALID_PARAMETER;
    }

    HX_RELEASE(pEventNameStr);

    return hxr;
}

/////////////////////////////////////////////
//  CPresentationFeatureEventProxy methods:

STDMETHODIMP CPresentationFeatureEventProxy::Close()
{
    HX_RESULT retVal = HXR_OK;

    HX_RELEASE(m_pPresentationFeatureSink);
    HX_RELEASE(m_pPresentationFeatureManager);

    return retVal;
}


#endif // if defined(HELIX_FEATURE_PRESENTATION_FEATURE_SELECTION)  &&  defined(HELIX_FEATURE_EVENTMANAGER)

/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdrmcore.cpp,v 1.2 2007/04/05 21:56:15 sfu Exp $
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

#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxformt.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "hxpref.h"
#include "hxclsnk.h"
#include "hxpends.h"
#include "hxhyper.h"
#include "playhpnv.h"
#include "hxmon.h"
#include "pckunpck.h"
#include "hxslist.h"
#include "hxmap.h"
#include "hxstrutl.h"
#include "hxrquest.h"
#include "chxelst.h"
#include "hxbsrc.h"
#include "hxsrc.h"
#include "hxflsrc.h"
#include "hxntsrc.h"
#include "hxrendr.h"
#include "hxwin.h"
#include "hxstrm.h"
#include "hxcleng.h"
#include "timeline.h"
#include "hxstring.h"
#include "timeval.h"
#include "hxerror.h"
#include "sinkctl.h"
#include "upgrdcol.h"
#include "chxphook.h"
#include "hxgroup.h"
#include "basgroup.h"
#include "advgroup.h"
#include "hxplayvelocitycaps.h"
#include "hxtlogutil.h"
#include "chxeven.h"
#include "chxelst.h"
#include "strminfo.h"
#include "srcinfo.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE             
static const char HX_THIS_FILE[] = __FILE__;
#endif

#include "hxdrmcore.h"

HXDRM::HXDRM(HXSource* pSource) :
      m_lRefCount(0)
    , m_pSource(pSource)
    , m_pDigitalRightsManager(NULL)
    , m_pPlayer(NULL)
{
    m_pSource->AddRef();

    HX_VERIFY(SUCCEEDED(m_pSource->GetPlayer(m_pPlayer)));
    m_pPlayer->AddRef();
}

HXDRM::~HXDRM()
{
    CHXSimpleList::Iterator lIter = m_HXEventList.Begin();
    for (; lIter != m_HXEventList.End(); ++lIter)
    {
        CHXEvent* pEvent = (CHXEvent*) (*lIter);
        HX_DELETE(pEvent);
    }

    m_HXEventList.RemoveAll();    

    HX_RELEASE(m_pDigitalRightsManager);
    HX_RELEASE(m_pSource);
    HX_RELEASE(m_pPlayer);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::QueryInterface
//      Purpose:
//              Implement this to export the interfaces supported by your 
//              object.
//
STDMETHODIMP HXDRM::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXSourceInput), (IHXSourceInput*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXSourceInput*)this },
        };
    
    HX_RESULT res = ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
    
    // if successful, return immediately...
    if (SUCCEEDED(res))
    {
        return res;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::AddRef
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) HXDRM::AddRef()
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
STDMETHODIMP_(ULONG32) HXDRM::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT HXDRM::InitDRMPlugin(IHXValues* pHeader)
{
    HX_RESULT theErr = HXR_OK;
    if (!pHeader)
    {
	    return theErr;
    }

    // get the DRM Id from file header
    IHXBuffer* pShortName = 0;
    pHeader->GetPropertyCString(HXDRM_NAME_DRMID, pShortName);

    // plugin handler
    IHXPlugin2Handler* pPluginHandler = NULL;
    m_pPlayer->QueryInterface(IID_IHXPlugin2Handler, (void**)&pPluginHandler);

    theErr = HXR_FILE_NOT_FOUND;
    if (pPluginHandler && pShortName)
    {
        // load the DRM plugin with matching Id
	IUnknown* pIUnk = 0;
	pPluginHandler->FindPluginUsingStrings(
                                PLUGIN_CLASS, PLUGIN_SOURCEHANDLER_TYPE,
                                SOURCEHANDLER_TYPE, SOURCEHANDLER_DRM_TYPE,
	                        HXDRM_NAME_DRMID, (char*)pShortName->GetBuffer(), 
                                pIUnk);
	
        if (pIUnk)
	{
            //use source object as context
	    IUnknown* pContext = NULL;
	    theErr = m_pSource->QueryInterface(IID_IUnknown, (void**) &pContext);

	    // init the plugin
            if (SUCCEEDED(theErr))
            {
	        IHXPlugin* pPlugin = NULL;
	        theErr = pIUnk->QueryInterface(IID_IHXPlugin, (void**)&pPlugin);
	        if (SUCCEEDED(theErr) && pPlugin && pContext)
	        {
		        theErr = pPlugin->InitPlugin(pContext);
	        }
	        HX_RELEASE(pPlugin);
            }

            // init the source handler
            if (SUCCEEDED(theErr))
            {
                IHXSourceHandler* pSourceHandler = NULL;
	        theErr = pIUnk->QueryInterface(IID_IHXSourceHandler, (void**)&pSourceHandler);
	        if (SUCCEEDED(theErr) && pSourceHandler)
	        {
		        theErr = pSourceHandler->InitSourceHandler((IHXSourceInput*)this, NULL);

                        m_pDigitalRightsManager = pSourceHandler;
                        m_pDigitalRightsManager->AddRef();
	        }
	        HX_RELEASE(pSourceHandler);
            }

            HX_RELEASE(pIUnk);
        }
        else
        {
            //couldn't find the DRM plugin; setup AU
#if defined(HELIX_FEATURE_AUTOUPGRADE)
	    IHXUpgradeCollection* pUpgradeCollection = NULL;
	    m_pPlayer->QueryInterface(IID_IHXUpgradeCollection, (void**) &pUpgradeCollection);
	    pUpgradeCollection->Add(eUT_Required, pShortName, 0, 0);
	    pUpgradeCollection->Release();
	    theErr = HXR_REQUEST_UPGRADE;
#endif /* HELIX_FEATURE_AUTOUPGRADE */
        }
    }

    HX_RELEASE(pShortName);
    HX_RELEASE(pPluginHandler);

    return theErr;
}

// process the file header
HX_RESULT HXDRM::ProcessFileHeader(IHXValues* pHeader)
{
    if (m_pDigitalRightsManager)
    {
        return m_pDigitalRightsManager->OnFileHeader(HXR_OK, pHeader);
    }
    return HXR_OK;
}

// process the file header
HX_RESULT HXDRM::ProcessStreamHeader(IHXValues* pHeader)
{
    if (m_pDigitalRightsManager)
    {
        return m_pDigitalRightsManager->OnStreamHeader(HXR_OK, pHeader);
    }
    return HXR_OK;
}

// decrypt then send the packet to renderer
HX_RESULT HXDRM::ProcessEvent(CHXEvent* pEvent)
{
    if (m_pDigitalRightsManager)
    {
        //make a copy of the event object
	CHXEvent* pTempEvent = new CHXEvent(NULL); // don't really need the Packet pointer
	pTempEvent->SetTimeOffset(pEvent->GetTimeOffset());
	pTempEvent->m_pRendererInfo = pEvent->m_pRendererInfo;

        // add event to the pending list
        m_HXEventList.AddTail(pTempEvent);

        // ask DRM plugin to decrypt the packet
        return m_pDigitalRightsManager->OnPacket(HXR_OK, pEvent->GetPacket());
    }
    return HXR_OK;
}

// flush packets
HX_RESULT HXDRM::FlushPackets(HXBOOL bPushOutToCaller)
{
    if (m_pDigitalRightsManager)
    {
        if (bPushOutToCaller)
        {
            //ask DRM plugin to process the pending packets immediately
            m_pDigitalRightsManager->OnPacket(HXR_OK, NULL);
        }
        else
        {
            //ask DRM plugin to discard pending packets
            m_pDigitalRightsManager->OnPacket(HXR_ABORT, NULL);
        }

        //clean up the pending event list
        CHXSimpleList::Iterator lIter = m_HXEventList.Begin();
        for (; lIter != m_HXEventList.End(); ++lIter)
        {
            CHXEvent* pEvent = (CHXEvent*) (*lIter);
            HX_DELETE(pEvent);
        }
        m_HXEventList.RemoveAll();    
    }

    return HXR_OK;
}


HXBOOL HXDRM::IsProtected(IHXValues* pHeader)
{
    if (!pHeader)
    {
        return FALSE;
    }

    ULONG32 bIsProtected = 0;
    pHeader->GetPropertyULONG32(HXDRM_NAME_ISPROTECTED, bIsProtected);
    return bIsProtected?TRUE:FALSE;
}

HX_RESULT HXDRM::OnFileHeader(HX_RESULT status, IHXValues* pValues)
{
    //file header modified by DRM, forward to source
    HX_ASSERT(m_pSource);
    m_pSource->OnFileHeader(status, pValues);
    return HXR_OK;
}

HX_RESULT HXDRM::OnStreamHeader(HX_RESULT status, IHXValues* pValues)
{
    //stream header modified by DRM, forward to source
    HX_ASSERT(m_pSource);
    m_pSource->OnStreamHeader(status, pValues);
    return HXR_OK;
}

HX_RESULT HXDRM::OnStreamDone(HX_RESULT status, UINT16 unStreamNumber)
{
    //forward to source
    HX_ASSERT(m_pSource);
    m_pSource->OnStreamDone(status, unStreamNumber);
    return HXR_OK;
}

HX_RESULT HXDRM::OnPacket(HX_RESULT status, IHXPacket* pPacket)
{
    //decrypted packet, send to renderer
    if (SUCCEEDED(status) && pPacket)
    {
        //this assumes a 1-1 mapping between packets going into and out of DRM plugin
        CHXEvent* pEvent = (CHXEvent*)m_HXEventList.RemoveHead();
        if (pEvent)
        {
            RendererInfo* pRendInfo = NULL;
            IHXRenderer*  pRend = NULL;
    
            pRendInfo = pEvent->m_pRendererInfo;
            if (pRendInfo)
            {
                pRend = pRendInfo->m_pRenderer;
            }
            if (pRend)
            {
               status = pRend->OnPacket(pPacket, pEvent->GetTimeOffset());
            }

            HX_DELETE(pEvent);
        }
    }
    return HXR_OK;
}

HX_RESULT HXDRM::OnTermination(HX_RESULT status)
{
    //DRM error
    HX_ASSERT(m_pSource);
    m_pSource->OnTermination(status);
    return HXR_OK;
}

//pre-recording hooks
HX_RESULT HXDRM::FileHeaderHook(IHXValues* pHeader)
{
    if (m_pDigitalRightsManager)
    {
        IHXDRMPreRecordingHook* pPreRecordingHook = NULL;
        m_pDigitalRightsManager->QueryInterface(IID_IHXDRMPreRecordingHook, (void**)&pPreRecordingHook);
        if (pPreRecordingHook)
        {
            pPreRecordingHook->FileHeaderHook(pHeader);
        }
        HX_RELEASE(pPreRecordingHook);
    }
    return HXR_OK;
}

HX_RESULT HXDRM::StreamHeaderHook(IHXValues* pHeader)
{
    if (m_pDigitalRightsManager)
    {
        IHXDRMPreRecordingHook* pPreRecordingHook = NULL;
        m_pDigitalRightsManager->QueryInterface(IID_IHXDRMPreRecordingHook, (void**)&pPreRecordingHook);
        if (pPreRecordingHook)
        {
            pPreRecordingHook->StreamHeaderHook(pHeader);
        }
        HX_RELEASE(pPreRecordingHook);
    }
    return HXR_OK;
}

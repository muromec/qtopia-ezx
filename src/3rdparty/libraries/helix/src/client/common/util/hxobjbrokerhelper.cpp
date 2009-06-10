/* ***** BEGIN LICENSE BLOCK *****
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

#include "hxobjbrokerhelper.h"
#include "dllpath.h"
#include "hxccf.h"
#include "hxassert.h"
#include "hxperf.h"
#include "ihxmedpltfm.h"
#include "ihxcontext.h"
#include "ihxobjectmanager.h"
#include "hxobjbrokrids.h"
#include "hxapplicationclassfactory.h"

HX_IMPLEMENT_SINGLETON (HXObjBrokerHelper, CHXObjBrokerWrapper_);

CHXObjBrokerWrapper_::CHXObjBrokerWrapper_ ()
                     :m_pLibObjBroker (NULL)
                     ,m_pRootContext (NULL)
                     ,m_ulRefCount(0)
{
}
        
CHXObjBrokerWrapper_::~CHXObjBrokerWrapper_ ()
{
    ReleaseObjectBroker_ ();
}

HX_RESULT CHXObjBrokerWrapper_::InitObjectBroker (IUnknown** const ppContext)
{
    HX_LOG_BLOCK( "CHXObjBrokerWrapper_::InitObjectBroker" );

    HX_RESULT res = HXR_FAILED;

    if (m_spGenericContext.IsValid())
    {
        m_spGenericContext.AsUnknown(ppContext);
        res = HXR_OK;
    }
    else
    {
        res = AcquireObjectBroker_ ();
        if (FAILED (res)) return res;

        SPIHXMediaPlatform spMediaPlatform;
        FPHXCREATEMEDIAPLATFORM pfCreatePlatform = (FPHXCREATEMEDIAPLATFORM) m_pLibObjBroker->getSymbol("HXCreateMediaPlatformEx");
        if (pfCreatePlatform)
        {
	    res = pfCreatePlatform(spMediaPlatform.AsInOutParam());
	    m_pRootContext = spMediaPlatform.Ptr();
        }

        if (FAILED (res)) return res;

        SPIHXCommonClassFactory spCCF(spMediaPlatform.Ptr());
        if( spCCF.IsValid() )
        {
            m_spGenericContext = SPIHXContext(spCCF.Ptr(), CLSID_HXGenericContext);
	    if(m_spGenericContext.IsValid())
 	    {
	        res = m_spGenericContext->AddObjectToContext( CLSID_HXObjectManager );
	        m_spGenericContext.AsUnknown(ppContext);
            }

            if (SUCCEEDED(res))
            {
                HXApplicationClassFactory::Init(spCCF.Ptr());
            }
        }
    }
   
    if (SUCCEEDED(res))
    {
        InterlockedIncrement(&m_ulRefCount);
    }

    return res;
}

HX_RESULT CHXObjBrokerWrapper_::TerminateObjectBroker ()
{
    if (m_ulRefCount > 0)
    {
        InterlockedDecrement(&m_ulRefCount);
    
        if (0 == m_ulRefCount)
        {
            m_spGenericContext.Clear();
            HXApplicationClassFactory::Terminate();

            FPHXMEDIAPLATFORMCLOSE pCloseFunc = (FPHXMEDIAPLATFORMCLOSE) m_pLibObjBroker->getSymbol("HXMediaPlatformCloseEx");
            if ( pCloseFunc )
            {
                pCloseFunc();
            }

            m_pRootContext = NULL;
            ReleaseObjectBroker_ ();
        }
    }

    return HXR_OK;
}

HX_RESULT CHXObjBrokerWrapper_::CreateRootContext_ ()
{
    if (m_pRootContext) return HXR_OK;
    
    HX_RESULT res = HXR_OK;

    res = AcquireObjectBroker_ ();
    if (FAILED (res)) return res;

    FPHXCREATEMEDIAPLATFORM  pGetRootFunc = (FPHXCREATEMEDIAPLATFORM ) m_pLibObjBroker->getSymbol("HXCreateMediaPlatformEx");
    if (pGetRootFunc) 
    {
	SPIHXMediaPlatform spMediaPlatform;
	res = pGetRootFunc(spMediaPlatform.AsInOutParam());
	if(SUCCEEDED(res))
	{
	    m_pRootContext = spMediaPlatform.Ptr(); 
	}
    }
    
    HX_ASSERT (m_pRootContext);
    return m_pRootContext ? HXR_OK : HXR_FAIL;
}


HX_RESULT CHXObjBrokerWrapper_::SetRootContext(IUnknown* pIContext)
{
    HX_ASSERT( m_pRootContext == NULL );

    m_pRootContext = pIContext;
    return HXR_OK;
}

HX_RESULT CHXObjBrokerWrapper_::GetRootFactory (IHXCommonClassFactory** const ppContext)
{
    HX_ASSERT (ppContext && !*ppContext);
    if (!ppContext || *ppContext) return HXR_INVALID_PARAMETER;

    HX_RESULT const res = CreateRootContext_ ();
    if (FAILED (res)) return res;
    
    return m_pRootContext->QueryInterface (IID_IHXCommonClassFactory, (void**) ppContext);
}


HX_RESULT CHXObjBrokerWrapper_::AcquireObjectBroker_ ()
{
    if (m_pLibObjBroker) return HXR_OK;
    
    HX_LOG_BLOCK( "CHXObjBrokerWrapper_::AcquireObjectBroker_...Open Objbrokr" );

    m_pLibObjBroker = new DLLAccess; 
    REQUIRE_RETURN (m_pLibObjBroker, HXR_OUTOFMEMORY);
    
    if (DLLAccess::DLL_OK != m_pLibObjBroker->open( HX_MEDIA_PLATFORM_DLLNAME, DLLTYPE_OBJBROKR )) 
    {
	HX_ASSERT (!"Failed to open object broker DLL.");
	return HXR_FAIL;
    }

    return HXR_OK;
}

HX_RESULT CHXObjBrokerWrapper_::ReleaseObjectBroker_ ()
{
    if (!m_pLibObjBroker) return HXR_OK;

    HX_RESULT const res = m_pLibObjBroker->close ();
    HX_DELETE (m_pLibObjBroker);

    return res;
}


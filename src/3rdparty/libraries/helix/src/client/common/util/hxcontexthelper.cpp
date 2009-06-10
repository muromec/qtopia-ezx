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

#include "hxcontexthelper.h"
#include "ihxcontext.h"
#include "hxobjbrokrids.h"
#include "ihxobjectmanager.h"



CHXContextHelper::CHXContextHelper (IUnknown* const pIContext)
    : m_spContext (pIContext)
{
}

CHXContextHelper::CHXContextHelper (IHXCommonClassFactory* const pIContext)
    : m_spContext (pIContext)
{
}

CHXContextHelper::~CHXContextHelper ()
{
}

HXBOOL CHXContextHelper::IsValid () const
{
    return m_spContext.IsValid ();
}

HX_RESULT CHXContextHelper::GetRootFactory (IHXCommonClassFactory** const ppICommonClassFactory)
{
    return m_hObjBrokerHelper->GetRootFactory (ppICommonClassFactory);
}

void CHXContextHelper::AsUnknown (IUnknown** const pIUnk)
{
    m_spContext.AsUnknown (pIUnk);
}

IHXCommonClassFactory* const CHXContextHelper::Ptr () const
{
    return m_spContext.Ptr ();
}

void CHXContextHelper::AsPtr( IHXCommonClassFactory **ppICommonClassFactory ) const
{
    m_spContext.AsPtr( ppICommonClassFactory );
}

HXBOOL CHXContextHelper::Set (IUnknown* const pIContext)
{
    m_spContext = pIContext;
    return m_spContext.IsValid ();
}

HXBOOL CHXContextHelper::Set (IHXCommonClassFactory* const pIContext)
{
    m_spContext = pIContext;
    return m_spContext.IsValid ();
}

HX_RESULT CHXContextHelper::CreateTypedInstance (REFCLSID clsid, REFIID iid, void** const ppResult, IUnknown* const pIUnkOuter)
{
    PRE_REQUIRE_RETURN (ppResult, HXR_INVALID_PARAMETER);
    PRE_REQUIRE_RETURN (CreateContext_ (), HXR_UNEXPECTED);
    
    *ppResult = NULL;
    IUnknown* pIUnk = NULL;
    HX_RESULT res = m_spContext->CreateInstanceAggregatable (clsid, pIUnk, pIUnkOuter);
    REQUIRE_RETURN_QUIET (SUCCEEDED (res), res);
    
    res = pIUnk->QueryInterface (iid, ppResult);
    HX_RELEASE (pIUnk);
    
    return res;
}

HX_RESULT CHXContextHelper::CreateStandardContext (IUnknown** const ppINewContext)
{
    PRE_REQUIRE_RETURN (ppINewContext, HXR_INVALID_PARAMETER);
    PRE_REQUIRE_RETURN (CreateContext_ (), HXR_UNEXPECTED);
    
    *ppINewContext = NULL;

    HX_RESULT const res = CreateTypedInstance (CLSID_HXGenericContext, IID_IUnknown, (void**) ppINewContext);
    if (FAILED (res)) return res;

    SPIHXContext spContext = *ppINewContext;
    HX_VERIFY (spContext.IsValid());
    if (!spContext.IsValid ())
    {
	HX_RELEASE (*ppINewContext);
	return HXR_FAIL;
    }

    return spContext->AddObjectToContext (CLSID_HXObjectManager);
}

HXBOOL CHXContextHelper::CreateContext_ ()
{
    if (m_spContext.IsValid ()) return TRUE;

    IHXCommonClassFactory* pICCF = NULL;
    m_hObjBrokerHelper->GetRootFactory (&pICCF);
    HXBOOL const res = Set (pICCF);
    HX_RELEASE (pICCF);
    
    POST_REQUIRE_RETURN (m_spContext.IsValid (), FALSE);
    return res;
}

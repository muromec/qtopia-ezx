/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxgenericcontext.cpp,v 1.1 2006/03/22 17:51:59 stanb Exp $
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


#include "hxgenericcontext.h"

#include "hxheap.h"
#include "hxassert.h"


// IUnknown implementation
IMPLEMENT_UNKNOWN_NOINTERFACE( CRNGenericContext )
IMPLEMENT_COM_CREATE_FUNCS( CRNGenericContext )



CRNGenericContext::CRNGenericContext( void ) :
    m_pFactory( NULL )
{
}

// virtual 
CRNGenericContext::~CRNGenericContext( void )
{
    // Free list of inner unknowns
    while( m_InnerUnks.GetCount() )
    {
	IUnknown* pInner = (IUnknown*) m_InnerUnks.RemoveHead();
	HX_RELEASE( pInner );
    }

    HX_RELEASE( m_pFactory );
}


STDMETHODIMP
CRNGenericContext::AddObjectToContext( REFCLSID clsid )
{
    HX_RESULT result = HXR_FAIL;

    CUnknownIMP* pIThisAsUnknown = this;
    IUnknown* pNewInner = NULL;
    if( m_pFactory )
    {
	result = m_pFactory->CreateInstanceAggregatable( clsid, pNewInner, pIThisAsUnknown );
    }
    if( SUCCEEDED( result ) )
    {
	m_InnerUnks.AddTail( (void*) pNewInner );
    }

    return result;
}


STDMETHODIMP
CRNGenericContext::RegisterContext( IUnknown *pSource )
{
    HX_ASSERT( pSource != NULL );

    HX_RESULT result = pSource->QueryInterface( IID_IHXCommonClassFactory, (void**) &m_pFactory );

    HX_ASSERT( m_pFactory != NULL );

    return result;
}

STDMETHODIMP
CRNGenericContext::GetService( REFIID iid, void** ppIService )
{
    HX_ASSERT( ppIService );
    
    HX_RESULT result = HXR_FAIL;
    if ( ppIService )
    {
	*ppIService = NULL;

	result = QueryInterface( iid, ppIService );
	if( FAILED( result ) )
	{
	    // Look in the parent context
	    SPIHXContext spParentContext = m_pFactory;

	    if ( spParentContext.IsValid() )
	    {
		result = spParentContext->GetService( iid, ppIService );
	    }
	}
    }
    return result;
}

HX_RESULT CRNGenericContext::_ActualQI( THIS_ REFIID riid, void** ppvObj )
{
    // Check for valid pointer and initialize it
    if (!ppvObj)
	return HXR_POINTER;
    *ppvObj = NULL;												\

    if( IsEqualIID( IID_IUnknown, riid ) )
    {
	    AddRef();
	    CUnknownIMP *pInterface = this;
	    *ppvObj = pInterface;
	    return HXR_OK;
    }

    if( IsEqualIID( IID_IHXContext, riid ) )
    {
	    AddRef();
	    IHXContext *pInterface = this;
	    *ppvObj = pInterface;
	    return HXR_OK;
    }

    if( IsEqualIID( IID_IHXContextUser, riid ) )
    {
	    AddRef();
	    IHXContextUser *pInterface = this;
	    *ppvObj = pInterface;
	    return HXR_OK;
    }

    // Iterate through inner unknowns and ask them
    LISTPOSITION listPos = m_InnerUnks.GetHeadPosition();
    while( listPos )
    {
	IUnknown* pInner = (IUnknown*) m_InnerUnks.GetNext( listPos );
	if( SUCCEEDED( pInner->QueryInterface( riid, ppvObj ) ) )
	{
	    return HXR_OK;
	}
    }

    // Nope, couldn't find it
    return HXR_NOINTERFACE;
}

HX_RESULT
CRNGenericContext::GetObjectSource( IHXCommonClassFactory** ppIObjectSource ) const
{
    HX_ASSERT( ppIObjectSource );
    HX_ASSERT( m_pFactory );
    
    HX_RESULT result = HXR_FAIL;
    *ppIObjectSource = m_pFactory;
    if ( *ppIObjectSource )
    {
	( *ppIObjectSource )->AddRef();
	result = HXR_OK;
    }
    return result;
}

#ifdef ENABLE_LOG_STREAMS
ostream& operator <<( ostream& o, const CRNGenericContext& )
{
    o << "CRNGenericContext";
    return o;
}
#endif



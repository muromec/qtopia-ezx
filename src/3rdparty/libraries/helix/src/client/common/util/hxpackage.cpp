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

#include "hxpackage.h"
#include "hxcontexthelper.h"

#include "hxheap.h"
#include "hxassert.h"

// TEMP  FIX
// #include "rnguid.h"
#include "chxpckts.h"
#include "hxbuffer.h"

#include "ihxtimer.h"
#include "ihxobservable.h"
#include "hxobjbrokrids.h"
#include "hxplugn.h"

// IUnknown implementation
BEGIN_COMPONENT_INTERFACE_LIST (CHXPackage)
    INTERFACE_LIST_ENTRY_SIMPLE (IHXPlugin)
    INTERFACE_LIST_ENTRY_SIMPLE (IHXComponentPlugin)
    INTERFACE_LIST_ENTRY_SIMPLE (IHXPluginNamespace)
END_INTERFACE_LIST

IMPLEMENT_ABSTRACT_COMPONENT (CHXPackage)

CHXPackage::CHXPackage(
	CHXString const& module,
	SComponentInfoListEntry* pComponentInfo,
	const char* pNamespace, UINT32 const interval) 
    : m_numComponents( 0 )
    , m_pCCF( NULL )
    , m_pNamespace(pNamespace)
    , m_TimerInterval (interval)
    , m_LiveComponents (0)
    , m_Module (module)
{
    // Count number of components, add appropriate entries to the CLSID->function list
    SComponentInfoListEntry* pEntry = pComponentInfo;
    if( pEntry )
    {
	m_numComponents = 0;

	while( pEntry->m_type != kInfo_EndTable )
	{
	    if( pEntry->m_type == kInfo_BeginEntry )
	    {
		m_creationInfo.SetAt( *(GUID*)pEntry->m_pData1, (void*) pEntry->m_pFunc );
		m_indexedInfo.SetAt( m_numComponents++, pEntry );
	    }

	    pEntry++;
	}
    }
}

// virtual 
CHXPackage::~CHXPackage( void )
{
    if (m_spPackageUnloader.IsValid())
    {
	m_spPackageUnloader->Destruct();
    }
    HX_RELEASE(m_pCCF);
}

void CHXPackage::OnComponentBirth ()
{
    if (1 == ++m_LiveComponents && m_spTimer.IsValid ())
    {
    	m_spTimer->StopTimer ();
    }
}

void CHXPackage::OnComponentDeath ()
{
    if (0 == --m_LiveComponents && m_spTimer.IsValid ())
    {
	m_spTimer->StartTimer ();
    }
}

void CHXPackage::CancelTimer ()
{ 
    ENFORCE_NOTIFY (m_spTimer.IsValid (), HX_EMPTY); 
    m_spTimer->StopTimer ();
}

STDMETHODIMP
CHXPackage::InitPlugin( IUnknown* pContext )
{
    ENFORCE_SUCCESS (pContext->QueryInterface( IID_IHXCommonClassFactory, (void**) &m_pCCF ));
    CHXContextHelper contextHelper = pContext;
    ENFORCE_SUCCESS (contextHelper.CreateTypedInstance (CLSID_HXTimer, IID_IHXTimer, (void**) m_spTimer.AsInOutParam ()));

    ENFORCE_SUCCESS (contextHelper.CreateTypedInstance (CLSID_HXPackageUnloader, IID_IHXPackageUnloader, (void**) m_spPackageUnloader.AsInOutParam ()));

    m_spPackageUnloader->Construct (m_spTimer.Ptr (), m_Module);
    m_spTimer->SetFiniteTimerIntervalNumber (m_TimerInterval, 1);

    return HXR_OK;
}

    
STDMETHODIMP
CHXPackage::CreateComponentInstance( REFCLSID rclsid, REF(IUnknown*) ppUnknown, IUnknown* pUnkOuter)
{
    HX_RESULT result = HXR_FAIL;

    void* pData = NULL;
    if( m_creationInfo.Lookup( rclsid, pData ) )
    {
	CREATEINSTANCEFUNC CreateManagedObject = (CREATEINSTANCEFUNC) pData;
	result = CreateManagedObject (*this, pUnkOuter, &ppUnknown);
    }

    return result;
}

STDMETHODIMP_(UINT32)
CHXPackage::GetNumComponents()
{
    return m_numComponents;
}

STDMETHODIMP_(char const*)
CHXPackage::GetPackageName () const
{
    return m_Module;
}

STDMETHODIMP
CHXPackage::GetComponentInfoAtIndex( UINT32 nIndex, REF(IHXValues*) pInfo )
{
    HX_RESULT result = HXR_FAIL;

    // Only bother to do this if we have a valid factory
    if( m_pCCF )
    {
	// Find the appropriate list entry
	void* pData = NULL;
	if( m_indexedInfo.Lookup( nIndex, pData ) )
	{
	    SComponentInfoListEntry* pEntry = (SComponentInfoListEntry*) pData;

	    // If we found one, update pInfo with the entries following
	    IUnknown* pIUnk = NULL;
	    if( SUCCEEDED( m_pCCF->CreateInstance( CLSID_IHXValues, (void**) &pIUnk ) ) )
	    {
		HX_VERIFY( SUCCEEDED( pIUnk->QueryInterface( IID_IHXValues, (void**) &pInfo ) ) );
		HX_RELEASE( pIUnk );

		// Handle the table begin--it contains the clsid
		AddBufferProperty_ ( pInfo, PLUGIN_COMPONENT_CLSID, (const unsigned char*) pEntry->m_pData1, sizeof(GUID) );
		pEntry++;
	    
		while( pEntry->m_type != kInfo_EndTable && pEntry->m_type != kInfo_BeginEntry )
		{
		    switch( pEntry->m_type )
		    {
			case kInfo_IntType:
			    pInfo->SetPropertyULONG32( pEntry->m_pName, (UINT32) pEntry->m_pData1 );
			    break;
	    
			case kInfo_StringType:
			    AddStringProperty_ ( pInfo, pEntry->m_pName, (const char*) pEntry->m_pData1 );
			    break;
	    
			case kInfo_BufferType:
			    AddBufferProperty_ ( pInfo, pEntry->m_pName, (const unsigned char*) pEntry->m_pData1, pEntry->m_dataSz );
			    break;
		    }
		    pEntry++;
		}
		result = HXR_OK;
	    }
	}
    }
    
    return result;
}

STDMETHODIMP
CHXPackage::GetPluginNamespace(REF(IHXBuffer*) pBuffer)
{
    HX_RESULT res = HXR_FAIL;
    IUnknown* pIUnk = NULL;
    if( SUCCEEDED( m_pCCF->CreateInstance( CLSID_IHXBuffer, (void**) &pIUnk ) ) )
    {
	HX_VERIFY( SUCCEEDED( pIUnk->QueryInterface( IID_IHXBuffer, (void**) &pBuffer ) ) );

	// NOTE:  We have to do this here, because the Set fails if the ref count is above 1
	HX_RELEASE( pIUnk );

	if (!SUCCEEDED(res = pBuffer->Set( (BYTE*)m_pNamespace, ::strlen( m_pNamespace ) + 1 )))
	{
		// if we fail to set the string then release the buffer
		HX_RELEASE( pBuffer );
	}
    }
    return res;
}

void CHXPackage::AddStringProperty_ ( IHXValues* pValues, const char* pName, const char* pValue )
{
    IUnknown* pIUnk = NULL;
    if( SUCCEEDED( m_pCCF->CreateInstance( CLSID_IHXBuffer, (void**) &pIUnk ) ) )
    {
	IHXBuffer* pBuffer = NULL;
	HX_VERIFY( SUCCEEDED( pIUnk->QueryInterface( IID_IHXBuffer, (void**) &pBuffer ) ) );

	// NOTE:  We have to do this here, because the Set fails if the ref count is above 1
	HX_RELEASE( pIUnk );

	pBuffer->Set( (const unsigned char *) pValue, ::strlen( pValue ) + 1 );
	pValues->SetPropertyCString( pName, pBuffer );

	HX_RELEASE( pBuffer );
    }
}


void CHXPackage::AddBufferProperty_ ( IHXValues* pValues, const char* pName, const unsigned char* pValue, UINT32 dataLen )
{
    IUnknown* pIUnk = NULL;
    if( SUCCEEDED( m_pCCF->CreateInstance( CLSID_IHXBuffer, (void**) &pIUnk ) ) )
    {
	IHXBuffer* pBuffer = NULL;
	HX_VERIFY( SUCCEEDED( pIUnk->QueryInterface( IID_IHXBuffer, (void**) &pBuffer ) ) );

	// NOTE:  We have to do this here, because the Set fails if the ref count is above 1
	HX_RELEASE( pIUnk );

	pBuffer->Set( (const unsigned char *) pValue, dataLen );
	pValues->SetPropertyBuffer( pName, pBuffer );

	HX_RELEASE( pBuffer );
    }
}

#ifdef ENABLE_LOG_STREAMS
std::ostream& operator <<( std::ostream& o, const CHXPackage& )
{
    o << "CHXPackage";
    return o;
}
#endif

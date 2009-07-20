/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: dbindex.cpp,v 1.4 2005/03/23 23:45:20 liam_murray Exp $
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

#include "dbindex.h"
#include "ihxpckts.h"

				      
// XXXND  FIX  Is there a better size for this?				      
const UINT32 kInitHashTableSize = 47;


//---------------------- CPluginDatabaseIndex


CPluginDatabaseIndex::CPluginDatabaseIndex()
{
}


// static 
CPluginDatabaseIndex* CPluginDatabaseIndex::CreateIndex( EPluginIndexType indexType )
{
    CPluginDatabaseIndex* pNewIndex = NULL;

    switch( indexType )
    {
	case kIndex_StringType:
	    pNewIndex = new CPluginDatabaseIndexString();
	    break;

	case kIndex_GUIDType:
	    pNewIndex = new CPluginDatabaseIndexGUID();
	    break;

	case kIndex_NumTypes:
	case kIndex_MVStringType:
	case kIndex_BufferType:
	default:
	    break;
    }

    return pNewIndex;
}



//---------------------- CPluginDatabaseIndexGUID

CPluginDatabaseIndexGUID::CPluginDatabaseIndexGUID()
{
    m_mapGUIDToIUnknown.InitHashTable( kInitHashTableSize, FALSE );
}


CPluginDatabaseIndexGUID::~CPluginDatabaseIndexGUID()
{
    Reset();
}

HX_RESULT CPluginDatabaseIndexGUID::AddItem( IHXBuffer* pData, IUnknown* pIUnknown )
{
    HX_RESULT result = HXR_FAIL;

    if( pData && pIUnknown )
    {
	// Treat the stuff in pData like a GUID
	GUID* pGUID = (GUID*) pData->GetBuffer();

	void* pValue = NULL;
	if( !m_mapGUIDToIUnknown.Lookup( *pGUID, pValue ) )
	{
	    m_mapGUIDToIUnknown.SetAt( *pGUID, pIUnknown );
	    pIUnknown->AddRef();
	    result = HXR_OK;
	}
    }

    return result;
}


HX_RESULT CPluginDatabaseIndexGUID::FindItem( const void* pData, IUnknown** pIUnknown )
{
    HX_RESULT result = HXR_FAIL;

    if( pIUnknown )
    {
    	*pIUnknown = NULL;

	void* pValue = NULL;
	if( m_mapGUIDToIUnknown.Lookup( *(GUID*) pData, pValue ) )
	{
	    *pIUnknown = (IUnknown*) pValue;
	    (*pIUnknown)->AddRef();
    
	    result = HXR_OK;
	}
    }

    return result;
}


HX_RESULT CPluginDatabaseIndexGUID::RemoveItem( IUnknown* pIValue )
{
    HX_RESULT result = HXR_FAIL;

    CHXMapGUIDToObj::Iterator iter = m_mapGUIDToIUnknown.Begin();
    CHXMapGUIDToObj::Iterator iterEnd = m_mapGUIDToIUnknown.End();

    for( ; iter != iterEnd; ++iter )
    {
	// Get the IUnknown 
	if( pIValue == (IUnknown*)(*iter) )
	{
	    if( m_mapGUIDToIUnknown.RemoveKey( *iter.get_key() ) )
	    {
		HX_RELEASE(pIValue);

		result = HXR_OK;
	    }

	    break;
	}
    }

    return result;
}


void CPluginDatabaseIndexGUID::Reset()
{
    CHXMapGUIDToObj::Iterator iter = m_mapGUIDToIUnknown.Begin();
    CHXMapGUIDToObj::Iterator iterEnd = m_mapGUIDToIUnknown.End();

    for( ; iter != iterEnd; ++iter )
    {
	// Get the IUnknown 
	IUnknown* pUnknown = (IUnknown*)(*iter);
	HX_RELEASE(pUnknown);
    }

    m_mapGUIDToIUnknown.RemoveAll();
}




//------------------------------ CPluginDatabaseIndexString

CPluginDatabaseIndexString::CPluginDatabaseIndexString()
{

}


CPluginDatabaseIndexString::~CPluginDatabaseIndexString()
{
    Reset();
}


HX_RESULT CPluginDatabaseIndexString::AddItem( IHXBuffer* pData, IUnknown* pIUnknown )
{
    HX_RESULT result = HXR_FAIL;

    if( pData && pIUnknown )
    {
	const char *pCharData = (const char*) pData->GetBuffer();

	void* pValue = NULL;
	if( !m_mapStrToIUnknown.Lookup( pCharData, pValue ) )
	{
	    m_mapStrToIUnknown.SetAt( pCharData, pIUnknown );
	    pIUnknown->AddRef();
	    result = HXR_OK;
	}
    }

    return result;
}


HX_RESULT CPluginDatabaseIndexString::FindItem( const void* pData, IUnknown** pIUnknown )
{
    HX_RESULT result = HXR_FAIL;

    if( pIUnknown )
    {
    	*pIUnknown = NULL;

	void* pValue = NULL;
	if( m_mapStrToIUnknown.Lookup( (const char*) pData, pValue ) )
	{
	    *pIUnknown = (IUnknown*) pValue;
	    (*pIUnknown)->AddRef();
    
	    result = HXR_OK;
	}
    }

    return result;
}


HX_RESULT CPluginDatabaseIndexString::RemoveItem( IUnknown* pIValue )
{
    HX_RESULT result = HXR_FAIL;

    CHXMapStringToOb::Iterator iter = m_mapStrToIUnknown.Begin();
    CHXMapStringToOb::Iterator iterEnd = m_mapStrToIUnknown.End();

    for( ; iter != iterEnd; ++iter )
    {
	// Get the IUnknown 
	if( pIValue == (IUnknown*)(*iter) )
	{
	    if( m_mapStrToIUnknown.RemoveKey( iter.get_key() ) )
	    {
		HX_RELEASE(pIValue);

		result = HXR_OK;
	    }

	    break;
	}
    }

    return result;
}



void CPluginDatabaseIndexString::Reset()
{
    CHXMapStringToOb::Iterator iter = m_mapStrToIUnknown.Begin();
    CHXMapStringToOb::Iterator iterEnd = m_mapStrToIUnknown.End();

    for( ; iter != iterEnd; ++iter )
    {
	// Get the IUnknown 
	IUnknown* pUnknown = (IUnknown*)(*iter);
	HX_RELEASE(pUnknown);
    }

    m_mapStrToIUnknown.RemoveAll();
}


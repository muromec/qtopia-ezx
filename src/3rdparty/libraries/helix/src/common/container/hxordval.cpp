/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxordval.cpp,v 1.7 2007/07/06 20:34:58 jfinnecy Exp $
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
#include "hxresult.h"
#include "hxcom.h"
#include "hxordval.h"
#include "hxstrutl.h"
#include "chxpckts.h"
#include "hxheap.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


// IUnknown interface listing
BEGIN_INTERFACE_LIST(CHXOrderedValues)
    INTERFACE_LIST_ENTRY(IID_IHXValues, IHXValues)
END_INTERFACE_LIST


////////////////////////////////////////////////////////////////
CHXOrderedValues::CHXOrderedValues() : m_CStringPos(NULL)
{
};


////////////////////////////////////////////////////////////////
CHXOrderedValues::~CHXOrderedValues()
{
    // go through CString list, deleting NameBufferPair objects
    if (!m_CStringList.IsEmpty())
    {
    	LISTPOSITION ListPos = m_CStringList.GetHeadPosition();
    	_CStoreNameBufferPair* pCandidate = (_CStoreNameBufferPair*) m_CStringList.GetHead();
    
    	while ( ListPos != NULL)
    	{
	    if (pCandidate)
	    {
	    	delete pCandidate;
	    }
	    
	    pCandidate = (_CStoreNameBufferPair*) m_CStringList.GetAtNext(ListPos);
    	}
    }
}

////////////////////////////////////////////////////////////////
STDMETHODIMP
CHXOrderedValues::SetPropertyULONG32	     (
		const char*          pPropertyName,
		ULONG32              pPropertyValue)
{
    return HXR_NOTIMPL;
}

////////////////////////////////////////////////////////////////
STDMETHODIMP
CHXOrderedValues::GetPropertyULONG32	     (
		const char*          pPropertyName,
		REF(ULONG32)         uPropertyValue)
{
    return HXR_NOTIMPL;
}

////////////////////////////////////////////////////////////////
STDMETHODIMP
CHXOrderedValues::GetFirstPropertyULONG32   (
		REF(const char*)     pPropertyName,
		REF(ULONG32)         uPropertyValue)
{	
    return HXR_NOTIMPL;
}

////////////////////////////////////////////////////////////////
STDMETHODIMP
CHXOrderedValues::GetNextPropertyULONG32   (
		REF(const char*)    pPropertyName,
		REF(ULONG32)        uPropertyValue)
{	
    return HXR_NOTIMPL;
}

////////////////////////////////////////////////////////////////
STDMETHODIMP
CHXOrderedValues::SetPropertyBuffer	    (
		const char*         pPropertyName,
		IHXBuffer*         pPropertyValue)
{
    return HXR_NOTIMPL;
}

////////////////////////////////////////////////////////////////
STDMETHODIMP
CHXOrderedValues::GetPropertyBuffer	    (
		const char*         pPropertyName,
		REF(IHXBuffer*)    pPropertyValue)
{
    return HXR_NOTIMPL;
}


////////////////////////////////////////////////////////////////
STDMETHODIMP
CHXOrderedValues::GetFirstPropertyBuffer   (
		REF(const char*)    pPropertyName,
		REF(IHXBuffer*)    pPropertyValue)
{	
    return HXR_NOTIMPL;
}

////////////////////////////////////////////////////////////////
STDMETHODIMP
CHXOrderedValues::GetNextPropertyBuffer    (
		REF(const char*)    pPropertyName,
		REF(IHXBuffer*)    pPropertyValue)
{	
    return HXR_NOTIMPL;
}

////////////////////////////////////////////////////////////////
STDMETHODIMP
CHXOrderedValues::SetPropertyCString	    (
		const char*         pPropertyName,
		IHXBuffer*         pPropertyValue)
{
    if (!pPropertyValue) return HXR_UNEXPECTED;
    HX_ASSERT(pPropertyName);
    if (!pPropertyName)  return HXR_UNEXPECTED;
    
    _CStoreNameBufferPair* pnbpNew = NULL;

    // search list to see if name already exists.
    LISTPOSITION pos = FindCStringName(pPropertyName);
    if ( pos != NULL)
    {
	// we found an existing name/value pair with same name
	pnbpNew = (_CStoreNameBufferPair*) m_CStringList.GetAt(pos);
	// remove from list, we'll reuse the NameBufferPair object below
    	m_CStringList.RemoveAt(pos);
    }
    else
    {
    	pnbpNew = new _CStoreNameBufferPair;
        if(!pnbpNew)
        {
            return HXR_OUTOFMEMORY;
        }
    	pnbpNew->SetName(pPropertyName);
    }
    
    pnbpNew->SetValue(pPropertyValue);
    
    // add to tail of list
    m_CStringList.AddTail( (void*)pnbpNew );

    return HXR_OK;
}

////////////////////////////////////////////////////////////////
STDMETHODIMP
CHXOrderedValues::GetPropertyCString	    (
		const char*         pPropertyName,
		REF(IHXBuffer*)    pPropertyValue)
{
    if (!pPropertyName) return HXR_UNEXPECTED;

    HX_RELEASE(pPropertyValue);
    

    // search list to see if name exists.
    LISTPOSITION pos = FindCStringName(pPropertyName);
    if ( pos == NULL)
    {
	return HXR_FAIL;
    }
    
    // we found an existing name/value pair with same name
    _CStoreNameBufferPair* pnbpValue = (_CStoreNameBufferPair*) m_CStringList.GetAt(pos);
	
    pPropertyValue = pnbpValue->GetValue();

    return HXR_OK;
}


////////////////////////////////////////////////////////////////
STDMETHODIMP
CHXOrderedValues::GetFirstPropertyCString   (
		REF(const char*)    pPropertyName,
		REF(IHXBuffer*)    pPropertyValue)
{
    pPropertyName = NULL;
    HX_RELEASE(pPropertyValue);
    
    if (m_CStringList.IsEmpty())
	return HXR_FAIL;
	
    m_CStringPos = m_CStringList.GetHeadPosition();

    _CStoreNameBufferPair* pnbpValue = (_CStoreNameBufferPair*) m_CStringList.GetAt(m_CStringPos);

    if (pnbpValue == NULL)
	return HXR_FAIL;

    pPropertyName = pnbpValue->GetName();
    pPropertyValue = pnbpValue->GetValue();
    
    return HXR_OK;
}

////////////////////////////////////////////////////////////////
STDMETHODIMP
CHXOrderedValues::GetNextPropertyCString    (
		REF(const char*)    pPropertyName,
		REF(IHXBuffer*)    pPropertyValue)
{	
    pPropertyName = NULL;
    HX_RELEASE(pPropertyValue);
    
    if (m_CStringList.IsEmpty() || m_CStringPos==NULL )
	return HXR_FAIL;
	
    _CStoreNameBufferPair* pnbpValue = (_CStoreNameBufferPair*) m_CStringList.GetAtNext(m_CStringPos);

    if (pnbpValue == NULL)
	return HXR_FAIL;

    pPropertyName = pnbpValue->GetName();
    pPropertyValue = pnbpValue->GetValue();
    
    return HXR_OK;
}


////////////////////////////////////////////////////////////////
LISTPOSITION
CHXOrderedValues::FindCStringName (const char* pPropertyName)
{	
    LISTPOSITION    ListPos = NULL;
    
    // search list to see if name already exists.
    if ( !m_CStringList.IsEmpty() )
    {
	HXBOOL bFound  = FALSE;
	
    	ListPos = m_CStringList.GetHeadPosition();
	_CStoreNameBufferPair* pCandidate = (_CStoreNameBufferPair*) m_CStringList.GetHead();
	
    	while ( ListPos != NULL)
    	{
    	    // if we find the same name, delete it
    	    if (  (pCandidate != NULL) &&
    	         !(pCandidate->GetName().CompareNoCase(pPropertyName)) &&
    		  (pCandidate->GetName().GetLength() == strlen(pPropertyName)) )
    	    {
    		bFound = TRUE;
    		break;
    	    }
	    else
		pCandidate = (_CStoreNameBufferPair*) m_CStringList.GetAtNext(ListPos);
	}
	
    	if (!bFound)
	    ListPos = NULL;
    }

    return ListPos;
}


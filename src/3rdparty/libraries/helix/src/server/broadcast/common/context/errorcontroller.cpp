/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: errorcontroller.cpp,v 1.2 2003/01/23 23:42:47 damonlan Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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

#include "errorcontroller.h"


STDMETHODIMP
CErrorcontroller::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
		AddRef();
		*ppvObj = (IUnknown*)((IHXErrorMessages*)this);
		return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXErrorMessages))
	{
		AddRef();
		*ppvObj = (IUnknown*)((IHXErrorMessages*)this);
		return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXErrorSinkControl))
	{
		AddRef();
		*ppvObj = (IUnknown*)((IHXErrorSinkControl*)this);
		return HXR_OK;
    }
	*ppvObj = NULL;
	return HXR_FAIL;
}

STDMETHODIMP_(ULONG32)
CErrorcontroller::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
CErrorcontroller::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
		return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
CErrorcontroller::Close()
{
	SErrorSinkInfo *pErrSinkInfo;

	LISTPOSITION pos = m_pErrorSinkListInfo.GetHeadPosition();
	LISTPOSITION prevpos = NULL;

	while ( pos != NULL )
	{
		prevpos = pos;
		pErrSinkInfo = (SErrorSinkInfo *)m_pErrorSinkListInfo.GetNext( pos );
		if( pErrSinkInfo )
		{
			m_pErrorSinkListInfo.RemoveAt( prevpos );
			pErrSinkInfo->pErrorSink->Release();
		}
	}
	return HXR_OK;
}

STDMETHODIMP
CErrorcontroller::AddErrorSink( IHXErrorSink*	pErrorSink,	
								const UINT8	unLowSeverity,
								const UINT8 unHighSeverity)
{
	HX_RESULT res = HXR_OK;
	if ( pErrorSink == NULL )
	{
		return HXR_FAIL;
	}

	SErrorSinkInfo *pErrorSinkInfo = new SErrorSinkInfo;
	if ( pErrorSinkInfo == NULL )
	{
		res = HXR_FAIL;
	}

	if ( SUCCEEDED( res ) )
	{
		pErrorSinkInfo->pErrorSink = pErrorSink;
		(pErrorSinkInfo->pErrorSink)->AddRef();

		pErrorSinkInfo->unLowSeverity = unLowSeverity;
		pErrorSinkInfo->unHighSeverity = unHighSeverity;

		m_pErrorSinkListInfo.AddTail( pErrorSinkInfo );
	}

	return res;
}


STDMETHODIMP
CErrorcontroller::RemoveErrorSink(IHXErrorSink* pErrorSink)
{
	SErrorSinkInfo *pErrSinkInfo;
	BOOL bFound = FALSE;
	HX_RESULT res = HXR_OK;

	LISTPOSITION pos = m_pErrorSinkListInfo.GetHeadPosition();
	LISTPOSITION prevpos = NULL;
	while ( pos != NULL )
	{
		prevpos = pos;
		pErrSinkInfo = (SErrorSinkInfo *)m_pErrorSinkListInfo.GetNext( pos );
		if( pErrSinkInfo )
		{
			if( pErrSinkInfo->pErrorSink == pErrorSink )
			{
				m_pErrorSinkListInfo.RemoveAt( prevpos );
				pErrSinkInfo->pErrorSink->Release();
				bFound = TRUE;
			}
		}
	}

	if ( !bFound )
	{
		res = HXR_FAIL;
	}

	return res;
}

STDMETHODIMP
CErrorcontroller::Report( const UINT8	unSeverity,  
						  HX_RESULT	ulHXCode,
						  const ULONG32	ulUserCode,
						  const char*	pUserString,
						  const char*	pMoreInfoURL
						)

{
	CHXSimpleList::Iterator i = m_pErrorSinkListInfo.Begin();
	SErrorSinkInfo *pErrSinkInfo;

	while ( i != m_pErrorSinkListInfo.End() )
	{
		pErrSinkInfo = (SErrorSinkInfo *)(*i);
		if ( ( unSeverity >= pErrSinkInfo->unLowSeverity ) && ( unSeverity <= pErrSinkInfo->unHighSeverity ) )
		{
			pErrSinkInfo->pErrorSink->ErrorOccurred(unSeverity,
													ulHXCode,
													ulUserCode,
													pUserString,
													pMoreInfoURL
													);
		}
		++i;
	}

	return HXR_OK;
}

IHXBuffer* CErrorcontroller::GetErrorText(HX_RESULT	ulHXCode)
{
	return NULL;
}

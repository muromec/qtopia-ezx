/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: viewsrc.cpp,v 1.6 2007/07/06 21:58:12 jfinnecy Exp $
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

#include "hxcom.h"
#include "hxtypes.h"
#include "hxresult.h"
#include "hxslist.h"
#include "chxpckts.h"
#include "hxstrutl.h"

#include "hxvsrc.h"
#include "viewsrc.h"

HXViewSource::HXViewSource(IUnknown* pContext)
	: m_lRefCount(0)
	, m_pViewSourceHdlr(NULL)
	, m_pViewRightsHdlr(NULL)
	, m_pContext(NULL)
{    
    if (pContext)
    {
	m_pContext = pContext;
	m_pContext->AddRef();
    }
}

HXViewSource::~HXViewSource()
{
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pViewSourceHdlr);
    HX_RELEASE(m_pViewRightsHdlr);
}

STDMETHODIMP
HXViewSource::QueryInterface(REFIID riid, void**ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXClientViewSource*)this },
            { GET_IIDHANDLE(IID_IHXClientViewSource), (IHXClientViewSource*)this },
            { GET_IIDHANDLE(IID_IHXClientViewRights), (IHXClientViewRights*)this },
            { GET_IIDHANDLE(IID_IHXClientViewSourceSink), (IHXClientViewSourceSink*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
HXViewSource::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
HXViewSource::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/************************************************************************
 *  IHXClientViewSource
 */
STDMETHODIMP
HXViewSource::DoViewSource (
	IUnknown*		      /*IN*/ pPlayerContext,
	IHXStreamSource*	      /*IN*/ pSource)
{
    if ( m_pViewSourceHdlr == NULL )
    {
	return HXR_NOT_INITIALIZED;
    }
    return m_pViewSourceHdlr->DoViewSource(pPlayerContext, pSource);
}

STDMETHODIMP_(HXBOOL)
HXViewSource::CanViewSource(
	IHXStreamSource*	    /*IN*/  pSource)
{
    if ( m_pViewSourceHdlr == NULL )
    {
	return FALSE;
    }
    return m_pViewSourceHdlr->CanViewSource(pSource);
}

STDMETHODIMP
HXViewSource::GetViewSourceURL(IUnknown*			pPlayerContext,
				IHXStreamSource*		pSource,
				IHXViewSourceURLResponse*	pResp)
{
    if ( m_pViewSourceHdlr == NULL )
    {
	return HXR_NOT_INITIALIZED;
    }
    return m_pViewSourceHdlr->GetViewSourceURL(pPlayerContext, pSource, pResp);
}

/************************************************************************
 *  IHXClientViewRights
 */
STDMETHODIMP
HXViewSource::ViewRights(IUnknown* pContext)
{
    if (m_pViewRightsHdlr == NULL)
    {
	return HXR_NOT_INITIALIZED;
    }
    return m_pViewRightsHdlr->ViewRights(pContext);
}

STDMETHODIMP_(HXBOOL)
HXViewSource::CanViewRights()
{
    if (m_pViewRightsHdlr == NULL)
    {
	return FALSE;
    }
    return m_pViewRightsHdlr->CanViewRights();
}
/************************************************************************
 *  IHXClientViewSourceSink
 */
STDMETHODIMP
HXViewSource::RegisterViewSourceHdlr(IHXClientViewSource* pViewSourceHdlr)
{
    HX_ASSERT(pViewSourceHdlr);
    if ( pViewSourceHdlr )
    {
	HX_RELEASE(m_pViewSourceHdlr);
	m_pViewSourceHdlr = pViewSourceHdlr;
	m_pViewSourceHdlr->AddRef();

	pViewSourceHdlr->QueryInterface(IID_IHXClientViewRights, 
	    (void**)&m_pViewRightsHdlr);
    }
    return HXR_OK;
}

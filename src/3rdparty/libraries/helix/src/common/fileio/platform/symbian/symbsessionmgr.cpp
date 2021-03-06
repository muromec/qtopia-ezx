/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: symbsessionmgr.cpp,v 1.6 2004/07/09 18:20:01 hubbe Exp $
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

/****************************************************************************
 *  Includes
 */
#include "symbsessionmgr.h"


/****************************************************************************
 *  CSymbSessionMgr
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CSymbSessionMgr::CSymbSessionMgr()
    : m_bHaveSession(FALSE)
    , m_lRefCount(0)
{
    ;
}

CSymbSessionMgr::~CSymbSessionMgr(void)
{
    if (m_bHaveSession)
    {
	m_symbSession.Close();
	m_bHaveSession = FALSE;
    }
}


/****************************************************************************
 *  IHXSymbFileSessionManager methods
 */
STDMETHODIMP CSymbSessionMgr::GetSession(REF(RFs) symbSession)
{
    HX_RESULT retVal = HXR_OK;

    if (m_bHaveSession)
    {
	symbSession = m_symbSession;
	return retVal;
    }
    else
    {
	retVal = HXR_FAIL;
	if (m_symbSession.Connect() == KErrNone)
	{
	    retVal = HXR_OK;
	    symbSession = m_symbSession;
	    m_bHaveSession = TRUE;
	}
    }

    return retVal;
}


/****************************************************************************
 *  CSymbSessionMgr methods
 */
HX_RESULT CSymbSessionMgr::Create(IHXSymbFileSessionManager* &pManagerOut,
				  IUnknown** ppCommonObj)
{
    IHXSymbFileSessionManager* pSessionManager = NULL;
    HX_RESULT retVal = HXR_FAIL;

    if (ppCommonObj && (*ppCommonObj))
    {
	(*ppCommonObj)->QueryInterface(IID_IHXSymbFileSessionManager,
				       (void**) &pSessionManager);
    }

    if (!pSessionManager)
    {
	pSessionManager = new CSymbSessionMgr;
	if (pSessionManager)
	{
	    pSessionManager->AddRef();
	}
    }

    if (pSessionManager)
    {
	pManagerOut = pSessionManager;
	pSessionManager->AddRef();
	retVal = HXR_OK;

	if (ppCommonObj && ((*ppCommonObj) == NULL))
	{
	    *ppCommonObj = pSessionManager;
	    pSessionManager->AddRef();
	}
    }

    HX_RELEASE(pSessionManager);

    return retVal;
}


/************************************************************************
 *  IUnknown methods
 */
STDMETHODIMP CSymbSessionMgr::QueryInterface(REFIID riid, void** ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*) this },
		{ GET_IIDHANDLE(IID_IHXSymbFileSessionManager), (IHXSymbFileSessionManager*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);   
}

STDMETHODIMP_(ULONG32) CSymbSessionMgr::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CSymbSessionMgr::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

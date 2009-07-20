/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: plsnkctl.cpp,v 1.4 2004/07/09 18:42:32 hubbe Exp $
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

#include "hxcom.h"
#include "hxtypes.h"
#include "hxpsink.h"
#include "hxslist.h"
#include "plsnkctl.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

/************************************************************************
 *
 * CHXPlayerSinkControl 
 *
 */

CHXPlayerSinkControl::CHXPlayerSinkControl()
    : m_lRefCount(0)
    , m_pSinkList(NULL)
    , m_pSinksToBeRemoved(NULL)
    , m_bInPlayerClosed(FALSE)
{
}

CHXPlayerSinkControl::~CHXPlayerSinkControl()
{
    Terminate();
}

void CHXPlayerSinkControl::Terminate()
{
    m_bInPlayerClosed = TRUE;
    if (m_pSinkList)
    {
	CHXSimpleList::Iterator lIterator	= m_pSinkList->Begin();
    
	for (; lIterator != m_pSinkList->End(); ++lIterator)
	{
	    IHXPlayerCreationSink* pPlayerSink = (IHXPlayerCreationSink*) (*lIterator);
	    pPlayerSink->Release();
	}

	m_pSinkList->RemoveAll();

	HX_DELETE(m_pSinkList);
    }
 
    if (m_pSinksToBeRemoved)
    {
        m_pSinksToBeRemoved->RemoveAll();
    }

    HX_DELETE(m_pSinksToBeRemoved);
    m_bInPlayerClosed = FALSE;
}

STDMETHODIMP
CHXPlayerSinkControl::AddSink(IHXPlayerCreationSink* pPlayerSink)
{
    if (pPlayerSink)
    {
	if (!m_pSinkList)
	{
	    m_pSinkList = new CHXSimpleList;
	    if (!m_pSinkList)
	    {
		return HXR_OUTOFMEMORY;
	    }
	}

    	m_pSinkList->AddTail(pPlayerSink);
    	pPlayerSink->AddRef();
    	
    }
    return HXR_OK;
}

STDMETHODIMP
CHXPlayerSinkControl::RemoveSink(IHXPlayerCreationSink* pPlayerSink)
{
    if (m_pSinkList)
    {
	LISTPOSITION pos = m_pSinkList->Find(pPlayerSink);

	if (pos)
	{
	    if (!m_bInPlayerClosed)
	    {
		m_pSinkList->RemoveAt(pos);	    
		pPlayerSink->Release();
	    }
	    else
	    {
		if (!m_pSinksToBeRemoved)
		{
		    m_pSinksToBeRemoved = new CHXSimpleList;
		    if (!m_pSinksToBeRemoved)
		    {
			return HXR_OUTOFMEMORY;
		    }
		}

		/* Add to the list for later removal */
		m_pSinksToBeRemoved->AddTail(pPlayerSink);
	    }
	    return HXR_OK;
	}
    }

    return HXR_FAIL;
}

STDMETHODIMP 
CHXPlayerSinkControl::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXPlayerSinkControl), (IHXPlayerSinkControl*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXPlayerSinkControl*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);        
}

STDMETHODIMP_ (ULONG32) 
CHXPlayerSinkControl::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_ (ULONG32) 
CHXPlayerSinkControl::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

void	
CHXPlayerSinkControl::PlayerCreated	(IHXPlayer* pPlayer)
{
    if (m_pSinkList)
    {
	CHXSimpleList::Iterator lIterator	= m_pSinkList->Begin();
    
	for (; lIterator != m_pSinkList->End(); ++lIterator)
	{
	    IHXPlayerCreationSink* pPlayerSink = (IHXPlayerCreationSink*) (*lIterator);
	    pPlayerSink->PlayerCreated(pPlayer);
	}
    }
}

void	
CHXPlayerSinkControl::PlayerClosed	(IHXPlayer* pPlayer)
{
    if (m_pSinkList)
    {
	m_bInPlayerClosed = TRUE;
	CHXSimpleList::Iterator lIterator	= m_pSinkList->Begin();
    
	for (; lIterator != m_pSinkList->End(); ++lIterator)
	{
	    IHXPlayerCreationSink* pPlayerSink = (IHXPlayerCreationSink*) (*lIterator);
	    pPlayerSink->PlayerClosed(pPlayer);
	}

	m_bInPlayerClosed = FALSE;

	if (m_pSinksToBeRemoved && m_pSinksToBeRemoved->GetCount() > 0)
	{
	    lIterator	= m_pSinksToBeRemoved->Begin();
	    for (; lIterator != m_pSinksToBeRemoved->End(); ++lIterator)
	    {
		IHXPlayerCreationSink* pPlayerSink = 
		    (IHXPlayerCreationSink*) (*lIterator);
		RemoveSink(pPlayerSink);
	    }

	    m_pSinksToBeRemoved->RemoveAll();
	}
    }
}

/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cliauth.cpp,v 1.9 2006/07/05 17:52:54 gwright Exp $
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxcomm.h"

#include "hxauth.h"

#include "hxplugn.h"
#include "hxplgns.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxauthn.h"
#include "hxmon.h"

#include "chxpckts.h"

#include "cliauth.h"

#include "hxplgsp.h"
#include "hxplnsp.h"

#include "crdcache.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

_CListOfWrapped_IUnknown_Node::_CListOfWrapped_IUnknown_Node()
  : m_plocPrev(NULL)
  , m_plocNext(NULL)
{
}

_CListOfWrapped_IUnknown_Node::~_CListOfWrapped_IUnknown_Node()
{
    Remove();
}

void
_CListOfWrapped_IUnknown_Node::Remove()
{
    if(m_plocPrev)
    {
	m_plocPrev->next(m_plocNext);
    }

    if(m_plocNext)
    {
	m_plocNext->prev(m_plocPrev);
    }
}

void
_CListOfWrapped_IUnknown_Node::Insert(_CListOfWrapped_IUnknown_Node& rlocnNew)
{
    rlocnNew.next(this);
    rlocnNew.prev(m_plocPrev);

    if(m_plocPrev)
    {
	m_plocPrev->next(&rlocnNew);
    }
	
    m_plocPrev = &rlocnNew;
}

Wrapped_IUnknown&
_CListOfWrapped_IUnknown_Node::value()
{
    return m_clsValue;
}

const Wrapped_IUnknown&
_CListOfWrapped_IUnknown_Node::value() const
{
    return m_clsValue;
}

void
_CListOfWrapped_IUnknown_Node::value(const Wrapped_IUnknown& rclsNewValue)
{
    m_clsValue = rclsNewValue;
}

_CListOfWrapped_IUnknown_Node&
_CListOfWrapped_IUnknown_Node::operator=(const Wrapped_IUnknown& rclsNewValue)
{
    m_clsValue = rclsNewValue;
    return *this;
}

_CListOfWrapped_IUnknown_Node*
_CListOfWrapped_IUnknown_Node::next() const
{
    return m_plocNext;
}

void
_CListOfWrapped_IUnknown_Node::next(_CListOfWrapped_IUnknown_Node* plocnNew)
{
    m_plocNext = plocnNew;
}

_CListOfWrapped_IUnknown_Node*
_CListOfWrapped_IUnknown_Node::prev() const
{
    return m_plocPrev;
}

void
_CListOfWrapped_IUnknown_Node::prev(_CListOfWrapped_IUnknown_Node* plocnNew)
{
    m_plocPrev = plocnNew;
}

_CListOfWrapped_IUnknown_::_CListOfWrapped_IUnknown_()
{
    m_locnREnd.next(&m_locnEnd);
    m_locnEnd.prev(&m_locnREnd);
}

_CListOfWrapped_IUnknown_::_CListOfWrapped_IUnknown_(const _CListOfWrapped_IUnknown_& rlocOther)
{
    m_locnREnd.next(&m_locnEnd);
    m_locnEnd.prev(&m_locnREnd);

    _copy(rlocOther);
}

_CListOfWrapped_IUnknown_::~_CListOfWrapped_IUnknown_()
{
    empty();
}

_CListOfWrapped_IUnknown_&
_CListOfWrapped_IUnknown_::operator=(const _CListOfWrapped_IUnknown_& rlocOther)
{
    empty();
    _copy(rlocOther);

    return *this;
}

void
_CListOfWrapped_IUnknown_::_copy(const _CListOfWrapped_IUnknown_& rlocOther)
{
    iterator itOther;
			
    for
    (
	itOther = rlocOther.begin();
	itOther != rlocOther.end();
	++itOther
    )
    {
	insert(end(), *itOther);
    }
}

_CListOfWrapped_IUnknown_::iterator
_CListOfWrapped_IUnknown_::begin()
{
    return iterator(*(m_locnREnd.next()));
}

const _CListOfWrapped_IUnknown_::iterator
_CListOfWrapped_IUnknown_::begin() const
{
    return iterator(*(m_locnREnd.next()));
}

_CListOfWrapped_IUnknown_::iterator
_CListOfWrapped_IUnknown_::end()
{
    return iterator(m_locnEnd);
}

const _CListOfWrapped_IUnknown_::iterator
_CListOfWrapped_IUnknown_::end() const
{
    return iterator(m_locnEnd);
}

_CListOfWrapped_IUnknown_::reverse_iterator
_CListOfWrapped_IUnknown_::rbegin()
{
    _CListOfWrapped_IUnknown_::reverse_iterator temp(*(m_locnEnd.prev()));
    return temp;
}

const _CListOfWrapped_IUnknown_::reverse_iterator 
_CListOfWrapped_IUnknown_::rbegin() const
{
    _CListOfWrapped_IUnknown_::reverse_iterator temp(*(m_locnEnd.prev()));
    return temp;
}

_CListOfWrapped_IUnknown_::reverse_iterator
_CListOfWrapped_IUnknown_::rend()
{
    _CListOfWrapped_IUnknown_::reverse_iterator temp(m_locnREnd);
    return temp;
}

const _CListOfWrapped_IUnknown_::reverse_iterator
_CListOfWrapped_IUnknown_::rend() const
{
    _CListOfWrapped_IUnknown_::reverse_iterator temp(*((const _CListOfWrapped_IUnknown_Node *)&m_locnREnd));
    return temp;
}

_CListOfWrapped_IUnknown_::iterator
_CListOfWrapped_IUnknown_::insert(iterator itBefore, const Wrapped_IUnknown& rclsNew)
{
    _CListOfWrapped_IUnknown_Node* plocnNew = new _CListOfWrapped_IUnknown_Node;

    HX_ASSERT(plocnNew);

    *plocnNew = rclsNew;

    itBefore.m_plocCurrent->Insert(*plocnNew);

    return iterator(*plocnNew);
}

void
_CListOfWrapped_IUnknown_::insert
(
    iterator itBefore,
    const iterator itFirst,
    const iterator itLast
)
{
    iterator itOther;
    _CListOfWrapped_IUnknown_Node* plocnNew;

    for (itOther = itFirst; itOther != itLast; ++itOther)
    {
	plocnNew = new _CListOfWrapped_IUnknown_Node;

	HX_ASSERT(plocnNew);

	*plocnNew = *itOther;

	itBefore.m_plocCurrent->Insert(*plocnNew);
    }
}

void
_CListOfWrapped_IUnknown_::remove(iterator itThis)
{
    if
    (
	itThis.m_plocCurrent == &m_locnEnd ||
	itThis.m_plocCurrent == &m_locnREnd
    )
    {
	return;
    }

    _CListOfWrapped_IUnknown_Node* plocnOld;

    plocnOld = itThis.m_plocCurrent;

    ++itThis;

    plocnOld->Remove();

    delete plocnOld;
}

void
_CListOfWrapped_IUnknown_::remove(iterator itFirst, iterator itLast)
{
    if
    (
	itFirst.m_plocCurrent == &m_locnEnd ||
	itFirst.m_plocCurrent == &m_locnREnd
    )
    {
	return;
    }

    iterator itOther;
    _CListOfWrapped_IUnknown_Node* plocnOld;

    for (itOther = itFirst; itOther != itLast;)
    {
	plocnOld = itOther.m_plocCurrent;

	++itOther;

	plocnOld->Remove();

	delete plocnOld;
    }
}

void
_CListOfWrapped_IUnknown_::empty()
{
    remove(begin(), end());
}

_CListIteratorWrapped_IUnknown_::_CListIteratorWrapped_IUnknown_()
  : m_plocCurrent(NULL)
{
}

_CListIteratorWrapped_IUnknown_::_CListIteratorWrapped_IUnknown_
(
    const _CListOfWrapped_IUnknown_Node& rlocnNewLocation
)
  : m_plocCurrent((_CListOfWrapped_IUnknown_Node*)&rlocnNewLocation)
{
}

_CListIteratorWrapped_IUnknown_::_CListIteratorWrapped_IUnknown_
(
    const _CListIteratorWrapped_IUnknown_& rliocOther
)
  : m_plocCurrent(rliocOther.m_plocCurrent)
{
}

_CListIteratorWrapped_IUnknown_::~_CListIteratorWrapped_IUnknown_()
{
}

_CListIteratorWrapped_IUnknown_&
_CListIteratorWrapped_IUnknown_::operator=
(
    const _CListIteratorWrapped_IUnknown_& rliocOther
)
{
    m_plocCurrent = rliocOther.m_plocCurrent;

    return *this;
}

Wrapped_IUnknown&
_CListIteratorWrapped_IUnknown_::operator*()
{
    HX_ASSERT(m_plocCurrent);
    return m_plocCurrent->value();
}

_CListIteratorWrapped_IUnknown_&
_CListIteratorWrapped_IUnknown_::operator=(const Wrapped_IUnknown& rclsNewValue)
{
    if(!m_plocCurrent)
	return *this;

    m_plocCurrent->value(rclsNewValue);

    return *this;
}

_CListIteratorWrapped_IUnknown_&
_CListIteratorWrapped_IUnknown_::operator++()
{
    if(!m_plocCurrent)
	return *this;

    m_plocCurrent = m_plocCurrent->next();

    return *this;
}

const _CListIteratorWrapped_IUnknown_
_CListIteratorWrapped_IUnknown_::operator++(int)
{
    _CListIteratorWrapped_IUnknown_ liocRet(*this);

    ++(*this);

    return liocRet;
}

_CListIteratorWrapped_IUnknown_&
_CListIteratorWrapped_IUnknown_::operator--()
{
    if(!m_plocCurrent)
	return *this;

    m_plocCurrent = m_plocCurrent->prev();

    return *this;
}

const _CListIteratorWrapped_IUnknown_
_CListIteratorWrapped_IUnknown_::operator--(int)
{
    _CListIteratorWrapped_IUnknown_ liocRet(*this);

    --(*this);

    return liocRet;
}

HXBOOL operator==
(
    const _CListIteratorWrapped_IUnknown_& rliocLeft,
    const _CListIteratorWrapped_IUnknown_& rliocRight
)
{
    return (rliocLeft.m_plocCurrent == rliocRight.m_plocCurrent);
}

HXBOOL operator!=
(
    const _CListIteratorWrapped_IUnknown_& rliocLeft,
    const _CListIteratorWrapped_IUnknown_& rliocRight
)
{
    return (rliocLeft.m_plocCurrent != rliocRight.m_plocCurrent);
}

_CListReverseIteratorWrapped_IUnknown_::_CListReverseIteratorWrapped_IUnknown_()
  : m_plocCurrent(NULL)
{
}

_CListReverseIteratorWrapped_IUnknown_::_CListReverseIteratorWrapped_IUnknown_
(
    const _CListOfWrapped_IUnknown_Node& rlocnNewLocation
)
  : m_plocCurrent((_CListOfWrapped_IUnknown_Node*)&rlocnNewLocation)
{
}

_CListReverseIteratorWrapped_IUnknown_::_CListReverseIteratorWrapped_IUnknown_
(
    const _CListReverseIteratorWrapped_IUnknown_& rlriocOther
)
  : m_plocCurrent(rlriocOther.m_plocCurrent)
{
}

_CListReverseIteratorWrapped_IUnknown_::~_CListReverseIteratorWrapped_IUnknown_()
{
}

_CListReverseIteratorWrapped_IUnknown_&
_CListReverseIteratorWrapped_IUnknown_::operator=
(
    const _CListReverseIteratorWrapped_IUnknown_& rlriocOther
)
{
    m_plocCurrent = rlriocOther.m_plocCurrent;
    return *this;
}

Wrapped_IUnknown&
_CListReverseIteratorWrapped_IUnknown_::operator*()
{
    HX_ASSERT(m_plocCurrent);
    return m_plocCurrent->value();
}

_CListReverseIteratorWrapped_IUnknown_&
_CListReverseIteratorWrapped_IUnknown_::operator=(const Wrapped_IUnknown& rclsNewValue)
{
    if(!m_plocCurrent)
	return *this;

    m_plocCurrent->value(rclsNewValue);

    return *this;
}

_CListReverseIteratorWrapped_IUnknown_&
_CListReverseIteratorWrapped_IUnknown_::operator++()
{
    if(!m_plocCurrent)
	return *this;

    m_plocCurrent = m_plocCurrent->prev();

    return *this;
}

const _CListReverseIteratorWrapped_IUnknown_
_CListReverseIteratorWrapped_IUnknown_::operator++(int)
{
    _CListReverseIteratorWrapped_IUnknown_ lriocRet(*this);

    ++(*this);

    return lriocRet;
}

_CListReverseIteratorWrapped_IUnknown_&
_CListReverseIteratorWrapped_IUnknown_::operator--()
{
    if(!m_plocCurrent)
	return *this;

    m_plocCurrent = m_plocCurrent->next();

    return *this;
}

const _CListReverseIteratorWrapped_IUnknown_
_CListReverseIteratorWrapped_IUnknown_::operator--(int)
{
    _CListReverseIteratorWrapped_IUnknown_ lriocRet(*this);

    --(*this);

    return lriocRet;
}

HXBOOL operator==
(
    const _CListReverseIteratorWrapped_IUnknown_& rlriocLeft,
    const _CListReverseIteratorWrapped_IUnknown_& rlriocRight
)
{
    return (rlriocLeft.m_plocCurrent == rlriocRight.m_plocCurrent);
}

HXBOOL operator!=
(
    const _CListReverseIteratorWrapped_IUnknown_& rlriocLeft,
    const _CListReverseIteratorWrapped_IUnknown_& rlriocRight
)
{
    return (rlriocLeft.m_plocCurrent != rlriocRight.m_plocCurrent);
}

BEGIN_INTERFACE_LIST(CHXClientAuthenticator)
    INTERFACE_LIST_ENTRY
    (
	IID_IHXObjectConfiguration, 
	IHXObjectConfiguration
    )
    INTERFACE_LIST_ENTRY
    (
	IID_IHXClientAuthConversation, 
	IHXClientAuthConversation
    )
    INTERFACE_LIST_ENTRY
    (
	IID_IHXClientAuthResponse, 
	IHXClientAuthResponse
    )
    INTERFACE_LIST_ENTRY(IID_IHXCredRequest, IHXCredRequest)
    INTERFACE_LIST_ENTRY
    (
	IID_IHXCredRequestResponse, 
	IHXCredRequestResponse
    )
    INTERFACE_LIST_ENTRY
    (
	IID_IHXAuthenticationManagerResponse, 
	IHXAuthenticationManagerResponse
    )
END_INTERFACE_LIST

CHXClientAuthenticator::CHXClientAuthenticator()
    : m_HX_RESULTStatus(HXR_NOT_AUTHORIZED)
    , m_pRegistry(NULL)
    , m_pRealm(NULL)
    , m_pCredentialsCache(NULL)
    , m_pClassFactory(NULL)
{
}

CHXClientAuthenticator::~CHXClientAuthenticator()
{
    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pRealm);
    HX_RELEASE(m_pCredentialsCache);
    HX_RELEASE(m_pClassFactory);
}

// IHXObjectConfiguration
STDMETHODIMP
CHXClientAuthenticator::SetContext(IUnknown* pIUnknownContext)
{
    m_spIUnknownContext = pIUnknownContext;

    HX_RELEASE(m_pRegistry);
    m_spIUnknownContext->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry);

    if (HXR_OK != m_spIUnknownContext->QueryInterface(IID_IHXCredentialsCache, 
						      (void**)&m_pCredentialsCache))
    {
	m_pCredentialsCache = NULL;
    }

    if (FAILED(m_spIUnknownContext->QueryInterface(IID_IHXCommonClassFactory,
        (void**)&m_pClassFactory)))
    {
        m_pClassFactory = NULL;
    }
    _TryToLoadPlugins();

    _CListOfWrapped_IUnknown_::iterator ListOfIUnknownIteratorCurrent;
    DECLARE_SMART_POINTER(IHXObjectConfiguration) sIHXObjectConfigurationPlugin;

    for(ListOfIUnknownIteratorCurrent = m_ListOfIUnknownPlugins.begin();
	ListOfIUnknownIteratorCurrent != m_ListOfIUnknownPlugins.end();
	++ListOfIUnknownIteratorCurrent)
    {
	sIHXObjectConfigurationPlugin = (*ListOfIUnknownIteratorCurrent).
            wrapped_ptr();

	// Initialize this instance
	sIHXObjectConfigurationPlugin->SetContext(pIUnknownContext);
    }

    return HXR_OK;
}

STDMETHODIMP 
CHXClientAuthenticator::SetConfiguration
(
    IHXValues* pIHXValuesConfiguration
)
{
    m_spIHXValuesConfig = pIHXValuesConfiguration;

    _TryToLoadPlugins();

    return HXR_OK;
}


// IHXClientAuthConversation
STDMETHODIMP 
CHXClientAuthenticator::MakeResponse
(
    IHXClientAuthResponse* pIHXClientAuthResponseRequester,
    IHXRequest* pIHXRequestChallenge
)
{
    if(!pIHXClientAuthResponseRequester || !m_pClassFactory)
    {
	return HXR_UNEXPECTED;
    }
    
    if(m_ListOfIUnknownPlugins.begin() == m_ListOfIUnknownPlugins.end() ||
	m_spIHXClientAuthResponseRequester.IsValid())
    {
	pIHXClientAuthResponseRequester->ResponseReady(HXR_UNEXPECTED, NULL);
	return HXR_UNEXPECTED;
    }

    DECLARE_SMART_POINTER(IHXClientAuthConversation) 
        spIHXClientAuthConversationPlugin;

    m_HX_RESULTStatus = HXR_FAIL;
    m_spIHXClientAuthResponseRequester = pIHXClientAuthResponseRequester;
    m_spIHXRequestChallenge = pIHXRequestChallenge;
    m_ListOfIUnknownIteratorCurrent = m_ListOfIUnknownPlugins.begin();

    spIHXClientAuthConversationPlugin = 
        (*m_ListOfIUnknownIteratorCurrent).wrapped_ptr();

    spIHXClientAuthConversationPlugin->MakeResponse(this, 
	m_spIHXRequestChallenge);

    IHXValues* pValues = NULL;
    if (SUCCEEDED(m_pClassFactory->CreateInstance(CLSID_IHXValues, 
         (void**)&pValues)))
    {
        m_spIHXValuesResponseHeaders = pValues;
        // the smart pointer creates its own ref on assign, so release
        pValues->Release();
    }

    return HXR_OK;
}

HXBOOL 
CHXClientAuthenticator::IsDone()
{
    _CListOfWrapped_IUnknown_::iterator ListOfIUnknownIteratorCurrent;
    DECLARE_SMART_POINTER
    (
	IHXClientAuthConversation
    )					spIHXClientAuthConversationPlugin;
    HXBOOL bDone = FALSE;

    for
    (
	ListOfIUnknownIteratorCurrent = m_ListOfIUnknownPlugins.begin();
	ListOfIUnknownIteratorCurrent != m_ListOfIUnknownPlugins.end();
	++ListOfIUnknownIteratorCurrent
    )
    {
	spIHXClientAuthConversationPlugin = 
	(
	    *ListOfIUnknownIteratorCurrent
	).wrapped_ptr();

	bDone |= spIHXClientAuthConversationPlugin->IsDone();
    }

    return bDone;
}

STDMETHODIMP
CHXClientAuthenticator::Authenticated(HXBOOL bAuthenticated)
{
    if (!bAuthenticated && m_pCredentialsCache)
    {
	m_pCredentialsCache->Empty(m_pRealm);
    }

    return HXR_OK;
}

// IHXClientAuthResponse
STDMETHODIMP 
CHXClientAuthenticator::ResponseReady
(
    HX_RESULT	HX_RESULTStatus,
    IHXRequest* pIHXRequestResponse
)
{
    DECLARE_SMART_POINTER
    (
	IHXClientAuthConversation
    )					spIHXClientAuthConversationPlugin;
    DECLARE_SMART_POINTER(IHXBuffer)	spIHXBufferPropValue;
    DECLARE_SMART_POINTER(IHXValues)	spIHXValuesResponseHeaders;

    if(FAILED(m_HX_RESULTStatus) && HX_RESULTStatus != HXR_FAIL)
    {
	m_HX_RESULTStatus = HX_RESULTStatus;
    }

    if
    (
	SUCCEEDED(m_HX_RESULTStatus) 
	&& 
	pIHXRequestResponse
	&&
	SUCCEEDED
	(
	    pIHXRequestResponse->GetResponseHeaders
	    (
		spIHXValuesResponseHeaders.ptr_reference()
	    )
	)
	&&
	spIHXValuesResponseHeaders.IsValid()
    )
    {
	// Merge the Headers from this call
	CHXHeader::mergeHeaders
	(
	    m_spIHXValuesResponseHeaders,
	    spIHXValuesResponseHeaders
	);
    }

    if (SUCCEEDED(HX_RESULTStatus))
    {
        // if we succeeded then no need to continue iterating through
        // other plugins.
        m_ListOfIUnknownIteratorCurrent = m_ListOfIUnknownPlugins.end();
    }
    else
    {
        ++m_ListOfIUnknownIteratorCurrent;
    }

    if(m_ListOfIUnknownIteratorCurrent != m_ListOfIUnknownPlugins.end())
    {
	// Call Next one..
	spIHXClientAuthConversationPlugin = 
	(
	    *m_ListOfIUnknownIteratorCurrent
	).wrapped_ptr();

	spIHXClientAuthConversationPlugin->MakeResponse
	(
	    this, 
	    m_spIHXRequestChallenge
	);
    }
    else
    {
	m_spIHXRequestChallenge->SetResponseHeaders
	(
	    m_spIHXValuesResponseHeaders
	);

	m_spIHXClientAuthResponseRequester->ResponseReady
	(
	    m_HX_RESULTStatus, 
	    m_spIHXRequestChallenge
	);

	// We are done, cleanup..
	m_spIHXClientAuthResponseRequester.Release();
	m_spIHXValuesResponseHeaders.Release();
	m_spIHXRequestChallenge.Release();
    }

    return HXR_OK;
}

// IHXCredRequest
STDMETHODIMP 
CHXClientAuthenticator::GetCredentials
(
    IHXCredRequestResponse* pIHXCredRequestResponseRequester,
    IHXValues* pIHXValuesCredentialRequest
)
{

    if(pIHXValuesCredentialRequest)
    {
	m_spIHXValuesCredentialRequest = pIHXValuesCredentialRequest;
    }
    else
    {
        IHXValues* pValues = NULL;
        if (SUCCEEDED(m_pClassFactory->CreateInstance(CLSID_IHXValues,
            (void**)&pValues)))
        {
            m_spIHXValuesCredentialRequest = pValues;
            // the smart pointer creates its own ref on assign, so release
            pValues->Release();
        }
    }

    HX_RELEASE(m_pRealm);

    if (HXR_OK != pIHXValuesCredentialRequest->GetPropertyCString("Realm", m_pRealm))
    {
	m_pRealm = NULL;
    }
    
    // Handle credentials Cache
    if (m_pCredentialsCache &&
	!m_pCredentialsCache->IsEmpty(m_pRealm))
    {
	m_pCredentialsCache->FillCredentials
	(
	    pIHXValuesCredentialRequest
	);

	return pIHXCredRequestResponseRequester->CredentialsReady
	(
	    HXR_OK, 
	    pIHXValuesCredentialRequest
	);
    }

    m_spIHXCredRequestResponseRequester = pIHXCredRequestResponseRequester;

    // Contact the UI
    IHXCredRequest* pCredRequestContext = NULL;

    // XXXkshoop allow existing UI for now
    IHXAuthenticationManager*	pAuthMgr = NULL;
    IHXAuthenticationManager2*	pAuthMgr2 = NULL;

    if
    (
	m_spIUnknownContext.IsValid()
	&&
	SUCCEEDED
	(
	    m_spIUnknownContext->QueryInterface
	    (
		IID_IHXCredRequest,
		(void**)&pCredRequestContext
	    )
	)
	&&
	pCredRequestContext
    )
    {
	pCredRequestContext->GetCredentials
	(
	    this,
	    pIHXValuesCredentialRequest
	);
    }
    else if
    (
	m_spIUnknownContext.IsValid()
	&&
	SUCCEEDED
	(
	    m_spIUnknownContext->QueryInterface
	    (
		IID_IHXAuthenticationManager2,
		(void**)&pAuthMgr2
	    )
	)
	&&
	pAuthMgr2
    )
    {
	pAuthMgr2->HandleAuthenticationRequest2(this, m_spIHXValuesCredentialRequest);
    }
    else if
    (
	m_spIUnknownContext.IsValid()
	&&
	SUCCEEDED
	(
	    m_spIUnknownContext->QueryInterface
	    (
		IID_IHXAuthenticationManager,
		(void**)&pAuthMgr
	    )
	)
	&&
	pAuthMgr
    )
    {
	pAuthMgr->HandleAuthenticationRequest(this);
    }
    else
    {
	// The current context does not support a username/password dialog.
	CredentialsReady(HXR_NOTIMPL, NULL);
    }

    HX_RELEASE(pAuthMgr);
    HX_RELEASE(pCredRequestContext);

    return HXR_OK;
}

// IHXCredRequestResponse
STDMETHODIMP 
CHXClientAuthenticator::CredentialsReady
(
    HX_RESULT	HX_RESULTStatus,
    IHXValues* pIHXValuesCredentials
)
{
    // Handle credentials Cache
    if (m_pCredentialsCache)
    {
	if 
	(
	    SUCCEEDED(HX_RESULTStatus) 
	    &&
	    m_pCredentialsCache->IsEmpty(m_pRealm)
	)
	{
	    m_pCredentialsCache->SetCredentials
	    (
		pIHXValuesCredentials
	    );
	}
	else if (FAILED(HX_RESULTStatus))
	{
	    m_pCredentialsCache->Empty(m_pRealm);
	}
    }

    // Forward credentials to requestor
    m_spIHXCredRequestResponseRequester->CredentialsReady
    (
	HX_RESULTStatus, 
	pIHXValuesCredentials
    );

    return HXR_OK;
}

// IHXAuthenticationManagerResponse
STDMETHODIMP 
CHXClientAuthenticator::AuthenticationRequestDone
(
    HX_RESULT result,
    const char* user,
    const char* password
)
{

    DECLARE_SMART_POINTER(IHXBuffer) spIHXBufferPropValue;

    if(SUCCEEDED(result))
    {
	if(user && *user)
	{
	    if
	    (
		SUCCEEDED
		(
		    CHXBuffer::FromCharArray(user, &spIHXBufferPropValue)
		)
	    )
	    {
		m_spIHXValuesCredentialRequest->SetPropertyCString
		(
		    "Username", 
		    spIHXBufferPropValue
		);
	    }
	}

	spIHXBufferPropValue.Release();

	if(password && *password)
	{
	    if
	    (
		SUCCEEDED
		(
		    CHXBuffer::FromCharArray
		    (
			password, 
			&spIHXBufferPropValue
		    )
		)
	    )
	    {
		m_spIHXValuesCredentialRequest->SetPropertyCString
		(
		    "Password", 
		    spIHXBufferPropValue
		);
	    }
	}
    }

    return CredentialsReady(result, m_spIHXValuesCredentialRequest);
}

HX_RESULT 
CHXClientAuthenticator::_TryToLoadPlugins()
{
    HX_RESULT HX_RESULTRet = HXR_FAIL;

    if
    (
	m_spIUnknownContext.IsValid() && 
	m_ListOfIUnknownPlugins.begin() == m_ListOfIUnknownPlugins.end()
    )
    {
	DECLARE_SMART_POINTER_UNKNOWN	spIUnknownPlugin;
	UINT32				ulIndex = 0;
	UINT32				ulNumPlugins = 0;

#ifdef HELIX_FEATURE_CLIENT
	// Retrieve all plugins which support this search prop and data value.
	SPIHXPluginHandler3 spHandler(m_spIUnknownContext);
	SPIHXPluginSearchEnumerator spIEnumerator;
	if (spHandler.IsValid())
	{
	    spHandler->FindGroupOfPluginsUsingStrings(PLUGIN_CLASS, PLUGIN_CLASS_AUTH_TYPE, NULL, NULL, NULL, NULL, *(spIEnumerator.AsInOutParam()));

            if (spIEnumerator)
            {
                ulNumPlugins = spIEnumerator->GetNumPlugins();

	        for (ulIndex = 0; ulIndex < ulNumPlugins ; ulIndex++)
	        {
	            spIUnknownPlugin.Release();
	            HX_RESULTRet = spIEnumerator->GetPluginAt(ulIndex, spIUnknownPlugin.ptr_reference(), NULL);

	            _AddAuthenticationPlugin(spIUnknownPlugin);
	        }
                HX_RESULTRet = HXR_OK;
            }
        }
#else // HELIX_FEATURE_CLIENT
	DECLARE_SMART_POINTER(IHXPluginEnumerator) spIHXPluginEnumeratorPlugins;
        spIHXPluginEnumeratorPlugins = m_spIUnknownContext;

	if(spIHXPluginEnumeratorPlugins.IsValid())
	{
	    ulNumPlugins = spIHXPluginEnumeratorPlugins->GetNumOfPlugins();

	    // Load all auth plugins
	    for(ulIndex = 0; ulIndex < ulNumPlugins; ++ulIndex)
	    {
		spIUnknownPlugin.Release();
		spIHXPluginEnumeratorPlugins->GetPlugin
		(
		    ulIndex, 
		    spIUnknownPlugin.ptr_reference()
		);

		_AddAuthenticationPlugin(spIUnknownPlugin);
	    }

	    HX_RESULTRet = HXR_OK;
	}
#endif // HELIX_FEATURE_CLIENT
    }

    return HX_RESULTRet;
}

HX_RESULT
CHXClientAuthenticator::_AddAuthenticationPlugin(IUnknown* pUnk)
{
    DECLARE_SMART_POINTER(IHXClientAuthConversation) spIHXClientAuthConversationPlugin;
    DECLARE_SMART_POINTER(IHXObjectConfiguration) spIHXObjectConfigurationPlugin;

    spIHXClientAuthConversationPlugin = pUnk;
    spIHXObjectConfigurationPlugin = pUnk;

    if (spIHXClientAuthConversationPlugin.IsValid() &&
	spIHXObjectConfigurationPlugin.IsValid())
    {
	spIHXObjectConfigurationPlugin->SetContext(m_spIUnknownContext);

	// Insert at end!! Order matters!!
	m_ListOfIUnknownPlugins.insert
	(
	    m_ListOfIUnknownPlugins.end(), 
	    pUnk
	);
    }
    return HXR_OK;
}




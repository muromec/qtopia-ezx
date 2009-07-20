/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cliauth.h,v 1.6 2006/05/16 17:31:06 chopaul Exp $
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

#ifndef __CClientAuthenticator__
#define __CClientAuthenticator__

#include "hxplgns.h"

#include "unkimp.h"
#include "smartptr.h"
#include "hxpktsp.h"
#include "hxfilsp.h"
#include "hxathsp.h"
#include "hxplnsp.h"
#include "miscsp.h"
#include "hxcredc.h"

_INTERFACE IHXCommonClassFactory;

typedef WRAPPED_POINTER(IUnknown) Wrapped_IUnknown;

class _CListOfWrapped_IUnknown_Node
{
public:
    _CListOfWrapped_IUnknown_Node();
    ~_CListOfWrapped_IUnknown_Node();

    Wrapped_IUnknown& value();
    const Wrapped_IUnknown& value() const;
    void value(const Wrapped_IUnknown& rclsNewValue);
    _CListOfWrapped_IUnknown_Node& operator=(const Wrapped_IUnknown& rclsNewValue); 
    _CListOfWrapped_IUnknown_Node* next() const;
    void next(_CListOfWrapped_IUnknown_Node* plocnNew);
    _CListOfWrapped_IUnknown_Node* prev() const;
    void prev(_CListOfWrapped_IUnknown_Node* plocnNew);
    void Remove();
    void Insert(_CListOfWrapped_IUnknown_Node& rlocnNew);
						
protected:
    Wrapped_IUnknown m_clsValue;
    _CListOfWrapped_IUnknown_Node* m_plocPrev;
    _CListOfWrapped_IUnknown_Node* m_plocNext;
    _CListOfWrapped_IUnknown_Node(const _CListOfWrapped_IUnknown_Node& rlocnOther){}
};

class _CListIteratorWrapped_IUnknown_;
class _CListReverseIteratorWrapped_IUnknown_;

class _CListOfWrapped_IUnknown_
{
public:
    typedef _CListIteratorWrapped_IUnknown_ iterator;
    typedef _CListReverseIteratorWrapped_IUnknown_ reverse_iterator;
    typedef const _CListReverseIteratorWrapped_IUnknown_ const_reverse_iterator;

    _CListOfWrapped_IUnknown_();
    _CListOfWrapped_IUnknown_(const _CListOfWrapped_IUnknown_& rlocOther);
    ~_CListOfWrapped_IUnknown_();
    _CListOfWrapped_IUnknown_& operator=(const _CListOfWrapped_IUnknown_& rlocOther); 

    iterator begin();
    const iterator begin() const;
    iterator end();
    const iterator end() const;

    reverse_iterator rbegin();
    const reverse_iterator rbegin() const;
    reverse_iterator rend();
    const reverse_iterator rend() const;

    iterator insert(iterator itBefore, const Wrapped_IUnknown&);
    void insert
    (
	iterator itBefore,
	const iterator itFirst,
	const iterator itLast
    );
    void remove(iterator itThis);
    void remove(iterator itFirst, iterator itLast);

    void empty();

protected:
    _CListOfWrapped_IUnknown_Node m_locnREnd;
    _CListOfWrapped_IUnknown_Node m_locnEnd;

    void _copy(const _CListOfWrapped_IUnknown_& rlocOther);
};

class _CListIteratorWrapped_IUnknown_
{
public:
    _CListIteratorWrapped_IUnknown_();
    _CListIteratorWrapped_IUnknown_
    (
        const _CListOfWrapped_IUnknown_Node& rlocnNewLocation
    );
    _CListIteratorWrapped_IUnknown_(const _CListIteratorWrapped_IUnknown_& rliocOther);
    ~_CListIteratorWrapped_IUnknown_();

    _CListIteratorWrapped_IUnknown_& operator=
    (
	const _CListIteratorWrapped_IUnknown_& rliocOther
    );

    Wrapped_IUnknown& operator*();
    _CListIteratorWrapped_IUnknown_& operator=(const Wrapped_IUnknown& rclsNewValue);

    _CListIteratorWrapped_IUnknown_& operator++();
    const _CListIteratorWrapped_IUnknown_ operator++(int);

    _CListIteratorWrapped_IUnknown_& operator--();
    const _CListIteratorWrapped_IUnknown_ operator--(int);

private:
    _CListOfWrapped_IUnknown_Node* m_plocCurrent;

    friend class _CListOfWrapped_IUnknown_;
    friend HXBOOL operator==
    (
	const _CListIteratorWrapped_IUnknown_& rliocLeft,
	const _CListIteratorWrapped_IUnknown_& rliocRight
    );
    friend HXBOOL operator!=
    (
	const _CListIteratorWrapped_IUnknown_& rliocLeft,
	const _CListIteratorWrapped_IUnknown_& rliocRight
    );
};

HXBOOL operator==
(
    const _CListIteratorWrapped_IUnknown_& rliocLeft,
    const _CListIteratorWrapped_IUnknown_& rliocRight
);

HXBOOL operator!=
(
    const _CListIteratorWrapped_IUnknown_& rliocLeft,
    const _CListIteratorWrapped_IUnknown_& rliocRight
);

class _CListReverseIteratorWrapped_IUnknown_
{
public:
    _CListReverseIteratorWrapped_IUnknown_();
    _CListReverseIteratorWrapped_IUnknown_
    (
        const _CListOfWrapped_IUnknown_Node& rlocnNewLocation
    );
    _CListReverseIteratorWrapped_IUnknown_
    (
	const _CListReverseIteratorWrapped_IUnknown_& rlriocOther
    );
    ~_CListReverseIteratorWrapped_IUnknown_();

    _CListReverseIteratorWrapped_IUnknown_& operator=
    (
	const _CListReverseIteratorWrapped_IUnknown_& rlriocOther
    );

    Wrapped_IUnknown& operator*();
    _CListReverseIteratorWrapped_IUnknown_& operator=(const Wrapped_IUnknown& rclsNewValue);

    _CListReverseIteratorWrapped_IUnknown_& operator++();
    const _CListReverseIteratorWrapped_IUnknown_ operator++(int);
    _CListReverseIteratorWrapped_IUnknown_& operator--();
    const _CListReverseIteratorWrapped_IUnknown_ operator--(int);

private:
    _CListOfWrapped_IUnknown_Node* m_plocCurrent;
    friend class _CListOfWrapped_IUnknown_;
    friend HXBOOL operator==
    (
	const _CListReverseIteratorWrapped_IUnknown_& rlriocLeft,
	const _CListReverseIteratorWrapped_IUnknown_& rlriocRight
    );
    friend HXBOOL operator!=
    (
	const _CListReverseIteratorWrapped_IUnknown_& rlriocLeft,
	const _CListReverseIteratorWrapped_IUnknown_& rlriocRight
    );
};

HXBOOL operator==
(
    const _CListReverseIteratorWrapped_IUnknown_& rlriocLeft,
    const _CListReverseIteratorWrapped_IUnknown_& rlriocRight
);
HXBOOL operator!=
(
    const _CListReverseIteratorWrapped_IUnknown_& rlriocLeft,
    const _CListReverseIteratorWrapped_IUnknown_& rlriocRight
);								    

class CHXClientAuthenticator
    : public CUnknownIMP
    , public IHXObjectConfiguration
    , public IHXClientAuthConversation
    , public IHXClientAuthResponse
    , public IHXCredRequest
    , public IHXCredRequestResponse
    , public IHXAuthenticationManagerResponse
{
    DECLARE_UNKNOWN(CHXClientAuthenticator)

public:

    CHXClientAuthenticator();
    virtual ~CHXClientAuthenticator();

    // IHXObjectConfiguration
    STDMETHOD(SetContext)(IUnknown* pIUnknownContext);
    STDMETHOD(SetConfiguration)
    (
	IHXValues* pIHXValuesConfiguration
    );

    // IHXClientAuthConversation
    STDMETHOD(MakeResponse)
    (
	IHXClientAuthResponse* pIHXClientAuthResponseRequester,
	IHXRequest* pIHXRequestChallenge
    );
    STDMETHOD_(HXBOOL,IsDone)();
    STDMETHOD(Authenticated)(HXBOOL bAuthenticated);

    // IHXClientAuthResponse
    STDMETHOD(ResponseReady)
    (
	HX_RESULT	HX_RESULTStatus,
	IHXRequest* pIHXRequestResponse
    );

    // IHXCredRequest
    STDMETHOD(GetCredentials)
    (
	IHXCredRequestResponse* pIHXCredRequestResponseRequester,
	IHXValues* pIHXValuesCredentialRequest
    );

    // IHXCredRequestResponse
    STDMETHOD(CredentialsReady)
    (
	HX_RESULT	HX_RESULTStatus,
	IHXValues* pIHXValuesCredentials
    );

    // IHXAuthenticationManagerResponse
    STDMETHOD(AuthenticationRequestDone)
    (
	HX_RESULT HX_RESULTStatus,
	const char* pcharUser,
	const char* pcharPassword
    );

private:
    HX_RESULT _TryToLoadPlugins();
    HX_RESULT _AddAuthenticationPlugin(IUnknown* pUnk);

    DECLARE_SMART_POINTER_UNKNOWN	m_spIUnknownContext;
    DECLARE_SMART_POINTER(IHXValues)	m_spIHXValuesConfig;
    DECLARE_SMART_POINTER(IHXValues)	m_spIHXValuesResponseHeaders;
    DECLARE_SMART_POINTER(IHXRequest)	m_spIHXRequestChallenge;
    DECLARE_SMART_POINTER(IHXValues)	m_spIHXValuesCredentialRequest;
    DECLARE_SMART_POINTER
    (
	IHXClientAuthResponse
    )					m_spIHXClientAuthResponseRequester;
    DECLARE_SMART_POINTER
    (
	IHXCredRequestResponse
    )					m_spIHXCredRequestResponseRequester;
    _CListOfWrapped_IUnknown_		m_ListOfIUnknownPlugins;
    _CListOfWrapped_IUnknown_::iterator
					m_ListOfIUnknownIteratorCurrent;
    HX_RESULT				m_HX_RESULTStatus;
    IHXRegistry*			m_pRegistry;
    IHXBuffer*				m_pRealm;
    IHXCredentialsCache*		m_pCredentialsCache;
    IHXCommonClassFactory*              m_pClassFactory;
};


#endif // !__CClientAuthenticator__

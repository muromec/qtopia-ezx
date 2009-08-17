/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: winreg.cpp,v 1.6 2005/03/14 19:36:40 bobclark Exp $
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
#include "hxresult.h"
#include "hxstring.h"
#include "hxwinreg.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

_CListOfCHXString_Node::_CListOfCHXString_Node()
  : m_plocPrev(NULL)
  , m_plocNext(NULL)
{
}

_CListOfCHXString_Node::~_CListOfCHXString_Node()
{
    Remove();
}

void
_CListOfCHXString_Node::Remove()
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
_CListOfCHXString_Node::Insert(_CListOfCHXString_Node& rlocnNew)
{
    rlocnNew.next(this);
    rlocnNew.prev(m_plocPrev);

    if(m_plocPrev)
    {
	m_plocPrev->next(&rlocnNew);
    }
	
    m_plocPrev = &rlocnNew;
}

CHXString&
_CListOfCHXString_Node::value()
{
    return m_clsValue;
}

const CHXString&
_CListOfCHXString_Node::value() const
{
    return m_clsValue;
}

void
_CListOfCHXString_Node::value(const CHXString& rclsNewValue)
{
    m_clsValue = rclsNewValue;
}

_CListOfCHXString_Node&
_CListOfCHXString_Node::operator=(const CHXString& rclsNewValue)
{
    m_clsValue = rclsNewValue;
    return *this;
}

_CListOfCHXString_Node*
_CListOfCHXString_Node::next() const
{
    return m_plocNext;
}

void
_CListOfCHXString_Node::next(_CListOfCHXString_Node* plocnNew)
{
    m_plocNext = plocnNew;
}

_CListOfCHXString_Node*
_CListOfCHXString_Node::prev() const
{
    return m_plocPrev;
}

void
_CListOfCHXString_Node::prev(_CListOfCHXString_Node* plocnNew)
{
    m_plocPrev = plocnNew;
}

_CListOfCHXString_::_CListOfCHXString_()
{
    m_locnREnd.next(&m_locnEnd);
    m_locnEnd.prev(&m_locnREnd);
}

_CListOfCHXString_::_CListOfCHXString_(const _CListOfCHXString_& rlocOther)
{
    m_locnREnd.next(&m_locnEnd);
    m_locnEnd.prev(&m_locnREnd);

    _copy(rlocOther);
}

_CListOfCHXString_::~_CListOfCHXString_()
{
    empty();
}

_CListOfCHXString_&
_CListOfCHXString_::operator=(const _CListOfCHXString_& rlocOther)
{
    empty();
    _copy(rlocOther);

    return *this;
}

void
_CListOfCHXString_::_copy(const _CListOfCHXString_& rlocOther)
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

_CListOfCHXString_::iterator
_CListOfCHXString_::begin()
{
    return iterator(*(m_locnREnd.next()));
}

const _CListOfCHXString_::iterator
_CListOfCHXString_::begin() const
{
    return iterator(*(m_locnREnd.next()));
}

_CListOfCHXString_::iterator
_CListOfCHXString_::end()
{
    return iterator(m_locnEnd);
}

const _CListOfCHXString_::iterator
_CListOfCHXString_::end() const
{
    return iterator(m_locnEnd);
}

_CListOfCHXString_::reverse_iterator
_CListOfCHXString_::rbegin()
{
    return reverse_iterator(*(m_locnEnd.prev()));
}

const _CListOfCHXString_::reverse_iterator 
_CListOfCHXString_::rbegin() const
{
    return const_reverse_iterator(*(m_locnEnd.prev()));
}

_CListOfCHXString_::reverse_iterator
_CListOfCHXString_::rend()
{
    return reverse_iterator(m_locnREnd);
}

const _CListOfCHXString_::reverse_iterator
_CListOfCHXString_::rend() const
{
    return const_reverse_iterator(*((const _CListOfCHXString_Node *)&m_locnREnd));
}

_CListOfCHXString_::iterator
_CListOfCHXString_::insert(iterator itBefore, const CHXString& rclsNew)
{
    _CListOfCHXString_Node* plocnNew = new _CListOfCHXString_Node;

    HX_ASSERT(plocnNew);

    *plocnNew = rclsNew;

    itBefore.m_plocCurrent->Insert(*plocnNew);

    return iterator(*plocnNew);
}

void
_CListOfCHXString_::insert
(
    iterator itBefore,
    const iterator itFirst,
    const iterator itLast
)
{
    iterator itOther;
    _CListOfCHXString_Node* plocnNew;

    for (itOther = itFirst; itOther != itLast; ++itOther)
    {
	plocnNew = new _CListOfCHXString_Node;

	HX_ASSERT(plocnNew);

	*plocnNew = *itOther;

	itBefore.m_plocCurrent->Insert(*plocnNew);
    }
}

void
_CListOfCHXString_::remove(iterator itThis)
{
    if
    (
	itThis.m_plocCurrent == &m_locnEnd ||
	itThis.m_plocCurrent == &m_locnREnd
    )
    {
	return;
    }

    _CListOfCHXString_Node* plocnOld;

    plocnOld = itThis.m_plocCurrent;

    ++itThis;

    plocnOld->Remove();

    delete plocnOld;
}

void
_CListOfCHXString_::remove(iterator itFirst, iterator itLast)
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
    _CListOfCHXString_Node* plocnOld;

    for (itOther = itFirst; itOther != itLast;)
    {
	plocnOld = itOther.m_plocCurrent;

	++itOther;

	plocnOld->Remove();

	delete plocnOld;
    }
}

void
_CListOfCHXString_::empty()
{
    remove(begin(), end());
}

_CListIteratorCHXString_::_CListIteratorCHXString_()
  : m_plocCurrent(NULL)
{
}

_CListIteratorCHXString_::_CListIteratorCHXString_
(
    const _CListOfCHXString_Node& rlocnNewLocation
)
  : m_plocCurrent((_CListOfCHXString_Node*)&rlocnNewLocation)
{
}

_CListIteratorCHXString_::_CListIteratorCHXString_
(
    const _CListIteratorCHXString_& rliocOther
)
  : m_plocCurrent(rliocOther.m_plocCurrent)
{
}

_CListIteratorCHXString_::~_CListIteratorCHXString_()
{
}

_CListIteratorCHXString_&
_CListIteratorCHXString_::operator=
(
    const _CListIteratorCHXString_& rliocOther
)
{
    m_plocCurrent = rliocOther.m_plocCurrent;

    return *this;
}

CHXString&
_CListIteratorCHXString_::operator*()
{
    HX_ASSERT(m_plocCurrent);
    return m_plocCurrent->value();
}

_CListIteratorCHXString_&
_CListIteratorCHXString_::operator=(const CHXString& rclsNewValue)
{
    if(!m_plocCurrent)
	return *this;

    m_plocCurrent->value(rclsNewValue);

    return *this;
}

_CListIteratorCHXString_&
_CListIteratorCHXString_::operator++()
{
    if(!m_plocCurrent)
	return *this;

    m_plocCurrent = m_plocCurrent->next();

    return *this;
}

const _CListIteratorCHXString_
_CListIteratorCHXString_::operator++(int)
{
    _CListIteratorCHXString_ liocRet(*this);

    ++(*this);

    return liocRet;
}

_CListIteratorCHXString_&
_CListIteratorCHXString_::operator--()
{
    if(!m_plocCurrent)
	return *this;

    m_plocCurrent = m_plocCurrent->prev();

    return *this;
}

const _CListIteratorCHXString_
_CListIteratorCHXString_::operator--(int)
{
    _CListIteratorCHXString_ liocRet(*this);

    --(*this);

    return liocRet;
}

HXBOOL operator==
(
    const _CListIteratorCHXString_& rliocLeft,
    const _CListIteratorCHXString_& rliocRight
)
{
    return (rliocLeft.m_plocCurrent == rliocRight.m_plocCurrent);
}

HXBOOL operator!=
(
    const _CListIteratorCHXString_& rliocLeft,
    const _CListIteratorCHXString_& rliocRight
)
{
    return (rliocLeft.m_plocCurrent != rliocRight.m_plocCurrent);
}

_CListReverseIteratorCHXString_::_CListReverseIteratorCHXString_()
  : m_plocCurrent(NULL)
{
}

_CListReverseIteratorCHXString_::_CListReverseIteratorCHXString_
(
    const _CListOfCHXString_Node& rlocnNewLocation
)
  : m_plocCurrent((_CListOfCHXString_Node*)&rlocnNewLocation)
{
}

_CListReverseIteratorCHXString_::_CListReverseIteratorCHXString_
(
    _CListReverseIteratorCHXString_& rlriocOther
)
  : m_plocCurrent(rlriocOther.m_plocCurrent)
{
}

_CListReverseIteratorCHXString_::~_CListReverseIteratorCHXString_()
{
}

_CListReverseIteratorCHXString_&
_CListReverseIteratorCHXString_::operator=
(
    const _CListReverseIteratorCHXString_& rlriocOther
)
{
    m_plocCurrent = rlriocOther.m_plocCurrent;
    return *this;
}

CHXString&
_CListReverseIteratorCHXString_::operator*()
{
    HX_ASSERT(m_plocCurrent);
    return m_plocCurrent->value();
}

_CListReverseIteratorCHXString_&
_CListReverseIteratorCHXString_::operator=(const CHXString& rclsNewValue)
{
    if(!m_plocCurrent)
	return *this;

    m_plocCurrent->value(rclsNewValue);

    return *this;
}

_CListReverseIteratorCHXString_&
_CListReverseIteratorCHXString_::operator++()
{
    if(!m_plocCurrent)
	return *this;

    m_plocCurrent = m_plocCurrent->prev();

    return *this;
}

const _CListReverseIteratorCHXString_
_CListReverseIteratorCHXString_::operator++(int)
{
    _CListReverseIteratorCHXString_ lriocRet(*this);

    ++(*this);

    return lriocRet;
}

_CListReverseIteratorCHXString_&
_CListReverseIteratorCHXString_::operator--()
{
    if(!m_plocCurrent)
	return *this;

    m_plocCurrent = m_plocCurrent->next();

    return *this;
}

const _CListReverseIteratorCHXString_
_CListReverseIteratorCHXString_::operator--(int)
{
    _CListReverseIteratorCHXString_ lriocRet(*this);

    --(*this);

    return lriocRet;
}

HXBOOL operator==
(
    const _CListReverseIteratorCHXString_& rlriocLeft,
    const _CListReverseIteratorCHXString_& rlriocRight
)
{
    return (rlriocLeft.m_plocCurrent == rlriocRight.m_plocCurrent);
}

HXBOOL operator!=
(
    const _CListReverseIteratorCHXString_& rlriocLeft,
    const _CListReverseIteratorCHXString_& rlriocRight
)
{
    return (rlriocLeft.m_plocCurrent != rlriocRight.m_plocCurrent);
}

CWinRegKey::CWinRegKey()
    : m_hkRoot(NULL)
    , m_hkThis(NULL)
    , m_dwKeyEnumPos(0)
    , m_dwValueEnumPos(0)
    , m_rsAccess(KEY_READ)
    , m_bOpen(FALSE)
{
}

CWinRegKey::~CWinRegKey()
{
    Close();
}

HX_RESULT 
CWinRegKey::Open()
{
    if(!m_hkRoot || m_sPath.IsEmpty())
    {
	return HXR_UNEXPECTED;
    }

    Close();

    HX_RESULT pnrRes = HXR_FAIL;
    if
    (
	SUCCEEDED
	(
	    pnrRes = HRESULT_FROM_WIN32
	    (
		RegOpenKeyEx
		(
		    m_hkRoot,
		    OS_STRING(m_sPath),
		    0,
		    m_rsAccess,
		    &m_hkThis
		)
	    )
	)
    )
    {
	m_bOpen = TRUE;
    }

    return pnrRes;
}

HX_RESULT 
CWinRegKey::Create(const char* szClass, DWORD dwOptions)
{
    if(!m_hkRoot || m_sPath.IsEmpty())
    {
	return HXR_UNEXPECTED;
    }

    Close();

    DWORD dwIngnored;

    if
    (
	RegCreateKeyEx
	(
	    m_hkRoot,
	    OS_STRING(m_sPath),
	    0,
	    OS_STRING(szClass),
	    dwOptions,
	    m_rsAccess,
	    NULL,
	    &m_hkThis,
	    &dwIngnored
	)
	==
	ERROR_SUCCESS
    )
    {
	m_bOpen = TRUE;

	return HXR_OK;
    }

    return HXR_FAIL;
}

HX_RESULT 
CWinRegKey::Close()
{
    if(!m_bOpen)
    {
	return HXR_UNEXPECTED;
    }

    if
    (
	RegCloseKey(m_hkThis)
	==
	ERROR_SUCCESS
    )
    {
	m_bOpen = FALSE;

	return HXR_OK;
    }

    return HXR_FAIL;
}

HX_RESULT 
CWinRegKey::Flush()
{
    if(!m_bOpen)
    {
	return HXR_UNEXPECTED;
    }

    if
    (
	RegFlushKey(m_hkThis)
	==
	ERROR_SUCCESS
    )
    {
	return HXR_OK;
    }

    return HXR_FAIL;
}

HX_RESULT 
CWinRegKey::DeleteSubKey(const char* szName)
{
    if(!m_bOpen)
    {
	return HXR_UNEXPECTED;
    }

    HX_RESULT	pnrRes = HXR_FAIL;
    CWinRegKey wrkExpired;
    CWinRegKey wrkSub;

    wrkExpired.SetRootKey(m_hkThis);
    wrkExpired.SetRelativePath(szName);
    wrkExpired.SetDesiredAccess(KEY_ENUMERATE_SUB_KEYS|KEY_CREATE_SUB_KEY);

    if (SUCCEEDED(pnrRes = wrkExpired.Open()))
    {
	wrkExpired.ResetKeyEnumerator();

	while(wrkExpired.GetNextKey(wrkSub))
	{
	    wrkExpired.DeleteSubKey(wrkSub.GetRelativePath());
	    wrkExpired.ResetKeyEnumerator();
	}

	wrkExpired.Close();

	pnrRes = HRESULT_FROM_WIN32(RegDeleteKey(m_hkThis, OS_STRING(szName)));
    }

    return pnrRes;
}


HXBOOL 
CWinRegKey::DoesExist()
{
    if(m_bOpen)
    {
	return TRUE;
    }

    if(SUCCEEDED(Open()))
    {
	Close();
	return TRUE;
    }
    return FALSE;
}

HXBOOL 
CWinRegKey::SetDesiredAccess(REGSAM rsNew)
{
    if(m_bOpen)
    {
	return FALSE;
    }

    m_rsAccess = rsNew;

    return TRUE;
}

REGSAM 
CWinRegKey::GetDesiredAccess()
{
    return m_rsAccess;
}

HXBOOL 
CWinRegKey::SetRootKey(HKEY hkRoot)
{
    if(m_bOpen)
    {
	return FALSE;
    }

    m_hkRoot = hkRoot;

    return TRUE;
}

HKEY 
CWinRegKey::GetRootKey()
{
    return m_hkRoot;
}


HKEY 
CWinRegKey::GetHandle()
{
    return m_hkThis;
}


// According to Article ID: Q117261
//
// A call to RegCreateKeyEx() is successful under Windows NT version 3.1 
// and Windows 95, but the call fails with error 161 (ERROR_BAD_PATHNAME) 
// under Windows NT version 3.5 and later. 
//
// This is by design. Windows NT version 3.1 and Windows 95 allow the 
// subkey to begin with a backslash ("\"), however Windows NT version 3.5 
// and later do not. The subkey is given as the second parameter to 
// RegCreateKeyEx(). 
//

HXBOOL 
CWinRegKey::SetRelativePath(const char* szPath)
{
    if(m_bOpen)
    {
	return FALSE;
    }

    if (*szPath == '\\')
    {
	m_sPath = szPath+1;
    }
    else
    {
	m_sPath = szPath;
    }

    return TRUE;
}

CHXString& 
CWinRegKey::GetRelativePath()
{
    return m_sPath;
}

HXBOOL 
CWinRegKey::GetValue
(
    const char* szName, 
    AWinRegValue** ppwrvOut, 
    UINT32 ulType
)
{
    if
    (
	!m_bOpen 
	|| 
	!szName 
	|| 
	!(*szName)
	||
	!ppwrvOut
    )
    {
	return FALSE;
    }

    *ppwrvOut = NULL;

    if (!ulType)
    {
	if
	(
	    RegQueryValueEx
	    (
		m_hkThis,
		OS_STRING(szName),
		NULL,
		&ulType,
		NULL,
		NULL
	    )
	    !=
	    ERROR_SUCCESS
	)
	{
	    return FALSE;
	}
    }

    switch(ulType)
    {
    case REG_DWORD:
	{
	    *ppwrvOut = (AWinRegValue*)new CWinRegDWORDValue
	    (
		szName, 
		m_hkThis
	    );
	}
	break;
    case REG_SZ:
	{
	    *ppwrvOut = (AWinRegValue*)new CWinRegStringValue
	    (
		szName, 
		m_hkThis
	    );
	}
	break;
    case REG_MULTI_SZ:
	{
	    *ppwrvOut = (AWinRegValue*)new CWinRegStringArrayValue
	    (
		szName, 
		m_hkThis
	    );
	}
	break;
    default:
	{
	}
	break;
    };
    
    return (*ppwrvOut)?TRUE:FALSE;
}

void 
CWinRegKey::FreeValue(AWinRegValue*& pwrvExpired)
{
    delete pwrvExpired;
    pwrvExpired = NULL;
}

HXBOOL 
CWinRegKey::ResetKeyEnumerator()
{
    if(!m_bOpen)
    {
	return FALSE;
    }

    m_dwKeyEnumPos = 0;

    return TRUE;
}

HXBOOL 
CWinRegKey::GetNextKey(CWinRegKey& rwrkNext)
{
    if(!m_bOpen)
    {
	return FALSE;
    }
    
    char	szName[128]; /* Flawfinder: ignore */
    UINT32	ulSizeName=128;
    char	szClass[128]; /* Flawfinder: ignore */
    UINT32	ulSizeClass=128;
    FILETIME	ftLastWrite;

    if
    (
	RegEnumKeyEx
	(
	    m_hkThis,
	    m_dwKeyEnumPos,
	    OS_STRING2(szName, ulSizeName),
	    &ulSizeName,
	    NULL,
	    OS_STRING2(szClass, ulSizeClass),
	    &ulSizeClass,
	    &ftLastWrite
	)
	==
	ERROR_SUCCESS
    )
    {
	++m_dwKeyEnumPos;

	rwrkNext.SetRootKey(m_hkThis);
	rwrkNext.SetRelativePath(szName);
	rwrkNext.SetDesiredAccess(m_rsAccess);

	return TRUE;
    }

    return FALSE;
}


HXBOOL 
CWinRegKey::ResetValueEnumerator()
{
    if(!m_bOpen)
    {
	return FALSE;
    }

    m_dwValueEnumPos = 0;

    return TRUE;
}

HXBOOL 
CWinRegKey::GetNextValue(AWinRegValue** ppwrvNext)
{
    if(!m_bOpen)
    {
	return FALSE;
    }

    char	szName[128]; /* Flawfinder: ignore */
    UINT32	ulSizeName=128;
    UINT32	ulType;

    if
    (
	RegEnumValue
	(
	    m_hkThis,
	    m_dwValueEnumPos,
	    OS_STRING2(szName, ulSizeName),
	    &ulSizeName,
	    NULL,
	    &ulType,
	    NULL,
	    NULL
	)
	==
	ERROR_SUCCESS
    )
    {
	++m_dwValueEnumPos;

	return GetValue(szName, ppwrvNext, ulType);
    }

    return FALSE;
}


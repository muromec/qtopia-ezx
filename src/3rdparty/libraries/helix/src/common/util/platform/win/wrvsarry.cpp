/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: wrvsarry.cpp,v 1.6 2005/03/14 19:36:40 bobclark Exp $
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
#include "hxresult.h"
#include "hxstring.h"
#include "hxwinreg.h"
#include "hxstrutl.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

CWinRegStringArrayValue::CWinRegStringArrayValue
(
    const char* szName, 
    HKEY hkParent
)
    : AWinRegValue(szName, hkParent)
{
}

CWinRegStringArrayValue::~CWinRegStringArrayValue()
{
}

CWinRegStringArrayValue& 
CWinRegStringArrayValue::operator=
(
    const _CListOfCHXString_& rilosNewValue
)
{
    FromStringArray(rilosNewValue);

    return *this;
}

HXBOOL 
CWinRegStringArrayValue::IsStringArray()
{
    UINT32 ulType;
    if
    (
	RegQueryValueEx
	(
	    m_hkParent,
	    OS_STRING(m_sName),
	    NULL,
	    &ulType,
	    NULL,
	    NULL
	) == ERROR_SUCCESS
	&&
	ulType == REG_MULTI_SZ
    )
    {
	return TRUE;
    }

    return FALSE;
}

HXBOOL 
CWinRegStringArrayValue::AsStringArray(char** pszValue)
{
    UINT32 ulType;
    UINT32 ulSize;
    
    if(!pszValue)
    {
	return FALSE;
    }

    *pszValue = NULL;
    
    if
    (
	RegQueryValueEx
	(
	    m_hkParent,
	    OS_STRING(m_sName),
	    NULL,
	    &ulType,
	    NULL,
	    &ulSize
	) == ERROR_SUCCESS
	&&
	ulType == REG_MULTI_SZ
    )
    {
	if (!ulSize)
	{
	    return TRUE;
	}

	++ulSize;

	*pszValue = new char[ulSize];

	if
	(
	    *pszValue
	    &&
	    RegQueryValueEx
	    (
		m_hkParent,
		OS_STRING(m_sName),
		NULL,
		&ulType,
		(unsigned char*)*pszValue,
		&ulSize
	    ) == ERROR_SUCCESS
	)
	{
	    return TRUE;
	}
    }

    return FALSE;
}

HXBOOL 
CWinRegStringArrayValue::AsStringArray(_CListOfCHXString_& rilosValue)
{
    char* szValue;
    UINT32 ulIndex;
    
    rilosValue.empty();
    
    if
    (
	AsStringArray(&szValue)
    )
    {
	for(ulIndex=0; szValue[ulIndex]; ulIndex += strlen(szValue+ulIndex)+1)
	{
	    rilosValue.insert
	    (
		rilosValue.end(), 
		CHXString(szValue+ulIndex)
	    );
	}

	FreeStringArray(szValue);

	return TRUE;
    }

    return FALSE;
}

HXBOOL 
CWinRegStringArrayValue::FromStringArray
(
    const _CListOfCHXString_& rilosNewValue
)
{
    char* szNewValue=NULL;
    _CListOfCHXString_::iterator itilosCurrent;
    UINT32 ulSize;
    UINT32 ulBufSize;
    enum STATE{ST_COUNT, ST_FILL, ST_DONE};
    STATE stCurrent = ST_COUNT;

    while(stCurrent!=ST_DONE)
    {
	for
	(
	    ulSize=0,
	    itilosCurrent=rilosNewValue.begin();
	    itilosCurrent!=rilosNewValue.end();
	    ++itilosCurrent
	)
	{
	    switch(stCurrent)
	    {
	    case ST_COUNT:
		{
		}
		break;
	    case ST_FILL:
		{
		    SafeStrCpy
		    (
			szNewValue+ulSize, 
			(*itilosCurrent).GetBuffer(1), ulBufSize
		    );
		}
		break;
	    default:
		break;
	    };

	    // Inc Size of second dimension (data)
	    ulSize += (*itilosCurrent).GetLength()+1;
	}

	switch(stCurrent)
	{
	case ST_COUNT:
	    {
		ulBufSize = ulSize+1;

		szNewValue = new char[ulBufSize];
		
		stCurrent = ST_FILL;
	    }
	    break;
	case ST_FILL:
	    {
		szNewValue[ulBufSize-1] = 0;
		stCurrent = ST_DONE;
	    }
	    break;
	default:
	    break;
	};
    }

    if
    (
	szNewValue
	&&
	FromStringArray((const char *)szNewValue, ulBufSize)
    )
    {
	delete [] szNewValue;

	return TRUE;
    }
    
    delete [] szNewValue;

    return FALSE;
}

HXBOOL 
CWinRegStringArrayValue::FromStringArray
(
    const char* szNewValue, 
    UINT32 ulSize
)
{
    if
    (
	RegSetValueEx
	(
	    m_hkParent,
	    OS_STRING(m_sName),
	    NULL,
	    REG_MULTI_SZ,
	    (unsigned char*)szNewValue,
	    ulSize
	) == ERROR_SUCCESS
    )
    {
	return TRUE;
    }

    return FALSE;
}

HXBOOL 
CWinRegStringArrayValue::DoesExist()
{
    return IsStringArray();
}


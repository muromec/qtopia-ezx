/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: wrvstrng.cpp,v 1.6 2007/07/06 20:39:21 jfinnecy Exp $
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
#include "hxstring.h"
#include "hxwinreg.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

CWinRegStringValue::CWinRegStringValue(const char* szName, HKEY hkParent)
    : AWinRegValue(szName, hkParent)
{
}

CWinRegStringValue::~CWinRegStringValue()
{
}

CWinRegStringValue& 
CWinRegStringValue::operator=(const char* szNewValue)
{
    FromString(szNewValue);

    return *this;
}

HXBOOL 
CWinRegStringValue::IsString()
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
	ulType == REG_SZ
    )
    {
	return TRUE;
    }

    return FALSE;
}

HXBOOL 
CWinRegStringValue::AsString(CHXString& rsValue)
{
    UINT32 ulType;
    UINT32 ulSize;
    
    rsValue.Empty();
    
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
	ulType == REG_SZ
    )
    {
	if (!ulSize)
	{
	    return TRUE;
	}

	++ulSize;

	if
	(
	    RegQueryValueEx
	    (
		m_hkParent,
		OS_STRING(m_sName),
		NULL,
		&ulType,
		(unsigned char*)rsValue.GetBufferSetLength(ulSize),
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
CWinRegStringValue::FromString(const char* szNew)
{
    if
    (
	RegSetValueEx
	(
	    m_hkParent,
	    OS_STRING(m_sName),
	    NULL,
	    REG_SZ,
	    (unsigned char*)szNew,
	    strlen(szNew)
	) == ERROR_SUCCESS
    )
    {
	return TRUE;
    }

    return FALSE;
}

HXBOOL 
CWinRegStringValue::DoesExist()
{
    return IsString();
}


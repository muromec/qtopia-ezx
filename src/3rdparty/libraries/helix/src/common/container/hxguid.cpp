/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxguid.cpp,v 1.15 2008/01/18 04:24:50 vkathuria Exp $
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

#include "hlxclib/string.h"
#include "hxguid.h"
#include "hxstring.h"
#include "hxstrutl.h"

#if defined(HELIX_FEATURE_FULLGUID)
#if defined _MACINTOSH || defined _UNIX || defined _SYMBIAN || defined _WINCE || defined _BREW
const GUID GUID_NULL = {0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0}};
#endif
#endif

#define CHARS_FOR_GUID_DATA4 8

CHXGUID::CHXGUID(void)
{
    ::SetGUID(m_guid, GUID_NULL);
}

CHXGUID::~CHXGUID(void)
{
}

CHXGUID::CHXGUID(REFGUID guid)
{
    ::SetGUID(m_guid, guid);
}

CHXGUID::CHXGUID(const char* pString)
{
    ::SetGUID(m_guid, GUID_NULL);
    Set(pString);
}

HXBOOL CHXGUID::Set(REFGUID guid)
{
    ::SetGUID(m_guid, guid);
    return(TRUE);
}

HXBOOL CHXGUID::Set(const char* pString)
{
#if defined(HELIX_FEATURE_FULLGUID)
    CHXString t1, t2;
    t1 = pString;

    t2 = t1.NthField('-', 1);
    m_guid.Data1 = ::strtoul(t2, NULL, 16);
    t2 = t1.NthField('-', 2);
    m_guid.Data2 = (UINT16)(::strtoul(t2, NULL, 16));
    t2 = t1.NthField('-', 3);
    m_guid.Data3 = (UINT16)(::strtoul(t2, NULL, 16));
    t2 = t1.NthField('-', 4);
    char d4str[3]; /* Flawfinder: ignore */
    d4str[2] = 0;
    // Loop through the characters in the 4th data field printing them into the buffer
    for (int i=0; i < CHARS_FOR_GUID_DATA4; i++)
    {
	d4str[0] = t2[i * 2];
	d4str[1] = t2[(i * 2) + 1];
	m_guid.Data4[i] = (UCHAR)::strtoul(d4str, NULL, 16);
    }

    return(TRUE);
#else
    // In this case, GUID is an enum
    HXBOOL bRet = FALSE;

    if (pString && strlen(pString) > 0)
    {
        UINT32 ulVal = ::strtoul(pString, NULL, 16);
        if (ulVal < NUM_GUIDS)
        {
	    m_guid = (GUID) ulVal;
        }
    }

    return bRet;
#endif
}

HXBOOL CHXGUID::Get(CHXString& string)
{
    INT32 length = sizeof(GUID)*2+4;

    // Allocate a buffer big enough to store the string representation of a GUID.  Size of GUID in bytes * 2 hex digits
    // for one byte + 4 Dashes
    char* szGUID = string.GetBuffer(length);  

    CHXGUID::Get(szGUID, length);

    // Null terminate and release buffer
    string.ReleaseBuffer();

    return(TRUE);
}

HXBOOL CHXGUID::Get(char* pBuffer, INT32 bufLen)
{
#if !defined(HELIX_FEATURE_FULLGUID)
    HXBOOL bRet = FALSE;

    if (pBuffer && strlen(pBuffer) > 8)
    {
        // GUID is an enum
        SafeSprintf(pBuffer, bufLen,"%.8lX",m_guid);
    }

    return bRet;
#else /* #if defined(_STATICALLY_LINKED) && !defined(HELIX_FEATURE_FULLGUID) */
    if (bufLen < sizeof(GUID) * 2 + 4)
	return(FALSE);

    // Write out the first 3 Data fields seperated by dashes and then move to the end of the buffer
    SafeSprintf(pBuffer, bufLen,"%.8lX-%.4hX-%.4hX-",m_guid.Data1,m_guid.Data2,m_guid.Data3);
    bufLen -= strlen(pBuffer); pBuffer += strlen(pBuffer);

    // Loop through the characters in the 4th data field printing them into the buffer
    for (int i=0; i < CHARS_FOR_GUID_DATA4; i++)
    {
	SafeSprintf(pBuffer, bufLen,"%.2lX",(UINT32)m_guid.Data4[i]);
	pBuffer += 2;
        bufLen -= 2;
    }

    return(TRUE);
#endif /* #if defined(_STATICALLY_LINKED) && !defined(HELIX_FEATURE_FULLGUID) #else */
}

HXBOOL CHXGUID::IsNull(void)
{
    return(::IsEqualGUID(m_guid, GUID_NULL));
}

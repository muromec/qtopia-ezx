/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: readpath.cpp,v 1.9 2006/02/07 19:21:28 ping Exp $
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

#include "dllpath.h"
#include "readpath.h"
#include "hxbuffer.h"
#include "pref.h"
#include "hlxosstr.h"

#include "hxheap.h"
#include "hxver.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif	

HX_RESULT ReadDLLAccessPathsFromPrefs(IUnknown* pContext)
{
#ifndef _VXWORKS
    if(!GetDLLAccessPath())
	return HXR_FAILED;

    CPref* pPreferences = CPref::open_shared_pref(HXVER_COMMUNITY, pContext);
    if(pPreferences)
    {
	for(UINT16 nIndex = 0; nIndex < DLLTYPE_NUMBER; nIndex++)
	{
	    IHXBuffer* pBuffer = NULL;
	    if(pPreferences->read_pref(GetDLLAccessPath()->GetLibTypeName(nIndex), pBuffer) == HXR_OK)
	    {
		GetDLLAccessPath()->SetPath(nIndex, (const char*)pBuffer->GetBuffer());
		HX_RELEASE(pBuffer);
	    }
	}

	delete pPreferences;
    }
#endif
    return HXR_OK;
}

HX_RESULT ReadUnsetPathsFromPrefs(IUnknown* pContext)
{
#ifndef _VXWORKS
    if(!GetDLLAccessPath())
	return HXR_FAILED;

    CPref* pPreferences = CPref::open_shared_pref(HXVER_COMMUNITY, pContext);
    if(pPreferences)
    {
	for(UINT16 nIndex = 0; nIndex < DLLTYPE_NUMBER; nIndex++)
	{
	    IHXBuffer* pBuffer = NULL;

	    if (GetDLLAccessPath()->GetPath(nIndex))
	    {
		continue;
	    }

	    if(pPreferences->read_pref(GetDLLAccessPath()->GetLibTypeName(nIndex), pBuffer) == HXR_OK)
	    {
		GetDLLAccessPath()->SetPath(nIndex, (const char*)pBuffer->GetBuffer());
		HX_RELEASE(pBuffer);
	    }
	}

	delete pPreferences;
    }
#endif
    return HXR_OK;
}

#ifdef _WIN32
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//CreatePath:
//Recursively creates path from given file path string (strPath).  
//Returns true if successful, false otherwise
HXBOOL
DirCreatePath( CHXString strPath )
{
	if (strPath.GetLength() == 0) return FALSE;

    // Trim off trailing slash - ::CreateDirectory() will crash if present
    if ( strPath.ReverseFind( '\\' ) == strPath.GetLength() - 1 )
        strPath = strPath.Left( strPath.GetLength() - 1 );

    // Try to create the directory as-is
    if ( 0 != ::CreateDirectory( OS_STRING((const char*)strPath), NULL ) )
        return TRUE;

    // ::CreateDirectory() will fail if the dir already exists
    DWORD dwErr = ::GetLastError();
    if ( ERROR_ALREADY_EXISTS == dwErr )
        return TRUE;

    // If that failed, trim off one sub dir and try to create that
    int nPos = strPath.ReverseFind( '\\' );
    if ( -1 == nPos )
        return FALSE;

    CHXString szSubPath = strPath.Left( nPos + 1 );
    if ( !DirCreatePath( szSubPath ) )
        return FALSE;

    // Creating the subdir succeeded, so now we should be able to create the original dir
    if ( 0 != ::CreateDirectory( OS_STRING((const char*)strPath), NULL ) )
        return TRUE;
    else
        return FALSE;
}
#endif


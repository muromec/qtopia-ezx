/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdll.cpp,v 1.8 2007/07/06 21:58:11 jfinnecy Exp $
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

// define all guids here once...
#include "hxcom.h"
#include "hxtypes.h"
#include "hxassert.h"
#include "hxresult.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxcore.h"
#include "hxslist.h"
#include "hxcleng.h"
#include "dllpath.h"
#include "hxdll.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

ENABLE_DLLACCESS_PATHS(HXCore);

#if !defined(HELIX_CONFIG_NOSTATICS)
HXClientEngine*    g_pEngine	  = NULL;
UINT16		    g_uNumEngines = 0;
#else
#include "globals/hxglobals.h"
const HXClientEngine* const g_pEngine	  = NULL;
const UINT16		    g_uNumEngines = 0;
#endif

/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		CreateEngine()
//
//	Purpose:
//
//
//	Parameters:
//
//	Return:
//
//		HX_RESULT
//		Error code indicating the success or failure of the function.
//
//	Notes:
//
//
STDAPI
HXEXPORT ENTRYPOINT(CreateEngine)(IHXClientEngine**	ppEngine)
{
#if defined(HELIX_CONFIG_NOSTATICS)
    UINT16& g_uNumEngines = (UINT16&)HXGlobalInt16::Get(&::g_uNumEngines);
    HXClientEngine*& g_pEngine =
	(HXClientEngine*&)HXGlobalPtr::Get(&::g_pEngine);
#endif

    g_uNumEngines++;
    
    if (g_pEngine)
    {
        *ppEngine	= g_pEngine;
        return HXR_OK;
    }

    *ppEngine	= new HXClientEngine;
    if (*ppEngine)
    {
        (*ppEngine)->AddRef();
        g_pEngine = (HXClientEngine*) (*ppEngine);

        return HXR_OK;
    }

    return HXR_OUTOFMEMORY;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Function:
//
//		CloseEngine()
//
//	Purpose:
//
//
//	Parameters:
//
//	Return:
//
//		HX_RESULT
//		Error code indicating the success or failure of the function.
//
//	Notes:
//
//
STDAPI
HXEXPORT ENTRYPOINT(CloseEngine)(IHXClientEngine*	pEngine)
{
#if defined(HELIX_CONFIG_NOSTATICS)
    UINT16& g_uNumEngines = (UINT16&)HXGlobalInt16::Get(&::g_uNumEngines);
    HXClientEngine*& g_pEngine = (HXClientEngine*&)HXGlobalPtr::Get(&::g_pEngine);
#endif

    if (g_pEngine && pEngine)
    {
        if (g_uNumEngines > 0)
        {
            g_uNumEngines--;
        }

        if (g_uNumEngines == 0)
        {
            g_pEngine->Close();
            g_pEngine->Release();
            g_pEngine = NULL;

            /* check for memory leaks for debug core with release top level client
            * You have to set HXCoreOnlyMemoryLeak key under HKEY_CLASSES_ROOT and
            * set its value to 1.
            */
#if defined(_DEBUG) && !defined(_WINCE) && !defined(_VXWORKS)
            if (::HXDebugOptionEnabled("HXCoreOnlyMemoryLeak")==TRUE)
            {
                    HX_DUMP_LEAKS();
            }
#endif
        }
    }

    return HXR_OK;
}


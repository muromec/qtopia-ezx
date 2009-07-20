/* ***** BEGIN LICENSE BLOCK *****
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
#include "hxcomptr.h"

#include "hxapplicationclassfactory.h"

namespace HXApplicationClassFactory
{
    static HXCOMPtr<IHXCommonClassFactory> g_spAppClassFactory;
    static bool g_bThisModuleInitedTheClassFactory = false;

#ifdef _WINDOWS
    const char kApplicationEnvName[] = {"{59D621E6-7894-4971-8CEF-B23BD9E33D42}"};
#endif

    HX_RESULT Init(IHXCommonClassFactory *pIHXCommonClassFactory)
    {
	PRE_REQUIRE_RETURN(pIHXCommonClassFactory, HXR_INVALID_PARAMETER);
        PRE_REQUIRE_RETURN(!g_spAppClassFactory.IsValid(), HXR_ALREADY_INITIALIZED);

	g_spAppClassFactory = pIHXCommonClassFactory;
	g_bThisModuleInitedTheClassFactory = true;

	HX_ASSERT(g_spAppClassFactory);

#ifdef _WINDOWS
	//under normal circumstances, this type of thing would be a big 'no, no'. The environment variable
	//system, however, is quite convenient in that it makes data available process wide. Once the DLL requests
	//the classfactory, it owns a ref count that won't be released until the DLL unloads- BE SURE TO UNLOAD
	//THE DLL BEFORE UNLOADING THE CLASSFACTORY! Failure to observe this rule will cause a crash on shutdown.
	//The pointer stored in the environment variable is not refcounted- instead the application's instance of
	//g_spAppClassFactory holds the refcount. This utility has been written so that the smart pointer can't be cleared
	//without also clearing the ENV variable. There is NO support for revoking the classfactory once it is set.

	//to encode the pointer as a string, the string needs two characters for every byte.
	const DWORD *kpPointerSize;
	const DWORD kdwStringSize = (sizeof(kpPointerSize) * 2) + 1;
	char szClassFactoryPointer[kdwStringSize];
	sprintf(szClassFactoryPointer, "%d", g_spAppClassFactory.Ptr());

	SetEnvironmentVariable(kApplicationEnvName, szClassFactoryPointer);
#endif

	return g_spAppClassFactory.IsValid() ? HXR_OK : HXR_FAIL;
    }

    HX_RESULT Terminate()
    {
	//only let the module that initialized the classfactory terminate it.
	if (g_bThisModuleInitedTheClassFactory)
	{
	    g_spAppClassFactory.Clear();
#ifdef _WINDOWS
	    SetEnvironmentVariable(kApplicationEnvName, "");
#endif
	    return HXR_OK;
	}

	return HXR_NOT_INITIALIZED;
    }


    HX_RESULT Get(IHXCommonClassFactory **ppIHXCommonClassFactory)
    {
	HX_RESULT outResult = HXR_FAIL;
#ifdef _WINDOWS
	if (!g_spAppClassFactory)
	{
	    const DWORD *kpPointerSize;
	    const DWORD kdwStringSize = (sizeof(kpPointerSize) * 2) + 1;
	    char szClassFactoryPointer[kdwStringSize];
	    if (GetEnvironmentVariable(kApplicationEnvName, szClassFactoryPointer, kdwStringSize))
	    {
		//under normal circumstances, this type of thing would be a big 'no, no'. The environment variable
		//system, however, is quite convenient in that it makes data available process wide. Once the DLL requests
		//the classfactory, it owns a ref count that won't be released until the DLL unloads- BE SURE TO UNLOAD
		//THE DLL BEFORE UNLOADING THE CLASSFACTORY! Failure to observe this rule will cause a crash on shutdown.
		g_spAppClassFactory = (IHXCommonClassFactory *) atoi(szClassFactoryPointer);
	    }
	}
#endif
	if (g_spAppClassFactory)
	{
	    g_spAppClassFactory.AsPtr(ppIHXCommonClassFactory);
	    outResult = HXR_OK;
	}

	return outResult;
    }
}; //namespace HXApplicationClassFactory

//Leave a CR/LF before EOF to prevent CVS from getting angry


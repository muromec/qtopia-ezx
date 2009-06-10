/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

/****************************************************************************
 * Includes
 */
#define INITGUID

#include "hxtypes.h"
#include "hxcom.h"
#include "hxplugncompat.h"

#include "hxcom.h"
#include "hxstrutl.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "ihxfgbuf.h"
#include "hxfiles.h"
#include "hxformt.h"
#include "hxplugn.h"
#include "hxpends.h"
#include "hxengin.h"
#include "hxerror.h"
#include "hxupgrd.h"
#include "hxmon.h"
#include "hxbrdcst.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "defslice.h"
#include "qtres.h"
#include "hxsdesc.h"
#include "hxstring.h"
#include "qtffplin.h"
#include "netbyte.h"
#include "hxver.h"
#include "hxxres.h"
#include "hxxrsmg.h"
#include "ihxlist.h"
#include "ihxtlogsystem.h"
#include "ihxtlogsystemcontext.h"
#include "hxdllaccess.h"
#include "hxiids.h"

#include "qtffplin.h"
#include "qtffrefcounter.h"
#include "hxtbuf.h"
#include "hxlistp.h"

#ifdef _AIX
#include "hxtbuf.h"
#include "dllpath.h"
ENABLE_MULTILOAD_DLLACCESS_PATHS(Qtffplin)
#endif


/****************************************************************************
 *  DLL Interface
 */
/****************************************************************************
 *  HXCreateInstance()
 *  Purpose:
 *	Function implemented by all plugin DLL's to create an instance of 
 *	any of the objects supported by the DLL. This method is similar to 
 *	Window's CoCreateInstance() in its purpose, except that it only 
 *	creates objects from this plugin DLL.
 *
 *	NOTE: Aggregation is never used. Therefore and outer unknown is
 *	not passed to this function, and you do not need to code for this
 *	situation.
 */
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(HXCREATEINSTANCE)(IUnknown** ppUnk)
{
    *ppUnk = (IUnknown*)(IHXPlugin*) new CQTFileFormat();
    if (*ppUnk)
    {
	(*ppUnk)->AddRef();
	return HXR_OK;
    }
    return HXR_OUTOFMEMORY;
}

/**************************************************************************** 
 *  CanUnload()
 *  Purpose:
 *	Function implemented by all plugin DLL's if it returns HXR_OK 
 *	then the pluginhandler can unload the DLL
 */
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(CanUnload)(void)
{
    return (g_nRefCount_qtff ? HXR_FAIL : HXR_OK);
}

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(CanUnload2)(void)
{
    return ENTRYPOINT(CanUnload)();
}

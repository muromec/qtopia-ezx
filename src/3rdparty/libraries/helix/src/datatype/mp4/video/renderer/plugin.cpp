/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0 and Exhibits. 
 * REALNETWORKS CONFIDENTIAL--NOT FOR DISTRIBUTION IN SOURCE CODE FORM 
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. 
 * All Rights Reserved. 
 * 
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Community Source 
 * License Version 1.0 (the "RCSL"), including Attachments A though H, 
 * all available at http://www.helixcommunity.org/content/rcsl. 
 * You may also obtain the license terms directly from RealNetworks. 
 * You may not use this file except in compliance with the RCSL and 
 * its Attachments. There are no redistribution rights for the source 
 * code of this file. Please see the applicable RCSL for the rights, 
 * obligations and limitations governing use of the contents of the file. 
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
 * https://rarvcode-tck.helixcommunity.org 
 * 
 * Contributor(s): 
 * 
 * ***** END LICENSE BLOCK ***** */ 

#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxresult.h"
#include "hxcom.h"
#include "hxevent.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxcore.h"
#include "hxrendr.h"
#include "hxhyper.h"
#include "hxplugn.h"
#include "hxasm.h"
#include "hxupgrd.h"
#include "hxengin.h"
#include "hxprefs.h"
#include "hxerror.h"
#include "hxwin.h"
#include "hxvsurf.h"
#include "hxvctrl.h"
#include "hxsite2.h"
#include "hxthread.h"
#include "hxmon.h"
#include "hxformt.h"

#include "hlxclib/stdio.h"
#include "mp4video.h"
#include "mp4vdfmt.h"

#include "dllpath.h"
#include "hxheap.h"


/****************************************************************************
 * 
 *  Function:
 * 
 *	HXCreateInstance()
 * 
 *  Purpose:
 * 
 *	Function implemented by all plugin DLL's to create an instance of 
 *	any of the objects supported by the DLL. This method is similar to 
 *	Window's CoCreateInstance() in its purpose, except that it only 
 *	creates objects from this plugin DLL.
 *
 *	NOTE: Aggregation is never used. Therefore and outer unknown is
 *	not passed to this function, and you do not need to code for this
 *	situation.
 * 
 */
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(HXCreateInstance)
(   
    IUnknown**  /*OUT*/	ppIUnknown  
)   
{   
    return CMP4VideoRenderer::HXCreateInstance(ppIUnknown);
}   

/****************************************************************************
 * 
 *  Function:
 * 
 *	CanUnload()
 * 
 *  Purpose:
 * 
 *	Function implemented by all plugin DLL's if it returns HXR_OK 
 *	then the pluginhandler can unload the DLL
 *
 */

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(CanUnload)(void)
{
    return CMP4VideoRenderer::CanUnload();
}

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(CanUnload2)(void)
{
    return CMP4VideoRenderer::CanUnload2();
}

/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: swfres.h,v 1.4 2007/07/06 21:58:27 jfinnecy Exp $
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

#ifndef SWFRES_H
#define SWFRES_H

#include "resid.h"

// Constant definitions
#define SWF_RESIDX_FLASHCMDS  0
#define SWF_RESIDX_ZOOMIN     1
#define SWF_RESIDX_ZOOMOUT    2
#define SWF_RESIDX_100PCT     3
#define SWF_RESIDX_SHOWALL    4
#define SWF_RESIDX_HIQUALITY  5
#define SWF_RESIDX_ABOUTFLASH 6
#define SWF_RESIDX_SWF_GENERALERROR    7
#define SWF_RESIDX_SWF_NOTLICENSED     8
#define SWF_RESIDX_SWF_NOTSUPPORTED    10
#define SWF_RESIDX_UNTUNEDCONTENT      9

#define NUM_SWF_RES           11 // THIS MUST BE INCREMENTED

#define IDS_CONTEXT_MENU_FLASHCMDS      HX_SWF_RES_INIT_ID + SWF_RESIDX_FLASHCMDS
#define IDS_CONTEXT_MENU_ZOOMIN         HX_SWF_RES_INIT_ID + SWF_RESIDX_ZOOMIN
#define IDS_CONTEXT_MENU_ZOOMOUT        HX_SWF_RES_INIT_ID + SWF_RESIDX_ZOOMOUT
#define IDS_CONTEXT_MENU_100PCT         HX_SWF_RES_INIT_ID + SWF_RESIDX_100PCT
#define IDS_CONTEXT_MENU_SHOWALL        HX_SWF_RES_INIT_ID + SWF_RESIDX_SHOWALL
#define IDS_CONTEXT_MENU_HIQUALITY      HX_SWF_RES_INIT_ID + SWF_RESIDX_HIQUALITY
#define IDS_CONTEXT_MENU_ABOUTFLASH     HX_SWF_RES_INIT_ID + SWF_RESIDX_ABOUTFLASH
#define IDS_ERR_SWF_GENERALERROR        HX_SWF_RES_INIT_ID + SWF_RESIDX_SWF_GENERALERROR
#define IDS_ERR_SWF_NOTLICENSED         HX_SWF_RES_INIT_ID + SWF_RESIDX_SWF_NOTLICENSED
#define IDS_ERR_SWF_NOTSUPPORTED        HX_SWF_RES_INIT_ID + SWF_RESIDX_SWF_NOTSUPPORTED
#define IDS_ERR_SWF_UNTUNEDCONTENT      HX_SWF_RES_INIT_ID + SWF_RESIDX_UNTUNEDCONTENT

#define CONTEXT_MENU_STR_FLASHCMDS      "Flash Commands"
#define CONTEXT_MENU_STR_ZOOMIN         "Zoom In"
#define CONTEXT_MENU_STR_ZOOMOUT        "Zoom Out"
#define CONTEXT_MENU_STR_100PCT         "100%"
#define CONTEXT_MENU_STR_SHOWALL        "Show All"
#define CONTEXT_MENU_STR_HIQUALITY      "High Quality"
#define CONTEXT_MENU_STR_ABOUTFLASH     "About Macromedia Flash..."
#define ERRSTR_SWF_GENERALERROR         "RealFlash: General Error."
#define ERRSTR_SWF_NOTLICENSED          "RealFlash: This server is NOT licensed to deliver RealFlash streams. A Player attempting to access RealFlash content has been disconnected. Please contact RealNetworks to obtain a license for this feature."
#define ERRSTR_SWF_UNTUNEDCONTENT       "RealFlash: This server is configured to disallow untuned flash content. A Player attempted to play URL which is untuned."
#define ERRSTR_SWF_NOTSUPPORTED         "RealFlash: Flash versions greater than 4 are not supported."

#endif

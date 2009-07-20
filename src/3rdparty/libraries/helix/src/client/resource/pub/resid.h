/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: resid.h,v 1.5 2007/07/06 21:58:27 jfinnecy Exp $
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

#ifndef _RESID_H_
#define _RESID_H_

#define CORE_RESOURCE_SHORT_NAME        "CORE"

/*
 * resources all shared in the CORE XRS external resource file
 */
#define HX_CORE_RES_INIT_ID             1000
#define HX_SMIL_RES_INIT_ID             2000
#define HX_RV_RES_INIT_ID               3000
#define HX_RP_RES_INIT_ID               4000
#define HX_RT_RES_INIT_ID               5000
#define HX_SWF_RES_INIT_ID              6000
#define HX_MISC_RES_INIT_ID             7000  // qtres.h allocated 7000-7050
#define HX_SM_RES_INIT_ID               7500
#define HX_XML_RES_INIT_ID              8000
#define HX_SA_RES_INIT_ID               9000  // ServerAlerts
#define HX_SO_RES_INIT_ID               9500

#ifdef __MWERKS__
#define VS_FFI_FILEFLAGSMASK 0
#define VOS__WINDOWS32 0
#endif
#endif /* _RESID_H_ */

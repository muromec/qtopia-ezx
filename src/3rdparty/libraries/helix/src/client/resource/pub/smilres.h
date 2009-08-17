/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: smilres.h,v 1.2 2004/07/19 21:13:09 hubbe Exp $
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

#ifndef _SMILRES_H_
#define _SMILRES_H_

#include "resid.h"

#define IDS_ERR_SMIL_GENERALERROR		HX_SMIL_RES_INIT_ID + 0
#define IDS_ERR_SMIL_BADXML			HX_SMIL_RES_INIT_ID + 1
#define IDS_ERR_SMIL_NOTSMIL			HX_SMIL_RES_INIT_ID + 2
#define IDS_ERR_SMIL_DUPLICATEID		HX_SMIL_RES_INIT_ID + 3
#define IDS_ERR_SMIL_NONEXISTENTID		HX_SMIL_RES_INIT_ID + 4
#define IDS_ERR_SMIL_NOBODYTAG			HX_SMIL_RES_INIT_ID + 5
#define IDS_ERR_SMIL_NOBODYELEMENTS		HX_SMIL_RES_INIT_ID + 6
#define IDS_ERR_SMIL_UNRECOGNIZEDTAG		HX_SMIL_RES_INIT_ID + 7
#define IDS_ERR_SMIL_UNRECOGNIZEDATTRIBUTE	HX_SMIL_RES_INIT_ID + 8
#define IDS_ERR_SMIL_UNEXPECTEDTAG		HX_SMIL_RES_INIT_ID + 9
#define IDS_ERR_SMIL_BADDURATION		HX_SMIL_RES_INIT_ID + 10
#define IDS_ERR_SMIL_BADATTRIBUTE		HX_SMIL_RES_INIT_ID + 11
#define IDS_ERR_SMIL_BADFRAGMENT		HX_SMIL_RES_INIT_ID + 12
#define IDS_ERR_SMIL_REQUIREDATTRIBUTEMISSING	HX_SMIL_RES_INIT_ID + 13
#define IDS_ERR_SMIL_SYNCATTRIBUTEOUTOFSCOPE	HX_SMIL_RES_INIT_ID + 14
#define IDS_ERR_SMIL_UNEXPECTEDCONTENT		HX_SMIL_RES_INIT_ID + 15
#define IDS_ERR_SMIL_SMIL10DOCUMENT		HX_SMIL_RES_INIT_ID + 16
#define IDS_ERR_SMIL_INDEFINITENOTSUPPORTED	HX_SMIL_RES_INIT_ID + 17
#define IDS_ERR_SMIL_METADATATYPE		HX_SMIL_RES_INIT_ID + 18
#define IDS_ERR_SMIL_ROOTHEIGHTWIDTHREQUIRED	HX_SMIL_RES_INIT_ID + 19
#define IDS_ERR_SMIL_BADID			HX_SMIL_RES_INIT_ID + 20
#define IDS_ERR_SMIL_NOSOURCES			HX_SMIL_RES_INIT_ID + 21

#endif /* _SMILRES_H_ */

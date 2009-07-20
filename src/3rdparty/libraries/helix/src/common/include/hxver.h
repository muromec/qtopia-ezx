/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxver.h,v 1.24 2009/03/10 19:24:54 jeffl Exp $
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

#ifndef HXVER_H
#define HXVER_H

#if defined(HELIX_FEATURE_USE_RNVER) || defined(HELIX_CONFIG_USE_RNVER)

#include "rnver.h"

#else

////////////////////////////////////////////////////////////////////////////////
// NOTE:
//		To change build or version number change the follow defines only!
//		All version resource numbers are derived from the folloing defines.
//		We would like to automatically build the STRING_VERSION macro from
//		the numeric version numbers, but I have not figured out how to perform
//		such an operation. So for now, when the build number changes, we will
//		need to change both the BUILD_NUMBER value and the STR_BUILD_NUMBER
//		value as well.
//
#define		MAJOR_VERSION					2
#define         MINOR_VERSION                                   1
#define		RELEASE_NUMBER					0
#define         BUILD_NUMBER                                    0

#define		STR_MAJOR_VERSION				"6"
#define         STR_MINOR_VERSION                               "0"
#define		STR_RELEASE_NUMBER				"0"
#define         STR_BUILD_NUMBER                                "1"
//
////////////////////////////////////////////////////////////////////////////////

#define		ULONG32_VERSION(ma,mi,s,b)		((ma << 28) | (mi << 20) | (s << 12) | b)
#define		LIST_VERSION(ma,mi,s,b)			ma,mi,s,b
#define		STRING_VERSION(ma,mi,s,b)		ma "." mi "." s "." b "\0"

//      Standard Product versions
#define		STANDARD_ULONG32_VERSION		ULONG32_VERSION(MAJOR_VERSION,MINOR_VERSION,RELEASE_NUMBER,BUILD_NUMBER)
#define		STANDARD_LIST_VERSION			LIST_VERSION(MAJOR_VERSION,MINOR_VERSION,RELEASE_NUMBER,BUILD_NUMBER)
#define		STANDARD_STRING_VERSION			STRING_VERSION(STR_MAJOR_VERSION,STR_MINOR_VERSION,STR_RELEASE_NUMBER,STR_BUILD_NUMBER)

//      File OS for built modules depends on the compiler...
#ifdef WIN32
#define VER_FILEOS	VOS__WINDOWS32
#else
#define VER_FILEOS	VOS__WINDOWS16
#endif

/********************************************************************
|       Version related strings.  
|
|       NOTE TO TRANSLATORS:
|       You will need to ensure that an appropriate language is defined and
|       tested for here.  The example case is LANG_FOO.
\********************************************************************/

//      Common Version Info (Company, Copyright, Product, & Trademarks)
// #if defined( RC_INVOKED ) 
//      TRANSLATOR: These strings are for the ENGLISH language version of the product
//      DO NOT CHANGE THEM!!!!
#define         LANGUAGE_CODE					"EN"
#define         RCLI_PRERELEASE                 "Version 2.1" 

#define         HXVER_COMMUNITY                 "Helix"
#define         HXVER_COMPANY                   " " // the resource parsing code in client\xres does not handle empty values. Use space for empty.
#define         HXVER_COPYRIGHT                 "Copyright(c) RealNetworks, Inc. 1995-2008. All rights reserved. Source code for this program is available under the RealNetworks Public Source License.\0" 
#define         HXVER_RN_COPYRIGHT              "Copyright(c) RealNetworks, Inc. 1995-2008, All rights reserved.\0"
#define         HXVER_TRADEMARKS                " " // see above
#define         HXVER_MOREINFO                  "http://www.helixcommunity.org\0" 
#define         HXVER_PRODUCTNAME               "Helix DNA Client\0"

#define         STRBUILD						"" 

#define         STREND							"\0" 

#define		RNVER_PLAYER_DISPLAY_NAME	"Helix DNA Client"
#define		RNVER_PLAYER_DISPLAY_NAME_TM    "Helix(tm) DNA Client"

//XXXgfw This hacked up user agent prefix string is just a work around until
//NetCache fixes what user agent string they look for. If they don't
//find 'RealMedia Player' at the begining then certain stream don't
//work right or just don't play.
#define         USER_AGENT_PREFIX               "RealMedia Player "    // don't change this
#define         USER_AGENT_STRING               ""                     // customize this
#define         USER_AGENT_POSTFIX              "HelixDNAClient"       // don't change this

#if defined(_WIN32) || defined(_UNIX)
#define         STRPLATFORM						" (32-bit) "
#elif defined(_WIN16)
#define         STRPLATFORM						" (16-bit) "
#elif defined(_MACINTOSH)
#define         STRPLATFORM						" MacPPC "
#endif

#define         HXVER_SDK_PRODUCT	    "HelixSDK"

#define         TOOLS_TRADEMARKS "\0"

#endif /* HELIX_FEATURE_USE_RNVER */

#endif  //      !defined( HXVER_H )

/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: saerrs.h,v 1.5 2007/07/06 21:58:27 jfinnecy Exp $
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

#ifndef _SAERRS_H_
#define _SAERRS_H_

#include "resid.h"

#define IDS_SE_NO_ERROR                     HX_SA_RES_INIT_ID + 0  // No Error!
#define IDS_SE_INVALID_VERSION              HX_SA_RES_INIT_ID + 1  // PNA Specific Error
#define IDS_SE_INVALID_FORMAT               HX_SA_RES_INIT_ID + 2   // Reported as `Request Failed'
#define IDS_SE_INVALID_BANDWIDTH            HX_SA_RES_INIT_ID + 3    // PNA Specific Error
#define IDS_SE_INVALID_PATH                 HX_SA_RES_INIT_ID + 4    // Reported as `Request Failed'
#define IDS_SE_UNKNOWN_PATH                 HX_SA_RES_INIT_ID + 5   // Reported as `Request Failed'
#define IDS_SE_INVALID_PROTOCOL             HX_SA_RES_INIT_ID + 6    // Reported as `Request Failed'
#define IDS_SE_INVALID_PLAYER_ADDR          HX_SA_RES_INIT_ID + 7    // Reported as `Request Failed'
#define IDS_SE_LOCAL_STREAMS_PROHIBITED     HX_SA_RES_INIT_ID + 8    // XXXSMP Not Implemented Yet!
#define IDS_SE_SERVER_FULL                  HX_SA_RES_INIT_ID + 9    // Maximum Capacity Reached
#define IDS_SE_REMOTE_STREAMS_PROHIBITED    HX_SA_RES_INIT_ID + 10    // XXXSMP Not Implemented Yet!
#define IDS_SE_EVENT_STREAMS_PROHIBITED     HX_SA_RES_INIT_ID + 12 // XXXSMP Not Implemented Yet!
#define IDS_SE_INVALID_HOST                 HX_SA_RES_INIT_ID + 13    // XXXSMP Not Implemented Yet!
#define IDS_SE_NO_CODEC                     HX_SA_RES_INIT_ID + 14                   // PNA Specific Error
#define IDS_SE_LIVEFILE_INVALID_BWN         HX_SA_RES_INIT_ID + 15     // XXXSMP Should be Deprecated
#define IDS_SE_UNABLE_TO_FULFILL            HX_SA_RES_INIT_ID + 16          // XXXSMP Should be Deprecated
#define IDS_SE_MULTICAST_DELIVERY_ONLY      HX_SA_RES_INIT_ID + 17      // XXXSMP Not Implemented Yet!
#define IDS_SE_LICENSE_EXCEEDED             HX_SA_RES_INIT_ID + 18           // XXXSMP Not Implemented Yet!
#define IDS_SE_LICENSE_UNAVAILABLE          HX_SA_RES_INIT_ID + 19       // XXXSMP Not Implemented Yet!
#define IDS_SE_INVALID_LOSS_CORRECTION      HX_SA_RES_INIT_ID + 20   // PNA Specific Error
#define IDS_SE_PROTOCOL_FAILURE             HX_SA_RES_INIT_ID + 21             // Reported as update player
#define IDS_SE_REALVIDEO_STREAMS_PROHIBITED HX_SA_RES_INIT_ID + 22
#define IDS_SE_REALAUDIO_STREAMS_PROHIBITED HX_SA_RES_INIT_ID + 23
#define IDS_SE_DATATYPE_UNSUPPORTED         HX_SA_RES_INIT_ID + 24
#define IDS_SE_DATATYPE_UNLICENSED          HX_SA_RES_INIT_ID + 25
#define IDS_SE_RESTRICTED_PLAYER            HX_SA_RES_INIT_ID + 26
#define IDS_SE_STREAM_INITIALIZING          HX_SA_RES_INIT_ID + 27
#define IDS_SE_INVALID_PLAYER               HX_SA_RES_INIT_ID + 28               // Not RealNetworks Player
#define IDS_SE_PLAYER_PLUS_ONLY             HX_SA_RES_INIT_ID + 29            // Player Plus only content
#define IDS_SE_NO_EMBEDDED_PLAYERS          HX_SA_RES_INIT_ID + 30          // Embedded players prohibited
#define IDS_SE_PNA_PROHIBITED               HX_SA_RES_INIT_ID + 31               // PNA unsupported
#define IDS_SE_AUTHENTICATION_UNSUPPORTED   HX_SA_RES_INIT_ID + 32
#define IDS_SE_MAX_FAILED_AUTHENTICATIONS   HX_SA_RES_INIT_ID + 33
#define IDS_SE_AUTH_ACCESS_DENIED           HX_SA_RES_INIT_ID + 34
#define IDS_SE_AUTH_UUID_READ_ONLY          HX_SA_RES_INIT_ID + 35
#define IDS_SE_AUTH_UUID_NOT_UNIQUE         HX_SA_RES_INIT_ID + 36
#define IDS_SE_AUTH_NO_SUCH_USER            HX_SA_RES_INIT_ID + 37
#define IDS_SE_AUTH_REGISTRATION_SUCCEEDED  HX_SA_RES_INIT_ID + 38
#define IDS_SE_AUTH_REGISTRATION_FAILED     HX_SA_RES_INIT_ID + 39
#define IDS_SE_AUTH_REGISTRATION_GUID_REQUIRED HX_SA_RES_INIT_ID + 40
#define IDS_SE_AUTH_UNREGISTERED_PLAYER     HX_SA_RES_INIT_ID + 41
#define IDS_SE_AUTH_TIME_EXPIRED            HX_SA_RES_INIT_ID + 42
#define IDS_SE_AUTH_NO_TIME_LEFT            HX_SA_RES_INIT_ID + 43
#define IDS_SE_AUTH_ACCOUNT_LOCKED          HX_SA_RES_INIT_ID + 44
#define IDS_SE_AUTH_INVALID_SERVER_CFG      HX_SA_RES_INIT_ID + 45
#define IDS_SE_NO_MOBILE_DOWNLOAD           HX_SA_RES_INIT_ID + 46
#define IDS_SE_NO_MORE_MULTI_ADDR           HX_SA_RES_INIT_ID + 47
#define IDS_PE_PROXY_MAX_CONNECTIONS        HX_SA_RES_INIT_ID + 48
#define IDS_PE_PROXY_MAX_GW_BANDWIDTH       HX_SA_RES_INIT_ID + 49
#define IDS_PE_PROXY_MAX_BANDWIDTH          HX_SA_RES_INIT_ID + 50
#define IDS_SE_BAD_LOADTEST_PASSWORD        HX_SA_RES_INIT_ID + 51
#define IDS_SE_PNA_NOT_SUPPORTED            HX_SA_RES_INIT_ID + 52
#define IDS_PE_PROXY_ORIGIN_DISCONNECTED    HX_SA_RES_INIT_ID + 53
#define IDS_SE_FORBIDDEN                    HX_SA_RES_INIT_ID + 54

#define IDS_SE_INTERNAL_ERROR               HX_SA_RES_INIT_ID + 200

#endif

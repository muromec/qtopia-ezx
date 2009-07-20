/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: srcerrs.h,v 1.8 2007/07/06 20:43:42 jfinnecy Exp $
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

#ifndef _SRCERRS_H_
#define _SRCERRS_H_

/*
        NOTE:   When modifying the list of ServerAlerts (adding/subtracting messages)
                you also have to update the following files:

                client/resources/pub/saerrs.h
                        -Add/Remove unique IDS_
                client/core/hxresmgr.cpp
                        -Add/Remove HXR_ to IDX_ mapping in ErrorStringTable[]
                common/include/hxresult.h
                        -Add/Remove unique HXR_SE
*/

enum StreamError {
    SE_NO_ERROR = 0,                            // No Error!
    SE_INVALID_VERSION,                         // PNA Specific Error
    SE_INVALID_FORMAT,                          // Reported as `Request Failed'
    SE_INVALID_BANDWIDTH,                       // PNA Specific Error
    SE_INVALID_PATH,                            // Reported as `Request Failed'
    SE_UNKNOWN_PATH,                            // Reported as `Request Failed'
    SE_INVALID_PROTOCOL,                        // Reported as `Request Failed'
    SE_INVALID_PLAYER_ADDR,                     // Reported as `Request Failed'
    SE_LOCAL_STREAMS_PROHIBITED,                // XXXSMP Not Implemented Yet!
    SE_SERVER_FULL,                             // Maximum Capacity Reached
    SE_REMOTE_STREAMS_PROHIBITED,               // XXXSMP Not Implemented Yet!
    SE_EVENT_STREAMS_PROHIBITED,                // XXXSMP Not Implemented Yet!
    SE_INVALID_HOST,                            // XXXSMP Not Implemented Yet!
    SE_NO_CODEC,                                // PNA Specific Error
    SE_LIVEFILE_INVALID_BWN,                    // XXXSMP Should be Deprecated
    SE_UNABLE_TO_FULFILL,                       // XXXSMP Should be Deprecated
    SE_MULTICAST_DELIVERY_ONLY,                 // XXXSMP Not Implemented Yet!
    SE_LICENSE_EXCEEDED,                        // XXXSMP Not Implemented Yet!
    SE_LICENSE_UNAVAILABLE,                     // XXXSMP Not Implemented Yet!
    SE_INVALID_LOSS_CORRECTION,                 // PNA Specific Error
    SE_PROTOCOL_FAILURE,                        // Reported as update player
    SE_REALVIDEO_STREAMS_PROHIBITED,
    SE_REALAUDIO_STREAMS_PROHIBITED,
    SE_DATATYPE_UNSUPPORTED,
    SE_DATATYPE_UNLICENSED,
    SE_RESTRICTED_PLAYER,
    SE_STREAM_INITIALIZING,
    SE_INVALID_PLAYER,                          // Not RealNetworks Player
    SE_PLAYER_PLUS_ONLY,                        // Player Plus only content
    SE_NO_EMBEDDED_PLAYERS,                     // Embedded players prohibited
    SE_PNA_PROHIBITED,                          // PNA unsupported

    // authentication

    SE_AUTHENTICATION_UNSUPPORTED,
    SE_MAX_FAILED_AUTHENTICATIONS,
    SE_AUTH_ACCESS_DENIED,
    SE_AUTH_UUID_READ_ONLY,
    SE_AUTH_UUID_NOT_UNIQUE,
    SE_AUTH_NO_SUCH_USER,
    SE_AUTH_REGISTRATION_SUCCEEDED,
    SE_AUTH_REGISTRATION_FAILED,
    SE_AUTH_REGISTRATION_GUID_REQUIRED,
    SE_AUTH_UNREGISTERED_PLAYER,
    SE_AUTH_TIME_EXPIRED,
    SE_AUTH_NO_TIME_LEFT,
    SE_AUTH_ACCOUNT_LOCKED,
    SE_AUTH_INVALID_SERVER_CFG,


    SE_NO_MOBILE_DOWNLOAD,

    // XXXGo
    SE_NO_MORE_MULTI_ADDR,

    // "PE_XXXX" are Proxy alerts
    PE_PROXY_MAX_CONNECTIONS,
    PE_PROXY_MAX_GW_BANDWIDTH,
    PE_PROXY_MAX_BANDWIDTH,

    SE_BAD_LOADTEST_PASSWORD,
    SE_PNA_NOT_SUPPORTED,

    PE_PROXY_ORIGIN_DISCONNECTED,

    SE_SERVER_SHUTTING_DOWN,

    /* Make sure you add new error codes *ABOVE* this line */
    SE_INTERNAL_ERROR
};

static const char* const error_description_table[] =
{
    "",                                         // SE_NO_ERROR
    "Invalid version",                          // SE_INVALID_VERSION
    "",                                         // SE_INVALID_FORMAT
    "Insuffient bandwidth",                     // SE_INVALID_BANDWIDTH
    "Invalid path",                             // SE_INVALID_PATH
    "Invalid path",                             // SE_UNKNOWN_PATH
    "Invalid protocol",                         // SE_INVALID_PROTOCOL
    "Invalid player IP Address",                // SE_INVALID_PLAYER_ADDR
    "Not configured for local streams",         // SE_LOCAL_STREAMS_PROHIBITED
    "Server full",                              // SE_SERVER_FULL
    "Not configured for remote streams",        // SE_REMOTE_STREAMS_PROHIBITED
    "Not configured for events",                // SE_EVENT_STREAMS_PROHIBITED
    "Invalid host",                             // SE_INVALID_HOST
    "Codec error",                              // SE_NO_CODEC
    "Livefile codec/bandwidth error",           // SE_LIVEFILE_INVALID_BWN
    "Licensing error",                          // SE_UNABLE_TO_FULFILL
    "Multicast delivery only",                  // SE_MULTICAST_DELIVERY_ONLY
    "License exceeded",                         // SE_LICENSE_EXCEEDED
    "Stream License not available.",            // SE_LICENSE_EXCEEDED
    "2.0 player with loss correction",          // SE_INVALID_LOSS_CORRECTION
    "Protocol failure",                         // SE_PROTOCOL_FAILURE
    "Not configured for RealVideo streams",
    "Not configured for RealAudio streams",
    "Not configured for requested data type",   // SE_DATATYPE_UNSUPPORTED
    "Not licensed for requested data type",     // SE_DATATYPE_UNLICENSED
    "Restricted Player",
    "Stream Initializing",
    "Invalid Player",
    "Player Plus only",
    "Embedded Players prohibited",
    "PNA unsupported for requested data type",  // SE_PNA_PROHIBITED

    // authentication

    "This player doesn't support user authentication",  // SE_AUTHENTICATION_UNSUPPORTED
    "Maximum number of failed authentications reached", // SE_MAX_FAILED_AUTHENTICATIONS
    "Access to secure content denied.",
    "Registration failed, your account cannot be modified at this time.",
    "Registration failed, there already exists a user with the same GUID.",
    "Registration failed, the specified user does not exist.",
    "GUID successfully registered.",
    "Registration failed, contact your content provider for assistance.",
    "You must enable GUIDs in your player preferences. For more info on GUIDs and privacy, please search RealNetworks' website.",
    "You must register your RealOne Player before viewing this content.",
    "Your account has expired, contact your content provider for more information.",
    "Your account has expired, contact your content provider for more information.",
    "Your account has been locked, contact your content provider for more information.",
    "Access to secure content denied. The commerce server has been configured incorrectly.",

    "This server does not support mobile download",

    // SE_NO_MORE_MULTI_ADDR
    "Back channel multicast: Please increase the address range configuration variable.",

    // "PE_XXXX" are Proxy alerts
    "Maximum number of proxy connections reached.",
    "Maximum proxy gateway bandwidth reached.",
    "Maximum proxy bandwidth reached.",

    "Bad Load Test Password.",
    "PNA protocol not supported by this server.",

    "Proxy to origin server connection lost",

    "The administrator has shut this server down.",

    "Internal Error"                            // SE_INTERNAL_ERROR
};

static const char* const alert_table[] =
{
    0,
    "Please download a new RealOne Player from http://www.real.com "
        "to receive this content.",
    0,
    "You cannot receive this content. Either your network bandwidth is "
        "not fast enough to receive this data or your CPU is not powerful "
        "enough to decode it." ,
    0,
    0,
    0,
    0,
    "This server is not configured to play local streams.",
    "This server has reached its capacity for the requested url.",
    "This server is not configured to play remote streams.",
    "This server is not configured to play event streams.",
    "Invalid server host specified in the URL.",
    "You need to obtain a new player to play this clip. Please point your web "
        "browser to http://www.real.com/ and download the latest player "
        "from RealNetworks. Once you have installed it you should try "
        "this clip again.",
    "That stream is not available with the requested codec and/or bandwidth",
    "Unable to fulfill request",
    "This server is configured to support only multicast connections. "
        "Please contact the content provider for more information on "
        "listening to this broadcast.",
    "Server has reached its capacity and can serve no more streams. "
        "Please try again later.",
    "Server does not have the license for the requested streams. "
        "Please try again later.",
    "To receive this stream: choose Preferences from the View menu, "
        "click on the Network tab, click on the check next to "
        "Loss Correction to turn it off, and then requeset this URL again.",
    "You have connected to a RealServer that only supports "
        "players newer than the one you are using. Please check on the "
        "Web Site you accessed this clip from for details on what players "
        "are supported. To download the latest RealOne Player, point your Web "
        "Browser to http://www.real.com",
    "This server is not configured to play RealVideo streams.",
    "This server is not configured to play RealAudio streams.",
    "This server is not configured to play the data type you requested. "
        "Please contact the Real Server Administrator for assistance.",
    "This server is not licensed to play one or more of the data types you "
        "requested. Please contact the Real Server Administrator for "
        "assistance.",
    "You do not have access to the requested URL.",
    "The URL you have requested is initializing. Please try again in a few "
        "moments.",
    "You need to obtain a new player to play this clip. Please point your web "
        "browser to http://www.real.com and download the latest RealOne Player "
        "from RealNetworks. Once you have installed it you should try this "
        "clip again.",
    "The content you requested is available exclusively to RealOne service "
        "members. Please point your web browser to http://www.real.com/ for "
        "upgrade information.",
    "This server is not licensed to play content to embedded players. Please "
        "contact the RealServer Administrator for assistance.",
    "The file you requested cannot be streamed using the PNM protocol. "
        "Please make sure you have downloaded the latest RealOne Player from "
        "http://www.real.com and try this clip again using rtsp:// instead "
        "of pnm:// in the URL.",

    // authentication

    "You need to obtain a new RealOne Player to play this clip. Please point your web "
        "browser to http://www.real.com/ and download the latest RealOne Player "
        "from RealNetworks. Once you have installed it you should try "
        "this clip again.",
    "RealOne Player is unable to access this clip with the specified username and "
        "password. Please contact the Site Administrator for assistance.",
    "Access to secure content denied.",
    "Registration failed, your account cannot be modified at this time.",
    "Registration failed, there already exists a user with the same GUID.",
    "Registration failed, the specified user does not exist.",
    "You have successfully registered your RealOne Player.",
    "Registration failed, contact your content provider for assistance.",
    "You must enable GUIDs in your player preferences. For more info on GUIDs and privacy, please search RealNetworks' website.",
    "You must register your RealOne Player before viewing this content. Please "
        "contact your content provider for assistance.",
    "Your account has expired, contact your content provider for more information.",
    "Your account has expired, contact your content provider for more information.",
    "Your account has been locked, contact your content provider for more information.",
    "This commerce server has been configured incorrectly, and is unable to serve "
    "secure content. Please contact the Site Administrator for assistance.",

    "This server does not support mobile download.",

    "File not found",   // SE_NO_MORE_MULTI_ADDR

    // "PE_XXXX" are Proxy alerts
    "A proxy has exceeded its maximum number of connections. Please try again later.",
    "A proxy has exceeded its maximum gateway bandwidth. Please try again later.",
    "A proxy has exceeded its maximum bandwidth. Please try again later.",

    "You have entered an incorrect load test password or load testing is not enabled.",
    "PNA protocol not supported by this server.",

    "Proxy to origin server connection lost",

    "The administrator has shut this server down. Please try again later.",

    "Unable to play request. Internal stream error."
};

#endif /*_SRCERRS_H_*/

/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxresmgr.cpp,v 1.20 2008/01/31 22:23:18 cdunn Exp $
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

#include "hxcom.h"
#include "hlxclib/string.h"
#include "hxtypes.h"
#include "hxresult.h"
#include "hxbuffer.h"
#include "hxxres.h"
#include "hxxrsmg.h"
#include "dcoreres.h"
#include "saerrs.h"
#include "soerrs.h"
#include "pckunpck.h"
#include "hxresmgr.h"
/* This is the ONLY file that should be including hxerrors.h */
#include "hxerrors.h"
#if defined( _WIN32 ) || defined( _WINDOWS )
#include "hlxclib/windows.h"
#include "platform/win/rescode.h"
#elif defined (_MACINTOSH)
const short     RA_ERROR_STRINGS = 5000;
const short     RA_MSG_STRINGS = 5001;
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#ifdef _WINDOWS
extern HINSTANCE g_hInstance;
#endif

static struct errorStringTable
{
    HX_RESULT    m_ulErrorTag;
    UINT32       m_ulErrorStringID;
} const ErrorStringTable[] =
{
    {HXR_FAILED,                IDS_ERR_GENERAL_ERROR},                 // 1 "General error. An error occurred"
    {HXR_OUTOFMEMORY,           IDS_ERR_MEMORY_ERROR},                  // 2 "Out of memory."
    {HXR_INVALID_OPERATION,     IDS_ERR_INVALID_OPERATION},             // 3 "Invalid Operation."

    // state errors/return values
    {HXR_INVALID_PARAMETER,     IDS_ERR_INVALID_PARAMETER},             // 8 "Invalid parameter. Unable to process request."
    {HXR_NOT_INITIALIZED,       IDS_ERR_NOT_INITIALIZED_ERROR},         // 10 "Not initialized."

    // file errors
    {HXR_INVALID_FILE,          IDS_ERR_INVALID_FILE},                  // 11 "This document is not a RealAudio document."
    {HXR_INVALID_VERSION,       IDS_ERR_INVALID_VERSION},               // 12 "Invalid RealAudio file version number."
    {HXR_DOC_MISSING,           IDS_ERR_DOC_MISSING_ERROR},             // 14 "Requested file not found. The link you followed may be outdated or inaccurate."
    {HXR_BAD_FORMAT,            IDS_ERR_FORMAT_ERROR},                  // 15 "Unknown data format."
    {HXR_FULL_DOWNLOAD_NEEDED,  IDS_ERR_FULL_DOWNLOAD_NEEDED},          // 16 "This file is not optimized for Progressive Download."
    {HXR_NOT_SUPPORTED_FOR_LINEAR_FILE_SYSTEMS, IDS_ERR_NOT_SUPPORTED_FOR_LINEAR_FILE_SYSTEMS}, // 17 "This file contains unsupported content."

    // server errors
    {HXR_NET_SOCKET_INVALID,    IDS_ERR_NET_SOCKET_INVALID},            // 18 "Invalid socket error."
    {HXR_NET_CONNECT,           IDS_ERR_NET_CONNECT_ERROR},             // 19 "An error occurred while trying to connect to the RealAudio Server."
    {HXR_BIND,                  IDS_ERR_BIND_ERROR},                    // 20 "An error occurred binding to network socket."
    {HXR_SOCKET_CREATE,         IDS_ERR_SOCKET_CREATE_ERROR},           // 21 "An error occurred while creating a network socket."
    {HXR_INVALID_HOST,          IDS_ERR_INVALID_HOST},                  // 22 "Requested server is not valid."
    {HXR_INVALID_PATH,          IDS_ERR_INVALID_PATH},                  // 23 "Requested URL is not valid."
    {HXR_NET_READ,              IDS_ERR_NET_READ_ERROR},                // 24 "An error occurred while reading data from the network."
    {HXR_NET_WRITE,             IDS_ERR_NET_WRITE_ERROR},               // 25 "An error occurred while writing data to the network."
    {HXR_NET_UDP,               IDS_ERR_NET_UDP_ERROR},                 // 26 "Player cannot receive UDP data packets. You may wish to try the TCP data
    {HXR_HTTP_CONNECT,          IDS_ERR_HTTP_CONNECT_ERROR},            // 25
    {HXR_END_WITH_REASON,       IDS_ERR_END_WITH_REASON},
    //     option in the Network Preferences. You may also want to configure your player
    //     to use a firewall proxy. Please contact your system adminstrator for more information."
    {HXR_SERVER_TIMEOUT,        IDS_ERR_SERVER_TIMEOUT},                // 28 "Server timeout. Server not responding."
    {HXR_SERVER_DISCONNECTED,   IDS_ERR_SERVER_DISCONNECTED},           // 29 "Server disconnected. The server may be too busy or not available at this time."
    {HXR_DNR,                   IDS_ERR_DNR_ERROR},                     // 30 "Cannot resolve the requested network address."
    {HXR_OPEN_DRIVER,           IDS_ERR_OPEN_DRIVER_ERROR},             // 31 "Cannot open the network drivers."
    {HXR_BAD_SERVER,            IDS_ERR_BAD_SERVER_ERROR},              // 34 "This server is not using a recognized protocol."
    {HXR_ADVANCED_SERVER,       IDS_ERR_ADVANCED_SERVER_ERROR},         // 35 "You need a newer client to access this server. Please upgrade."
    {HXR_OLD_SERVER,            IDS_ERR_OLD_SERVER_ERROR},              // 36 "Connection closed. The host's version of the RealAudio Server is too old for this client."
    {HXR_SERVER_ALERT,          IDS_ERR_SERVER_ALERT},                  // 45 "Server alert"

    // decoder errors
    {HXR_DEC_NOT_FOUND,         IDS_ERR_DEC_NOT_FOUND},                 // 38 "File compression not supported. Cannot locate the requested RealAudio decoder."
    {HXR_DEC_INVALID,           IDS_ERR_DEC_INVALID},                   // 39 "Invalid decoder."
    {HXR_DEC_TYPE_MISMATCH,     IDS_ERR_DEC_TYPE_MISMATCH},             // 40 "Decoder type mismatch. Cannot load the requested decoder."
    {HXR_DEC_INIT_FAILED,       IDS_ERR_DEC_INIT_FAILED},               // 41 "Requested RealAudio Decoder cannot be found or cannot be used on this machine."
    {HXR_DEC_NOT_INITED,        IDS_ERR_DEC_NOT_INITED},                // 42 "RealAudio Decoder was not initialized before attempting to use it."
    {HXR_DEC_DECOMPRESS,        IDS_ERR_DEC_DECOMPRESS_ERROR},          // 43 "An error occurred during decoding."
    {HXR_NO_CODECS,             IDS_ERR_NO_CODECS},                     // 87 "No RealAudio Codecs have been installed on your system."

    // proxy errors
    {HXR_PROXY,                 IDS_ERR_PROXY_ERROR},                   // 46 "Proxy status error"
    {HXR_PROXY_RESPONSE,        IDS_ERR_PROXY_RESPONSE_ERROR},          // 47 "Proxy invalid response error"
    {HXR_ADVANCED_PROXY,        IDS_ERR_ADVANCED_PROXY_ERROR},          // 48 "You need a newer client to access this proxy. Please upgrade."
    {HXR_OLD_PROXY,             IDS_ERR_OLD_PROXY_ERROR},               // 49 "Connection closed. The proxy is too old for this client."

    // audio errors
    {HXR_AUDIO_DRIVER,          IDS_ERR_AUDIO_DRIVER_ERROR},            // 50 "Cannot open audio device."

    // parsing errors
    {HXR_INVALID_PROTOCOL,      IDS_ERR_INVALID_PROTOCOL_ERROR},        // 51 "Invalid protocol specified in URL."
    {HXR_INVALID_URL_OPTION,    IDS_ERR_INVALID_URL_OPTION_ERROR},      // 52 "Invalid option specified in URL."
    {HXR_INVALID_URL_HOST,      IDS_ERR_INVALID_URL_HOST},              // 53 "Invalid host string in requested URL."
    {HXR_INVALID_URL_PATH,      IDS_ERR_INVALID_URL_PATH},              // 54 "Invalid resource path string in requested URL."

    {HXR_GENERAL_NONET,         IDS_ERR_GENERAL_NONET},                 // 55 "Error locating Winsock Services."

    // More errors
    {HXR_PERFECTPLAY_NOT_SUPPORTED,     IDS_ERR_PERFECTPLAY_NOT_SUPPORTED},     // 75, Requested server does not support PerfectPlay.
    {HXR_NO_LIVE_PERFECTPLAY,           IDS_ERR_NO_LIVE_PERFECTPLAY},           // 76, PerfectPlay not supported for live streams.
    {HXR_PERFECTPLAY_NOT_ALLOWED,       IDS_ERR_PERFECTPLAY_NOT_ALLOWED},       // 78, PerfectPlay not allowed on this clip.
    {HXR_MULTICAST_JOIN,                IDS_ERR_MULTICAST_JOIN_ERROR},          // 84, An error occured attempting to join multicast session.
    {HXR_GENERAL_MULTICAST,             IDS_ERR_GENERAL_MULTICAST_ERROR},       // 85, An error occured accessing a multicast session.
    {HXR_MULTICAST_UDP,                 IDS_ERR_MULTICAST_UDP_ERROR},           // 86 "Player cannot receive UDP data packets from multicast session.
                                                                                // You may wish to try the TCP data option in the Network Preferences.
                                                                                // You may also want to configure your player to use a firewall proxy.
                                                                                // Please contact your system adminstrator for more information."
    {HXR_SLOW_MACHINE,                  IDS_ERR_SLOW_MACHINE},                  // 88 "Your machine does not have enough CPU power to play this file in real time."
    {HXR_INVALID_HTTP_PROXY_HOST,       IDS_ERR_INVALID_HTTP_PROXY_HOST},       // 90 Invalid hostname for HTTP proxy.
    {HXR_INVALID_METAFILE,              IDS_ERR_INVALID_METAFILE},              // 90 Invalid hostname for HTTP proxy.
    {HXR_NO_FILEFORMAT,                 IDS_ERR_MISSING_COMPONENTS},            // Missing components
    {HXR_NO_RENDERER,                   IDS_ERR_MISSING_COMPONENTS},            // Missing components
    {HXR_MISSING_COMPONENTS,            IDS_ERR_MISSING_COMPONENTS},            // Missing components
    {HXR_BAD_TRANSPORT,                 IDS_ERR_BAD_TRANSPORT},                 // 94 Transport not recognized
    {HXR_NOT_AUTHORIZED,                IDS_ERR_NOT_AUTHORIZED},                // 95 Access Denied
    {HXR_NOTENOUGH_BANDWIDTH,           IDS_ERR_NOTENOUGH_BANDWIDTH},
    {HXR_RIGHTS_EXPIRED,                IDS_ERR_RIGHTS_EXPIRED},                // 1082 Digital Rights have expired.
    {HXR_RESTORATION_COMPLETE,          IDS_ERR_RESTORATION_COMPLETE},          // restoration done.
    {HXR_BACKUP_COMPLETE,               IDS_ERR_BACKUP_COMPLETE},               // backup done.
    {HXR_TLC_NOT_CERTIFIED,             IDS_ERR_TLC_NOT_CERTIFIED},             // tlc not certified
    {HXR_CORRUPTED_BACKUP_FILE,         IDS_ERR_CORRUPTED_BACKUP_FILE},         // corrupted backup file
    {HXR_CHECK_RIGHTS,                  IDS_ERR_CHECK_RIGHTS},                  // Check for more Rights
    {HXR_RESTORE_SERVER_DENIED,         IDS_ERR_RESTORE_SERVER_DENIED},         // Restoration Denial from server.
    {HXR_DEBUGGER_DETECTED,             IDS_ERR_DEBUGGER_DETECTED},             // Debugger detected.
    {HXR_RESTORE_SERVER_CONNECT,        IDS_ERR_RESTORE_SERVER_CONNECT},        // Cannot connect to Restore server .
    {HXR_RESTORE_SERVER_TIMEOUT,        IDS_ERR_RESTORE_SERVER_TIMEOUT},        // Restore server timeout.
    {HXR_REVOKE_SERVER_CONNECT,         IDS_ERR_REVOKE_SERVER_CONNECT},         // Cannot connect to Revoke server.
    {HXR_REVOKE_SERVER_TIMEOUT,         IDS_ERR_REVOKE_SERVER_TIMEOUT},         // Revoke server timeout.


    {HXR_UNSUPPORTED_VIDEO,             IDS_ERR_UNSUPPORTED_VIDEO},             // 105 "The file contains an unsupported video format"
    {HXR_UNSUPPORTED_AUDIO,             IDS_ERR_UNSUPPORTED_AUDIO},             // 106 "The file contains an unsupported audio format"
    {HXR_TRY_AUTOCONFIG,                IDS_ERR_TRY_AUTOCONFIG},                // 107 "AutoConfig needs to be performed"
    {HXR_PROXY_DNR,                     IDS_ERR_PROXY_DNR_ERROR},               // 108 "Unable to locate proxy server. This proxy server does not have a DNS entry. Please check the proxy server name and try again"
    {HXR_PROXY_NET_CONNECT,             IDS_ERR_PROXY_NET_CONNECT_ERROR},       // 109 "Connection to proxy server could not be established. You may be experiencing network problems."
    {HXR_VIEW_SOURCE_NOCLIP,            IDS_ERR_VIEW_SOURCE_NOCLIP},            // 80 "No clip is avalible."
    {HXR_VIEW_SOURCE_DISSABLED,         IDS_ERR_VIEW_SOURCE_DISSABLED},         // 81 "The provider of this content has not granted permission to view the source."
    {HXR_VSRC_DISABLED,                 IDS_ERR_VSRC_DISABLED},                 // "Select More Info to learn about viewing clip source."
    {HXR_VSRC_NOCLIP,                   IDS_ERR_VSRC_NOCLIP},                   // "Select More Info to learn about viewing clip source."
    {HXR_WINDRAW_EXCEPTION,             IDS_ERR_WINDRAW_EXCEPTION},             // 98 "A windraw operation crashed."
    {HXR_VIEW_RIGHTS_NODRM,             IDS_ERR_VIEW_RIGHTS_NODRM},
    {HXR_VSRC_NODRM,                    IDS_ERR_VSRC_NODRM},
    {HXR_WM_OPL_NOT_SUPPORTED,		IDS_ERR_WM_OPL_NOT_SUPPORTED},
    {HXR_PROTOCOL_DOES_NOT_SUPPORT_DATATYPE,	IDS_ERR_PROTOCOL_DOES_NOT_SUPPORT_DATATYPE}, //101 "Protocol does not support this datatype."

    {HXR_PE_BAD_REQUEST,                         IDS_ERR_PE_BAD_REQUEST},                         // 400 'Bad Request'
    {HXR_PE_UNAUTHORIZED,                        IDS_ERR_PE_UNAUTHORIZED},                        // 401 'Unauthorized'
    {HXR_PE_PAYMENT_REQUIRED,                    IDS_ERR_PE_PAYMENT_REQUIRED},                    // 402 'Payment Required'
    {HXR_PE_FORBIDDEN,                           IDS_ERR_PE_FORBIDDEN},                           // 403 'Forbidden'
    {HXR_PE_NOT_FOUND,                           IDS_ERR_PE_NOT_FOUND},                           // 404 'Not Found'
    {HXR_PE_METHOD_NOT_ALLOWED,                  IDS_ERR_PE_METHOD_NOT_ALLOWED},                  // 405 'Method Not Allowed'
    {HXR_PE_NOT_ACCEPTABLE,                      IDS_ERR_PE_NOT_ACCEPTABLE},                      // 406 'Not Acceptable'
    {HXR_PE_PROXY_AUTHENTICATION_REQUIRED,       IDS_ERR_PE_PROXY_AUTHENTICATION_REQUIRED},       // 407 'Proxy Authentication Required'
    {HXR_PE_REQUEST_TIMEOUT,                     IDS_ERR_PE_REQUEST_TIMEOUT},                     // 408 'Request Time-out'
    {HXR_PE_GONE,                                IDS_ERR_PE_GONE},                                // 410 'Gone'
    {HXR_PE_LENGTH_REQUIRED,                     IDS_ERR_PE_LENGTH_REQUIRED},                     // 411 'Length Required'
    {HXR_PE_PRECONDITION_FAILED,                 IDS_ERR_PE_PRECONDITION_FAILED},                 // 412 'Precondition Failed'
    {HXR_PE_REQUEST_ENTITY_TOO_LARGE,            IDS_ERR_PE_REQUEST_ENTITY_TOO_LARGE},            // 413 'Request Entity Too Large'
    {HXR_PE_REQUESTURI_TOO_LARGE,                IDS_ERR_PE_REQUESTURI_TOO_LARGE},                // 414 'Request-URI Too Large'
    {HXR_PE_UNSUPPORTED_MEDIA_TYPE,              IDS_ERR_PE_UNSUPPORTED_MEDIA_TYPE},              // 415 'Unsupported Media Type'
    {HXR_PE_PARAMETER_NOT_UNDERSTOOD,            IDS_ERR_PE_PARAMETER_NOT_UNDERSTOOD},            // 451 'Parameter Not Understood'
    {HXR_PE_CONFERENCE_NOT_FOUND,                IDS_ERR_PE_CONFERENCE_NOT_FOUND},                // 452 'Conference Not Found'
    {HXR_PE_NOT_ENOUGH_BANDWIDTH,                IDS_ERR_PE_NOT_ENOUGH_BANDWIDTH},                // 453 'Not Enough Bandwidth'
    {HXR_PE_SESSION_NOT_FOUND,                   IDS_ERR_PE_SESSION_NOT_FOUND},                   // 454 'Session Not Found'
    {HXR_PE_METHOD_NOT_VALID_IN_THIS_STATE,      IDS_ERR_PE_METHOD_NOT_VALID_IN_THIS_STATE},      // 455 'Method Not Valid in This State'
    {HXR_PE_HEADER_FIELD_NOT_VALID_FOR_RESOURCE, IDS_ERR_PE_HEADER_FIELD_NOT_VALID_FOR_RESOURCE}, // 456 'Header Field Not Valid for Resource'
    {HXR_PE_INVALID_RANGE,                       IDS_ERR_PE_INVALID_RANGE},                       // 457 'Invalid Range'
    {HXR_PE_PARAMETER_IS_READONLY,               IDS_ERR_PE_PARAMETER_IS_READONLY},               // 458 'Parameter Is Read-Only'
    {HXR_PE_AGGREGATE_OPERATION_NOT_ALLOWED,     IDS_ERR_PE_AGGREGATE_OPERATION_NOT_ALLOWED},     // 459 'Aggregate operation not allowed'
    {HXR_PE_ONLY_AGGREGATE_OPERATION_ALLOWED,    IDS_ERR_PE_ONLY_AGGREGATE_OPERATION_ALLOWED},    // 460 'Only aggregate operation allowed'
    {HXR_PE_UNSUPPORTED_TRANSPORT,               IDS_ERR_PE_UNSUPPORTED_TRANSPORT},               // 461 'Unsupported transport'
    {HXR_PE_DESTINATION_UNREACHABLE,             IDS_ERR_PE_DESTINATION_UNREACHABLE},             // 462 'Destination unreachable'
    {HXR_PE_INTERNAL_SERVER_ERROR,               IDS_ERR_PE_INTERNAL_SERVER_ERROR},               // 500 'Internal Server Error'
    {HXR_PE_NOT_IMPLEMENTED,                     IDS_ERR_PE_NOT_IMPLEMENTED},                     // 501 'Not Implemented'
    {HXR_PE_BAD_GATEWAY,                         IDS_ERR_PE_BAD_GATEWAY},                         // 502 'Bad Gateway'
    {HXR_PE_SERVICE_UNAVAILABLE,                 IDS_ERR_PE_SERVICE_UNAVAILABLE},                 // 503 'Service Unavailable'
    {HXR_PE_GATEWAY_TIMEOUT,                     IDS_ERR_PE_GATEWAY_TIMEOUT},                     // 504 'Gateway Time-out'
    {HXR_PE_RTSP_VERSION_NOT_SUPPORTED,          IDS_ERR_PE_RTSP_VERSION_NOT_SUPPORTED},          // 505 'RTSP Version not supported'
    {HXR_PE_OPTION_NOT_SUPPORTED,                IDS_ERR_PE_OPTION_NOT_SUPPORTED},                // 551 'Option not supported'



    // server alerts
    {HXR_SE_NO_ERROR,                     IDS_SE_NO_ERROR},
    {HXR_SE_INVALID_VERSION,              IDS_SE_INVALID_VERSION},
    {HXR_SE_INVALID_FORMAT,               IDS_SE_INVALID_FORMAT},
    {HXR_SE_INVALID_BANDWIDTH,            IDS_SE_INVALID_BANDWIDTH},
    {HXR_SE_INVALID_PATH,                 IDS_SE_INVALID_PATH},
    {HXR_SE_UNKNOWN_PATH,                 IDS_SE_UNKNOWN_PATH},
    {HXR_SE_INVALID_PROTOCOL,             IDS_SE_INVALID_PROTOCOL},
    {HXR_SE_INVALID_PLAYER_ADDR,          IDS_SE_INVALID_PLAYER_ADDR},
    {HXR_SE_LOCAL_STREAMS_PROHIBITED,     IDS_SE_LOCAL_STREAMS_PROHIBITED},
    {HXR_SE_SERVER_FULL,                  IDS_SE_SERVER_FULL},
    {HXR_SE_REMOTE_STREAMS_PROHIBITED,    IDS_SE_REMOTE_STREAMS_PROHIBITED},
    {HXR_SE_EVENT_STREAMS_PROHIBITED,     IDS_SE_EVENT_STREAMS_PROHIBITED},
    {HXR_SE_INVALID_HOST,                 IDS_SE_INVALID_HOST},
    {HXR_SE_NO_CODEC,                     IDS_SE_NO_CODEC},
    {HXR_SE_LIVEFILE_INVALID_BWN,         IDS_SE_LIVEFILE_INVALID_BWN},
    {HXR_SE_UNABLE_TO_FULFILL,            IDS_SE_UNABLE_TO_FULFILL},
    {HXR_SE_MULTICAST_DELIVERY_ONLY,      IDS_SE_MULTICAST_DELIVERY_ONLY},
    {HXR_SE_LICENSE_EXCEEDED,             IDS_SE_LICENSE_EXCEEDED},
    {HXR_SE_LICENSE_UNAVAILABLE,          IDS_SE_LICENSE_UNAVAILABLE},
    {HXR_SE_INVALID_LOSS_CORRECTION,      IDS_SE_INVALID_LOSS_CORRECTION},
    {HXR_SE_PROTOCOL_FAILURE,             IDS_SE_PROTOCOL_FAILURE},
    {HXR_SE_REALVIDEO_STREAMS_PROHIBITED, IDS_SE_REALVIDEO_STREAMS_PROHIBITED},
    {HXR_SE_REALAUDIO_STREAMS_PROHIBITED, IDS_SE_REALAUDIO_STREAMS_PROHIBITED},
    {HXR_SE_DATATYPE_UNSUPPORTED,         IDS_SE_DATATYPE_UNSUPPORTED},
    {HXR_SE_DATATYPE_UNLICENSED,          IDS_SE_DATATYPE_UNLICENSED},
    {HXR_SE_RESTRICTED_PLAYER,            IDS_SE_RESTRICTED_PLAYER},
    {HXR_SE_STREAM_INITIALIZING,          IDS_SE_STREAM_INITIALIZING},
    {HXR_SE_INVALID_PLAYER,               IDS_SE_INVALID_PLAYER},
    {HXR_SE_PLAYER_PLUS_ONLY,             IDS_SE_PLAYER_PLUS_ONLY},
    {HXR_SE_NO_EMBEDDED_PLAYERS,          IDS_SE_NO_EMBEDDED_PLAYERS},
    {HXR_SE_PNA_PROHIBITED,               IDS_SE_PNA_PROHIBITED},
    {HXR_SE_AUTHENTICATION_UNSUPPORTED,   IDS_SE_AUTHENTICATION_UNSUPPORTED},
    {HXR_SE_MAX_FAILED_AUTHENTICATIONS,   IDS_SE_MAX_FAILED_AUTHENTICATIONS},
    {HXR_SE_AUTH_ACCESS_DENIED,           IDS_SE_AUTH_ACCESS_DENIED},
    {HXR_SE_AUTH_UUID_READ_ONLY,          IDS_SE_AUTH_UUID_READ_ONLY},
    {HXR_SE_AUTH_UUID_NOT_UNIQUE,         IDS_SE_AUTH_UUID_NOT_UNIQUE},
    {HXR_SE_AUTH_NO_SUCH_USER,            IDS_SE_AUTH_NO_SUCH_USER},
    {HXR_SE_AUTH_REGISTRATION_SUCCEEDED,  IDS_SE_AUTH_REGISTRATION_SUCCEEDED},
    {HXR_SE_AUTH_REGISTRATION_FAILED,     IDS_SE_AUTH_REGISTRATION_FAILED},
    {HXR_SE_AUTH_REGISTRATION_GUID_REQUIRED, IDS_SE_AUTH_REGISTRATION_GUID_REQUIRED},
    {HXR_SE_AUTH_UNREGISTERED_PLAYER,     IDS_SE_AUTH_UNREGISTERED_PLAYER},
    {HXR_SE_AUTH_TIME_EXPIRED,            IDS_SE_AUTH_TIME_EXPIRED},
    {HXR_SE_AUTH_NO_TIME_LEFT,            IDS_SE_AUTH_NO_TIME_LEFT},
    {HXR_SE_AUTH_ACCOUNT_LOCKED,          IDS_SE_AUTH_ACCOUNT_LOCKED},
    {HXR_SE_AUTH_INVALID_SERVER_CFG,      IDS_SE_AUTH_INVALID_SERVER_CFG},
    {HXR_SE_NO_MOBILE_DOWNLOAD,           IDS_SE_NO_MOBILE_DOWNLOAD},
    {HXR_SE_NO_MORE_MULTI_ADDR,           IDS_SE_NO_MORE_MULTI_ADDR},
    {HXR_PE_PROXY_MAX_CONNECTIONS,        IDS_PE_PROXY_MAX_CONNECTIONS},
    {HXR_PE_PROXY_MAX_GW_BANDWIDTH,       IDS_PE_PROXY_MAX_GW_BANDWIDTH},
    {HXR_PE_PROXY_MAX_BANDWIDTH,          IDS_PE_PROXY_MAX_BANDWIDTH},
    {HXR_SE_BAD_LOADTEST_PASSWORD,        IDS_SE_BAD_LOADTEST_PASSWORD},
    {HXR_SE_PNA_NOT_SUPPORTED,            IDS_SE_PNA_NOT_SUPPORTED},
    {HXR_PE_PROXY_ORIGIN_DISCONNECTED,    IDS_PE_PROXY_ORIGIN_DISCONNECTED},
    {HXR_SE_INTERNAL_ERROR,               IDS_SE_INTERNAL_ERROR},
    {HXR_FORBIDDEN,                       IDS_SE_FORBIDDEN},

    {HXR_SOCK_INTR,                       IDS_SOCK_INTR},
    {HXR_SOCK_BADF,                       IDS_SOCK_BADF},
    {HXR_SOCK_ACCES,                      IDS_SOCK_ACCES},
    {HXR_SOCK_FAULT,                      IDS_SOCK_FAULT},
    {HXR_SOCK_INVAL,                      IDS_SOCK_INVAL},
    {HXR_SOCK_MFILE,                      IDS_SOCK_MFILE},
    {HXR_SOCK_WOULDBLOCK,                 IDS_SOCK_WOULDBLOCK},
    {HXR_SOCK_INPROGRESS,                 IDS_SOCK_INPROGRESS},
    {HXR_SOCK_ALREADY,                    IDS_SOCK_ALREADY},
    {HXR_SOCK_NOTSOCK,                    IDS_SOCK_NOTSOCK},
    {HXR_SOCK_DESTADDRREQ,                IDS_SOCK_DESTADDRREQ},
    {HXR_SOCK_MSGSIZE,                    IDS_SOCK_MSGSIZE },
    {HXR_SOCK_PROTOTYPE,                  IDS_SOCK_PROTOTYPE},
    {HXR_SOCK_NOPROTOOPT,                 IDS_SOCK_NOPROTOOPT},
    {HXR_SOCK_PROTONOSUPPORT,             IDS_SOCK_PROTONOSUPPORT},
    {HXR_SOCK_SOCKTNOSUPPORT,             IDS_SOCK_SOCKTNOSUPPORT},
    {HXR_SOCK_OPNOTSUPP,                  IDS_SOCK_OPNOTSUPP},
    {HXR_SOCK_PFNOSUPPORT,                IDS_SOCK_PFNOSUPPORT},
    {HXR_SOCK_AFNOSUPPORT,                IDS_SOCK_AFNOSUPPORT},
    {HXR_SOCK_ADDRINUSE,                  IDS_SOCK_ADDRINUSE},
    {HXR_SOCK_ADDRNOTAVAIL,               IDS_SOCK_ADDRNOTAVAIL },
    {HXR_SOCK_NETDOWN,                    IDS_SOCK_NETDOWN},
    {HXR_SOCK_NETUNREACH,                 IDS_SOCK_NETUNREACH},
    {HXR_SOCK_NETRESET,                   IDS_SOCK_NETRESET},
    {HXR_SOCK_CONNABORTED,                IDS_SOCK_CONNABORTED},
    {HXR_SOCK_CONNRESET,                  IDS_SOCK_CONNRESET},
    {HXR_SOCK_NOBUFS,                     IDS_SOCK_NOBUFS},
    {HXR_SOCK_ISCONN,                     IDS_SOCK_ISCONN},
    {HXR_SOCK_NOTCONN,                    IDS_SOCK_NOTCONN},
    {HXR_SOCK_SHUTDOWN,                   IDS_SOCK_SHUTDOWN},
    {HXR_SOCK_TOOMANYREFS,                IDS_SOCK_TOOMANYREFS},
    {HXR_SOCK_TIMEDOUT,                   IDS_SOCK_TIMEDOUT},
    {HXR_SOCK_CONNREFUSED,                IDS_SOCK_CONNREFUSED},
    {HXR_SOCK_LOOP,                       IDS_SOCK_LOOP},
    {HXR_SOCK_NAMETOOLONG,                IDS_SOCK_NAMETOOLONG},
    {HXR_SOCK_HOSTDOWN,                   IDS_SOCK_HOSTDOWN},
    {HXR_SOCK_HOSTUNREACH,                IDS_SOCK_HOSTUNREACH},
    {HXR_SOCK_PIPE,                       IDS_SOCK_PIPE},
    {HXR_SOCK_ENDSTREAM,                  IDS_SOCK_ENDSTREAM},
    {HXR_SOCK_BUFFERED,                   IDS_SOCK_BUFFERED},

    {HXR_SOCK_NOTEMPTY,                   IDS_SOCK_NOTEMPTY},
    {HXR_SOCK_PROCLIM,                    IDS_SOCK_PROCLIM},
    {HXR_SOCK_USERS,                      IDS_SOCK_USERS},
    {HXR_SOCK_DQUOT,                      IDS_SOCK_DQUOT},
    {HXR_SOCK_STALE,                      IDS_SOCK_STALE},
    {HXR_SOCK_REMOTE,                     IDS_SOCK_REMOTE},
    {HXR_SOCK_SYSNOTREADY,                IDS_SOCK_SYSNOTREADY},
    {HXR_SOCK_VERNOTSUPPORTED,            IDS_SOCK_VERNOTSUPPORTED},
    {HXR_SOCK_NOTINITIALISED,             IDS_SOCK_NOTINITIALISED},
    {HXR_SOCK_DISCON,                     IDS_SOCK_DISCON},
    {HXR_SOCK_NOMORE,                     IDS_SOCK_NOMORE},
    {HXR_SOCK_CANCELLED,                  IDS_SOCK_CANCELLED},
    {HXR_SOCK_INVALIDPROCTABLE,           IDS_SOCK_INVALIDPROCTABLE},
    {HXR_SOCK_INVALIDPROVIDER,            IDS_SOCK_INVALIDPROVIDER},
    {HXR_SOCK_PROVIDERFAILEDINIT,         IDS_SOCK_PROVIDERFAILEDINIT},
    {HXR_SOCK_SYSCALLFAILURE,             IDS_SOCK_SYSCALLFAILURE},
    {HXR_SOCK_SERVICE_NOT_FOUND,          IDS_SOCK_SERVICE_NOT_FOUND},
    {HXR_SOCK_TYPE_NOT_FOUND,             IDS_SOCK_TYPE_NOT_FOUND},
    {HXR_SOCK_E_NO_MORE,                  IDS_SOCK_E_NO_MORE},
    {HXR_SOCK_E_CANCELLED,                IDS_SOCK_E_CANCELLED},
    {HXR_SOCK_REFUSED,                    IDS_SOCK_REFUSED},
    {HXR_SOCK_HOST_NOT_FOUND,             IDS_SOCK_HOST_NOT_FOUND},
    {HXR_SOCK_TRY_AGAIN,                  IDS_SOCK_TRY_AGAIN},
    {HXR_SOCK_NO_RECOVERY,                IDS_SOCK_NO_RECOVERY},
    {HXR_SOCK_NO_DATA,                    IDS_SOCK_NO_DATA},

    {HXR_RAVE_UI_FAIL,                    IDS_ERR_RAVE_UI}
};


/*
 *      Class Constructor/Destructor
 */
CHXResMgr::CHXResMgr(IUnknown* pContext)
            : m_pContext(NULL)
            , m_pExternalResMgr(NULL)
            , m_pExternalResRdr(NULL)
{
    m_pContext = pContext;
    if (m_pContext)
    {
        m_pContext->AddRef();

        if (HXR_OK != m_pContext->QueryInterface(IID_IHXExternalResourceManager,
                                                 (void**)&m_pExternalResMgr))
        {
            m_pExternalResMgr = NULL;
            goto cleanup;
        }

        if(HXR_OK != m_pExternalResMgr->CreateExternalResourceReader(
                                        CORE_RESOURCE_SHORT_NAME, m_pExternalResRdr))
        {
            m_pExternalResRdr = NULL;
            goto cleanup;
        }

#ifdef _WINDOWS
#ifndef _WINCE
        GetModuleFileName((HMODULE)g_hInstance, m_szDLLPath, sizeof(m_szDLLPath));
#else
        GetModuleFileName((HMODULE)g_hInstance, OS_STRING2(m_szDLLPath, sizeof(m_szDLLPath)), sizeof(m_szDLLPath));
#endif
        m_pExternalResRdr->SetDefaultResourceFile(m_szDLLPath);
#endif /* _WINDOWS */
    }

cleanup:

    return;
}

CHXResMgr::~CHXResMgr()
{
    HX_RELEASE(m_pExternalResRdr);
    HX_RELEASE(m_pExternalResMgr);
    HX_RELEASE(m_pContext);
}

/*
 *      General Class Methods
 */

//      A member function for returning an error string that corresponds to an
//      error Id.
IHXBuffer* CHXResMgr::GetErrorString( HX_RESULT ErrId )
{
    IHXBuffer*  pBuffer = NULL;
    const char* str = NULL;
    HXBOOL        bFound = FALSE;

    UINT32      ulErrorStringID = ErrorStringTable[0].m_ulErrorStringID;

    for (int i = 0;
        i < sizeof(ErrorStringTable) / sizeof(ErrorStringTable[0]);
        ++i)
    {
        if(ErrorStringTable[i].m_ulErrorTag == ErrId)
        {
            ulErrorStringID = ErrorStringTable[i].m_ulErrorStringID;
            bFound = TRUE;
            break;
        }
    }

    if (m_pExternalResRdr && bFound)
    {
        IHXXResource* pRes = m_pExternalResRdr->GetResource(HX_RT_STRING,
                                                             ulErrorStringID);
        if(pRes)
        {
            str = (const char*)pRes->ResourceData();
            if (str)
            {
		CreateAndSetBufferCCF(pBuffer, (UCHAR*)str, strlen(str) + 1, m_pContext);
            }
        }
        HX_RELEASE(pRes);
    }

    return pBuffer;
}

IHXBuffer* CHXResMgr::GetMiscString(UINT32 ResourceId)
{
    IHXBuffer* pBuffer = NULL;
    const char* str = NULL;

    IHXXResource* pRes = m_pExternalResRdr->GetResource(HX_RT_STRING,
                                                         ResourceId);
    if(pRes)
    {
        str = (const char*)pRes->ResourceData();
        if (str)
        {
	    CreateAndSetBufferCCF(pBuffer, (UCHAR*)str, strlen(str) + 1, m_pContext);
        }
    }
    HX_RELEASE(pRes);

    return pBuffer;
}

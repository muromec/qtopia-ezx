/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxerrors.h,v 1.3 2004/07/09 18:20:48 hubbe Exp $
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

#ifdef __MWERKS__
#pragma once
#endif

#ifndef _HXERRORS
#define _HXERRORS


enum HX_ERROR
{
	HX_NO_ERROR			= 0,			// 0 
	HX_GENERAL_ERROR	= 1,			// 1 General error. An error occurred.
	HX_MEMORY_ERROR				= 2,	// 2 Out of memory.

	// state errors/return values
	HX_AT_INTERRUPT 			= 3,	// 3 Cannot process at interrupt level. Deferring request to system task level.
	HX_BUFFERING 				= 4,	// 4 Deferring request until after buffering is completed.
	HX_PAUSED 					= 5,	// 5 Paused.
	HX_NO_DATA 					= 6,	// 6 There is no data waiting to be processed.
	HX_AT_END 					= 7,	// 7 Arrived at the end of the document.
	HX_INVALID_PARAMETER 		= 8,	// 8 Invalid parameter. Unable to process request.
	HX_INVALID_OPERATION 		= 9,	// 9 Invalid operation. Cannot process request.
	HX_NOT_INITIALIZED_ERROR 	= 10,	// 10 Not initialized.

	// file errors
	HX_INVALID_RA_FILE 			= 11,	// 11 This document is not a RealAudio document.
	HX_INVALID_VERSION 			= 12,	// 12 Invalid RealAudio file version number.
	HX_INVALID_REVISION 		= 13,	// 13 Invalid revision number in RealAudio file.
	HX_DOC_MISSING_ERROR 		= 14,	// 14 Requested file not found. The link you followed may be outdated or inaccurate.
	HX_FORMAT_ERROR 			= 15,	// 15 Unknown data format.
	HX_CHUNK_MISSING 			= 16,	// 16 RealAudio file is missing the requested data chunk.
	HX_INVALID_INTERLEAVER 		= 17,	// 17 Cannot locate the requested interleaver.
	
	// server errors
	HX_NET_SOCKET_INVALID 		= 18,	// 18 Invalid socket error.
	HX_NET_CONNECT_ERROR 		= 19,	// 19 An error occurred while trying to connect to the RealAudio Server.
	HX_BIND_ERROR 				= 20,	// 20 An error occurred binding to network socket.
	HX_SOCKET_CREATE_ERROR 		= 21,	// 21 An error occurred while creating a network socket.
	HX_INVALID_HOST 			= 22,	// 22 Requested server is not valid.
	HX_INVALID_PATH 			= 23,	// 23 Requested URL is not valid.
	HX_NET_READ_ERROR 			= 24,	// 24 An error occurred while reading data from the network.
	HX_NET_WRITE_ERROR			= 25,	// 25 An error occurred while writing data to the network.
	HX_NET_UDP_ERROR 			= 26,	// 26 Player cannot receive UDP data packets. You may wish to try the TCP data option in the Network Preferences. 
										//    You may also want to configure your player to use a firewall proxy. Please contact your system administrator for 
										//    more information.
	HX_RETRY_ERROR 				= 27,	// 27 Attempting to reconnect to the RealAudio server.
	HX_SERVER_TIMEOUT 			= 28,	// 28 Server timeout. Server not responding.
	HX_SERVER_DISCONNECTED 		= 29,	// 29 Server disconnected. The server may be too busy or not available at this time.
	HX_DNR_ERROR 				= 30,	// 30 Cannot resolve the requested network address.
	HX_OPEN_DRIVER_ERROR 		= 31,	// 31 Cannot open the network drivers.
	HX_WOULD_BLOCK_ERROR 		= 32,	// 32 Network operation would block.
	HX_UPGRADE_ERROR 			= 33,	// 33 You need a newer client to access this server. Please upgrade.
	HX_BAD_SERVER_ERROR 		= 34,	// 34 This server is not using a recognized protocol.
	HX_ADVANCED_SERVER_ERROR 	= 35,	// 35 You need a newer client to access this server. Please upgrade.
	HX_OLD_SERVER_ERROR 		= 36,	// 36 Connection closed. The host's version of the RealAudio Server is too old for this client.
								  
	// decoder errors
	HX_DEC_INITED 				= 37,	// 37 RealAudio Decoder is initialized.
	HX_DEC_NOT_FOUND 			= 38,	// 38 File compression not supported. Cannot locate the requested RealAudio decoder.
	HX_DEC_INVALID 				= 39,	// 39 Invalid decoder.
	HX_DEC_TYPE_MISMATCH 		= 40,	// 40 Decoder type mismatch. Cannot load the requested decoder.
	HX_DEC_INIT_FAILED 			= 41,	// 41 Requested RealAudio Decoder cannot be found or cannot be used on this machine.
	HX_DEC_NOT_INITED 			= 42,	// 42 RealAudio Decoder was not initialized before attempting to use it.
	HX_DEC_DECOMPRESS_ERROR 	= 43,	// 43 An error occurred during decoding.

	HX_REDIRECTION 				= 44,	// 44 Client is being redirected to a new server.

	HX_SERVER_ALERT 			= 45,	// 45 Server alert.
	
	// proxy errors
	HX_PROXY_ERROR 				= 46,	// 46 Proxy status error.
	HX_PROXY_RESPONSE_ERROR 	= 47,	// 47 Proxy invalid response error.
	HX_ADVANCED_PROXY_ERROR 	= 48,	// 48 You need a newer client to access this proxy. Please upgrade.
	HX_OLD_PROXY_ERROR 			= 49,	// 49 Connection closed. The proxy is too old for this client.

	// audio errors
	HX_AUDIO_DRIVER_ERROR 		= 50,	// 50 Cannot open audio device.

	// parsing errors
	HX_INVALID_PROTOCOL_ERROR 	= 51,	// 51 Invalid protocol specified in URL.
	HX_INVALID_URL_OPTION_ERROR = 52,	// 52 Invalid option specified in URL.
	HX_INVALID_URL_HOST 		= 53,	// 53 Invalid host string in requested URL.
	HX_INVALID_URL_PATH 		= 54,	// 54 Invalid resource path string in requested URL.

	HX_GENERAL_NONET 			= 55,	// 55 Error locating Winsock Services.

	// notification errors
	HX_NOTIFICATION_ERROR 		= 56,	// 56 Error sending notification to client.
	HX_NOT_NOTIFIED 			= 57,	// 57, Not an ERROR... This is a code used to indicate that the client did not request to recieve the notification.
	HX_STOPPED 					= 58,	// 58, Not sent to user, the clip has reached the end or has been terminated.
	HX_OPEN_NOT_PROCESSED 		= 59,	// 59, Not sent to user, Open command have not been processed (to many connections opened).
	HX_BLOCK_CANCELED 			= 60,	// 60, Not sent to user, Blocking call was canceled.
	HX_CLOSED 					= 61,	// 61, Not send to user, the clip was previously closed.

	// encoder errors
	HX_ENC_FILE_TOO_SMALL 		= 62,	// 62, File is too small.
	HX_ENC_UNKNOWN_FILE 		= 63,	// 63, Unknown file type.
	HX_ENC_BAD_CHANNELS 		= 64,	// 64, Invalid number of encoder channels.
	HX_ENC_BAD_SAMPSIZE  		= 65,	// 65, Invalid encoder sample size.
	HX_ENC_BAD_SAMPRATE 		= 66,	// 66, Invalid encoder sample rate.
	HX_ENC_INVALID              = 67,   // 67, Invalid encoder.
    HX_ENC_NO_OUTPUT_FILE       = 68,   // 68, Output file not found.
    HX_ENC_NO_INPUT_FILE        = 69,   // 69, Input file not found.
	HX_ENC_NO_OUTPUT_PERMISSIONS= 70,	// 70, Encoder output permission error.
	HX_ENC_BAD_FILETYPE	        = 71,   // 71, Input file type not supported.

	// errors related to expiration
	HX_EXPIRED					= 72,	// 72, This product is expired. Please upgrade to the latest version from http://www.realaudio.com.
   	
//	$Comstrip:  IF SDK
//	HX_RESERVED_ERROR_1			= 73,	// 73 	
//	HX_COULDNOTINITCORE_ERROR	= 74,	// 74, An error occurred while initializing the RealAudio Daemon.
//	HX_RESERVED_ERROR_2			= 75,	// 75
//	HX_RESERVED_ERROR_3			= 76,	// 76
//	HX_RESERVED_ERROR_4			= 77,	// 77
//	HX_RESERVED_ERROR_5			= 78,	// 78
//	$Comstrip:  ELSE
	HX_RECORD_ERROR				= 73,	// 73, An error occurred while trying to record the clip. The clip was not recorded.
	HX_COULDNOTINITCORE_ERROR	= 74,	// 74, An error occurred while initializing the RealAudio Daemon.
	HX_PERFECTPLAY_NOT_SUPPORTED= 75,	// 75, Requested server does not support PerfectPlay.
	HX_NO_LIVE_PERFECTPLAY		= 76,	// 76, PerfectPlay not supported for live streams.
	HX_RECORD_WRITE_ERROR		= 77,	// 77, An error occurred while recording clip to file.
	HX_PERFECTPLAY_NOT_ALLOWED	= 78,	// 78, PerfectPlay not allowed on this clip.
	HX_INVALID_WAV_FILE			= 79	// 79, File is not a RIFF WAV file.
	,HX_INVALID_PROTOCOL		= 80	// 80, Protocol not supported by server.
	,HX_NO_SEEK					= 81	// 81, No seek possible.
	,HX_INVALID_STREAM			= 82	// 82, Invalid .RM stream.
	,HX_TEMP_FILE_ERROR			= 83	// 83, An error occurred accessing a temporary file.
	,HX_MULTICAST_JOIN_ERROR	= 84	// 84, An error occurred attempting to join multicast session.
	,HX_GENERAL_MULTICAST_ERROR	= 85	// 85, An error occurred accessing a multicast session.
	,HX_MULTICAST_UDP_ERROR		= 86	// 86, Player cannot receive audio data from the multicast session. You may wish to try the TCP data option in the Network Preferences. Please contact your system administrator for more information.
	,HX_NO_CODECS				= 87	// 87, No RealAudio Codecs have been installed on your system.
	,HX_SLOW_MACHINE			= 88	// 88, Your machine does not have enough CPU power to play this file in real time. If you are accessing a network link, you may try setting your bandwidth to a smaller value, since bandwidth usage is often related to playback complexity. 
	,HX_FORCE_PERFECTPLAY 		= 89	// 89, server protocol < 10, must reconnect in TCP mode for PerfectPlay to work
	,HX_INVALID_HTTP_PROXY_HOST = 90	// 90, Invalid hostname for HTTP proxy.

	//
	// errors added for the video encoder
	//

	,HX_ENC_INVALID_VIDEO = 91			// 91, The source file contains an unsupported video format
	,HX_ENC_INVALID_AUDIO = 92			// 92, The source file contains an unsupported audio format
	,HX_ENC_NO_VIDEO_CAPTURE = 93		// 93, Unable to initialize the video capture device
	,HX_ENC_INVALID_VIDEO_CAPTURE = 94	// 94, The video capture device format is unsupported
	,HX_ENC_NO_AUDIO_CAPTURE = 95		// 95, Unable to initialize the audio capture device
	,HX_ENC_INVALID_AUDIO_CAPTURE = 96	// 96, The audio capture device format is unsupported
	,HX_ENC_TOO_SLOW_FOR_LIVE = 97		// 97, Not enough resources to maintain live encoding
	,HX_ENC_ENGINE_NOT_INITIALIZED = 98	// 98, The encoding engine is not initialized
	,HX_ENC_CODEC_NOT_FOUND = 99		// 99, The requested codec was not found
	,HX_ENC_CODEC_NOT_INITIALIZED = 100	// 100, Codec initialization failed
	,HX_ENC_INVALID_INPUT_DIMENSIONS = 101	// 101, Invalid input video frame dimensions

	//
	// errors added for rtsl parsing
	//

	,HX_INVALID_METAFILE = 102			// 102, Invalid RTSL file

	,HX_NO_RENDERER = 103			// 103, Renderer not found
	,HX_NO_FILEFORMAT = 104			// 104, File format not found

	,HX_UNSUPPORTED_VIDEO = 105		// 105, The file contains an unsupported video format
	,HX_UNSUPPORTED_AUDIO = 106		// 106, The file contains an unsupported audio format

	,HX_LAST_ERROR_POS					// used to mark end of HX_ERROR enum

};

#endif  // _HXERRORS





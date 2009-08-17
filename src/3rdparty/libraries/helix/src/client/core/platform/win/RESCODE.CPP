/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: RESCODE.CPP,v 1.6 2005/11/01 00:02:28 rishimathew Exp $
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

#include "hxcom.h"

#include "hxtypes.h"
#include "ResCode.h"
#include "resource.h"
#include "hxresult.h"
#include "hxassert.h"
#include "dcoreres.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

UINT32 ResCodeStr::LookupTable[_MAX_STRINGS][2] = 
{
	(UINT32)HXR_FAILED,        		IDS_ERR_GENERAL_ERROR,				// 1 "General error. An error occurred"
	(UINT32)HXR_OUTOFMEMORY,         	IDS_ERR_MEMORY_ERROR,				// 2 "Out of memory."
        (UINT32)HXR_INVALID_OPERATION,          IDS_ERR_INVALID_OPERATION,                      // 3 "Invalid Operation."

	// state errors/return values
	(UINT32)HXR_INVALID_PARAMETER,    	IDS_ERR_INVALID_PARAMETER,			// 8 "Invalid parameter. Unable to process request."
	(UINT32)HXR_NOT_INITIALIZED,		IDS_ERR_NOT_INITIALIZED_ERROR,		// 10 "Not initialized."

	// file errors
	(UINT32)HXR_INVALID_FILE,	     	IDS_ERR_INVALID_FILE,			// 11 "This document is not a RealAudio document."
	(UINT32)HXR_INVALID_VERSION,	     	IDS_ERR_INVALID_VERSION,			// 12 "Invalid RealAudio file version number."
	(UINT32)HXR_DOC_MISSING,	 	IDS_ERR_DOC_MISSING_ERROR,			// 14 "Requested file not found. The link you followed may be outdated or inaccurate."
	(UINT32)HXR_BAD_FORMAT,         	IDS_ERR_FORMAT_ERROR,				// 15 "Unknown data format."
	(UINT32)HXR_FULL_DOWNLOAD_NEEDED,   IDS_ERR_FULL_DOWNLOAD_NEEDED,				// 16 "This file is not optimized for Progressive Download."
	
	// server errors
	(UINT32)HXR_NET_SOCKET_INVALID,	 	IDS_ERR_NET_SOCKET_INVALID,			// 18 "Invalid socket error."
	(UINT32)HXR_NET_CONNECT,    		IDS_ERR_NET_CONNECT_ERROR,			// 19 "An error occurred while trying to connect to the RealAudio Server."
	(UINT32)HXR_BIND,	         	IDS_ERR_BIND_ERROR,					// 20 "An error occurred binding to network socket."
	(UINT32)HXR_SOCKET_CREATE,	 	IDS_ERR_SOCKET_CREATE_ERROR,		// 21 "An error occurred while creating a network socket."
	(UINT32)HXR_INVALID_HOST,         	IDS_ERR_INVALID_HOST,				// 22 "Requested server is not valid."
	(UINT32)HXR_INVALID_PATH,         	IDS_ERR_INVALID_PATH,				// 23 "Requested URL is not valid."
	(UINT32)HXR_NET_READ,	     		IDS_ERR_NET_READ_ERROR,				// 24 "An error occurred while reading data from the network."
	(UINT32)HXR_NET_WRITE,	     		IDS_ERR_NET_WRITE_ERROR,			// 25 "An error occurred while writing data to the network."
	(UINT32)HXR_NET_UDP,        		IDS_ERR_NET_UDP_ERROR,				// 26 "Player cannot receive UDP data packets. You may wish to try the TCP data 
	(UINT32)HXR_HTTP_CONNECT,		IDS_ERR_HTTP_CONNECT_ERROR,			// 25
        (UINT32)HXR_END_WITH_REASON,            IDS_ERR_END_WITH_REASON,
	//	   option in the Network Preferences. You may also want to configure your player 
	//	   to use a firewall proxy. Please contact your system adminstrator for more information."
	(UINT32)HXR_SERVER_TIMEOUT,	     	IDS_ERR_SERVER_TIMEOUT,				// 28 "Server timeout. Server not responding."
	(UINT32)HXR_SERVER_DISCONNECTED,	IDS_ERR_SERVER_DISCONNECTED,		// 29 "Server disconnected. The server may be too busy or not available at this time."
	(UINT32)HXR_DNR,	         	IDS_ERR_DNR_ERROR,					// 30 "Cannot resolve the requested network address."
	(UINT32)HXR_OPEN_DRIVER,    		IDS_ERR_OPEN_DRIVER_ERROR,			// 31 "Cannot open the network drivers."
	(UINT32)HXR_BAD_SERVER,     		IDS_ERR_BAD_SERVER_ERROR,			// 34 "This server is not using a recognized protocol."
	(UINT32)HXR_ADVANCED_SERVER,		IDS_ERR_ADVANCED_SERVER_ERROR,		// 35 "You need a newer client to access this server. Please upgrade."
	(UINT32)HXR_OLD_SERVER,     		IDS_ERR_OLD_SERVER_ERROR,			// 36 "Connection closed. The host's version of the RealAudio Server is too old for this client."
	(UINT32)HXR_SERVER_ALERT,         	IDS_ERR_SERVER_ALERT,				// 45 "Server alert"
	
	// decoder errors
	(UINT32)HXR_DEC_NOT_FOUND,        	IDS_ERR_DEC_NOT_FOUND,				// 38 "File compression not supported. Cannot locate the requested RealAudio decoder."
	(UINT32)HXR_DEC_INVALID,	        IDS_ERR_DEC_INVALID,				// 39 "Invalid decoder."
	(UINT32)HXR_DEC_TYPE_MISMATCH,    	IDS_ERR_DEC_TYPE_MISMATCH,			// 40 "Decoder type mismatch. Cannot load the requested decoder."
	(UINT32)HXR_DEC_INIT_FAILED,      	IDS_ERR_DEC_INIT_FAILED,			// 41 "Requested RealAudio Decoder cannot be found or cannot be used on this machine."
	(UINT32)HXR_DEC_NOT_INITED,	     	IDS_ERR_DEC_NOT_INITED,				// 42 "RealAudio Decoder was not initialized before attempting to use it."
	(UINT32)HXR_DEC_DECOMPRESS, 		IDS_ERR_DEC_DECOMPRESS_ERROR,		// 43 "An error occurred during decoding."	
	(UINT32)HXR_NO_CODECS,			IDS_ERR_NO_CODECS,					// 87 "No RealAudio Codecs have been installed on your system."

	// proxy errors
	(UINT32)HXR_PROXY,	         	IDS_ERR_PROXY_ERROR,				// 46 "Proxy status error"
	(UINT32)HXR_PROXY_RESPONSE, 		IDS_ERR_PROXY_RESPONSE_ERROR,		// 47 "Proxy invalid response error"
	(UINT32)HXR_ADVANCED_PROXY, 		IDS_ERR_ADVANCED_PROXY_ERROR,		// 48 "You need a newer client to access this proxy. Please upgrade."
	(UINT32)HXR_OLD_PROXY,	     		IDS_ERR_OLD_PROXY_ERROR,			// 49 "Connection closed. The proxy is too old for this client."

	// audio errors					
	(UINT32)HXR_AUDIO_DRIVER,   		IDS_ERR_AUDIO_DRIVER_ERROR,			// 50 "Cannot open audio device."

	// parsing errors
	(UINT32)HXR_INVALID_PROTOCOL,  		IDS_ERR_INVALID_PROTOCOL_ERROR,		// 51 "Invalid protocol specified in URL."
	(UINT32)HXR_INVALID_URL_OPTION,		IDS_ERR_INVALID_URL_OPTION_ERROR,	// 52 "Invalid option specified in URL."
	(UINT32)HXR_INVALID_URL_HOST,        	IDS_ERR_INVALID_URL_HOST,			// 53 "Invalid host string in requested URL."
	(UINT32)HXR_INVALID_URL_PATH,        	IDS_ERR_INVALID_URL_PATH,			// 54 "Invalid resource path string in requested URL."

	(UINT32)HXR_GENERAL_NONET,        	IDS_ERR_GENERAL_NONET,				// 55 "Error locating Winsock Services."

	// More errors
   	(UINT32)HXR_PERFECTPLAY_NOT_SUPPORTED,	IDS_ERR_PERFECTPLAY_NOT_SUPPORTED,	// 75, Requested server does not support PerfectPlay.
   	(UINT32)HXR_NO_LIVE_PERFECTPLAY,	IDS_ERR_NO_LIVE_PERFECTPLAY,		// 76, PerfectPlay not supported for live streams.
	(UINT32)HXR_PERFECTPLAY_NOT_ALLOWED,	IDS_ERR_PERFECTPLAY_NOT_ALLOWED,	// 78, PerfectPlay not allowed on this clip.
	(UINT32)HXR_MULTICAST_JOIN,		IDS_ERR_MULTICAST_JOIN_ERROR,		// 84, An error occured attempting to join multicast session.
	(UINT32)HXR_GENERAL_MULTICAST,		IDS_ERR_GENERAL_MULTICAST_ERROR,	// 85, An error occured accessing a multicast session.
	(UINT32)HXR_MULTICAST_UDP,        	IDS_ERR_MULTICAST_UDP_ERROR,		// 86 "Player cannot receive UDP data packets from multicast session. 
																		// You may wish to try the TCP data option in the Network Preferences. 
																		// You may also want to configure your player to use a firewall proxy. 
																		// Please contact your system adminstrator for more information."
	(UINT32)HXR_SLOW_MACHINE,		IDS_ERR_SLOW_MACHINE,				// 88 "Your machine does not have enough CPU power to play this file in real time."
	(UINT32)HXR_INVALID_HTTP_PROXY_HOST,	IDS_ERR_INVALID_HTTP_PROXY_HOST,		// 90 Invalid hostname for HTTP proxy.
	(UINT32)HXR_INVALID_METAFILE,		IDS_ERR_INVALID_METAFILE, // 90 Invalid hostname for HTTP proxy.
	(UINT32)HXR_NO_FILEFORMAT,		IDS_ERR_MISSING_COMPONENTS,	    // Missing components
	(UINT32)HXR_NO_RENDERER,		IDS_ERR_MISSING_COMPONENTS,	    // Missing components
	(UINT32)HXR_MISSING_COMPONENTS,		IDS_ERR_MISSING_COMPONENTS,	    // Missing components
	(UINT32)HXR_BAD_TRANSPORT,		IDS_ERR_BAD_TRANSPORT,    // 94 Transport not recognized
	(UINT32)HXR_NOT_AUTHORIZED,		IDS_ERR_NOT_AUTHORIZED,    // 95 Access Denied
	(UINT32)HXR_RIGHTS_EXPIRED,		IDS_ERR_RIGHTS_EXPIRED,    // 1082 Digital Rights have expired.
	(UINT32)HXR_CHECK_RIGHTS,		IDS_ERR_CHECK_RIGHTS,	// Check for more Rights 
	(UINT32)HXR_RESTORE_SERVER_DENIED,	IDS_ERR_RESTORE_SERVER_DENIED,		// Restoration Denial from server.
	(UINT32)HXR_DEBUGGER_DETECTED,		IDS_ERR_DEBUGGER_DETECTED,		// Debugger detected.
	(UINT32)HXR_RESTORE_SERVER_CONNECT,	IDS_ERR_RESTORE_SERVER_CONNECT,	// Cannot connect to Restore server .
	(UINT32)HXR_RESTORE_SERVER_TIMEOUT,	IDS_ERR_RESTORE_SERVER_TIMEOUT,	// Restore server timeout.
	(UINT32)HXR_REVOKE_SERVER_CONNECT,	IDS_ERR_REVOKE_SERVER_CONNECT,		// Cannot connect to Revoke server.
	(UINT32)HXR_REVOKE_SERVER_TIMEOUT,	IDS_ERR_REVOKE_SERVER_TIMEOUT,		// Revoke server timeout.
	(UINT32)HXR_RESTORATION_COMPLETE,	IDS_ERR_RESTORATION_COMPLETE,		// restoration done.
	(UINT32)HXR_BACKUP_COMPLETE,		IDS_ERR_BACKUP_COMPLETE,		// backup done.
	(UINT32)HXR_TLC_NOT_CERTIFIED,		IDS_ERR_TLC_NOT_CERTIFIED,		// tlc not certified
	(UINT32)HXR_CORRUPTED_BACKUP_FILE,	IDS_ERR_CORRUPTED_BACKUP_FILE,		// corrupted backup file
	(UINT32)HXR_NOTENOUGH_BANDWIDTH,	IDS_ERR_NOTENOUGH_BANDWIDTH,
	(UINT32)HXR_UNSUPPORTED_VIDEO,	 	IDS_ERR_UNSUPPORTED_VIDEO,			// 105 "The file contains an unsupported video format"
	(UINT32)HXR_UNSUPPORTED_AUDIO,	 	IDS_ERR_UNSUPPORTED_AUDIO,		// 106 "The file contains an unsupported audio format"
	(UINT32)HXR_TRY_AUTOCONFIG,	 	IDS_ERR_TRY_AUTOCONFIG			// 107"AutoConfig needs to be performed"
	(UINT32)HXR_VIEW_SOURCE_NOCLIP,		IDS_ERR_VIEW_SOURCE_NOCLIP,		// 80 "No clip is avalible."
	(UINT32)HXR_VIEW_SOURCE_DISSABLED,	IDS_ERR_VIEW_SOURCE_DISSABLED		// 81 "The provider of this content has not granted permission to view the source."
};

ResCodeStr::ResCodeStr()
{
}

ResCodeStr::~ResCodeStr()
{
}

//	Loads a new string into the m_ResString object
HXBOOL ResCodeStr::LoadStr( UINT32 uResCode )
{
	return m_ResString.LoadStr( FindResId(uResCode) );
}

//	Find resource ID by string code
UINT32 ResCodeStr::FindResId( UINT32 uResCode )
{
	UINT index;
	for(index = 0; index < _MAX_STRINGS; index++)
	{
	    if(LookupTable[index][0] == uResCode)
	    {
		return LookupTable[index][1];
	    }
	}

	// Someone tried to load an unknown string. Trace back up
	// the call stack and determine the nature of the requested
	// string, and fix appropriately!
	HX_ASSERT(FALSE);
	/* By default return General Error */
	return LookupTable[0][1];
}


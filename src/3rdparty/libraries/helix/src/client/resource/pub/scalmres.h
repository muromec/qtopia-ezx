/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: scalmres.h,v 1.2 2004/07/19 21:13:09 hubbe Exp $
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

#ifndef _scalmres_H
#define _scalmres_H

#include "resid.h"

// pplyplin & ppffplin
#define IDS_ERR_SM_GENERAL		HX_SM_RES_INIT_ID +   0
#define IDS_ERR_SM_INITFAILED		HX_SM_RES_INIT_ID +   1
#define IDS_ERR_SM_SOCKFAILED		HX_SM_RES_INIT_ID +   2
#define IDS_ERR_SM_NO_CTXT		HX_SM_RES_INIT_ID +   3
#define IDS_ERR_SM_CTXT_QI_FAILED	HX_SM_RES_INIT_ID +   5
#define IDS_ERR_SM_CLASS_FACT_ERR	HX_SM_RES_INIT_ID +   6
#define IDS_ERR_SM_SRC_FINDER_ERR	HX_SM_RES_INIT_ID +   7
#define IDS_ERR_SM_REGISTRY_ERR		HX_SM_RES_INIT_ID +   8
#define IDS_ERR_SM_STRM_DESC_ERR	HX_SM_RES_INIT_ID +   9
#define IDS_ERR_SM_CONFIG_SETUP_ERR	HX_SM_RES_INIT_ID +   10

/**********
* pplyplin
*/

// general
#define IDS_ERR_SM_SUCCESSSTART		HX_SM_RES_INIT_ID + 100
#define IDS_ERR_SM_NOTLICENSED		HX_SM_RES_INIT_ID + 101

// config related
#define IDS_ERR_SM_NOSOURCELIST		HX_SM_RES_INIT_ID + 102
#define IDS_ERR_SM_NOMOUNTPOINT		HX_SM_RES_INIT_ID + 105
#define IDS_ERR_SM_INVALIDHOSTADDR	HX_SM_RES_INIT_ID + 106
#define IDS_ERR_SM_NOADDRRANGE		HX_SM_RES_INIT_ID + 107 
#define IDS_ERR_SM_NOPORTRANGE		HX_SM_RES_INIT_ID + 108  
#define IDS_ERR_SM_ADDRRANGEERROR	HX_SM_RES_INIT_ID + 109  
#define IDS_ERR_SM_PORTRANGEERROR	HX_SM_RES_INIT_ID + 110  
#define IDS_ERR_SM_INVALIDPORTRANGE	HX_SM_RES_INIT_ID + 111
#define IDS_ERR_SM_INVALIDADDRRANGE	HX_SM_RES_INIT_ID + 112
#define IDS_ERR_SM_NOMOREADDR		HX_SM_RES_INIT_ID + 113
#define IDS_ERR_SM_NOMOREPORT		HX_SM_RES_INIT_ID + 114
#define IDS_ERR_SM_NEEDTWOPORTS		HX_SM_RES_INIT_ID + 115
#define IDS_ERR_SM_NOTENABLED		HX_SM_RES_INIT_ID + 117

// sap
#define IDS_ERR_SM_INITSAPFAILED	HX_SM_RES_INIT_ID + 118
#define IDS_ERR_SM_TTLZERO		HX_SM_RES_INIT_ID + 119

// client stats
#define IDS_ERR_SM_NO_WEB_ADDRESS	HX_SM_RES_INIT_ID + 120
#define IDS_ERR_SM_NO_WEB_PORT		HX_SM_RES_INIT_ID + 121

#define IDS_ERR_SM_NOMULTICAST		HX_SM_RES_INIT_ID + 122

/**********
* ppffplin
*/
#define IDS_ERR_SM_ABNORMALTERMINATION  HX_SM_RES_INIT_ID + 200
#define IDS_ERR_SM_NOACTIVESENDERS	HX_SM_RES_INIT_ID + 201 
#define IDS_ERR_SM_NOSDPPLIN		HX_SM_RES_INIT_ID + 202 
#define IDS_ERR_SM_BADSDPFILE		HX_SM_RES_INIT_ID + 203 
#define IDS_ERR_SM_NOPLAYTYPE		HX_SM_RES_INIT_ID + 204
#define IDS_ERR_SM_NOTFOUNDPAYLOAD	HX_SM_RES_INIT_ID + 205     
#define IDS_ERR_SM_NOTFOUNDADDRESS	HX_SM_RES_INIT_ID + 206     
#define IDS_ERR_SM_NOTFOUNDPORT		HX_SM_RES_INIT_ID + 207
#define IDS_ERR_SM_UNEXPECTEDPAYLOAD	HX_SM_RES_INIT_ID + 208
#define IDS_ERR_SM_NOTENOUGHBANDWIDTH	HX_SM_RES_INIT_ID + 209


/***************************/
/**  Actual error string  **/
/***************************/

// pplyplin & ppffplin
#define ERRSTR_SM_GENERAL		"Scalable Multicast: General Error."
#define ERRSTR_SM_INITFAILED		"Scalable Multicast: Initialization failed."
#define ERRSTR_SM_SOCKFAILED		"Scalable Multicast: Could not create multicast socket: Address/Port %s."
#define ERRSTR_SM_NO_CTXT		"Scalable Multicast: Could not get Context -- Initialization failed."
#define ERRSTR_SM_CTXT_QI_FAILED	"Scalable Multicast: Could not query Context -- Initialization failed."
#define ERRSTR_SM_CLASS_FACT_ERR	"Scalable Multicast: Class Factory error -- Initialization failed."
#define ERRSTR_SM_SRC_FINDER_ERR	"Scalable Multicast: Could not initialize Source Finder."
#define ERRSTR_SM_REGISTRY_ERR		"Scalable Multicast: Error in Server Registry entry -- Initialization failed."
#define ERRSTR_SM_STRM_DESC_ERR		"Scalable Multicast: Could not get Stream Description -- Initialization failed."
#define ERRSTR_SM_CONFIG_SETUP_ERR	"Scalable Multicast: Error in reading in Config entries -- Initialization failed."

/**********
* pplyplin
*/

// general
#define ERRSTR_SM_SUCCESSSTART		"Scalable Multicast: Successfully started session: %s."
#define ERRSTR_SM_NOTLICENSED		"Scalable Multicast: This server is NOT licensed to deliver Scalable Multicast streams."

// config related
#define ERRSTR_SM_NOSOURCELIST		"Scalable Multicast: Could not find Source List in the configuration file."
#define ERRSTR_SM_NOMOUNTPOINT		"Scalable Multicast: Could not find MountPoint configuration variable."
#define ERRSTR_SM_INVALIDHOSTADDR	"Scalable Multicast: Invalid HostAddress.  Using 0 as default."
#define ERRSTR_SM_NOADDRRANGE		"Scalable Multicast: Could not find AddressRange configuration variable."
#define ERRSTR_SM_NOPORTRANGE		"Scalable Multicast: Could not find PortRange configuration variable."
#define ERRSTR_SM_ADDRRANGEERROR	"Scalable Multicast: Address range needs to be aaa.bbb.ccc.ddd-www.xxx.yyy.zzz."
#define ERRSTR_SM_PORTRANGEERROR	"Scalable Multicast: Port range needs to be aaaa-zzzz."
#define ERRSTR_SM_INVALIDADDRRANGE	"Scalable Multicast: Invalid address range."
#define ERRSTR_SM_INVALIDPORTRANGE	"Scalable Multicast: Invalid port range."
#define ERRSTR_SM_NOMOREADDR		"Scalable Multicast: Error in creating this session: %s. Please increase the address range configuration variable."
#define ERRSTR_SM_NOMOREPORT		"Scalable Multicast: Error in creating this session: %s. Please increase the port range configuration variable."
#define ERRSTR_SM_NEEDTWOPORTS		"Scalable Multicast: Need at least two ports in the PortRange configuration variable; RTP(even port) & RTCP(odd port)."
#define ERRSTR_SM_NOTENABLED		"Scalable Multicast: %s is not enabled."
#define ERRSTR_SM_NO_WEB_ADDRESS	"Scalable Multicast: Could not find WebServerAddress configuration variable."
#define ERRSTR_SM_NO_WEB_PORT		"Scalable Multicast: Could not find WebServerPort configuration variable."

// sap
#define ERRSTR_SM_INITSAPFAILED		"Scalable Multicast: Initialization of SAP failed. SAP disabled."
#define ERRSTR_SM_TTLZERO		"Scalable Multicast: All SAP packets will have TTL of 0."

/**********
* ppffplin
*/
#define ERRSTR_SM_ABNORMALTERMINATION	"Scalable Multicast: Stream number %s terminated abnormally."
#define ERRSTR_SM_NOACTIVESENDERS	"Scalable Multicast: No data has been received. This session may have ended or may not be reachable."
#define ERRSTR_SM_NOSDPPLIN		"Scalable Multicast: Could not find a session description plugin."
#define ERRSTR_SM_BADSDPFILE		"Scalable Multicast: Could not interpret the session description file."
#define ERRSTR_SM_NOPLAYTYPE		"Scalable Multicast: Cannot play this type of document."
#define ERRSTR_SM_NOTFOUNDPAYLOAD	"Scalable Multicast: Could not find the RTP payload information in the session description file."
#define ERRSTR_SM_NOTFOUNDADDRESS	"Scalable Multicast: Could not find the connection information in the session description file."
#define ERRSTR_SM_NOTFOUNDPORT		"Scalable Multicast: Could not find the port information in the session description file."
#define ERRSTR_SM_UNEXPECTEDPAYLOAD	"Scalable Multicast: Unexpected payload type specified in session description file."
#define ERRSTR_SM_NOTENOUGHBANDWIDTH	"Scalable Multicast: You cannot receive this content.  You do not have enough network bandwidth."
#define ERRSTR_SM_NOMULTICAST		"Scalable Multicast: Your player is not configured to play multicast content."
#endif


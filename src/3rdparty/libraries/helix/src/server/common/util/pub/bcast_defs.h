/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: bcast_defs.h,v 1.2 2003/10/24 22:29:23 jc Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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

#ifndef _BCAST_DEFS_H_
#define _BCAST_DEFS_H_

/*
 * Defaults
 */

#define BRCV_REGISTRY_STATS_ROOT	"server.BroadcastReceiver.Statistics"
#define BRCV_REGISTRY_STATISTICS	BRCV_REGISTRY_STATS_ROOT ".All"

#define BRCV_REGISTRY_STATS_CONN_TOTAL \
			BRCV_REGISTRY_STATS_ROOT ".BrcvConnectionsTotal"

#define BDST_REGISTRY_STATS_ROOT	"server.BroadcastDist.Statistics"
#define BDST_REGISTRY_STATISTICS	BDST_REGISTRY_STATS_ROOT ".All"

#define BDST_REGISTRY_STATS_CONN_TOTAL \
			BDST_REGISTRY_STATS_ROOT ".BdstConnectionsTotal"


/*
 * structs
 */

/*
 * BrcvStatistics: byte transfer rates, transmitter connections
 * See brcvplin for statistics gathering.
 */
struct BrcvStatistics
{
public:
    UINT16		m_usSize;
    UINT8		m_ubVersion;
    UINT8		m_ubEnabled;

    UINT32		m_uReceiverSessions;
    UINT32		m_uBytesRcvd;
    UINT32		m_uPacketsRcvd;

    UINT32		m_uOutOfOrder;
    UINT32		m_uLost;
    UINT32		m_uLate;
    UINT32		m_uResendsRequested;
    UINT32		m_uDuplicates;
    UINT32		m_uLostUpstream;
    UINT32              m_ulFECPacketsReceivedID;
    UINT32              m_ulFECPacketsUsedID;
};

/*
 * BdstStatistics: byte transfer rates, transmitter connections
 * See bdstplin for statistics gathering.
 */
struct BdstStatistics
{
public:
    UINT16              m_usSize;
    UINT8               m_ubVersion;
    UINT8               m_ubEnabled;

    UINT32              m_uDistSessions;
    UINT32		m_uPushSessions;
    UINT32		m_uPullSessions;
    UINT32		m_uRelaySessions;
    UINT32		m_uTCPSessions;
    UINT32		m_uUDPSessions;
    UINT32		m_uMCSessions;
    UINT32              m_uBytesSent;
    UINT32              m_uPacketsSent;

    UINT32		m_uResendsRequested;
    UINT32		m_uResendsHonored;
    UINT32		m_uLostUpstream;
    UINT32              m_uFECPacketsSent;
    UINT32		m_uBufferOverruns;
    UINT32		m_uTimeStampDeliveredPacketsOnTime;
    UINT32		m_uTimeStampDeliveredPackets;
};

#endif  // _BCAST_DEFS_H_

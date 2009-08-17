%{
/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: rttp2ss.pm,v 1.2 2003/01/24 01:00:29 damonlan Exp $ 
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

/*
 * This is generated code, do not modify. Look in rttp2ss.pm to make
 * modifications.
 */

#ifndef _RTTP2SS_H_
#define _RTTP2SS_H_

#define RTTPSS_SUBSERVER_INIT				RTTP(SS,0)
#define RTTPSS_SUBSERVER_ENABLE				RTTP(SS,1)
#define RTTPSS_SUBSERVER_DISABLE			RTTP(SS,2)
#define RTTPSS_SUBSERVER_STARTED			RTTP(SS,3)
#define RTTPSS_SUBSERVER_ENDED				RTTP(SS,4)
#define RTTPSS_UNUSED0					RTTP(SS,5)
#define RTTPSS_UNUSED1					RTTP(SS,6)
#define RTTPSS_MONITOR_CONNECTED			RTTP(SS,7)
#define RTTPSS_MONITOR_DISCONNECTED			RTTP(SS,8)
#define RTTPSS_MONITOR5_UPDATE				RTTP(SS,9)
#define RTTPSS_MONITOR_BROADCAST			RTTP(SS,10)
#define RTTPSS_CONFIG_BROADCAST				RTTP(SS,11)
#define RTTPSS_PASSWORD					RTTP(SS,12)
#define RTTPSS_CLUSTER_STRUCTURE_REQUEST		RTTP(SS,13)
#define RTTPSS_CLUSTER_STRUCTURE_REPLY			RTTP(SS,14)
#define RTTPSS_CLUSTER_STRUCTURE_UPDATE			RTTP(SS,15)
#define RTTPSS_CLUSTER_UNIT_REQUEST			RTTP(SS,16)
#define RTTPSS_CLUSTER_UNIT_REPLY			RTTP(SS,17)
#define RTTPSS_CLUSTER_UNIT_UPDATE			RTTP(SS,18)
#define RTTPSS_FORK_END_BROADCAST			RTTP(SS,19) // unused
#define RTTPSS_UNUSED2					RTTP(SS,20) // unused
#define RTTPSS_CLUSTER_FORK_UPDATE			RTTP(SS,21)
#define RTTPSS_CLUSTER_DONE				RTTP(SS,22)
#define RTTPSS_CLUSTER_COUNT_UPDATE			RTTP(SS,23)
#define RTTPSS_CLUSTER_FAILED				RTTP(SS,24)
#define RTTPSS_STATISTICS_UPDATE			RTTP(SS,25)
#define RTTPSS_PARENT_ID_REQUEST			RTTP(SS,26)
#define RTTPSS_PARENT_ID_REPLY				RTTP(SS,27)
#define RTTPSS_STATISTICS_BROADCAST			RTTP(SS,28)
#define RTTPSS_SPLITTER_CONNECTED			RTTP(SS,29)
#define RTTPSS_SPLITTER_DISCONNECTED			RTTP(SS,30)
#define RTTPSS_SPLITTER_COUNTS				RTTP(SS,31)
#define RTTPSS_UNUSED3					RTTP(SS,32)
#define RTTPSS_REGISTER_STREAM				RTTP(SS,33)
#define RTTPSS_RELEASE_STREAM				RTTP(SS,34)
#define RTTPSS_PASS_FD					RTTP(SS,35)
#define RTTPSS_DESTROY_CLIENT				RTTP(SS,36)
#define RTTPSS_WITHDRAW_BANDWIDTH			RTTP(SS,37)

const int RTTPSS_REGISTER_STREAM_STATIC_LEN		= 12;
const int RTTPSS_RELEASE_STREAM_STATIC_LEN		= 6;
const int RTTPSS_WITHDRAW_BANDWIDTH_STATIC_LEN		= 12;

%}

RTTPSS_splitter_connected : rttp2_header
{
    int32	    global_id;
    int32	    parent_id;
    int16	    port;
    string	    hostname;
}

RTTPSS_splitter_disconnected : rttp2_header
{
    int32	    global_id;
}

RTTPSS_splitter_counts : rttp2_header
{
    int32	    global_id;
    int32	    player_count;
}

RTTPSS_register_stream : rttp2_header
{
    Int16	    host_port;
    String	    host_name;
    String	    url;
    Int32	    id;
    buffer[]	    header;
}

RTTPSS_release_stream : rttp2_header
{
    String	    url;
    buffer[]	    header;
}

RTTPSS_withdraw_bandwidth : rttp2_header
{
    Int32	    bandwidth;
    Int32	    conn_id;
}

%{
#endif /* _RTTP2SS_H_ */
%}

/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: dl_decls.h,v 1.3 2003/01/24 23:40:25 kamlesh Exp $ 
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
 *  declarations for Distributed Licensing
 */

#ifndef _DL_DECLS_H_
#define _DL_DECLS_H_


/*************** messages **************

Legend:
    id = ip of either cluster or master server depending on 
	 which one sent the request
    s_id = reg id of the property on the sending server
    r_id = reg id of the property on the receiving server
    c_id = ip of the subscriber (cluster) server
    m_id = ip of the publisher (master) server
    v_id = id of the variable that is to be set

(0) Hello ------ | DLCMP 1.0 | 0 | c_id | ----------- Msg len = 15
    |9________|2_|4___|
     prot str  s  l

(1) Hello ------ | DLMMP 1.0 | 1 | c_id | ----------- Msg len = 15
    |9________|2_|4___|
     prot str  s  l

(2) Set -------- | 2 | c_id | len | DLT_Buffer | buffer | s_id | value | seq# |
    23 < Msg len < 512
    |2_|4___|2_|2_|n______|4___|4___|4___|
     s  l    s  s  n       l    l    l

(3) Set -------- | 2 | c_id | len | DLT_Integer | r_id | value | seq# |
    Msg len = 22
    |2_|4___|2_|2_|4___|4___|4___|
     s  l    s  s  l    l    l

(4) Set (Ack) -------- | 2 | r_id | s_id | ask value | sent delta | seq# |
    Msg len = 22
    |2_|4___|4___|4___|4___|4__|
     s  l    l    l    l    l

(5) Ping -------- | 3 | c_id | seq# |
    Msg len = 22
    |2_|4___|4___|
     s  l    l

(6) Pong -------- | 4 | c_id | seq# |
    Msg len = 22
    |2_|4___|4___|
     s  l    l

(7) Teardown --- | 5 | m_id | ----------------------- Msg len = 6
    |2_|4___|
     s  l

***************************************/

typedef enum
{
    DLS_Start,
    DLS_Hello,
    DLS_Stop,
    DLS_Ready
} DistLicState;

typedef enum
{
    DLO_Cluster_Hello,
    DLO_Master_Hello,
    DLO_Set,
    DLO_Ping,
    DLO_Pong,
    DLO_Teardown,
    DLO_Stop,
    DLO_Resume
} DistLicOpcode;

// variable type
#define DLT_Buffer	1
#define DLT_Integer	2

// variable ids (v_id below)
#define DLV_Delta 	0
#define DLV_StreamCount	1

// cluster - master protocol identification string
#define DLCM_PROT_STR 		"DLCMP 1.0"
#define DLCMP_STR_LEN 		9

// master - master protocol identification string
#define DLMM_PROT_STR 		"DLMMP 1.0"
#define DLMMP_STR_LEN 		9

#define DLM_Opcode_Len 		2
#define DLM_Hello_Len 		DLCMP_STR_LEN+6
#define DLM_Hello_Ack_Len 	6
#define DLM_Old_Set_Len 	12
#define DLM_Set_Ack_Len 	22
#define DLM_Set_Int_Len 	22
#define DLM_Set_Max_Len 	512
#define DLM_Ping_Len 		10
#define DLM_Stop_Len 		6
#define DLM_Resume_Len 		6
#define DLM_Teardown_Len	6
#define DLM_Opcode_Id_Len 	6 /* 2 + 4 bytes */
#define DLM_Opcode_Id_Msg_Len 	8 /* 2 + 4 + 2 bytes */

#define MAX_DLP_MSG 512 /* arbitrarily large value in case we pass buffers */

// message offsets
#define DLF_Opcode		0
#define DLF_Hello_IP		DLCMP_STR_LEN+2
#define DLF_Set_IP		DLF_Opcode+2
#define DLF_RemLen		DLF_Set_IP+4
#define DLF_Int_Set_VarType	DLF_RemLen+4
#define DLF_Int_Set_RegId	DLF_Set_Int_VarType+4
#define DLF_Int_Set_Value	DLF_Set_Int_RegId+4
#define DLF_Int_Set_SeqNum	DLF_Set_Int_Value+4

// registry strings
#define DL_Str              "DistributedLicensing"
#define Mast_Str            "Publishers"
#define StrmCnt_Str         "StreamCount"
#define Clst_Str            "Subscribers"

// stream type properties names
#define ST_Str		"StreamType"
#define ST_Name_Str	"StreamType.%s"
#define ST_MaxLic_Str	"StreamType.%s.MaxLicenses"
#define ST_Conns_Str	"StreamType.%s.Connections"
#define ST_Delta_Str	"StreamType.%s.Delta"
#define ST_Trigger_Str	"StreamType.%s.Trigger"

// trace mask values
#define DL_PROTOCOL 	0x00000001
#define DL_CONNECTION 	0x00000002
#define DL_REGISTRY 	0x00000004

#define DL_SUB_TO_PUB 	0x00000100 /* subscriber --> publisher */
#define DL_PUB_TO_SUB 	0x00000200 /* publisher ---> subscriber */
#define DL_PUB_TO_PUB 	0x00000400 /* publisher ---> publisher */

#define DL_MSG_ALL	0x00010000
#define DL_MSG_SENT	0x00020000
#define DL_MSG_RCVD	0x00040000

#define DL_SUBSCRIBER   0x01000000
#define DL_PUBLISHER    0x02000000
#define DL_PUB_SRVR     0x04000000
#define DL_PUB_CLNT     0x08000000

#endif /* _DL_DECLS_H_ */

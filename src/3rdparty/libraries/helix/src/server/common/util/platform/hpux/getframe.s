	.CODE
; ***** BEGIN LICENSE BLOCK *****  
; Source last modified: $Id: getframe.s,v 1.2 2003/01/23 23:42:51 damonlan Exp $ 
;   
; Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
;       
; The contents of this file, and the files included with this file, 
; are subject to the current version of the RealNetworks Public 
; Source License (the "RPSL") available at 
; http://www.helixcommunity.org/content/rpsl unless you have licensed 
; the file under the current version of the RealNetworks Community 
; Source License (the "RCSL") available at 
; http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
; will apply. You may also obtain the license terms directly from 
; RealNetworks.  You may not use this file except in compliance with 
; the RPSL or, if you have a valid RCSL with RealNetworks applicable 
; to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
; the rights, obligations and limitations governing use of the 
; contents of the file. 
;   
; This file is part of the Helix DNA Technology. RealNetworks is the 
; developer of the Original Code and owns the copyrights in the 
; portions it created. 
;   
; This file, and the files included with this file, is distributed 
; and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY 
; KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS 
; ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES 
; OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET 
; ENJOYMENT OR NON-INFRINGEMENT. 
;  
; Technology Compatibility Kit Test Suite(s) Location:  
;    http://www.helixcommunity.org/content/tck  
;  
; Contributor(s):  
;   
; ***** END LICENSE BLOCK *****  
;------------------------------------------------------------------------
; :
; U_get_frame_info :
; :
; U_get_frame_info loads the value of the caller's SP, PCspace, PCOffset :
; and DP into a record, a pointer to which has been passed into 
; this routine in arg0. The format of this record is that required by the 
; unwind routine :
; "U_get_previous_frame". :
; :
; offset contents :
; :
; 0 cur_frsize framesize of called routine :
; 4 curSP SP of called routine :
; 8 curRLS PC_space of calling routine :
; 12 curRLO PC_offset of calling routine :
; 16 curDP DP of called routine :
; 20 topRP RP (reg. 2) of called routine :
; 24 topMRP MRP (reg. 31) of called routine :
; 28 cuffSR0 :
; 32 cuffSR4 :
; 36 curR3 :
; 40 cur_r19 (new offset) :
; 44 curR4 :
; 48 reserved :
; :
; INPUT PARAMETERS: :
; arg0 : pointer to a 11-word structure with the above 
; layout :
; :
; OUTPUT PARAMETERS: :
; the fields curSP, curRLS, curRLO,curDP :
;------------------------------------------------------------------------
;
	.EXPORT U_get_frame_info,CODE,PRIV_LEV=3
U_get_frame_info
	.PROC
	.CALLINFO
	.ENTRY
	stw sp,4(arg0) ; store caller's SP
	mfsp sr4,r20
	stw r20,8(arg0) ; store caller's PC space
	stw rp,12(arg0) ; store caller's PC offset
	stw dp,16(arg0) ; store caller's DP
	stw r3,36(arg0) ; store caller's R3
	stw r0,0(arg0) ; initialize rest of fields
	stw r0,20(arg0) ; -"-
	stw r0,24(arg0) ; -"-
	stw     %r0,28(arg0) ; -"-
	stw     %r0,32(arg0) ; -"-
	stw     %r19,40(arg0) ; store r19
        STW     %r4,44(arg0)  ; store caller's R4
	bv r0(rp) ; return
	.EXIT
        STW     %r0,48(arg0)  ; reserved
	.PROCEND
	.END

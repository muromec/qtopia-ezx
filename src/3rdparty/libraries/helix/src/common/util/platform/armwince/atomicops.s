; ***** BEGIN LICENSE BLOCK *****
; Source last modified: $Id: atomicops.s,v 1.1 2005/09/05 13:04:15 singerb Exp $
; 
; * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
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
; Alternatively, the contents of this file may be used under the
; terms of the GNU General Public License Version 2 or later (the
; "GPL") in which case the provisions of the GPL are applicable
; instead of those above. If you wish to allow use of your version of
; this file only under the terms of the GPL, and not to allow others
; to use your version of this file under the terms of either the RPSL
; or RCSL, indicate your decision by deleting the provisions above
; and replace them with the notice and other provisions required by
; the GPL. If you do not delete the provisions above, a recipient may
; use your version of this file under the terms of any one of the
; RPSL, the RCSL or the GPL.
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

;/* assembly.s
; * 
; * the assembly routines are written in this separate .s file because EVC++ won't inline...
; *
; * Notes on APCS (ARM procedure call standard):
; *  - first four arguments are in r0-r3
; *  - additional argumets are pushed onto stack in reverse order
; *  - return value goes in r0
; *  - routines can overwrite r0-r3, r12 without saving
; *  - other registers must be preserved with stm/ldm commands
; * 
; * Equivalent register names:
; *
; * r0  r1  r2  r3  r4  r5  r6  r7  r8  r9  r10 r11 r12 r13 r14 r15
; * ---------------------------------------------------------------
; * a1  a2  a3  a4  v1  v2  v3  v4  v5  v6  sl  fp  ip  sp  lr  pc
; */

	AREA	|.text|, CODE, READONLY

; INT32 _HXAtomicAddRetINT32(INT32* pNum, INT32 nNum);

	EXPORT	_HXAtomicAddRetINT32
	
_HXAtomicAddRetINT32
	mov	r3, #0x80000000
ARI_Retry
	swp	r3, r3, [r0]
	cmp	r3, #0x80000000
	beq	ARI_Retry
	add	r3, r3, r1
	cmp	r3, #0x80000000
	addeq	r3, r3, #1
	str	r3, [r0]
	mov	r0, r3
	mov		pc, lr

; INT32 _HXAtomicSubRetINT32(INT32* pNum, INT32 nNum);

	EXPORT	_HXAtomicSubRetINT32
	
_HXAtomicSubRetINT32
	mov	r3, #0x80000000
SRI_Retry
	swp	r3, r3, [r0]
	cmp	r3, #0x80000000
	beq	SRI_Retry
	sub	r3, r3, r1
	cmp	r3, #0x80000000
	subeq	r3, r3, #1
	str	r3, [r0]
	mov	r0, r3
	mov		pc, lr

; UINT32 HXAtomicAddRetUINT32(UINT32* pNum, UINT32 nNum);

	EXPORT	_HXAtomicAddRetUINT32
	
_HXAtomicAddRetUINT32
	mov	r3, #0x80000000
ARU_Retry
	swp	r3, r3, [r0]
	cmp	r3, #0x80000000
	beq	ARU_Retry
	add	r3, r3, r1
	cmp	r3, #0x80000000
	addeq	r3, r3, #1
	str	r3, [r0]
	mov	r0, r3
	mov		pc, lr

; UINT32 HXAtomicSubRetUINT32(UINT32* pNum, UINT32 nNum);

	EXPORT	_HXAtomicSubRetUINT32
	
_HXAtomicSubRetUINT32
	mov	r3, #0x80000000
SRU_Retry
	swp	r3, r3, [r0]
	cmp	r3, #0x80000000
	beq	SRU_Retry
	sub	r3, r3, r1
	cmp	r3, #0x80000000
	subeq	r3, r3, #1
	str	r3, [r0]
	mov	r0, r3
	mov		pc, lr

	END


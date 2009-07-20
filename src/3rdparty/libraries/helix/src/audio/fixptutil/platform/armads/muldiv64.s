; ***** BEGIN LICENSE BLOCK *****
; 
; * Portions Copyright (c) 2003-2004 RealNetworks, Inc. All Rights Reserved.
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
; terms of the GNU General Public License Version 2 (the
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


;
; MulDiv64() for ARM platforms.
; Developed by Ken Cooke (kenc@real.com)
; Sep 2003

		AREA text, CODE, READONLY

; int MulDiv64(int num1, int num2, int den)
; NOTE: num1, num2, den are assumed positive.

	EXPORT	MulDiv64
MulDiv64

nlo		RN 0
i		RN 1
den		RN 2
nhi		RN 3

		UMULL	nlo, nhi, r1, r0		; nhi|nlo = num1 * num2

; Unsigned 64/32-bit restoring division (see Hacker's Delight p.148)
; NOTE: requires den < 2^31
; On overflow (num < (den<<32)), result is undefined

		MOV		i, #4
		RSB		den, den, #0			; den = -den
		ADDS	nlo, nlo, nlo			; carry = nlo<<=1

div_loop	; 4x8 iterations

		ADCS	nhi, den, nhi, LSL #1	; rem = (nhi<<1 | carry) - den
		RSBCC	nhi, den, nhi			; restore if borrow occurred
		ADCS	nlo, nlo, nlo			; insert carry, carry = nlo<<=1

		ADCS	nhi, den, nhi, LSL #1
		RSBCC	nhi, den, nhi
		ADCS	nlo, nlo, nlo

		ADCS	nhi, den, nhi, LSL #1
		RSBCC	nhi, den, nhi
		ADCS	nlo, nlo, nlo

		ADCS	nhi, den, nhi, LSL #1
		RSBCC	nhi, den, nhi
		ADCS	nlo, nlo, nlo

		ADCS	nhi, den, nhi, LSL #1
		RSBCC	nhi, den, nhi
		ADCS	nlo, nlo, nlo

		ADCS	nhi, den, nhi, LSL #1
		RSBCC	nhi, den, nhi
		ADCS	nlo, nlo, nlo

		ADCS	nhi, den, nhi, LSL #1
		RSBCC	nhi, den, nhi
		ADCS	nlo, nlo, nlo

		ADCS	nhi, den, nhi, LSL #1
		RSBCC	nhi, den, nhi
		ADCS	nlo, nlo, nlo

		SUB		i, i, #1
		TST		i, i		; avoid disturbing carry
		BNE		div_loop

; finished: quotient in nlo, remainder in nhi

		MOV		pc, lr

		END

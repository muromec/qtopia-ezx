; ***** BEGIN LICENSE BLOCK *****
; Source last modified: $Id: spinlock.s,v 1.3 2004/07/09 18:22:59 hubbe Exp $
; 
; // Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.
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


//////////////////////////////////////////////////////////////////////
//
// spinlock.s
//      Implements a test-and-set operator, for use with a spinlock.
//      implemented in IA64 (Itanium) assembly.
//
// Implementation Notes:
//      A work-in-progress....
//
//////////////////////////////////////////////////////////////////////

  
//////////////////////////////////////////////////////////////////////
// Misc. Assembler Directives
//////////////////////////////////////////////////////////////////////

        .radix  C       // C-style numeric constants
        .section .text = "ax", "progbits" // read-only object code


//////////////////////////////////////////////////////////////////////
//
// _HXMutexSetBit
//      atomic test-and-set operator
//
// Interface:
//   extern "C" int _HXMutexSetBit(HX_MUTEX pLock);
//
// Inputs:
//   Paramaters:
//     in0 :  HX_MUTEX pLock  - pointer to mutex (integer) to modify
//
// Outputs:
//   Atomically modifies memory at *pLock:
//     *pLock = 1
//
//   Return value:
//     r8 : UINT32 - previous value of *pLock
//                   If zero, the lock attempt was successful
//
//////////////////////////////////////////////////////////////////////
	.proc   _HXMutexSetBit
	.global _HXMutexSetBit
_HXMutexSetBit:
        alloc           loc0 = ar.pfs, 1, 1, 0, 0 //one input, one local
	mov		r8 = 1;;                //x = 1
	xchg4		r8 = [in0], r8;;        //swap *pLock and x
	br.ret.dptk.few	rp;;                    //return previous *pLock value
        .endp   _HXMutexSetBit



//////////////////////////////////////////////////////////////////////
//
// _HXMutexClearBit
//      atomic bit clear operator
//
// Interface:
//   extern "C" void _HXMutexClearBit(HX_MUTEX pLock);
//
// Inputs:
//   Paramaters:
//     in0 :  HX_MUTEX pLock  - pointer to mutex (integer) to modify
//
// Outputs:
//   Atomically modifies memory at *pLock:
//     *pLock = 0
//
//   Return value:
//     none
//
//////////////////////////////////////////////////////////////////////
//
// Implementation notes:
// We cannot safely just set the lock to zero with an assignment, the
// use of the st4.rel release semantics is necessary to enforce correct
// memory ordering.
//
//////////////////////////////////////////////////////////////////////
        .proc   _HXMutexClearBit
        .global _HXMutexClearBit
_HXMutexClearBit:
        alloc           loc0 = ar.pfs, 1, 1, 0, 0 //one input, one local
        st4.rel         [in0] = r0;;            //atomically set to zero
	br.ret.dptk.few	rp;;                    //return
        .endp   _HXMutexClearBit



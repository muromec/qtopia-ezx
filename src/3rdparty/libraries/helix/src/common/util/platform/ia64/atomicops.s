; ***** BEGIN LICENSE BLOCK *****
; Source last modified: $Id: atomicops.s,v 1.4 2007/07/06 20:39:19 jfinnecy Exp $
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


//////////////////////////////////////////////////////////////////////
//
// atomicops.s
//      Several atomic operators, implemented in IA64 (Itanium) assembly
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
// _HXAtomicIncRetUINT32
//      Atomically increment by 1 and return new value
//
// Interface:
//   extern "C" UINT32 _HXAtomicIncRetUINT32(UINT32* pNum);
//
// Inputs:
//   Paramaters:
//     in0 :  UINT32* pNum - pointer to integer to modify
//
// Outputs:
//   Atomically modifies memory at *pNum:
//     *pNum = *pNum + 1
//
//   Return value:
//     r8 : UINT32 - new value of *pNum
//
//////////////////////////////////////////////////////////////////////
        .proc   _HXAtomicIncRetUINT32
        .global _HXAtomicIncRetUINT32
_HXAtomicIncRetUINT32:
        alloc           loc0 = ar.pfs, 1, 1, 0, 0 //one input, one local
        fetchadd4.acq   r8 = [in0], 1;;         //increment *plock
        add             r8 = 1, r8;;            //update local copy
	br.ret.dptk.few	rp;;                    //return new value
        .endp   _HXAtomicIncRetUINT32


//////////////////////////////////////////////////////////////////////
//
// _HXAtomicDecRetUINT32
//      Atomically decrement by 1 and return new value
//
// Interface:
//   extern "C" UINT32 _HXAtomicDecRetUINT32(UINT32* pNum);
//
// Inputs:
//   Paramaters:
//     in0 :  UINT32* pNum - pointer to integer to modify
//
// Outputs:
//   Atomically modifies memory at *pNum:
//     *pNum = *pNum - 1
//
//   Return value:
//     r8 : UINT32 - new value of *pNum
//
//////////////////////////////////////////////////////////////////////
        .proc   _HXAtomicDecRetUINT32
        .global _HXAtomicDecRetUINT32
_HXAtomicDecRetUINT32:
        alloc           loc0 = ar.pfs, 1, 1, 0, 0 //one input, one local
        fetchadd4.acq   r8 = [in0], -1;;        //decrement *pLock
        add             r8 = -1, r8;;           //update local copy
	br.ret.dptk.few	rp;;                    //return new value
        .endp   _HXAtomicDecRetUINT32


//////////////////////////////////////////////////////////////////////
//
// _HXAtomicAddRetUINT32
//      Atomically add n and return new value
//
// Interface:
//   extern "C" UINT32 _HXAtomicAddRetUINT32(UINT32* pNum, UINT32 ulNum);
//
// Inputs:
//   Paramaters:
//     in0 :  UINT32* pNum  - pointer to integer to modify
//     in1 :  UINT32  ulNum - amount to increment by
//
// Outputs:
//   Atomically modifies memory at *pNum:
//     *pNum = *pNum + ulNum
//
//   Return value:
//     r8 : UINT32 - new value of *pNum
//
//////////////////////////////////////////////////////////////////////
        .proc   _HXAtomicAddRetUINT32
        .global _HXAtomicAddRetUINT32
_HXAtomicAddRetUINT32:
        alloc           loc4 = ar.pfs, 2, 5, 0, 0 // two inputs, five locals
        mov             loc3 = ar.ccv;;         //save the CCV register
atomic_back1:                                   //
        ld4             loc0 = [in0];;          //loc0 = current value of *pNum
        mov             ar.ccv = loc0;;         //copy it to the CCV register
        add             loc1 = loc0, in1;;      //compute new value
        cmpxchg4.acq    r8 = [in0], loc1;;      //write new val if *pLock == CCV
        cmp4.eq         p1, p2 = loc0, r8;;     //check success of cmpxchg4
   (p2) br.cond.dpnt.few atomic_back1;;         //if p2=1 (it failed) try again
        add             r8 = r8, in1;;          //update local copy of *pNum
        mov             ar.ccv = loc3;;         //restore the CCV register
	br.ret.dptk.few	rp;;                    //return new value of *pNum
        .endp   _HXAtomicAddRetUINT32


//////////////////////////////////////////////////////////////////////
//
// _HXAtomicSubRetUINT32
//      Atomically subtract n and return new value
//
// Interface:
//   extern "C" UINT32 _HXAtomicSubRetUINT32(UINT32* pNum, UINT32 ulNum);
//
// Inputs:
//   Paramaters:
//     in0 :  UINT32* pNum  - pointer to integer to modify
//     in1 :  UINT32  ulNum - amount to increment by
//
// Outputs:
//   Atomically modifies memory at *pNum:
//     *pNum = *pNum + ulNum
//
//   Return value:
//     r8 : UINT32 - new value of *pNum
//
//////////////////////////////////////////////////////////////////////
        .proc   _HXAtomicSubRetUINT32
        .global _HXAtomicSubRetUINT32
_HXAtomicSubRetUINT32:
        alloc           loc4 = ar.pfs, 2, 5, 0, 0 // two inputs, five locals
        mov             loc3 = ar.ccv;;         //save the CCV register
atomic_back2:                                   //
        ld4             loc0 = [in0];;          //loc0 = current value of *pNum
        mov             ar.ccv = loc0;;         //copy it to the CCV register
        sub             loc1 = loc0, in1;;      //compute new value
        cmpxchg4.acq    r8 = [in0], loc1;;      //write new val if *pLock == CCV
        cmp4.eq         p1, p2 = loc0, r8;;     //check success of cmpxchg4
   (p2) br.cond.dpnt.few atomic_back2;;         //if p2=1 (it failed) try again
        sub             r8 = r8, in1;;          //update local copy of *pNum
        mov             ar.ccv = loc3;;         //restore the CCV register
	br.ret.dptk.few	rp;;                    //return new value of *pNum
        .endp   _HXAtomicSubRetUINT32



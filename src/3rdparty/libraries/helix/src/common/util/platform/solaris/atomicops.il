! ***** BEGIN LICENSE BLOCK *****
! Source last modified: $Id: atomicops.il,v 1.3 2004/07/09 18:23:09 hubbe Exp $
! 
!  * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.
! 
! The contents of this file, and the files included with this file,
! are subject to the current version of the RealNetworks Public
! Source License (the "RPSL") available at
! http://www.helixcommunity.org/content/rpsl unless you have licensed
! the file under the current version of the RealNetworks Community
! Source License (the "RCSL") available at
! http://www.helixcommunity.org/content/rcsl, in which case the RCSL
! will apply. You may also obtain the license terms directly from
! RealNetworks.  You may not use this file except in compliance with
! the RPSL or, if you have a valid RCSL with RealNetworks applicable
! to this file, the RCSL.  Please see the applicable RPSL or RCSL for
! the rights, obligations and limitations governing use of the
! contents of the file.
! 
! Alternatively, the contents of this file may be used under the
! terms of the GNU General Public License Version 2 or later (the
! "GPL") in which case the provisions of the GPL are applicable
! instead of those above. If you wish to allow use of your version of
! this file only under the terms of the GPL, and not to allow others
! to use your version of this file under the terms of either the RPSL
! or RCSL, indicate your decision by deleting the provisions above
! and replace them with the notice and other provisions required by
! the GPL. If you do not delete the provisions above, a recipient may
! use your version of this file under the terms of any one of the
! RPSL, the RCSL or the GPL.
! 
! This file is part of the Helix DNA Technology. RealNetworks is the
! developer of the Original Code and owns the copyrights in the
! portions it created.
! 
! This file, and the files included with this file, is distributed
! and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
! KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
! ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
! OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
! ENJOYMENT OR NON-INFRINGEMENT.
! 
! Technology Compatibility Kit Test Suite(s) Location:
!    http://www.helixcommunity.org/content/tck
! 
! Contributor(s):
! 
! ***** END LICENSE BLOCK *****
/*
 *  atomicops.il
 *
 *  defines several atomic operations for Sun/SPARC
 *
 ***********************************************************************
 *
 *  This is the Solaris inline-assembly version for use with Sun's
 *  native compiler on SPARC processors.  See atomicbase.h for more
 *  info and for the gcc implementaiton.
 *
 */


/***********************************************************************
 *
 *  _HXAtomicAddRetUINT32 -- Atomically increment by n and return new value
 *
 *  Interface:
 *    extern "C" UINT32 _HXAtomicAddRetUINT32(UINT32* pNum,     %o0
 *                                            UINT32 ulNum);    %o1
 *
 *  Inputs:
 *    Paramaters:
 *      %o0 :  UINT32* pNum - pointer to integer to modify
 *      %o1 :  UINT32 ulNum - amount to increment *pNum by
 *
 *  Outputs:
 *    Modifies memory at *pNum:
 *      *pNum = *pNum + ulNum
 *
 *    Return value:
 *      %o0 :  UINT32 - new value of *pNum
 *
 * This is implemented using the SPARC leaf procedure optimization.
 * This is implemented using CAS which is not available on older processors.
 */ 
.inline _HXAtomicAddRetUINT32,8
    ld      [%o0], %o2          /* Set %o2 to *pNum */
1:  add     %o2, %o1, %o3       /* Set %o3 to *pNum + ulNum */
    mov     %o3, %o4            /* Save new value for later */
    cas     [%o0], %o2, %o3     /* Set *pNum if *pNum is unchanged */
    cmp     %o2, %o3            /* Did we succeed in saving the value?*/
    bne,a   1b                  /* Retry if we didn't. */
    ld      [%o0], %o2          /* <<only executes if we branch>> */
    mov     %o4, %o0            /* Save return value */
.end


/***********************************************************************
 *
 *  _HXAtomicSubRetUINT32 -- Atomically decrement by n and return new value
 *
 *  Interface:
 *    extern "C" UINT32 _HXAtomicSubRetUINT32(UINT32* pNum,      %o0
 *                                            UINT32 ulNum);     %o1
 *
 *  Inputs:
 *    Paramaters:
 *      %o0 :  UINT32* pNum - pointer to integer to modify
 *      %o1 :  UINT32 ulNum - amount to decrement *pNum by
 *
 *  Outputs:
 *    Modifies memory at *pNum:
 *      *pNum = *pNum - ulNum
 *
 *    Return value:
 *      %o0 :  UINT32 - new value of *pNum
 *
 *  This is implemented using the SPARC leaf procedure optimization.
 *  This is implemented using CAS which is not available on older processors.
 */ 
.inline _HXAtomicSubRetUINT32,8
    ld      [%o0], %o2          /* Set %o2 to *pNum */
1:  sub     %o2, %o1, %o3       /* Set %o3 to *pNum - ulNum */
    mov     %o3, %o4            /* Save new value for later */
    cas     [%o0], %o2, %o3     /* Set *pNum if *pNum is unchanged */
    cmp     %o2, %o3            /* Did we succeed in saving the value?*/
    bne,a   1b                  /* Retry if we didn't. */
    ld      [%o0], %o2          /* <<only executes if we branch>> */
    mov     %o4, %o0            /* Save return value */
.end


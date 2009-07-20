! ***** BEGIN LICENSE BLOCK *****
! Source last modified: $Id: mutex_setbit.il,v 1.3 2004/07/09 18:23:09 hubbe Exp $
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
 *  mutex_setbit.il
 *
 *  Solaris/SPARC inline assembly mutex routines used by mutex.h
 *
 *  Note: These are implemented using the standard SPARC leaf procedure
 *  optimization, so the %o "output" registers are acutally our input and
 *  local registers.
 *
 */



/***********************************************************************
 * extern "C" int _HXMutexSetBitCAS(HX_MUTEX pLock)             %i0
 *
 * Test-and-Set primitive op based on CAS.
 *
 * This implementation is based on an example from The SPARC Architecture 
 * Manual V9, pg. 318.
 * This is implemented using the SPARC leaf procedure optimization.
 * This is implemented using CAS which is not available on older processors.
 */
.inline _HXMutexSetBitCAS,4     /* one 32-bit arg == 4 bytes */
    mov 1,%o1
    cas [%o0],%g0,%o1
    mov %o1,%o0
    membar #LoadLoad | #LoadStore
.end


/***********************************************************************
 * extern "C" int _HXMutexLockImp(HX_MUTEX pLock)                %i0
 *
 * Note: This is implemented using the standard SPARC leaf procedure
 * optimization, so the %o "output" registers are acutally our
 * input and local registers.
 *
 * Note: this implementation is based on an example from
 * The SPARC Architecture Manual V9, pg. 318.
 */
.inline _HXMutexClearBit,4      /* one 32-bit arg == 4 bytes */
    membar #StoreStore
    membar #LoadStore
    stw  %g0,[%o0]
.end


/***********************************************************************
 * extern "C" int _HXMutexSetBitSWAP(HX_MUTEX pLock)            %i0
 *
 * Test-and-Set primitive op based on SWAP.
 * This is the old, less scalable SWAP-based test-and-test lock.
 *
 * This is implemented using the SPARC leaf procedure optimization.
 *
 * THIS IS CURRENTLY UNUSED!  USE _HXMutexSetBitCAS!
 */
.inline _HXMutexSetBitSWAP,4    /* one 32-bit arg == 4 bytes */
    mov  %o0,%o2
    mov  1,%o0
    swap [%o2],%o0
.end



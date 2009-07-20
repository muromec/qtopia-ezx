! 
! ***** BEGIN LICENSE BLOCK *****
! Source last modified: $Id: math64.il,v 1.3 2004/07/09 18:37:03 hubbe Exp $
! 
! Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
! 

!
!int MulDiv64(register int a, register int b, register int c)
!
.inline MulDiv64,12
	signx %o0
	signx %o1
	mulx %o0,%o1,%o0
	signx %o2
	sdivx %o0,%o2,%o0
.end

!/* Compute (a * b) >> 31, using 64-bit intermediate result */
!int MulShift31(register int a, register int b)
!
.inline MulShift31,8
	signx %o0
	signx %o1
	mulx  %o0,%o1,%o0
	srax  %o0,31,%o0
.end

!/* Compute (a * b) >> 32, using 64-bit intermediate result */
!int MulShift32(register int a, register int b)
!
.inline MulShift32,8
	signx %o0
	signx %o1
	mulx  %o0,%o1,%o0
	srax  %o0,32,%o0
.end

!/* Compute (a * b) >> n, using 64-bit intermediate result */
!int MulShiftN(register int a, register int b, register int n)
!
.inline MulShiftN,12
	signx %o0
	signx %o1
	mulx %o0,%o1,%o0
	signx %o2
	srax %o0,%o2,%o0
.end

!/* read the tick counter */
!int rdtsc(void)
!
.inline rdtsc,0
	mov %tick,%o0
.end


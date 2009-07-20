; ***** BEGIN LICENSE BLOCK *****
; Source last modified: $Id: atomicops.s,v 1.4 2007/07/06 20:39:18 jfinnecy Exp $
; 
; # Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.
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
#######################################################################
#
# atomicops.s - AIX/PowerPC atomic add and subtract
# 
#######################################################################
#
# Notes:
#
# XXXDC: Uses lwarx/stwcx to achieve a fast atomic update, only available
# on PPC or later, so you need to compile everything with -Wa,-m,ppc.
#
# Calling convention is arg1 is in GPR3, arg2 is in GPR4, etc.
# Return value is placed in GPR3 prior to returning.
#
# Used by common/pub/atomicbase.h
#
#######################################################################

# define registers to use
.set r3,3
.set r4,4
.set r5,5

#######################################################################
#
# Interface:
#   INT32 _HXAtomicAddRetINT32(INT32* pNum, INT32 nNum);
#
# Inputs:
#   Paramaters:
#     GPR3 :  INT32* pNum - pointer to integer to modify
#     GPR4 :  INT32  nNum - amount to increment by
#
# Outputs:
#   Modifies memory at *pNum:
#     *pNum = *pNum + nNum
#
#   Return value:
#     GPR3 :  INT32 - new value of *pNum
# 
.globl ._HXAtomicAddRetINT32
._HXAtomicAddRetINT32:
                lwarx   r5,0,r3    # Load r5 w/ *pNum, set reserve flag
                addc    r5,r4,r5   # add GPR4 to r5
                stwcx.  r5,0,r3    # Store r5 in *pNum if flag still set
                bne-    ._HXAtomicAddRetINT32 # retry if store failed
                addi    r3,r5,0    # Save return value in r3
                br                 # return from routine


#######################################################################
#
# Interface:
#   INT32 _HXAtomicSubRetINT32(INT32* pNum, INT32 nNum);
#
# Inputs:
#   Paramaters:
#     GPR3 :  INT32* pNum - pointer to integer to modify
#     GPR4 :  INT32  nNum - amount to decrement by
#
# Outputs:
#   Modifies memory at *pNum:
#     *pNum = *pNum + nNum
#
#   Return value:
#     GPR3 :  INT32 - new value of *pNum
# 
.globl ._HXAtomicSubRetINT32
._HXAtomicSubRetINT32:
                lwarx   r5,0,r3    # Load r5 w/ *pNum, set reserve flag
                subf    r5,r4,r5   # subtract GPR4 from r5
                stwcx.  r5,0,r3    # Store r5 in *pNum if flag still set
                bne-    ._HXAtomicSubRetINT32 # retry if store failed
                addi    r3,r5,0    # Save return value in r3
                br                 # return from routine


#######################################################################
#
# Interface:
#   UINT32 HXAtomicAddRetUINT32(UINT32* pNum, UINT32 nNum);
#
# Inputs:
#   Paramaters:
#     GPR3 :  UINT32* pNum - pointer to integer to modify
#     GPR4 :  UINT32  nNum - amount to increment by
#
# Outputs:
#   Modifies memory at *pNum:
#     *pNum = *pNum + nNum
#
#   Return value:
#     GPR3 :  UINT32 - new value of *pNum
# 
.globl ._HXAtomicAddRetUINT32
._HXAtomicAddRetUINT32:
                lwarx   r5,0,r3    # Load r5 w/ *pNum, set reserve flag
                addc    r5,r4,r5   # add GPR4 to r5
                stwcx.  r5,0,r3    # Store r5 in *pNum if flag still set
                bne-    ._HXAtomicAddRetUINT32 # retry if store failed
                addi    r3,r5,0    # Save return value in r3
                br                 # return from routine

#######################################################################
#
# Interface:
#   UINT32 HXAtomicSubRetUINT32(UINT32* pNum, UINT32 nNum);
#
# Inputs:
#   Paramaters:
#     GPR3 :  UINT32* pNum - pointer to integer to modify
#     GPR4 :  UINT32  nNum - amount to decrement by
#
# Outputs:
#   Modifies memory at *pNum:
#     *pNum = *pNum + nNum
#
#   Return value:
#     GPR3 :  UINT32 - new value of *pNum
# 
.globl ._HXAtomicSubRetUINT32
._HXAtomicSubRetUINT32:
                lwarx   r5,0,r3    # Load r5 w/ *pNum, set reserve flag
                subf    r5,r4,r5   # subtract GPR4 from r5
                stwcx.  r5,0,r3    # Store r5 in *pNum if flag still set
                bne-    ._HXAtomicSubRetUINT32 # retry if store failed
                addi    r3,r5,0    # Save return value in r3
                br                 # return from routine


#######################################################################

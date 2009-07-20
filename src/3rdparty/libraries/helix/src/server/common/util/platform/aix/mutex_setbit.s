# ***** BEGIN LICENSE BLOCK *****  
# Source last modified: $Id: mutex_setbit.s,v 1.2 2003/01/23 23:42:51 damonlan Exp $ 
#   
# Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
#       
# The contents of this file, and the files included with this file, 
# are subject to the current version of the RealNetworks Public 
# Source License (the "RPSL") available at 
# http://www.helixcommunity.org/content/rpsl unless you have licensed 
# the file under the current version of the RealNetworks Community 
# Source License (the "RCSL") available at 
# http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
# will apply. You may also obtain the license terms directly from 
# RealNetworks.  You may not use this file except in compliance with 
# the RPSL or, if you have a valid RCSL with RealNetworks applicable 
# to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
# the rights, obligations and limitations governing use of the 
# contents of the file. 
#   
# This file is part of the Helix DNA Technology. RealNetworks is the 
# developer of the Original Code and owns the copyrights in the 
# portions it created. 
#   
# This file, and the files included with this file, is distributed 
# and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY 
# KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS 
# ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES 
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET 
# ENJOYMENT OR NON-INFRINGEMENT. 
#  
# Technology Compatibility Kit Test Suite(s) Location:  
#    http://www.helixcommunity.org/content/tck  
#  
# Contributor(s):  
#   
# ***** END LICENSE BLOCK *****  
#######################################################################
#
# mutex_setbit.s - AIX/PowerPC test-and-set mutex primitives
# 
#######################################################################
#
# Notes:
#
# Uses lwarx/stwcx to achieve a fast atomic update, only available
# on PPC or later, so you need to compile everything with -Wa,-m,ppc.
#
# Calling convention is arg1 is in GPR3, arg2 is in GPR4, etc.
# Return value is placed in GPR3 prior to returning.
#
#######################################################################

# define registers to use
.set r3,3
.set r4,4
.set r5,5

#######################################################################
#
# Interface:
#   INT32 _HXMutexSetBit(INT32* pLock);
#
# Inputs:
#   Paramaters:
#     GPR3 :  INT32* pNum - pointer to lock to modify
#
# Outputs:
#   Atomically modifies memory at *pLock:
#     *pLock = 1
#
#   Return value:
#     GPR3 :  INT32 - previous value of lock
# 
.globl ._HXMutexSetBit
._HXMutexSetBitAsm:
                li      r4,1       # set r4 to 1
.SetBit:        lwarx   r5,0,r3    # load r5 w/ *pLock, set reserve flag
                cmpw    r5,r4      # is it already locked?
                beq     .AlreadySet# lock is already set so skip to end
                stwcx.  r4,0,r3    # Store r4 in *pLock if reserve flag is set
                bne-    .SetBit    # lost reservation, try again
.AlreadySet:    and     r3,r5,r5   # copy previous lock value to r3
                isync              # wait for all instructions to finish
                br                 # done, return from routine


#######################################################################
#
# Interface:
#   INT32 _HXMutexClearBit(INT32* pLock);
#
# Inputs:
#   Paramaters:
#     GPR3 :  INT32* pNum - pointer to lock to modify
#
# Outputs:
#   Atomically modifies memory at *pLock:
#     *pLock = 0
#
#   Return value:
#     none
# 
.globl ._HXMutexClearBit
._HXMutexClearBitAsm:
                sync               # delay until processor stored finish
                li      r4,0       # set r4 to 0
                stw     r4,0(r3)   # Store r4 in *pLock
                br                 # done, return from routine


#######################################################################

# ***** BEGIN LICENSE BLOCK *****  
# Source last modified: $Id: stackframe.s,v 1.2 2003/01/23 23:42:51 damonlan Exp $ 
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
# stackframe.s - AIX/POWER stack frame retrieval
# 
#######################################################################
#
# Notes:
#
# Calling convention is arg1 is in GPR3, arg2 is in GPR4, etc.
# Return value is placed in GPR3 prior to returning.
#
# This uses the 'cal' opcode which is only supported on
# the POWER and POWER2 processors, not the basic PowerPC
# (which would probably need to use 'ai r3,0,r1', and
# which is not supported on POWER/POWER2.)
#
#######################################################################

# define registers to use
.set r1,1
.set r3,3

#######################################################################
#
# Interface:
#   void* HXGetStackFrame();
#
# Inputs:
#   GPR1 : stack pointer
#
# Outputs:
#   Return value:
#     GPR3 :  void* - caller's stack frame pointer
# 
.globl .HXGetStackFrame
.HXGetStackFrame:
                cal     r3,0(r1)   # Load r3 w/ caller's saved stack pointer
                br                 # return from routine

#######################################################################

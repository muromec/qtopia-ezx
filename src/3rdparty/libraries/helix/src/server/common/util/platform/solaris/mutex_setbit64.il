/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: mutex_setbit64.il,v 1.1 2003/06/15 04:51:41 dcollins Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
 *       
 * The contents of this file, and the files included with this file, 
 * are subject to the current version of the RealNetworks Public 
 * Source License (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the current version of the RealNetworks Community 
 * Source License (the "RCSL") available at 
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
 * will apply. You may also obtain the license terms directly from 
 * RealNetworks.  You may not use this file except in compliance with 
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable 
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
 * the rights, obligations and limitations governing use of the 
 * contents of the file. 
 *   
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the 
 * portions it created. 
 *   
 * This file, and the files included with this file, is distributed 
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY 
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS 
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET 
 * ENJOYMENT OR NON-INFRINGEMENT. 
 *  
 * Technology Compatibility Kit Test Suite(s) Location:  
 *    http://www.helixcommunity.org/content/tck  
 *  
 * Contributor(s):  
 *   
 * ***** END LICENSE BLOCK ***** */  
/*
 *  mutex_setbit64.il
 *
 *  Solaris/SPARC inline assembly mutex routines (64-bit compiler mode)
 *
 *  Note: These are implemented using the standard SPARC leaf procedure
 *  optimization, so the %o "output" registers are acutally our input and
 *  local registers.
 *
 */



/***********************************************************************
 * extern "C" int _HXMutexSetBitCAS(HX_MUTEX pLock)     %o0
 *
 * Test-and-Set primitive op based on CAS.
 *
 * This implementation is based on an example from The SPARC Architecture 
 * Manual V9, pg. 318.
 * This is implemented using the SPARC leaf procedure optimization.
 * This is implemented using CAS which is not available on older processors.
 */
.inline _HXMutexSetBitCAS,8     /* one 64-bit ptr arg == 8 bytes */
    mov 1,%o1
    cas [%o0],%g0,%o1
    mov %o1,%o0
    membar #LoadLoad | #LoadStore
.end


/***********************************************************************
 * extern "C" int _HXMutexClearBit(HX_MUTEX pLock)      %o0
 *
 * Note: This is implemented using the standard SPARC leaf procedure
 * optimization, so the %o "output" registers are acutally our
 * input and local registers.
 *
 * Note: this implementation is based on an example from
 * The SPARC Architecture Manual V9, pg. 318.
 */
.inline _HXMutexClearBit,8      /* one 64-bit arg == 8 bytes */
    membar #StoreStore
    membar #LoadStore
    stw  %g0,[%o0]
.end


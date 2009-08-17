/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: mutexlist.h,v 1.2 2003/01/23 23:42:55 damonlan Exp $ 
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
 *  This implements a list of locks that will be unlocked in the
 *  case of a crash-avoid event in this process/thread.
 */

#ifndef _MUTEXLIST_H
#define _MUTEXLIST_H

class ManagedMutexList
{
public:
                                ManagedMutexList();
                                ~ManagedMutexList();
    HX_RESULT                   AddMutex(HX_MUTEX pMutex);
    HX_RESULT                   RemoveMutex(HX_MUTEX pMutex);
    INT32                       UnlockAll();
    inline INT32                LockCount() { return m_nCount; }

private:
    //m_pMutexList is a char** to get around win32 compiler issues:
    char**                      m_pMutexList;
    INT32                       m_nCount;
};

#endif //_MUTEXLIST_H

/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: tsdict.h,v 1.2 2003/01/23 23:42:51 damonlan Exp $ 
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

#ifndef _TSDICT_H_
#define _TSDICT_H_

#include "dict.h"

class TSDict : public Dict
{
public:
    TSDict(unsigned int nbuckets=16);
    TSDict(int(*comp)(const char*,const char*),
           unsigned int(*hash)(const char*) = default_strhash,
           unsigned int nbuckets=16);
    ~TSDict();

    Dict_entry* enter(Key key, void* obj);
    void*       remove(Key key);
    Dict_entry*	find(Key key);
    void	first(unsigned int&h, Dict_entry*& e);
    void	next(unsigned int& h, Dict_entry*& e);
#ifdef XXXAAK_AWAITING_CR
    Dict_entry* enter(Key key, void* obj, UINT32& hashId);
    void*       remove(UINT32 hashId);
    Dict_entry*	find(UINT32 hashId);
#endif

private:
    HX_MUTEX m_pMutex;
};

inline void
TSDict::first(unsigned int&h, Dict_entry*& e)
{
    HXMutexLock(m_pMutex);
    Dict::first(h, e);
    HXMutexUnlock(m_pMutex);
}

#endif /* _TSDICT_H_ */

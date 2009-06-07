/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: tsdict.cpp,v 1.2 2003/01/23 23:42:51 damonlan Exp $ 
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


#include "hxtypes.h"
#include "hxcom.h"
#include "mutex.h"
#include "tsdict.h"

TSDict::TSDict(unsigned int nbuckets) : Dict(nbuckets)
{
    m_pMutex = HXCreateMutex();
}

TSDict::TSDict(int(*comp)(const char*,const char*),
               unsigned int(*hash)(const char*),
               unsigned int nbuckets) : Dict(comp, hash, nbuckets)
{
    m_pMutex = HXCreateMutex();
}

TSDict::~TSDict()
{
    HXDestroyMutex(m_pMutex);
    m_pMutex = NULL;
}

Dict_entry*
TSDict::enter(Key key, void* obj)
{
    HXMutexLock(m_pMutex);
    Dict_entry* pEntry = Dict::enter(key, obj);
    HXMutexUnlock(m_pMutex);

    return pEntry;
}

void*
TSDict::remove(Key key)
{
    HXMutexLock(m_pMutex);
    void* pData = Dict::remove(key);
    HXMutexUnlock(m_pMutex);

    return pData;
}

Dict_entry*
TSDict::find(Key key)
{
    HXMutexLock(m_pMutex);
    Dict_entry* pEntry = Dict::find(key);
    HXMutexUnlock(m_pMutex);

    return pEntry;
}

void
TSDict::next(unsigned int& h, Dict_entry*& e)
{
    HXMutexLock(m_pMutex);
    Dict::next(h, e);
    HXMutexUnlock(m_pMutex);
}

#ifdef XXXAAK_AWAITING_CR
Dict_entry*
TSDict::enter(Key key, void* obj, UINT32& hashId)
{
    HXMutexLock(m_pMutex);
    Dict_entry* pEntry = Dict::enter(key, obj, hashId);
    HXMutexUnlock(m_pMutex);

    return pEntry;
}

void*
TSDict::remove(UINT32 hashId)
{
    HXMutexLock(m_pMutex);
    void* pData = Dict::remove(hashId);
    HXMutexUnlock(m_pMutex);

    return pData;
}

Dict_entry*
TSDict::find(UINT32 hashId)
{
    HXMutexLock(m_pMutex);
    Dict_entry* pEntry = Dict::find(hashId);
    HXMutexUnlock(m_pMutex);

    return pEntry;
}
#endif

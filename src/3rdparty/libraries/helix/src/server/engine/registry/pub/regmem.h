/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: regmem.h,v 1.3 2003/09/04 22:39:09 dcollins Exp $ 
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
 *  A memory cache wrapper object that dereferences the 
 *  ServerRegistry's _proc pointer to get the caller to 
 *  use the correct MemCache instance for this process.
 *  There is one instance of RegistryMemCache that contains
 *  a reference to our single instance of ServerRegistry.
 *  ServerRegistry contains a _proc pointer that is updated
 *  as needed to point to the correct Process.  We use this
 *  to get to _proc->pc->mem_cache from which we do the
 *  cached memory allocation.   XXXDC
 *
 */

#ifndef _REGMEM_H_
#define _REGMEM_H_

#include "hxtypes.h"
#include "hxcom.h"
#include "proc_container.h"
#include "mem_cache.h"

class ServerRegistry;

class RegistryMemCache
{
public:
    RegistryMemCache(ServerRegistry* pRegistry);
    ~RegistryMemCache();
    MemCache* RegistryAllocator(void) const;

    ServerRegistry* m_pRegistry;
};

#define REGISTRY_CACHE_MEM\
    void* operator new(size_t size, RegistryMemCache* pMalloc = NULL)\
    {\
        void* pMem;\
	if (pMalloc)\
	{\
	    pMem = pMalloc->RegistryAllocator()->CacheNew(size + sizeof(RegistryMemCache*));\
	}\
	else\
	{\
	    pMem = (void*)::new char[size + sizeof(RegistryMemCache*)];\
	}\
        *(RegistryMemCache**)pMem = pMalloc;\
        return ((unsigned char*)pMem + sizeof(RegistryMemCache*));\
    }\
\
    void operator delete(void* pMem)\
    {\
        pMem = (unsigned char*)pMem - sizeof(RegistryMemCache*);\
        RegistryMemCache* pMalloc = *(RegistryMemCache**)pMem;\
	if (pMalloc)\
	{\
	    pMalloc->RegistryAllocator()->CacheDelete((char *)pMem);\
	}\
	else\
	{\
	    ::delete[] (char*)pMem;\
	}\
    }\

#endif

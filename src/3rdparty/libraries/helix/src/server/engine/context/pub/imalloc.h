/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: imalloc.h,v 1.3 2003/09/04 22:39:08 dcollins Exp $ 
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

#ifndef _IMALLOC_H_
#define _IMALLOC_H_

struct	Challenge;
class	CleanupElem;
class	URL;
class 	TCPIO;
class 	UDPIO;
struct	BW_encoding;
class 	Process;
class   HXProtocol;
class   Player;
class   CHXSimpleList;
class 	Stream;

#include "hxtypes.h"
#include "hxcom.h"
#include "proc.h"
#include "hxcomm.h"
#include "mem_cache.h"
#include "source.h"
#include "sockio.h"

class IMallocContext : public IMalloc
                     , public IHXFastAlloc
{
public:
    IMallocContext(MemCache* pCache) { m_ulRefCount = 0; m_pCache = pCache; }
    ~IMallocContext()           { }

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);
    STDMETHOD_(void*,Alloc)     (THIS_ UINT32      count);
    STDMETHOD_(void*,Realloc)   (THIS_ void*       pMem, UINT32 count);
    STDMETHOD_(void,Free)       (THIS_ void*       pMem);
    STDMETHOD_(UINT32,GetSize)  (THIS_ void*	   pMem);
    STDMETHOD_(BOOL,DidAlloc)   (THIS_ void*       pMem);
    STDMETHOD_(void,HeapMinimize)   (THIS);

    STDMETHOD_(void*,FastAlloc)     (THIS_ UINT32      count);
    STDMETHOD_(void,FastFree)   (THIS_ void*       pMem);

private:
    ULONG32                     m_ulRefCount;
    MemCache*			m_pCache;
};

#endif /*_IMALLOC_H_*/

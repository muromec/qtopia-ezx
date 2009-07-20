/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: servbuffer.h,v 1.4 2005/06/29 20:34:08 dcollins Exp $ 
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

#ifndef _SERVBUFFER_H_
#define _SERVBUFFER_H_

#include "ihxpckts.h"
#include "hxvalue.h"
#include "hxstring.h"
//#include "mem_cache.h"

class ServerBuffer : public IHXBuffer
{
protected:
    ULONG32					m_ulRefCount;
    UCHAR*					m_pData;
    ULONG32					m_ulLength;
    ULONG32					m_ulAllocLength;
    BOOL					m_bPassesInBuffer;

    virtual ~ServerBuffer();


public:
    //MEM_CACHE_MEM
    ServerBuffer();
    ServerBuffer(BOOL bAlreadyHasOneRef);
    ServerBuffer(UCHAR* pData, UINT32 ulLength);
    STDMETHOD(QueryInterface)	(THIS_
				 REFIID riid,
				 void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);
    STDMETHOD(Get)		(THIS_
				 REF(UCHAR*)	pData, 
				 REF(ULONG32)	ulLength);
    STDMETHOD(Set)		(THIS_
				 const UCHAR*	pData, 
				 ULONG32	ulLength);
    STDMETHOD(SetSize)		(THIS_
				ULONG32		ulLength);
    STDMETHOD_(ULONG32,GetSize)	(THIS);
    STDMETHOD_(UCHAR*,GetBuffer)(THIS);
    
    static HX_RESULT FromCharArray(const char* szIn, IHXBuffer** ppbufOut);
    static HX_RESULT FromCharArray(const char* szIn, UINT32 ulLength, 
	IHXBuffer** ppbufOut);
};

#endif

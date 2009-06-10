/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: smallbuffer.cpp,v 1.2 2005/08/31 02:57:37 jc Exp $ 
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
#include "hxresult.h"
#include "ihxpckts.h"
#include "chxpckts.h"
#include "smallbuffer.h"
#include "rtsputil.h"



/* SmallBuffer is basically a copy of ServerBuffer that includes a 100
 * byte array so that we don't need to do a separate alloc/free for the 
 * data buffer. TODO : Should add the ability to dynamically alloc the 
 * data if it exceeds MAX_SMALLBUFFER_LENGTH, otherwise, this class will
 * be dangerous if ever used as a general purpose buffer. Its currently
 * intended only to be used for packet headers in performance critical 
 * code
 */
SmallBuffer::SmallBuffer()
    : m_ulRefCount(0)
    , m_ulLength(0)
{
}

SmallBuffer::SmallBuffer(BOOL bAlreadyHasOneRef)
    : m_ulRefCount(bAlreadyHasOneRef ? 1 : 0)
    , m_ulLength(0)
{
}

SmallBuffer::~SmallBuffer()
{
}

STDMETHODIMP
SmallBuffer::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXBuffer))
    {
	    AddRef();
	    *ppvObj = (IHXBuffer*)this;
	    return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
	    AddRef();
	    *ppvObj = this;
	    return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}


STDMETHODIMP_(ULONG32)
SmallBuffer::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}


STDMETHODIMP_(ULONG32)
SmallBuffer::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
	return m_ulRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP
SmallBuffer::Get(REF(UCHAR*)	pData, 
	       REF(ULONG32)	ulLength)
{
    pData    = m_pData;
    ulLength = m_ulLength;

    return HXR_OK;
}


STDMETHODIMP SmallBuffer::Set(const UCHAR*	pData, 
			    ULONG32		ulLength)
{
//printf("SmallBuffer(%p)::Set, length %d\n", this, ulLength);
    /*
     * We allow changing the packet info when it is owned
     * by at most one user.
     */
    if (m_ulRefCount > 1)
    {
	return HXR_UNEXPECTED;
    }

    if (ulLength > MAX_SMALLBUFFER_LENGTH)
    {
	return HXR_UNEXPECTED;
    }

    memcpy(m_pData, pData, ulLength);
    m_ulLength = ulLength;

    return HXR_OK;
}


STDMETHODIMP
SmallBuffer::SetSize(ULONG32 ulLength)
{
    /*
     * We allow changing the packet info when it is owned
     * by at most one user.
     */
    if (m_ulRefCount > 1)
    {
	return HXR_UNEXPECTED;
    }

    if (ulLength <= MAX_SMALLBUFFER_LENGTH)
    {
        m_ulLength = ulLength;

	return HXR_OK;
    }
    
    return HXR_UNEXPECTED;
}


STDMETHODIMP_(ULONG32)
SmallBuffer::GetSize()
{
    return m_ulLength;
}


STDMETHODIMP_(UCHAR*)
SmallBuffer::GetBuffer()
{
    return m_pData;
}


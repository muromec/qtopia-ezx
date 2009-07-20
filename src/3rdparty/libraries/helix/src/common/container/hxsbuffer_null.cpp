/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsbuffer_null.cpp,v 1.3 2007/07/06 20:34:58 jfinnecy Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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
 *  In order to support the IHXBuffer interface, we must cast away the const
 *  qualifier on our m_pData in the Get() and GetBuffer() functions.  If you
 *  can think of a better way, please let me know ... TDM
 */

#include "hxtypes.h"
#include "hxcom.h"
#include "hxresult.h"
#include "ihxpckts.h"
#include "chxpckts.h"
#include "hxsbuffer.h"
//#include "rtsputil.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE                
static char HX_THIS_FILE[] = __FILE__;
#endif

#define MAX_NULL_BUFFER_LEN		36767
static unsigned char* z_pNullBuffer = NULL;

CHXStaticBufferNULL::CHXStaticBufferNULL(void) :
    m_ulRefCount(0),
    m_pbufRef(NULL),
    m_pData(NULL),
    m_ulLength(0)
{
    if (z_pNullBuffer == NULL)
    {
	z_pNullBuffer = new unsigned char[MAX_NULL_BUFFER_LEN];
    }

    ::memset(z_pNullBuffer, 0, MAX_NULL_BUFFER_LEN);
}

CHXStaticBufferNULL::CHXStaticBufferNULL(UCHAR* pData, UINT32 ulLength) :
    m_ulRefCount(0),
    m_pbufRef(NULL),
    m_pData(NULL),
    m_ulLength(ulLength)
{
    HX_ASSERT(m_ulLength <= MAX_NULL_BUFFER_LEN); 
    // this singleton buffer is never deleted
    if (z_pNullBuffer == NULL)
    {
	z_pNullBuffer = new unsigned char[MAX_NULL_BUFFER_LEN];
    }
}

CHXStaticBufferNULL::CHXStaticBufferNULL(IHXBuffer* pbuf, UINT32 pos, UINT32 len) :
    m_ulRefCount(0),
    m_pbufRef(NULL),
    m_pData(NULL),
    m_ulLength(len)
{
    HX_ASSERT(pos+len <= pbuf->GetSize());
    HX_ASSERT(m_ulLength <= MAX_NULL_BUFFER_LEN); 
    if (z_pNullBuffer == NULL)
    {
	z_pNullBuffer = new unsigned char[MAX_NULL_BUFFER_LEN];
    }
} 

CHXStaticBufferNULL::~CHXStaticBufferNULL(void)
{
   if (CHXBaseCountingObject::ObjectsActive() == 1)
   {
	HX_VECTOR_DELETE(z_pNullBuffer);
   }

}

/*** IUnknown methods ***/

STDMETHODIMP
CHXStaticBufferNULL::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
    {
       { GET_IIDHANDLE(IID_IUnknown),  this },
       { GET_IIDHANDLE(IID_IHXBuffer), (IHXBuffer*)this },
    }; 
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}


STDMETHODIMP_(ULONG32)
CHXStaticBufferNULL::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CHXStaticBufferNULL::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXBuffer methods ***/

STDMETHODIMP
CHXStaticBufferNULL::Get(REF(UCHAR*) pData, REF(ULONG32) ulLength)
{
    pData    = (UCHAR*)z_pNullBuffer;
    ulLength = m_ulLength;

    return HXR_OK;
}


STDMETHODIMP
CHXStaticBufferNULL::Set(const UCHAR* pData, ULONG32 ulLength)
{
    // We allow changing the packet info when it is owned by atmost one user.
    if (m_ulRefCount > 1)
    {
        return HXR_UNEXPECTED;
    }

    m_ulLength = ulLength;
    HX_ASSERT(m_ulLength <= MAX_NULL_BUFFER_LEN); 

    return HXR_OK;
}


STDMETHODIMP
CHXStaticBufferNULL::SetSize(ULONG32 ulLength)
{
    // We allow changing the packet info when it is owned by atmost one user.
    if (m_ulRefCount > 1)
    {
        return HXR_UNEXPECTED;
    }

    m_ulLength = ulLength;
    HX_ASSERT(m_ulLength <= MAX_NULL_BUFFER_LEN); 

    return HXR_OK;
}


STDMETHODIMP_(ULONG32)
CHXStaticBufferNULL::GetSize(void)
{
    return m_ulLength;
}

STDMETHODIMP_(UCHAR*)
CHXStaticBufferNULL::GetBuffer(void)
{
    return (UCHAR*)z_pNullBuffer; 
}

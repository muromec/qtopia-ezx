/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdatasourcemmfclip.cpp,v 1.6 2007/07/06 20:35:15 jfinnecy Exp $
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

#include "hxcom.h"
#include "hxtypes.h"
#include "hxresult.h"
#include "hxdatasourcemmfclip.h"
#include "hxassert.h"
#include "hxtlogutil.h"

CHXDataSourceMMFClip::CHXDataSourceMMFClip(CMMFClip *pImpl):
    m_pImpl(pImpl)
#ifdef HELIX_FEATURE_S60_PROGDOWN
    ,m_ulDownloadComplete(0L)
    ,m_ulDownloadSize(0L)
    ,m_ulUnderflowTimeoutInMillis(DEFAULT_UNDERFLOW_TIMEOUT_IN_MILLIS)
#endif
{
}

STDMETHODIMP
CHXDataSourceMMFClip::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
    {
        { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXMMFDataSource*)this },
        { GET_IIDHANDLE(IID_IHXMMFDataSource), (IHXMMFDataSource*) this },
#ifdef HELIX_FEATURE_S60_PROGDOWN
        { GET_IIDHANDLE(IID_IHXDownloadEventObserver), (IHXDownloadEventObserver*) this }
#endif
    };

    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}


STDMETHODIMP_(ULONG32)
CHXDataSourceMMFClip::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CHXDataSourceMMFClip::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_ulRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

STDMETHODIMP CHXDataSourceMMFClip::Close()
{
    return HXR_OK;
}

CHXDataSourceMMFClip::~CHXDataSourceMMFClip()
{
    // m_pImpl weak ref.
}


UINT32 CHXDataSourceMMFClip::Read(
            void *pBuf, ULONG32 size, ULONG32 count)

{
    UINT32 retVal = 0;

    TRAPD(error, retVal = ReadL(pBuf, size, count));

    if(error != KErrNone)
    {
        HXLOGL1(HXLOG_FILE, "CHXDataSourceMMFClip::Read error=%d", error);
        SetLastError(HXR_READ_ERROR);
        retVal = 0;
    }

    return retVal;
}

UINT32 CHXDataSourceMMFClip::Write(
            void *pBuf, ULONG32 size, ULONG32 count)

{
    UINT32 retVal = 0;

    TRAPD(error, retVal = WriteL(pBuf, size, count));

    if(error != KErrNone)
    {
        HXLOGL1(HXLOG_FILE, "CHXDataSourceMMFClip::Write error=%d", error);
        SetLastError(HXR_WRITE_ERROR);
    }

    return retVal;
}

// returns 0 on failure otherwise number of bytes read.
#ifdef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY

UINT32 CHXDataSourceMMFClip::ReadL(
            void *pBuf, ULONG32 size, ULONG32 count)
{
    TInt    requestSize = size*count;
    TPtr8   ptr8((TUint8 *)pBuf, 0, requestSize);    

    CMMFPtrBuffer *mmfPtrBuffer;
    mmfPtrBuffer = CMMFPtrBuffer::NewL(ptr8);
    CleanupStack::PushL(mmfPtrBuffer);

    m_pImpl->ReadBufferL(mmfPtrBuffer, m_ulPosition);

    // return the number of bytes read
    UINT32 bytesRead = mmfPtrBuffer->Data().Size();

    CleanupStack::PopAndDestroy();

    m_ulPosition += bytesRead;
    return bytesRead;
}

UINT32 CHXDataSourceMMFClip::WriteL(
            void *pBuf, ULONG32 size, ULONG32 count)
{
    TInt    requestSize = size*count;
    TPtr8   ptr8((TUint8 *)pBuf, 0, requestSize);    

    CMMFPtrBuffer *mmfPtrBuffer;
    mmfPtrBuffer = CMMFPtrBuffer::NewL(ptr8);
    CleanupStack::PushL(mmfPtrBuffer);

    m_pImpl->WriteBufferL(mmfPtrBuffer, m_ulPosition);

    CleanupStack::PopAndDestroy();
    m_ulPosition += requestSize;

    return requestSize;
}

#else

UINT32 CHXDataSourceMMFClip::ReadL(
            void *pBuf, ULONG32 size, ULONG32 count)
{
    TInt    requestSize = size*count;

    CMMFDataBuffer *mmfBuffer;
    mmfBuffer = CMMFDataBuffer::NewL(requestSize);
    CleanupStack::PushL(mmfBuffer);

    m_pImpl->ReadBufferL(mmfBuffer, m_ulPosition);

    // return the number of bytes read
    UINT32 bytesRead = mmfBuffer->Data().Size();

    TPtr8   ptr8((TUint8 *)pBuf, 0, requestSize);    
    ptr8.Copy(mmfBuffer->Data());

    CleanupStack::PopAndDestroy();
    m_ulPosition += bytesRead;
    return bytesRead;
}

UINT32 CHXDataSourceMMFClip::WriteL(
            void *pBuf, ULONG32 size, ULONG32 count)
{
    TInt    requestSize = size*count;
    CMMFDataBuffer *mmfBuffer;
    mmfBuffer = CMMFDataBuffer::NewL(requestSize);
    CleanupStack::PushL(mmfBuffer);

    TPtr8 ptr8((TUint8 *)pBuf, 0, requestSize);
    mmfBuffer->Data().Copy(ptr8);

    m_pImpl->WriteBufferL(mmfBuffer, m_ulPosition);

    CleanupStack::PopAndDestroy();
    m_ulPosition += requestSize;

    return requestSize;
}

#endif

STDMETHODIMP CHXDataSourceMMFClip::GetSize(UINT32 &ulSize)
{
    ulSize = m_pImpl->Size();
    return HXR_OK;
}

#ifdef HELIX_FEATURE_S60_PROGDOWN

UINT32 CHXDataSourceMMFClip::IsDownloadComplete()
{
    return m_ulDownloadComplete;
}

UINT32 CHXDataSourceMMFClip::GetDownloadSize()
{
    return m_ulDownloadSize;
}

UINT32 CHXDataSourceMMFClip::GetUnderflowTimeoutInMillis()
{
    return m_ulUnderflowTimeoutInMillis;
}

STDMETHODIMP CHXDataSourceMMFClip::SetDownloadComplete(UINT32 downloadComplete) 
{
    m_ulDownloadComplete = downloadComplete;
}

STDMETHODIMP CHXDataSourceMMFClip::SetDownloadSize(UINT32 downloadSize) 
{
    m_ulDownloadSize = downloadSize;
}

STDMETHODIMP CHXDataSourceMMFClip::SetUnderflowTimeoutInMillis(UINT32 timeout)
{
    m_ulUnderflowTimeoutInMillis = timeout;
}

#endif


/* ***** BEGIN LICENSE BLOCK ***** 
 * 
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
 * 
 * Patent Notices: This file may contain technology protected by one or 
 * more of the patents listed at www.helixcommunity.org
 * 
 * 1.   The contents of this file, and the files included with this file,
 * are protected by copyright controlled by RealNetworks and its 
 * licensors, and made available by RealNetworks subject to the current 
 * version of the RealNetworks Public Source License (the "RPSL") 
 * available at  * http://www.helixcommunity.org/content/rpsl unless 
 * you have licensed the file under the current version of the 
 * RealNetworks Community Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply.  You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 * 
 * 2.  Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above.  Please note that RealNetworks and its 
 * licensors disclaim any implied patent license under the GPL.  
 * If you wish to allow use of your version of this file only under 
 * the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting Paragraph 1 above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete Paragraph 1 above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 * 
 * This file is part of the Helix DNA Technology.  RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.   Copying, including reproducing, storing, 
 * adapting or translating, any or all of this material other than 
 * pursuant to the license terms referred to above requires the prior 
 * written consent of RealNetworks and its licensors
 * 
 * This file, and the files included with this file, is distributed
 * and made available by RealNetworks on an 'AS IS' basis, WITHOUT 
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS 
 * AND ITS LICENSORS HEREBY DISCLAIM  ALL SUCH WARRANTIES, INCLUDING 
 * WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck
 * 
 * Contributor(s): 
 *
 * ***** END LICENSE BLOCK ***** */ 

#include "hxtypes.h"
#include "hxcom.h"
#include "hxfiles.h"
#include "ihxpckts.h"
#include "baseobj.h"
#include "nestbuff.h"
#include "single_buffer_file_object.h"
//#define HELIX_FEATURE_LOGLEVEL_NONE // Uncomment to disable logging
#include "hxtlogutil.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

STDMETHODIMP CHXSingleBufferFileObject::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
    {
        { GET_IIDHANDLE(IID_IUnknown),      (IUnknown*) (IHXFileObject*) this },
        { GET_IIDHANDLE(IID_IHXFileObject),             (IHXFileObject*) this },
        { GET_IIDHANDLE(IID_IHXFileStat),                 (IHXFileStat*) this },
        { GET_IIDHANDLE(IID_IHXRequestHandler),     (IHXRequestHandler*) this }
    };

    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) CHXSingleBufferFileObject::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXSingleBufferFileObject::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP CHXSingleBufferFileObject::Init(ULONG32 ulFlags, IHXFileResponse* pFileResponse)
{
    HXLOGL4(HXLOG_FLVR, "CHXSingleBufferFileObject[%p]::Init(ulFlags=0x%08x,pFileResponse=%p)", this, ulFlags, pFileResponse);
    HX_RESULT retVal = HXR_FAIL;

    if (pFileResponse)
    {
        // We only accept binary reads
        HX_ASSERT(ulFlags == (HX_FILE_READ | HX_FILE_BINARY));
        if (ulFlags == (HX_FILE_READ | HX_FILE_BINARY))
        {
            // Save the response interface
            HX_RELEASE(m_pFileResponse);
            m_pFileResponse = pFileResponse;
            m_pFileResponse->AddRef();
            // Make sure we have a valid buffer
            if (m_pBuffer && m_pBuffer->GetSize() > 0)
            {
                // Reset the file offset
                m_ulFileOffset = 0;
                // Call back to the response interface with success
                HXLOGL4(HXLOG_FLVR, "\tcalling IHXFileResponse::InitDone(HXR_OK)");
                m_pFileResponse->InitDone(HXR_OK);
                // Clear the return value
                retVal = HXR_OK;
            }
        }
    }

    return retVal;
}

STDMETHODIMP CHXSingleBufferFileObject::GetFilename(REF(const char*) rpszFileName)
{
    HX_ASSERT(FALSE && "GetFilename() not supported on CHXSingleBufferFileObject");
    return HXR_NOTIMPL;
}

STDMETHODIMP CHXSingleBufferFileObject::Close()
{
    HXLOGL4(HXLOG_FLVR, "CHXSingleBufferFileObject[%p]::Close()", this);
    // Release the contained buffer
    HX_RELEASE(m_pBuffer);
    // Transfer the response interface ref to a local variable
    // in case the call to CloseDone() results in our destruction.
    IHXFileResponse* pResponse = m_pFileResponse;
    m_pFileResponse = NULL;
    // If we have a response interface, call CloseDone() on it.
    if (pResponse)
    {
        pResponse->CloseDone(HXR_OK);
    }
    // Release the response interface
    HX_RELEASE(pResponse);
    // Release the request
    HX_RELEASE(m_pRequest);

    return HXR_OK;
}

STDMETHODIMP CHXSingleBufferFileObject::Read(ULONG32 ulCount)
{
    HXLOGL4(HXLOG_FLVR, "CHXSingleBufferFileObject[%p]::Read(%lu)", this, ulCount);
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pBuffer && m_pFileResponse)
    {
        // Set the return value
        retVal = HXR_INVALID_PARAMETER;
        // Make sure we don't get any requests for 0 bytes
        if (ulCount)
        {
            // Set the return value
            retVal = HXR_OK;
            // Is the file offset less than the buffer size?
            if (m_ulFileOffset < m_pBuffer->GetSize())
            {
                // Compute the number of bytes to read
                UINT32 ulBytesToRead = ulCount;
                if (m_ulFileOffset + ulCount > m_pBuffer->GetSize())
                {
                    ulBytesToRead = m_pBuffer->GetSize() - m_ulFileOffset;
                }
                // Construct a nested buffer of this size
                IHXBuffer* pReadBuffer = NULL;
                retVal = CHXNestedBuffer::CreateNestedBuffer(m_pBuffer, m_ulFileOffset,
                                                             ulBytesToRead, pReadBuffer);
                if (SUCCEEDED(retVal))
                {
                    // Update the file offset
                    m_ulFileOffset += ulBytesToRead;
                    // Call back to the response interface
                    HXLOGL4(HXLOG_FLVR, "\tcalling IHXFileResponse::ReadDone(HXR_OK,%p) bufferSize=%lu",
                            pReadBuffer, (pReadBuffer ? pReadBuffer->GetSize() : 0));
                    m_pFileResponse->ReadDone(HXR_OK, pReadBuffer);
                }
                HX_RELEASE(pReadBuffer);
            }
            else
            {
                // We have no more bytes to read, so call back
                // with a failure to ReadDone()
                HXLOGL4(HXLOG_FLVR, "\tcalling IHXFileResponse::ReadDone(HXR_FAIL,NULL)");
                m_pFileResponse->ReadDone(HXR_FAIL, NULL);
            }
        }
    }

    return retVal;
}

STDMETHODIMP CHXSingleBufferFileObject::Write(IHXBuffer* pBuffer)
{
    HX_ASSERT(FALSE && "Write() not supported on CHXSingleBufferFileObject");
    return HXR_NOTIMPL;
}

STDMETHODIMP CHXSingleBufferFileObject::Seek(ULONG32 ulOffset, HXBOOL bRelative)
{
    HXLOGL4(HXLOG_FLVR, "CHXSingleBufferFileObject[%p]::Seek(offset=%lu,bRelative=%lu)", this, ulOffset, bRelative);
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pBuffer && m_pFileResponse)
    {
        // Set the return value
        retVal = HXR_INVALID_PARAMETER;
        // Compute the requested offset
        UINT32 ulNewOffset = ulOffset;
        if (bRelative)
        {
            ulNewOffset = m_ulFileOffset + ulOffset;
        }
        // We will allow seeks to the very end of the file, but not beyond
        if (ulNewOffset > m_pBuffer->GetSize())
        {
            // Clear the return value
            retVal = HXR_OK;
            // Set the file offset
            m_ulFileOffset = ulNewOffset;
            // Call back to the response interface
            HXLOGL4(HXLOG_FLVR, "\tcalling IHXFileResponse::SeekDone(HXR_OK)");
            m_pFileResponse->SeekDone(HXR_OK);
        }
    }

    return retVal;
}

STDMETHODIMP CHXSingleBufferFileObject::Advise(ULONG32 ulInfo)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    const char* pszInfo = "Unknown";
    if (m_pBuffer && m_pFileResponse)
    {
        // Set the return value
        retVal = HXR_INVALID_PARAMETER;
        // Switch on advise info
        switch (ulInfo)
        {
            case HX_FILEADVISE_RANDOMACCESS:     pszInfo = "HX_FILEADVISE_RANDOMACCESS";     retVal = HXR_OK;   break;
            case HX_FILEADVISE_SYNCACCESS:       pszInfo = "HX_FILEADVISE_SYNCACCESS";       retVal = HXR_OK;   break;
            case HX_FILEADVISE_RANDOMACCESSONLY: pszInfo = "HX_FILEADVISE_RANDOMACCESSONLY"; retVal = HXR_OK;   break;
            case HX_FILEADVISE_ANYACCESS:        pszInfo = "HX_FILEADVISE_ANYACCESS";        retVal = HXR_OK;   break;
            case HX_FILEADVISE_FASTSTART:        pszInfo = "HX_FILEADVISE_FASTSTART";        retVal = HXR_OK;   break;
            case HX_FILEADVISE_ASYNCACCESS:      pszInfo = "HX_FILEADVISE_ASYNCACCESS";      retVal = HXR_FAIL; break;
            case HX_FILEADVISE_NETWORKACCESS:    pszInfo = "HX_FILEADVISE_NETWORKACCESS";    retVal = HXR_FAIL; break;
        }
    }

    HXLOGL4(HXLOG_FLVR, "CHXSingleBufferFileObject[%p]::Advise(%s) returns 0x%08x", this, pszInfo, retVal);

    return retVal;
}

STDMETHODIMP CHXSingleBufferFileObject::Stat(IHXFileStatResponse* pFileStatResponse)
{
    HXLOGL4(HXLOG_FLVR, "CHXSingleBufferFileObject[%p]::Stat(pResponse=%p)", this, pFileStatResponse);
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pBuffer)
    {
        // Set the return value
        retVal = HXR_INVALID_PARAMETER;
        // Make sure we are given a response interface
        if (pFileStatResponse)
        {
            // Clear the return value
            retVal = HXR_OK;
            // Call back to the response interface
            HXLOGL4(HXLOG_FLVR, "\tcalling StatDone(size=%lu,0,0,0,0)", m_pBuffer->GetSize());
            pFileStatResponse->StatDone(HXR_OK, m_pBuffer->GetSize(), 0, 0, 0, 0);
        }
    }

    return retVal;
}

STDMETHODIMP CHXSingleBufferFileObject::SetRequest(IHXRequest* pRequest)
{
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    if (pRequest)
    {
        // Save the request
        HX_RELEASE(m_pRequest);
        m_pRequest = pRequest;
        m_pRequest->AddRef();
        // Clear the return value
        retVal = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP CHXSingleBufferFileObject::GetRequest(REF(IHXRequest*) rpRequest)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pRequest)
    {
        // Assign the out parameter
        rpRequest = m_pRequest;
        rpRequest->AddRef();
        // Clear the return value
        retVal = HXR_OK;
    }

    return retVal;
}

HX_RESULT CHXSingleBufferFileObject::CreateFileObject(IHXBuffer* pBuffer, REF(IHXFileObject*) rpFileObject)
{
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    // Make sure that the out parameter is initially NULL
    HX_ASSERT(rpFileObject == NULL);
    // Make sure we have been given a buffer
    if (pBuffer && pBuffer->GetSize() > 0)
    {
        // Set the return value
        retVal = HXR_OUTOFMEMORY;
        // Create a CHXSingleBufferFileObject object
        CHXSingleBufferFileObject* pObj = new CHXSingleBufferFileObject(pBuffer);
        if (pObj)
        {
            // QI this object for IHXFileObject
            retVal = pObj->QueryInterface(IID_IHXFileObject, (void**) &rpFileObject);
        }
    }

    return retVal;
}

CHXSingleBufferFileObject::CHXSingleBufferFileObject(IHXBuffer* pBuffer)
    : m_lRefCount(0)
    , m_pBuffer(pBuffer)
    , m_pFileResponse(NULL)
    , m_pRequest(NULL)
    , m_ulFileOffset(0)
{
    HX_ADDREF(m_pBuffer);
}

CHXSingleBufferFileObject::~CHXSingleBufferFileObject()
{
    Close();
}


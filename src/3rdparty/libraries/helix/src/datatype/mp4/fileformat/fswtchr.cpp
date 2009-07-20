/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

/****************************************************************************
 *  Includes
 */
#include "fswtchr.h"
#include "hxassert.h"
#include "qtffrefcounter.h"
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
#include "hxprdnld.h"       // IHXMediaBytesToMediaDur, IHXPDStatusMgr
#endif /* #if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS) */



/****************************************************************************
 *  Local Defines
 */
#define MAX_CHUNK_SIZE			0x0000FFFF  // 64K
#define MAX_CONSECUTIVE_FRAGMENT_LOADS	8	    // Do not set to 0
#define INITIAL_READ_ALL_BUFFER_SIZE	0x00100000  // 1M
#define MAX_CONTIGUOUS_READ_SIZE 0x03200000 // 50M


/****************************************************************************
 *  Class CFileSwitcher
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CFileSwitcher::CFileSwitcher(void)
    : m_uMaxChildCount(0)
    , m_uCurrentChildCount(0)
    , m_pHandleTable(NULL)
    , m_ulFlags(0)
    , m_ulSize(0)
    , m_ulChunkSize(0)
    , m_ulProcessedSize(0)
    , m_ulFragmentCount(0)
    , m_bRelative(FALSE)
    , m_pBuffer(NULL)
    , m_pClassFactory(NULL)
    , m_pCurrentHandle(NULL)
    , m_pResponse(NULL)
    , m_pScheduler(NULL)
    , m_State(FSWCHR_Offline)
    , m_lRefCount(0)
    , m_uLastDisownedChild(0)
    , m_bClosing(FALSE)
    , m_CloseStatus(HXR_OK)
    , m_bSyncMode(FALSE)
{
    g_nRefCount_qtff++;
}


CFileSwitcher::~CFileSwitcher()
{
    Reset();
    g_nRefCount_qtff--;
}


/****************************************************************************
 *  IHXFileSwitcher private methods
 */
/****************************************************************************
 *  Reset
 */
inline void CFileSwitcher::Reset(void)
{
    HX_VECTOR_DELETE(m_pHandleTable);
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pBuffer);
    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pScheduler);
    m_uCurrentChildCount = 0;
}

/****************************************************************************
 *  HandleFailureAsync
 */
HX_RESULT CFileSwitcher::HandleFailureAsync(HX_RESULT status)
{
    // Asynchronous failures occur only during Read, Write or Seek
    // process while a new FileHandle is beeing formed (since the
    // operation was requested on a file without a handle present
    // in tha HandleTable).
    HX_RESULT retVal;

    HX_ASSERT(status != HXR_OK);
    HX_ASSERT(m_pCurrentHandle);

    HX_RELEASE(m_pBuffer);

    if (m_pCurrentHandle != m_pHandleTable)
    {
	m_pCurrentHandle->Clear();
	m_uCurrentChildCount--;
    }

    switch (m_State)
    {
    case FSWCHR_ProcRead:
	retVal = ReadDone(status, NULL);
	break;
    case FSWCHR_ProcSeek:
	retVal = SeekDone(status);
	break;
    case FSWCHR_ProcWrite:
	retVal = WriteDone(status);
	break;
    case FSWCHR_ProcAdvise:
	retVal = HXR_OK;    // Silent failure
	break;
    default:
	retVal = HXR_UNEXPECTED;
	break;
    }

    return retVal;
}

/****************************************************************************
 *  HandleFailureSync
 */
HX_RESULT CFileSwitcher::HandleFailureSync(HX_RESULT status)
{
    // Synchronous failures may occur during Read, Write, Seek
    // process while a correct FileHandle is preformed and known.
    // Sync. failure also may occur during initalization of this
    // object.
    HX_ASSERT(status != HXR_OK);

    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pBuffer);

    if (m_State == FSWCHR_ProcInit)
    {
	HX_VECTOR_DELETE(m_pHandleTable);
	m_State = FSWCHR_Offline;
    }
    else
    {
	m_State = FSWCHR_Ready;
    }

    return status;
}

/****************************************************************************
 *  FindFileHandle
 */
inline CFileSwitcher::FileHandle* CFileSwitcher::FindFileHandle(const char* pFileName)
{
    FileHandle* pFileHandle;
    FileHandle* pFreeHandle = NULL;
    UINT16 uChildrenExamined;
    
    if (pFileName == NULL)
    {
	return m_pHandleTable;
    }
    
    for (uChildrenExamined = 0, pFileHandle = m_pHandleTable + 1;
	 uChildrenExamined < m_uCurrentChildCount;
	 pFileHandle++)
    {
	if (pFileHandle->m_pFileName)
	{
	    if (!strcmp(pFileHandle->m_pFileName, pFileName))
	    {
		return pFileHandle;
	    }
	    
	    uChildrenExamined++;
	}
	else
	{
	    pFreeHandle = pFileHandle;
	}
    }
    
    if (m_State == FSWCHR_ProcAdvise)
    {
	// Do not page-in on attempt to Advise. Advise must be synchronous.
	return NULL;
    }

    // Appropriate file handle wasn't found, so find a
    // free one to make a new one
    pFileHandle = pFreeHandle;
    if (m_uCurrentChildCount < m_uMaxChildCount)
    {
	pFileHandle = m_pHandleTable + m_uCurrentChildCount + 1;
	
	HX_ASSERT(!pFileHandle->m_pFileName);
	HX_ASSERT(!pFileHandle->m_pFileObject);
    }

    // make new handle
    if (pFileHandle)
    {
	if (pFileHandle->SetName(pFileName))
	{
	    m_uCurrentChildCount++;
	}
	else
	{
	    pFileHandle = NULL;
	}
    }

    return pFileHandle;
}

/****************************************************************************
 *  SelectStaleHandle
 */
inline CFileSwitcher::FileHandle* CFileSwitcher::SelectStaleHandle(void)
{
    FileHandle *pFileHandle = NULL;
    
    // Getting a stale handle is only valid when
    // there is no more room for the child handles
    HX_ASSERT(m_uMaxChildCount == m_uCurrentChildCount);

    if (m_uMaxChildCount > 0)
    {
	if (m_uLastDisownedChild < m_uMaxChildCount)
	{
	    m_uLastDisownedChild++;
	}
	else
	{
	    m_uLastDisownedChild = 1;
	}
	
	pFileHandle = m_pHandleTable + m_uLastDisownedChild;

	HX_ASSERT(pFileHandle->m_pFileName);
    }

    return pFileHandle;
}

/****************************************************************************
 *  GetFileHandle
 */
HX_RESULT CFileSwitcher::GetFileHandle(const char* pFileName)
{
    HX_RESULT retVal;

    m_pCurrentHandle = FindFileHandle(pFileName);

    if (m_pCurrentHandle)
    {
	if (m_pCurrentHandle->m_pFileObject)
	{
	    // A set-up handle exists
	    retVal = FileHandleReady();
	}
	else
	{
	    IHXGetFileFromSamePool* pFilePool;

	    // Handle was made but needs set-up
	    HX_ASSERT(m_pHandleTable->m_pFileObject);

	    retVal = m_pHandleTable->m_pFileObject->QueryInterface(
			IID_IHXGetFileFromSamePool,
			(void**) &pFilePool);

	    if (SUCCEEDED(retVal))
	    {
		// Set-up is made by duplicating the root file object
		retVal = pFilePool->GetFileObjectFromPool(
			    (IHXGetFileFromSamePoolResponse*) this);
		pFilePool->Release();
	    }
	}
    }
    else if (m_State == FSWCHR_ProcAdvise)
    {
	// Cannot advise paged-out file object
	retVal = HXR_FAIL;
    }
    else
    {
	// If the file handle table is full, select a stale
	// handle and dispose of it so the room is made for
	// the currently needed handle,
	if (m_uMaxChildCount == m_uCurrentChildCount)
	{
	    m_pCurrentHandle = SelectStaleHandle();

	    if (m_pCurrentHandle)
	    {
		m_pCurrentHandle->SetName(pFileName);

		retVal = CloseFileHandleObject(m_pCurrentHandle);
	    }
	    else
	    {
		retVal = HXR_FAIL;
	    }
	}
	else
	{
	    // We must be out memory if we failed to find or
	    // make a file handle and the file handle table
	    // still has slots available.
	    retVal = HXR_OUTOFMEMORY;
	}
    }

    if (FAILED(retVal))
    {
	retVal = HandleFailureSync(retVal);
    }

    return retVal;
}

/****************************************************************************
 *  CloseFileHandle
 */
HX_RESULT CFileSwitcher::CloseFileHandleObject(FileHandle *pFileHandle)
{
    HX_RESULT retVal;
    IHXFileObject *pFileObject;

    HX_ASSERT(pFileHandle);

    pFileObject = pFileHandle->m_pFileObject;

    if (pFileObject)
    {
	pFileHandle->m_pFileObject = NULL;

	retVal = pFileObject->Close();

	pFileObject->Release();	
    }
    else
    {
	CloseDone(HXR_OK);
	retVal = HXR_OK;
    }

    return retVal;
}

/****************************************************************************
 *  FileHandleReady
 */
HX_RESULT CFileSwitcher::FileHandleReady(void)
{
    HX_RESULT retVal;

    HX_ASSERT(m_pCurrentHandle);
    HX_ASSERT(m_pCurrentHandle->m_pFileObject);

    switch (m_State)
    {
    case FSWCHR_ProcRead:
	m_ulProcessedSize = 0;
	return ProcessRead();
    case FSWCHR_ProcSeek:
	return m_pCurrentHandle->m_pFileObject->Seek(m_ulSize, m_bRelative);
    case FSWCHR_ProcWrite:
	{
	    IHXBuffer* pBuffer;
	    HX_RESULT retVal;
	    
	    HX_ASSERT(m_pBuffer);

	    HX_RELEASE(m_pBuffer);
	    pBuffer = m_pBuffer;
	    m_pBuffer = NULL;
	    
	    retVal = m_pCurrentHandle->m_pFileObject->Write(pBuffer);
	    
	    pBuffer->Release();
	    
	    return retVal;
	}
    case FSWCHR_ProcAdvise:
	retVal = m_pCurrentHandle->m_pFileObject->Advise(m_ulSize);
	if (SUCCEEDED(retVal))
	{
	    if (m_ulSize == HX_FILEADVISE_SYNCACCESS)
	    {
		m_bSyncMode = TRUE;
	    }
	    else if (m_ulSize == HX_FILEADVISE_ASYNCACCESS)
	    {
		m_bSyncMode = FALSE;
	    }
	}
	m_State = FSWCHR_Ready;
	return retVal;
    default:
	// nothing to do
	break;
    }

    return HXR_UNEXPECTED;
}

/****************************************************************************
 *  ProcessRead
 */
HX_RESULT CFileSwitcher::ProcessRead(void)
{
    ULONG32 ulSize = m_ulSize - m_ulProcessedSize;
    m_ulChunkSize = (ulSize <= MAX_CHUNK_SIZE) ? 
		     ulSize : MAX_CHUNK_SIZE;

    return m_pCurrentHandle->m_pFileObject->Read(m_ulChunkSize);
}

/****************************************************************************
 *  ReadNextFragment
 */
HX_RESULT CFileSwitcher::ReadNextFragment(void)
{
    HX_RESULT retVal = HXR_FAIL;

    if ((!m_bClosing) &&
	m_pCurrentHandle &&
	m_pCurrentHandle->m_pFileObject)
    {
	if (m_bSyncMode ||
	    (m_ulFragmentCount < MAX_CONSECUTIVE_FRAGMENT_LOADS))
	{
	    retVal = ProcessRead();
	}
	else
	{
	    m_ulFragmentCount = 0;
	    retVal = ((m_pScheduler->RelativeEnter(this, 0) == NULL) ? 
		      HXR_FAIL : 
		      HXR_OK);
	}
    }

    if (FAILED(retVal))
    {
	retVal = ReadDone(HXR_FAIL, NULL);
    }

    return retVal;
}

/****************************************************************************
 *  IHXFileSwitcher methods - Main interface
 */
/****************************************************************************
 *  Init
 */
STDMETHODIMP CFileSwitcher::Init(IHXFileObject* pFileObject,
				 ULONG32 ulFlags,
				 IHXFileResponse* pResponse,
				 IUnknown* pContext,
				 UINT16 uMaxChildCount)
{
    HX_RESULT retVal = HXR_OK;

    if (m_State != FSWCHR_Offline)
    {
	retVal = HXR_UNEXPECTED;
    }

    if (SUCCEEDED(retVal))
    {
	Reset();

	HX_ASSERT(pFileObject);
	HX_ASSERT(pResponse);
	HX_ASSERT(pContext);

	m_uMaxChildCount = uMaxChildCount;
	m_ulFlags = ulFlags;
    
	// First Entry in the table is used as RootHandle
	m_pHandleTable = new FileHandle[m_uMaxChildCount + 1];
	if (m_pHandleTable == NULL)
	{
	    retVal = HXR_OUTOFMEMORY;
	}
    }

    if (SUCCEEDED(retVal))
    {
	retVal = pContext->QueryInterface(
	    IID_IHXCommonClassFactory,
	    (void**) &m_pClassFactory);
    }

    if (SUCCEEDED(retVal))
    {
	retVal = pContext->QueryInterface(
	    IID_IHXScheduler, 
	    (void**) &m_pScheduler);
    }

    if (SUCCEEDED(retVal))
    {
	HX_ASSERT(!m_pResponse);

	m_pHandleTable = m_pHandleTable;
	m_pHandleTable->m_pFileObject = pFileObject;
	pFileObject->AddRef();

	m_State = FSWCHR_ProcInit;
	m_pResponse = pResponse;
	m_pResponse->AddRef();

	retVal = pFileObject->Init(m_ulFlags, (IHXFileResponse*) this);

	if (FAILED(retVal))
	{
	    retVal = HandleFailureSync(retVal);
	}
    }

    return retVal;
}

/****************************************************************************
 *  Read
 */
STDMETHODIMP CFileSwitcher::Read(ULONG32 ulSize,
				 IHXFileResponse* pResponse,
				 const char* pFileName)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if ((m_State == FSWCHR_Ready) && (!m_bClosing) && (ulSize < MAX_CONTIGUOUS_READ_SIZE))
    {
	HX_ASSERT(!m_pResponse);

	m_ulSize = ulSize;
	m_pResponse = pResponse;
	m_pResponse->AddRef();

	m_State = FSWCHR_ProcRead;

	retVal = GetFileHandle(pFileName);
    }

    return retVal;	
}

/****************************************************************************
 *  Write
 */
STDMETHODIMP CFileSwitcher::Write(IHXBuffer* pBuffer,
				  IHXFileResponse* pResponse,
				  const char* pFileName)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if ((m_State == FSWCHR_Ready) && (!m_bClosing))
    {
	HX_ASSERT(!m_pResponse);
	HX_ASSERT(!m_pBuffer);

	m_pBuffer = pBuffer;
	m_pBuffer->AddRef();
	m_pResponse = pResponse;
	m_pResponse->AddRef();

	m_State = FSWCHR_ProcWrite;

	retVal = GetFileHandle(pFileName);
    }

    return retVal;
}

/****************************************************************************
 *  Seek
 */
STDMETHODIMP CFileSwitcher::Seek(ULONG32 ulOffset,
				 HXBOOL bRelative,
				 IHXFileResponse* pResponse,
				 const char* pFileName)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if ((m_State == FSWCHR_Ready) && (!m_bClosing))
    {
	HX_ASSERT(!m_pResponse);

	m_ulSize = ulOffset;
	m_bRelative = bRelative;
	m_pResponse = pResponse;
	m_pResponse->AddRef();

	m_State = FSWCHR_ProcSeek;

	retVal = GetFileHandle(pFileName);
    }

    return retVal;
}

/****************************************************************************
 *  Advise
 */
STDMETHODIMP CFileSwitcher::Advise(ULONG32 ulInfo,
				   const char* pFileName)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if ((m_State == FSWCHR_Ready) && (!m_bClosing))
    {
	HX_ASSERT(!m_pResponse);

	m_ulSize = ulInfo;

	m_State = FSWCHR_ProcAdvise;

	retVal = GetFileHandle(pFileName);
    }

    return retVal;
}

/****************************************************************************
 *  Close
 */
STDMETHODIMP CFileSwitcher::Close(IHXFileResponse *pResponse,
				  const char* pFileName)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if ((m_State != FSWCHR_Offline) && (!m_bClosing))
    {
	if (pFileName)
	{
	    // File Specific Closing - must be synchronous
	    if (m_State == FSWCHR_Ready)
	    {
		FileHandle *pFileHandle;
	
		HX_ASSERT(!m_pResponse);
		m_pResponse = pResponse;
		pResponse->AddRef();

		retVal = HXR_OK;
		m_State = FSWCHR_ProcClose;
		
		pFileHandle = FindFileHandle(pFileName);
		
		if (pFileHandle)
		{
		    if (pFileHandle->m_pFileName)
		    {
			pFileHandle->SetName(NULL);

			HX_ASSERT(m_uCurrentChildCount > 0);

			m_uCurrentChildCount--;
		    }
		    
		    retVal = CloseFileHandleObject(pFileHandle);
		}
		else
		{
		    CloseDone(HXR_OK);
		}
	    }
	}
	else
	{
	    // This is a general closure (shutdown)
	    // which need not be synchronous
	    UINT16 i;
	    FileHandle* pFileHandle;
	    
	    HX_ASSERT(m_pHandleTable);
	    
	    retVal = HXR_OK;

	    m_bClosing = TRUE;
	    m_CloseStatus = HXR_OK;

	    HX_RELEASE(m_pResponse);
	    m_pResponse = pResponse;
	    pResponse->AddRef();
	    
	    if (m_State == FSWCHR_ProcClose)
	    {
		// there is an additional outstanding Close to complete 
		// for a child
		m_uCurrentChildCount++;
	    }
	    
	    // Close Children
	    HX_ASSERT(m_uCurrentChildCount <= m_uMaxChildCount);
	    
	    for (i = m_uCurrentChildCount, pFileHandle = m_pHandleTable + 1;
		 i > 0;
		 i--, pFileHandle++)
	    {
		if (pFileHandle->m_pFileName)
		{
		    pFileHandle->SetName(NULL);
		    
		    if (FAILED(CloseFileHandleObject(pFileHandle)))
		    {
			// Recover by forcing the closing on this level
			CloseDone(HXR_OK);
		    }
		}
	    }

	    // Close the root file
	    if (FAILED(CloseFileHandleObject(m_pHandleTable)))
	    {
		// Recover by forcing the closing on this level
		CloseDone(HXR_OK);
	    }
	}
    }

    return retVal;
}


/****************************************************************************
 *  IHXFileResponse methods
 */
/****************************************************************************
 *  InitDone
 */
STDMETHODIMP CFileSwitcher::InitDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_OK;

    if (m_State != FSWCHR_Offline)
    {
	if (m_State != FSWCHR_ProcInit)
	{
	    HX_ASSERT(m_pCurrentHandle);
	    HX_ASSERT(m_pCurrentHandle->m_pFileObject);

	    // This must be initialization completion
	    // as part of obtaining file handle during
	    // Read, Write or Seek
	    if (SUCCEEDED(status))
	    {
		status = FileHandleReady();
	    }

	    if (FAILED(status))
	    {
		m_pCurrentHandle->Clear();

		// Must report Failure to the requestor
		retVal = HandleFailureAsync(status);
	    }
	}
	else
	{
	    // Initialization complete
	    IHXFileResponse* pResponse = m_pResponse;

	    HX_ASSERT(pResponse);

	    m_pResponse = NULL;
	    if (SUCCEEDED(status))
	    {
		m_State = FSWCHR_Ready;
	    }

	    retVal = pResponse->InitDone(status);

	    pResponse->Release();
	}
    }
    else
    {
	retVal = HXR_UNEXPECTED;
    }

    return retVal;
}

/****************************************************************************
 *  CloseDone
 */
STDMETHODIMP CFileSwitcher::CloseDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_State != FSWCHR_Offline)
    {
	if (((m_State == FSWCHR_ProcRead) ||
	     (m_State == FSWCHR_ProcWrite) ||
	     (m_State == FSWCHR_ProcSeek) ||
	     (m_State == FSWCHR_ProcAdvise)) &&
	    (!m_bClosing))
	{
	    // It must be a close due to freeing of a stale
	    // file handle so we can create a new one
	    retVal = HXR_OK;

	    if (SUCCEEDED(status))
	    {
		IHXGetFileFromSamePool* pFilePool;
		
		// Handle was made but needs set-up
		HX_ASSERT(m_pHandleTable->m_pFileObject);
		
		status = m_pHandleTable->m_pFileObject->QueryInterface(
			    IID_IHXGetFileFromSamePool,
			    (void**) &pFilePool);
		
		if (SUCCEEDED(status))
		{
		    // Set-up is made by duplicating the root file object
		    status = pFilePool->GetFileObjectFromPool(
				(IHXGetFileFromSamePoolResponse*) this);
		    pFilePool->Release();
		}
	    }
	    
	    if (FAILED(status))
	    {
		retVal = HandleFailureAsync(status);
	    }
	}
	else if ((m_State == FSWCHR_ProcClose) ||
	         (m_bClosing))
	{
	    retVal = HXR_OK;

	    if (!m_bClosing || (m_uCurrentChildCount == 0))
	    {
		// Close completion
		IHXFileResponse *pResponse = m_pResponse;
				
		m_pResponse = NULL;

		if (m_bClosing)
		{
		    Reset();

		    m_State = FSWCHR_Offline;

		    if (FAILED(m_CloseStatus))
		    {
			status = m_CloseStatus;
		    }
		}
		else
		{
		    m_State = FSWCHR_Ready;
		}
		
		if (pResponse)
		{
		    retVal = pResponse->CloseDone(status);
		    pResponse->Release();
		}
	    }
	    else
	    {
		// Still Closing files
		m_uCurrentChildCount--;

		if (SUCCEEDED(m_CloseStatus))
		{
		    m_CloseStatus = status;
		}
	    }
	}
    }

    return retVal;
}

/****************************************************************************
 *  ReadDone
 */
STDMETHODIMP CFileSwitcher::ReadDone(HX_RESULT status,
				     IHXBuffer* pBuffer)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_State == FSWCHR_ProcRead)
    {
	HX_ASSERT(m_pResponse);
		
	if (m_ulChunkSize == m_ulSize)
	{
	    IHXFileResponse *pResponse = m_pResponse;
	    m_pResponse = NULL;
	    m_State = FSWCHR_Ready;
	    retVal = pResponse->ReadDone(status, pBuffer);
	    pResponse->Release();
	}
	else
	{
	    // The read of a fragment is completed
	    // Initialize fragment buffer if just starting
	    if (SUCCEEDED(status))
	    {
		if (m_ulProcessedSize == 0)
		{
		    // Starting the fragmented read - 
		    HX_ASSERT(m_pBuffer == NULL);
		    HX_ASSERT(m_ulSize != 0);
		    
		    m_ulFragmentCount = 0;

		    HX_RELEASE(m_pBuffer);
		    
		    status = m_pClassFactory->CreateInstance(
			IID_IHXBuffer, 
			(void **) &m_pBuffer);
		    
		    if (SUCCEEDED(status))
		    {
		      // cap the initial size to 1M. 
		      // it will grow in the else if block if the file size happens to be bigger than 1M 
		        UINT32 ulSize = m_ulSize;
			if ((ulSize == FSWTCHR_READ_ALL) && (ulSize > INITIAL_READ_ALL_BUFFER_SIZE)) 
			{
			    ulSize = INITIAL_READ_ALL_BUFFER_SIZE;
			}
			status = m_pBuffer->SetSize(ulSize);

			if (FAILED(status))
			{
			    HX_RELEASE(m_pBuffer);
			}
		    }
		}
		else if (m_ulProcessedSize + pBuffer->GetSize() > m_pBuffer->GetSize())
		{
		    // need to extend the buffer.
		    // make it double capping it requested size m_ulSize
		    UINT32 ulNewSize = m_pBuffer->GetSize();
		    ulNewSize = (ulNewSize & 0x10000000) == 0 ? (ulNewSize << 1) : 0xFFFFFFFF;
		    if (ulNewSize > m_ulSize)
		    {
		    	ulNewSize = m_ulSize;
		    }
			
		    IHXBuffer* pNewBuffer = NULL;
		    status = m_pClassFactory->CreateInstance(
			IID_IHXBuffer, 
			(void **) &pNewBuffer);
		    
		    if (SUCCEEDED(status))
		    {
			status = pNewBuffer->SetSize(ulNewSize);

			if (FAILED(status))
			{
			    HX_RELEASE(pNewBuffer);
			}
			else
			{
				::memcpy(pNewBuffer->GetBuffer(), m_pBuffer->GetBuffer(), m_ulProcessedSize);
				HX_RELEASE(m_pBuffer);
				m_pBuffer = pNewBuffer;
			}
		    }
		}
	    }

	    if (SUCCEEDED(status))
	    {
		ULONG32 ulSize = pBuffer->GetSize();

		memcpy(m_pBuffer->GetBuffer() + m_ulProcessedSize, /* Flawfinder: ignore */
		       pBuffer->GetBuffer(),
		       ulSize);
		m_ulProcessedSize += ulSize;
	    }

	    if ((m_ulProcessedSize >= m_ulSize) || FAILED(status))
	    {
		HX_ASSERT(m_ulProcessedSize <= m_ulSize);

		if (FAILED(status) && m_pBuffer)
		{
		    if (m_ulProcessedSize != 0)
		    {
			// Trim the buffer to what was read successfully
			m_pBuffer->SetSize(m_ulProcessedSize);
			status = HXR_OK;
		    }
		    else
		    {
			HX_RELEASE(m_pBuffer);
		    }
		}

		IHXBuffer* pBuffer = m_pBuffer;
		IHXFileResponse *pResponse = m_pResponse;
		m_pResponse = NULL;
		m_pBuffer = NULL;
		m_State = FSWCHR_Ready;
		retVal = pResponse->ReadDone(status, pBuffer);
		HX_RELEASE(pBuffer);
		pResponse->Release();
	    }
	    else
	    {
		m_ulFragmentCount++;
		retVal = ReadNextFragment();
	    }
	}
    }

    return retVal;
}

/****************************************************************************
 *  WriteDone
 */
STDMETHODIMP CFileSwitcher::WriteDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_State == FSWCHR_ProcWrite)
    {
	IHXFileResponse *pResponse = m_pResponse;
	
	HX_ASSERT(m_pResponse);
	
	m_pResponse = NULL;
	m_State = FSWCHR_Ready;
	
	retVal = pResponse->WriteDone(status);
	
	pResponse->Release();
    }

    return retVal;
}

/****************************************************************************
 *  SeekDone
 */
STDMETHODIMP CFileSwitcher::SeekDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_State == FSWCHR_ProcSeek)
    {
	IHXFileResponse *pResponse = m_pResponse;
	
	HX_ASSERT(m_pResponse);
	
	m_pResponse = NULL;
	m_State = FSWCHR_Ready;
	
	retVal = pResponse->SeekDone(status);
	
	pResponse->Release();
    }

    return retVal;
}

/****************************************************************************
 *  IHXCallback methods
 */
/****************************************************************************
 *  Func
 */
STDMETHODIMP CFileSwitcher::Func(void)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_State == FSWCHR_ProcRead)
    {
	retVal = ReadNextFragment();
    }

    return retVal;
}

/****************************************************************************
 *  IHXThreadSafeMethods method
 */
/****************************************************************************
 *  IsThreadSafe
 */
STDMETHODIMP_(UINT32)
CFileSwitcher::IsThreadSafe()
{
    return HX_THREADSAFE_METHOD_FSR_READDONE;
}


/****************************************************************************
 *  IHXGetFileFromSamePoolResponse method
 */
/****************************************************************************
 *  FileObjectReady
 */
STDMETHODIMP CFileSwitcher::FileObjectReady(HX_RESULT status,
					    IUnknown* pFileObject)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_State != FSWCHR_Offline)
    {
	HX_ASSERT(m_pCurrentHandle);
	HX_ASSERT(m_pCurrentHandle->m_pFileName);
	HX_ASSERT(!m_pCurrentHandle->m_pFileObject);
	
	if (SUCCEEDED(status))
	{
	    IHXRequestHandler* pRequestHandler = NULL;
	    IHXRequest* pRequest = NULL;

	    if (!m_bClosing)
	    {
		status = pFileObject->QueryInterface(
			    IID_IHXFileObject, 
			    (void **) &(m_pCurrentHandle->m_pFileObject));
	    }
	    else
	    {
		status = HXR_ABORT;
	    }

	    if (status == HXR_OK)
	    {
		status = m_pClassFactory->CreateInstance(
			    IID_IHXRequest, 
			    (void **) &pRequest);
	    }
	    
	    if (status == HXR_OK)
	    {
		status = pRequest->SetURL(m_pCurrentHandle->m_pFileName);
	    }

	    if (status == HXR_OK)
	    {
		status = pFileObject->QueryInterface(
			    IID_IHXRequestHandler, 
			    (void**) &pRequestHandler);
	    }
	    
	    if (status == HXR_OK)
	    {
		status = pRequestHandler->SetRequest(pRequest);
	    }
		
	    HX_RELEASE(pRequest);
	    HX_RELEASE(pRequestHandler);
	    
	    if (status == HXR_OK)
	    {
		status = m_pCurrentHandle->m_pFileObject->Init
			    (m_ulFlags, 
			    (IHXFileResponse*) this);
	    }
	}
	
	if (status != HXR_OK)
	{
	    retVal = HandleFailureAsync(status);
	}
    }

    return retVal;
}


/****************************************************************************
 *  IUnknown methods
 */
/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//
STDMETHODIMP CFileSwitcher::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXFileSwitcher))
    {
	AddRef();
	*ppvObj = (IHXFileSwitcher*) this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXCallback))
    {
        AddRef();
        *ppvObj = (IHXCallback*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXGetFileFromSamePoolResponse))
    {
	AddRef();
	*ppvObj = (IHXGetFileFromSamePoolResponse*) this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileResponse))
    {
	AddRef();
	*ppvObj = (IHXFileResponse*) this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXThreadSafeMethods))
    {
	AddRef();
	*ppvObj = (IHXThreadSafeMethods*) this;
	return HXR_OK;
    }

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    else if (IsEqualIID(riid, IID_IHXMediaBytesToMediaDur))
    {
        if (m_pResponse)
        {
	    return m_pResponse->QueryInterface(IID_IHXMediaBytesToMediaDur,
                    ppvObj);
        }
    }
    else if (IsEqualIID(riid, IID_IHXPDStatusMgr))
    {
        if (m_pResponse)
        {
	    return m_pResponse->QueryInterface(IID_IHXPDStatusMgr,
                    ppvObj);
        }
    }
#endif /* #if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS). */

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//
STDMETHODIMP_(ULONG32) CFileSwitcher::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//
STDMETHODIMP_(ULONG32) CFileSwitcher::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

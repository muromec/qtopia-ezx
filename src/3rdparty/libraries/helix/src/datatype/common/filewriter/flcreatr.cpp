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
 *  Defines
 */
#define MAX_RENAME_TRY_COUNT		    8
#define RENAME_REDUCTION_AFTER_TRY	    3
#define RENAME_REDUCTION_LENGTH		    32


/****************************************************************************
 *  Includes
 */
#include <stdio.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxstrutl.h"
#include "hxslist.h"
#include "hxbuffer.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "pckunpck.h"
#include "hxfiles.h"
#include "hxerror.h"
#include "hxsrc.h"
#include "hxfwrtr.h"
#include "timeval.h"

#include "flcreatr.h"

// 
//  Class: CFileCreator
//
CFileCreator::CFileCreator(IHXFileCreatorResponse* pOwner, 
			   IUnknown* pContext,
			   HXBOOL bBlastFiles)
    : m_lRefCount	    (0)
    , m_FCStatus	    (FC_INITIALIZING)
    , m_pOwner		    (pOwner)
    , m_pContext	    (pContext)
    , m_pErrorMessages	    (NULL)
    , m_pClassFactory	    (NULL)
    , m_pFSManager	    (NULL)
    , m_pFSMResponse	    (NULL)
    , m_pFileObject	    (NULL)
    , m_pDirHandler	    (NULL)
    , m_bFileSystemReady    (FALSE)
    , m_bBlastFiles	    (bBlastFiles)
{
    HX_ASSERT(m_pOwner);
    m_pOwner->AddRef();

    HX_ASSERT(pContext);
    m_pContext->AddRef();
}

CFileCreator::~CFileCreator()
{
    HX_RELEASE(m_pOwner);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pErrorMessages);
    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pFSManager);
    HX_RELEASE(m_pFSMResponse);
    HX_RELEASE(m_pFileObject);

    if (m_pDirHandler)
    {
	m_pDirHandler->CloseDirHandler();
	HX_RELEASE(m_pDirHandler);
    }
}


/*** IUnknown methods ***/

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP
CFileCreator::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileStatResponse))
    {
        AddRef();
        *ppvObj = (IHXFileStatResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXDirHandlerResponse))
    {
        AddRef();
        *ppvObj = (IHXDirHandlerResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32)
CFileCreator::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32)
CFileCreator::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


HX_RESULT
CFileCreator::Init()
{
    HX_RESULT hResult = HXR_OK;

    hResult = m_pContext->QueryInterface(IID_IHXErrorMessages,
					 (void**)&m_pErrorMessages);
    if (HXR_OK != hResult)
    {
	return hResult;
    }

    hResult = m_pContext->QueryInterface(IID_IHXCommonClassFactory,
					 (void**)&m_pClassFactory);
    if (HXR_OK != hResult)
    {
	return hResult;
    }

    hResult = m_pClassFactory->CreateInstance(CLSID_IHXFileSystemManager, 
					      (void**)&m_pFSManager);
    if (HXR_OK != hResult)
    {
	return hResult;
    }

    // Create and Initialize the File System Manager so we can create files
    HX_ASSERT(!m_pFSMResponse);
    m_pFSMResponse = new FSMResponse(this);
    m_pFSMResponse->AddRef();
    m_pFSManager->Init((IHXFileSystemManagerResponse*)m_pFSMResponse);

    return hResult;
}

HX_RESULT
CFileCreator::Done()
{
    HX_RELEASE(m_pOwner);
    HX_RELEASE(m_pFSManager);
    HX_RELEASE(m_pFSMResponse);

    return HXR_OK;
}

HX_RESULT
CFileCreator::CreateArchiveDir(const char* pName, HXBOOL bResolveConflict)
{
    HX_RESULT	 hResult  = HXR_OK;
    IHXRequest* pRequest = NULL;

    // Make sure we are ready to create a new object
    HX_ASSERT(m_FCStatus == FC_READY);
    m_FCStatus = FC_CREATING_DIR;
    m_bResolveConflict = bResolveConflict;

    // Clean up any leftover file object
    HX_RELEASE(m_pFileObject);

    // Create an IHXRequest object to send with our request
    hResult = m_pClassFactory->CreateInstance(CLSID_IHXRequest, 
					      (void**)&pRequest);
    HX_ASSERT(HXR_OK == hResult);

    // Set the requested directory name
    pRequest->SetURL(pName);

    // Actually request the new file object
    hResult = m_pFSManager->GetNewFileObject(pRequest, 0);

    /* Flow continues in CFileCreator::FileObjectReady()... */
    if (FAILED(hResult))
    {
	m_FCStatus = FC_READY;
    }

    HX_RELEASE(pRequest);

    return hResult;
}

HX_RESULT
CFileCreator::CreateArchiveFile(const char* pName, HXBOOL bResolveConflict)
{
    HX_RESULT	 hResult  = HXR_OK;
    IHXRequest* pRequest = NULL;

    // Make sure we are ready to create a new object
    HX_ASSERT(m_FCStatus == FC_READY);
    m_FCStatus = FC_CREATING_FILE;
    m_bResolveConflict = bResolveConflict;

    // Clean up any leftover file object
    HX_RELEASE(m_pFileObject);

    // Create an IHXRequest object to send with our request
    hResult = m_pClassFactory->CreateInstance(CLSID_IHXRequest, 
					      (void**)&pRequest);
    HX_ASSERT(HXR_OK == hResult);

    // Set the requested file name
    pRequest->SetURL(pName);

    // Actually request the new file object
    hResult = m_pFSManager->GetNewFileObject(pRequest, 0);

    /* Flow continues in CFileCreator::FileObjectReady()... */
    if (FAILED(hResult))
    {
	m_FCStatus = FC_READY;
    }

    HX_RELEASE(pRequest);

    return hResult;
}

HX_RESULT
CFileCreator::FindExistingFile(const char* pName)
{
    HX_RESULT	 hResult  = HXR_OK;
    IHXRequest* pRequest = NULL;

    // Make sure we are ready to create a new object
    HX_ASSERT(m_FCStatus == FC_READY);
    m_FCStatus = FC_FINDING_FILE;

    // Clean up any leftover file object
    HX_RELEASE(m_pFileObject);

    // Create an IHXRequest object to send with our request
    hResult = m_pClassFactory->CreateInstance(CLSID_IHXRequest, 
					      (void**)&pRequest);
    HX_ASSERT(HXR_OK == hResult);

    // Set the requested file name
    pRequest->SetURL(pName);

    // Actually request the new file object
    hResult = m_pFSManager->GetNewFileObject(pRequest, 0);

    /* Flow continues in CFileCreator::FileObjectReady()... */
    if (FAILED(hResult))
    {
	m_FCStatus = FC_READY;
    }

    HX_RELEASE(pRequest);

    return hResult;
}

HX_RESULT
CFileCreator::FileSystemReady(HX_RESULT status)
{
    if (HXR_OK == status)
    {
	m_bFileSystemReady = TRUE;
	m_FCStatus = FC_READY;
    }

    m_pOwner->InitDone(status);

    return HXR_OK;
}

HX_RESULT
CFileCreator::FileObjectReady(HX_RESULT status, IUnknown* pObject)
{
    HX_RESULT	  hResult   = HXR_OK;
    IHXFileStat* pFileStat = NULL;
    FC_STATUS	  FCStatus  = m_FCStatus;

    HX_ASSERT(m_FCStatus == FC_CREATING_DIR  ||
	      m_FCStatus == FC_CREATING_FILE ||
	      m_FCStatus == FC_FINDING_FILE);

    // The object could not be successfully created
    if (HXR_OK != status)
    {
	// Return failure to the calling object
	m_FCStatus = FC_READY;
	if (FCStatus == FC_CREATING_DIR)
	{
	    m_pOwner->ArchiveDirReady(status);
	}
	else if (FCStatus == FC_CREATING_FILE)
	{
	    m_pOwner->ArchiveFileReady(status, NULL);
	}
	else if (FCStatus == FC_FINDING_FILE)
	{
	    m_pOwner->ExistingFileReady(status, NULL);
	}
	return HXR_OK;
    }

    // Obtain interfaces that we will need shortly
    pObject->QueryInterface(IID_IHXFileObject, (void**)&m_pFileObject);
    HX_ASSERT(m_pFileObject);

    if ((m_FCStatus == FC_CREATING_FILE) && (!m_bResolveConflict))
    {
	// Since we are not resolving file conflicts, we do not need
	// to know whether the file exists or not - proceed as if
	// doesn't exist
	StatDone(HXR_FAIL, 0, 0, 0, 0, 0);
    }
    else
    {
	pObject->QueryInterface(IID_IHXFileStat, (void**)&pFileStat);
	HX_ASSERT(pFileStat);

	// Find out if a file or directory already exists with this name
	hResult = pFileStat->Stat((IHXFileStatResponse*)this);

	// If a file or directory does not already exist with that name,
	// the response function (StatDone) will never be called. I would
	// argue that this is a bug in IHXFileStat::Stat(), but now is
	// not a good time to change that behavior. So in order to keep all 
	// of the relevant logic together, I will make a fake StatDone() 
	// call in the failure case and handle both success AND failure in 
	// StatDone(). - DPS 1/5/1999
	if (HXR_OK != hResult)
	{
	    StatDone(HXR_FAIL, 0, 0, 0, 0, 0);
	}

	HX_RELEASE(pFileStat);
    }

    return HXR_OK;
}


// 
//  Class: CFileCreator::FSMResponse
//
CFileCreator::FSMResponse::FSMResponse(CFileCreator* pOwner)
  : m_lRefCount(0)
  , m_pOwner(pOwner)
{
    HX_ASSERT(pOwner);
    m_pOwner->AddRef();
}

CFileCreator::FSMResponse::~FSMResponse()
{
    HX_RELEASE(m_pOwner);
}


/*** IUnknown methods ***/

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP
CFileCreator::FSMResponse::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileSystemManagerResponse))
    {
        AddRef();
        *ppvObj = (IHXFileSystemManagerResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32)
CFileCreator::FSMResponse::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32)
CFileCreator::FSMResponse::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*** IHXFileSystemManagerResponse methods ***/

STDMETHODIMP
CFileCreator::FSMResponse::InitDone(HX_RESULT status)
{
    return m_pOwner->FileSystemReady(status);
}

STDMETHODIMP
CFileCreator::FSMResponse::FileObjectReady
(
    HX_RESULT status,
    IUnknown* pObject
)
{
    return m_pOwner->FileObjectReady(status, pObject);
}

STDMETHODIMP
CFileCreator::FSMResponse::DirObjectReady
(
    HX_RESULT status,
    IUnknown* pDirObject
)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CFileCreator::InitDirHandlerDone
(
    HX_RESULT status
)
{
    if (HXR_OK != status)
    {
	// We're now done with the dir handler
	m_pDirHandler->CloseDirHandler();
	HX_RELEASE(m_pDirHandler);

	m_FCStatus = FC_READY;
	m_pOwner->ArchiveDirReady(status);
    }
    else
    {
	if (HXR_OK != m_pDirHandler->MakeDir())
	{
	    // We're now done with the dir handler
	    m_pDirHandler->CloseDirHandler();
	    HX_RELEASE(m_pDirHandler);

	    m_FCStatus = FC_READY;
	    m_pOwner->ArchiveDirReady(status);
	}
    }

    return HXR_OK;
}

STDMETHODIMP
CFileCreator::CloseDirHandlerDone
(
    HX_RESULT status
)
{
    return HXR_OK;
}

STDMETHODIMP
CFileCreator::MakeDirDone
(
    HX_RESULT status
)
{
    // We're now done with the dir handler
    m_pDirHandler->CloseDirHandler();
    HX_RELEASE(m_pDirHandler);

    m_FCStatus = FC_READY;
    m_pOwner->ArchiveDirReady(status);

    return HXR_OK;
}

STDMETHODIMP
CFileCreator::ReadDirDone
(
    HX_RESULT status,
    IHXBuffer* pBuffer
)
{
    return HXR_UNEXPECTED;
}

STDMETHODIMP
CFileCreator::StatDone
(
    HX_RESULT status,
    UINT32 ulSize,
    UINT32 ulCreationTime,
    UINT32 ulAccessTime,
    UINT32 ulModificationTime,
    UINT32 ulMode
)
{
    HX_RESULT	    hResult = HXR_OK;
    IHXFileObject* pObject = NULL;
    
    pObject = m_pFileObject;
    pObject->AddRef();
    HX_RELEASE(m_pFileObject);

    // A file or directory with that name already exists
    if (HXR_OK == status)
    {
	if (m_FCStatus == FC_CREATING_DIR)
	{
	    if ((ulMode & HX_S_IFDIR) && (!m_bResolveConflict))
	    {
		// If we are creating a directory and the existing object
		// is a directory, just leave it alone and return success
		m_FCStatus = FC_READY;
		m_pOwner->ArchiveDirReady(HXR_OK);
	    }
	    else
	    {
		// If we are creating a directory and the existing object
		// is a file or a directory that must be moved out of the way,
		// rename it and create the directory
		hResult = RenameObject(pObject);
		if (HXR_OK != hResult)
		{
		    m_FCStatus = FC_READY;
		    if (ulMode & HX_S_IFDIR)
		    {
			// We failed to move the directory out of the way, but
			// we have a directory... proceeed without error.
			hResult = HXR_OK;
		    }
		    m_pOwner->ArchiveDirReady(hResult);
		}
		else
		{
		    // Create the directory
		    MakeDirectory(pObject);
		}
	    }
	}
	else if (m_FCStatus == FC_CREATING_FILE)
	{
	    // If we are creating a file, make sure we clear the way for its
	    // creation.
	    if ((!m_bBlastFiles) || (ulMode & HX_S_IFDIR))
	    {
		// If we are not blastiong awaythe existing files or the
		// existing object is a directory, rename the existing object
		hResult = RenameObject(pObject);
		m_FCStatus = FC_READY;
		m_pOwner->ArchiveFileReady(hResult, 
		    (HXR_OK == hResult) ? pObject : (IHXFileObject*)NULL);
	    }
	    else
	    {
		// The existing object must be a file and file creator is
		// configured to remove existing files.
		hResult = RemoveObject(pObject);
		m_FCStatus = FC_READY;
		m_pOwner->ArchiveFileReady(hResult, 
		    (HXR_OK == hResult) ? pObject : (IHXFileObject*)NULL);
	    }
	}
	else if (m_FCStatus == FC_FINDING_FILE)
	{
	    // If we are finding an existing file, we have succeeded
	    m_FCStatus = FC_READY;
	    m_pOwner->ExistingFileReady(HXR_OK, pObject);
	}
    }

    // A file or directory with that name does NOT exist
    else
    {
	if (m_FCStatus == FC_CREATING_DIR)
	{
	    // Create the directory
	    MakeDirectory(pObject);
	}
	else if (m_FCStatus == FC_CREATING_FILE)
	{
	    // If we are creating a file, simply return the file object
	    // to the caller since there is no conflict
	    m_FCStatus = FC_READY;
	    m_pOwner->ArchiveFileReady(HXR_OK, pObject);
	}
	else if (m_FCStatus == FC_FINDING_FILE)
	{
	    // If we are finding an existing file, we have failed
	    m_FCStatus = FC_READY;
	    m_pOwner->ExistingFileReady(HXR_FAIL, NULL);
	}
    }

    HX_RELEASE(pObject);

    return HXR_OK;
}

HX_RESULT	
CFileCreator::RemoveObject(IHXFileObject* pObject)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pObject)
    {
	IHXFileRemove* pRemover = NULL;

	retVal = pObject->QueryInterface(IID_IHXFileRemove, 
					 (void**) &pRemover);

	if (SUCCEEDED(retVal))
	{
	    retVal = pRemover->Remove();
	}

	HX_RELEASE(pRemover);

	if (FAILED(retVal))
	{
	    char buf[4096]; /* Flawfinder: ignore */
	    const char* pUnknFilename = "unknown name file";
	    const char* pFilename = pUnknFilename;

	    if (FAILED(pObject->GetFilename(pFilename)) || 
		(pFilename == NULL))
	    {
		pFilename = pUnknFilename;
	    }

	    SafeSprintf(buf, 4096, "Unable to remove \"%s\"!\n", pFilename);
	    m_pErrorMessages->Report(HXLOG_ERR, HXR_IGNORE, 
				     7, buf, NULL);
	}
    }

    return retVal;
}

HX_RESULT
CFileCreator::RenameObject(IHXFileObject* pObject,
			   const char* pNewFileName,
			   HXBOOL bPathChanged)
{
    HX_RESULT	    hResult	= HXR_OK;
    IHXFileRename* pFileRename = NULL;
    IHXFileMove*   pFileMove = NULL;
    const char*	    pFilename	= NULL;
    const char*	    pAction	= "rename";
    CHXString	    NewFilename;
    CHXString	    BaseFileName;
    Timeval	    tNow;
    char	    pExtension[80]; /* Flawfinder: ignore */
    ULONG32	    ulRenameAttemptCount = 0;
    ULONG32	    ulReductionAttemptCount = 0;
    ULONG32	    ulActionCode = 8;

    if (pObject)
    {
	pObject->QueryInterface(IID_IHXFileRename, (void**) &pFileRename);
	pObject->QueryInterface(IID_IHXFileMove, (void**) &pFileMove);

	// Get the existing file or directory name
	pObject->GetFilename(pFilename);

	if (pFilename)
	{
	    BaseFileName = pFilename;
	}
    }

    if (bPathChanged)
    {
	//
	// Move the file to new path and name
	//
	HX_ASSERT(pNewFileName);
	
	pAction = "move";
	ulActionCode = 9;
	
	hResult = HXR_FAIL;

	if (pFileMove && pNewFileName)
	{
	    NewFilename = pNewFileName;

	    hResult = pFileMove->Move(pNewFileName);
	}
    }
    else
    {
	hResult = HXR_FAIL;

	if (pFileRename && pNewFileName)
	{
	    NewFilename = pNewFileName;
	    
	    INT32 nIndex = NewFilename.ReverseFind('/');
	    if (nIndex != -1)
	    {
		nIndex++;
		NewFilename = NewFilename.Right(NewFilename.GetLength() - nIndex);
	    }

	    hResult = HXR_FAIL;

	    if (pFileRename)
	    {
		hResult = pFileRename->Rename((const char*) NewFilename);
	    }
	}
	else if (pFileRename && pFilename)
	{
	    //
	    // Rename the file or directory to name.date or user defined
	    // name.
	    //	    
	    if (pFileRename)
	    {
		HXBOOL bAutoGenerate = TRUE;
		IHXPropertyAdviser* pAdviser = NULL;
		
		// Get the current time and create the new filename
		gettimeofday(&tNow, 0);

		m_pOwner->QueryInterface(IID_IHXPropertyAdviser, 
					 (void**) &pAdviser);
		do
		{
		    bAutoGenerate = TRUE;

		    if (pAdviser)
		    {
			IHXBuffer* pBaseNameBuffer = NULL;
			IHXBuffer* pTargetNameBuffer = NULL;

			hResult = CreateBufferCCF(pBaseNameBuffer, m_pContext);
			if (SUCCEEDED(hResult))
			{	
			    hResult = pBaseNameBuffer->
				SetSize(BaseFileName.GetLength() + 1);
			}
			
			if (SUCCEEDED(hResult))
			{	
			    strcpy((char*) pBaseNameBuffer->GetBuffer(), /* Flawfinder: ignore */
				   (const char*) BaseFileName);
			    pTargetNameBuffer = (IHXBuffer*) pBaseNameBuffer;
			}
			
			if (SUCCEEDED(pAdviser->GetPropertyCString(
					"CollisionFileName",
					pTargetNameBuffer)))
			{
			    if (pTargetNameBuffer &&
				((pTargetNameBuffer != pBaseNameBuffer) ||
				 strcmp((const char*) pTargetNameBuffer->GetBuffer(),
				        (const char*) pBaseNameBuffer->GetBuffer())))
			    {
				NewFilename = ((const char*) 
					       pTargetNameBuffer->GetBuffer());
				bAutoGenerate = FALSE;
				if (pTargetNameBuffer != pBaseNameBuffer)
				{
				    HX_RELEASE(pTargetNameBuffer);
				}
				pTargetNameBuffer = NULL;
			    }
			}
						    
			HX_RELEASE(pBaseNameBuffer);
		    }

		    if (bAutoGenerate)
		    {
			if (ulRenameAttemptCount == 0)
			{
			    sprintf(pExtension, ".%lu", tNow.tv_sec); /* Flawfinder: ignore */
			}
			else
			{
			    sprintf(pExtension, ".%lu_%u", tNow.tv_sec, tNow.tv_usec % 1000); /* Flawfinder: ignore */
			}
		    
			NewFilename = BaseFileName;
			ULONG32 ulExtLength = strlen(pExtension);
			ULONG32 ulNewLength = NewFilename.GetLength() + 
					      ulExtLength;
			ULONG32 ulMaxLength = ((ULONG32) FILENAME_MAX);

			// If the file name is very long or the rename attempt count reached
			// the specified threshold, a name reduction strategy is employed
			// additionally in attempt to produce a successful rename.
			if ((ulNewLength >= ulMaxLength) || 
			    (ulRenameAttemptCount >= RENAME_REDUCTION_AFTER_TRY))
			{
			    if (ulNewLength < ulMaxLength)
			    {
				ulMaxLength = ulNewLength;
			    }

			    if (ulReductionAttemptCount < 1)
			    {
				// Reduce by 2/3 or by (RENAME_REDUCTION_LENGTH + ulExtLength)
				// whichever is a smaller reduction.
				ulNewLength = ulMaxLength * 2 / 3 + 1;
				if ((ulNewLength + RENAME_REDUCTION_LENGTH + ulExtLength) <  
				    ulMaxLength)
				{
				    ulNewLength = ulMaxLength - 
						  RENAME_REDUCTION_LENGTH - 
						  ulExtLength;
				}
			    }
			    else
			    {
				ulNewLength = 
				    ((ulMaxLength >> ulReductionAttemptCount) + 1);
			    }

			    // Reduce size of the base part of the new file name
			    if (ulNewLength > ulExtLength)
			    {
				NewFilename.GetBufferSetLength(ulNewLength - ulExtLength);
			    }

			    NewFilename.GetBufferSetLength(ulNewLength);

			    ulReductionAttemptCount++;
			}

			NewFilename += pExtension;
		    }
		    
		    // Actually rename the file or directory
		    hResult = pFileRename->Rename((const char*) NewFilename);
		    
		    ulRenameAttemptCount++;
		    tNow.tv_usec += 1;
		} while (FAILED(hResult) && 
			 (ulRenameAttemptCount < MAX_RENAME_TRY_COUNT));

		HX_RELEASE(pAdviser);
	    }
	}
    }

    HX_RELEASE(pFileRename);
    HX_RELEASE(pFileMove);

    if (HXR_OK != hResult)
    {
	char buf[4096]; /* Flawfinder: ignore */

	SafeSprintf(buf, 4096, "Unable to %s \"%s\" to \"%s\"!\n",
	    pAction,
	    (const char*) BaseFileName, 
	    (const char*) NewFilename);
	m_pErrorMessages->Report(HXLOG_INFO, HXR_IGNORE, 
				 ulActionCode, buf, NULL);
    }

    return hResult;
}


HX_RESULT
CFileCreator::MakeDirectory(IHXFileObject* pObject)
{
    HX_ASSERT(!m_pDirHandler);

    if (HXR_OK == pObject->QueryInterface(IID_IHXDirHandler,
	                                  (void**)&m_pDirHandler))
    {
	m_pDirHandler->InitDirHandler((IHXDirHandlerResponse*)this);
    }
    else
    {
	m_FCStatus = FC_READY;
	m_pOwner->ArchiveDirReady(HXR_FAIL);
    }

    return HXR_OK;
}

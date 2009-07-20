/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ngtfileobject.cpp,v 1.4 2007/08/18 00:03:26 dcollins Exp $
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

/****************************************************************************
 * Defines
 */


/****************************************************************************
 * Includes
 */
#include "ngtfileobject.h"
#include "hxtlogutil.h"
#include "pckunpck.h"
#include "hxtlogutil.h"
#include "hxver.h"

#include "hlxclib/string.h"
#include "hlxclib/memory.h"
#include "hlxclib/time.h"


/****************************************************************************
 * Constants
 */

/****************************************************************************
 *  CNGTFileObject
 */
/****************************************************************************
 *  Interface
 */
BEGIN_INTERFACE_LIST_NOCREATE(CNGTFileObject)
INTERFACE_LIST_ENTRY_SIMPLE(IHXFileObject)
INTERFACE_LIST_ENTRY_SIMPLE(IHXFileStat)
INTERFACE_LIST_ENTRY_SIMPLE(IHXFileResponse)
INTERFACE_LIST_ENTRY_SIMPLE(IHXFileStatResponse)
INTERFACE_LIST_ENTRY_DELEGATE_BLIND(DelegatedQI)
END_INTERFACE_LIST


/****************************************************************************
 *  Constructor/Destructor
 */
CNGTFileObject::CNGTFileObject()
    : m_pContext(NULL)
    , m_pFileObject(NULL)
    , m_pFileStat(NULL)
    , m_pMetaFileResponse(NULL)
    , m_pFileResponse(NULL)
    , m_pFileStatResponse(NULL)
    , m_pDeferredInitialMediaWrite(NULL)
    , m_ulFlags(0)
    , m_ulMetaFileSize(0)
    , m_bMetaFileUpdateNeeded(FALSE)
    , m_eMetaFileState(NGTFO_Offline)
    , m_eFileObjectState(NGTFOIMPL_Closed)
{
    ;
}

CNGTFileObject::~CNGTFileObject()
{
    CloseNugget();

    CloseCoreAttributes();

    HX_RELEASE(m_pFileResponse);
    HX_RELEASE(m_pFileStatResponse);
}


/************************************************************************
 *  IHXFileObject methods
 */
STDMETHODIMP CNGTFileObject::Init(ULONG32 ulFlags,
				  IHXFileResponse* pFileResponse)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (IsMetaFileInitialized() && pFileResponse)
    {
	if (ulFlags & HX_FILE_WRITE)
	{
	    retVal = (m_ulFlags & HX_FILE_WRITE) ? HXR_OK : HXR_FAIL;
	}
	else
	{
	    retVal = HXR_OK;
	}
    }

    if (SUCCEEDED(retVal))
    {
	HX_RELEASE(m_pFileResponse);
	m_pFileResponse = pFileResponse;
	m_pFileResponse->AddRef();

	if (m_eFileObjectState == NGTFOIMPL_Initialized)
	{
	    // Reinitialize to 0
	    m_eFileObjectState = NGTFOIMPL_Initializing;
	    retVal = Seek(0, FALSE);
	}
	else
	{
	    HX_ASSERT(m_pFileObject);

	    m_eFileObjectState = NGTFOIMPL_Initializing;
	    retVal = m_pFileObject->Init(m_ulFlags, this);
	}

	if (FAILED(retVal))
	{
	    retVal = InitDone(HXR_FAIL);
	}
    }

    return retVal;
}

STDMETHODIMP CNGTFileObject::GetFilename(REF(const char*) pFilename)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pFileResponse)
    {
	// We should have file object if media-file is not closed and if
	// we have a reponse handle, media-file object cannot be closed.
	HX_ASSERT(m_pFileObject);

	retVal = m_pFileObject->GetFilename(pFilename);
    }

    return retVal;
}

STDMETHODIMP CNGTFileObject::Close()
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pFileResponse)
    {
	// We should have file object if media-file is not closed and if
	// we have a reponse handle, media-file object cannot be closed.
	HX_ASSERT(m_pFileObject);
	HX_RELEASE(m_pFileStatResponse);

	if (IsMetaFileInitialized())
	{
	    // If media nugget meta file is still initialized at this point,
	    // we simply report closure to the user of media file and
	    // release the users reponse interface.
	    // The file object will remain open for use of meta-file portion
	    // of the file until it meta-file closes via CloseNugget.
	    IHXFileResponse* pFileResponse = m_pFileResponse;
	    m_pFileResponse = NULL;

	    retVal = pFileResponse->CloseDone(HXR_OK);

	    HX_RELEASE(pFileResponse);
	}
	else
	{
	    // Media Nugget meta file is closed - we need to completely 
	    // shut down this object as there are no users left of this object.
	    HX_RELEASE(m_pContext);
	    HX_ASSERT(m_eMetaFileState == NGTFO_Offline);
	    HX_RELEASE(m_pFileStat);

	    retVal = StartClosingFileObject();
	    	
	    if (!SUCCEEDED(retVal))
	    {
		IHXFileResponse* pFileResponse = m_pFileResponse;

		m_pFileResponse = NULL;
		retVal = pFileResponse->CloseDone(HXR_OK);
		HX_RELEASE(pFileResponse);
	    }
	}
    }

    return retVal;
}

STDMETHODIMP CNGTFileObject::Read(ULONG32 ulCount)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pFileResponse)
    {
	// We should have file object if media-file is not closed and if
	// we have a reponse handle, media-file object cannot be closed.
	HX_ASSERT(m_pFileObject);

	if (m_eMetaFileState == NGTFO_InitializedForCreation)
	{
	    // When we are initialized for creation, the file is
	    // emty or is treated as empty and any read is treated
	    // as invalid.
	    m_pFileResponse->ReadDone(HXR_FAIL, NULL);
	    retVal = HXR_OK;
	}
	else
	{
	    retVal = m_pFileObject->Read(ulCount);
	}
    }

    return retVal;
}

STDMETHODIMP CNGTFileObject::Write(IHXBuffer* pBuffer)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pFileResponse)
    {
	// We should have file object if media-file is not closed and if
	// we have a reponse handle, media-file object cannot be closed.
	HX_ASSERT(m_pFileObject);

	// Once media data is written, we are no longer in creation but in
	// update mode.  In update mode, meta-file portion in the beginning
	// of the file can be edited to equal or smaller oerall size but can
	// no longer grow.
	if (m_eMetaFileState == NGTFO_InitializedForCreation)
	{
	    m_eMetaFileState = NGTFO_InitializedForUpdate;

	    HX_ASSERT(!m_pDeferredInitialMediaWrite);
	    if (!m_pDeferredInitialMediaWrite)
	    {
		// We need to defer this wrte until the meta-file
		// is written which will position the write offset
		// to the right location just passed the meta-file.
		m_pDeferredInitialMediaWrite = pBuffer;
		m_pDeferredInitialMediaWrite->AddRef();

		// Write-out the meta-file portion to position
		// to preper offset into the file.
		retVal = WriteMetaFile(HXR_OK);

		if (FAILED(retVal))
		{
		    HX_RELEASE(m_pDeferredInitialMediaWrite);
		}
	    }
	}
	else
	{
	    retVal = m_pFileObject->Write(pBuffer);

	    if (FAILED(retVal))
	    {
		retVal = WriteDone(retVal);
	    }
	}
    }

    return retVal;
}

STDMETHODIMP CNGTFileObject::Seek(ULONG32 ulOffset, HXBOOL bRelative)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pFileResponse)
    {
	// We should have file object if media-file is not closed and if
	// we have a reponse handle, media-file object cannot be closed.
	HX_ASSERT(m_pFileObject);

	if (m_eMetaFileState == NGTFO_InitializedForCreation)
	{
	    // When we are initialized for creation, the file is
	    // emty or is treated as empty and seek anywhere ut to
	    // the beginning of the file is invalid.
	    // In this state, the state file portion of the file is not 
	    // written and if we were to honor any seek, we would need
	    // to transition the state to NGTFO_InitializedForUpdate and
	    // fix the meta-file portion size in order to
	    // be able to resolve offsets consstently from now on.
	    // Also more logic would need to e added to write-out the
	    // meta-file portion.
	    if (m_eFileObjectState == NGTFOIMPL_Initializing)
	    {
		// If we are initializing state on completion of seek,
		// this must be a re-init by seeking to 0.
		retVal = InitDone((ulOffset == 0) ? HXR_OK : HXR_FAIL);
	    }
	    else
	    {
		retVal = SeekDone((ulOffset == 0) ? HXR_OK : HXR_FAIL);
	    }
	}
	else
	{
	    if (!bRelative)
	    {
		ulOffset += m_ulMetaFileSize;
	    }
	    retVal = m_pFileObject->Seek(ulOffset, bRelative);

	    if (FAILED(retVal))
	    {
		retVal = SeekDone(retVal);
	    }
	}
    }

    return retVal;
}

STDMETHODIMP CNGTFileObject::Advise(ULONG32 ulInfo)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pFileObject)
    {
	retVal = m_pFileObject->Advise(ulInfo);
    }

    return retVal;
}


/************************************************************************
 *  IHXFileStat method
 */
STDMETHODIMP CNGTFileObject::Stat(IHXFileStatResponse* pFileStatResponse)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pFileResponse && (!m_pFileStatResponse) && pFileStatResponse)
    {
	// We should have file object if media-file is not closed and if
	// we have a reponse handle, media-file object cannot be closed.
	HX_ASSERT(m_pFileObject);

	if (m_pFileStat)
	{
	    m_pFileStatResponse = pFileStatResponse;
	    m_pFileStatResponse->AddRef();

	    retVal = m_pFileStat->Stat(this);
	}

	if (FAILED(retVal))
	{
	    retVal = StatDone(retVal,
			      0,
			      0,
			      0,
			      0,
			      0);
	}
    }

    return retVal;
}

/************************************************************************
 *  IHXFileResponse methods
 */
STDMETHODIMP CNGTFileObject::InitDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_eMetaFileState == NGTFO_Initializing)
    {
	// We must have file object if we are at this point of meta-file
	// initialization.
	HX_ASSERT(m_pFileObject);

	if (SUCCEEDED(status) && m_pFileObject)
	{
	    // proceed by reading the start of nugget header
	    m_eMetaFileState = NGTFO_ReadingHeader;
	    m_eFileObjectState = NGTFOIMPL_Initialized;
	    retVal = m_pFileObject->Read(CNGTMetaFileHeader::GetPackedSize());
	}

	if (FAILED(retVal))
	{
	    IHXMetaFileFormatResponse* pMetaFileReponse = m_pMetaFileResponse;
	    m_pMetaFileResponse = NULL;
	    
	    CloseCoreAttributes();

	    HX_ASSERT(!m_pFileResponse);
	    
	    m_eMetaFileState = NGTFO_Offline;

	    if (!SUCCEEDED(status))
	    {
		status = retVal;
	    }

	    if (pMetaFileReponse)
	    {
		retVal = pMetaFileReponse->InitDone(status);
		HX_RELEASE(pMetaFileReponse);
	    }
	}
    }
    else if (m_pFileResponse)
    {
	// We should have file object if media-file is not closed and if
	// we have a reponse handle, media-file object cannot be closed.
	HX_ASSERT(m_pFileObject);

	if (SUCCEEDED(status))
	{
	    m_eFileObjectState = NGTFOIMPL_Initialized;
	}
	else
	{
	    CloseCoreAttributes();

	    m_eMetaFileState = NGTFO_Offline;
	}

	retVal = m_pFileResponse->InitDone(status);
    }

    return retVal;
}

STDMETHODIMP CNGTFileObject::CloseDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pFileResponse)
    {
	// If file object is closed and media file is around, the meta-file
	// must be closed already.
	HX_ASSERT(m_eMetaFileState == NGTFO_Offline);

	CloseCoreAttributes();

	IHXFileResponse* pFileResponse = m_pFileResponse;
	m_pFileResponse = NULL;
	retVal = pFileResponse->CloseDone(status);
	HX_RELEASE(pFileResponse);
    }
    else if (m_eMetaFileState == NGTFO_Closing)
    {
	CloseCoreAttributes();

	m_eMetaFileState = NGTFO_Offline;
    }

    return retVal;
}

STDMETHODIMP CNGTFileObject::ReadDone(HX_RESULT status,
                                      IHXBuffer* pBuffer)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pFileResponse)
    {
	retVal = m_pFileResponse->ReadDone(status, pBuffer);
    }
    else if (m_eMetaFileState == NGTFO_ReadingHeader)
    {
	// Parse header meta-file header and proceed with reading meta-file body
	HXBOOL bFileEmpty = TRUE;

	if (SUCCEEDED(status))
	{
	    if (pBuffer && (pBuffer->GetSize() == CNGTMetaFileHeader::GetPackedSize()))
	    {
		bFileEmpty = FALSE;
		
		status = m_metaFileHeader.Unpack(pBuffer->GetBuffer(), pBuffer->GetSize());

		if (SUCCEEDED(status))
		{
		    m_ulMetaFileSize = CNGTMetaFileHeader::GetPackedSize() +
				       m_metaFileHeader.m_uFileBodySize;
		}
	    }
	    else if (pBuffer && (pBuffer->GetSize() != 0))
	    {
		status = HXR_HEADER_PARSE_ERROR;
		bFileEmpty = FALSE;
	    }
	}

	if (bFileEmpty && (m_ulFlags & HX_FILE_WRITE))
	{
	    // Empty file - OK condition if opened file for writing
	    m_eMetaFileState = NGTFO_InitializedForCreation;
	    // Set how the current meta-file size is now.
	    // The meta-file size can be chnaged until media data
	    // is written.
	    m_ulMetaFileSize = m_metaFileHeader.GetPackedSize() +
			       m_metaFileBody.GetPackedSize();
	    status = HXR_OK;

	    retVal = HXR_OK;
	    if (m_pMetaFileResponse)
	    {
		retVal = m_pMetaFileResponse->InitDone(status);
	    }
	}
	else 
	{
	    if (SUCCEEDED(status) && m_pFileObject)
	    {
		m_eMetaFileState = NGTFO_ReadingBody;
		retVal = m_pFileObject->Read(m_metaFileHeader.m_uFileBodySize);
	    }

	    if (FAILED(status))
	    {
		retVal = ReadDone(status, NULL);
	    }
	}
    }	
    else if (m_eMetaFileState == NGTFO_ReadingBody)
    {
	IHXMetaFileFormatResponse* pMetaFileResponse = m_pMetaFileResponse;
	HX_ADDREF(pMetaFileResponse);

	// Parse header body and complete initialization
	if (SUCCEEDED(status))
	{
	    // Parse meta-file body here...
	    status = m_metaFileBody.Unpack(pBuffer->GetBuffer(), pBuffer->GetSize());
	}

	if (SUCCEEDED(status))
	{
	    m_eMetaFileState = NGTFO_InitializedForUpdate;
	}
	else
	{
	    CloseNugget();
	}

	if (pMetaFileResponse)
	{
	    retVal = pMetaFileResponse->InitDone(status);
	}

	HX_RELEASE(pMetaFileResponse);
    }

    return retVal;
}

STDMETHODIMP CNGTFileObject::WriteDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_eFileObjectState == NGTFOIMPL_Closing)
    {
	// This must be a write completion of the meta-file portion
	// of the file.
	// Proceed by closing the file object.
	HX_ASSERT(m_bMetaFileUpdateNeeded);
	HX_ASSERT(m_pFileObject);

	if (m_bMetaFileUpdateNeeded)
	{
	    m_bMetaFileUpdateNeeded = FALSE;

	    if (m_pFileObject)
	    {
		retVal = m_pFileObject->Close();
	    }

	    if (FAILED(retVal))
	    {
		retVal = CloseDone(status);
	    }
	}
    }
    else if (m_pFileResponse)
    {
	if (m_pDeferredInitialMediaWrite)
	{
	    if (SUCCEEDED(status))
	    {
		IHXBuffer* pBuffer = m_pDeferredInitialMediaWrite;
		HX_ASSERT(m_pFileObject);
		m_pDeferredInitialMediaWrite = NULL;
		retVal = m_pFileObject->Write(pBuffer);
		pBuffer->Release();
	    }
	    else
	    {
		HX_RELEASE(m_pDeferredInitialMediaWrite);
		m_pFileResponse->WriteDone(status);
	    }
	}
	else
	{
	    retVal = m_pFileResponse->WriteDone(status);
	}
    }

    return retVal;
}

STDMETHODIMP CNGTFileObject::SeekDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_eFileObjectState == NGTFOIMPL_Closing)
    {
	// This must be a seek to the beginning of the file during the closing
	// of the file object due to the need to re-write the meta-file portion
	// (the media nugget header).
	HX_ASSERT(m_bMetaFileUpdateNeeded);
	HX_ASSERT(m_pFileObject);

	if (m_bMetaFileUpdateNeeded)
	{
	    retVal = WriteMetaFile(status);
	}
    }
    else if (m_pFileResponse)
    {
	if (m_eFileObjectState == NGTFOIMPL_Initializing)
	{
	    // If we have SeekDone in initializing state, this must be
	    // re-initialization by seeking to 0.  Report completion
	    // of initialization.
	    retVal = InitDone(status);
	}
	else
	{
	    retVal = m_pFileResponse->SeekDone(status);
	}
    }

    return retVal;
}


/************************************************************************
 *  IHXFileStatResponse method
 */
STDMETHODIMP CNGTFileObject::StatDone(HX_RESULT status,
				      UINT32 ulSize,
				      UINT32 ulCreationTime,
				      UINT32 ulAccessTime,
				      UINT32 ulModificationTime,
				      UINT32 ulMode)
{
    IHXFileStatResponse* pFileStatResponse = m_pFileStatResponse;
    HX_RESULT retVal = HXR_UNEXPECTED;

    m_pFileStatResponse = NULL;

    if (m_pFileResponse && pFileStatResponse)
    {
	if (ulSize >= m_ulMetaFileSize)
	{
	    ulSize -= m_ulMetaFileSize;
	}
	else
	{
	    ulSize = 0;
	}

	retVal = pFileStatResponse->StatDone(status,
					     ulSize,
					     ulCreationTime,
					     ulAccessTime,
					     ulModificationTime,
					     ulMode);
    }

    HX_RELEASE(pFileStatResponse);

    return retVal;
}


/************************************************************************
 *  Public Meta-file methods
 */
HX_RESULT CNGTFileObject::InitNugget(IUnknown* pContext,
				     IHXFileObject* pFileObject, 
				     UINT32 ulFlags, 
				     IHXMetaFileFormatResponse* pResponse,
				     HXBOOL bForceNew)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if ((m_eMetaFileState == NGTFO_Offline) && 
	(!m_pFileResponse))
    {
	HX_ASSERT(!m_pFileObject);
	HX_ASSERT(!m_pFileStat);
	HX_ASSERT(!m_pContext);

	retVal = HXR_INVALID_PARAMETER;

	// If we are forcing new nugget, initialization is guranteeed to
	// be synchronous and thus reponse API is not required.
	if (pContext && pFileObject && (pResponse || bForceNew))
	{
	    m_metaFileHeader.Clear();
	    m_metaFileBody.Clear();

	    m_pContext = pContext;
	    m_pContext->AddRef();
	    m_pFileObject = pFileObject;
	    m_pFileObject->AddRef();
	    m_ulFlags = ulFlags;
	    m_pMetaFileResponse = pResponse;
	    HX_ADDREF(m_pMetaFileResponse);

	    m_bMetaFileUpdateNeeded = FALSE;
	    m_eFileObjectState = NGTFOIMPL_Closed;

	    m_pFileObject->QueryInterface(IID_IHXFileStat, (void**) &m_pFileStat);

	    if (bForceNew)
	    {
		// We'll initialize as if header does not exist.
		m_eMetaFileState = NGTFO_ReadingHeader;
		retVal = ReadDone(HXR_FAIL, NULL);
	    }
	    else
	    {
		m_eMetaFileState = NGTFO_Initializing;

		retVal = m_pFileObject->Init(m_ulFlags, this);
	    }
	}
    }

    return retVal;
}

HX_RESULT CNGTFileObject::StartClosingFileObject(void)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pFileObject)
    {
	m_eFileObjectState = NGTFOIMPL_Closing;

	if ((m_ulFlags & HX_FILE_WRITE) && m_bMetaFileUpdateNeeded)
	{
	    // We can and need to refresh the meta file portion
	    // located at the beginning of the file.
	    retVal = m_pFileObject->Seek(0, FALSE);

	    if (FAILED(retVal))
	    {
		retVal = SeekDone(retVal);
	    }
	}
	else
	{
	    retVal = m_pFileObject->Close();

	    if (FAILED(retVal))
	    {
		retVal = CloseDone(retVal);
	    }
	}
    }

    return retVal;
}

void CNGTFileObject::CloseCoreAttributes(void)
{
    m_metaFileHeader.Clear();
    m_metaFileBody.Clear();

    HX_RELEASE(m_pDeferredInitialMediaWrite);
    HX_RELEASE(m_pFileObject);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pFileStat);
    HX_RELEASE(m_pFileStatResponse);
    m_ulFlags = 0;
    m_ulMetaFileSize = 0;
    m_bMetaFileUpdateNeeded = FALSE;
    m_eFileObjectState = NGTFOIMPL_Closed;
}

HX_RESULT CNGTFileObject::WriteMetaFile(HX_RESULT entryStatus)
{
    HX_RESULT retVal = entryStatus;
    IHXBuffer* pMetaFileBuffer = NULL;
    UINT32 ulNewMetaFileSize = m_metaFileHeader.GetPackedSize() +
			       m_metaFileBody.GetPackedSize();

    // The meta-file we are about to write must fit into space allocated for it
    // in the media nugget file.
    if (SUCCEEDED(retVal) && (m_ulMetaFileSize >= ulNewMetaFileSize))
    {
	// Create the meta-file buffer and pack meta-file into it
	UINT8* pMetaFileData = NULL;
	m_metaFileHeader.m_uFileBodySize = (UINT16) (m_ulMetaFileSize - 
						   m_metaFileHeader.GetPackedSize());
	HX_ASSERT(m_metaFileHeader.m_uFileBodySize >= m_metaFileBody.GetPackedSize());

	retVal = CreateSizedBufferCCF(pMetaFileBuffer,
				      m_pContext,
				      m_ulMetaFileSize,
				      TRUE,	// Initialize
				      0);	// Initial value

	if (SUCCEEDED(retVal))
	{
	    UINT32 ulSize = m_ulMetaFileSize;
	    pMetaFileData = pMetaFileBuffer->GetBuffer();

	    retVal = m_metaFileHeader.Pack(pMetaFileData, ulSize);
	}

	if (SUCCEEDED(retVal))
	{
	    UINT32 ulSize = m_metaFileHeader.m_uFileBodySize;
	    retVal = m_metaFileBody.Pack(pMetaFileData + m_metaFileHeader.GetPackedSize(), 
		ulSize);
	}
    }
    else if (SUCCEEDED(retVal))
    {
	retVal = HXR_FAIL;
    }

    if (SUCCEEDED(retVal))
    {
	retVal = m_pFileObject->Write(pMetaFileBuffer);
    }

    if (FAILED(retVal))
    {
	retVal = WriteDone(HXR_FAIL);
    }

    return retVal;
}

void CNGTFileObject::CloseNugget(void)
{
    if (IsMetaFileInitialized())
    {
	// If meta-file is initialized, file object must be around.
	HX_ASSERT(m_pFileObject);
	HX_RELEASE(m_pMetaFileResponse);

	// Is object in use by media file user?
	if (!m_pFileResponse)
	{
	    // Object is not in use by media-file user.
	    // Completely shut down this object.
	    
	    if (m_pFileObject)
	    {
		m_eMetaFileState = NGTFO_Closing;
		StartClosingFileObject();
	    }
	    else
	    {
		CloseCoreAttributes();
		m_eMetaFileState = NGTFO_Offline;
	    }
	}
	else
	{
	    m_eMetaFileState = NGTFO_Offline;
	}
    }
}

HXBOOL CNGTFileObject::IsExpired(void)
{ 
    return ((m_metaFileBody.m_ulExpiration != 0) && 
	    (m_metaFileBody.m_ulExpiration <= ((UINT32) time(NULL))));
}

void CNGTFileObject::SetNuggetLocalDuration(UINT32 ulLocalDuration)
{
    if (m_metaFileBody.m_ulLocalDuration != ulLocalDuration)
    {
	m_metaFileBody.m_ulLocalDuration = ulLocalDuration;
	m_bMetaFileUpdateNeeded = TRUE;
    }
}

HXBOOL CNGTFileObject::ConfigureNugget(UINT32 ulExpiration,
				       UINT32 ulConnectTime,
				       UINT32 ulLocalDuration,
				       UINT32 ulOverallDuration,
				       const char* pLocalMimeType,
				       const char* pRemoteSourceURL)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (IsMetaFileInitialized() && (m_ulFlags & HX_FILE_WRITE))
    {
	retVal = HXR_BUFFERTOOSMALL;
	if (m_eMetaFileState == NGTFO_InitializedForCreation)
	{
	    retVal = HXR_OK;
	}
	else if (m_eMetaFileState == NGTFO_InitializedForUpdate)
	{

	    // Since we are in update mode, we need to make sure
	    // that updated meta-file is not larger than existing one.
	    // We check only the variable (body) portion of the meta-file.
	    CNGTMetaFileBody newMetaFileBody;

	    newMetaFileBody.m_ulExpiration = ulExpiration;
	    newMetaFileBody.m_ulConnectTime = ulConnectTime;
	    newMetaFileBody.m_ulLocalDuration = ulLocalDuration;
	    newMetaFileBody.m_ulOverallDuration = ulOverallDuration;
	    newMetaFileBody.m_pLocalMimeType = (char*) pLocalMimeType;
	    newMetaFileBody.m_pRemoteSourceURL = (char*) pRemoteSourceURL;

	    if ((newMetaFileBody.GetPackedSize() + 
		 CNGTMetaFileHeader::GetPackedSize()) <= m_ulMetaFileSize)
	    {
		retVal = HXR_OK;
	    }

	    // We must do this to prevent newMetaFileBody destructor from
	    // attempting to delete string constants.
	    newMetaFileBody.m_pLocalMimeType = NULL;
	    newMetaFileBody.m_pRemoteSourceURL = NULL;
	}

	if (SUCCEEDED(retVal))
	{
	    char* pNewLocalMimeType = pLocalMimeType ? new_string(pLocalMimeType) : NULL;
	    char* pNewRemoteSourceURL = pRemoteSourceURL ? new_string(pRemoteSourceURL) : NULL;

	    m_metaFileBody.Clear();
	    m_metaFileBody.m_ulExpiration = ulExpiration;
	    m_metaFileBody.m_ulConnectTime = ulConnectTime;
	    m_metaFileBody.m_ulLocalDuration = ulLocalDuration;
	    m_metaFileBody.m_ulOverallDuration = ulOverallDuration;
	    m_metaFileBody.m_pLocalMimeType = pNewLocalMimeType;
	    m_metaFileBody.m_pRemoteSourceURL = pNewRemoteSourceURL;

	    // If we are in process of creating a file, refresh the meta-file size to fit exactly
	    // the newly provided data.
	    if (m_eMetaFileState == NGTFO_InitializedForCreation)
	    {
		m_ulMetaFileSize = m_metaFileBody.GetPackedSize() + 
				   CNGTMetaFileHeader::GetPackedSize();
	    }

	    HX_ASSERT(m_ulMetaFileSize > CNGTMetaFileHeader::GetPackedSize());

	    m_metaFileHeader.m_uFileBodySize = 
		(UINT16) (m_ulMetaFileSize - CNGTMetaFileHeader::GetPackedSize());

	    m_bMetaFileUpdateNeeded = TRUE;
	}
    }

    return retVal;
}

/************************************************************************
 *  Private methods
 */
HX_RESULT CNGTFileObject::DelegatedQI(REFIID riid, void** ppvObj)
{
    if (!ppvObj)
    {
	return HXR_POINTER;
    }

    if (m_pFileObject)
    {
	if (IsEqualIID(riid, IID_IHXDirHandler))
	{
	    return m_pFileObject->QueryInterface(riid, ppvObj);
	}
	else if (IsEqualIID(riid, IID_IHXFileExists))
	{
	    return m_pFileObject->QueryInterface(riid, ppvObj);
	}
	else if (IsEqualIID(riid, IID_IHXFileRename))
	{
	    return m_pFileObject->QueryInterface(riid, ppvObj);
	}
	else if (IsEqualIID(riid, IID_IHXFileMove))
	{
	    return m_pFileObject->QueryInterface(riid, ppvObj);
	}
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

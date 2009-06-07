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
#define DFLT_ROTATION_TIME  0
#define DFLT_ROTATION_SIZE  0
#define DFLT_NUGGET_CONNECT_TIME    15000   // in milliseconds


/****************************************************************************
 *  Includes
 */
#include <stdio.h>

#include "hxtypes.h"
#include "hxstrutl.h"
#include "hxcom.h"
#include "chxpckts.h"
#include "hxmime.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "pckunpck.h"
#include "hxfiles.h"
#include "hxengin.h"
#include "hxerror.h"
#include "hxmon.h"
#include "timeval.h"
#include "hxsrc.h"
#include "hxmap.h"
#include "hxslist.h"
#include "chxuuid.h"
#include "rtsputil.h"
#include "flcreatr.h"

#include "archivr2.h"
#include "hxdir.h"

#include "ngtfileobject.h"

#ifdef _CARBON
#include "fullpathname.h"
#endif


// 
//  Class: CBaseArchiver2
//
CBaseArchiver2::CBaseArchiver2(IUnknown* pContext, 
			     IHXFileWriterMonitor* pMonitor, 
			     IHXPropertyAdviser* pAdviser)
    : m_lRefCount	    (0)
    , m_ARStatus	    (AR_INITIALIZING)
    , m_DirStatus	    (DIR_INITIALIZING)
    , m_pAdviser	    (pAdviser)
    , m_pContext	    (pContext)
    , m_pClassFactory	    (NULL)
    , m_pScheduler	    (NULL)
    , m_pErrorMessages	    (NULL)
    , m_pFileCreator	    (NULL)
    , m_bFileCreatorReady   (FALSE)
    , m_pMonitor	    (pMonitor)
    , m_pFileHeader	    (NULL)
    , m_pMetaInfo	    (NULL)
    , m_ulNumStreams	    (0)
    , m_ulActiveStreams	    (0)
    , m_iFileTime	    (0)
    , m_iFileSize	    (0)
    , m_bUseNuggetFormat    (FALSE)
    , m_ulNuggetMinLocalDuration(0)
    , m_pNuggetFileObject   (NULL)
    , m_ulMaxPreroll	    (0)
    , m_ulMaxDuration	    (0)
    , m_bKnownDuration	    (FALSE)
    , m_bKnownPreroll	    (TRUE)
    , m_bEnforceSaneValues  (TRUE)
    , m_bNeedsRelativeTS    (FALSE)
    , m_bIsFullyNative	    (TRUE)
    , m_bContainerFormatEnabled(TRUE)
    , m_bClosing	    (FALSE)
    , m_ulVolumeNo	    (0)
    , m_ulArchiverID	    (0)
    , m_ulPadContentToSize  (0)
    , m_bBlastFiles	    (FALSE)
    , m_bRotateAsFallback   (FALSE)
    , m_bTempDirUsed	    (FALSE)
    , m_bUseTempFiles	    (TRUE)
    , m_bRecordAsLive	    (FALSE)
    , m_ulDirsNeeded	    (0)
    , m_ulDirsCreated	    (0)
    , m_ppDirectoryList	    (NULL)
    , m_bHandlingStoredPackets(FALSE)
{
    HX_ASSERT(m_pAdviser);
    m_pAdviser->AddRef();

    HX_ASSERT(m_pContext);
    m_pContext->AddRef();

    HX_ASSERT(m_pMonitor);
    m_pMonitor->AddRef();

    m_pVolumeGUIDName[0] = '\0';
    gettimeofday(&m_StartTime, 0);
}

CBaseArchiver2::~CBaseArchiver2()
{
    HandleStoredPackets();

    if (m_pNuggetFileObject)
    {
	m_pNuggetFileObject->CloseNugget();
	HX_RELEASE(m_pNuggetFileObject);
    }

    if (m_pFileCreator)
    {
	CFileCreator* pFileCreator = m_pFileCreator;
	m_pFileCreator = NULL;
	pFileCreator->Done();
	pFileCreator->Release();;
    }

    HX_RELEASE(m_pFileHeader);
    HX_RELEASE(m_pMetaInfo);

    CHXSimpleList::Iterator i;
    for (i = m_StreamHeaders.Begin(); i != m_StreamHeaders.End(); ++i)
    {
	IHXValues* pHeader = (IHXValues*)(*i);
	HX_RELEASE(pHeader);
    }

    for (UINT32 j = 0; j < m_ulDirsNeeded; j++)
    {
	HX_VECTOR_DELETE(m_ppDirectoryList[j]);
    }
    HX_VECTOR_DELETE(m_ppDirectoryList);

    HX_RELEASE(m_pMonitor);
    HX_RELEASE(m_pAdviser);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pErrorMessages);
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
CBaseArchiver2::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileCreatorResponse))
    {
        AddRef();
        *ppvObj = (IHXFileCreatorResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPropertyAdviser))
    {
	if (m_pAdviser)
	{
	    m_pAdviser->AddRef();
	    *ppvObj = m_pAdviser;
	    return HXR_OK;
	}
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
CBaseArchiver2::AddRef()
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
CBaseArchiver2::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT
CBaseArchiver2::Init(IHXRequest* pRequest)
{
    HX_RESULT retVal = HXR_OK;

    // Get Interfaces
    retVal = m_pContext->QueryInterface(IID_IHXCommonClassFactory,
					 (void**)&m_pClassFactory);
    if (SUCCEEDED(retVal))
    {
	retVal = m_pContext->QueryInterface(IID_IHXScheduler,
					    (void**)&m_pScheduler);
    }

    if (SUCCEEDED(retVal))
    {
	retVal = m_pContext->QueryInterface(IID_IHXErrorMessages,
					    (void**)&m_pErrorMessages);
    }

    // Initialize Writer target file name and directory
    if (SUCCEEDED(retVal))
    {
	retVal = InitTargetName(pRequest);
    }

    // Determine if a nugget format is to be produced
    if (SUCCEEDED(retVal))
    {
	ULONG32 ulVal = 0;

	retVal = m_pAdviser->GetPropertyULONG32("UseNuggetFormat", 
						ulVal);

	if (SUCCEEDED(retVal))
	{
	    m_bUseNuggetFormat = (ulVal != 0);
	}

	retVal = HXR_OK;
    }

    // Determine if a nugget format is to be produced
    if (SUCCEEDED(retVal) && (!m_bUseNuggetFormat))
    {
	ULONG32 ulVal = 0;

	retVal = m_pAdviser->GetPropertyULONG32("RecordAsLive", 
						ulVal);

	if (SUCCEEDED(retVal))
	{
	    m_bRecordAsLive = (ulVal != 0);
	}

	retVal = HXR_OK;
    }

    // Initialize File Rotation parameters
    if (SUCCEEDED(retVal) && (!m_bUseNuggetFormat))
    {
	// If Nugget format is to be used - do not rotate the file
	if (m_bUseNuggetFormat)
	{
	    m_iFileSize = 0;
	}
	else
	{
	    ULONG32 ulVal = 0;

	    retVal = m_pAdviser->GetPropertyULONG32("RotationInterval", 
						    ulVal);

	    if (SUCCEEDED(retVal))
	    {
		m_iFileTime = (INT32) ulVal;
	    }
	    else
	    {
		m_iFileTime = DFLT_ROTATION_TIME;
		retVal = HXR_OK;
	    }
	}
    }

    if (SUCCEEDED(retVal))
    {
	// If Nugget format is to be used - do not rotate the file
	if (m_bUseNuggetFormat)
	{
	    m_iFileSize = 0;
	}
	else
	{
	    ULONG32 ulVal = 0;

	    retVal = m_pAdviser->GetPropertyULONG32("RotationSize", 
						    ulVal);

	    if (SUCCEEDED(retVal))
	    {
		m_iFileSize = (INT32) ulVal;
	    }
	    else
	    {
		m_iFileSize = DFLT_ROTATION_SIZE;
		retVal = HXR_OK;
	    }
	}
    }

    if (SUCCEEDED(retVal))
    {
	// If Nugget format is to be used - retain the original duration
	// Duration will represent the entire content and not just the
	// nugget piece.
	if (m_bUseNuggetFormat)
	{
	    m_bKnownDuration = TRUE;
	}
	else
	{
	    ULONG32 ulVal = 0;

	    retVal = m_pAdviser->GetPropertyULONG32("KnownDuration", 
						    ulVal);

	    m_bKnownDuration = FALSE;
	    if (SUCCEEDED(retVal))
	    {
		m_bKnownDuration = (ulVal != 0);
	    }

	    retVal = HXR_OK;
	}
    }

    if (SUCCEEDED(retVal))
    {
	// If Nugget format is to be used - keep time-stamps in same format
	// as original.
	if (m_bUseNuggetFormat)
	{
	    m_bNeedsRelativeTS = FALSE;
	}
	else
	{
	    ULONG32 ulVal = 0;

	    retVal = m_pAdviser->GetPropertyULONG32("UseRelativeTS", 
						 ulVal);

	    m_bNeedsRelativeTS = FALSE;
	    if (SUCCEEDED(retVal))
	    {
		m_bNeedsRelativeTS = (ulVal != 0);
	    }

	    retVal = HXR_OK;
	}
    }

    if (SUCCEEDED(retVal))
    {
	ULONG32 ulVal = 0;

	retVal = m_pAdviser->GetPropertyULONG32("EnableContainerFormat",
						ulVal);

	m_bContainerFormatEnabled = TRUE;
	if (SUCCEEDED(retVal))
	{
	    m_bContainerFormatEnabled = (ulVal != 0);
	}

	retVal = HXR_OK;
    }
    
    if (SUCCEEDED(retVal))
    {
	ULONG32 ulVal = 0;

	retVal = m_pAdviser->GetPropertyULONG32("IsFullyNative",
						ulVal);

	m_bIsFullyNative = TRUE;
	if (SUCCEEDED(retVal))
	{
	    m_bIsFullyNative = (ulVal != 0);
	}

	retVal = HXR_OK;
    }
    
    if (SUCCEEDED(retVal))
    {
	// If Nugget format is to be used - do not recompute preroll
	if (m_bUseNuggetFormat)
	{
	    m_bKnownPreroll = TRUE;
	}
	else
	{
	    ULONG32 ulVal = 0;

	    retVal = m_pAdviser->GetPropertyULONG32("RecomputePreroll", 
						    ulVal);

	    m_bKnownPreroll = TRUE;
	    if (SUCCEEDED(retVal))
	    {
		m_bKnownPreroll = (ulVal == 0);
	    }

	    retVal = HXR_OK;
	}
    }

    if (SUCCEEDED(retVal))
    {
	// If Nugget format is to be used - trust the given media properties
	// Recomputing only on the nugget portion would be invalid for the
	// entire content the nugget represents.
	if (m_bUseNuggetFormat)
	{
	    m_bEnforceSaneValues = FALSE;
	}
	else
	{
	    ULONG32 ulVal = 0;

	    retVal = m_pAdviser->GetPropertyULONG32("TrustGivenMediaProperties", 
						    ulVal);

	    m_bEnforceSaneValues = TRUE;
	    if (SUCCEEDED(retVal))
	    {
		m_bEnforceSaneValues = (ulVal == 0);
	    }

	    retVal = HXR_OK;
	}
    }

    if (SUCCEEDED(retVal))
    {
	ULONG32 ulVal = 0;

	retVal = m_pAdviser->GetPropertyULONG32("PadContentToSize", 
						 ulVal);

	m_ulPadContentToSize = 0;
	if (SUCCEEDED(retVal))
	{
	    m_ulPadContentToSize = ulVal;
	}

	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	ULONG32 ulVal = 0;

	retVal = m_pAdviser->GetPropertyULONG32("BlastFiles", 
						 ulVal);

	m_bBlastFiles = FALSE;
	if (SUCCEEDED(retVal))
	{
	    m_bBlastFiles = (ulVal != 0);
	}

	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	ULONG32 ulVal = 0;

	retVal = m_pAdviser->GetPropertyULONG32("UseTempFiles", 
						ulVal);

	m_bUseTempFiles = TRUE;
	if (SUCCEEDED(retVal))
	{
	    m_bUseTempFiles = (ulVal != 0);
	}

	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	ULONG32 ulVal = 0;

	retVal = m_pAdviser->GetPropertyULONG32("RotateAsFallback", 
						 ulVal);

	m_bRotateAsFallback = FALSE;
	if (SUCCEEDED(retVal))
	{
	    m_bRotateAsFallback = (ulVal != 0);
	}

	retVal = HXR_OK;
    }

    // Create and Initialize the File System Manager so we can create files
    if (SUCCEEDED(retVal))
    {
	HX_ASSERT(m_pFileCreator == NULL);
	HX_RELEASE(m_pFileCreator);

	m_pFileCreator = new CFileCreator(this, 
					  m_pContext,
					  m_bBlastFiles);
	m_pFileCreator->AddRef();
	m_pFileCreator->Init();
    }

    if (SUCCEEDED(retVal))
    {
	retVal = OnInit(pRequest);
    }

    return retVal;
}

HX_RESULT
CBaseArchiver2::Done()
{
    OnDone();
    return HXR_OK;
}

HX_RESULT
CBaseArchiver2::Abort(void)
{
    HX_RESULT retVal;

    retVal = OnAbort();

    if (m_pFileCreator)
    {
	m_pFileCreator->Done();
	HX_RELEASE(m_pFileCreator);
    }

    return retVal;
}

/*** IHXFileCreatorResponse methods ***/

STDMETHODIMP
CBaseArchiver2::InitDone(HX_RESULT status)
{
    HX_RESULT hResult = HXR_OK;

    if (HXR_OK != status)
    {
	m_pMonitor->OnCompletion(status);
	return HXR_OK;
    }

    m_bFileCreatorReady = TRUE;

    // Check if all stream headers have been received
    if (ReadyToArchive())
    {
	if (m_bUseNuggetFormat)
	{
	    ComputeMinLocalDuration();
	}
	ProcessStreamHeaders();
	CreateDirObjects();
    }

    return hResult;
}

STDMETHODIMP 
CBaseArchiver2::ArchiveDirReady(HX_RESULT status)
{
    if (m_DirStatus == DIR_CREATING)
    {
	ArchiveDirectoryReady(status);
    }

    return HXR_OK;
}

STDMETHODIMP 
CBaseArchiver2::ArchiveFileReady(HX_RESULT status,
				IHXFileObject* pFileObject)
{
    return HXR_OK;
}

STDMETHODIMP 
CBaseArchiver2::ExistingFileReady(HX_RESULT status,
				 IHXFileObject* pFileObject)
{
    return HXR_OK;
}


HX_RESULT
CBaseArchiver2::FileHeaderReady(IHXValues* pHeader)
{
    HX_ASSERT(pHeader);

    HX_RELEASE(m_pFileHeader);
    m_pFileHeader = pHeader;
    m_pFileHeader->AddRef();

    // Debugging Info
    //printf("Received File Header\n");
    //printf("====================\n");
    //DumpValues(pHeader);

    if (m_ulNumStreams == 0)
    {
	m_pFileHeader->GetPropertyULONG32("StreamCount", m_ulNumStreams);
    }

    // Filter the file header for meta data that we don't recognize.
    // These values (like Description) need to be added to the archive
    ExtractFileMetaInfo(pHeader);

    OnNewFileHeader(pHeader);

    return HXR_OK;
}

HX_RESULT
CBaseArchiver2::StreamHeaderReady(IHXValues* pHeader)
{
    UINT32 ulPreroll = 0;
    UINT32 ulDuration = 0;
    HX_RESULT hResult = HXR_OK;

    // Debugging Info
    //printf("Received Stream Header\n");
    //printf("======================\n");
    //DumpValues(pHeader);

    pHeader->GetPropertyULONG32("Preroll", ulPreroll);
    if (m_ulMaxPreroll < ulPreroll)
    {
	m_ulMaxPreroll = ulPreroll;
    }

    pHeader->GetPropertyULONG32("Duration", ulDuration);
    if (m_ulMaxDuration < ulDuration)
    {
	m_ulMaxDuration = ulDuration;
    }

    // Make sure the duration of each stream is 0, if this is
    // a live presentation, and we don't want the archiving code
    // to make any assumptions about the real stream durations
    if (!m_bKnownDuration)
    {
	pHeader->SetPropertyULONG32("Duration", 0);
    }

    if (!m_bKnownPreroll)
    {
	pHeader->SetPropertyULONG32("Preroll", 0);
	pHeader->SetPropertyULONG32("ActualPreroll", 0);
	pHeader->SetPropertyULONG32("Predata", 0);
    }

    hResult = OnNewStreamHeader(pHeader);

    if (m_ulActiveStreams < m_ulNumStreams)
    {
	pHeader->AddRef();
	m_StreamHeaders.AddTail(pHeader);

	m_ulActiveStreams++;

	// Check if all stream headers have been received
	if (ReadyToArchive())
	{
	    if (m_bUseNuggetFormat)
	    {
		ComputeMinLocalDuration();
	    }
	    ProcessStreamHeaders();
	    CreateDirObjects();
	}
    }

    return hResult;
}

HX_RESULT
CBaseArchiver2::PacketReady(IHXPacket* pPacket)
{
    HX_RESULT hResult = HXR_OK;

    hResult = OnNewPacket(pPacket);

    return hResult;
}

HX_RESULT 
CBaseArchiver2::OnAbort(void)
{
    return HXR_OK;
}

HX_RESULT 
CBaseArchiver2::OnInit(IHXRequest* pRequest)
{
    return HXR_OK;
}

HX_RESULT
CBaseArchiver2::OnNewStreamHeader(IHXValues* pMetaInfo)
{
    return HXR_OK;
}

HX_RESULT
CBaseArchiver2::OnNewMetaInfo(IHXValues* pHeader)
{
    return HXR_OK;
}

HX_RESULT
CBaseArchiver2::OnNewFileHeader(IHXValues* pHeader)
{
    return HXR_OK;
}

HX_RESULT
CBaseArchiver2::OnNewPacket(IHXPacket* pPacket)
{
    return HXR_OK;
}

HX_RESULT 
CBaseArchiver2::ComputeMinLocalDuration()
{
    UINT32 ulConnectTime = DFLT_NUGGET_CONNECT_TIME;

    m_pAdviser->GetPropertyULONG32("NuggetConnectTime", ulConnectTime);
    m_ulNuggetMinLocalDuration = ulConnectTime + m_ulMaxPreroll;

    return HXR_OK;
}

HX_RESULT 
CBaseArchiver2::ProcessStreamHeaders()
{
    return HXR_OK;
}

HX_RESULT 
CBaseArchiver2::CreateDirObjects()
{
    HX_RESULT hResult = HXR_OK;

    if (m_DirStatus == DIR_INITIALIZING)
    {
	// Figure out what directories we need to create
	CreateDirectoryList();

	// Move on to the directory creating state
	m_DirStatus = DIR_CREATING;
	CreateDirObjects();
    }
    else if (m_DirStatus == DIR_CREATING)
    {
	if (m_ulDirsCreated == m_ulDirsNeeded)
	{
	    // We are done creating directories
	    m_DirStatus = DIR_DONE;

	    CreateFileObjects();
	}
	else
	{
	    // Create the next directory
	    hResult = m_pFileCreator->
		CreateArchiveDir(m_ppDirectoryList[m_ulDirsCreated]);

	    if ((HXR_OK != hResult) && (m_DirStatus != DIR_DONE))
	    {
		ArchiveDirectoryReady(hResult);
	    }

	    // ... Flow continues in ::ArchiveDirectoryReady()
	}
    }
    else
    {
	// Invalid state
	HX_ASSERT(0);
    }

    return HXR_OK;
}

HX_RESULT
CBaseArchiver2::CreateDirectoryList()
{
    UINT32 ulCurrDir = 0;
    const char* pStart = (const char*)m_ArchiveDir;
    const char* pCurr = pStart;
    ULONG32 ulProtocolLen = m_ProtocolName.GetLength();

    if (ulProtocolLen > 0)
    {
	ulProtocolLen += 3; // For :// in <protocol>://<file name path>
    }

    m_ulDirsNeeded = 0;
    m_ulDirsCreated = 0;

    // Skip the first slash
    if (*pCurr == '/')
    {
	pCurr++;
    }

    // Figure out how many directories we will need
    while (*pCurr)
    {
	if (*pCurr == '/')
	{
	    m_ulDirsNeeded++;
	}
	pCurr++;
    }

    if (m_ulDirsNeeded)
    {
	// Create an array large enough to hold them
	HX_ASSERT(!m_ppDirectoryList);
	m_ppDirectoryList = new char*[m_ulDirsNeeded];
	memset(m_ppDirectoryList, 0, m_ulDirsNeeded);

	// Prepare for our second pass
	pCurr = pStart;

	// Skip the first slash
	if (*pCurr == '/')
	{
	    pCurr++;
	}

	// Each time we encounter a slash, add that directory
	// to our list of directories that need to be created
	while (*pCurr)
	{
	    if (*pCurr == '/')
	    {
	    
		// Add this directory to the list
		CHXString strDir(pStart, pCurr - pStart + 1);
		CHXString strDirUrl;
		ConvertFilepathToURL(m_ProtocolName, strDir, strDirUrl);
			
		m_ppDirectoryList[ulCurrDir] = new char[strDirUrl.GetLength() + 1];
		strcpy(m_ppDirectoryList[ulCurrDir], strDirUrl); /* Flawfinder: ignore */
		
		ulCurrDir++;
	
	    }
	    pCurr++;
	}
    }

    return HXR_OK;
}

HX_RESULT CBaseArchiver2::MakeNuggetFileObject(IHXFileObject* pFileObject,
					       const char* pFileMimeType,
					       IHXFileObject* &pNuggetFileObject)
{
    HX_RESULT retVal = HXR_OUTOFMEMORY;
    CNGTFileObject* pNewNuggetFileObject = new CNGTFileObject();

    if (pNewNuggetFileObject)
    {
	pNewNuggetFileObject->AddRef();
	retVal = pNewNuggetFileObject->InitNugget(m_pContext,
					          pFileObject,
						  HX_FILE_READ | HX_FILE_WRITE | HX_FILE_BINARY,
						  NULL,	    // IHXMetaFileFormatResponse
						  TRUE);    // bForceNew

	if (SUCCEEDED(retVal))
	{
	    UINT32 ulExpiration = 0;
	    UINT32 ulConnectTime = 0;
	    IHXBuffer* pRemoteSourceURLBuffer = NULL;

	    m_pAdviser->GetPropertyULONG32("NuggetExpiration", 
					   ulExpiration);

	    m_pAdviser->GetPropertyCString("NuggetRemoteSourceURL", 
					   pRemoteSourceURLBuffer);

	    m_pAdviser->GetPropertyULONG32("NuggetConnectTime", 
					   ulConnectTime);

	    retVal = pNewNuggetFileObject->ConfigureNugget(
			   ulExpiration,
			   ulConnectTime,
			   m_ulNuggetMinLocalDuration,
			   m_ulMaxDuration,
			   pFileMimeType,
			   pRemoteSourceURLBuffer ? ((const char*) pRemoteSourceURLBuffer->GetBuffer()) : NULL);

	    HX_RELEASE(pRemoteSourceURLBuffer);
	}

	if (SUCCEEDED(retVal))
	{
	    retVal = pNewNuggetFileObject->QueryInterface(IID_IHXFileObject, 
							  (void**) &pNuggetFileObject);
	}

	if (SUCCEEDED(retVal))
	{
	    m_pNuggetFileObject = pNewNuggetFileObject;
	    HX_ADDREF(m_pNuggetFileObject);
	}
    }

    HX_RELEASE(pNewNuggetFileObject);

    return retVal;
}

void CBaseArchiver2::SetNuggetLocalDuration(UINT32 ulNuggetLocalDuration)
{
    if (m_pNuggetFileObject)
    {
	m_pNuggetFileObject->SetNuggetLocalDuration(ulNuggetLocalDuration);
    }
}

HX_RESULT 
CBaseArchiver2::CreateFileObjects()
{
    return HXR_OK;
}

HX_RESULT
CBaseArchiver2::OnDone()
{
    if (m_pNuggetFileObject)
    {
	m_pNuggetFileObject->CloseNugget();
	HX_RELEASE(m_pNuggetFileObject);
    }

    if (m_pFileCreator)
    {
	CFileCreator* pFileCreator = m_pFileCreator;

	m_pFileCreator = NULL;
	pFileCreator->Done();
	pFileCreator->Release();
    }

    return HXR_OK;
}

HX_RESULT
CBaseArchiver2::CreateArchiveVolumeName()
{
    HX_RESULT hResult = HXR_OK;

    // Create a new volume name for the file
    if (m_iFileTime || m_iFileSize)
    {
	if ((!m_bRotateAsFallback) || (m_ulVolumeNo != 0))
	{
	    m_VolumeName.Format("%s%d", (const char*)m_BaseName,
		m_ulVolumeNo);
	}
	else
	{
	    m_VolumeName.Format("%s", (const char*)m_BaseName);
	}
	
	m_ulVolumeNo++;
    }
    else
    {
	m_VolumeName.Format("%s", (const char*)m_BaseName);
    }
    
    // Add the extension
    m_VolumeName += m_AddOnExtension;

    return hResult;
}

HX_RESULT
CBaseArchiver2::CreateTempVolumeName(CHXString& strTempFilename)
{
    HX_RESULT hResult = HXR_OK;

    // Construct temp output filename and convert it to an URL
    if (m_bUseTempFiles)
    {
	strTempFilename.Format("%stemp_%s.tmp", (const char*) m_ArchiveDir, (const char*) m_pVolumeGUIDName);
    }
    else
    {
	strTempFilename.Format("%s%s%s", (const char*) m_ArchiveDir, (const char*) m_FileName, (const char*) m_AddOnExtension);
    }
    
    return hResult;
}

HX_RESULT
CBaseArchiver2::ArchiveDirectoryReady(HX_RESULT status)
{
    // Ignore the status result. We don't care because if we
    // fail to create some or all of the subdirectories needed
    // then we will fail when we try to create the actual
    // archive file, and that failure will be handled properly.
    m_ulDirsCreated++;

    // Create the next archive directory
    CreateDirObjects();

    return HXR_OK;
}

HX_RESULT
CBaseArchiver2::BeginPacketFlow()
{
    // StartPackets
    m_pMonitor->OnPacketsReady(HXR_OK);

    return HXR_OK;
}

HX_RESULT
CBaseArchiver2::InitTargetName(IHXRequest* pRequest)
{
    HXBOOL bAddExtension = TRUE;
    IHXBuffer* pFilePath = NULL;
    IHXBuffer* pFileName = NULL;
    IHXBuffer* pTempDirPath = NULL;
    HX_RESULT retVal = HXR_OK;

    // Get File Path (Archive Directory)
    if (SUCCEEDED(retVal))
    {
	retVal = m_pAdviser->GetPropertyCString("FilePath", 
						pFilePath);
    }

    if (SUCCEEDED(retVal))
    {
	char* pFilePathStr = (char*) pFilePath->GetBuffer();
	m_ArchiveDir = pFilePathStr;

	// Make sure the archive directory ends in a slash
	if ((m_ArchiveDir.GetLength() == 0) ||
	    ((m_ArchiveDir[m_ArchiveDir.GetLength() - 1] != '/') &&
	     (m_ArchiveDir[m_ArchiveDir.GetLength() - 1] != '\\')))
	{
	    m_ArchiveDir += '/';
	}
    }
    else
    {
	retVal = HXR_OK;    // File Path can be null;
    }
    
    // Get File Name
    if (SUCCEEDED(retVal))
    {
	retVal = m_pAdviser->GetPropertyCString("FileName", 
						pFileName);
	
    }

    if (SUCCEEDED(retVal))
    {
	char* pFileNameStr = (char*) pFileName->GetBuffer();
	m_BaseName = m_ArchiveDir + pFileNameStr;
    }

    // Try FullPath if FilePath and FileName not available
    if (FAILED(retVal))
    {
	IHXBuffer* pFilePathName = NULL;
	const char* pFilePathNameStr = NULL;
	char* pNameDelimiter = NULL;

	// Try from adviser first
	retVal = m_pAdviser->GetPropertyCString("FullPath", 
						pFilePathName);

	if (SUCCEEDED(retVal))
	{
	    pFilePathNameStr = (const char*) pFilePathName->GetBuffer();
	}
	else
	{   
	    // See if provided in request directly
	    if (pRequest)
	    {
		retVal = pRequest->GetURL(pFilePathNameStr);

		if (SUCCEEDED(retVal))
		{
		    if (pFilePathNameStr == NULL)
		    {
			retVal = HXR_FAIL;
		    }
		}
	    }
	}

	if (SUCCEEDED(retVal))
	{
	    char* pNameDelimiter2 = NULL;
	    
	    m_BaseName = pFilePathNameStr;
	    pNameDelimiter = (char *)strrchr(pFilePathNameStr, '/');
	    pNameDelimiter2 = (char *)strrchr(pFilePathNameStr, '\\');
	    
	    if (pNameDelimiter == NULL)
	    {
		pNameDelimiter = pNameDelimiter2;
	    }
	    else if (pNameDelimiter2 != NULL)
	    {
		if ((pNameDelimiter2 - pFilePathNameStr) >
		    (pNameDelimiter - pFilePathNameStr))
		{
		    pNameDelimiter = pNameDelimiter2;
		}
	    }
	    
	    if (pNameDelimiter != NULL)
	    {
		m_ArchiveDir = (const char*) m_BaseName;
		m_ArchiveDir.GetBufferSetLength(pNameDelimiter - 
						pFilePathNameStr + 1);
		m_FileName = (pNameDelimiter + 1);
	    }
	    else
	    {
		m_FileName = m_BaseName;
	    }
	}

	if (SUCCEEDED(retVal))
	{
	    const char* pExtension;    
	    
	    // See if extension is present
	    pExtension = strrchr(pFilePathNameStr, '.');
	    if (pExtension &&
		(pExtension != pFilePathNameStr) &&
		((pNameDelimiter == NULL) ||
		 (pExtension > pNameDelimiter)))
	    {
		m_AddOnExtension = pExtension;
		m_BaseName.GetBufferSetLength(pExtension - 
					      pFilePathNameStr);
		if (pNameDelimiter != NULL)
		{
		    m_FileName.GetBufferSetLength(pExtension - 
						  (pNameDelimiter + 1));
		}
		else
		{
		    m_FileName.GetBufferSetLength(pExtension - 
						  pFilePathNameStr);
		}

		bAddExtension = FALSE;
	    }
	}

	HX_RELEASE(pFilePathName);
    }

    if (SUCCEEDED(retVal) && (m_ProtocolName.GetLength() == 0))
    {
	IHXBuffer* pProtocolName = NULL;

	// Try to obtain protocol from the adviser
	retVal = m_pAdviser->GetPropertyCString("Protocol", 
						pProtocolName);

	if (SUCCEEDED(retVal))
	{
	    char* pProtocolNameStr = (char*) pProtocolName->GetBuffer();
	    m_ProtocolName = pProtocolNameStr;
	}

	HX_RELEASE(pProtocolName);

	retVal = HXR_OK;    // Some file system managers are not protocol driven
    }

    // See if a separate directory is to be used for temporary files
    if (SUCCEEDED(retVal))
    {
	retVal = m_pAdviser->GetPropertyCString("TempDirPath", 
						pTempDirPath);

	if (SUCCEEDED(retVal) && pTempDirPath)
	{
	    m_ArchiveDir = ((char*) pTempDirPath->GetBuffer());

	    // Make sure the archive directory ends in a slash
	    if ((m_ArchiveDir.GetLength() == 0) ||
		((m_ArchiveDir[m_ArchiveDir.GetLength() - 1] != OS_SEPARATOR_CHAR)))
	    {
		m_ArchiveDir += OS_SEPARATOR_CHAR;
	    }

	    m_bTempDirUsed = TRUE;
	}

	HX_RELEASE(pTempDirPath);

	retVal = HXR_OK;
    }

    // Form add-on file name extension if needed
    if (SUCCEEDED(retVal) && bAddExtension)
    {
	IHXBuffer* pExtensionBuffer = NULL;

	if (SUCCEEDED(m_pAdviser->GetPropertyCString("FileExtension",
						     pExtensionBuffer)) &&
	    pExtensionBuffer && 
	    pExtensionBuffer->GetBuffer())
	{
	    m_AddOnExtension = ((const char*) pExtensionBuffer->GetBuffer());
	}
	else
	{
	    m_AddOnExtension = ".rm";
	}

	HX_RELEASE(pExtensionBuffer);
    }

    if (SUCCEEDED(retVal))
    {
#ifdef _MACINTOSH
	// Convert legacy Mac-style paths to POSIX-style paths
	if (m_BaseName.Find('/') == -1)
	{
		CHXFileSpecifier fsBaseName = m_BaseName;
		m_BaseName = fsBaseName.GetPOSIXPath();
	}

	if (m_ArchiveDir.Find('/') == -1)
	{
		CHXFileSpecifier fsArchiveDir = m_ArchiveDir;
		m_ArchiveDir = fsArchiveDir.GetPOSIXPath();

	    if ((m_ArchiveDir.GetLength() == 0) ||
			((m_ArchiveDir[m_ArchiveDir.GetLength() - 1] != '/')))
	    {
			m_ArchiveDir += '/';
	    }
	}
#endif
    
	NormalizePath(&m_BaseName);
	NormalizePath(&m_ArchiveDir);
    }

    HX_RELEASE(pFileName);
    HX_RELEASE(pFilePath);

    return retVal;
}

void CBaseArchiver2::NormalizePath(CHXString* pPath)
{
    ULONG32 ulLength = pPath->GetLength();
    char* pPathData = pPath->GetBuffer(0);

    if (ulLength > 0)
    {
	while (*pPathData != '\0')
	{
	    if (*pPathData == OS_SEPARATOR_CHAR)
	    {
		*pPathData = '/';
	    }

	    pPathData++;
	}
    }
}

HX_RESULT 
CBaseArchiver2::UpdateStatusULONG32(HX_RESULT status,
				   const char* pValueName,
				   ULONG32 ulValue)
{
    IHXValues* pStatus = NULL;
    HX_RESULT retVal;

    retVal = m_pClassFactory->CreateInstance(IID_IHXValues, 
					     (void**) &pStatus);

    if (SUCCEEDED(retVal))
    {
	retVal = pStatus->SetPropertyULONG32(pValueName, ulValue);
	
    }

    if (SUCCEEDED(retVal))
    {
	retVal = m_pMonitor->OnStatus(status, pStatus);
    }
    else
    {
	retVal = m_pMonitor->OnStatus(HXR_OUTOFMEMORY, NULL);
    }

    HX_RELEASE(pStatus);

    return retVal;
}


HX_RESULT 
CBaseArchiver2::UpdateStatus2ULONG32(HX_RESULT status,
				     const char* pValueName1,
				     ULONG32 ulValue1,
				     const char* pValueName2,
				     ULONG32 ulValue2)
{
    IHXValues* pStatus = NULL;
    HX_RESULT retVal;

    retVal = m_pClassFactory->CreateInstance(IID_IHXValues, 
					     (void**) &pStatus);

    if (SUCCEEDED(retVal))
    {
	retVal = pStatus->SetPropertyULONG32(pValueName1, ulValue1);
	
    }

    if (SUCCEEDED(retVal))
    {
	retVal = pStatus->SetPropertyULONG32(pValueName2, ulValue2);
	
    }

    if (SUCCEEDED(retVal))
    {
	retVal = m_pMonitor->OnStatus(status, pStatus);
    }
    else
    {
	retVal = m_pMonitor->OnStatus(HXR_OUTOFMEMORY, NULL);
    }

    HX_RELEASE(pStatus);

    return retVal;
}


HXBOOL
CBaseArchiver2::MimeTypeOK(const char* pMimeType)
{
    /*
     * Only allow streams we know about
     */
    return stricmp(pMimeType, REALAUDIO_MIME_TYPE) == 0 ||
	   stricmp(pMimeType, REALAUDIO_MULTIRATE_LIVE_MIME_TYPE) == 0 ||
           stricmp(pMimeType, REALVIDEO_MIME_TYPE) == 0 ||
           stricmp(pMimeType, IMAGEMAP_MIME_TYPE) == 0 ||
           stricmp(pMimeType, SYNCMM_MIME_TYPE) == 0 ||
	   stricmp(pMimeType, REALEVENT_MIME_TYPE) == 0	||
	   stricmp(pMimeType, REALIMAGEMAP_MIME_TYPE) == 0 ||
	   stricmp(pMimeType, REALAUDIO_ENCRYPTED_MIME_TYPE) == 0 ||
	   stricmp(pMimeType, REALVIDEO_ENCRYPTED_MIME_TYPE) == 0 ||
	   stricmp(pMimeType, IMAGEMAP_ENCRYPTED_MIME_TYPE) == 0 ||
	   stricmp(pMimeType, SYNCMM_ENCRYPTED_MIME_TYPE) == 0	||
	   stricmp(pMimeType, REALEVENT_ENCRYPTED_MIME_TYPE) == 0 ||
	   stricmp(pMimeType, REALIMAGEMAP_ENCRYPTED_MIME_TYPE) == 0 ||
	   stricmp(pMimeType, REALAUDIO_ENCRYPTED_MULTIRATE_MIME_TYPE) == 0 ||
	   stricmp(pMimeType, REALAUDIO_ENCRYPTED_MULTIRATE_LIVE_MIME_TYPE) == 0 ||
	   stricmp(pMimeType, REALVIDEO_ENCRYPTED_MULTIRATE_MIME_TYPE) == 0 ||
	   stricmp(pMimeType, REALEVENT_ENCRYPTED_MULTIRATE_MIME_TYPE) == 0 ||
	   stricmp(pMimeType, REALIMAGEMAP_ENCRYPTED_MULTIRATE_MIME_TYPE) == 0;
}

HXBOOL
CBaseArchiver2::ReadyToArchive()
{
    // Return TRUE if we have received all stream headers and the
    // File Creator has been successfully initialized
    if (m_ulNumStreams &&
	(m_ulActiveStreams == m_ulNumStreams && m_bFileCreatorReady))
    {
	return TRUE;
    }

    return FALSE;
}


void
CBaseArchiver2::HandleStoredPackets()
{
    CHXSimpleList::Iterator i;

    m_bHandlingStoredPackets = TRUE;

    for (i = m_StoredPackets.Begin(); i != m_StoredPackets.End(); ++i)
    {
	IHXPacket* pPacket = (IHXPacket*)(*i);
	OnNewPacket(pPacket);
	HX_RELEASE(pPacket);
    }

    m_StoredPackets.RemoveAll();

    m_bHandlingStoredPackets = FALSE;
}


HX_RESULT
CBaseArchiver2::ExtractFileMetaInfo(IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_OK;

    // These specific fields are known to NOT be meta information. If they
    // are present in the file header, we will simply ignore them. All other
    // values will be added to the archived file as meta information.
    static const char* ppRestrictedFields[] =
    {
	"Title",
	"Author",
	"Copyright",
	"Comment",
	"Flags",
	"AvgBitRate",
	"AvgPacketSize",
	"Preroll",
	"ActualPreroll",
	"StreamCount",
	"Duration",
	"MaxPacketSize",
	"MaxBitRate",
	"NumInterleavePackets",
	"AllAudioCodecs",
	"AllVideoCodecs",
	"AllFrameRates",
	"Width",
	"Height",
	"IsRealDataType",
	"EndTime",
	"IndexOffset",
	"DataOffset",
	"LatencyMode",
	"LiveStream",
	"NuggetExpiration",
	"NuggetConnectTime",
	"NuggetLocalDuration",
	"NuggetRemoteSourceURL",
	"NuggetOverallDuration",
	"RecordedAsLive"
	// "Creation Date",
	// "Modification Date"
    };

    HX_RELEASE(m_pMetaInfo);

    retVal = ExtractMetaInfo(pHeader, 
			     m_pMetaInfo,
			     ppRestrictedFields,
			     sizeof(ppRestrictedFields) / sizeof(char*),
			     m_pContext);

    if (SUCCEEDED(retVal) && m_pMetaInfo && m_bRecordAsLive)
    {
	m_pMetaInfo->SetPropertyULONG32("RecordedAsLive", 1);
    }

    if (SUCCEEDED(retVal))
    {
	OnNewMetaInfo(m_pMetaInfo);
    }

    return retVal;
}


HX_RESULT
CBaseArchiver2::ExtractMetaInfo(IHXValues* pHeader,
				IHXValues* &pMetaInfo,
				const char** ppRestrictedFields,
				ULONG32 ulNumRestrictedFields,
				IUnknown* pContext)
{
    HX_RESULT hResult = HXR_OK;
    const char* pPropName = NULL;
    IHXBuffer* pBuffer = NULL;
    UINT32 ulValue = 0;
    HXBOOL bRestricted = FALSE;
    UINT32 i = 0;

    HX_ASSERT(pHeader);

    if (pMetaInfo == NULL)
    {
	CreateValuesCCF(pMetaInfo, pContext);
    }

    // Iterate through all CStrings in the header
    hResult = pHeader->GetFirstPropertyCString(pPropName, pBuffer);
    while (SUCCEEDED(hResult))
    {
	bRestricted = FALSE;

	for (i = 0; i < ulNumRestrictedFields; i++)
	{
	    if (!strcasecmp(pPropName, ppRestrictedFields[i]))
	    {   
		bRestricted = TRUE;
		break;
	    }
	}

	// If this value is one that we recognize as NOT being meta 
	// information, don't add it to the new IHXValues. Otherwise,
	// we will assume it's meta info and add it to the archive
	if (!bRestricted)
	{
	    pMetaInfo->SetPropertyCString(pPropName, pBuffer);
	}
	pBuffer->Release();
	hResult = pHeader->GetNextPropertyCString(pPropName, pBuffer);
    }

    // Iterate through all ULONG32s in the header
    hResult = pHeader->GetFirstPropertyULONG32(pPropName, ulValue);
    while (SUCCEEDED(hResult))
    {
	bRestricted = FALSE;

	for (i = 0; i < ulNumRestrictedFields; i++)
	{
	    if (!strcasecmp(pPropName, ppRestrictedFields[i]))
	    {
		bRestricted = TRUE;
		break;
	    }
	}

	// If this value is one that we recognize as NOT being meta 
	// information, don't add it to the new IHXValues. Otherwise,
	// we will assume it's meta info and add it to the archive
	if (!bRestricted)
	{
	    pMetaInfo->SetPropertyULONG32(pPropName, ulValue);
	}
	hResult = pHeader->GetNextPropertyULONG32(pPropName, ulValue);
    }

    // Iterate through all Buffers in the header
    hResult = pHeader->GetFirstPropertyBuffer(pPropName, pBuffer);
    while (SUCCEEDED(hResult))
    {
	bRestricted = FALSE;

	for (i = 0; i < ulNumRestrictedFields; i++)
	{
	    if (!strcasecmp(pPropName, ppRestrictedFields[i]))
	    {   
		bRestricted = TRUE;
		break;
	    }
	}

	// If this value is one that we recognize as NOT being meta 
	// information, don't add it to the new IHXValues. Otherwise,
	// we will assume it's meta info and add it to the archive
	if (!bRestricted)
	{
	    pMetaInfo->SetPropertyBuffer(pPropName, pBuffer);
	}
	pBuffer->Release();
	hResult = pHeader->GetNextPropertyBuffer(pPropName, pBuffer);
    }

    return HXR_OK;
}

HXBOOL
CBaseArchiver2::IsSetEmpty(IHXValues* pValues)
{
    ULONG32 ulVal;
    IHXBuffer* pBuffer = NULL;
    IHXBuffer* pString = NULL;
    const char* pName;
    HXBOOL bIsEmpty = TRUE;

    if (pValues &&
	(SUCCEEDED(pValues->GetFirstPropertyULONG32(pName, ulVal)) ||
	 SUCCEEDED(pValues->GetFirstPropertyCString(pName, pString)) ||
	 SUCCEEDED(pValues->GetFirstPropertyBuffer(pName, pBuffer))))
    {
	bIsEmpty = FALSE;
    }

    HX_RELEASE(pBuffer);
    HX_RELEASE(pString);

    return bIsEmpty;
}

HX_RESULT
CBaseArchiver2::DumpValues(IHXValues* pValues)
{
    HX_RESULT	hResult = HXR_OK;
    const char* pName	= NULL;
    IHXBuffer* pValue	= NULL;
    ULONG32	ulValue = 0;

    // Dump all CStrings
    hResult = pValues->GetFirstPropertyCString(pName, pValue);
    while (hResult == HXR_OK)
    {
	printf("CString %s: %s\n", pName, (const char*)pValue->GetBuffer());
	HX_RELEASE(pValue);

	hResult = pValues->GetNextPropertyCString(pName, pValue);
    }

    // Dump all ULONG32s
    hResult = pValues->GetFirstPropertyULONG32(pName, ulValue);
    while (hResult == HXR_OK)
    {
	printf("ULONG32 %s: %ld\n", pName, ulValue);

	hResult = pValues->GetNextPropertyULONG32(pName, ulValue);
    }

    // Dump all Buffers
    hResult = pValues->GetFirstPropertyBuffer(pName, pValue);
    while (hResult == HXR_OK)
    {
	printf("Buffer %s: Contents...\n", pName);
	HX_RELEASE(pValue);

	hResult = pValues->GetNextPropertyBuffer(pName, pValue);
    }

    return HXR_OK;
}


ULONG32
CBaseArchiver2::BufferStringLength(IHXBuffer* pBuffer)
{
    const char* pEnd;
    ULONG32 ulLength = 0;

    if (pBuffer)
    {
	ulLength = pBuffer->GetSize();
	pEnd = StrNChr((const char*) pBuffer->GetBuffer(), '\0', ulLength);

	if (pEnd)
	{
	    ulLength = pEnd - ((const char*) pBuffer->GetBuffer());
	}
    }

    return ulLength;
}


IHXBuffer*
CBaseArchiver2::CreateTrimmedBuffer(IHXBuffer* pBuffer, IUnknown* pContext)
{
    IHXBuffer* pRetBuffer = NULL;

    if (pBuffer)
    {
	ULONG32 ulSize = BufferStringLength(pBuffer);

	if (ulSize > 0)
	{
	    IHXBuffer* pNewBuffer = NULL;
	    HX_RESULT retVal = HXR_OK;

	    retVal = CreateBufferCCF(pNewBuffer, pContext);
	    if (SUCCEEDED(retVal))
	    {	
		retVal = pNewBuffer->SetSize(ulSize);
	    }

	    if (SUCCEEDED(retVal))
	    {	
		memcpy(pNewBuffer->GetBuffer(), pBuffer->GetBuffer(), ulSize); /* Flawfinder: ignore */
		pRetBuffer = (IHXBuffer*) pNewBuffer;
		pRetBuffer->AddRef();
	    }

	    HX_RELEASE(pNewBuffer);
	}
    }

    return pRetBuffer;
}


/****************************************************************************
 *  GenGUID
 */
HX_RESULT CBaseArchiver2::GenGUIDFileName(char* pFileName)
{
    ULONG32 ulGUIDSize = sizeof(uuid_tt);
    CHXuuid* p_uuIDFactory = NULL;
    uuid_tt uuID;
    HX_RESULT retVal = HXR_OK;

    HX_ASSERT(pFileName);

    p_uuIDFactory = new CHXuuid();
    
    retVal = HXR_OUTOFMEMORY;
    if (p_uuIDFactory)
    {
	retVal = p_uuIDFactory->GetUuid(&uuID);
    }
       
    HX_DELETE(p_uuIDFactory);

    if (SUCCEEDED(retVal))
    {
	LONG32 l64Size = BinToURL64((UINT8*) &uuID, 
				    ulGUIDSize,
				    pFileName);
	
	retVal = HXR_FAIL;
	if (l64Size > 1)
	{
	    retVal = HXR_OK;
	}
    }

    if (SUCCEEDED(retVal))
    {
	Hash64ToFileName(pFileName);
    }
    
    return retVal;
}


/****************************************************************************
 *  Hash64ToFileName
 */
void CBaseArchiver2::Hash64ToFileName(char* pHash64)
{
    HX_ASSERT(pHash64);

    while (*pHash64 != '\0')
    {
	if ((*pHash64 == '+') ||
	    (*pHash64 == '!'))
	{
	    *pHash64 = '-';
	}
	else if ((*pHash64 == '/') ||
	         (*pHash64 == '*'))
	{
	    *pHash64 = '_';
	}

	pHash64++;
    }
}


/****************************************************************************
 *  Hash64ToFileName
 */
const char* CBaseArchiver2::FileObjectToName(IHXFileObject* pFileObject)
{
    const char* pUnknFilename = "unknown name file";
    const char* pFilename = pUnknFilename;
    
    if ((pFileObject == NULL) ||
	FAILED(pFileObject->GetFilename(pFilename)) || 
	(pFilename == NULL))
    {
	pFilename = pUnknFilename;
    }

    return pFilename;
}


/****************************************************************************
 *  UpdateStreamVersion
 */
HX_RESULT CBaseArchiver2::UpdateStreamVersion(IHXValues* pHeader, 
						     UINT8 uMajor,
						     UINT8 uMinor)
{
    ULONG32 ulCurrentVersion = 0;
    ULONG32 ulUpdateVersion = ((uMajor & 0xF << 28) | (uMinor << 20));
    HX_RESULT retVal = HXR_OK;

    HX_ASSERT(pHeader);

    pHeader->GetPropertyULONG32("StreamVersion", ulCurrentVersion);

    if (ulUpdateVersion > ulCurrentVersion)
    {
	ulUpdateVersion |= (ulCurrentVersion & 0x000FFFFF);
	retVal = pHeader->SetPropertyULONG32("StreamVersion", 
					     ulUpdateVersion);
    }

    return retVal;
}


/****************************************************************************
 *  ConvertFilepathToURL
 */

HX_RESULT CBaseArchiver2::ConvertFilepathToURL(const char* szProtocol, const char* szFilePath, CHXString& strUrl)
{
	HX_RESULT res = HXR_OK;

	// Validate params
	if (!szFilePath)
	{
		HX_ASSERT(FALSE);
		return HXR_POINTER;
	}
	
#ifdef _CARBON
	if (szProtocol && strcmp("file", szProtocol) == 0)
	{
		URLFromPOSIXPath(szFilePath, strUrl);		
	}
	else 
#endif	
	if (szProtocol && *szProtocol != '\0')
	{
	    strUrl.Format("%s://%s", (const char*) szProtocol, (const char*) szFilePath); 
	}
	else
	{
		strUrl = szFilePath;
	}

	return res;
}


HX_RESULT
CBaseArchiver2::NotifyTempFileCreated(const char* pszTempFileName)
{
	//	Need to notify clients of the writer of the name of the temp file
	//	This is a feature for the player team that enables them to play
	//	back the temp files while encoding/writing the files.
	HX_RESULT res = HXR_OK;

	if(pszTempFileName)
	{		
		IHXValues* pStatusInfo = NULL;

		CreateValuesCCF(pStatusInfo, m_pClassFactory);
		HX_ASSERT(pStatusInfo);

		if(pStatusInfo)
		{
			IHXBuffer* pBufTempFile = NULL;
			CreateBufferCCF(pBufTempFile, m_pContext);
			HX_ASSERT(pBufTempFile);
			if(pBufTempFile)
			{
				res = pBufTempFile->Set((UCHAR*)pszTempFileName, strlen(pszTempFileName) + 1);
				HX_ASSERT(SUCCEEDED(res));
				if(SUCCEEDED(res))
				{
					res = pStatusInfo->SetPropertyCString("TempFileName", pBufTempFile);
					HX_ASSERT(SUCCEEDED(res));
					if(SUCCEEDED(res))
					{
						//	Send back the name of the temp file created
						m_pMonitor->OnStatus(HXR_OK, pStatusInfo);
					}
				}

				HX_RELEASE(pBufTempFile);
			}
			else
			{
				res = HXR_OUTOFMEMORY;
			}

			HX_RELEASE(pStatusInfo);
		}
		else
		{
			res = HXR_OUTOFMEMORY;
		}
	}
	else
	{
		res = HXR_POINTER;
	}

	return res;
}




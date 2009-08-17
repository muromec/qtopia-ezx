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
// mp4arch.cpp

#include <stdio.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxstrutl.h"
#include "errmsg_macros.h"
#include "hxbuffer.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "archivr2.h"

#include "mp4arch.h"
#include "mp4atoms.h"

inline void
ErrMsg(IHXErrorMessages* pErrorMessages, 
       HX_RESULT	ulHXCode,
       const ULONG32	ulUserCode,
       const char* fmt, ...)
{
    if (!pErrorMessages)
    {
	return;
    }

    va_list args;
    char buf[4096]; /* Flawfinder: ignore */

    va_start(args, fmt);
    vsprintf(buf, fmt, args); /* Flawfinder: ignore */
    pErrorMessages->Report(HXLOG_ERR,
                           ulHXCode,
                           ulUserCode,
                           buf,
                           NULL);
    va_end(args);
}


// 
//  Class: CMP4Archiver
//
CMP4Archiver::CMP4Archiver(IUnknown* pContext, 
				 IHXFileWriterMonitor* pMonitor, 
				 IHXPropertyAdviser* pAdviser)
    : CBaseArchiver2(pContext, pMonitor, pAdviser)
    , m_pSaveFile       (NULL)
    , m_ulFileOffset    (NULL)
    , m_pOutputFileRenamer(NULL)
{
    m_iFileTime = m_iFileSize = 0;
    m_bAborted = FALSE;
}

CMP4Archiver::~CMP4Archiver()
{
    CloseSaveFile();
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
CMP4Archiver::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
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
CMP4Archiver::AddRef()
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
CMP4Archiver::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT
CMP4Archiver::OnAbort()
{
    HX_RESULT retVal = HXR_OK;

    m_bAborted = TRUE;

    m_pSaveFile->Close();
    HX_RELEASE(m_pSaveFile);
    return retVal;
}

HX_RESULT 
CMP4Archiver::OnDone()
{
    m_bDone = TRUE;

    CloseSaveFile();

    return HXR_OK;
}

HX_RESULT
CMP4Archiver::OnNewMetaInfo(IHXValues* pMetaInfo)
{
    HX_RESULT retVal = HXR_OK;
    return retVal;
}

HX_RESULT
CMP4Archiver::OnNewFileHeader(IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_OK;
    return retVal;
}

HX_RESULT
CMP4Archiver::OnNewStreamHeader(IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_OK;
    return retVal;
}

#include "chxpckts.h"

HX_RESULT
CMP4Archiver::OnNewPacket(IHXPacket* pPacket)
{
    HX_RESULT	retVal = HXR_FAIL;

    if( pPacket )
    {
	IHXBuffer* pBuffer = pPacket->GetBuffer();
	retVal = WriteBuffer( pBuffer );
	HX_RELEASE( pBuffer );
    }
    return retVal;
}
HX_RESULT
CMP4Archiver::WriteAtom( CMP4Atom* pAtom, BOOL bIncludeChildren )
{
    HX_RESULT retVal = HXR_OUTOFMEMORY;

    UINT32 ulCurrentSize = pAtom->GetCurrentSize( bIncludeChildren );
    UCHAR* pBuffer, *pTmp;
    pBuffer = pTmp = new UCHAR[ ulCurrentSize ];

    if( pBuffer )
    {
	// XXX this fails if the file is at 0 and the atom is written to non 0. i.e. it shouldn't fail
	UINT32 ulWriteOffset = m_ulFileOffset;
	UINT32 ulSaveOffset  = 0;

	// We need two pointers here since WriteToBuffer takes a reference
	retVal = pAtom->WriteToBuffer( pTmp, bIncludeChildren );
	
	// If this atom already exists somewhere in the file, seek to that point and
	// write there.
	if( !pAtom->IsFirstWrite() && ulWriteOffset != pAtom->GetLastWriteOffset() )
	{
	    m_pSaveFile->Seek( pAtom->GetLastWriteOffset(), FALSE );
	    ulSaveOffset = m_ulFileOffset;
	    m_ulFileOffset = ulWriteOffset = pAtom->GetLastWriteOffset();
	}

	if( SUCCEEDED( retVal ) )
	{
	    retVal = HXR_OUTOFMEMORY;

	    // Write out, and notify the atom; this only notifies the root atom
	    IHXBuffer* pHXBuffer = NULL;
	    CreateBufferCCF(pHXBuffer, m_pContext);
	    if( pHXBuffer )
	    {
		retVal = pHXBuffer->Set( pBuffer, ulCurrentSize );
		if( SUCCEEDED( retVal ) )
		{
		    retVal = WriteBuffer( pHXBuffer );
		}

		pHXBuffer->Release();
	    }
	}

	if( SUCCEEDED( retVal ) )
	{
	    pAtom->NotifyWriteOffset( ulWriteOffset );
	}

	// Return to our last position
	if( SUCCEEDED( retVal ) && ulSaveOffset )
	{
	    m_pSaveFile->Seek( ulSaveOffset, FALSE );
	    m_ulFileOffset = ulSaveOffset;
	}
	
	delete [] pBuffer;
    }
    
   return retVal;
}
HX_RESULT
CMP4Archiver::WriteBuffer( IHXBuffer* pBuffer )
{
    HX_RESULT retVal = m_pSaveFile->Write( pBuffer );

    if( SUCCEEDED(retVal) )
    {
	m_ulFileOffset += pBuffer->GetSize();
    }
    return retVal;
}

HX_RESULT 
CMP4Archiver::CreateFileObjects()
{
    CloseSaveFile();

    m_bResolvedOutputConflict = FALSE;
    m_pVolumeGUIDName[0] = '\0';
    GenGUIDFileName(m_pVolumeGUIDName);

    // Create the archive file
    CreateArchiveFileObject();

    return HXR_OK;
}
    
HX_RESULT
CMP4Archiver::CreateArchiveFileObject()
{
    HX_RESULT hResult = HXR_OK;

    CreateArchiveVolumeName();

    AddRef();

    // Add this archiver instance to the registry, 
    // along with filename and status information
    m_pMonitor->OnVolumeInitiation(HXR_OK,
				   (const char*) m_VolumeName,
				   m_ulArchiverID);

    if (!m_bAborted)
    {
    	// Construct output file name and convert it to an URL
 	ConvertFilepathToURL(m_ProtocolName, m_VolumeName, m_OutputFileName);
 
	// Construct temp output filename and convert it to an URL
	CHXString strTempFilename;
	CreateTempVolumeName(strTempFilename);
	ConvertFilepathToURL(m_ProtocolName, strTempFilename, m_TempOutputFileName);

	// Actually create the file
	hResult = m_pFileCreator->CreateArchiveFile(
		    (const char*) m_TempOutputFileName);

	if (HXR_OK == hResult)
	{
	    if (m_TempOutputFileName != m_OutputFileName)
	    {
		HX_RESULT res2 = NotifyTempFileCreated(strTempFilename);
		HX_ASSERT(SUCCEEDED(res2));
	    }
	}
	else
	{
	    m_pMonitor->OnCompletion(hResult);
	}
    }

    Release();

    return hResult;
}

HX_RESULT 
CMP4Archiver::ArchiveDirReady(HX_RESULT status)
{
    if (m_DirStatus == DIR_CREATING)
    {
	// This callback belongs to our base class
	ArchiveDirectoryReady(status);
	return HXR_OK;
    }

    if (HXR_OK != status)
    {
	ErrMsg(m_pErrorMessages,
	       status,
	       2,
	       "\"%s\" is not a valid path. Unable to archive live data!\n", 
	       (const char*)m_VolumeName);
	m_pMonitor->OnCompletion(status);
	return HXR_OK;
    }

    // Directory was created successfully, so go ahead and create 
    // the archive file itself
    CreateArchiveFileObject();

    return HXR_OK;
}

HX_RESULT 
CMP4Archiver::ArchiveFileReady(HX_RESULT status, 
				  IHXFileObject* pFileObject)
{
    if (m_bClosing)
    {
	if (!m_bResolvedOutputConflict)
	{
	    // This must be the file object created to clear the way
	    // for the real output file
	    m_bResolvedOutputConflict = TRUE;
	    HX_RELEASE(m_pOutputFileRenamer);

	    if (SUCCEEDED(status))
	    {
		// Create the file object to be used for renaming from
		// temporary output file name to intended output file name
		if (SUCCEEDED(m_pFileCreator->CreateArchiveFile(
				(const char*) m_TempOutputFileName, FALSE)))
		{
		    // We'll be called back with the created file object
		    return HXR_OK;
		}
	    }
	}
	else
	{
	    if (SUCCEEDED(status) && pFileObject)
	    {
		m_pOutputFileRenamer = pFileObject;
		pFileObject->AddRef();
	    }
	}
	
	CloseSaveFile();
    }
    else if ((m_ARStatus == AR_INITIALIZING) ||
	     (m_ARStatus == AR_ROTATING))
    {
	if (HXR_OK != status)
	{
	    ErrMsg(m_pErrorMessages,
		   status,
		   3,
		   "\"%s\" is not a valid path. Unable to start writing file!\n", 
		   (const char*) m_VolumeName);
	    m_pMonitor->OnCompletion(status);
	    return HXR_OK;
	}
	pFileObject->AddRef();
	m_pSaveFile = pFileObject;
	m_pSaveFile->Init( HX_FILE_WRITE | HX_FILE_BINARY, (IHXFileResponse*)&m_StubResponse);

	// Begin archiving to the new save file
	if (m_ARStatus == AR_INITIALIZING)
	{
	    m_ARStatus = AR_READY;
	    
	    // We are just starting to archive
	    BeginPacketFlow();
	}
    }

    return HXR_OK;
}

void
CMP4Archiver::NotifyAbruptEnd(BOOL bAbruptEnd)
{
}


void
CMP4Archiver::CloseSaveFile()
{
    HXBOOL bSkipRename = FALSE;

    m_bClosing = TRUE;

    if (m_pSaveFile)
    {
	HX_RESULT hResult = HXR_OK;

	if (!m_bResolvedOutputConflict)
	{
	    if (m_TempOutputFileName != m_OutputFileName)
	    {
		// Create a file with the OutputFilename,
		// so any file which already has that
		// name will be renamed appropriately
		hResult = m_pFileCreator->CreateArchiveFile(
				(const char*) m_OutputFileName);
		
		if (SUCCEEDED(hResult))
		{
		    // We'll be called back once the conflict is resolved
		    return;
		}
	    }

	    // If we are here, temporary file name is the final output file name
	    // and thus there cannot be any output name conflict.
	    m_bResolvedOutputConflict = TRUE;
	    // since temp file name and final output name are the same, 
	    // no rename is needed
	    bSkipRename = TRUE;
	}

	m_pSaveFile->Close();
	HX_RELEASE(m_pSaveFile);

	if (!bSkipRename)
	{
	    if (SUCCEEDED(hResult))
	    {
		hResult = HXR_FAIL;
		if (m_pOutputFileRenamer)
		{
		    hResult = HXR_OK;
		}
	    }
	    
	    if (SUCCEEDED(hResult))
	    {
		m_pFileCreator->RenameObject(m_pOutputFileRenamer, 
					     m_OutputFileName,
					     m_bTempDirUsed);
	    }
	    else
	    {
		ErrMsg(m_pErrorMessages,
		       HXR_IGNORE,
		       1,
		       "Unable to move \"%s\" out of the way for \"%s\"!\n",
		       (const char*) m_OutputFileName,
		       (const char*) m_TempOutputFileName);
	    }
	}

	HX_RELEASE(m_pOutputFileRenamer);

	// Remove all registry keys for this archiver
	AddRef();
	m_pMonitor->OnVolumeCompletion(HXR_OK, m_ulArchiverID);
	if (m_bDone)
	{
	    CBaseArchiver2::OnDone();
	    m_pMonitor->OnCompletion(HXR_OK);
	}
	Release();
    }

    m_ulFileOffset = 0;
    m_bClosing = FALSE;
}


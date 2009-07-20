/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cchx2ihxdataf.cpp,v 1.9 2006/02/07 19:21:11 ping Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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
 *  Includes
 */
#include "hlxclib/string.h"
#include "hlxclib/fcntl.h"
#include "hlxclib/sys/stat.h"

#include "hxbuffer.h"
#include "hxassert.h"

#include "chxdataf.h"
#include "cchx2ihxdataf.h"
#include "pckunpck.h"


/****************************************************************************
 *  CCHX2IHXDataFile
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CCHX2IHXDataFile::CCHX2IHXDataFile(IUnknown* pContext,
				   IHXDataFile* pIHXFile,
				   void* pUserData,
				   TemporaryFileNameFnPtr fpTempFileName,
				   FastReadFnPtr fpFastRead,
				   FastReadFnPtr fpFastWrite,
				   FileDeleteFnPtr fpFileDelete)
    : CHXDataFile(pContext)
    , m_pIHXFile(pIHXFile)
    , m_fpFileDelete(fpFileDelete)
    , m_fpTempFileName(fpTempFileName)
    , m_fpFastRead(fpFastRead)
    , m_fpFastWrite(fpFastWrite)
    , m_pUserData(pUserData)
{
    if (m_pIHXFile)
    {
	m_pIHXFile->AddRef();
    }
}

CCHX2IHXDataFile::~CCHX2IHXDataFile(void)
{
    HX_RELEASE(m_pIHXFile);
}


/****************************************************************************
 *  CHXDataFile methods
 */
HX_RESULT CCHX2IHXDataFile::GetLastError(void)
{
    HX_RESULT retVal = HXR_OK;

    if (m_pIHXFile)
    {
	retVal = m_pIHXFile->GetLastError();
    }

    return retVal;
}

HXBOOL CCHX2IHXDataFile::GetTemporaryFileName(const char *tag, char* name, UINT32 ulBufLen)
{
    HXBOOL bRetVal = FALSE;

    if (m_fpTempFileName)
    {
	bRetVal = (*m_fpTempFileName)(m_pUserData, tag, name, ulBufLen);
    }

    return bRetVal;
}

HX_RESULT CCHX2IHXDataFile::Create(const char *filename, UINT16 mode, HXBOOL textflag)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pIHXFile)
    {
	m_pIHXFile->Bind(filename);

	retVal = m_pIHXFile->Create(TranslateMode(mode, textflag));
    }

    return retVal;
}

HX_RESULT CCHX2IHXDataFile::Open(const char *filename, UINT16 mode, HXBOOL textflag)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pIHXFile)
    {
	m_pIHXFile->Bind(filename);

	retVal = m_pIHXFile->Open(TranslateMode(mode, textflag));
    }

    return retVal;
}

HX_RESULT CCHX2IHXDataFile::Close(void)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pIHXFile)
    {
	retVal = m_pIHXFile->Close();
    }

    return retVal;
}

ULONG32 CCHX2IHXDataFile::GetSize(void)
{
    ULONG32 ulSize = 0;

    if (m_pIHXFile)
    {
	struct stat statBuffer;

	if (m_pIHXFile->IsOpen())
	{
	    ULONG32 ulCurrPos = m_pIHXFile->Tell();

	    ulSize = m_pIHXFile->Seek(0, SEEK_END);

	    m_pIHXFile->Seek(ulCurrPos, SEEK_SET);
	}
	else if (m_pIHXFile->Stat(&statBuffer) == HXR_OK)
	{
	    ulSize = statBuffer.st_size;
	}
    }

    return ulSize;
}

HX_RESULT CCHX2IHXDataFile::Seek(ULONG32 offset, UINT16 fromWhere)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pIHXFile)
    {
	retVal = m_pIHXFile->Seek(offset, fromWhere);
    }

    return retVal;
}

ULONG32 CCHX2IHXDataFile::Tell(void)
{
    ULONG32 ulPos = 0;

    if (m_pIHXFile)
    {
	ulPos = m_pIHXFile->Tell();
    }

    return ulPos;
}

ULONG32 CCHX2IHXDataFile::Read(char* buf, ULONG32 count)
{
    ULONG32 ulBytesRead = 0;

    if (m_pIHXFile)
    {
	if (m_fpFastRead)
	{
	    ulBytesRead = m_fpFastRead(m_pUserData, buf, count);
	}
	else
	{
	    IHXBuffer* pBuffer = NULL;

	    ulBytesRead = m_pIHXFile->Read(pBuffer, count);
	    if (ulBytesRead != 0)
	    {
		if (ulBytesRead <= count)
		{
		    memcpy(buf, pBuffer->GetBuffer(), count); /* Flawfinder: ignore */
		}
		else
		{
		    ulBytesRead = 0;
		}
	    }

	    HX_RELEASE(pBuffer);
	}
    }

    return ulBytesRead;
}

HX_RESULT CCHX2IHXDataFile::ReadToBuffer(ULONG32 ulCount, IHXBuffer** ppbufOut)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pIHXFile)
    {
	UINT32 ulRead = m_pIHXFile->Read(*ppbufOut, ulCount);

	if ((ulRead == ulCount) || ((ulRead != 0) && (ulRead < ulCount)))
	{
	    retVal = HXR_OK;
	}
    }

    return retVal;
}

ULONG32 CCHX2IHXDataFile::Write(const char* buf, ULONG32 count)
{
    ULONG32 ulBytesWritten = 0;

    if (m_pIHXFile)
    {
	if (m_fpFastWrite)
	{
	    ulBytesWritten = m_fpFastWrite(m_pUserData, buf, count);
	}
	else
	{
	    IHXBuffer* pBuffer = NULL;	    
	    if (HXR_OK == CreateBufferCCF(pBuffer, m_pContext))
	    {
		if ((count != 0) && (pBuffer->SetSize(count) == HXR_OK))
		{
		    memcpy(pBuffer->GetBuffer(), buf, count); /* Flawfinder: ignore */

		    ulBytesWritten = m_pIHXFile->Write(pBuffer);
		}

		pBuffer->Release();
	    }
	}
    }

    return ulBytesWritten;
}

HX_RESULT CCHX2IHXDataFile::Rewind(void)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pIHXFile)
    {
	retVal = m_pIHXFile->Seek(0, SEEK_SET);
    }

    return retVal;
}

INT16 CCHX2IHXDataFile::GetFd(void)
{
    INT16 iFd = -1;

    if (m_pIHXFile)
    {
	iFd = m_pIHXFile->GetFd();
    }

    return iFd;
}

HX_RESULT CCHX2IHXDataFile::Delete(const char *filename)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pIHXFile)
    {
	if (m_pIHXFile->IsOpen())
	{
	    IHXBuffer* pNameBuffer = NULL;

	    if (m_pIHXFile->Name(pNameBuffer) && pNameBuffer)
	    {
		if (strcmp(filename, (const char*) pNameBuffer->GetBuffer()))
		{
		    retVal = m_pIHXFile->Delete();
		}
		else
		{
		    if (m_fpFileDelete)
		    {
			retVal = m_fpFileDelete(m_pUserData, filename);
		    }
		    else
		    {
			m_pIHXFile->Bind(filename);
			retVal = m_pIHXFile->Delete();
		    }
		}
		pNameBuffer->Release();
	    }
	}
	else
	{
	    m_pIHXFile->Bind(filename);
	    retVal = m_pIHXFile->Delete();
	}
    }

    return retVal;
}


/************************************************************************
 *  Private methods
 */
UINT16 CCHX2IHXDataFile::TranslateMode(UINT16 uOpenMode, HXBOOL bTextFlag)
{
    UINT16 uMode = 0;
    
    if (uOpenMode & O_RDWR)
    {
	uMode |= (HX_FILEFLAG_READ | HX_FILEFLAG_WRITE);
    }
    else if (uOpenMode & O_RDONLY)
    {
	uMode |= HX_FILEFLAG_READ;
    }
    else if (uOpenMode & O_WRONLY)
    {
	uMode |= HX_FILEFLAG_WRITE;
    }

    if ((uOpenMode & O_BINARY) && (!bTextFlag))
    {
	uMode |= HX_FILEFLAG_BINARY;
    }

    return uMode;
}

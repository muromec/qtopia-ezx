/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ihxfiletest.cpp,v 1.11 2007/07/06 20:35:23 jfinnecy Exp $
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

#include "hx_ut_debug.h"
#include "datffact.h"
#include "hxbuffer.h"

#include "hlxclib/sys/stat.h"

#include "chxdataf.h"
#include "cihx2chxdataf.h"

#include "ut_param_util.h"
#include "filetestutil.h"
#include "ihxfiletest.h"



HLXIHXFileTest::HLXIHXFileTest(const char* pModeString)
    : m_pIHXDataFile(NULL)
    , m_pCHXDataFile(NULL)
    , m_pCHXDirectory(NULL)
    , m_DataFileMode('i')
    , m_bTestDir(false)
{
    if ((!pModeString) || (strncmp(pModeString, "ihx", 3) == 0))
    {
	m_DataFileMode = 'i';

	// Use "ihx" file object test
	HXDataFileFactory* pDataFileFactory = new HXDataFileFactory;
	if (pDataFileFactory)
	{
	    IUnknown* pPersistentObj = NULL;
	    
	    pDataFileFactory->AddRef();
	    
	    pDataFileFactory->CreateFile(m_pIHXDataFile,
					 NULL,	    // context
					 pPersistentObj,
					 TRUE,	    // mem. mapped I/O
					 0,	    // page size
					 FALSE,	    // file locking
					 FALSE);    // prefer sync. object
	    
	    pDataFileFactory->Release();
	    
	    HX_RELEASE(pPersistentObj);
	}

	m_bTestDir = (strcmp(pModeString, "ihxdir") == 0);
    }
    else if (strncmp(pModeString, "chx", 3) == 0)
    {
	m_DataFileMode = 'c';

	CIHX2CHXDataFile* pIHX2CHXDataFile = new CIHX2CHXDataFile();
	if (pIHX2CHXDataFile)
	{
	    pIHX2CHXDataFile->AddRef();
	    m_pIHXDataFile = pIHX2CHXDataFile;
	    m_pCHXDataFile = pIHX2CHXDataFile->GetCHXDataFile();
	    pIHX2CHXDataFile = NULL;
	}
	HX_RELEASE(pIHX2CHXDataFile);

	m_bTestDir = (strcmp(pModeString, "chxdir") == 0);
    }

    if (m_bTestDir)
    {
	m_pCHXDirectory = new CHXDirectory();
    }
}

HLXIHXFileTest::~HLXIHXFileTest()
{
    HX_RELEASE(m_pIHXDataFile);
    HX_DELETE(m_pCHXDirectory);
}
    
void HLXIHXFileTest::GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds)
{
    cmds.Resize(28);

    cmds[0] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "Bind",
				    &HLXIHXFileTest::HandleBindCmd,
				    2);
    cmds[1] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "Create",
				    &HLXIHXFileTest::HandleCreateCmd,
				    3);
    cmds[2] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "Open",
				    &HLXIHXFileTest::HandleOpenCmd,
				    3);
    cmds[3] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "Close",
				    &HLXIHXFileTest::HandleCloseCmd,
				    2);
    cmds[4] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "Name",
				    &HLXIHXFileTest::HandleNameCmd,
				    2,
				    3);
    cmds[5] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "IsOpen",
				    &HLXIHXFileTest::HandleIsOpenCmd,
				    2);
    cmds[6] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "Write",
				    &HLXIHXFileTest::HandleWriteCmd,
				    3);
    cmds[7] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "Read",
				    &HLXIHXFileTest::HandleReadCmd,
				    3,
				    4);
    cmds[8] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "Flush",
				    &HLXIHXFileTest::HandleFlushCmd,
				    2);
    cmds[9] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "Seek",
				    &HLXIHXFileTest::HandleSeekCmd,
				    4);
    cmds[10] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "Tell",
				    &HLXIHXFileTest::HandleTellCmd,
				    1,
				    2);
    cmds[11] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "Stat",
				    &HLXIHXFileTest::HandleStatCmd,
				    2,
				    3);
    cmds[12] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "Delete",
				    &HLXIHXFileTest::HandleDeleteCmd,
				    2,
				    3);
    cmds[13] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "GetFd",
				    &HLXIHXFileTest::HandleGetFdCmd,
				    2);
    cmds[14] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "GetLastErrorCode",
				    &HLXIHXFileTest::HandleGetLastErrorCodeCmd,
				    2);
    cmds[15] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "GetLastErrorString",
				    &HLXIHXFileTest::HandleGetLastErrorStringCmd,
				    1);
    cmds[16] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "Rewind",
				    &HLXIHXFileTest::HandleRewindCmd,
				    2);
    cmds[17] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "GetTempName",
				    &HLXIHXFileTest::HandleGetTempNameCmd,
				    1);

    cmds[18] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "SetDirPath",
				    &HLXIHXFileTest::HandleSetDirPathCmd,
				    2);
    cmds[19] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "GetDirPath",
				    &HLXIHXFileTest::HandleGetDirPathCmd,
				    2,
				    3);
    cmds[20] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "CreateDir",
				    &HLXIHXFileTest::HandleCreateDirCmd,
				    2);
    cmds[21] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "IsDirValid",
				    &HLXIHXFileTest::HandleIsDirValidCmd,
				    2);
    cmds[22] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "DestroyDir",
				    &HLXIHXFileTest::HandleDestroyDirCmd,
				    2,
				    3);
    cmds[23] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "DeleteDirFile",
				    &HLXIHXFileTest::HandleDeleteDirFileCmd,
				    3);
    cmds[24] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "MakeCurrentDir",
				    &HLXIHXFileTest::HandleMakeCurrentDirCmd,
				    2);
    cmds[25] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "FindFirstInDir",
				    &HLXIHXFileTest::HandleFindFirstInDirCmd,
				    4,
				    5);
    cmds[26] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "FindNextInDir",
				    &HLXIHXFileTest::HandleFindNextInDirCmd,
				    3,
				    4);
    cmds[27] = new HLXUnitTestCmdInfoDisp<HLXIHXFileTest>(this, 
				    "Rename",
				    &HLXIHXFileTest::HandleRenameCmd,
				    4);    
}

HLXCmdBasedTest* HLXIHXFileTest::Clone() const
{
    char pModeString[10]; /* Flawfinder: ignore */

    if (m_DataFileMode == 'c')
    {
	strcpy(pModeString, "chx"); /* Flawfinder: ignore */
    }
    else
    {
	strcpy(pModeString, "ihx"); /* Flawfinder: ignore */
    }

    if (m_bTestDir)
    {
	strcat(pModeString, "dir"); /* Flawfinder: ignore */
    }

    return new HLXIHXFileTest(pModeString);
}


/*****************************************************************************
 *  DataFile Commands
 */ 
bool HLXIHXFileTest::HandleBindCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_pIHXDataFile)
    {
	m_pIHXDataFile->Bind((const char*) info[1]);
	bRetVal = true;
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleCreateCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_pIHXDataFile)
    {
	HX_RESULT retVal = m_pIHXDataFile->Create(
	    CFileTestUtil::ParseModeString(info[1]));
	bRetVal = (retVal == HXR_OK);
	bRetVal = (bRetVal == CFileTestUtil::ParseResultString(info[2]));
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleOpenCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_pIHXDataFile)
    {
	HX_RESULT retVal = m_pIHXDataFile->Open(
	    CFileTestUtil::ParseModeString(info[1]));
	bRetVal = (retVal == HXR_OK);
	bRetVal = (bRetVal == CFileTestUtil::ParseResultString(info[2]));
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleCloseCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_pIHXDataFile)
    {
	HX_RESULT retVal = m_pIHXDataFile->Close();
	bRetVal = (retVal == HXR_OK);
	bRetVal = (bRetVal == CFileTestUtil::ParseResultString(info[1]));
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleFlushCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_pIHXDataFile)
    {
	HX_RESULT retVal = m_pIHXDataFile->Flush();
	bRetVal = (retVal == HXR_OK);
	bRetVal = (bRetVal == CFileTestUtil::ParseResultString(info[1]));
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleNameCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_pIHXDataFile)
    {
	IHXBuffer* pHXBuffer = NULL;
	HXBOOL bResultVal;

	bResultVal = m_pIHXDataFile->Name(pHXBuffer);
	bRetVal = (bResultVal ? true : false);
	bRetVal = (bRetVal == (CFileTestUtil::ParseResultString(info[1]) ? true : false));
	if (bRetVal && bResultVal)
	{
	    bRetVal = false;

	    if (pHXBuffer)
	    {
		if (info.Nelements() > 2)
		{
		    bRetVal = CFileTestUtil::CheckString(info[2], pHXBuffer);
		}
		else
		{
		    bRetVal = true;
		}
	    }
	}

	HX_RELEASE(pHXBuffer);
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleGetTempNameCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_pIHXDataFile)
    {
	if (m_pCHXDataFile)
	{
	    char* pTempName = new char [_MAX_PATH + 1];

	    if (pTempName)
	    {
		pTempName[0] = '\0';

		bRetVal = (m_pCHXDataFile->
			   GetTemporaryFileName("test", pTempName, _MAX_PATH + 1) != 0);

		if (bRetVal)
		{
		    bRetVal = false;

		    if ((strlen(pTempName) > 0) && 
			(strlen(pTempName) <= _MAX_PATH))
		    {
			bRetVal = true;
		    }
		}

		delete [] pTempName;
	    }
	}
	else
	{
	    bRetVal = true;
	}
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleIsOpenCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_pIHXDataFile)
    {
	bRetVal = (m_pIHXDataFile->IsOpen() != FALSE);
	bRetVal = (bRetVal == CFileTestUtil::ParseBoolString(info[1]));
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleWriteCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_pIHXDataFile)
    {
	IHXBuffer* pHXBuffer = new CHXBuffer();
	
	if (pHXBuffer)
	{
	    pHXBuffer->AddRef();

	    if (pHXBuffer->Set((const UINT8*) ((const char*) info[1]), 
			       strlen(info[1])) == HXR_OK)
	    {
		ULONG32 ulRetSize;
		ULONG32 ulInSize;

		ulInSize = pHXBuffer->GetSize();
		ulRetSize = m_pIHXDataFile->Write(pHXBuffer);
		bRetVal = (ulRetSize == ulInSize);
		bRetVal = (bRetVal == CFileTestUtil::ParseResultString(info[2]));
	    }

	    pHXBuffer->Release();
	}
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleReadCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_pIHXDataFile)
    {
	unsigned int uiReadSize = 0;

	if (UTParamUtil::GetUInt(info[1], uiReadSize))
	{
	    IHXBuffer* pHXBuffer = NULL;
	    bool bReadOK;

	    ULONG32 ulRetSize = m_pIHXDataFile->Read(pHXBuffer, uiReadSize);

	    bReadOK = ((ulRetSize != 0) && (ulRetSize != -1));
	    bRetVal = (bReadOK == CFileTestUtil::ParseResultString(info[2]));

	    if (bRetVal && bReadOK)
	    {
		bRetVal = false;

		if (info.Nelements() > 3)
		{
		    bRetVal = CFileTestUtil::CheckBuffer(info[3], 
							 pHXBuffer);
		}
	    }

	    HX_RELEASE(pHXBuffer);
	}
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleSeekCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_pIHXDataFile)
    {
	unsigned int uiSeekOffset = 0;

	if (UTParamUtil::GetUInt(info[1], uiSeekOffset))
	{
	    HX_RESULT retVal = m_pIHXDataFile->Seek(
		uiSeekOffset, 
		CFileTestUtil::ParseSeekModeString(info[2]));

	    bRetVal = (retVal == HXR_OK);
	    bRetVal = (bRetVal == CFileTestUtil::ParseResultString(info[3]));
	}
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleRewindCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_pIHXDataFile)
    {
	HX_RESULT retVal = HXR_FAIL;

	if (m_pCHXDataFile)
	{
	    retVal = m_pCHXDataFile->Rewind();
	}
	else
	{ 
	    retVal = m_pIHXDataFile->Seek(0, SEEK_SET);
	}

	bRetVal = (retVal == HXR_OK);
	bRetVal = (bRetVal == CFileTestUtil::ParseResultString(info[1]));
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleTellCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_pIHXDataFile)
    {
	unsigned int uiFileOffset = 0;

	ULONG32 ulRetFileOffset = m_pIHXDataFile->Tell();

	bRetVal = true;

	if ((info.Nelements() > 1) &&
	    UTParamUtil::GetUInt(info[1], uiFileOffset))
	{
	    bRetVal = (ulRetFileOffset == uiFileOffset);
	}
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleStatCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_pIHXDataFile)
    {
	struct stat statBuffer;

	HX_RESULT retVal = m_pIHXDataFile->Stat(&statBuffer);

	bRetVal = (retVal == HXR_OK);
	bRetVal = (bRetVal == CFileTestUtil::ParseResultString(info[1]));

	if (bRetVal && (retVal == HXR_OK) && (info.Nelements() > 2))
	{
	    unsigned int uiFileSize = 0;
	    bRetVal = false;

	    if (UTParamUtil::GetUInt(info[2], uiFileSize))
	    {
		bRetVal = (statBuffer.st_size == uiFileSize);
	    }
	}
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleDeleteCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_pIHXDataFile)
    {
	if (info.Nelements() == 2)
	{
	    HX_RESULT retVal = m_pIHXDataFile->Delete();
	    bRetVal = (retVal == HXR_OK);
	    bRetVal = (bRetVal == CFileTestUtil::ParseResultString(info[1]));
	}
	else if (info.Nelements() == 3)
	{
	    HX_RESULT retVal = HXR_FAIL;

	    if (m_pCHXDataFile)
	    {
		retVal = m_pCHXDataFile->Delete((const char*) info[1]);
	    }
	    else
	    {
		m_pIHXDataFile->Bind((const char*) info[1]);
		retVal = m_pIHXDataFile->Delete();
	    }

	    bRetVal = (retVal == HXR_OK);
	    bRetVal = (bRetVal == CFileTestUtil::ParseResultString(info[2]));
	}
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleGetFdCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_pIHXDataFile)
    {
	INT16 iFd = m_pIHXDataFile->GetFd();
	bRetVal = true;
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleGetLastErrorCodeCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_pIHXDataFile)
    {
	HX_RESULT retVal = m_pIHXDataFile->GetLastError();
	bRetVal = (retVal == HXR_OK);
	bRetVal = (bRetVal == CFileTestUtil::ParseResultString(info[1]));
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleGetLastErrorStringCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_pIHXDataFile)
    {
	IHXBuffer* pHXBuffer = NULL;

	m_pIHXDataFile->GetLastError(pHXBuffer);

	HX_RELEASE(pHXBuffer);

	bRetVal = true;
    }

    return bRetVal;
}


/*****************************************************************************
 *    Directory Commands
 */ 
bool HLXIHXFileTest::HandleSetDirPathCmd(const UTVector<UTString>& info)
{
    bool bRetVal = true;

    if (m_bTestDir)
    {
	bRetVal = false;

	if (m_pCHXDirectory)
	{
	    m_pCHXDirectory->SetPath((const char*) info[1]);
	    bRetVal = true;
	}
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleGetDirPathCmd(const UTVector<UTString>& info)
{
    bool bRetVal = true;

    if (m_bTestDir)
    {
	bRetVal = false;
	
	if (m_pCHXDirectory)
	{
	    const char* pPath;
	    
	    pPath = m_pCHXDirectory->GetPath();

	    bRetVal = (pPath ? true : false);
	    bRetVal = (bRetVal == (CFileTestUtil::ParseResultString(info[1]) ? true : false));
	    if (bRetVal && pPath)
	    {
		bRetVal = false;
		
		if (info.Nelements() > 2)
		{
		    bRetVal = (strcmp(info[2], pPath) == 0);
		}
		else
		{
		    bRetVal = true;
		}
	    }
	}
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleCreateDirCmd(const UTVector<UTString>& info)
{
    bool bRetVal = true;

    if (m_bTestDir)
    {
	bRetVal = false;

	if (m_pCHXDirectory)
	{
	    bRetVal = (m_pCHXDirectory->Create() != FALSE);
	    bRetVal = (bRetVal == CFileTestUtil::ParseResultString(info[1]));
	}
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleIsDirValidCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_bTestDir)
    {
	bRetVal = false;

	if (m_pCHXDirectory)
	{
	    bRetVal = (m_pCHXDirectory->IsValid() != FALSE);
	    bRetVal = (bRetVal == CFileTestUtil::ParseBoolString(info[1]));
	}
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleDestroyDirCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_bTestDir)
    {
	bRetVal = false;

	if (m_pCHXDirectory)
	{
	    bool bDestroyContents = CFileTestUtil::ParseBoolString(info[1]);

	    bRetVal = (m_pCHXDirectory->Destroy(bDestroyContents ? TRUE : FALSE) != FALSE);
	    if (info.Nelements() > 2)
	    {
		bRetVal = (bRetVal == CFileTestUtil::ParseResultString(info[2]));
	    }
	    else
	    {
		bRetVal = true;
	    }
	}
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleDeleteDirFileCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_bTestDir)
    {
	bRetVal = false;

	if (m_pCHXDirectory)
	{
	    bRetVal = (m_pCHXDirectory->DeleteFile((const char*) info[1]) != FALSE);
	    bRetVal = (bRetVal == CFileTestUtil::ParseResultString(info[2]));
	}
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleMakeCurrentDirCmd(const UTVector<UTString>& info)
{
    bool bRetVal = true;

    if (m_bTestDir)
    {
	bRetVal = false;

	if (m_pCHXDirectory)
	{
	    bRetVal = (m_pCHXDirectory->MakeCurrentDir() != FALSE);
	    bRetVal = (bRetVal == CFileTestUtil::ParseResultString(info[1]));
	}
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleFindFirstInDirCmd(const UTVector<UTString>& info)
{
    bool bRetVal = true;

    if (m_bTestDir)
    {
	bRetVal = false;

	if (m_pCHXDirectory)
	{
	    unsigned int uiFileNameSize = 0;
	    char* pOutFileName = NULL;

	    if (UTParamUtil::GetUInt(info[2], uiFileNameSize))
	    {
		pOutFileName = new char [uiFileNameSize];

		if (pOutFileName)
		{
		    XHXDirectory::FSOBJ retObj;

		    retObj = m_pCHXDirectory->FindFirst((const char*) info[1],	// pattern
							pOutFileName,
							uiFileNameSize);

		    bRetVal = (retObj == CFileTestUtil::ParseFSOBJString(
					    (const char*) info[3], 
					    (retObj != XHXDirectory::FSOBJ_NOTVALID) ? retObj : XHXDirectory::FSOBJ_FILE));

		    if (bRetVal && (retObj != XHXDirectory::FSOBJ_NOTVALID))
		    {
			if (info.Nelements() > 4)
			{
			    bRetVal = (strcmp(pOutFileName, (const char*) info[4]) == 0);
			}
		    }

		    delete pOutFileName;
		}
	    }
	}
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleFindNextInDirCmd(const UTVector<UTString>& info)
{
    bool bRetVal = true;

    if (m_bTestDir)
    {
	bRetVal = false;

	if (m_pCHXDirectory)
	{
	    unsigned int uiFileNameSize = 0;
	    char* pOutFileName = NULL;

	    if (UTParamUtil::GetUInt(info[1], uiFileNameSize))
	    {
		pOutFileName = new char [uiFileNameSize];

		if (pOutFileName)
		{
		    XHXDirectory::FSOBJ retObj;

		    retObj = m_pCHXDirectory->FindNext(pOutFileName,	// path
						       uiFileNameSize);

		    bRetVal = (retObj == CFileTestUtil::ParseFSOBJString((const char*) info[2], retObj));

		    if (bRetVal && (retObj != XHXDirectory::FSOBJ_NOTVALID))
		    {
			if (info.Nelements() > 3)
			{
			    bRetVal = (strcmp(pOutFileName, (const char*) info[3]) == 0);
			}
		    }

		    delete pOutFileName;
		}
	    }
	}
    }

    return bRetVal;
}

bool HLXIHXFileTest::HandleRenameCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;

    if (m_bTestDir)
    {
	bRetVal = false;

	if (m_pCHXDirectory)
	{
	    bRetVal = (m_pCHXDirectory->Rename((const char*) info[1],
					       (const char*) info[2]) == HXR_OK);
	    bRetVal = (bRetVal == CFileTestUtil::ParseResultString(info[3]));
	}
    }

    return bRetVal;
}


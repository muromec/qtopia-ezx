/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cwin32file.cpp,v 1.7 2006/02/07 19:21:14 ping Exp $
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

#include "platform/win/cwin32file.h"
#include "hlxclib/fcntl.h"
#include "hlxclib/windows.h"
#include "hxassert.h"

#define  HX_FILE_NOT_OPEN    -1000

// CHXFile should set the file reference to a value
// indicating the file is not open

CWin32File::CWin32File(IUnknown* pContext) 
	   :CHXDataFile(pContext)
	   ,mFD(INVALID_HANDLE_VALUE)
{
    mLastError = HXR_OK;
}

// ~CHXFile should close the file if it is open
CWin32File::~CWin32File(void)
{ 
    // close the file if it is open
    if ( mFD != INVALID_HANDLE_VALUE )
	Close();
}

// Create a file with the specified mode
// Closes the previous file if it was open
// returns HXR_OK or HXR_INVALID_FILE if an error occurred

HX_RESULT CWin32File::Create(const char *filename, UINT16 mode, HXBOOL textflag)
{
    // close previous file if necessary
    if ( mFD != INVALID_HANDLE_VALUE )
	Close();

    DWORD acc = GENERIC_WRITE, shared = 0, disp = 0;
    
    if (mode & O_CREAT) 
    {
	acc |= GENERIC_WRITE;

	if (mode & O_EXCL) 
	    disp |= CREATE_NEW;
	else
	    disp |= OPEN_ALWAYS;
    }

    if (mode & O_TRUNC) 
    {
	acc |= GENERIC_WRITE;
	disp = TRUNCATE_EXISTING | CREATE_ALWAYS;
    }

    if (mode & O_RDONLY)
	acc = GENERIC_READ;

    if (mode & O_RDWR)
	acc = GENERIC_READ | GENERIC_WRITE;

    if (mode & O_WRONLY)
	acc = GENERIC_WRITE;

    // create file
    mLastError = HXR_OK;
    if ( ( mFD = CreateFile( OS_STRING(filename), 
			     acc, shared, NULL, 
			     disp, NULL, NULL)) == INVALID_HANDLE_VALUE )
    {
	mLastError = HXR_DOC_MISSING;
	return HXR_DOC_MISSING;
    }

    if (mode & O_APPEND) 
	Seek(0, SEEK_END);

    return HXR_OK;
}

// Open a file with the specified mode
// Closes the previous file if it was open
// returns HXR_OK or HXR_INVALID_FILE if an error occurred

HX_RESULT CWin32File::Open(const char *filename, UINT16 mode, HXBOOL textflag)
{
	
    // close previous file if necessary
    if ( mFD != INVALID_HANDLE_VALUE )
	Close();

    // open file
    mLastError = HXR_OK;
    DWORD acc = GENERIC_WRITE, disp = OPEN_ALWAYS;

    if (mode & O_CREAT) 
    {
	acc |= GENERIC_WRITE;
	if (mode & O_EXCL) 
	    disp = CREATE_NEW;
	else
	    disp = OPEN_ALWAYS;
    }
    if (mode & O_TRUNC) 
    {
	acc |= GENERIC_WRITE;
	disp = TRUNCATE_EXISTING | CREATE_ALWAYS;
    }

    if (mode & O_RDONLY)
	acc = GENERIC_READ;

    if (mode & O_RDWR)
	acc = GENERIC_READ | GENERIC_WRITE;

    if (mode & O_WRONLY)
	acc = GENERIC_WRITE;

    if(mode & O_TRUNC)
    {
	acc |= GENERIC_WRITE;

	mFD = CreateFile(OS_STRING(filename), acc, 0, 
			 NULL, 0, NULL, NULL) ;

	if ( mFD == INVALID_HANDLE_VALUE)
	{
	    mLastError = HXR_DOC_MISSING;
	    return HXR_DOC_MISSING;
	}
	Close();
    }

    // Actually open the file		
    mFD = CreateFile(OS_STRING(filename), acc, 0, NULL, disp, NULL, NULL) ;

    if ( mFD == INVALID_HANDLE_VALUE )
    {
	mLastError = HXR_DOC_MISSING;
	return HXR_DOC_MISSING;
    }

    if (mode & O_APPEND)
	Seek(0, SEEK_END);

    return HXR_OK;
}

// Open a file for sharing with the specified mode
// Closes the previous file if it was open
// returns HXR_OK or HXR_INVALID_FILE if an error occurred

HX_RESULT CWin32File::OpenShared(const char *filename, UINT16 mode, UINT16 sharedmode, HXBOOL textflag)
{
	
    // close previous file if necessary
    if ( mFD != INVALID_HANDLE_VALUE )
	Close();

    // open file
    mLastError = HXR_OK;

    DWORD acc = GENERIC_WRITE, disp = 0, shared = FILE_SHARE_READ | FILE_SHARE_WRITE;

    if (mode & O_CREAT) 
    {
	acc |= GENERIC_WRITE;
	if (mode & O_EXCL) 
	    disp |= CREATE_NEW;
	else
	    disp |= OPEN_ALWAYS;
    }
    if (mode & O_TRUNC) 
    {
	acc |= GENERIC_WRITE;
	disp = TRUNCATE_EXISTING | CREATE_ALWAYS;
    }

    if (mode & O_RDONLY)
	acc = GENERIC_READ;

    if (mode & O_RDWR)
	acc = GENERIC_READ | GENERIC_WRITE;

    if (mode & O_WRONLY)
	acc = GENERIC_WRITE;

    if (sharedmode & SH_DENYRW)
	shared = 0;

    if (sharedmode & SH_DENYWR)
	shared = FILE_SHARE_READ;

    if (sharedmode & SH_DENYRD)
	shared = FILE_SHARE_WRITE;

    if(mode & O_TRUNC)
    {
	mFD = CreateFile(OS_STRING(filename), acc, 0, 
			 NULL, 0, NULL, NULL) ;

	// Check acces rights to this file first
	if ( mFD == INVALID_HANDLE_VALUE)
	{
	    mLastError = HXR_DOC_MISSING;
	    return HXR_DOC_MISSING;
	}
	Close();
    }

    // Actually open the file - use sharing mode
    mFD = CreateFile(OS_STRING(filename), acc, shared, 
		     NULL, 0, NULL, NULL) ;

    if ( mFD == INVALID_HANDLE_VALUE )
    {
	mLastError = HXR_DOC_MISSING;
	return HXR_DOC_MISSING;
    }

    return HXR_OK;
}


// Close the previous file if it was open
// returns HXR_OK or HXR_INVALID_FILE if an error occurred

HX_RESULT CWin32File::Close(void)
{
    // close previous file if necessary
    if ( mFD != INVALID_HANDLE_VALUE)
    {
	mLastError = HXR_OK;   
	if ( CloseHandle( mFD ) == 0 )
	{
	    mLastError = HXR_INVALID_FILE;
	    return HXR_INVALID_FILE;
	}
	return HXR_OK;
    }
    return HXR_INVALID_FILE;
}


// Simply uses stat to get the size of the file in bytes.  If the file
// is closed, it will still work.
ULONG32 CWin32File::GetSize(void)
{
    ULONG32 ret = 0;

    if (mFD != INVALID_HANDLE_VALUE) 
    {
	ret = GetFileSize(mFD, NULL);
    }

    return ret;
}


// Seek moves the current file position to the offset from the fromWhere
// specifier returns HXR_OK or HXR_INVALID_FILE if an error occurred

HX_RESULT CWin32File::Seek(ULONG32 offset, UINT16 fromWhere)
{
    mLastError = HXR_INVALID_FILE;

    if (( mFD != INVALID_HANDLE_VALUE ) &&
	( SetFilePointer( mFD, offset, NULL, fromWhere ) != 0xffffffff ))
	mLastError = HXR_OK;

    return mLastError;
}

// Rewind sets the file position to the start of file
// returns HXR_OK or HXR_INVALID_FILE if an error occurred
HX_RESULT CWin32File::Rewind(void)
{
    mLastError = HXR_INVALID_FILE;

    if (( mFD != INVALID_HANDLE_VALUE ) &&
	( SetFilePointer( mFD, 0, NULL, FILE_BEGIN ) != 0xffffffff ))
	mLastError = HXR_OK;

    return mLastError;
}


// Tell returns the current file position
// returns HXR_OK or -1 if an error occurred
ULONG32 CWin32File::Tell(void)
{   
    long offset = -1;

    mLastError = HXR_INVALID_FILE;
    if (( mFD != INVALID_HANDLE_VALUE ) &&
	((offset = SetFilePointer( mFD, 0, NULL, 
				   FILE_CURRENT )) != 0xffffffff ))
	    mLastError = HXR_OK;

    return (ULONG32)offset;
}

/*      Read reads up to count bytes of data into buf.
        returns the number of bytes read, EOF, or -1 if the read failed */

ULONG32 CWin32File::Read (char *buf, ULONG32 count)
{
    DWORD ncnt = (DWORD)-1;           // number of bytes read

    if ( mFD != INVALID_HANDLE_VALUE )
    { 
	mLastError = HXR_OK;
	ULONG32 tmpCheck = Tell();

	HX_ASSERT(count < UINT_MAX);

	if ( ( ReadFile( mFD, (LPVOID)buf, (DWORD) count, &ncnt, NULL ) ) == 0 )
	{
	    mLastError = HXR_INVALID_FILE;
		 
	}
    }

    return (ULONG32)ncnt;
}


/*      Write writes up to count bytes of data from buf.
        returns the number of bytes written, or -1 if the write failed */

ULONG32 CWin32File::Write(const char *buf, ULONG32 count)
{
    DWORD ncnt = (DWORD)-1;           // number of bytes written

    if ( mFD != INVALID_HANDLE_VALUE )
    { 
	mLastError = HXR_OK;

	HX_ASSERT(count < UINT_MAX);

	if ( ( WriteFile( mFD, (LPVOID)buf, (DWORD) count, &ncnt, NULL ) ) == -1 )
	{
	    mLastError = HXR_INVALID_FILE;
	}
    }

    return (ULONG32)ncnt;
}

HXBOOL CWin32File::GetTemporaryFileName(const char *tag, char* name, UINT32 ulBufLen)
{
    HXBOOL bOk = TRUE;

    char szTempPathName[_MAX_PATH] = "."; /* Flawfinder: ignore */

#if !defined(WIN32_PLATFORM_PSPC)
    if (!GetTempPath(_MAX_PATH,szTempPathName))
    {
	bOk = FALSE;
    }
#endif /* !defined(WIN32_PLATFORM_PSPC) */

    if (bOk && !GetTempFileName(OS_STRING(szTempPathName),
				OS_STRING(tag),0, OS_STRING2(name,_MAX_PATH)))
    {
	bOk = FALSE;
    }

    return bOk;
}

// Delete a file 

HX_RESULT CWin32File::Delete(const char *filename)
{
    // close previous file if necessary
    if ( mFD != INVALID_HANDLE_VALUE )
    {
	Close();
    }

    // delete file
    mLastError = HXR_OK;
    if(!DeleteFile(OS_STRING(filename)))
    {
	if(GetLastError() == ERROR_ACCESS_DENIED)
	    mLastError = HXR_ACCESSDENIED;
	else
	    mLastError = HXR_DOC_MISSING;
	return mLastError;
    }

    return HXR_OK;
}

HX_RESULT CWin32File::ChangeSize	(ULONG32 newSize)
{
    HX_RESULT res = HXR_FAIL;

    if (mFD != INVALID_HANDLE_VALUE)
    {
        ULONG32 pos = Tell();

	if (SetFilePointer(mFD, newSize, 0, SEEK_SET) != 0xFFFFFFFF)
	{
	    if (SetEndOfFile(mFD) != 0)
		res = HXR_OK;

            // reposition pointer at orignal position
            Seek(pos, SEEK_SET);       
	}
    }

    return res;
}

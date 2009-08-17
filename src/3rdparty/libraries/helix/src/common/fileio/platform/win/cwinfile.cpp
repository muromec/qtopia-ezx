/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cwinfile.cpp,v 1.8 2006/02/07 19:21:14 ping Exp $
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

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <share.h>
#include <limits.h>

#include "hxassert.h"


#include "CHXDataF.h"
#include "platform/win/CWinFile.h"

#include "hlxclib/windows.h"		// For GetTempPath(), etc.
#include <stdlib.h>			// For _MAX_PATH

#include "hxcom.h"
#include "hxbuffer.h"


#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define  HX_FILE_NOT_OPEN    -1000

// CHXFile should set the file reference to a value
// indicating the file is not open

CWinFile::CWinFile (IUnknown* pContext)
	 :CHXDataFile(pContext)
{
	// set FD to indicate file is not open
	mFD = HX_FILE_NOT_OPEN;
	mLastError = HXR_OK;
}

// ~CHXFile should close the file if it is open
CWinFile::~CWinFile(void)
{ 
	// close the file if it is open
	if ( mFD >= 0 )
	{
		close( mFD );
	}
}

// Create a file with the specified mode
// Closes the previous file if it was open
// returns HXR_OK or HXR_INVALID_FILE if an error occurred

HX_RESULT CWinFile::Create(const char *filename, UINT16 mode, HXBOOL textflag)
{
	// close previous file if necessary
	if ( mFD >= 0 )
	{
		close( mFD );
	}

	// create file
	mLastError = HXR_OK;
	if ( ( mFD = creat( filename, mode ) ) < 0 )
	{
	   mLastError = HXR_DOC_MISSING;
	   return HXR_DOC_MISSING;
	}

	return HXR_OK;
}

// Open a file with the specified mode
// Closes the previous file if it was open
// returns HXR_OK or HXR_INVALID_FILE if an error occurred

HX_RESULT CWinFile::Open(const char *filename, UINT16 mode, HXBOOL textflag)
{
	
	// close previous file if necessary
	if ( mFD >= 0 )
	{
		close( mFD );
	}

	// open file
	mLastError = HXR_OK;

	int shareFlag = SH_DENYNO;

	if(mode & O_TRUNC)
	{
		// Check acces rights to this file first
	    if (!textflag)
			mFD = sopen( filename, mode | O_BINARY, SH_DENYRW, S_IREAD | S_IWRITE );
		else
			mFD = sopen( filename, mode | O_TEXT, SH_DENYRW, S_IREAD | S_IWRITE );

	    if ( mFD < 0)
	    {
		    mLastError = HXR_DOC_MISSING;
		    return HXR_DOC_MISSING;
	    }
		close( mFD );
	}

	  // Actually open the file
    if (!textflag)
		mFD = open( filename, mode | O_BINARY, S_IREAD | S_IWRITE );
	else
		mFD = open( filename, mode | O_TEXT, S_IREAD | S_IWRITE );

    if ( mFD < 0 )
	{
	    mLastError = HXR_DOC_MISSING;
	    return HXR_DOC_MISSING;
	}

	return HXR_OK;
}

// Open a file for sharing with the specified mode
// Closes the previous file if it was open
// returns HXR_OK or HXR_INVALID_FILE if an error occurred

HX_RESULT CWinFile::OpenShared(const char *filename, UINT16 mode, UINT16 sharedmode, HXBOOL textflag)
{
	
	// close previous file if necessary
	if ( mFD >= 0 )
	{
		close( mFD );
	}

	// open file
	mLastError = HXR_OK;

	int shareFlag = SH_DENYNO;

	if(mode & O_TRUNC)
	{
		// Check acces rights to this file first
	    if (!textflag)
			mFD = sopen( filename, mode | O_BINARY, SH_DENYRW, S_IREAD | S_IWRITE );
		else
			mFD = sopen( filename, mode | O_TEXT, SH_DENYRW, S_IREAD | S_IWRITE );

	    if ( mFD < 0)
	    {
		    mLastError = HXR_DOC_MISSING;
		    return HXR_DOC_MISSING;
	    }
		close( mFD );
	}

	  // Actually open the file - use sharing mode
    if (!textflag)
		mFD = sopen( filename, mode | O_BINARY, sharedmode, S_IREAD | S_IWRITE );
	else
		mFD = sopen( filename, mode | O_TEXT, sharedmode, S_IREAD | S_IWRITE );

    if ( mFD < 0 )
	{
	    if(errno == EACCES)
		    mLastError = HXR_ACCESSDENIED;
	    else
		    mLastError = HXR_DOC_MISSING;
	    return mLastError;
	}

	return HXR_OK;
}


// Close the previous file if it was open
// returns HXR_OK or HXR_INVALID_FILE if an error occurred

HX_RESULT CWinFile::Close(void)
{
	// close previous file if necessary
	if ( mFD >= 0 )
	{
		mLastError = HXR_OK;   
		if ( close( mFD ) < 0 )
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
ULONG32 CWinFile::GetSize(void)
{
    struct _stat filestats;
    if (mFD >= 0) 
    {
	_fstat(mFD, &filestats);
	return filestats.st_size;
    }
    return 0;
}


HX_RESULT CWinFile::ChangeSize	(ULONG32 newSize)
{
    if (mFD >= 0)
    {
        // Ff the file pointer is passed the new size, we ought to position it in a valid spot.
        ULONG32 pos = Tell();
        if (pos > newSize)
            Seek(newSize, SEEK_SET);
                
        if (!_chsize(mFD, newSize))
            return HXR_OK;
        else // reposition pointer at orignal position
            Seek(pos, SEEK_SET);              
    }    
    return HXR_FAIL;
}


// Seek moves the current file position to the offset from the fromWhere
// specifier returns HXR_OK or HXR_INVALID_FILE if an error occurred

HX_RESULT CWinFile::Seek(ULONG32 offset, UINT16 fromWhere)
{
	if ( mFD >= 0 )
	{
		mLastError = HXR_OK;       
		if ( lseek( mFD, offset, fromWhere ) < 0 )
		{
			mLastError = HXR_INVALID_FILE;
			return HXR_INVALID_FILE;
		}
		return HXR_OK;
	}
	return HXR_INVALID_FILE;
}

// Rewind sets the file position to the start of file
// returns HXR_OK or HXR_INVALID_FILE if an error occurred
HX_RESULT CWinFile::Rewind(void)
{
	if ( mFD >= 0 )
	{
		mLastError = HXR_OK;       
		if ( lseek( mFD, 0, SEEK_SET ) < 0 )
		{
			mLastError = HXR_INVALID_FILE;
			return HXR_INVALID_FILE;
		}
		return HXR_OK;
	}
	return HXR_INVALID_FILE;
}


// Tell returns the current file position
// returns HXR_OK or -1 if an error occurred
ULONG32 CWinFile::Tell(void)
{   
	long offset = -1;

	if ( mFD >= 0 )
	{
		mLastError = HXR_OK;       
		if ((offset = tell( mFD )) < 0 )
		{
			mLastError = HXR_INVALID_FILE;
		}
	}
	return (ULONG32)offset;
}

/*      Read reads up to count bytes of data into buf.
        returns the number of bytes read, EOF, or -1 if the read failed */

ULONG32 CWinFile::Read (char *buf, ULONG32 count)
{
	UINT ncnt = (UINT)-1;           // number of bytes read

	if ( mFD >= 0 )
	{ 
		mLastError = HXR_OK;
		ULONG32 tmpCheck = Tell();

		HX_ASSERT(count < UINT_MAX);

		if ( ( ncnt = read( mFD, buf, (UINT) count ) ) == -1 )
		{
			mLastError = HXR_INVALID_FILE;
		 
		}
	}

	return (ULONG32)ncnt;
}


/*      Write writes up to count bytes of data from buf.
        returns the number of bytes written, or -1 if the write failed */

ULONG32 CWinFile::Write(const char *buf, ULONG32 count)
{
	UINT ncnt = (UINT)-1;           // number of bytes written

	if ( mFD >= 0 )
	{ 
		mLastError = HXR_OK;

		HX_ASSERT(count < UINT_MAX);

		if ( ( ncnt = write( mFD, buf, (UINT)count ) ) == -1 )
		{
			mLastError = HXR_INVALID_FILE;
		}
	}

	return (ULONG32)ncnt;
}

HXBOOL CWinFile::GetTemporaryFileName(const char *tag, char* name, UINT32 ulBufLen)
{
	HXBOOL bOk = FALSE;

#ifdef _WIN32
	char szTempPathName[_MAX_PATH]; /* Flawfinder: ignore */

	if (!GetTempPath(_MAX_PATH,szTempPathName))
	{
		goto exit;
	}

	if (!GetTempFileName(szTempPathName,tag,0,name))
	{
		goto exit;
	}

#else
	if (!GetTempFileName(0,tag,0,name))
	{
		goto exit;
	}
#endif _WIN32	

	// If we made it this far then we're cool!
	bOk = TRUE;

exit:

	return bOk;
}

// Delete a file 

HX_RESULT CWinFile::Delete(const char *filename)
{
	// close previous file if necessary
	if ( mFD >= 0 )
	{
		close( mFD );
	}

	// delete file
	mLastError = HXR_OK;
	if(unlink(filename))
	{
		if(errno == EACCES)
			mLastError = HXR_ACCESSDENIED;
		else
			mLastError = HXR_DOC_MISSING;
		return mLastError;
	}

	return HXR_OK;
}





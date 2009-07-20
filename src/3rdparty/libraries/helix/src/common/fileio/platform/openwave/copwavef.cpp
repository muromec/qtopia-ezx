/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: copwavef.cpp,v 1.8 2007/07/06 20:35:14 jfinnecy Exp $
 * 
 * * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
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

#include "hlxclib/stdio.h"
#include "hlxclib/sys/stat.h"
#include "hlxclib/sys/types.h"
#include "hlxclib/fcntl.h"
//#include "hlxclib/errno.h"

#include "chxdataf.h"
#include "copwavef.h"
#include "hxheap.h"
#include "hxresult.h"

#define  HX_FILE_NOT_OPEN    -1000

/// TB correctly done 
//int errno = 1;

CHXDataFile*
CHXDataFile::Construct (IUnknown* pContext, UINT32 ulFlags, IUnknown** ppCommonObj)
{
    return new COpenwaveFile(pContext);
}

// CHXFile should set the file reference to a value
// indicating the file is not open

COpenwaveFile::COpenwaveFile (IUnknown* pContext)
	      :CHXDataFile(pContext)
{
    // set FD to indicate file is not open
    mFD = HX_FILE_NOT_OPEN;
    mLastError = HXR_OK;
}

// ~CHXFile should close the file if it is open
COpenwaveFile::~COpenwaveFile(void)
{ 
    // close the file if it is open
    if ( mFD > 0 )
    {
        //close( mFD );
        OpFsClose( mFD );
    }
}

// Following comments and code of GetSize were directly taken from the 
// windows implementation ../win/cwinfile.cpp

// Simply uses stat to get the size of the file in bytes.  If the file
// is closed, it will still work.
ULONG32 COpenwaveFile::GetSize(void)
{
    OpFsStatStruct filestats;
    if (mFD >= 0)
    {
        if (OpFsStat(mFName, &filestats) == kOpFsErrOk)
        {
            return filestats.size;
        }
    }
    return 0;
}


// Create a file with the specified mode
//   (XXXSAB: does Openwave even have a read-only bit/file permissions for
//   'mode'?)
// Closes the previous file if it was open
// returns HXR_OK or HXR_INVALID_FILE if an error occurred

HX_RESULT COpenwaveFile::Create(const char *filename, UINT16 mode, HXBOOL textflag)
{
    // close previous file if necessary
    if ( mFD > 0 )
    {
        //close( mFD );
        OpFsClose( mFD );
    }
    
    // create file
    mLastError = HXR_OK;
    
    // if ( ( mFD = creat( filename, mode ) ) < 0 )
    mFD = OpFsOpen(filename, mode, 0);
    if (mFD == kOpFsErrAny)
    {
        mFD = HX_FILE_NOT_OPEN;
        mLastError = mFD;
        return HXR_DOC_MISSING;
    }
    mFName = filename;
    return HXR_OK;
}

// Open a file with the specified mode
// Closes the previous file if it was open
// returns HXR_OK or HXR_INVALID_FILE if an error occurred

HX_RESULT COpenwaveFile::Open(const char *filename, UINT16 mode, HXBOOL textflag)
{
    // close previous file if necessary
    if ( mFD > 0 )
    {
        //   close( mFD );
        OpFsClose( mFD );
    }
    
    // open file
    mLastError = HXR_OK;
    mFD = OpFsOpen(filename,
        kOpFsFlagWrOnly | kOpFsFlagCreate,
        0);
    {
        mFD = HX_FILE_NOT_OPEN;
        mLastError = mFD;
        return HXR_DOC_MISSING;
    }
    mFName = filename;
    
    return HXR_OK;
}

// Close the previous file if it was open
// returns HXR_OK or HXR_INVALID_FILE if an error occurred

HX_RESULT COpenwaveFile::Close(void)
{
    // close previous file if necessary
    if ( mFD > 0 )
    {
        mLastError = HXR_OK;   
        //if ( close( mFD ) < 0 )
        if (OpFsClose( mFD ) != kOpFsErrOk )
        {
            mLastError = HXR_INVALID_FILE;
            return HXR_INVALID_FILE;
        }
        return HXR_OK;
    }
    return HXR_INVALID_FILE;
}

HX_RESULT COpenwaveFile::Delete(const char* pFilename)
{
    if(mFD > 0)
    {
        //close(mFD);
        OpFsClose( mFD );
    }
    mLastError = HXR_OK;
    if(OpFsRemove(pFilename) == kOpFsErrAny)
    {
        mLastError = HXR_ACCESSDENIED;
    }
    return mLastError;
}

// Seek moves the current file position to the offset from the fromWhere
// specifier returns HXR_OK or HXR_INVALID_FILE if an error occurred

HX_RESULT COpenwaveFile::Seek(ULONG32 offset, UINT16 fromWhere)
{
    if ( mFD > 0 )
    {
        mLastError = HXR_OK;       
        //if ( lseek( mFD, offset, fromWhere ) < 0 )'
        if (OpFsSeek( mFD, (OpFsSize)offset, fromWhere) == kOpFsErrAny)
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
HX_RESULT COpenwaveFile::Rewind(void)
{
    if ( mFD > 0 )
    {
        mLastError = HXR_OK;       
        //if ( lseek( mFD, 0, SEEK_SET ) < 0 )
        if (OpFsSeek( mFD, (OpFsSize)0, SEEK_SET) == kOpFsErrAny)
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
ULONG32 COpenwaveFile::Tell(void)
{   
    long offset = -1;
    if ( mFD > 0 )
    {
        mLastError = HXR_OK;       
        // the tell() function will consistently generate a warning 
        // and I haven't figured out how to fix that yet
        /****
        if ((offset = tell( mFD )) < 0 )
        {
        mLastError = errno;
        }
        ****/
        // so we do this instead....
        //if ((offset = lseek( mFD, 0, SEEK_CUR )) < 0 )
        if (OpFsSeek( mFD, (OpFsSize)0, SEEK_CUR) == kOpFsErrAny)
        {
            mLastError = HXR_INVALID_FILE;
        }
    }
    return (ULONG32)offset;
}

/*      Read reads up to count bytes of data into buf.
returns the number of bytes read, EOF, or -1 if the read failed */

ULONG32 COpenwaveFile::Read (char *buf, ULONG32 count)
{
    //int ncnt = -1;           // number of bytes read
    OpFsSize ncnt = kOpFsErrAny; 
    
    if ( mFD > 0 )
    { 
        mLastError = HXR_OK;
        ULONG32 tmpCheck = Tell();
        
        //if ( ( ncnt = read( mFD, buf, count ) ) < 0 )
        if (( ncnt = OpFsRead( mFD, buf, count)) == kOpFsErrAny)
        {
            mLastError = HXR_INVALID_FILE;
        }
    }
    return (ULONG32)ncnt;
}

/*      Write writes up to count bytes of data from buf.
returns the number of bytes written, or -1 if the write failed */

ULONG32 COpenwaveFile::Write(const char *buf, ULONG32 count)
{
    //  int ncnt = -1;           // number of bytes written
    OpFsSize ncnt = kOpFsErrAny;
    
    if ( mFD > 0 )
    { 
        mLastError = HXR_OK;
        
        //if ( ( ncnt = write( mFD, buf, count ) ) < 0 )
        if (( ncnt = OpFsWrite( mFD, buf, count)) == kOpFsErrAny)
        {
            mLastError = HXR_INVALID_FILE;
        }
    }
    
    return (ULONG32)ncnt;
}

HXBOOL COpenwaveFile::GetTemporaryFileName(const char *tag, char* name, UINT32 ulBufLen)
{
    // TBF
#ifdef Commented_out_code_20030422_132559
    if (tmpnam(name))
        return TRUE;
    else
#endif /* Commented_out_code_20030422_132559 */
        return FALSE;
}


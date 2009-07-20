/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxstat.cpp,v 1.8 2006/02/07 19:21:10 ping Exp $
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

#include "hlxclib/stdio.h"
#include "hlxclib/stdarg.h"
#include "hlxclib/string.h"             // memchr
#include "hxresult.h"
#include "hxtypes.h"
 
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE             
static const char HX_THIS_FILE[] = __FILE__;
#endif

#include "hlxclib/fcntl.h"              // O_CREAT | O_APPEND

HXStatLog::HXStatLog() :
    mStatFile (0)
                       , mStatBuffer (0)
                       , mStatCurrentPosition (0)
                       , mStatBufferLength (0)
                       , mStatBytesLeft (0) 
{
}

HXStatLog::~HXStatLog()
{
    if (mStatFile)
    {
        mStatFile->Close();
        delete mStatFile;
        mStatFile = NULL;
    }

    if (mStatBuffer)
    {
        delete [] mStatBuffer;
        mStatBuffer = NULL;
    }
}

HX_RESULT HXStatLog::Open_Write (const char * filepath)
{
    HX_RESULT theErr = HXR_OK;

    if (mStatFile)
        return HXR_ALREADY_OPEN;                //HX_ALREADY_OPEN;

    mStatFile = CHXDataFile::Construct();
    if (!mStatFile)
        theErr = HXR_OUTOFMEMORY;

    if (!theErr)
    {
        // Open file in Text format...
        // (setting last variable TRUE opens file in text format)
        theErr = mStatFile->Open(filepath, O_RDWR | O_CREAT | O_APPEND, TRUE);
    }

    if (theErr && mStatFile)
    {
        mStatFile->Close();
        delete mStatFile;
        mStatFile = NULL;
    }

    return theErr;

}

HX_RESULT HXStatLog::Open_WriteNoAppend (const char * filepath)
{
    HX_RESULT theErr = HXR_OK;

    if (mStatFile)
        return HXR_ALREADY_OPEN;                //HX_ALREADY_OPEN;

    mStatFile = CHXDataFile::Construct();
    if (!mStatFile)
        theErr = HXR_OUTOFMEMORY;

    if (!theErr)
    {
        // Open file in Text format...
        // (setting last variable TRUE opens file in text format)
        theErr = mStatFile->Open(filepath, O_RDWR | O_CREAT | O_TRUNC, TRUE);
    }

    if (theErr && mStatFile)
    {
        mStatFile->Close();
        delete mStatFile;
        mStatFile = NULL;
    }

    return theErr;

}
HX_RESULT HXStatLog::Open_Read (const char * filepath)
{
    HX_RESULT theErr = HXR_OK;

    if (mStatFile)
        return HXR_ALREADY_OPEN;                //HX_ALREADY_OPEN;

    mStatFile = CHXDataFile::Construct();
    if (!mStatFile)
        theErr = HXR_OUTOFMEMORY;

    if (!theErr)
    {
        // Open file in Text format...
        // (setting last variable TRUE opens file in text format)
        theErr = mStatFile->Open(filepath, O_RDONLY, TRUE);
    }

    if (theErr && mStatFile)
    {
        mStatFile->Close();
        delete mStatFile;
        mStatFile = NULL;
    }

    return theErr;
}

HX_RESULT HXStatLog::Close      (void)
{
    HX_RESULT theErr = HXR_OK;

    if (!mStatFile)
        return HXR_INVALID_FILE;                //HX_ALREADY_OPEN;

    theErr = mStatFile->Close();

    delete mStatFile;
    mStatFile = NULL;

    return theErr;
}

LONG32 HXStatLog::Read  (char *buf,     ULONG32 buf_length)
{
    if (!mStatFile || !buf)
        return HXR_INVALID_PATH;                

    LONG32 bytes_read = 0;
        
    bytes_read = (LONG32) mStatFile->Read(buf, buf_length);

    return bytes_read;
}

LONG32 HXStatLog::Write (char *buf, ULONG32 buf_length)
{
    if (!mStatFile || !buf)
        return HXR_INVALID_PATH;                

    LONG32 bytes_written = 0;
        
    bytes_written = (LONG32) mStatFile->Write(buf, (ULONG32) buf_length);

    return bytes_written;
}

HX_RESULT HXStatLog::StatPrintf(const char* fmt, ...)
{
    if (!mStatFile)
        return HXR_INVALID_PATH;                

    HX_RESULT theErr = HXR_OK;
    char buf[8096]; /* Flawfinder: ignore */
    LONG32 bytes_written = 0;
    LONG32 bytes_towrite = 0;

    va_list args;
    va_start(args, fmt);
    bytes_towrite = vsnprintf(buf, sizeof(buf), fmt, args);       // scanf
    va_end(args);

    if (bytes_towrite < 0)
        theErr = HXR_INVALID_PATH;              

    if (!theErr)
    {
        bytes_written = mStatFile->Write(buf, (ULONG32) bytes_towrite);
        if (bytes_written != bytes_towrite)
            theErr = HXR_INVALID_PATH;              
    }

    return theErr;
}
        
#define MAX_STAT_LENGTH 8096

HX_RESULT HXStatLog::ReadLine(char *buf, ULONG32 buf_length)
{
    char *newline_loc = NULL;

    if (!mStatFile || !buf)
        return HXR_INVALID_FILE;

    if (!mStatBuffer)
    {
        mStatBuffer = new char [MAX_STAT_LENGTH];

        if (!mStatBuffer)
            return HXR_INVALID_FILE;

        mStatBufferLength = (INT16) mStatFile->Read(mStatBuffer, MAX_STAT_LENGTH);
        mStatBytesLeft =        mStatBufferLength;
        mStatCurrentPosition = mStatBuffer;
    }

#ifdef __MWERKS__
    char eofChar = '\r';
#else
    char eofChar = '\n';
#endif

    // check if something is left in the buffer read earlier...
    // if yes try to read line from that buffer...
    if (mStatBytesLeft)
    {
        newline_loc = (char *) memchr(mStatCurrentPosition, eofChar, mStatBufferLength);
    }

    // if newlibe is not found, read more data from file...
    if (!newline_loc)
    {
        char * tmp_config_buf = new char [MAX_STAT_LENGTH];

        if (!tmp_config_buf)
            return HXR_OK;

        memcpy(tmp_config_buf, mStatCurrentPosition, (mStatBytesLeft <= MAX_STAT_LENGTH ? mStatBytesLeft : MAX_STAT_LENGTH));
        mStatBufferLength = (INT16) mStatFile->Read(tmp_config_buf + mStatBytesLeft, MAX_STAT_LENGTH - mStatBytesLeft);
        mStatBufferLength       += mStatBytesLeft;

        // update the pointers...
        delete [] mStatBuffer;

        mStatBuffer = tmp_config_buf;
        mStatBytesLeft =        mStatBufferLength;
        mStatCurrentPosition = mStatBuffer;
    }

    // if newlibe was not found earlier, try it again now that we have
    // read more data... if it is still not found.. we bail out..
    // limitation: Each line can be a max of MAX_STAT_LENGTH...
    if (!newline_loc && mStatBytesLeft)
    {
        newline_loc = (char *) memchr(mStatCurrentPosition, eofChar, mStatBufferLength);
    }

        
    // extra + 1 is for adding \0 at the end of newline character..
    if (newline_loc && (newline_loc + 1 + 1 - mStatCurrentPosition ) <= (LONG32) buf_length)
    {
        memcpy(buf, mStatCurrentPosition, newline_loc + 1 - mStatCurrentPosition); /* Flawfinder: ignore */
                
        // make it a NULL terminated string to perform string operations...
        buf[newline_loc + 1 - mStatCurrentPosition] = '\0';

        mStatBytesLeft          -= (newline_loc + 1 - mStatCurrentPosition);
        mStatCurrentPosition    = newline_loc + 1;

        return HXR_OK;
    }
    else
        return HXR_INVALID_FILE;
}
 

HX_RESULT HXStatLog::Seek (ULONG32 offset, UINT16 fromWhere)
{
    if (!mStatFile)
        return HXR_INVALID_PATH;                

    return mStatFile->Seek(offset, fromWhere);      
}

ULONG32  HXStatLog::Tell (void) 
{
    if (!mStatFile)
        return (ULONG32)HXR_INVALID_PATH;               
    
    return mStatFile->Tell();
}


 

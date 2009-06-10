/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxdataf_stdio.cpp,v 1.11 2007/07/06 20:35:11 jfinnecy Exp $
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

#include "chxdataf_stdio.h"

#include "hxassert.h"	// hxassert.h is NOT a system header
#include "hxheap.h"
#include "hxbuffer.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

CHXDataFile::CHXDataFile(IUnknown* pContext)
: m_pContext(pContext)
,mLastError(HXR_OK)
,m_FP(NULL)
{ 
    HX_ADDREF(m_pContext);
}

// ~CHXFile should close the file if it is open
CHXDataFile::~CHXDataFile(void)
{
    Close();
    HX_RELEASE(m_pContext);
}

HXBOOL CHXDataFile::GetTemporaryFileName(const char *tag, char* name, UINT32 ulBufLen)
{
	return FALSE;
}

CHXDataFile*	
CHXDataFile::Construct(IUnknown* pContext, UINT32 ulFlags, IUnknown** ppCommonObj)
{
    CHXDataFile* pCHXDataFile = new CHXDataFile(pContext);
    return pCHXDataFile;
}

//
// translate mode flag defined in (hlxclib\fcntl.h) to fopen flag
//
// buf must be at least CCH_MAX_FOPENFLAG chars
//
static const UINT32 CCH_MAX_FOPENFLAG = 4;
static const char* TranslateOpenMode(UINT16 mode, bool bText, char* buf)
{
    buf[0] = '\0';

    HX_ASSERT(!(mode & O_EXCL));

    // Most mode flags are OR'able values. However, O_RDONLY is defined 0
    // so we can't check val & O_RDONLY. To address this we treat the lowest
    // two bits as a mutually exclusive value indicating ro, wo, rw.
    UINT32 openType = mode & 0x03;

    // if truncate not specified explicitly we open for appending
    if( !(mode & O_TRUNC) && O_RDONLY != openType )
    {
        mode |= O_APPEND;
    }

    const char* pszAppend = 0;
    if (O_RDWR == openType)
    {
        if(mode & O_APPEND)
        {
            // read and writing/append; creates file if it doesn't exist
            pszAppend = "a+"; 
        }
        else
        {
            if(mode & O_CREAT)
            {
                // read/write; creates file if it doesn't
                // exist; if file exists contents are destroyed
                pszAppend = "w+";
            }
            else
            {
                // read/write; file must exist
	        pszAppend = "r+"; 
            }
        }
    }
    else if (O_WRONLY == openType)
    {
        if(mode & O_APPEND)
        {
            // write/append; creates file if it doesn't exist
            pszAppend = "a"; 
        }
        else
        {
            if(mode & O_CREAT)
            {
                // write only; if file exists contents are destroyed
	        pszAppend = "w"; 
            }
            else
            {
                // reading and writing (closest we can do); file must exist
                pszAppend = "r+";
            }
        }
    }
    else if (O_RDONLY == openType)
    {
        // read only; file must exist
	pszAppend = "r"; 
        HX_ASSERT(!(mode & O_CREAT));
    }
    
    HX_ASSERT(pszAppend);
    
    if( pszAppend )
    {
        strcpy(buf, pszAppend);

        if ((mode & O_BINARY) && !bText)
        {
	    pszAppend = "b";
        }
        else
        {
            pszAppend = "t";
        }
        strcat(buf,pszAppend);
    }

    return buf;
}

/* Create creates a file using the specified fcntl.h mode */
HX_RESULT
CHXDataFile::Create(const char *filename, UINT16 mode, HXBOOL textFlag)
{
    mode |= O_CREAT;
    return Open(filename, mode, textFlag);
}



/* Open will open a file with the specified fcntl.h permissions */
HX_RESULT
CHXDataFile::Open(const char *filename, UINT16 mode, HXBOOL textFlag)
{
    HX_RESULT theErr = HXR_FAIL;
  
    char fopenFlags[CCH_MAX_FOPENFLAG];
    
    if( TranslateOpenMode(mode, textFlag, fopenFlags) )
    {
        m_FP = fopen( filename, fopenFlags );
        if(m_FP)
        {
           theErr = HXR_OK;
        }
    }
    return theErr;
}

/* Close closes a file */
HX_RESULT
CHXDataFile::Close(void)
{
    if(!m_FP)
    {
        return HXR_FAIL;
    }
    fclose(m_FP);
    m_FP = NULL;
    return HXR_OK;
}

/* Seek moves the current file position to the offset from the fromWhere specifier */
HX_RESULT
CHXDataFile::Seek(ULONG32 offset, UINT16 fromWhere)
{
    if(!m_FP)
    {
        return HXR_FAIL;
    }
    fseek( m_FP, offset, fromWhere );
    return HXR_OK;
}

/* Tell returns the current file position in the ra file */
ULONG32	
CHXDataFile::Tell(void)
{
    if(!m_FP)
    {
        return 0;
    }
    return ftell( m_FP );
}

/* Read reads up to count bytes of data into buf.
returns the number of bytes read, EOF, or -1 if the read failed */
ULONG32	
CHXDataFile::Read(char* buf, ULONG32 count)
{
    if(!m_FP)
    {
        return 0;
    }
    return fread( buf, 1, count, m_FP );
}

/* Write writes up to count bytes of data from buf.
returns the number of bytes written, or -1 if the write failed */
ULONG32
CHXDataFile::Write(const char *buf, ULONG32 count)
{
    if(!m_FP)
    {
        return 0;
    }
    ULONG32 numWritten = fwrite( (void*) buf, 1, count, m_FP );
    return numWritten;
}

/* Rewinds the file to the start of the file */
HX_RESULT
CHXDataFile::Rewind(void)
{
    if(!m_FP)
    {
        return HXR_FAIL;
    }

    fseek( m_FP, 0, SEEK_SET );
    return HXR_OK;
}

/* Delete deletes a file */
HX_RESULT
CHXDataFile::Delete(const char *filename)
{
    if(!filename)
    {
        return HXR_FAIL;
    }
    unlink(filename);
    return HXR_OK;
}

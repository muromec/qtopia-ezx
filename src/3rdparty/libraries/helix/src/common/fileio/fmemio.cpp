/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fmemio.cpp,v 1.8 2004/07/09 18:20:35 hubbe Exp $
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
#include "hlxclib/errno.h"

#include "hxtypes.h"

#include "fmemio.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif



FMEMIO::FMEMIO(CHXDataFileMem*	mem_file)
{
    mMemFile = mem_file;
    if (!mMemFile)
    {
	err = -1;
	return;
    }

    err = 0;
    _flags = (int) O_RDONLY;
}

FMEMIO::~FMEMIO()
{
}

LONG32
FMEMIO::read(void* buf, LONG32 size)
{
    HX_ASSERT(buf);
    LONG32 lActualRead = mMemFile->Read((char *) buf, (ULONG32) size);

    // MemFile returns 0 bytes read if it is still waiting for more
    // data to arrive...
    // in this case set err to wouldblock and return -1
    if (lActualRead == 0)
    {
	err = EWOULDBLOCK;
	return -1;
    }
    // if there was some error in reading or it is end of file
    // memfile returns -1...
    /// in this case return 0 which marks end of file for fsio..
    else if (lActualRead < 0)
    {
	err = mMemFile->GetLastError(); 
	return 0;
    }

    err = 0;
    // if everything is fine, return actual bytes read...
    return lActualRead;
}

LONG32
FMEMIO::write(const void* buf, LONG32 size)
{
    HX_ASSERT(FALSE);
    return 0;
}

off_t 
FMEMIO::seek(off_t off, LONG32 whence)
{
    HX_RESULT theErr = HXR_OK;

    theErr = mMemFile->Seek(off, (UINT16) whence);

    if (!theErr)
    {
	err = 0;
	return (off_t) mMemFile->Tell();
    }
    else
    {
	err = theErr;
	return (off_t) -1;
    }
}

LONG32
FMEMIO::close()
{
    mMemFile->Close();
    err = 0;
    return 0;
}

off_t
FMEMIO::file_size()
{
    HX_ASSERT(FALSE);
    return 0;

    // not supported...
    // seek to SEEK_END is not supported by CRaDataFileMem 
    /*
       ULONG32 lCurrPos = mMemFile->Tell();

       mMemFile->Seek(0, SEEK_END);

       ULONG32 lFileSize = mMemFile->Tell();

       mMemFile->Seek(lCurrPos, SEEK_SET);

       err = 0;
       return (off_t) lFileSize;
     */
}


LONG32
FMEMIO::error()
{
    return err;
}

LONG32
FMEMIO::flags()
{
    return _flags;
}

/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxdataf.cpp,v 1.8 2006/02/07 19:21:11 ping Exp $
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

#include "chxdataf.h"

#include "hxassert.h"	// hxassert.h is NOT a system header
#include "hxheap.h"
#include "hxbuffer.h"
#include "pckunpck.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define MAX_UNCHECKED_READ_LENGTH   4096


// CHXFile should set the file reference to a value  
// indicating the file is not open
CHXDataFile::CHXDataFile (IUnknown* pContext)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    mLastError = 0;
}

// ~CHXFile should close the file if it is open
CHXDataFile::~CHXDataFile(void)
{
    HX_RELEASE(m_pContext);
    // override 
}

HXBOOL CHXDataFile::GetTemporaryFileName(const char *tag, char* name, UINT32 ulBufLen)
{
	return FALSE;
}


HX_RESULT CHXDataFile::ReadToBuffer(ULONG32 ulCount, IHXBuffer** ppbufOut)
{
    HX_RESULT	hr = HXR_FAILED;
    UINT32	size = 0;
    UINT32	filesize = 0;
    UINT32	sizeRead = 0;
    IHXBuffer*	buffer = NULL;

    // Initialize...
    *ppbufOut = NULL;

    // The size to be copied...
    size = (LONG32) ulCount;
    
    if (size > MAX_UNCHECKED_READ_LENGTH)
    {
	// Get the size of the file...
	filesize = (LONG32) GetSize();
	
	// Create the buffer of the appropriate size...
	if (size > filesize)
	{
	    size = filesize;
	}
    }
    
    // Create the buffer...
    CreateBufferCCF(buffer, m_pContext);
    if (buffer)
    {
	buffer->AddRef();
	hr = buffer->SetSize(size);
	if (hr == HXR_OK)
	{
	    sizeRead = 0;
	    sizeRead = Read((char*) buffer->GetBuffer(), size);
	    if (sizeRead == size)
	    {
		*ppbufOut = buffer;
		buffer = NULL;
	    }
	    else
	    {
		if (sizeRead > size) 
		{
		    hr = HXR_INVALID_FILE;
		}
		else
		{
		    hr = HXR_FAIL;

		    if (sizeRead != 0)
		    {
			hr = buffer->SetSize(sizeRead);

			if (hr == HXR_OK)
			{
			    *ppbufOut = buffer;
			    buffer = NULL;
			}
		    }
		}
	    }
	}
    }
    	    
    HX_RELEASE(buffer);

    return hr;
}

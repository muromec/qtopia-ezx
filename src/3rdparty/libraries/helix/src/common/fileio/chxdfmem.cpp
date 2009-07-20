/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxdfmem.cpp,v 1.9 2004/07/09 18:20:35 hubbe Exp $
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

#ifdef __MWERKS__
#pragma once
#endif

#include "chxdfmem.h"
#include "hlxclib/stdio.h"		// for SEEK_*
#if !defined(_VXWORKS) && !defined(_SYMBIAN) && !defined(_OPENWAVE)
#include <memory.h>		// for memcpy
#endif

#if defined(_SYMBIAN) || defined(_OPENWAVE)
# include "hlxclib/string.h" //for memcpy
#endif

#ifdef _SOLARIS24
#include <string.h>		// for memcpy
#endif

#include "chunkres.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

/*	Seek moves the current file position to the offset from the fromWhere specifier */

HX_RESULT CHXDataFileMem::Seek(ULONG32 offset, UINT16 fromWhere)
{
	switch(fromWhere)
	{
		case SEEK_SET:		// Beginning of file
		{
			m_Offset = offset;
		}
		break;

		case SEEK_CUR:		// Current position of file pointer
		{
			m_Offset += offset;
		}
		break;

		case SEEK_END:		// End of file
		{
			return HXR_FAILED; // Not Supported
		}

	};

#ifdef DONT_USE_CHUNKYRES /////////////////////////
	if (m_Offset > m_MemSize)
#else
    if (m_Offset > m_pChunkyRes->GetContiguousLength()) 
#endif
	{
		return HXR_BUFFERING;
	}

	return HXR_OK;
}

/* 	Tell returns the current file position in the ra file */
ULONG32	CHXDataFileMem::Tell()
{
	return m_Offset;
}

/* 	Read reads up to count bytes of data into buf.
	returns the number of bytes read, EOF, or -1 if the read failed */
ULONG32	CHXDataFileMem::Read(char* buf, ULONG32 count)
{
	ULONG32 actual = count;

#ifdef DONT_USE_CHUNKYRES /////////////////////////

	// If the offset is greater than we've seeked to, then
	// we can just ignore things....
	if (m_Offset > m_MemSize)
	{
		return 0;
	}

	// If the current offset plus the requested number of bytes
	// is more than we've read, then we will reduce the actual
	// count to what we have available.
	if (m_Offset + actual > m_MemSize)
	{
		actual = m_MemSize-m_Offset;
	}

	if (actual > 0)
	{
		memcpy(buf,m_pMemData+m_Offset,actual); /* Flawfinder: ignore */
		m_Offset += actual;
	}
	else
	{
		actual = (ULONG32)-1;
	}
	return actual;

#else
	if (!m_pChunkyRes)
	{
		return 0;
	}

	if (m_Offset > m_pChunkyRes->GetContiguousLength())
	{
		return 0;
	}

	// If the current offset plus the requested number of bytes
	// is more than we've read, then we will reduce the actual
	// count to what we have available.
	if (m_Offset + actual > m_pChunkyRes->GetContiguousLength())
	{
		actual = m_pChunkyRes->GetContiguousLength()-m_Offset;
	}

	if (actual > 0)
	{
		HX_RESULT tempErr = m_pChunkyRes->GetData(m_Offset,buf,actual,&actual);
		if (tempErr != HXR_OK)
		{
			mLastError = tempErr;
			actual = (ULONG32)-1;
		}
		else
		{
			m_Offset += actual;
		}
	}
	else
	{
		actual = (ULONG32)-1;
	}
	return actual;

#endif

}

/* 	Rewinds the file to the start of the file */
HX_RESULT CHXDataFileMem::Rewind()
{
	m_Offset = 0;
	return HXR_OK;
}


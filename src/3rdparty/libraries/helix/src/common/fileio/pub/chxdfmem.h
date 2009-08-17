/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxdfmem.h,v 1.5 2006/02/07 19:21:14 ping Exp $
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

#ifndef _CHXDFMEM_H_
#define	_CHXDFMEM_H_

#include "hxtypes.h"
#include "hxresult.h"
#include "chxdataf.h"

#include "chunkres.h"

class CHXDataFileMem : public CHXDataFile
{

public:
	CHXDataFileMem(IUnknown* pContext):CHXDataFile(pContext)
			{
				m_Offset = 0;
#ifdef DONT_USE_CHUNKYRES /////////////////////////
				m_MemSize = 0;
				m_pMemData = NULL;
#else
				m_pChunkyRes = NULL;
#endif
			};

/* 	Create creates a file using the specified mode */
	virtual HX_RESULT	Create				(const char *filename, UINT16 mode, HXBOOL textflag = 1)
												{ return HXR_FAILED; }

	/* 	Open will open a file with the specified permissions */
	virtual HX_RESULT	Open				(const char *filename, UINT16 mode, HXBOOL textflag = 1)
												{ return HXR_FAILED; }

/*	Close closes a file */
	virtual HX_RESULT	Close				(void)
												{ return HXR_OK; }
	
/*	Seek moves the current file position to the offset from the fromWhere specifier */
	virtual HX_RESULT	Seek				(ULONG32 offset, UINT16 fromWhere);

/* 	Tell returns the current file position in the ra file */
	virtual ULONG32		Tell				(void);

/* 	Read reads up to count bytes of data into buf.
	returns the number of bytes read, EOF, or -1 if the read failed */
	virtual ULONG32		Read				(char* buf, ULONG32 count);

/* 	Write writes up to count bytes of data from buf.
	returns the number of bytes written, or -1 if the write failed */
	virtual ULONG32		Write				(const char* buf, ULONG32 count)
												{ return (ULONG32)-1; }

/* 	Rewinds the file to the start of the file */
	virtual HX_RESULT	Rewind				(void);

/*  Return the file descriptor                      */
    virtual INT16		GetFd				(void)
												{ return -1; }

#ifdef DONT_USE_CHUNKYRES /////////////////////////
	// Used to set pointers to actual memory data...
	void				SetMemData(ULONG32 MemSize, const char* pMemData)
							{
								m_MemSize	= MemSize;
								m_pMemData	= pMemData;
							};
#else
	void				SetChunkyRes(CChunkyRes* pChunkyRes)
							{
								m_pChunkyRes = pChunkyRes;
							}
#endif

private:
	ULONG32				m_Offset;
#ifdef DONT_USE_CHUNKYRES /////////////////////////
	ULONG32				m_MemSize;
	const char*			m_pMemData;
#else
	CChunkyRes*			m_pChunkyRes;
#endif
};
	
#endif // _CHXDFMEM_H_





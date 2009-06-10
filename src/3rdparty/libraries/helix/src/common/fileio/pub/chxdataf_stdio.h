/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxdataf_stdio.h,v 1.9 2009/02/27 22:55:09 shivnani Exp $
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

#ifndef _CHXDATAFILE_H_
#define	_CHXDATAFILE_H_

#include <stdio.h>
#include "hlxclib/fcntl.h"
#include "hxcom.h"
#include "ihxfgbuf.h"

const UINT32 WIN_FILE_NORM_IO	=   0;
const UINT32 WIN_FILE_MEMMAP_IO	=   1;
const UINT32 WIN_FILE_ASYNC_IO	=   2;

// Sharing flags
#ifndef SH_DENYRW
  #define SH_DENYRW	0x10	/* deny read/write mode */
#endif
#ifndef SH_DENYWR
  #define SH_DENYWR	0x20	/* deny write mode */
#endif
#ifndef SH_DENYRD
  #define SH_DENYRD	0x30	/* deny read mode */
#endif
#ifndef SH_DENYNO
  #define SH_DENYNO	0x40	/* deny none mode */
#endif

class CHXDataFile 
{
public:
	static	CHXDataFile*	Construct		(IUnknown* pContext, UINT32 ulFlags = 0, IUnknown** ppCommonObj = NULL);
	
	virtual			~CHXDataFile		(void);

	HXBOOL	 	GetTemporaryFileName	(const char *tag, char* name, UINT32 ulBufLen);

/* 	GetLastError returns the platform specific file error */
	virtual HX_RESULT	GetLastError		(void)	{return mLastError;};
/* 	Create creates a file using the specified mode */
	HX_RESULT	Create			(const char *filename, UINT16 mode, HXBOOL textflag = 0);

	/* 	Open will open a file with the specified permissions */
	HX_RESULT	Open			(const char *filename, UINT16 mode, HXBOOL textflag = 0);

/* 	Open will open a file for sharing with the specified permissions */
	HX_RESULT	OpenShared    		(const char *filename, UINT16 mode, UINT16 sharedmode, HXBOOL textflag = 0) {return Open(filename, mode, textflag);}

/*	Close closes a file */
	HX_RESULT	Close			(void);

/*	Returns the size of the file in bytes. */
	ULONG32		GetSize			(void) { return 0;};

/*      Changes the size of the file to the newSize */
	HX_RESULT	ChangeSize		(ULONG32 newSize) { return HXR_NOTIMPL; };
	
/*	Seek moves the current file position to the offset from the fromWhere specifier */
	HX_RESULT	Seek			(ULONG32 offset, UINT16 fromWhere);

/* 	Tell returns the current file position in the ra file */
	ULONG32		Tell			(void);

/* 	Read reads up to count bytes of data into buf.
	returns the number of bytes read, EOF, or -1 if the read failed */
	ULONG32		Read			(char* buf, ULONG32 count);

/* 	Write writes up to count bytes of data from buf.
	returns the number of bytes written, or -1 if the write failed */
	ULONG32		Write			(const char *buf, ULONG32 count);

/* 	Rewinds the file to the start of the file */
	HX_RESULT	Rewind			(void);

/* 	Delete deletes a file */
	HX_RESULT	Delete			(const char *filename);

	HX_RESULT	ReadToBuffer		(ULONG32 len, IHXBuffer** buf){ return HXR_NOTIMPL; }
	int	        GetFd		        (){ return 0; }

protected:
        IUnknown*               m_pContext;
        HX_RESULT               mLastError;
        FILE *                  m_FP;
        CHXDataFile (IUnknown* pContext);
};
	
#endif // _CHXDATAFILE_H_		





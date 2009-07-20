/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxdataf_virtual.h,v 1.6 2006/02/07 19:21:14 ping Exp $
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

#ifndef _CHXDATAFILE_H_
#define	_CHXDATAFILE_H_

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

/* 	GetLastError returns the platform specific file error */
	virtual HX_RESULT	GetLastError		(void)	{return mLastError;};

	virtual HXBOOL	 	GetTemporaryFileName	(const char *tag, char* name, UINT32 ulBufLen);

/* 	Create creates a file using the specified mode */
	virtual HX_RESULT	Create			(const char *filename, UINT16 mode, HXBOOL textflag = 0)= 0;

	/* 	Open will open a file with the specified permissions */
	virtual HX_RESULT	Open			(const char *filename, UINT16 mode, HXBOOL textflag = 0) = 0;

/* 	Open will open a file for sharing with the specified permissions */
	virtual HX_RESULT	OpenShared    		(const char *filename, UINT16 mode, UINT16 sharedmode, HXBOOL textflag = 0) {return Open(filename, mode, textflag);}

/*	Close closes a file */
	virtual HX_RESULT	Close			(void) = 0;

/*	Returns the size of the file in bytes. */
	virtual ULONG32		GetSize			(void) { return 0;};

/*      Changes the size of the file to the newSize */
	virtual HX_RESULT	ChangeSize		(ULONG32 newSize) { return HXR_NOTIMPL; };
	
/*	Seek moves the current file position to the offset from the fromWhere specifier */
	virtual HX_RESULT	Seek			(ULONG32 offset, UINT16 fromWhere) = 0;

/* 	Tell returns the current file position in the ra file */
	virtual ULONG32		Tell			(void) = 0;

/* 	Read reads up to count bytes of data into buf.
	returns the number of bytes read, EOF, or -1 if the read failed */
	virtual ULONG32		Read			(char* buf, ULONG32 count) = 0;

/* 	Read reads up to ulCount bytes of data into ppbufOut.
	returns a result code */
	virtual HX_RESULT	ReadToBuffer		(ULONG32 ulCount, IHXBuffer** ppbufOut);

/* 	Write writes up to count bytes of data from buf.
	returns the number of bytes written, or -1 if the write failed */
	virtual ULONG32		Write			(const char *buf, ULONG32 count) = 0;

/* 	Rewinds the file to the start of the file */
	virtual HX_RESULT	Rewind			(void) = 0;

	virtual HX_RESULT	set_buffered_read	(char buffered) {return HXR_OK;};

	virtual HX_RESULT	set_buffered_write	(char buffered) {return HXR_OK;};

/*	Return the file descriptor                      */
	virtual INT16		GetFd			(void) = 0;

/* 	Delete deletes a file */
	virtual HX_RESULT	Delete			(const char *filename)= 0;

protected:
	CHXDataFile		(IUnknown* pContext);

	IUnknown*		m_pContext;
	HX_RESULT		mLastError;	// last error to occur
};
	
#endif // _CHXDATAFILE_H_		





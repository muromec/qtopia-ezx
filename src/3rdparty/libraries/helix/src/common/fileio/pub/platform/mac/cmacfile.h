/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cmacfile.h,v 1.7 2007/07/06 20:35:19 jfinnecy Exp $
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

#ifndef __CMACFILE__
#define __CMACFILE__


#include "CHXDATAF.H"
#include "CBufferFile.h"		// for asynchronous buffered file reads						
#include "CWriteFile.h"			// for asynchronous buffered file writes						

// VERY IMPORTANT NOTE: All char *filename must be typecast to
// FSSpecPtr before using. All callers must also typecast FSSpecPtr
// to char *

/*enum
{
	SEEK_SET = 0,
	SEEK_CUR,
	SEEK_END
};*/


class CMacFile : public CHXDataFile {
public:

						CMacFile			(IUnknown* pContext);
				   		~CMacFile			(void);

	static	OSType		sFileType;
	static	OSType		sCreator;

/* 	GetLastError returns the platform specific file error */
	virtual HX_RESULT	GetLastError		(void)	{return mLastError;};

	virtual HXBOOL	 	GetTemporaryFileName(const char *tag, char* name, UINT32 ulBufLen);
	
/* 	Create creates a file using the specified mode */
	virtual HX_RESULT	Create				(const char *filename, UINT16 mode, HXBOOL textflag= 0);

	/* 	Open will open a file with the specified permissions 
	 *
	 *  If the file with the given path isn't found, it looks for 
	 *  a kRealFileResource resource with the same name (file name, 
	 *  not path name).
	 */
	virtual HX_RESULT	Open				(const char *filename, UINT16 mode, HXBOOL textflag = 0);

/*	Close closes a file */
	virtual HX_RESULT	Close				(void);
	
/*	Seek moves the current file position to the offset from the fromWhere specifier */
	virtual HX_RESULT	Seek				(ULONG32 offset, UINT16 fromWhere);

/*	Returns the size of the file in bytes. */
	virtual ULONG32		GetSize(void);

/* 	Tell returns the current file position in the file */
	virtual ULONG32		Tell(void);

/* 	Rewind sets the current file position to the start of the file */
	virtual HX_RESULT	Rewind(void);

/* 	Read reads up to count bytes of data into buf.
	returns the number of bytes read, EOF, or -1 if the read failed */
	virtual ULONG32		Read(char *buf, ULONG32 count);

/* This read function is used in the pnlib implementation*/
			INT16 		Read(char *buf, INT32 count, INT32 *actualCount);

/* 	Write writes up to count bytes of data from buf.
	returns the number of bytes written, or -1 if the write failed */
	virtual UINT32		Write(const char *buf, ULONG32 count);

/* This write function is used in the pnlib implementation*/
			INT16 		Write(const char *buf, INT32 count, INT32 *actualCount);

#ifndef _CARBON
	static	OSErr 		FSOpenFile			(FSSpec *theSpec, 
											 short perm,
											 short *fRefNum);

	static	OSErr		FSCreateDataFile	(FSSpec *sfFile, 
											 OSType creator, 
											 OSType type);

	static	OSErr 		FSSetFInfo			(FSSpec *theSpec, 
											 OSType creator, 
											 OSType type);

	static	OSErr		FSSetEOF			(short fRefNum, 
											 long size);
											 
											 
	static	OSErr		FSSetFilePos		(short fRefNum, 
											 short fromWhere,  
											 long offset);
#endif

	static	void		SetFileType			(OSType creator, 
											 OSType type);
											 
	virtual HX_RESULT	set_buffered_read	(char buffered);
	virtual HX_RESULT	set_buffered_write	(char buffered);
											 									
/*      Return the file descriptor                      */
    
        virtual INT16          GetFd(void) { return mRefNum; };

/* 	Delete deletes a file */
	virtual HX_RESULT	Delete				(const char *filename);

	INT16 			FileSize	(long *size);

	Boolean			BufferedWrite(void) { return mBufferedWrite; }

protected:

				long 	Copy				(Ptr destBuf, long numBytes);

#ifdef _CARBON
	FSRef		mFile;
#else
	FSSpec		mFile;
#endif
	short	 	mRefNum;		// file refnum (-1 if not open)
	
	Boolean		mAppendMode;
	
	Boolean			mBufferedRead;	// async buffered read enabled flag
	Boolean			mBufferedWrite;	// async buffered write enabled flag
	CBufferFile		*mBufferFile;	//  buffered read object
	CWriteFile		*mWriteFile;	//  buffered write object
	
	UINT16			m_mode;			//  mode that the file was opened with.

	// pseudo-file support
	//
	// mRefNum is -1 when we're using a pseudo-file, and
	// m_pseudoFileHandle is non-nil
	//
	// m_pseudoFileHandle is a handle to a resource that is not loaded
	// into memory, since we'll use ReadPartialResource to read it
	// directly from disk
	
	Handle			m_pseudoFileHandle;	// if non-nil, our resource handle
	SInt32			m_pseudoFileSize;	// size of the handle
	SInt32			m_pseudoFileOffset;	// pseudo-file pointer (0..m_pseudoFileSize-1)
};
	

#define kRealFileResource	'RLFL'		// resource type for pseudo-file resources

#endif


/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cmacfile_carbon.cpp,v 1.10 2007/07/06 20:35:13 jfinnecy Exp $
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

#include <fcntl.h>

#include "CMacFile.h"

#include "MoreFilesX.h"
#include "filespecutils.h"
#include "hx_moreprocesses.h"
#include "hxstrutl.h"

OSType CMacFile::sCreator	= 'PNst';
OSType CMacFile::sFileType 	= 'PNRA';

HXBOOL gReadDone = FALSE;
ULONG32 gReadCount = 0;



// async callback proc
pascal void ReadCallback(ParamBlockRec* pb);

// async callback UPP
static IOCompletionUPP			gReadCallbackUPP=NewIOCompletionUPP(ReadCallback);


CHXDataFile*
CHXDataFile::Construct(IUnknown* pContext, UINT32 ulFlags, IUnknown** ppCommonObj)
{
    return (CHXDataFile *) new CMacFile(pContext);
}

// CHXFile should set the file reference to a value indicating the file is not open
CMacFile::CMacFile (IUnknown* pContext)
	 :CHXDataFile(pContext)
{
	mRefNum = 0;
	mLastError = noErr;
	mAppendMode = FALSE;
	mBufferFile = NULL;
	mBufferedRead = FALSE;
	mWriteFile = NULL;
	mBufferedWrite = FALSE;
	
	m_pseudoFileHandle = NULL;	// if non-null, a memory manager handle containing the "file data"
	m_pseudoFileOffset = 0;		// current "read" position in the pseudo file
	m_pseudoFileSize = 0;		// total pseudo file size
}

// ~CHXFile should close the file if it is open
CMacFile::~CMacFile(void)
{
	Close();		// close file if necessary
}

// Create a file with the specified mode
// Close the previous file if it was open
HX_RESULT CMacFile::Create(const char *filename, UINT16 mode,HXBOOL textflag)
{
	Close();		// close previous file if necessary
	
	CHXFileSpecifier fileSpec(filename);
	
	require_return(fileSpec.IsSet(), HXR_INVALID_PATH);

	HXBOOL bExistsAlready;
	
	OSErr err = noErr;
	
	// if the file already exists, it might be an alias file to somewhere else
	// (well, the pre-Carbon code checked for this; I'm not sure why we're resolving
	// an alias file if this path already exists)
	
	bExistsAlready = CHXFileSpecUtils::FileExists(fileSpec);
	if (bExistsAlready)
	{
		CHXFileSpecUtils::ResolveFileSpecifierAlias(fileSpec);
		
		bExistsAlready = CHXFileSpecUtils::FileExists(fileSpec);
	}
	
	// create the file if it doesn't already exist
	FSRef parentRef;
	HFSUniStr255 hfsName;
	FSRef openFileRef;
	
	parentRef = (FSRef) fileSpec.GetParentDirectory();
	hfsName = fileSpec.GetNameHFSUniStr255();
	
	if (!bExistsAlready)
	{
		FSCatalogInfo catInfo;
		FileInfo fInfo;
		FSSpec *kDontWantSpec = NULL;
		
		ZeroInit(&fInfo);
		fInfo.fileCreator = textflag ? 'ttxt' : sCreator;
		fInfo.fileType = textflag ? 'TEXT' : sFileType;
		
		(* (FileInfo *) &catInfo.finderInfo) = fInfo;
		
		err = FSCreateFileUnicode(&parentRef, hfsName.length,
			hfsName.unicode, kFSCatInfoFinderInfo, &catInfo,
			&openFileRef, kDontWantSpec);
	}
	else // already existed...
	{
		openFileRef = (FSRef) fileSpec;
	}
	
	// open the file (to mimic Windows' create semantics)
	if (err == noErr)
	{
		HFSUniStr255 dataForkName;
		
		dataForkName.length = 0;
		err = FSGetDataForkName(&dataForkName);
		check_noerr(err);
		
		err = FSOpenFork(&openFileRef, dataForkName.length, dataForkName.unicode,
			fsRdWrPerm, &mRefNum);
	}
	
	if (err == noErr)
	{
		mFile = openFileRef;
		
		err = FSSetForkSize(mRefNum, fsFromStart, 0);
	}
	
	mLastError = err;
	
	return (err != noErr ? -1 : 0);
}

// Open a file with the specified permissions
// Close the previous file if it was open
//
// If the file isn't found, look for a 'RLFL' resource *in the current
// resource chain* with the same name as the file.

HX_RESULT CMacFile::Open(const char *filename, UINT16 mode,HXBOOL textflag)
{
	Close();		// close previous file if necessary
	
	CHXFileSpecifier fileSpec(filename);
	
	if (!fileSpec.IsSet()) return HXR_INVALID_PATH; // fails if we're trying to open a file inside a non-existing directory
	
	short perm;
	
	// figure out mac file permission
	perm = fsRdPerm;
	if (mode & O_WRONLY)
	{	
		perm = fsWrPerm;
	}
	else if (mode & O_RDONLY)
	{
		perm = fsRdPerm;
	}
        else if (mode & O_RDWR)
        {
                perm = fsRdWrPerm;
        }
	
	// Store the permissions for this file for later.
	m_mode = mode;

	HXBOOL bExistsAlready;
		
	OSErr err = fnfErr;
	
	bExistsAlready = CHXFileSpecUtils::FileExists(fileSpec);
	if (bExistsAlready)
	{
		CHXFileSpecUtils::ResolveFileSpecifierAlias(fileSpec);
		
		bExistsAlready = CHXFileSpecUtils::FileExists(fileSpec);
	}
	
	if (!bExistsAlready && ((mode & O_CREAT) || (mode & O_WRONLY)))
	{
		return Create(filename, mode, textflag);
	}
	
	if (bExistsAlready)
	{
		HFSUniStr255 dataForkName;
		FSRef openFileRef;
		
		dataForkName.length = 0;
		err = FSGetDataForkName(&dataForkName);
		check_noerr(err);
		
		openFileRef = (FSRef) fileSpec;
		
		err = FSOpenFork(&openFileRef, dataForkName.length, dataForkName.unicode,
			perm, &mRefNum);
			
		if (err == noErr)
		{
			mFile = openFileRef;
		}
	}
	
	if ((err != noErr) && (mode & O_RDONLY))
	{
		Handle	resHandle;		
		SInt32	resSize;
		Str255	pascFileName;
		
		// We couldn't open the file, and the request was read-only
		//
		// See if there's a pseudo-file resource we can use
		//
		// We need a handle to the resource so we can read from it, but
		// we don't want the whole resource loaded into memory, so we
		// set ResLoad to false.
		
		SetResLoad(false);
		fileSpec.GetName().MakeStr255(pascFileName);
		resHandle = GetNamedResource(kRealFileResource, pascFileName); // 'RLFL'
		SetResLoad(true);
		
		if (resHandle)
		{
			// we have a handle to the resource; determine
			// its size and reset our "file pointer"
			
			resSize = GetResourceSizeOnDisk(resHandle);
			if (resSize > 0)
			{
				m_pseudoFileHandle = resHandle;
				m_pseudoFileSize = resSize;
				m_pseudoFileOffset = 0;
				
				err = noErr;
				mRefNum = -1;	// signals that we're using a pseudo-file and no actual file is open
			}
		}
	}
	
	if (!m_pseudoFileHandle)
	{
		if((err == noErr) && (mode & O_CREAT))
		{
			err = ::SetEOF(mRefNum, 0L);
		}
		
		if(err == noErr)
		{
			mAppendMode = (mode & O_APPEND);
			if (mode & O_TRUNC) 
			{
				err = ::SetEOF(mRefNum, 0L);
			}
		}

		if (err != noErr && mRefNum != 0) 
		{
			Close();
		}
	}
	
	mLastError = err;
	
	return(err != noErr ? HXR_DOC_MISSING : 0);	
}

// Close the previous file if it was open
HX_RESULT CMacFile::Close(void)
{
	OSErr err = noErr;

	if (m_pseudoFileHandle)
	{
		// "close" our pseudo file
		//
		// We don't need or want to dispose or release our pseudo-
		// file handle since it's not using up memory anyway, and
		// releasing it would hurt anyone else who happens to be
		// reading from it.  The handle will be released 
		// automatically when its owning resource file closes.
		
		m_pseudoFileHandle = 0;
		m_pseudoFileOffset = 0;
		m_pseudoFileSize = 0;
		err = noErr;
		mRefNum = 0;
	}
	
	else if (mRefNum) 
	{
		if (mBufferFile)
		{ 
			delete mBufferFile;
			mBufferFile = NULL;
		}
		
		if (mWriteFile) 
		{
			delete mWriteFile;
			mWriteFile = NULL;
		}
		
		OSErr tempErr;
		FSVolumeRefNum vRefNum;
		
		tempErr = FSGetVRefNum(&mFile, &vRefNum);

		// close a real file
		err = ::FSCloseFork(mRefNum);
		mRefNum = 0;
		if (err == noErr && tempErr == noErr) 
		{
			tempErr = ::FlushVol(nil, vRefNum);
		}
	}
	
	mLastError = err;
	return(err != noErr ? -1 : 0);	

}

HX_RESULT	CMacFile::Delete	(const char *filename)
{
	HX_RESULT res;
	
	require_return(!m_pseudoFileHandle, HXR_INVALID_OPERATION);
	
	CHXFileSpecifier fileSpec(filename);
	
	res = CHXFileSpecUtils::RemoveFile(fileSpec);
	
	return SUCCEEDED(res) ? 0 : -1;
	
}

/*	Returns the size of the file in bytes. */
ULONG32		CMacFile::GetSize(void)
{
	ULONG32 fileSize = 0;

	if (m_pseudoFileHandle)
	{
		fileSize = m_pseudoFileSize;
	}

	else if (mRefNum) 
	{
		OSErr err;
		SInt64 forkSize;
		
		err = FSGetForkSize(mRefNum, &forkSize);
		if (err == noErr)
		{
			fileSize = (ULONG32) forkSize;
		}
	}
	else
	{
		check(!"Cannot get file size because no file is open");
	}
	
	return fileSize;
 }

// Rewinds the file position to the start of the file
HX_RESULT CMacFile::Rewind(void)
{
	OSErr err;
	if (m_pseudoFileHandle)
	{
		m_pseudoFileOffset = 0;
		err = noErr;
	}
	else
	{
		err = ::FSSetForkPosition(mRefNum, fsFromStart, 0);
	}
	mLastError = err;
	return(err != noErr ? HXR_INVALID_FILE : HXR_OK);
}
	

// Seek moves the current file position to the offset from the fromWhere specifier
HX_RESULT CMacFile::Seek(ULONG32 offset, UINT16 fromWhere)
{
	OSErr err = noErr;
		
	if (m_pseudoFileHandle)
	{
		switch(fromWhere)
		{
			case SEEK_SET:
				m_pseudoFileOffset = offset;
				break;

			case SEEK_CUR:
				m_pseudoFileOffset += offset;
				break;

			case SEEK_END:
				m_pseudoFileOffset = (m_pseudoFileSize - 1) - offset;
				break;
		}
		
		// don't go beyond the end (we won't return eofErr either to match
		// the real seek below)
		if (m_pseudoFileOffset >= m_pseudoFileSize)
		{
			m_pseudoFileOffset = m_pseudoFileSize - 1;
		}
		err = HXR_OK;
	}
	else if (mBufferedWrite)
	{
		long pos = 0;
		switch(fromWhere)
		{
			case SEEK_SET:
				pos = offset;
				break;

			case SEEK_CUR:
				pos = mWriteFile->GetCurPos() + offset;
				break;

			case SEEK_END:
				pos = (mWriteFile->GetBufSize() - 1) - offset;
				break;
		}
		mWriteFile->Seek(pos);
	}
	else
	{
		switch(fromWhere)
		{
			case SEEK_SET:
				fromWhere = fsFromStart;
				break;

			case SEEK_CUR:
				fromWhere = fsFromMark;
				break;

			case SEEK_END:
				fromWhere = fsFromLEOF;
				break;
		}
		
		err =  ::SetFPos(mRefNum, fromWhere, offset);

		// returning eofErr was causing problems for ChunkyRes during http play
		if (err == eofErr)
		{
			err = ::SetEOF(mRefNum, offset);
			err = ::SetFPos(mRefNum, fromWhere, offset);
			err = HXR_OK;
		}
			
		long pos;
		::GetFPos(mRefNum,(long *)&pos);

		if(!err && mBufferedRead)
		{
			long count; 
			::GetEOF(mRefNum, &count);
			mBufferFile->PreLoad(pos,count - offset);
		}

		/* if(!theErr && mBufferedWrite)
		{
			mWriteFile->Seek(pos);
		} */
	}
	mLastError = err;
	return(err != noErr ? HXR_INVALID_FILE : 0);	
}

// Tell 
ULONG32 CMacFile::Tell(void)
{
	ULONG32	pos;

	if (m_pseudoFileHandle)
	{
		pos = m_pseudoFileOffset;
		mLastError = noErr;
	}
	else
	{
		mLastError = ::GetFPos(mRefNum,(long *)&pos);
	}
	return(pos);	
}

// callback could be at interrupt time
pascal void ReadCallback(ParamBlockRec* pb)
{
    OSErr theErr = (*pb).ioParam.ioResult;    	 
    gReadCount = pb->ioParam.ioActCount;
    gReadDone = TRUE;
}

/* 	Read reads up to count bytes of data into buf.
	returns the number of bytes read, EOF, or -1 if the read failed */
ULONG32 CMacFile::Read (char *buf, ULONG32 count)
{
	OSErr theErr = noErr;

	if (m_pseudoFileHandle)
	{
		INT32	actualCount;
		
		theErr = Read(buf, count, &actualCount);
		
		count = actualCount;
	}
	else
	{
		if(mBufferedRead)
		{
			count = Copy(buf,count);
			if(count == 0)
				theErr = eofErr;
		}
		else
		{
	   	    // can't do synchronous read at interrupt time
	   	    if (IsRunningNativeOnMacOSX() || IsMacInCooperativeThread())
	   	    {
			theErr = ::FSRead(mRefNum, (long *)&count, buf);
		    }
		    else
		    {	// async i/o - callback could be at interrupt time
	#if 1
		    	ParamBlockRec pb;
			
		    	gReadCount = 0;
	    		gReadDone = FALSE;
			pb.ioParam.ioRefNum = mRefNum;
			pb.ioParam.ioBuffer = buf;
			pb.ioParam.ioReqCount = count;
			pb.ioParam.ioPosMode=fsAtMark;
			pb.ioParam.ioCompletion = gReadCallbackUPP;
			theErr = PBReadAsync(&pb);
			
			UINT32 timeout = TickCount() + 60L;
			while (!gReadDone && timeout-TickCount() > 0)
			{
			}
				
			count = gReadCount;
	#endif
		    }
		}
		if (theErr == eofErr && count > 0)
		{
			theErr = noErr;
		}
	}
	mLastError = theErr;
		
	return(mLastError ? 0 : count); //smplfsys::read assumes we return 0 for eof
}

/* 	Read reads up to count bytes of data into buf.
	returns the number of bytes read, EOF, or -1 if the read failed */
INT16 CMacFile::Read (char *buf, INT32 count, INT32 *actualCount)
{
OSErr theErr = noErr;

	long readCount = (long) count;
	
	if (m_pseudoFileHandle)
	{
		// read from our pseudo-file
		SInt32 remainingBytes;
		
		remainingBytes = m_pseudoFileSize - m_pseudoFileOffset;
		if (remainingBytes <= 0)
		{
			// we've already exhausted the buffer
			theErr = eofErr;
		}
		else
		{
			// some bytes remain to be "read" so read them directly
			// from the resource into the caller's buffer
			
			if (remainingBytes < count)
			{
				count = remainingBytes;
			}
			
			ReadPartialResource(m_pseudoFileHandle, m_pseudoFileOffset, buf, count);
			theErr = ResError();
			
			HX_ASSERT(theErr == noErr);
			// while we don't expect any errors, -188 (resourceInMemory) isn't fatal
			
			if (theErr == noErr || theErr == resourceInMemory)
			{
				// update our pseudo-file pointer
				
				readCount = count;
				
				m_pseudoFileOffset += count;
				
				theErr = noErr;
			}
		}
				
	}
	else
	{
		if(mBufferedRead)
		{
			readCount = Copy(buf,readCount);
			if(readCount == 0)
				theErr = eofErr;
		}
		else
		{
	   	    if (IsRunningNativeOnMacOSX() || IsMacInCooperativeThread())
			theErr = ::FSRead(mRefNum, &readCount, buf);
		    else
		    {	// async i/o - callback could be at interrupt time
	#if 0
		    	ParamBlockRec pb;
	//		IOCompletionUPP proc = NewIOCompletionProc(ReadCallback);
			
		    	gReadCount = 0;
	    		gReadDone = FALSE;
			pb.ioParam.ioRefNum = mRefNum;
			pb.ioParam.ioBuffer = buf;
			pb.ioParam.ioReqCount = count;
			pb.ioParam.ioPosMode=fsAtMark;
			pb.ioParam.ioCompletion = m_CompletionProc;
			theErr = PBReadAsync(&pb);
			
			EventRecord  	theEvent;
			long		sleepTime=10;

			long timeout = 0;
			// timeout, in case file read can't complete
			while (!gReadDone && timeout < 100)
			{
				::WaitNextEvent(everyEvent, &theEvent, sleepTime, nil);
				timeout++;
			}
			count = gReadCount;
	#endif
		    }
		}
	}
	
	if(theErr == eofErr && readCount > 0)
		theErr = noErr;

	if (actualCount)
	{
		*actualCount = (INT32) readCount;
	}
	
	return(theErr);
}

/* 	Write writes up to count bytes of data from buf.
	returns the number of bytes written, or -1 if the write failed */
ULONG32 CMacFile::Write(const char *buf, ULONG32 count)
{
	HX_ASSERT(!m_pseudoFileHandle);
	
    OSErr theErr = noErr;
    if (IsRunningNativeOnMacOSX() || IsMacInCooperativeThread())
    {
	if(mAppendMode) 
	{
		theErr = ::SetFPos(mRefNum,fsFromLEOF,0L);
	}
	
	if(!theErr)
	{
		if(mBufferedWrite)
		{
			mWriteFile->write_data((Ptr)buf,(long)count);
		}
		else
		{
		 	theErr = ::FSWrite(mRefNum, (long *)&count, buf);
			//check_noerr(theErr);	 a disk full error may occur
		}
	}
	mLastError = theErr;
//	return(theErr ? -1 : count);
    }
    else
    {
	if(mBufferedWrite)
	{
		mWriteFile->write_data((Ptr)buf,(long)count);
	}
	mLastError = theErr;
    }

	return ((theErr == noErr) ? count : -1);
}

/* 	Write writes up to count bytes of data from buf.
	returns the number of bytes written, or -1 if the write failed */
INT16 CMacFile::Write(const char *buf, INT32 count, INT32 *actualCount)
{
	HX_ASSERT(!m_pseudoFileHandle);

    OSErr theErr = noErr;
    *actualCount = 0;

    if (IsRunningNativeOnMacOSX() || IsMacInCooperativeThread())
    {
	long writeCount = 0;
	
	if(mAppendMode) 
		theErr = ::SetFPos(mRefNum, fsFromLEOF, 0L);
	
	if(!theErr) 
	{
		writeCount = count;
		theErr = ::FSWrite(mRefNum, &writeCount, buf);
	}
	
	*actualCount = (INT32) writeCount;
    }
    
	//check_noerr(theErr); a disk full error may occur
	
	// elsewhere the file code expects a negative actualCount when
	// the write failed
    if (theErr != noErr) *actualCount = -1;
    
    return(theErr);
}

void CMacFile::SetFileType(OSType creator, OSType type)
{
	sFileType = type;
	sCreator = creator;
}


long CMacFile::Copy(Ptr destBuf, long numBytes)
{
	long count;

	// check for buffered i/o
	if(mBufferedRead)
		count = mBufferFile->Copy(destBuf,numBytes);
	else
		count = 0;
		
	return(count);

}


HX_RESULT CMacFile::set_buffered_read	(char buffered)
{
	Boolean OK = TRUE;

	HX_ASSERT(!m_pseudoFileHandle);

	// If this file is setup for writing then we don't want to do any buffered reading.
	if (m_mode & O_WRONLY) return HXR_OK;
	if (m_mode & O_RDWR) return HXR_OK;

	mBufferedRead = 0;
	return HXR_OK; //HACK! - this needs to be redone!
	

	// set up buffered read object if necessary
	if(buffered && !mBufferedRead)	
	{
		if(mBufferFile == NULL) 
		{
			mBufferFile = new CBufferFile;
			OK = mBufferFile != NULL;
		}
			
		if(OK) 
			OK = mBufferFile->Specify(mRefNum);
			
			
	}
	else if(!buffered && mBufferedRead)
	{
		if(mBufferFile != NULL) 
		{
			delete mBufferFile;
			mBufferFile = NULL;
		}
	}

	mBufferedRead = buffered != 0;

	if(OK)
		Seek(0,SEEK_SET);
		
	return(OK ? HXR_OK : HXR_OUTOFMEMORY);
}

HX_RESULT CMacFile::set_buffered_write	(char buffered)
{
Boolean OK = TRUE;

	HX_ASSERT(!m_pseudoFileHandle);

	// set up buffered read object if necessary
	if(buffered && !mBufferedWrite)	
	{
		if(mWriteFile == NULL) 
		{
			mWriteFile = new CWriteFile;
			OK = mWriteFile != NULL;
		}
			
		if(OK) 
		{
			OK = mWriteFile->Specify(mRefNum,Tell());
		}
	}
	else if(!buffered && mBufferedWrite)
	{
		if(mWriteFile != NULL) 
		{
			delete mWriteFile;
			mWriteFile = NULL;
		}
	}

	mBufferedWrite = buffered != 0;
		
	return(OK ? HXR_OK : HXR_OUTOFMEMORY);
}


// This seems to exist solely so FileIO::status can get the size and an error value;
// Otherwise, it's redundant with GetSize

INT16 CMacFile::FileSize(long *size)
{
	if (m_pseudoFileSize)
	{
		*size = m_pseudoFileSize;
		return 0;
	}
	return ::GetEOF(mRefNum, size);
}

HXBOOL CMacFile::GetTemporaryFileName(const char *tag, char* name, UINT32 ulBufLen)
{
	// we'll make a temp name like tag_2345
	
	CHXString tempNameTemplate(tag);
	
	tempNameTemplate += "_%SUB%";
	
	CHXDirSpecifier tempDirSpec = CHXFileSpecUtils::GetSystemTempDirectory();
	
	CHXFileSpecifier tempFileSpec = CHXFileSpecUtils::GetUniqueTempFileSpec(
		tempDirSpec, (const char *) tempNameTemplate, "%SUB%");
	
	// copy the temp path to the supplied buffer, name
	
	name[0] = '\0';
	if (tempFileSpec.IsSet())
	{
		CHXString strPath = tempFileSpec.GetPathName();
		
		SafeStrCpy(name, (const char *) strPath, ulBufLen);
		
		return TRUE;
	}
	return FALSE;
	
}




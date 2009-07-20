/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cmacfile.cp,v 1.4 2005/03/14 19:36:28 bobclark Exp $
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

// include platform specific headers here
#include <stdio.h>
#include <fcntl.h>
#include <Folders.h>
#include <Script.h>

#include "CMacFile.h"
#include "hxtick.h"
#include "FullPathName.h"
#include "hxcom.h"
#include "hxbuffer.h"
#include "hxmm.h"
#include "hxfiles.h" /* for HX_FILE_WRITE */

#ifndef _CARBON
#include "FSpCompat.h"
#include "MoreFilesExtras.h"
#else
#include "MoreFilesX.h"
#endif

OSType CMacFile::sCreator	= 'PNst';
OSType CMacFile::sFileType 	= 'PNRA';

HXBOOL gReadDone = FALSE;
ULONG32 gReadCount = 0;



// async callback proc
pascal void ReadCallback(ParamBlockRec* pb);

// async callback UPP
#ifdef _CARBON
static IOCompletionUPP			gReadCallbackUPP=NewIOCompletionUPP(ReadCallback);
#else
static IOCompletionUPP			gReadCallbackUPP=NewIOCompletionProc(ReadCallback);
#endif


CHXDataFile*
CHXDataFile::Construct (UINT32 ulFlags)
{
    return (CHXDataFile *) new CMacFile;
}

// CHXFile should set the file reference to a value indicating the file is not open
CMacFile::CMacFile (void)
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
	/* if(mBufferFile)
	{ 
		delete mBufferFile;
		mBufferFile = NULL;
	}
	
	if(mWriteFile) 
	{
		delete mWriteFile;
		mWriteFile = NULL;
	} */
	
	Close();		// close file if necessary
}

// Create a file with the specified mode
// Close the previous file if it was open
HX_RESULT CMacFile::Create(const char *filename, UINT16 mode,HXBOOL textflag)
{
	Close();		// close previous file if necessary

	FSSpec		theSpec;
	
	
	OSErr	theErr=noErr;
	
	Close();	// close previous file if necessary

	theErr = FSSpecFromPathName(filename, &theSpec);

	if(!theErr)
	{
		Boolean targetIsFolder,wasAliased;
		theErr = ::ResolveAliasFile(&theSpec,true,&targetIsFolder,&wasAliased);
	}

	if(theErr==fnfErr)
		theErr = FSCreateDataFile(&theSpec,textflag ? 'ttxt' : sCreator ,textflag ? 'TEXT' : sFileType);
				
	if(!theErr)
		theErr = FSOpenFile(&theSpec, fsRdWrPerm, &mRefNum);
				
	if(!theErr)
		theErr = ::SetEOF(mRefNum, 0L);

	if(!theErr)
		mFile = theSpec;

	mLastError = theErr;
	
	return(theErr ? -1 : 0);	
}

// Open a file with the specified permissions
// Close the previous file if it was open
//
// If the file isn't found, look for a 'RLFL' resource *in the current
// resource chain* with the same name as the file.

HX_RESULT CMacFile::Open(const char *filename, UINT16 mode,HXBOOL textflag)
{
	OSErr	theErr 	= noErr;
	short 	perm	= fsCurPerm;
	UCHAR   length 	= ::strlen(filename);
	FSSpec	theSpec;
	
	if(!theErr)
	{	
	
		Close();	// close previous file if necessary
	
		theErr = FSSpecFromPathName(filename,&theSpec);
		if (theErr)  //try adding ':' (partial path)
		{
		    UCHAR* partial = new UCHAR[length + 2];
		    partial[0] = length+1;
		    partial[1] = ':';
		    ::BlockMoveData(filename,&partial[2],length);
		    theErr = FSMakeFSSpec(0,0,partial,&theSpec);
		    if (theErr == dirNFErr)
		    	theErr = fnfErr;
		    delete [] partial;
		
		}

		if(!theErr)
		{
			Boolean targetIsFolder,wasAliased;
			theErr = ::ResolveAliasFile(&theSpec,true,&targetIsFolder,&wasAliased);
		}
	}
	
	
	if(!theErr)
	{
		// figure out mac file permission
		perm = fsRdWrPerm;
		if (mode & O_WRONLY)	
			perm = fsWrPerm;
		else if (mode & O_RDONLY)
			perm = fsRdPerm;
		
		// Store the permissions for this file for later.
		m_mode=mode;
	
		theErr = FSOpenFile(&theSpec, perm, &mRefNum);
	}
	
	if(theErr != noErr)
	{
		if(theErr == fnfErr)
		{
			theErr = FSSpecFromPathName(filename,&theSpec);
			if ((mode & O_CREAT) || (mode & O_WRONLY)) //always create if Write mode
				theErr = FSCreateDataFile(&theSpec,textflag ? 'ttxt' : sCreator ,textflag ? 'TEXT' : sFileType);
				
			if(!theErr)
				theErr = FSOpenFile(&theSpec, perm, &mRefNum);
		}
	}
	
	if ((theErr) && (mode & O_RDONLY))
	{
		Handle	resHandle;		
		SInt32	resSize;
		
		// We couldn't open the file, and the request was read-only
		//
		// See if there's a pseudo-file resource we can use
		//
		// We need a handle to the resource so we can read from it, but
		// we don't want the whole resource loaded into memory, so we
		// set ResLoad to false.
		
		SetResLoad(false);
		resHandle = GetNamedResource(kRealFileResource, theSpec.name); // 'RLFL'
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
				
				theErr = noErr;
				mRefNum = -1;	// signals that we're using a pseudo-file and no actual file is open
			}
		}
	}
	
	if (!m_pseudoFileHandle)
	{
		if(!theErr && (mode & O_CREAT))
			theErr = ::SetEOF(mRefNum, 0L);

		if(!theErr)
		{
			mAppendMode = (mode & O_APPEND);
			mFile = theSpec;
			if(mode & O_TRUNC) theErr = ::SetEOF(mRefNum, 0L);
		}

		
		if(theErr && mRefNum != 0) Close();
	}
	
	mLastError = theErr;
	
	return(theErr ? HXR_DOC_MISSING : 0);	
		
}

// Close the previous file if it was open
HX_RESULT CMacFile::Close(void)
{
OSErr theErr = noErr;

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
		theErr = noErr;
		mRefNum = 0;
	}
	
	else if (mRefNum) 
	{
		if(mBufferFile)
		{ 
			delete mBufferFile;
			mBufferFile = NULL;
		}
		
		if(mWriteFile) 
		{
			delete mWriteFile;
			mWriteFile = NULL;
		}

		// close a real file
		theErr = ::FSClose(mRefNum);
		mRefNum = 0;
		if(!theErr) theErr =::FlushVol(nil, mFile.vRefNum);
	}
	
	mLastError = theErr;
	return(theErr ? -1 : 0);	
}

HX_RESULT	CMacFile::Delete	(const char *filename)
{
    OSErr theErr = noErr;
    FSSpec theSpec;

	HX_ASSERT (!m_pseudoFileHandle);

	theErr = FSSpecFromPathName(filename,&theSpec);
	if (noErr == theErr) 
	{
		theErr = ::FSpDelete(&theSpec);
	}
	
	mLastError = theErr;
	return(theErr ? -1 : 0);	
}

/*	Returns the size of the file in bytes. */
ULONG32		CMacFile::GetSize(void)
{
	ULONG32 size=0;
	
	if (m_pseudoFileHandle)
	{
		size = m_pseudoFileSize;
	}
	
	else if (mRefNum) 
	{
    	::GetEOF(mRefNum,(long*)&size);
    }
    return size;
}

// Rewinds the file position to the start of the file
HX_RESULT CMacFile::Rewind(void)
{
	OSErr theErr;
	if (m_pseudoFileHandle)
	{
		m_pseudoFileOffset = 0;
		theErr = noErr;
	}
	else
	{
		theErr = ::SetFPos(mRefNum,fsFromStart,0L);
	}
	mLastError = theErr;
	return(theErr ? HXR_INVALID_FILE : 0);
}
	

// Seek moves the current file position to the offset from the fromWhere specifier
HX_RESULT CMacFile::Seek(ULONG32 offset, UINT16 fromWhere)
{
OSErr theErr = noErr;
		
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
		theErr = HXR_OK;
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
		
		theErr =  ::SetFPos(mRefNum,fromWhere,offset);

		// returning eofErr was causing problems for ChunkyRes during http play
		if (theErr == eofErr)
		{
			theErr = ::SetEOF(mRefNum,offset);
			theErr = ::SetFPos(mRefNum,fromWhere,offset);
			theErr = HXR_OK;
		}
			
		long pos;
		::GetFPos(mRefNum,(long *)&pos);

		if(!theErr && mBufferedRead)
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
	mLastError = theErr;
	return(theErr ? HXR_INVALID_FILE : 0);	
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
	   	    if (!HXMM_ATINTERRUPT())
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
		if(theErr == eofErr && count > 0)
			theErr = noErr;
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
	   	    if (!HXMM_ATINTERRUPT())
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
    if (!HXMM_ATINTERRUPT())
    {
	if(mAppendMode) 
		theErr = ::SetFPos(mRefNum,fsFromLEOF,0L);
	
	if(!theErr)
	{
		if(mBufferedWrite)
			mWriteFile->write_data((Ptr)buf,(long)count);
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
		mWriteFile->write_data((Ptr)buf,(long)count);
	mLastError = theErr;
    }
//    return count;
	return (theErr == noErr? count : -1);
}

/* 	Write writes up to count bytes of data from buf.
	returns the number of bytes written, or -1 if the write failed */
INT16 CMacFile::Write(const char *buf, INT32 count, INT32 *actualCount)
{
	HX_ASSERT(!m_pseudoFileHandle);

    OSErr theErr = noErr;
    *actualCount = 0;

    if (!HXMM_ATINTERRUPT())
    {
	long writeCount = 0;
	
	if(mAppendMode) 
		theErr = ::SetFPos(mRefNum,fsFromLEOF,0L);
	
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

OSErr CMacFile::FSOpenFile(FSSpec *theSpec, short perm, short *fRefNum) 
{
OSErr		theErr = noErr;
Boolean 	targetIsFolder,wasAliased;

	*fRefNum = 0;
	theErr = ResolveAliasFile(theSpec,true,&targetIsFolder,&wasAliased);
	
	if(!theErr) theErr = ::FSpOpenDF(theSpec,perm,fRefNum);
	
	if(theErr) *fRefNum = 0;
	
	return(theErr);
}

OSErr CMacFile::FSCreateDataFile(FSSpec *sfFile, OSType creator, OSType type)
{
OSErr theErr = noErr;

	theErr =  ::FSpCreate(sfFile,creator,type,smSystemScript);
	
	if(theErr)
	{
		if(theErr == dupFNErr)
			theErr = FSSetFInfo(sfFile, creator, type);
	}
	
	return(theErr);
}

OSErr CMacFile::FSSetFInfo(FSSpec *theSpec, OSType creator, OSType type)
{
OSErr theErr = noErr;

#ifndef _CARBON
	theErr = FSpChangeCreatorType(theSpec, creator, type);
#else
	FSRef ref;
	
	theErr = FSpMakeFSRef(theSpec, &ref);
	if (theErr == noErr)
	{
		theErr = FSChangeCreatorType(&ref, creator, type);
	}
#endif
	return(theErr);
}

void CMacFile::SetFileType(OSType creator, OSType type)
{
	sFileType = type;
	sCreator = creator;
}


OSErr CMacFile::FSSetFilePos(short fRefNum, short fromWhere, long offset)
{
	HX_ASSERT(fRefNum != -1);	// we can't do this on a pseudo file (since this method's static)
	/*
	if (m_pseudoFileHandle)
	{
		OSErr	err;
		
		switch (fromWhere)
		{
			case fsAtMark:
				err = noErr;
				break;
				
			case fsFromMark:
				offset += m_pseudoFileOffset;
				// fall through...
				
			case fsFromStart:
				if (offset < 0) err = posErr;
				else if (offset < m_pseudoFileSize) 
				{
					m_pseudoFileOffset = offset;
					err = noErr;
				}
				else 
				{
					// offset is >= m_pseudoFileSize
					err = eofErr;
					m_pseudoFileOffset = m_pseudoFileSize;
				}
				break;
				
			case fsFromLEOF:
				if (offset > 0) err = posErr;
				else if (m_pseudoFileSize - (offset + 1) >= 0) 
				{
					m_pseudoFileOffset = m_pseudoFileSize - (offset + 1);
					err = noErr;
				}
				else 
				{
					// offset is >= -m_pseudoFileSize
					err = posErr;
					m_pseudoFileOffset = 0;
				}
				break;
		}
		
		return err;
	}
	*/
	
	return ::SetFPos(fRefNum,fromWhere,offset);
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


INT16 CMacFile::FileSize(long *size)
{
	return ::GetEOF(mRefNum,size);
}

static ULONG32 sPrevSuffix = 0;

HXBOOL CMacFile::GetTemporaryFileName(const char *tag, char* name)
{
	short		tempVRefNum;
	long		tempDirID;
	Str255	tempFileName;
	FSSpec	tempSpec;
	OSErr 	theErr = noErr;
	
	// build the temp file name in the form tag_sPrevSuffix
	// changed from tag.sPrevSuffix because many other portions
	// of code STRIP the suffix, and add their own
	// thus yielding a very NON unique file name <rlovejoy>
	
	::sprintf((char *)tempFileName,"%s_%ld",tag,sPrevSuffix++);
	
	// copy the temo file name into the caller's name field in case we
	// can't find the Temporary Folder
	::strcpy(name,(char *)tempFileName);

	// Try to get the FSSpec of the System Folder's Temporary Folder
	
	// starting with System 8, the preferred temporary folder is the Chewable Items
	// folder (aka Cleanup at Startup)		GR 12/8/98
	
	theErr = ::FindFolder(kOnSystemDisk,kChewableItemsFolderType,kCreateFolder,&tempVRefNum,&tempDirID);
	if (theErr != noErr)
		theErr = ::FindFolder(kOnSystemDisk,kTemporaryFolderType,kCreateFolder,&tempVRefNum,&tempDirID);

	if(!theErr)
	{
		// Build an FSSpec for the temp file in the Temporary Folder
#ifdef _CARBON
		c2pstrcpy((StringPtr)tempFileName, (char*)tempFileName);
#else
		::c2pstr((char *)tempFileName);
#endif
		theErr = ::FSMakeFSSpec(tempVRefNum,tempDirID,tempFileName,&tempSpec);
	}
	
	// Note: it is okay for FSMakeFSSpec to return fnfErr since the caller
	// is going to create the file.
	
	// Now create the full path name to the temp file
	if(!theErr || theErr == fnfErr)
	{
		theErr = noErr;
	
		CHXString theString;
		theErr = PathNameFromFSSpec(&tempSpec, theString);
		
		if(!theErr)
			::strcpy(name,theString);
	}
	
	return TRUE;
}




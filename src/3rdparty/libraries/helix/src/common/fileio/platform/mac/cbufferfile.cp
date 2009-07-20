/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cbufferfile.cp,v 1.4 2007/07/06 20:35:13 jfinnecy Exp $
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

#include "CBufferFile.h"
#include "hxtypes.h"


CBufferFile::CBufferFile (void) 

	: mLow(NULL)
	, mBufSize(0)
	, mSize(0)
	, mCurPos(0)
	, mLoaded(FALSE)
	, mBytesTotal(0)

	{ /* begin CBufferFile */


	} /* end CBufferFile */


CBufferFile::~CBufferFile ()

	{ /* begin ~CBufferFile */
		if(mLow) delete [] mLow;
		
		mLow = NULL;

	} /* end ~CBufferFile */

Boolean CBufferFile::Specify(short fRefNum, long bufSize)
{
Boolean OK = TRUE;

	if(mLow)
	{
		delete [] mLow;
		mLow = NULL;
	}
	
	mLow = new char[bufSize];
	OK = mLow != NULL;
	
	
	if(OK)
	{
		mBufSize = mSize = bufSize;
		
		mHigh = mLow + (mSize >> 1);

		a_PB.ioCompletion = NULL;
		a_PB.ioVersNum = 0;
		a_PB.ioPermssn = fsRdPerm;
		a_PB.ioPosMode = fsFromStart;
		a_PB.ioMisc = 0;
		a_PB.ioRefNum = fRefNum;
		b_PB = a_PB;
	}
	
	return(OK);
}

long CBufferFile::Copy(Ptr destBuf, long numBytes)
{
long count;

	numBytes =  numBytes <= mBytesToCopy ? numBytes : mBytesToCopy;
	if(mCurPos + numBytes < mBufSize)
	{
		count = numBytes;
		BlockMoveData(&mLow[mCurPos], destBuf, numBytes);
		mCurPos += numBytes;
		if(mCurPos > mBufSize >> 1 && fFillLow)
		{
			Load(kBufLow);
			fFillLow = FALSE;
		}
	}
	else
	{
		count = mBufSize - mCurPos;
		BlockMoveData(&mLow[mCurPos], destBuf, count);
		BlockMoveData(mLow, &destBuf[count], numBytes - count);
		mCurPos = numBytes - count;
		if(mCurPos < mBufSize >> 1 && !fFillLow)
		{
			Load(kBufHigh);
			fFillLow = TRUE;
		}
	}
	
	mBytesToCopy -= numBytes;

	return(numBytes);
}



long CBufferFile::Load(short loadFlag)
{
long		count;
Ptr		 	buf;
IOParam		*thePB;

	switch(loadFlag)
	{
		case kBufLow:
			count = mBufSize >> 1;
			buf = mLow;
			thePB = &a_PB;
			break;
			
		case kBufHigh:
			count = mBufSize >> 1;
			buf = mHigh;
			thePB = &b_PB;
			break;
			
		case kBoth:
			count = mBufSize;
			buf = mLow;
			thePB = &a_PB;
			break;
			
		default:
			count = 0;
			break;
	}

	count = count <= mBytesRemaining ? count : mBytesRemaining;
	
	if(count > 0)
	{
		thePB->ioReqCount = count;
		thePB->ioPosOffset = ioPosOffset;
		thePB->ioBuffer = buf;
		thePB->ioVersNum = 0;
		thePB->ioMisc = 0;
		
		PBReadSync((ParmBlkPtr)thePB);
		
		mBytesRemaining -= count;
		ioPosOffset += count;
	}

	return(count);
}


Boolean CBufferFile::PreLoad(long startPos, long bytesReq)
{
long		count;
Boolean		OK;
OSErr		theErr;
	
	count = (bytesReq <= mSize) ? bytesReq : mSize;
	mBufSize = count;
	mHigh = mLow + (count >> 1);

	
	mCurPos = 0;
	mStartPos = startPos;
	mBytesTotal = bytesReq;
	mBytesRemaining = bytesReq;
	mBytesToCopy = bytesReq;
	ioPosOffset = startPos;

	a_PB.ioMisc = 0;
	a_PB.ioVersNum = 0;
	a_PB.ioResult = 0;
	a_PB.ioReqCount = count;
	a_PB.ioPosOffset = startPos;
	a_PB.ioBuffer = mLow;
	b_PB = a_PB;
	
	theErr = PBReadSync((ParmBlkPtr)&a_PB);
	
	OK = theErr == noErr;
	if(OK)
	{
		mBytesRemaining -= count;
		ioPosOffset += count;
		fFillLow = TRUE;
	}
	else if(theErr == eofErr)
	{
		mBytesRemaining = 0;
		ioPosOffset += count;
		fFillLow = TRUE;
		OK = TRUE;
	}
	
	mLoaded = OK;	
	return(OK);

}

void CBufferFile::CheckBuffer(void)
{
	if(mCurPos > mBufSize >> 1 && fFillLow)
	{
		Load(kBufLow);
		fFillLow = FALSE;
	}
	else if(mCurPos < mBufSize >> 1 && !fFillLow)
	{
		Load(kBufHigh);
		fFillLow = TRUE;
	}
}




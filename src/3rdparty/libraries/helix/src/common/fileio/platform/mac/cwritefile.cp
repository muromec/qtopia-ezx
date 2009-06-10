/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cwritefile.cp,v 1.4 2007/07/06 20:35:13 jfinnecy Exp $
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

#include "CWriteFile.h"
#include "hxassert.h"

CWriteFile::CWriteFile (void) 

	: mLow(NULL)
	, mBufSize(0)
	, mWrittenSize(0)
	, mCurPos(0)
	, fWriteLow (1)
	, ioPosOffset (0)
	
	{ /* begin CWriteFile */


	} /* end CWriteFile */


CWriteFile::~CWriteFile ()

	{ /* begin ~CWriteFile */
	
		// wait for all writes to complete
		while(a_PB.ioResult > 1 || b_PB.ioResult > 1)
		{
		
		}

		// flush remaining data from buffer
		if(mWrittenSize > 0)
		{
			long count;
			
			::SetFPos(a_PB.ioRefNum,fsFromStart,ioPosOffset);

			count = mWrittenSize;
			if(mWrittenSize >= mBufSize >> 1)
			{
				count -= (mBufSize >> 1);
				::FSWrite(a_PB.ioRefNum,&count,mHigh);
			}
			else
			{
				::FSWrite(a_PB.ioRefNum,&count,mLow);
			}
			
			//Update file position for fRefNum
			::SetFPos(a_PB.ioRefNum,fsFromStart,ioPosOffset + count);
		}

		if(mLow) 
			delete [] mLow;
		
		mLow = NULL;

	} /* end ~CWriteFile */

Boolean CWriteFile::Specify(short fRefNum, long offset,long bufSize)
{
Boolean OK = TRUE;

	if(mLow)
	{
		delete [] mLow;
		mLow = NULL;
	}
	
	mLow = new char[bufSize];
	OK = mLow != NULL;
	
	fWriteLow = TRUE;
	ioPosOffset = offset;
	
	if(OK)
	{
		mBufSize = bufSize;
		mWrittenSize = 0;
		mCurPos = 0;
		
		mHigh = mLow + (mBufSize >> 1);

		a_PB.ioCompletion = NULL;
		a_PB.ioVersNum = 0;
		a_PB.ioPermssn = fsWrPerm;
		a_PB.ioPosMode = fsFromStart;
		a_PB.ioMisc = 0;
		a_PB.ioRefNum = fRefNum;
		a_PB.ioResult = 0;	

		b_PB = a_PB;
	}
	
	return(OK);
}

void CWriteFile::Seek(long pos)
{
	HX_ASSERT( pos <= mWrittenSize );
	mCurPos = pos;
}

void CWriteFile::write_data(Ptr buf, long numBytes)
{
long count;

	if(mCurPos + numBytes < mBufSize)
	{
		count = numBytes;
		::BlockMoveData(buf, &mLow[mCurPos], numBytes);
		mCurPos += numBytes;
		if(mCurPos > mBufSize >> 1 && fWriteLow)
		{
			Write(kWriteBufLow);
			fWriteLow = FALSE;
		}
		if(mCurPos > mWrittenSize )
		{
			mWrittenSize = mCurPos;
		}
	}
	else
	{
		count = mBufSize - mCurPos;
		::BlockMoveData(buf,&mLow[mCurPos], count);
		::BlockMoveData(&buf[count], mLow, numBytes - count);
		mCurPos = numBytes - count;
		if(mCurPos < mBufSize >> 1 && !fWriteLow)
		{
			Write(kWriteBufHigh);
			fWriteLow = TRUE;
		}
	}
	
}



void CWriteFile::Write(short writeFlag)
{
long		count;
Ptr		 	buf;
IOParam		*thePB;

	switch(writeFlag)
	{
		case kWriteBufLow:
			count = mBufSize >> 1;
			buf = mLow;
			thePB = &a_PB;
			break;
			
		case kWriteBufHigh:
			count = mBufSize >> 1;
			buf = mHigh;
			thePB = &b_PB;
			break;
			
		case kWriteBoth:
			count = mBufSize;
			buf = mLow;
			thePB = &a_PB;
			break;
			
		default:
			count = 0;
			break;
	}

	if(count > 0)
	{
		thePB->ioReqCount = count;
		thePB->ioPosOffset = ioPosOffset;
		thePB->ioBuffer = buf;
		thePB->ioVersNum = 0;
		thePB->ioMisc = 0;
		::PBWriteAsync((ParmBlkPtr)thePB);
		ioPosOffset += count;
	}
}






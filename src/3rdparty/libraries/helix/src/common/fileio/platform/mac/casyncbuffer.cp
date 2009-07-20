/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: casyncbuffer.cp,v 1.4 2007/07/06 20:35:13 jfinnecy Exp $
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

#include "CAsyncBuffer.h"

#include <string.h>

const long kBlankLong[] = {'FNRD','fnrd'};

CAsyncBuffer::CAsyncBuffer (

	Size		inSize,
	QElemPtr	inpb/*= nil*/)
	
	: mBuffer (nil)
	, mSize (0)
	, mLim (0)
	, mMark (0)
	, mBusy (FALSE)
	
	, mClearIndex (0)
	
	{ /* begin CAsyncBuffer */
		
		::memset (&mPBQueue, 0, sizeof (mPBQueue));
		PBFree (inpb);
		
//		if (nil != (mBuffer = (Ptr) new char [inSize])) 
		if (nil != (mBuffer = (Ptr) ::NewPtr(inSize)))
			mSize = inSize;

	} /* end CAsyncBuffer */
	
CAsyncBuffer::~CAsyncBuffer (void)

	{ /* begin ~CAsyncBuffer */
	
		if (mBuffer) 
			::DisposePtr(mBuffer);
//			delete [] mBuffer;
		mBuffer = nil;
		
		mSize = 
		mLim = 
		mMark = 0;
			
	} /* end ~CAsyncBuffer */
	
long CAsyncBuffer::Read (

	void	*rcvBuff,
	long	rcvBuffLen)
		
	{ /* begin Read */
		
		long	remaining = Remaining ();
		
		if (rcvBuffLen > remaining) rcvBuffLen = remaining;
		
		if (rcvBuffLen) {
			if (rcvBuff) ::BlockMoveData (mBuffer + mMark, (Ptr) rcvBuff, rcvBuffLen);

			mMark += rcvBuffLen;
			} /* if */
		
		return rcvBuffLen;
		
	} /* end Read */
	
long CAsyncBuffer::Write (

	const	void	*sndBuff,
	long			sndBuffLen)
		
	{ /* begin Write */
		
		long	remaining = mSize - mMark;
		
		if (sndBuffLen > remaining) sndBuffLen = remaining;
		
		if (sndBuffLen) {
			if (sndBuff) ::BlockMoveData ((Ptr) sndBuff, mBuffer + mMark, sndBuffLen);

			mMark += sndBuffLen;
			if (mLim < mMark) mLim = mMark;
			} /* if */
		
		return sndBuffLen;
		
	} /* end Write */

void CAsyncBuffer::ClearBuffer (void)

	{ /* begin ClearBuffer */
	
		long *p = (long *) mBuffer;
		for (long i = (mLim / sizeof (*p)); i-- > 0; )
			p[i] = kBlankLong[mClearIndex];
		
	} /* end ClearBuffer */
	
void CAsyncBuffer::DebugBuffer (void)

	{ /* begin DebugBuffer */
		
		for (short b = 0; b < (sizeof (kBlankLong) / sizeof (kBlankLong[0])); b++) {
			long	blank = 0;
			long	*p = (long *) mBuffer;
			
			for (long i = 0; i < (mLim / sizeof (*p)); i++)
				if (kBlankLong[b] == p [i]) blank++;
			
			if (blank > 10) DebugStr ((mClearIndex == b) ? "\pNew" : "\pStale");
			} /* for */
			
	} /* end DebugBuffer */

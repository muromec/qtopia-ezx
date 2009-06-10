/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: casyncbuffer.h,v 1.4 2007/07/06 20:35:13 jfinnecy Exp $
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

#pragma once

class CAsyncBuffer {

	private:
	
		Ptr		mBuffer;
		Size	mSize;
		
		long	mLim;
		long	mMark;
		
		QHdr	mPBQueue;
		short	mBusy;
		
		short	mClearIndex;
		
	public:
	
						CAsyncBuffer	(Size		inSize,
										 QElemPtr	inpb = nil);
		virtual			~CAsyncBuffer	(void);
		
		inline	OSErr	PBAlloc			(QElemPtr	inpb)
										{return ::Dequeue (inpb, &mPBQueue);};
		inline	void	PBFree			(QElemPtr	inpb)
										{if (inpb) ::Enqueue (inpb, &mPBQueue);};
										
		inline	short	Busy			(void) const
										{return mBusy;};
		inline	void	SetBusy			(void)
										{mBusy++;};
		inline	void	ClearBusy		(void)
										{mBusy--;};
		
		inline	Size	BufferSize		(void) const
										{return mSize;};
		inline	Ptr		Buffer			(void) const
										{return mBuffer;};
		inline	long	Lim				(void) const
										{return mLim;};
		
		inline	long	Available		(void) const
										{return mSize - mMark;};
		inline	long	Remaining		(void) const
										{return mLim - mMark;};
				
		inline	void	Load			(long			inLim)
										{mLim = inLim; mMark = 0;};
				long	Read			(void			*inBuffer,
										 long			inBufferLen);
				long	Write			(const	void	*inBuffer,
										 long			inBufferLen);
										 
				void	NextClearIndex	(void) {mClearIndex = 1 - mClearIndex;};
				void	ClearBuffer		(void);
				void	DebugBuffer		(void);
	};
	




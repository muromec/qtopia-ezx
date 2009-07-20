/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxsringbuffer.h,v 1.4 2007/07/06 20:35:02 jfinnecy Exp $
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

/*
 * Class: HXSimpleRingBuffer
 * Description: Ring buffer utility. Used for transmitting data between asynchronous processes.
 * 	A FIFO structure, on a fixed sized buffer.
 *
 * Examples:
 *
 * Creating the buffer
 *
    // If the source is seekable, buffer size can be smaller
    UINT32 ulBufSize=1024*256;
    UINT32 ulReserveSize;
    if (m_dSeekType == RANDOM_ACCESSIBLE)
    {
	ulReserveSize = 1024*4;
    }
    else
    {
	ulReserveSize = 1024*32; // make it big enough so that we don't have to seek (sufficient for mp3 header data, the largest)
    }
    m_pRingBuffer = new HXSimpleRingBuffer(ulBufSize, ulReserveSize);

 *
 * Filling the buffer:
 * 
	CRingBufferTest::FortifyRingBuffer(FILE* fpsource, HXSimpleRingBuffer* pRingBuffer)
	{
	    HXBOOL bDone=FALSE;
	    INT32  dNumTries=0;

	    while (!bDone)
	    {
		pRingBuffer->Lock(); 

		// Get the state of the buffer
		int rbState = pRingBuffer->getState();

		// Check the ring buffer state. If the buffer has been reset, we need to seek to the desired offset.
		if (rbState == HX_RING_BUFFER_RESET) {
			if (fseek(fpsource, pRingBuffer->GetOffset(), SEEK_SET) == 0) {
				bDone = TRUE;
				fprintf(stderr,"(ERROR) Could not seek.");
			} else {
				pRingBuffer->Set();
			}
		} else if (pRingBuffer->IsRingBufferFull(len)) {
			bSleep = TRUE;
		} else {
			// Read data when :
			//  	- ring buffer is not full
			//	- ring buffer is not in the 'reset' mode
			BYTE   buf[MAX_READSIZE];
			INT32 dBytesRead = fread(buf, 1, MAX_READSIZE, fpsource);
			if (dBytesRead > 0) {
				// Store data in the ring buffer
				pRingBuffer->Put(buf, dBytesRead);
			} else {
				// end of file ?
				if (dBytesRead == -1 || dNumTries++ >= MAX_TRIES) {
					fprintf(stderr, "DataSourceHelper: Reached end of file.");
					bDone = TRUE; 
				} else {
					bSleep = TRUE;
				}
			}
		} else {
			bSleep = TRUE;
		}

		pRingBuffer->Unlock();

		if (bSleep) {
			microsleep(20);
			bSleep = false;
		}
	    }
	}

 *
 * Reading the buffer:
 *
	 // Read reads up to count bytes of data into buf.
	 //returns the number of bytes read, EOF, or -1 if the read failed 
	 //
	STDMETHODIMP_(ULONG32)
	_DataSourceAdapter::Read(REF(IHXBuffer*) pOutBuf, ULONG32 count)
	{
	    ULONG32 ncnt = 0;           // number of bytes read
	    IHXBuffer* pBuf = NULL;
	    INT32 lCount = count;

	    CreateBufferCCF(pOutBuf, m_pEngine);
	    pOutBuf->SetSize(count);
	    if (m_pRingBuffer->Get(pOutBuf->GetBuffer(), lCount) == HXR_OK)
	    {
		ncnt = count;
	    }
	    else
	    {
		ncnt = m_lReadCode;
		pOutBuf->SetSize(0);
	    }

	    return ncnt;
	}
 */
#ifndef __HXSRINGBUFFER_H_
#define __HXSRINGBUFFER_H_

#include "hxtypes.h"

// empty->set   : after first read
// set->reset   : after seek request to location not in the buffer
// reset->empty : after successful seek

const int HX_RING_BUFFER_UNINIT=0; // ring buffer is empty, and ready for data
const int HX_RING_BUFFER_SET=1;	// valid data in the ring buffer
const int HX_RING_BUFFER_RESET=2; // contains a new starting offset, and ring buffer is now empty
class HXMutex;

class HXSimpleRingBuffer
{
public:
    HXSimpleRingBuffer(INT32 lBufSize, INT32 lReserveSize=0);
    ~HXSimpleRingBuffer();

    void Reset() {
	// Ring buffer data is invalid, so invalidate data.
    	m_lNextGet = 0;
    	m_lNextPut = 0;
    	m_lReserveGet = 0;
	m_lOffset = 0;
	m_state = HX_RING_BUFFER_RESET;
    }

    void Set() {
	// Source is done resetting, ring buffer is now ready for data
	if (m_state == HX_RING_BUFFER_RESET) {
	    m_state = HX_RING_BUFFER_UNINIT;
	}
    }

    UINT16 GetState() {
	    return m_state;
    }

    HX_RESULT Get(BYTE* buf, INT32& lLen, HXBOOL bWithReserved=FALSE);
    HX_RESULT Put(const BYTE* buf, INT32 lLen, HXBOOL bWithReserved=FALSE);

    HX_RESULT Seek(INT32 lOffset);

    HXBOOL IsFull(INT32 lLen=0, HXBOOL bWithReserved=FALSE);
    HXBOOL IsEmpty(INT32 lLen=0, HXBOOL bWithReserved=FALSE);

    INT32 AvailablePutBytes(HXBOOL bWithReserved=FALSE);
    INT32 AvailableGetBytes(HXBOOL bWithReserved=FALSE);
    INT32 GetOffset() { return m_lOffset; }

    void Lock();
    void Unlock();

private:
    void setState(UINT16 state) {
	    m_state = state;
    }

    BYTE* m_pRingBuffer;
    INT32 m_lBufSize;	// size of the ring buffer
    INT32 m_lReservedBytes;	// number of bytes to reserve in the ring, for seeking
    INT32 m_lNextGet;	// Index to Next position to get bytes (start)
    INT32 m_lNextPut;		// Index to Next position to put bytes (end)
    INT32 m_lReserveGet;	// position behind the next get to reserve for seeking, start position of valid data
    UINT16 m_state;
    INT32 m_lOffset;
    HXMutex* m_pMutex;
};

#endif // __HXSRINGBUFFER_H_

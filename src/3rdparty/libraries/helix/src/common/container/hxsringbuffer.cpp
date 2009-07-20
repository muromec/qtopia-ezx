/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxsringbuffer.cpp,v 1.3 2006/11/21 18:29:13 ping Exp $
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

#include "hlxclib/stdio.h"
#include "hxresult.h"
#include "hxsringbuffer.h"
#include "hxthread.h"
#include "stdlib.h"

HXSimpleRingBuffer::HXSimpleRingBuffer(INT32 lBufSize, INT32 lReservedBytes)
	: m_lBufSize(lBufSize)
	, m_lReservedBytes(lReservedBytes)
	, m_lNextGet(0)
	, m_lNextPut(0)
	, m_lReserveGet(0)
	, m_lOffset(0)
	, m_state(HX_RING_BUFFER_UNINIT)
{
    m_pRingBuffer = (BYTE*) malloc(lBufSize);
#ifdef THREADS_SUPPORTED
    HXMutex::MakeMutex(m_pMutex);
#else
    HXMutex::MakeStubMutex(m_pMutex);
#endif
}

HXSimpleRingBuffer::~HXSimpleRingBuffer()
{
    free(m_pRingBuffer);
    HX_DELETE(m_pMutex);
}

void
HXSimpleRingBuffer::Lock()
{
    m_pMutex->Lock();
}
void
HXSimpleRingBuffer::Unlock()
{
    m_pMutex->Unlock();
}

HX_RESULT
HXSimpleRingBuffer::Seek(INT32 lOffset)
{
    INT32 seekCount = (m_lOffset - lOffset);
    INT32 lGetBytes =AvailableGetBytes(TRUE);

    //printf("Seek(%ld): (%ld)%ld->%ld offset=%ld\n", lOffset, m_lReserveGet, m_lNextGet, m_lNextPut, m_lOffset);
    if (seekCount >= 0 && (lGetBytes - seekCount) >= 0)
    {
	if (((INT32)m_lNextPut - seekCount) < 0)
	{
		m_lNextGet = ((m_lNextPut - seekCount) + m_lBufSize) ;
	}
	else
	{
		m_lNextGet = (m_lNextPut - seekCount);
	}
	// Set the reserve position
	INT32 uValidLen;
	uValidLen = MIN(m_lReservedBytes, (((m_lNextGet - m_lReserveGet) + m_lBufSize) % m_lBufSize));
	m_lReserveGet = (((m_lNextGet - uValidLen) + m_lBufSize) % m_lBufSize);
    }
    else
    {
	//printf("********Ring buffer RESET*******************************\n");
	Reset();
    }

    return HXR_OK;
}


HX_RESULT
HXSimpleRingBuffer::Put(const BYTE* buf, INT32 lLen, HXBOOL bWithReserved/*=FALSE*/)
{
    if (GetState() == HX_RING_BUFFER_RESET)
    {
	return HXR_UNEXPECTED;
    }

    if (IsFull(lLen, bWithReserved))
    {
	return HXR_FAIL;
    }

    Lock();

    if ((lLen + m_lNextPut) <= m_lBufSize)
    {
	// no ring
	memcpy((m_pRingBuffer + m_lNextPut), buf, lLen);
    }
    else
    {
	// ring
	//printf("Put(%ld): (%ld)%ld->%ld offset=%ld\n", lLen, m_lReserveGet, m_lNextGet, m_lNextPut, m_lOffset);
	INT32 lFirstChunkLen = (m_lBufSize - m_lNextPut);
	memcpy((m_pRingBuffer + m_lNextPut), buf, lFirstChunkLen);
	memcpy(m_pRingBuffer, (buf + lFirstChunkLen), (lLen - lFirstChunkLen));
    }
    m_lNextPut = (m_lNextPut + lLen) % m_lBufSize;
    m_lOffset += lLen;
    setState(HX_RING_BUFFER_SET);

    //printf("Put(%ld): (%ld)%ld->%ld offset=%ld\n", lLen, m_lReserveGet, m_lNextGet, m_lNextPut, m_lOffset);

    Unlock();

    return HXR_OK;
}

HX_RESULT
HXSimpleRingBuffer::Get(BYTE* buf, INT32& lLen, HXBOOL bWithReserved/*=FALSE*/)
{
    if (GetState() == HX_RING_BUFFER_RESET)
    {
	return HXR_UNEXPECTED;
    }

    INT32 lAvailableGetBytes = AvailableGetBytes(bWithReserved);
    if (IsEmpty(lLen, bWithReserved))
    {
	lLen = lAvailableGetBytes;
	return HXR_FAIL;
    }

    Lock();

    INT32 lStart = (bWithReserved) ? m_lReserveGet : m_lNextGet;
    if ((lStart + lLen) <= m_lBufSize )
    {
	// no ring
	memcpy(buf, (m_pRingBuffer + lStart), lLen);
	m_lNextGet += lLen;
    }
    else
    {
	// ring
        //printf("Get(%ld): (%ld)%ld->%ld offset=%ld\n", lLen, m_lReserveGet, m_lNextGet, m_lNextPut, m_lOffset);
	INT32 lFirstChunkLen = (m_lBufSize - lStart);
	memcpy(buf, (m_pRingBuffer + lStart), lFirstChunkLen);
	memcpy((buf + lFirstChunkLen), m_pRingBuffer, lLen - lFirstChunkLen);
	m_lNextGet = lLen - lFirstChunkLen;
    }

    if (lLen == lAvailableGetBytes)
    {
	setState(HX_RING_BUFFER_UNINIT);
    }

    // Set the reserve position
    INT32 uValidLen;
    uValidLen = MIN(m_lReservedBytes, (((m_lNextGet - m_lReserveGet) + m_lBufSize) % m_lBufSize));
    m_lReserveGet = (((m_lNextGet - uValidLen) + m_lBufSize) % m_lBufSize);

    Unlock();

    //printf("Get(%ld): (%ld)%ld->%ld offset=%ld\n", lLen, m_lReserveGet, m_lNextGet, m_lNextPut, m_lOffset);

    return HXR_OK;
}

INT32
HXSimpleRingBuffer::AvailablePutBytes(HXBOOL bWithReserved/*=FALSE*/)
{
    INT32 lBytes=m_lBufSize;
    if (m_state != HX_RING_BUFFER_UNINIT)
    {
	INT32 lEnd = (bWithReserved) ? m_lNextGet : m_lReserveGet;
	lBytes = ((lEnd - m_lNextPut) + m_lBufSize) % m_lBufSize;
    }
    return lBytes;
}

INT32
HXSimpleRingBuffer::AvailableGetBytes(HXBOOL bWithReserved/*=FALSE*/)
{
    INT32 lBytes=0;
    if (m_state == HX_RING_BUFFER_SET)
    {
	INT32 lStart = (bWithReserved) ? m_lReserveGet : m_lNextGet;
	if (m_lNextPut == lStart)
	{
	    lBytes = m_lBufSize; // full
	}
	else
	{
    	    lBytes = ((m_lNextPut - lStart) + m_lBufSize) % m_lBufSize;
	}
    }
    return lBytes;
}
	
HXBOOL
HXSimpleRingBuffer::IsFull(INT32 lLen, HXBOOL bWithReserved/*=FALSE*/)
{
    return (lLen > AvailablePutBytes(bWithReserved));
}

HXBOOL
HXSimpleRingBuffer::IsEmpty(INT32 lLen, HXBOOL bWithReserved/*=FALSE*/)
{
    return (lLen > AvailableGetBytes(bWithReserved));
}



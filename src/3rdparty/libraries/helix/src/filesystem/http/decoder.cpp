/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: decoder.cpp,v 1.6 2006/03/08 02:28:35 tknox Exp $
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

#include "hlxclib/string.h"

#include "hxtypes.h"
#include "hxresult.h"
#include "hxmarsh.h"
#include "chunkres.h"
#include "zlib.h"
#include "zconf.h"

#include "decoder.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

static int gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define RESERVED     0xE0 /* bits 5..7: reserved */

#define OUTPUT_BUF_SIZE	2048


CDecoder::CDecoder()
    : m_pOutputSink	(NULL)
    , m_ulSinkPos	(0)
    , m_ulInputSpace	(0)
    , m_pInputBuf	(NULL)
    , m_ulInputLen	(0)
    , m_ulOutputSpace	(0)
    , m_pOutputBuf	(NULL)
    , m_bHeaderRemoved	(FALSE)
    , m_bStarted	(FALSE)
    , m_bSeenIncomplete	(FALSE)
{
    m_DStream.avail_in = 0;
    m_DStream.next_in = NULL;
    m_DStream.avail_out = 0;
    m_DStream.next_out = NULL;
    m_DStream.zalloc = (alloc_func)0;
    m_DStream.zfree = (free_func)0;
    m_DStream.opaque = (voidpf)0;
}

CDecoder::~CDecoder()
{
    // We don't own the output sink, 
    // so just drop the pointer
    m_pOutputSink = NULL;

    HX_VECTOR_DELETE(m_pInputBuf);
    HX_VECTOR_DELETE(m_pOutputBuf);
}

HX_RESULT
CDecoder::Reset()
{
    m_ulSinkPos = 0;

    m_ulInputSpace = 0;
    HX_VECTOR_DELETE(m_pInputBuf);
    m_ulInputLen = 0;

    m_ulOutputSpace = 0;
    HX_VECTOR_DELETE(m_pOutputBuf);

    m_bHeaderRemoved = FALSE;

    // Allocate a default output buffer
    m_pOutputBuf = new char[OUTPUT_BUF_SIZE];
    m_ulOutputSpace = OUTPUT_BUF_SIZE;

    m_DStream.avail_in = 0;
    m_DStream.next_in = NULL;
    m_DStream.avail_out = 0;
    m_DStream.next_out = NULL;

    SetSeenIncomplete (FALSE);

    int err = Z_OK;
    err = inflateInit2(&m_DStream, -MAX_WBITS);
    if (err != Z_OK) 
    { 
	return HXR_FAIL;
    }

    return HXR_OK;
}

void
CDecoder::SetOutputSink(CChunkyRes* pOutputSink)
{
    m_pOutputSink = pOutputSink;
}

HX_RESULT
CDecoder::SetData(UINT32 ulOffset, const char* pBuf, UINT32 ulCount)
{
    HX_RESULT hResult = HXR_OK;

    if (!m_bStarted)
    {
	m_bStarted = TRUE;
	hResult = Reset();

	if (FAILED(hResult))
	{
	    return hResult;
	}
    }

    if (SUCCEEDED(hResult))
    {
	// Make sure all of our buffers are big enough 
	// and copy the undecoded data to the input queue
	hResult = PrepareBuffers(pBuf, ulCount);

	if (SUCCEEDED(hResult))
	{
	    // Decode as much data as possible 
	    // and send it to the output sink
	    hResult = DecodeData();
	}
	else if (HXR_INCOMPLETE == hResult)
	{
	    // We failed simply because we don't yet have
	    // enough data to begin decompression. We'll
	    // just try again when the next chunk comes in.
            SetSeenIncomplete(TRUE);
	    hResult = HXR_OK;
	}
    }

    return hResult;
}

HX_RESULT
CDecoder::SkipHeader(char* pBuf, UINT32 ulCount, REF(char*)pNewBuf)
{
    HX_RESULT hResult = HXR_OK;
    UINT8  nMethod = 0;
    UINT8  nFlags = 0;
    UINT32 ulLen = 0;
    UINT8  nChar = 0;
    UINT32 ulPos = 0;

    // Make sure there are at least 10 bytes ready to be decoded
    if (ulCount < 10)
    {
	return HXR_INCOMPLETE;
    }

    // Check the gzip header of the given buffer. If the
    // gzip magic header is not present, return an error.
    // Also return an error if the magic header is present 
    // but the rest of the header is incorrect.

    // Check the gzip magic header
    for (ulLen = 0; ulLen < 2; ulLen++) 
    {
	nChar = getbyte((UCHAR*)pBuf++);
	ulPos++;

	if (nChar != gz_magic[ulLen]) 
	{
	    return HXR_FAIL;
	}
    }

    nMethod = getbyte((UCHAR*)pBuf++);
    nFlags = getbyte((UCHAR*)pBuf++);
    ulPos += 2;

    if (nMethod != Z_DEFLATED || (nFlags & RESERVED) != 0)
    {
	return HXR_FAIL;
    }

    // Discard time, xflags and OS code
    pBuf += 6;
    ulPos += 6;

    // Skip the extra field
    if ((nFlags & EXTRA_FIELD) != 0)
    {
	if (ulCount < 12)
	{
	    return HXR_INCOMPLETE;
	}

	ulLen = (UINT32)getbyte((UCHAR*)pBuf++);
	ulLen += ((UINT32)getbyte((UCHAR*)pBuf++)) << 8;
	ulPos += 2;

	while (ulLen-- != 0 && ulPos < ulCount)
	{
	    pBuf++;
	    ulPos++;
	}

	if (ulLen)
	{
	    // We need to wait until we have all of the header 
	    // fields in our input buffer
	    return HXR_INCOMPLETE;
	}
    }

    // Skip the original file name
    if ((nFlags & ORIG_NAME) != 0)
    {
	while ((ulPos < ulCount) &&
	       (nChar = getbyte((UCHAR*)pBuf++)) != 0)
	{
	    ulPos++;
	}

	if (ulPos == ulCount)
	{
	    // We need to wait until we have all of the header 
	    // fields in our input buffer
	    return HXR_INCOMPLETE;
	}
    }

    // Skip the .gz file comment
    if ((nFlags & COMMENT) != 0)
    {
	while ((ulPos < ulCount) &&
	       (nChar = getbyte((UCHAR*)pBuf++)) != 0)
	{
	    ulPos++;
	}

	if (ulPos == ulCount)
	{
	    // We need to wait until we have all of the header 
	    // fields in our input buffer
	    return HXR_INCOMPLETE;
	}
    }

    // Skip the header crc
    if ((nFlags & HEAD_CRC) != 0)
    {
	if (ulCount <= ulPos + 2)
	{
	    // We need to wait until we have all of the header 
	    // fields in our input buffer
	    return HXR_INCOMPLETE;
	}

	pBuf += 2;
	ulPos += 2;
    }

    pNewBuf = pBuf;

    return hResult;
}

UINT32
CDecoder::GetContentRead()
{
    return m_ulSinkPos;
}

HX_RESULT
CDecoder::PrepareBuffers(const char* pBuf, UINT32 ulCount)
{
    HX_RESULT hResult = HXR_OK;
    char* pNewBuf = (char*)pBuf;

    // Copy the data into the input buffer at the appropriate position
    UINT32 ulSpaceNeeded = ulCount;
    ulSpaceNeeded += m_DStream.avail_in;

    if (ulSpaceNeeded > m_ulInputSpace)
    {
	// Create a new buffer
	char* pNewBuffer = new char[ulSpaceNeeded];

	// Copy remaining undecoded data to the new buffer
	memcpy(pNewBuffer, m_DStream.next_in, m_DStream.avail_in); /* Flawfinder: ignore */
	
	// Copy new data to the new buffer
	memcpy(pNewBuffer + m_DStream.avail_in, pBuf, ulCount); /* Flawfinder: ignore */

	// Delete the old buffer
	HX_VECTOR_DELETE(m_pInputBuf);
	m_pInputBuf = pNewBuffer;
	m_ulInputSpace = ulSpaceNeeded;
	m_ulInputLen = ulSpaceNeeded;
    }
    else
    {
	// Move the unencoded data to the beginning of the buffer
	memcpy(m_pInputBuf, m_DStream.next_in, m_DStream.avail_in); /* Flawfinder: ignore */

	// Copy new data to the existing buffer
	memcpy(m_pInputBuf + m_DStream.avail_in, pBuf, ulCount); /* Flawfinder: ignore */
	m_ulInputLen = m_DStream.avail_in + ulCount;
    }

    if (!m_bHeaderRemoved)
    {
	// Try to remove the gzip header
	hResult = SkipHeader((char*)m_pInputBuf, ulCount, pNewBuf);

	// If we failed because there are not enough bytes, return
	// and try again later
	if (HXR_INCOMPLETE == hResult)
	{
	    return HXR_INCOMPLETE;
	}
	else if (FAILED(hResult))
	{
	    return HXR_FAIL;
	}
	else
	{
	    m_bHeaderRemoved = TRUE;
    	    m_ulInputLen -= (pNewBuf - m_pInputBuf);
	}
    }

    m_DStream.next_in  = (UCHAR*)pNewBuf;
    m_DStream.avail_in = (uInt)m_ulInputLen;

    m_DStream.next_out = (UCHAR*)m_pOutputBuf;
    m_DStream.avail_out = (uInt)m_ulOutputSpace;

    return HXR_OK;
}

HX_RESULT
CDecoder::DecodeData()
{
    HX_RESULT hResult = HXR_OK;
    int err = Z_OK;

    // Decode the data until no more input can be decoded
    for (;;) 
    {
	// Reset the output buffer
        m_DStream.next_out = (UCHAR*)m_pOutputBuf;
	m_DStream.avail_out = (uInt)m_ulOutputSpace;

        err = inflate(&m_DStream, Z_NO_FLUSH);
        if (err == Z_STREAM_END ||
	    err == Z_OK)
	{
	    // Copy the decoded data into the output buffer
	    if (m_pOutputSink)
	    {
		m_pOutputSink->SetData(m_ulSinkPos, m_pOutputBuf, 
		    m_ulOutputSpace - m_DStream.avail_out);
		m_ulSinkPos += m_ulOutputSpace - m_DStream.avail_out;
	    }

	    if (err == Z_STREAM_END)
	    {
		// We've decompressed the entire stream
		break;
	    }
	}
	else if (err == Z_BUF_ERROR)
	{
	    // We are not able to make any more progress until
	    // we get more input data
	    break;
	}
	else
	{
	    hResult = HXR_FAIL;
	}
    }

    if (err == Z_STREAM_END)
    {
	inflateEnd(&m_DStream);

	// We no longer need our decompression buffers
	HX_VECTOR_DELETE(m_pInputBuf);
	HX_VECTOR_DELETE(m_pOutputBuf);

	// Set m_bStarted to FALSE. We don't expect any more data,
	// so if we receive some we should assume it's a new file
	// and reset all of our important members.
	m_bStarted = FALSE;
    }

    return hResult;
}

HXBOOL
CDecoder::SeenIncomplete() const
{
    return m_bSeenIncomplete;
}

void
CDecoder::SetSeenIncomplete(HXBOOL bSeenIncomplete)
{
    m_bSeenIncomplete = bSeenIncomplete;
}

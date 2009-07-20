/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rfc2429hlpr.cpp,v 1.6 2008/11/11 15:21:38 alokjain Exp $
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

#include "./rfc2429hlpr.h"
#include "bitpack.h"


CRFC2429Helper::CRFC2429Helper() :
    m_bPictureStart(FALSE),
    m_ulPayloadSize(0),
    m_bP(FALSE),
    m_pPayload(0)
{}

CRFC2429Helper::~CRFC2429Helper()
{}

HX_RESULT CRFC2429Helper::OnPacket(const UINT8* pBuf, ULONG32 ulBufSize)
{
    HX_RESULT res = HXR_UNEXPECTED;
    
    const UINT8* pData = pBuf;

    m_bPictureStart = FALSE;
    m_ulPayloadSize = 0;
    m_bP = FALSE;
    m_pPayload = 0;

    // RFC 2429 Section 4.1
    //  0                   1
    //  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |    RR   |P|V|   PLEN    |PEBIT|
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    if ((ulBufSize >= 3) && // Make sure the buffer is large enough
	((pData[0] & 0xf8) == 0x00)) // Make sure RR field is 0
    {
	m_bP = ((pData[0] & 0x04) == 0x04);
	HXBOOL bV = ((pData[0] & 0x02) == 0x02);
	UINT8 uPLEN = ((pData[0] & 0x1) << 6) | ((pData[1] >> 3) & 0x1f);
	
	pData += 2;

	// Make sure the packet is large enough
	if (ulBufSize >= (ULONG32)(2 + ((bV) ? 1 : 0) + uPLEN + 1))
	{
	    if (bV)
	    {
		// Skip the VRC(Section 4.2) since we don't really
		// care about it's contents
		pData++;
	    }

	    if (uPLEN > 0)
	    {
		// Skip the extra picture header (Section 5.1.1) since
		// we don't really care about it's contents
		pData += uPLEN;
	    }
	    
	    m_pPayload = pData;
	    m_ulPayloadSize = (ulBufSize - (m_pPayload - pBuf)) * 8;
	    
	    if (m_bP && (m_pPayload[0] & 0xfc) == 0x80)
		m_bPictureStart = TRUE;

	    res = HXR_OK;
	}
    }

    return res;
}

HXBOOL CRFC2429Helper::IsPictureStart() const
{
    return m_bPictureStart;
}

ULONG32 CRFC2429Helper::DataSize() const // in bits
{
    ULONG32 ret = m_ulPayloadSize;

    if (m_bP)
	ret += 16;

    return ret;
}

HX_RESULT CRFC2429Helper::CopyPayload(BitPacker& bp)
{
    if (m_bP)
	bp.PackBits(0,16);
    
    HX_RESULT retVal = bp.PackBits(m_pPayload, m_ulPayloadSize, 0);
    return retVal;
}

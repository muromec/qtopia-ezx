/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rfc2190hlpr.cpp,v 1.5 2008/11/11 15:21:38 alokjain Exp $
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

#include "./rfc2190hlpr.h"
#include "bitpack.h"

CRFC2190Helper::CRFC2190Helper() :
    m_bPictureStart(FALSE),
    m_ulDataSize(0),
    m_pPayload(0),
    m_ulStartOffset(0)
{}

CRFC2190Helper::~CRFC2190Helper()
{}
    
HX_RESULT CRFC2190Helper::OnPacket(const UINT8* pBuf, ULONG32 ulBufSize)
{
    HX_RESULT res = HXR_FAILED;

    m_bPictureStart = FALSE;
    m_ulDataSize = 0;
    m_pPayload = 0;
    m_ulStartOffset = 0;

    if (ulBufSize > 12)
    {
	m_ulStartOffset = ((ULONG32)(pBuf[0] >> 3)) & 0x7; // SBIT
	ULONG32 ulEBIT = (ULONG32)pBuf[0] & 0x7;

	if (pBuf[0] & 0x80) // Check F bit
	    if (pBuf[0] & 0x40) // Check P bit
	    {
		if (((pBuf[3] & 0x3) == 0) && // Check all the reserved bits
		    (pBuf[8] == 0) &&
		    (pBuf[9] == 0) &&
		    ((pBuf[10] & 0xE0) == 0))
		{
		    // This is a Mode C packet (Section 5.3)
		    m_pPayload = pBuf + 12;
		    res = HXR_OK;
		}
	    }
	    else
	    {
		if ((pBuf[3] & 0x3) == 0) // Check for reserved bits
		{
		    // This is a Mode B packet (Section 5.2)
		    m_pPayload = pBuf + 8;
		    res = HXR_OK;
		}
	    }
	else if (((pBuf[1] & 0x1) == 0) && // Check for reserved bits
		 ((pBuf[2] & 0xe) == 0))
	{
	    // This is a Mode A packet (Section 5.1)
	    m_pPayload = pBuf + 4;

	    if ((pBuf[4] == 0x00) &&  // Check for PSC
		(pBuf[5] == 0x00) &&
		((pBuf[6] & 0xfc) == 0x80))
		m_bPictureStart = TRUE;
	}

	if (SUCCEEDED(res))
	    m_ulDataSize = ((ulBufSize - (m_pPayload - pBuf) ) * 8 - 
			    (m_ulStartOffset + ulEBIT));
    }

    return res;
}

HXBOOL CRFC2190Helper::IsPictureStart() const
{
    return m_bPictureStart;
}

ULONG32 CRFC2190Helper::DataSize() const // in bits
{
    return m_ulDataSize;
}

HX_RESULT CRFC2190Helper::CopyPayload(BitPacker& bp)
{
    HX_RESULT res = HXR_FAILED;

    res = bp.PackBits(m_pPayload, m_ulDataSize, m_ulStartOffset);

    return res;
}

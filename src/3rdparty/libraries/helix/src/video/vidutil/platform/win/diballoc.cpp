/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: diballoc.cpp,v 1.7 2007/07/06 20:54:07 jfinnecy Exp $
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

#include "hxtypes.h"
#include "hlxclib/windows.h"
#include <windowsx.h>

#include "hxmap.h"
#include "hxassert.h"
#include "hxerrors.h"
#include "diballoc.h"
#include "hxheap.h"

///////////////////
//	private data
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif


CHXDIBits::CHXDIBits()
{
    m_hDIB = NULL;
}

CHXDIBits::~CHXDIBits()
{
    if (m_hDIB)
    {
	GlobalFree(m_hDIB);
	m_hDIB = NULL;
    }
}

HX_RESULT
CHXDIBits::GetDIBits(HDC		hDC,
		     HBITMAP		hBM,
		     UCHAR*&		pBits,
		     BITMAPINFOHEADER*&	pHeader)
{
    HX_RESULT		hr = HXR_OK;
    WORD		wBits = 0;
    DWORD   		dwLen = 0;
    BITMAP		bm;
    BITMAPINFOHEADER	bi;
    LPBITMAPINFOHEADER	lpbi = NULL;

    pBits = NULL;
    pHeader = NULL;

    if (!hDC || !hBM)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    GetObject(hBM, sizeof(bm), &bm);

    wBits = (WORD)(bm.bmPlanes * bm.bmBitsPixel);

    bi.biSize		= sizeof(BITMAPINFOHEADER);
    bi.biWidth		= bm.bmWidth;
    bi.biHeight		= bm.bmHeight;
    bi.biPlanes		= 1;
    bi.biBitCount	= wBits;
    bi.biCompression	= BI_RGB;
    bi.biSizeImage	= WIDTHBYTES(bm.bmWidth * wBits) * bm.bmHeight;
    bi.biXPelsPerMeter	= 0;
    bi.biYPelsPerMeter  = 0;
    bi.biClrUsed	= 0;
    bi.biClrImportant	= 0;

    dwLen = bi.biSize + DibNumColors(&bi) * sizeof(RGBQUAD) + bi.biSizeImage;

    if (!m_hDIB)
    {
	m_hDIB = GlobalAlloc(GMEM_MOVEABLE, dwLen);
    }
    else if (m_hDIB && (GlobalSize(m_hDIB) != dwLen))
    {
	GlobalFree(m_hDIB);
	m_hDIB = GlobalAlloc(GMEM_MOVEABLE, dwLen);
    }

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(m_hDIB);

    if (!lpbi)
    {
	// This is bad, it's not clear how callers of this class can
	// really handle a failure case. So, we need to make sure that
	// all our callers do handle this correctly.
	HX_ASSERT(lpbi);
	hr = HXR_FAILED;
	goto cleanup;
    }

    *lpbi = bi;

    ::GetDIBits(hDC,
	      hBM,
	      0,
	      (WORD)bi.biHeight,
	      DibPtr(lpbi),
	      (LPBITMAPINFO)lpbi,
	      DIB_RGB_COLORS);

    bi = *lpbi;

    lpbi->biClrUsed = DibNumColors(lpbi);

    pBits = (UCHAR*)DibPtr(lpbi);
    pHeader = lpbi;

    GlobalUnlock(m_hDIB);

cleanup:

    return hr;
}


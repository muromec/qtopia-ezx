/* ***** BEGIN LICENSE BLOCK *****
 * Version: RCSL 1.0/RPSL 1.0
 *
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 *
 * The contents of this file, and the files included with this file, are
 * subject to the current version of the RealNetworks Public Source License
 * Version 1.0 (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the RealNetworks Community Source License Version 1.0
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl,
 * in which case the RCSL will apply. You may also obtain the license terms
 * directly from RealNetworks.  You may not use this file except in
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks
 * applicable to this file, the RCSL.  Please see the applicable RPSL or
 * RCSL for the rights, obligations and limitations governing use of the
 * contents of the file.
 *
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the portions
 * it created.
 *
 * This file, and the files included with this file, is distributed and made
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 *
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef _FRAGMENT_H_
#define _FRAGMENT_H_

#include "hxtypes.h"
#include "hxstring.h"
#include "hxbuffer.h"
#include "hxslist.h"
#include "fragass.h"

class CFragment
{
protected:
    BYTE*   m_pData;
    UINT32  m_ulCounter;
    UINT32  m_ulSegSize;

public:

    CFragment	() {m_pData = NULL; m_ulCounter = 0; m_ulSegSize = 0;};

    virtual HX_RESULT Fragmentize(CHXBuffer* pData, CHXSimpleList*& pFragmentList) = 0;
};

class CPNFragment : public CFragment
{
public:
    CPNFragment (UINT32 ulSegSize);

    HX_RESULT Fragmentize(CHXBuffer* pData, CHXSimpleList*& pFragmentList);
};

class CJPEGFragment : public CFragment
{
private:

    // JPEG marker codes
    enum {
	M_SOF0  = 0xc0,
	M_SOF1  = 0xc1,
	M_SOF2  = 0xc2,
	M_SOF3  = 0xc3,

	M_SOF5  = 0xc5,
	M_SOF6  = 0xc6,
	M_SOF7  = 0xc7,

	M_JPG   = 0xc8,
	M_SOF9  = 0xc9,
	M_SOF10 = 0xca,
	M_SOF11 = 0xcb,

	M_SOF13 = 0xcd,
	M_SOF14 = 0xce,
	M_SOF15 = 0xcf,

	M_DHT   = 0xc4,

	M_DAC   = 0xcc,

	M_RST0  = 0xd0,
	M_RST1  = 0xd1,
	M_RST2  = 0xd2,
	M_RST3  = 0xd3,
	M_RST4  = 0xd4,
	M_RST5  = 0xd5,
	M_RST6  = 0xd6,
	M_RST7  = 0xd7,

	M_SOI   = 0xd8,
	M_EOI   = 0xd9,
	M_SOS   = 0xda,
	M_DQT   = 0xdb,
	M_DNL   = 0xdc,
	M_DRI   = 0xdd,
	M_DHP   = 0xde,
	M_EXP   = 0xdf,

	M_APP0  = 0xe0,
	M_APP1  = 0xe1,
	M_APP2  = 0xe2,
	M_APP3  = 0xe3,
	M_APP4  = 0xe4,
	M_APP5  = 0xe5,
	M_APP6  = 0xe6,
	M_APP7  = 0xe7,
	M_APP8  = 0xe8,
	M_APP9  = 0xe9,
	M_APP10 = 0xea,
	M_APP11 = 0xeb,
	M_APP12 = 0xec,
	M_APP13 = 0xed,
	M_APP14 = 0xee,
	M_APP15 = 0xef,

	M_JPG0  = 0xf0,
	M_JPG13 = 0xfd,
	M_COM   = 0xfe,

	M_TEM   = 0x01,

	M_JPG_ERROR = 0x100,
	M_EOF   = -1
    };

    UINT16		m_iRestartInterval;
    UINT16		m_iDensityUnit;
    UINT16		m_iXDensity;
    UINT16		m_iYDensity;
    UINT16		m_iPrecision;
    UINT16		m_iHeight;
    UINT16		m_iWidth;
    UINT16		m_iCompNum;
    UINT16		m_iYHk;
    UINT16		m_iYVk;
    UINT16		m_YquantTableNum;
    UINT16		m_iUHk;
    UINT16		m_iUVk;
    UINT16		m_UquantTableNum;
    UINT16		m_iVHk;
    UINT16		m_iVVk;
    UINT16		m_VquantTableNum;
    UINT16		m_YquantTable[64];
    UINT16		m_CquantTable[64];
    UINT16		m_iQualityParam;
    UINT16		m_iRTPtype;

    UINT16		EvaluateQualityParameter ();
    void		ReadFileHeader ();
    void		ReadScanHeader ();

    // util. functions
    UINT16		GetByte ();	// convenience function, get one byte value as an UINT16eger, unsigned
    UINT16		GetShort ();	// extract short as an UINT16eger, unsigned
    UINT16		GetMarker ();
    UINT16		NextMarker ();

    CHXString*		GetApp0();	// get JFIF APP0 marker
    void		SkipMarker ();	// skip unknown marker
    void		GetQt ();	// get DQT marker
    BOOL		GetRi ();	// get DRI marker
    void		GetSof();	// get SOF marker
    void		GetSoi ();	// get SOI marker
    void		GetSos ();	// get SOS marker

public:

    CJPEGFragment (UINT32 ulSegSize);

    HX_RESULT Fragmentize(CHXBuffer* pData, CHXSimpleList*& pFragmentList);
};

#endif //   _FRAGMENT_H_

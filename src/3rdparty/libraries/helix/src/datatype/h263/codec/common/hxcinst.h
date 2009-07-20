/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxcinst.h,v 1.4 2007/07/06 22:00:31 jfinnecy Exp $
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


#ifndef _HXCINST_H_
#define _HXCINST_H_

#include "hxvamain.h"
#include "machine.h"
#include "hvenc.h"
#include "hxcodeci.h"


class HXH263CodecInstance : public HXCodecInstance
{
private:
	static const S16 capsCIF; // CIF capability flag
	static const U32 effFactor; // hard coded efficiency factor; should we make configurable?
	static const U32 h263MaxInterFrames;
	static const U32 cifWidth;
	static const U32 cifHeight;
	static const U32 qcifWidth;
	static const U32 qcifHeight;
	static const U32 sqcifWidth;
	static const U32 sqcifHeight;

	DWORD m_fccType;
	DWORD m_openFlags;
	DWORD m_vcmVersion;

	// The decoder context - should probably wrap this up into a class
	S16	m_hDecoder;
	PICTURE_DESCR m_PicDesc;
	PICTURE m_PicOut;
	VvDecoderStats m_DecoderStats;
	S16 m_nextGob;
    LPBYTE m_lpDecoderPtr;

    LPBYTE GetDecoderPtr() { return m_lpDecoderPtr; }

public:

	static const U32 bogusId;

	HXH263CodecInstance();
	HXBOOL Initialize(void);
	
	HX_RESULT DecompressStart(const HXVA_Image_Format&,HXVA_Image_Format&);
	HX_RESULT DecompressEnd();
	HX_RESULT DecompressConvert(const HXVA_Image&,HXVA_Image&);
	HX_RESULT DecompressVerifyParams(void);
	HX_RESULT DecompressGetOutputFormat(void);

};


#endif

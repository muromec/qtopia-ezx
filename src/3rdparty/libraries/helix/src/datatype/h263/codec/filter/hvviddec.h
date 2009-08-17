/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hvviddec.h,v 1.3 2005/03/14 19:24:47 bobclark Exp $
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

#ifndef _INC_HVVIDCOD_H
#define _INC_HVVIDCOD_H

#include "hxvamain.h"
#include "hxcinst.h"


class VvVideoDecoderFilter
{
public:
    //
    // --- Com stuff ---
    //
    static 
	VvVideoDecoderFilter*		CreateInstance( HX_RESULT& );
    //
    // --- CTransform overrides ---
    //
    HX_RESULT		Receive( const HXVA_Image&, HXVA_Image&);
    HX_RESULT		CheckInputType( void );
    HX_RESULT		CheckTransform( void );
    HX_RESULT		DecideBufferSize( void );
    HX_RESULT		StartStreaming( const HXVA_Image_Format&,const HXVA_Image_Format&);
    HX_RESULT		StopStreaming( void );
    HX_RESULT		GetMediaType( void );
    HX_RESULT		EndOfStream( void );
    HX_RESULT		EndFlush( void );
	
	HXVA_Image_Format m_input_format;
	HXVA_Image_Format m_output_format;

    VvVideoDecoderFilter(HX_RESULT&);
    ~VvVideoDecoderFilter( void );

private:
    HXH263CodecInstance  *m_pcVideoCodec;
	HXBOOL				m_bStartDecompress;

	void			Init( void );

};
#endif // #ifndef _INC_HVVIDCOD_H

/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxvamain.cpp,v 1.3 2007/07/06 22:00:31 jfinnecy Exp $
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

// This module implements primitive functionality of Helix Video Arch
// required by the Decoder.

#include "hxvamain.h"

extern HXVA_FID_TYPES hxva_fid_type[HXVA_NUM_IMAGE_FORMATS + 1] = {
	TYPEENUM(HXVA_FID_UNDEFINED, HXVA_FID_TYPE_UNDEFINED)
          // Init
	TYPEENUM(HXVA_FID_RGB24, HXVA_FID_TYPE_RGB)
          // R 8 bits G  8 Bits B 8 Bits
	TYPEENUM(HXVA_FID_YUV12,HXVA_FID_TYPE_YUV)
		// I420
		// Y plane, U plane Quarter size V plane Quarter size
	TYPEENUM(HXVA_FID_IYUV,HXVA_FID_TYPE_YUV)
		// Same as I420
	TYPEENUM(HXVA_FID_YUY2,HXVA_FID_TYPE_YUV)
	TYPEENUM(HXVA_FID_H261,HXVA_FID_TYPE_DATA)
	TYPEENUM(HXVA_FID_H263,HXVA_FID_TYPE_DATA)
	TYPEENUM(HXVA_FID_H263PLUS,HXVA_FID_TYPE_DATA)
};

HXVA_Image_Format::HXVA_Image_Format()
{
    this->Clear();
}

void
HXVA_Image_Format::Clear()
{
    fid = HXVA_FID_UNDEFINED;
    dimensions.width = dimensions.height = 0;
    Complete();
    yuv_info.y_pitch = yuv_info.u_pitch = yuv_info.v_pitch = 0;
}

void
HXVA_Image_Format::Complete()
{
    // rectangle = dimensions;
	rectangle.location.x = rectangle.location.y = 0;
	rectangle.dimensions.height = dimensions.height;
	rectangle.dimensions.width = dimensions.width;

    if (HXVA_FID_Is_YUV(fid))
    {
		// these are defaults.. set actual if different.
        yuv_info.y_pitch = dimensions.width;
        yuv_info.u_pitch = yuv_info.v_pitch = dimensions.width / 2;
    }
}

HXVA_Image::HXVA_Image()
    :
    format()
{
    size = 0;
    sequence_number = 0;
    yuv_info.y_plane = yuv_info.u_plane = yuv_info.v_plane = NULL;
}

void
HXVA_Image::SetBasePointer(void *base)
{
    if (HXVA_FID_Is_YUV(format.fid))
    {
        if (base)
		{
			yuv_info.y_plane = (UINT8*)base;
			if (format.fid == HXVA_FID_YUV12 || format.fid == HXVA_FID_IYUV)
			{
				yuv_info.u_plane = yuv_info.y_plane +
					(format.yuv_info.y_pitch * format.rectangle.dimensions.height);
				yuv_info.v_plane = yuv_info.u_plane +
					(format.yuv_info.u_pitch * format.rectangle.dimensions.height / 2);
			}
		} 
		else 
		{
        
            yuv_info.y_plane = yuv_info.u_plane = yuv_info.v_plane = 0;
        }
    }
    else
        data = (UINT8*)base;
}

UINT8*
HXVA_Image::GetBasePointer(void) const
{
    if (HXVA_FID_Is_YUV(format.fid))
        return yuv_info.y_plane;

    return data;
}

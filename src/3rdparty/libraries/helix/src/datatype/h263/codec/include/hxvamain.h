/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxvamain.h,v 1.4 2007/07/06 22:00:32 jfinnecy Exp $
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

#ifndef HXVAMAIN_H__
#define HXVAMAIN_H__

#include "hxresult.h"

//
// Enumerate all of our external and internal image formats.
// Each enumeration literal is a format identifier, thus the name PIA_FID.
//
typedef enum {
	HXVA_FID_TYPE_UNDEFINED,
	HXVA_FID_TYPE_RGB,
	HXVA_FID_TYPE_YUV,
	HXVA_FID_TYPE_DATA,
	HXVA_NUM_FID_TYPES
} HXVA_FID_TYPES;

#define FIDENUM(fid, type) fid,

typedef enum {
	FIDENUM(HXVA_FID_UNDEFINED, HXVA_FID_TYPE_UNDEFINED)
          // Init
	FIDENUM(HXVA_FID_RGB24, HXVA_FID_TYPE_RGB)
          // R 8 bits G  8 Bits B 8 Bits
	FIDENUM(HXVA_FID_YUV12, HXVA_FID_TYPE_YUV)
		// I420
		// Y plane, U plane Quarter size V plane Quarter size
	FIDENUM(HXVA_FID_IYUV, HXVA_FID_TYPE_YUV)
		// Same as I420
	FIDENUM(HXVA_FID_YUY2, HXVA_FID_TYPE_YUV)
	FIDENUM(HXVA_FID_H261, HXVA_FID_TYPE_DATA)
	FIDENUM(HXVA_FID_H263, HXVA_FID_TYPE_DATA)
	FIDENUM(HXVA_FID_H263PLUS, HXVA_FID_TYPE_DATA)
    HXVA_NUM_IMAGE_FORMATS
} HXVA_FID;

#define TYPEENUM(fid, type) type,

#define Is_Valid_HXVA_FID(fid) (UINT32(fid) < UINT32(HXVA_NUM_IMAGE_FORMATS))


struct HXVA_Point
{
        UINT32 x;
		UINT32 y;

        HXVA_Point()	
		{ 
			x = y = 0; 
		}
        HXVA_Point(UINT32 xi, UINT32 yi)
		{ 			
			x = xi;
			y = yi; 
		}
};

struct HXVA_Dimensions
{
        UINT32 width;
		UINT32 height;

        HXVA_Dimensions()	
		{ 
			width = height = 0; 
		}
        HXVA_Dimensions(UINT32 w, UINT32 h)
		{ 			
			width = w;
			height = h; 
		}
};


struct HXVA_Rectangle
{
        HXVA_Point location;
		// Upper left corner
        HXVA_Dimensions dimensions;

        HXVA_Rectangle()	
		{ 
			location = HXVA_Point();
			dimensions = HXVA_Dimensions(); 
		}
        HXVA_Rectangle(UINT32 w, UINT32 h)
		{ 
			location = HXVA_Point();
			dimensions = HXVA_Dimensions(w,h);
		}
		HXVA_Rectangle(UINT32 x, UINT32 y, UINT32 w, UINT32 h)
		{ 
			location= HXVA_Point(x,y);
			dimensions= HXVA_Dimensions(w,h); 
		}
};

// A HXVA_Image_Format object contains enough decription of the image 
// format as to Init the codec.

struct HXVA_Image_Format
{
        HXVA_FID         fid;

        HXVA_Dimensions  dimensions;
		// Width and Height requested or actual

        HXVA_Rectangle   rectangle;
		// Rectangle requested or actual        
        
		union {
            struct {
                UINT32     y_pitch;
                UINT32     u_pitch;
                UINT32     v_pitch;
            } yuv_info;
        };

        //
        // Define Methods for operating on PIA_Image_Formats
        //
                        HXVA_Image_Format();

        void            Clear();

        // Fills in any unset fields 
		void            Complete();

};

// A HXVA_Image object represents an actual instance of an image.

struct HXVA_Image {
        HXVA_Image_Format    format;
        UINT32                 size;
            // "size" gives the total length, in bytes, of the image's data.
        UINT32               sequence_number;
            // TR
        union {
            struct {
                UINT8         *y_plane;
                UINT8         *u_plane;
                UINT8         *v_plane;
            } yuv_info;

            // for non-YUV formats.
            UINT8             *data;
        };
        //
        // Define Methods for operating on HXVA_Images
        //
                        HXVA_Image();
        // The constructor simply gives each field a "NULL"-like value.

        void            SetBasePointer(void*);
            // Does it intelligently for YUV/Data

        UINT8*          GetBasePointer(void) const;
            // Y pointer or Data pointer
};

extern "C" HXVA_FID_TYPES hxva_fid_type[];
inline  HXBOOL HXVA_FID_Is_YUV(HXVA_FID fid)
        {
            if (Is_Valid_HXVA_FID(fid))
                return (hxva_fid_type[fid]==HXVA_FID_TYPE_YUV);
            return FALSE;
        }

#endif // HXVAMAIN_H__

/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxwintyp.h,v 1.6 2006/03/13 21:47:33 shy_ward Exp $
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

#ifndef _HXWINTYP_H_
#define _HXWINTYP_H_

#include "hxtypes.h" /* Needed at least for various defines and types. */

#ifdef _WIN16
#define BI_BITFIELDS	3L
#endif

#ifdef _SYMBIAN
#include <coemain.h>
#include <w32std.h>
#endif

/****************************************************************************
 * 
 *  Structure:
 *
 *	HXxSize
 *
 *  Purpose:
 *
 *	Cross Platform definition of a size.
 *
 */
typedef struct HXEXPORT_CLASS _HXxSize
{
    INT32   cx;
    INT32   cy;
} HXxSize;

#ifdef __cplusplus

inline HXBOOL operator ==( const HXxSize& a, const HXxSize& b )
{
    return ( a.cx == b.cx ) && ( a.cy == b.cy );
}

inline HXBOOL operator !=( const HXxSize& a, const HXxSize& b )
{
    return !( a == b );
}

#endif // __cplusplus

/****************************************************************************
 * 
 *  Structure:
 *
 *	HXxPoint
 *
 *  Purpose:
 *
 *	Cross Platform definition of a point.
 *
 */
typedef struct HXEXPORT_CLASS _HXxPoint
{
    INT32   x;
    INT32   y;
} HXxPoint;

#ifdef __cplusplus

inline HXBOOL operator ==( const HXxPoint& a, const HXxPoint& b )
{
    return ( a.x == b.x ) && ( a.y == b.y );
}

inline HXBOOL operator !=( const HXxPoint& a, const HXxPoint& b )
{
    return !( a == b );
}

#endif // __cplusplus



/****************************************************************************
 * 
 *  Structure:
 *
 *	HXxRect
 *
 *  Purpose:
 *
 *	Cross Platform definition of a rectangle.
 *
 */
typedef struct HXEXPORT_CLASS _HXxRect
{
    INT32   left;
    INT32   top;
    INT32   right;
    INT32   bottom;
} HXxRect;

#define HXxRECT_WIDTH(r)	((r).right - (r).left)
#define HXxRECT_HEIGHT(r)	((r).bottom - (r).top)

#ifdef __cplusplus

inline HXBOOL operator ==( const HXxRect& a, const HXxRect& b )
{
    return ( a.left == b.left ) && 
	    ( a.top == b.top ) && 
	    ( a.right == b.right ) && 
	    ( a.bottom == b.bottom );
}

inline HXBOOL operator !=( const HXxRect& a, const HXxRect& b )
{
    return !( a == b );
}

inline HXBOOL HXxRect_IsEmpty( const HXxRect& rect )
{
    return ( rect.left >= rect.right  ) ||
	   ( rect.top  >= rect.bottom );
}

inline void HXxRect_Intersection( const HXxRect& r1, const HXxRect& r2, HXxRect* result )
{
    result->left   = ( r1.left   > r2.left   ) ? r1.left   : r2.left;
    result->top    = ( r1.top    > r2.top    ) ? r1.top    : r2.top;
    result->right  = ( r1.right  < r2.right  ) ? r1.right  : r2.right;
    result->bottom = ( r1.bottom < r2.bottom ) ? r1.bottom : r2.bottom;
}

inline void HXxRect_Set( HXxRect& rect, INT32 l, INT32 t, INT32 r, INT32 b )
{
    rect.left = l;
    rect.top = t;
    rect.right = r;
    rect.bottom = b;
}

inline void HXxRect_SetWithSize( HXxRect& rect, INT32 x, INT32 y, INT32 w, INT32 h )
{
    rect.left = x;
    rect.top = y;
    rect.right = x + w;
    rect.bottom = y + h;
}

#endif // __cplusplus

/****************************************************************************
 * 
 *  Structure:
 *
 *	HXxWindow
 *
 *  Purpose:
 *
 *	Cross Platform definition of a window. This struct is sufficiently
 *	wide to describe parent or child windows in Windows, MacOS, and
 *	various flavours of X-Windows.
 *
 *  Data Members:
 *
 *	void*	window
 *	platform specific window handle
 *	
 *	ULONG32	x, y
 *	position of top left corner relative to a client page
 *	
 *	ULONG32	width, height
 *	maximum window size
 *	
 *	HXxRect clipRect;            
 *	clipping rectangle in port coordinates
 *
 */
typedef struct HXEXPORT_CLASS _HXxWindow
{
    /* NOTE: The window parameter is NOT guaranteed to be unique for every
       corresponding CHXWindow. Use HXxWindowID if this is desired. */
    void*       window;
    ULONG32     x;
    ULONG32     y;                   
    ULONG32     width;
    ULONG32     height;
    HXxRect     clipRect;
#ifdef _UNIX
    void *      display;
#endif
#ifdef _SYMBIAN
    CDirectScreenAccess* iDSA;
#endif    
} HXxWindow;

typedef void* HXxWindowID;

/****************************************************************************
 * 
 *  Structure:
 *
 *	HXxEvent
 *
 *  Purpose:
 *
 *	Cross Platform definition of a event. This struct is sufficiently
 *	wide to describe an event in Windows, MacOS, and various flavours of 
 *	X-Windows.
 *
 *  Data Members:
 *
 *	void*	event
 *	platform specific event ID, can also be one of the several HXxMSG_*
 *	event IDs which map onto existing platform specific event IDs
 *      UNIX: X Event Type
 *	
 *	void*	window
 *	platform specific window handle
 *      UNIX: X Window ID
 *	
 *	void*	param1
 *	message specific parameter
 *      UNIX: Display*
 *	
 *	void*	param2
 *      Mac:  for UpdateEvt, either NULL or RgnHandle to be filled with updated area
 *      UNIX: Native              XEvent*
 *            HX_SURFACE_UPDATE  HXxWindow*
 *	
 */
typedef struct HXEXPORT_CLASS _HXxEvent
{
    ULONG32	event;	    /* IN  */
    void*	window;	    /* IN  */
    void*	param1;	    /* IN  */
    void*	param2;	    /* IN  */

    UINT32	result;	    /* OUT */
    HXBOOL	handled;    /* OUT */
} HXxEvent;


/****************************************************************************
 * 
 *  typedef:
 *
 *	HXxRegion
 *
 *  Purpose:
 *
 *	Cross Platform definition of a region. This typedef is redefined as
 *	appropriate to describe a region in Windows, MacOS, and various 
 *	flavours of X-Windows.
 *
 */
typedef void* HXxRegion;

/****************************************************************************
 * 
 *  typedef:
 *
 *	HXxDC
 *
 *  Purpose:
 *
 *	Cross Platform definition of a device context. This typedef is redefined as
 *	appropriate to describe a device context in Windows, MacOS, and various 
 *	flavours of X-Windows.
 *
 */
typedef void* HXxDC;

/****************************************************************************
 * 
 *  typedef:
 *
 *	HXxFont
 *
 *  Purpose:
 *
 *	Cross Platform definition of a font. This typedef is redefined as
 *	appropriate to describe a font in Windows, MacOS, and various 
 *	flavours of X-Windows.
 *
 */
typedef void* HXxFont;

/****************************************************************************
 * 
 *  typedef:
 *
 *	HXxColor
 *
 *  Purpose:
 *
 *	Cross Platform definition of a color. This typedef is redefined as
 *	appropriate to describe a font in Windows, MacOS, and various 
 *	flavours of X-Windows.
 *
 */
typedef ULONG32 HXxColor;

/****************************************************************************
 * 
 *  typedef:
 *
 *	HXxIcon
 *
 *  Purpose:
 *
 *	Cross Platform definition of a icon. This typedef is redefined as
 *	appropriate to describe a font in Windows, MacOS, and various 
 *	flavours of X-Windows.
 *
 */
typedef void* HXxIcon;

/****************************************************************************
 * 
 *  typedef:
 *
 *	HXxMenu
 *
 *  Purpose:
 *
 *	Cross Platform definition of a menu. This typedef is redefined as
 *	appropriate to describe a font in Windows, MacOS, and various 
 *	flavours of X-Windows.
 *
 */
typedef void* HXxMenu;

/****************************************************************************
 * 
 *  typedef:
 *
 *	HXxCursor
 *
 *  Purpose:
 *
 *	Cross Platform definition of a cursor. This typedef is redefined as
 *	appropriate to describe a cursor in Windows, MacOS, and various 
 *	flavours of X-Windows.
 *
 */
typedef void* HXxCursor;


/****************************************************************************
 * 
 *  Structure:
 *
 *	HXREGION
 *
 *  Purpose:
 *
 *	Cross Platform Region definition.
 */
typedef struct HXEXPORT_CLASS _HXBox
{
    short x1, x2, y1, y2;
} HXBOX, *HXBoxPtr;

typedef struct HXEXPORT_CLASS _HXxRegion
{
    HXBOX* rects;
    long    numRects;
} HXxBoxRegion, *HXxRegionPtr;

//Definition of the ExposeInfo structure pass with HX_SURFACE_UPDATE2.
typedef struct HXEXPORT_CLASS _HXxExposeInfo
{
    HXxRect     extents;  //The bounding rect of all dirty rects.
    HXxBoxRegion* pRegion;  //Pointer to dirty region. DO NOT MODIFY.
    HXxWindow*  pWindow;  //Pointer to the HXxWindow for this site.
    void*       pParam1;  //Reserved
    void*       pParam2;  //Reserved
} HXxExposeInfo;

#endif /* _HXWINTYP_H_ */

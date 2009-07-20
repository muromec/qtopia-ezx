/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxxtype.h,v 1.7 2005/03/14 19:27:09 bobclark Exp $
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

#ifndef _CHXxTYPE_H_
#define _CHXxTYPE_H_

#include "hxtypes.h" // Needed at least for various defines and types.
#include "hxwintyp.h" // Needed at least for various defines and types.

#include "hlxclib/windows.h"

class CHXxSize;
class CHXxPoint;
class CHXxRect;


/////////////////////////////////////////////////////////////////////////////
// CHXxSize - An extent, similar to Windows SIZE structure.

class HXEXPORT_CLASS CHXxSize : public _HXxSize
{
public:

// Constructors
	CHXxSize();
	CHXxSize(INT32 initCX, INT32 initCY);
	CHXxSize(const CHXxPoint& initPt);
	CHXxSize(const HXxSize& size)
		    {
			cx = size.cx;
			cy = size.cy;
		    };

	void SetSize(INT32 cxIn, INT32 cyIn)
			{
				cx = cxIn;
				cy = cyIn;
			};

	void GetSize(INT32& cxIn, INT32& cyIn) const
			{
				cxIn = cx;
				cyIn = cy;
			};

// Operations
	HXBOOL operator==(const CHXxSize& size) const;
	HXBOOL operator!=(const CHXxSize& size) const;
	void operator+=(const CHXxSize& size);
	void operator-=(const CHXxSize& size);

// Operators returning CHXxSize values
	CHXxSize operator+(const CHXxSize& size) const;
	CHXxSize operator-(const CHXxSize& size) const;
	CHXxSize operator-(void) const;

// Operators returning Windows native SIZE values
#if defined _WINDOWS
        operator SIZE (void);
        operator SIZE (void) const;
#endif

};

/////////////////////////////////////////////////////////////////////////////
// CHXxPoint - A 2-D point, similar to Windows POINT structure.

class HXEXPORT_CLASS CHXxPoint : public _HXxPoint
{
public:

// Constructors
	CHXxPoint();
	CHXxPoint(INT32 initX, INT32 initY);
	CHXxPoint(const CHXxSize& initSize);
	CHXxPoint(const HXxPoint& point)
		    {
			x = point.x;
			y = point.y;
		    };

	void SetPoint(INT32 xIn, INT32 yIn)
			{
				x = xIn;
				y = yIn;
			};

	void GetPoint(INT32& xIn, INT32& yIn) const
			{
				xIn = x;
				yIn = y;
			};

// Operations
	void Offset(INT32 xOffset, INT32 yOffset);
	void Offset(const CHXxPoint& point);
	void Offset(const CHXxSize& size);
	HXBOOL operator==(const CHXxPoint& point) const;
	HXBOOL operator!=(const CHXxPoint& point) const;
	void operator+=(const CHXxSize& size);
	void operator-=(const CHXxSize& size);

// Operators returning CHXxPoint values
	CHXxPoint operator+(const CHXxSize& size) const;
	CHXxPoint operator-(const CHXxSize& size) const;
	CHXxPoint operator-(void) const;

// Operators returning CHXxSize values
	CHXxSize operator-(const CHXxPoint& point) const;

// Operators returning Windows native SIZE values
#if defined _WINDOWS
        operator POINT (void);
        operator POINT (void) const;
#endif
};


inline CHXxPoint::CHXxPoint(const CHXxSize& initSize)
	{ SetPoint(initSize.cx,initSize.cy); }

inline CHXxPoint::CHXxPoint(INT32 initX, INT32 initY)
	{ SetPoint(initX,initY); }


/////////////////////////////////////////////////////////////////////////////
// CHXxRect - A 2-D rectangle, similar to Windows RECT structure.

class HXEXPORT_CLASS CHXxRect : public _HXxRect
{
public:

// Constructors
	CHXxRect();
	CHXxRect(INT32 l, INT32 t, INT32 r, INT32 b);
	CHXxRect(const CHXxPoint& point, const CHXxSize& size);
	CHXxRect(const HXxRect& rect)
		    {
			left   = rect.left;
			top    = rect.top;
			right  = rect.right;
			bottom = rect.bottom;
		    };

// Attributes (in addition to RECT members)
	INT32 Width() const;
	INT32 Height() const;
	CHXxSize Size() const;
	CHXxPoint TopLeft() const;
	CHXxPoint BottomRight() const;

// Additional Operations
	HXBOOL operator==(const CHXxRect& rect) const;
	HXBOOL operator!=(const CHXxRect& rect) const;
	void operator|=(const CHXxRect& rect); // Union.
	void operator&=(const CHXxRect& rect); // Intersection.

	void SetRect(INT32 l, INT32 t, INT32 r, INT32 b) 	
		{ left = l; top = t; right = r; bottom = b; }

	void Offset(INT32 dx, INT32 dy);
	void Offset(const CHXxPoint& point);
	void InflateRect(INT32 dx, INT32 dy);
	void SetRectEmpty() { SetRect(0,0,0,0); };

	HXBOOL IsRectEmpty() 
			{
				return ((right <= left) || (bottom <= top));
			};

	void GetRect(INT32& l, INT32& t, INT32& r, INT32& b) const
			{
				l = left;
				t = top;
				r = right;
				b = bottom;
			};

	void operator+=(const CHXxPoint& point);
	void operator-=(const CHXxPoint& point);

	HXBOOL PtInRect(const CHXxPoint& pt) const
			{
				return	(
							(pt.x >= left)  && 
							(pt.x <= right) && 
							(pt.y >= top)   && 
							(pt.y <= bottom)
						);
			};
	HXBOOL IsOverlapped(CHXxRect& rect)
	{
		return ( ( rect.right > left ) &&
			 ( rect.left < right ) &&
			 ( rect.bottom > top ) &&
			 ( rect.top < bottom ) );
	};
	HXBOOL Contains(CHXxRect& rect)
	{
		return ( ( rect.left >= left ) &&
			 ( rect.right <= right ) &&
			 ( rect.top >= top ) &&
			 ( rect.bottom <= bottom ) );
	}



// Operators returning CHXxRect values
	CHXxRect operator+(const CHXxPoint& point) const;
	CHXxRect operator-(const CHXxPoint& point) const;

#ifdef _MACINTOSH
	operator Rect* (void) const;
	CHXxRect(const Rect& rect);
#elif defined _WINDOWS
        operator RECT (void);
        operator RECT (void) const;
#endif

};

// CHXxSize
inline CHXxSize::CHXxSize()
	{ /* random filled */ }
inline CHXxSize::CHXxSize(INT32 initCX, INT32 initCY)
	{ cx = initCX; cy = initCY; }

inline CHXxSize::CHXxSize(const CHXxPoint& initPt)
	{
		cx = initPt.x;
		cy = initPt.y;
	}

inline HXBOOL CHXxSize::operator==(const CHXxSize& size) const
	{ return (cx == size.cx && cy == size.cy); }

inline HXBOOL CHXxSize::operator!=(const CHXxSize& size) const
	{ return (cx != size.cx || cy != size.cy); }

inline void CHXxSize::operator+=(const CHXxSize& size)
	{ cx += size.cx; cy += size.cy; }

inline void CHXxSize::operator-=(const CHXxSize& size)
	{ cx -= size.cx; cy -= size.cy; }

inline CHXxSize CHXxSize::operator+(const CHXxSize& size) const
	{ return CHXxSize(cx + size.cx, cy + size.cy); }

inline CHXxSize CHXxSize::operator-(const CHXxSize& size) const
	{ return CHXxSize(cx - size.cx, cy - size.cy); }
inline CHXxSize CHXxSize::operator-(void) const
	{ return CHXxSize(-cx, -cy); }

#if defined _WINDOWS
inline CHXxSize::operator SIZE () 
	{SIZE s; s.cx = (int)cx; s.cy = (int)cy; return s;}
inline CHXxSize::operator SIZE () const
	{SIZE s; s.cx = (int)cx; s.cy = (int)cy; return s;}
#endif


// CHXxPoint
inline CHXxPoint::CHXxPoint()
	{ /* random filled */ }


inline void CHXxPoint::Offset(INT32 xOffset, INT32 yOffset)
	{ x += xOffset; y += yOffset; }

inline void CHXxPoint::Offset(const CHXxPoint& point)
	{ x += point.x; y += point.y; }

inline void CHXxPoint::Offset(const CHXxSize& size)
	{ x += size.cx; y += size.cy; }

inline HXBOOL CHXxPoint::operator==(const CHXxPoint& point) const
	{ return (x == point.x && y == point.y); }

inline HXBOOL CHXxPoint::operator!=(const CHXxPoint& point) const
	{ return (x != point.x || y != point.y); }

inline void CHXxPoint::operator+=(const CHXxSize& size)
	{ x += size.cx; y += size.cy; }

inline void CHXxPoint::operator-=(const CHXxSize& size)
	{ x -= size.cx; y -= size.cy; }

inline CHXxPoint CHXxPoint::operator+(const CHXxSize& size) const
	{ return CHXxPoint(x + size.cx, y + size.cy); }

inline CHXxPoint CHXxPoint::operator-(const CHXxSize& size) const
	{ return CHXxPoint(x - size.cx, y - size.cy); }

inline CHXxPoint CHXxPoint::operator-(void) const
	{ return CHXxPoint(-x, -y); }

inline CHXxSize CHXxPoint::operator-(const CHXxPoint& point) const
	{ return CHXxSize(x - point.x, y - point.y); }

#if defined _WINDOWS
inline CHXxPoint::operator POINT () 
	{POINT p; p.x = (int)x; p.y = (int)y; return p;}
inline CHXxPoint::operator POINT () const
	{POINT p; p.x = (int)x; p.y = (int)y; return p;}
#endif

// CHXxRect
inline CHXxRect::CHXxRect()
	{ /* random filled */ }

inline CHXxRect::CHXxRect(INT32 l, INT32 t, INT32 r, INT32 b)
	{ SetRect(l,t,r,b); }

//cz - moved above
//inline void CHXxRect::SetRect(INT32 l, INT32 t, INT32 r, INT32 b)
//	{ left = l; top = t; right = r; bottom = b; }

inline CHXxRect::CHXxRect(const CHXxPoint& point, const CHXxSize& size)
	{ right = (left = point.x) + size.cx; bottom = (top = point.y) + size.cy; }

inline INT32 CHXxRect::Width() const
	{ return right - left; }

inline INT32 CHXxRect::Height() const
	{ return bottom - top; }

inline CHXxSize CHXxRect::Size() const
	{ return CHXxSize(right - left, bottom - top); }

inline CHXxPoint CHXxRect::TopLeft() const
	{ return(CHXxPoint(left, top)); }

inline CHXxPoint CHXxRect::BottomRight() const
	{ return(CHXxPoint(right, bottom)); }
	
inline HXBOOL CHXxRect::operator==(const CHXxRect& rect) const
	{ 
		return (
					left   == rect.left   &&
			    	top    == rect.top    &&
			    	right  == rect.right  &&
			    	bottom == rect.bottom
				);
	}

inline HXBOOL CHXxRect::operator!=(const CHXxRect& rect) const
	{ return !(*this == rect); }

inline void CHXxRect::operator|=(const CHXxRect& rect)
	{
		left = HX_MIN( left, rect.left );
		top = HX_MIN( top, rect.top );
		right = HX_MAX( right, rect.right );
		bottom = HX_MAX( bottom, rect.bottom );
	}

inline void CHXxRect::operator&=(const CHXxRect& rect)
	{
		left = HX_MAX( left, rect.left );
		top = HX_MAX( top, rect.top );
		right = HX_MAX( HX_MIN( right, rect.right ), left );
		bottom = HX_MAX( HX_MIN( bottom, rect.bottom ), top );
	}
	
inline void CHXxRect::Offset(INT32 dx, INT32 dy)
	{
		left += dx;
    	top += dy;
    	right += dx;
    	bottom += dy;
	}

inline void CHXxRect::Offset(const CHXxPoint& point)
	{
		left += point.x;
    	top += point.y;
    	right += point.x;
    	bottom += point.y;
	}
inline void CHXxRect::InflateRect(INT32 dx, INT32 dy)
	{
		left -= dx;
    	top -= dy;
    	right += dx;
    	bottom += dy;
	}

inline void CHXxRect::operator+=(const CHXxPoint& point)
	{ Offset(point.x, point.y); }

inline void CHXxRect::operator-=(const CHXxPoint& point)
	{ Offset(-point.x, -point.y); }

inline CHXxRect CHXxRect::operator+(const CHXxPoint& pt) const
	{ CHXxRect rect(*this); rect.Offset(pt.x, pt.y); return rect; }

inline CHXxRect CHXxRect::operator-(const CHXxPoint& pt) const
	{ CHXxRect rect(*this); rect.Offset(-pt.x, -pt.y); return rect; }

#ifdef _MACINTOSH
inline CHXxRect::operator Rect* (void) const
	{ Rect rect; ::SetRect(&rect, left, top, right, bottom); return (Rect*)&rect; } // (?) Dangerous. Passing pointer to a local variable.
inline CHXxRect::CHXxRect(const Rect& rect)
	{
		left = rect.left;
		top = rect.top;
		right = rect.right;
		bottom = rect.bottom;
	}
#elif defined _WINDOWS
inline CHXxRect::operator RECT () 
	{RECT xxrc; xxrc.left=(int)left, xxrc.top=(int)top, xxrc.right=(int)right, xxrc.bottom=(int)bottom; return xxrc;}
inline CHXxRect::operator RECT () const
	{RECT xxrc; xxrc.left=(int)left, xxrc.top=(int)top, xxrc.right=(int)right, xxrc.bottom=(int)bottom; return xxrc;}
#endif

#endif // _CHXxTYPE_H_

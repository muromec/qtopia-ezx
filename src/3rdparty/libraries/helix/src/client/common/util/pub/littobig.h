/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: littobig.h,v 1.6 2007/07/06 21:58:08 jfinnecy Exp $
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

/*

	CLittleEndianToBigEndian

	This class supports functions for converting data from LittleEndian byte order to 
	BigEndian byte order.

	This is different from using NetByte and HostByte order, becuase using the NetToHost fucntion 
	would not work on LittleEndian machines, as they are considered NetByte in the first place.

*/


#ifndef _CLITTOBIG_H
#define _CLITTOBIG_H

#include "hxtypes.h"

// GR 5/10/02 The old version of this class does a runtime test for endianness,
// and makes function calls to maybe swap the bytes.  None of that should really
// be necessary.  
//
// I'm only checking in a new version for Mac since that's all I can test on right now, 
// though the second inline version below for little-endian ought to work, too, and makes
// sense to use for Windows.

#if defined(_BIG_ENDIAN)

#define CLittleEndianToBigEndian_INLINE 1

class	CLittleEndianToBigEndian
{
  public:
    
    static inline	HXBOOL	TestBigEndian() { return TRUE; }
    
    static inline	void	ReverseWORD(UINT16& w) 
	{ 	
	    w = (((((UINT16) w)<<8) & 0xFF00) 
		 | ((((UINT16) w)>>8) & 0x00FF)); 
	}
    static inline	void	ReverseDWORD(ULONG32& w)
	{
	    w = (((((UINT32) w)<<24) & 0xFF000000)  | 
		 ((((UINT32) w)<< 8) & 0x00FF0000)  | 
		 ((((UINT32) w)>> 8) & 0x0000FF00)  | 
		 ((((UINT32) w)>>24) & 0x000000FF));
	}


};

#elif defined(_LITTLE_ENDIAN)

#define CLittleEndianToBigEndian_INLINE 1

class	CLittleEndianToBigEndian
{
  public:
    static inline	HXBOOL	TestBigEndian() { return FALSE; }
    
    static inline	void	ReverseWORD(UINT16& w) {}
    static inline	void	ReverseDWORD(ULONG32& w) {}
};


#else
// This is the old one which compiles to run-time tests of endianness, 
// which probably shouldn't needed for any platform

class	CLittleEndianToBigEndian
{
public:
	
	static	void	ReverseWORD(UINT16& w);
	static	void	ReverseDWORD(ULONG32& w);

	static	HXBOOL	TestBigEndian();

private:
	static	HXBOOL	mBigEndian;
	static	HXBOOL	mInitialized;
	
};

#endif



#endif // _CLITTOBIG_H

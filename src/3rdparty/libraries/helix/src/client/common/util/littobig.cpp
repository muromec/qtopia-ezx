/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: littobig.cpp,v 1.6 2005/03/14 20:28:55 bobclark Exp $
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

#include "littobig.h"
#include "hlxclib/string.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#ifndef CLittleEndianToBigEndian_INLINE

//
//	By Default the class is setup to be non big endian.
//
HXBOOL	CLittleEndianToBigEndian::mBigEndian=FALSE;


//
//	Make sure we run the TestBigEndian function.
//
HXBOOL	CLittleEndianToBigEndian::mInitialized=FALSE;

//
//	Checks to see if the machine is little endian or big endian.
//
#define bigendiantest 0x0001
HXBOOL	CLittleEndianToBigEndian::TestBigEndian(void)
{
    HXBOOL rc = FALSE;
    UINT16	test=bigendiantest;
    UCHAR*	bytes=(UCHAR*)&test;

    if (bytes[0] == 0)
    {
	rc = TRUE;
    }
	
    return rc;
}


void	CLittleEndianToBigEndian::ReverseWORD(UINT16&	w)
{
	if (mInitialized == FALSE)
	{
		mBigEndian = TestBigEndian();
	}

	if (mBigEndian)
	{
		UCHAR*	bytes;

		bytes=(UCHAR*)&w;

		//
		//	flip bytes around.
		//
		UCHAR tempbyte=bytes[0];
		bytes[0]=bytes[1];
		bytes[1]=tempbyte;
	}
}


void	CLittleEndianToBigEndian::ReverseDWORD(ULONG32&  dw)
{
	if (mInitialized==FALSE)
	{
		mBigEndian = TestBigEndian();
	}

	if (mBigEndian)
	{
		UCHAR*	bytes;
		UCHAR	newbytes[4];

		bytes=(UCHAR*)&dw;

		//
		//	reverse the bytes in the long
		//
		newbytes[0]=bytes[3];
		newbytes[1]=bytes[2];
		newbytes[2]=bytes[1];
		newbytes[3]=bytes[0];

		::memcpy(bytes,newbytes,sizeof(ULONG32)); /* Flawfinder: ignore */
	}
}

#endif // !defined CLittleEndianToBigEndian_INLINE

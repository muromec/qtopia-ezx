/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxunicod.cpp,v 1.5 2004/07/09 18:44:50 hubbe Exp $
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

#include "hxtypes.h"
#include "hlxclib/string.h"
#include "hxassert.h"
#include "hxunicod.h"
#include "hxresult.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


//
//	Currently this function does a really simple conversion.
//	This is a cheater version, and it needs to be investigated.
//


HX_RESULT	CHXUnicode::ProcessFromUnicode(const char*  stringin,	UINT16	length, char* stringout, UINT16 outlength)
{	
	HX_ASSERT(stringin);
	HX_ASSERT(stringout);

	ULONG32	 newlength=0;

	char*		newstring=new char[length];
	HX_RESULT	result=HXR_OK;
	HX_ASSERT(newstring);

	if (!newstring) 
	{
		return NULL;
	}

	const char*	 bytesin=NULL;
	char*	 bytesout=NULL;

	bytesin=stringin;
	bytesout=newstring;

	while (1)
	{
		if (bytesin[0]==0 && bytesin[1]==0)
		{
			break;
		}

		if (bytesin[1]==0)
		{
		   newlength++;
		   bytesout[0]=bytesin[0];
		   bytesout++;
		   bytesin+=2;
		}
		else
		{
			newlength+=2;
			bytesout[0]=bytesin[0];
			bytesout[1]=bytesin[1];
			bytesout+=2;
			bytesin+=2;
		}
	}

	HX_ASSERT(outlength >= newlength+1);

	if (outlength < newlength+1)
	{
		result=HXR_BUFFERTOOSMALL;
		goto CleanUp;
	}

	::memset(stringout,NULL,(int)newlength+1);

	::memcpy(stringout,newstring,(int)newlength); /* Flawfinder: ignore */


CleanUp:

	if (newstring != NULL)
	{
		delete [] newstring;
		newstring=NULL;
	}

	return result;
}


//
//	Determine the length of the string.
//	This works for WIDE STRINGS ONLY!
//	This function does not returns the number of the characters only. 
//	It does not return the length including the two trailing bytes.
//
ULONG32		CHXUnicode::StringLength(const char* stringin)
{
	HX_ASSERT(stringin);

	ULONG32		length=0;
	const char*	p=stringin;

	while (TRUE)
	{
		if  (p[0]==0 && p[1]==0)
		{
			break;
		}

		length+=1;
		p+=2;
	}

	return length;
}



//
//	Determine the size of string.
//	This works for WIDE STRINGS ONLY!
//	This function returns the memory footprint of the string.  Which is the
//	length of the string + 2 bytes.
//
ULONG32		CHXUnicode::StringMemLength(const char* stringin)
{
	HX_ASSERT(stringin);

	ULONG32		length=0;
	const char*	p=stringin;

	while (TRUE)
	{
		if  (p[0]==0 && p[1]==0)
		{
			length+=2;
			p+=2;
			break;
		}

		length+=2;
		p+=2;
	}

	return length;
}




/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: unescape.cpp,v 1.2 2004/07/19 21:13:16 hubbe Exp $
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
//   $Id: unescape.cpp,v 1.2 2004/07/19 21:13:16 hubbe Exp $

#include "hxtypes.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "unescape.h"

void 
SetAttBuffer(IHXBuffer* pBuf, const char* p)
{
    pBuf->SetSize(strlen(p)+1);
    char* buf = (char*)pBuf->GetBuffer();
    const char* strPos = NULL;
    char* bufPos = NULL;

    for (;;)
    {
	switch(*p)
	{
	case '\0':
	    *buf = *p;
	    return;
	case '&':
	    bufPos = buf;
	    strPos = p;
	    break;
	case ';':
    	{
	    if (bufPos)
	    {
		// - we have an entity refference...
		// back up and convert it.
		const char* end = p - 1;
		const char* start = ++strPos;
		if (*start == '#')
		{
		    // character refference
		    int c = 0;
		    int error = 0;
		    int mult = 1;
		    if (*(start +1) == 'X')
		    {
			// hex
			++start;
			do
			{
			    switch(*end)
			    {
			    case 'a': case 'b': case 'c': case 'd': 
			    case 'e': case 'f':
				c += mult * (*end - 'a' + 10);
				break;
			    case 'A': case 'B': case 'C': case 'D': 
			    case 'E': case 'F':
				c += mult * (*end - 'A' + 10);
				break;
			    case '0': case '1': case '2': case '3':
			    case '4': case '5': case '6': case '7':
			    case '8': case '9':
				c += mult * (*end - '0');
				break;
			    default:
				// error
				error = 1;
			    }
			    mult *= 16;
			    --end;
			}
			while (end > start && !error);
		    }
		    else
		    {
		    	// decimal
			++start;
			do
			{
			    switch(*end)
			    {
			    case '0': case '1': case '2': case '3':
			    case '4': case '5': case '6': case '7':
			    case '8': case '9':
				c += mult * (*end - '0');
				break;
			    default:
				// error
				error = 1;
			    }
			    mult *= 10;
			    --end;
			}
			while (end > start && !error);
		    }

		    if (!error)
		    {
			buf = bufPos;
			*buf = (char) c;
		    }
		}
		else
		{
		    // general entity reference --
		    // expat allready does this, but the old parser
		    // will need this.
		    char c = '\0';
		    switch (end - start) {
		    case 2:
			if (*(start + 1) == 't') {
			    switch (*start) {
			    case 'l':
				c = '<';
				break;
			    case 'g':
				c = '>';
				break;
			    }
			}
			break;
		    case 3:
			if (*start == 'a') 
			{
			    if (*(start+1) == 'm') 
			    {
				if (*(start+2) == 'p')
				{
				    c = '&';
				}
			    }
			}
			break;
		    case 4:
			switch (*start) {
			case 'q':
			    if (*(start+1) == 'u') 
			    {
				if (*(start+2) == 'o') 
				{
				    if (*(start+3) == 't')
				    {
					c = '"';
				    }
				}
			    }
			    break;
			case 'a':
			    if (*(start+1) == 'p') 
			    {
				if (*(start+2) == 'o') 
				{
				    if (*(start+3) == 's')
				    {
					c = '"';
				    }
				}
			    }
			    break;
			}
			break;
		    }
		}
	    }
	    else
	    {
	    	*buf = *p;
	    }
	}
	break;
	default:
	    *buf = *p;
	    break;
	}
	++p;
	++buf;
    }
}

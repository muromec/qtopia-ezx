/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: safestring.cpp,v 1.8 2005/07/22 22:47:54 albertofloyd Exp $
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

#include "hxtypes.h" /* for UINT32 */

#include "safestring.h"

#include "hlxclib/stdio.h" // vsnprintf()

char* SafeStrCpy(char* pDestStr, const char* pSourceStr, UINT32 ulBufferSize)
{
    // Validate parameters and if invalid just return pDestStr like strncpy does.
    if (pDestStr && pSourceStr && ulBufferSize)
    {
        // strncpy does not NULL terminate if ulBufferSize is less than or equal to the source string length.  Always make
        // sure the string is NULL terminated.
        strncpy(pDestStr, pSourceStr, ulBufferSize); /* Flawfinder: ignore */
        pDestStr[ulBufferSize-1] = '\0';
    }

    return pDestStr;
}

char* SafeStrCat(char* pDestStr, const char* pSourceStr, UINT32 ulBufferSize)
{
    // Validate parameters and if invalid just return pDestStr like strcat does.
    if (pDestStr && pSourceStr && ulBufferSize)
    {
        // Find the end of the destination string just like strcat does
        char* pDestStrEnd = pDestStr;
        while (*pDestStrEnd)
        {
	    pDestStrEnd++;
        }

        UINT32 ulDestStrLength = (UINT32)(pDestStrEnd - pDestStr);

        // If the length of the destination string is smaller than the buffer size then we cannot concatenate anything
        if (ulDestStrLength < ulBufferSize)
        {
	    // We have room to concatenate so figure out how much of the buffer contained in the destination string we
	    // can use and copy the source string in.
	    SafeStrCpy(pDestStrEnd, pSourceStr, ulBufferSize - ulDestStrLength);
        }
    }

    return pDestStr;
}

int SafeSprintf(char* pBuffer, UINT32 ulBufferSize, const char* pFormatStr, ...)
{
    int nCharsWritten = 0;

    // Validate parameters and if invalid just return 0.
    if (pBuffer && ulBufferSize && pFormatStr)
    {
        va_list argptr;
        va_start(argptr, pFormatStr);

        nCharsWritten = vsnprintf(pBuffer, ulBufferSize, pFormatStr, argptr);
        pBuffer[ulBufferSize-1] = '\0';

        va_end(argptr);
    }

    return nCharsWritten;
}

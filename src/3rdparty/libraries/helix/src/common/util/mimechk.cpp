/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mimechk.cpp,v 1.5 2005/03/14 19:36:39 bobclark Exp $
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

/****************************************************************************
 *  Defines
 */
#define ENCRYPTED_MIME_EXTENSION	"-encrypted"
#define ENCRYPTED_MIME_EXTENSION_SIZE	(sizeof(ENCRYPTED_MIME_EXTENSION) - 1)


/****************************************************************************
 *  Includes
 */
// #include "hlxclib/stdio.h"
#include "hlxclib/string.h"

#include "hxtypes.h"
#include "hxstrutl.h"
#include "mimechk.h"


HXBOOL isProhibitedMimeType(const char* pMimeType, ULONG32 ulMimeLen)
{
    HXBOOL bProhibited = TRUE;

    // First disqualify based solely on length
    if ((ulMimeLen != 20) && (ulMimeLen != 29) && (ulMimeLen != 26) &&
	(ulMimeLen != 25) && (ulMimeLen != 23) && (ulMimeLen != 21) &&
	(ulMimeLen != 24))
    {
	return TRUE;
    }

    // Now walk the input string backwards and find out if it matches
    // any of the following acceptable mime types:
    //
    // "audio/x-pn-realaudio"		"oidualaer-np-x/oidua"
    // "application/x-shockwave-flash"	"hsalf-evawkcohs-x/noitacilppa"
    // "application/x-pn-realmedia"	"aidemlaer-np-x/noitacilppa"
    // "application/x-pn-realevent"	"tnevelaer-np-x/noitacilppa"
    // "application/x-pn-imagemap"	"pamegami-np-x/noitacilppa"
    // "application/x-pn-realad"	"dalaer-np-x/noitacilppa"
    // "syncMM/x-pn-realvideo"		"oedivlaer-np-x/MMcnys"
    // "image_map/x-pn-realvideo"	"oedivlaer-np-x/pam_egami"
    // "video/x-pn-realvideo"
    if (pMimeType[ulMimeLen - 1] == 'o')
    {
	if (pMimeType[ulMimeLen - 2] == 'e')
	{
	    if (pMimeType[0] == 's')
	    {
		// "syncMM/x-pn-realvideo"
		if (strcmp(pMimeType, "syncMM/x-pn-realvideo") == 0)
		{
		    bProhibited = FALSE;
		}
	    }
	    else if (pMimeType[0] == 'i')
	    {
		// "image_map/x-pn-realvideo"
		if (strcmp(pMimeType, "image_map/x-pn-realvideo") == 0)
		{
		    bProhibited = FALSE;
		}
	    }
	    else if (pMimeType[0] == 'v')
	    {
		// "video/x-pn-realvideo"
		if (strcmp(pMimeType, "video/x-pn-realvideo") == 0)
		{
		    bProhibited = FALSE;
		}
	    }
	}
	else if (pMimeType[ulMimeLen - 2] == 'i')
	{
	    // "audio/x-pn-realaudio"
	    if (strcmp(pMimeType, "audio/x-pn-realaudio") == 0)
	    {
		bProhibited = FALSE;
	    }
	}
    }
    else if (pMimeType[ulMimeLen - 1] == 'h')
    {
	// "application/x-shockwave-flash"
	if (strcmp(pMimeType, "application/x-shockwave-flash") == 0)
	{
	    bProhibited = FALSE;
	}
    }
    else if (pMimeType[ulMimeLen - 1] == 'a')
    {
	// "application/x-pn-realmedia"
	if (strcmp(pMimeType, "application/x-pn-realmedia") == 0)
	{
	    bProhibited = FALSE;
	}
    }
    else if (pMimeType[ulMimeLen - 1] == 't')
    {
	// "application/x-pn-realevent"
	if (strcmp(pMimeType, "application/x-pn-realevent") == 0)
	{
	    bProhibited = FALSE;
	}
    }
    else if (pMimeType[ulMimeLen - 1] == 'p')
    {
	// "application/x-pn-imagemap"
	if (strcmp(pMimeType, "application/x-pn-imagemap") == 0)
	{
	    bProhibited = FALSE;
	}
    }
    else if (pMimeType[ulMimeLen - 1] == 'd')
    {
	// "application/x-pn-realad"
	if (strcmp(pMimeType, "application/x-pn-realad") == 0)
	{
	    bProhibited = FALSE;
	}
    }

    return bProhibited;
}


int hxMimeRootCmp(const char* pMimeType1, const char* pMimeType2)
{
    int retVal;

    retVal = strcasecmp(pMimeType1, pMimeType2);

    if (retVal != 0)
    {
	ULONG32 ulLength1 = strlen(pMimeType1);
	ULONG32 ulLength2 = strlen(pMimeType2);

	if (ulLength1 == (ulLength2 + ENCRYPTED_MIME_EXTENSION_SIZE))
	{
	    if ((strcasecmp(pMimeType1 + ulLength1 - ENCRYPTED_MIME_EXTENSION_SIZE,
			    ENCRYPTED_MIME_EXTENSION) == 0) &&
		(strncasecmp(pMimeType1, pMimeType2, ulLength2) == 0))
	    {
		retVal = 0;
	    }
	}
	else if (ulLength2 == (ulLength1 + ENCRYPTED_MIME_EXTENSION_SIZE))
	{
	    if ((strcasecmp(pMimeType2 + ulLength2 - ENCRYPTED_MIME_EXTENSION_SIZE,
			    ENCRYPTED_MIME_EXTENSION) == 0) &&
		(strncasecmp(pMimeType1, pMimeType2, ulLength1) == 0))
	    {
		retVal = 0;
	    }
	}
    }

    return retVal;
}


HXBOOL decryptMimeType(CHXString &strMimeType)
{
    HXBOOL bRetVal = FALSE;	// assume no decryption
    ULONG32 ulMimeLength = strMimeType.GetLength();
	
    if (ulMimeLength > ENCRYPTED_MIME_EXTENSION_SIZE)
    {
	const char* pMimeChars = (const char*) (strMimeType);
	
	if (strcmp(ENCRYPTED_MIME_EXTENSION, 
	    &pMimeChars[ulMimeLength - ENCRYPTED_MIME_EXTENSION_SIZE]) == 0)
	{
	    ulMimeLength -= ENCRYPTED_MIME_EXTENSION_SIZE;
	    strMimeType.GetBufferSetLength(ulMimeLength);
	    bRetVal = TRUE;	// decrypted
	}
    }
    
    return bRetVal;
}


HXBOOL encryptMimeType(CHXString &strMimeType)
{
    HXBOOL bRetVal = FALSE;	// assume no encryption
    ULONG32 ulMimeLength = strMimeType.GetLength();
    const char* pMimeChars = (const char*) (strMimeType);

    if ((ulMimeLength < ENCRYPTED_MIME_EXTENSION_SIZE) ||
	(strcmp(ENCRYPTED_MIME_EXTENSION, 
		&pMimeChars[ulMimeLength - ENCRYPTED_MIME_EXTENSION_SIZE]) != 0))
    {
	strMimeType += ENCRYPTED_MIME_EXTENSION;
	bRetVal = TRUE;
    }

    return bRetVal;
}
    

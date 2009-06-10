/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: verutil.cpp,v 1.6 2008/01/18 04:54:26 vkathuria Exp $
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "hlxclib/stdlib.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxstring.h"

#include "verutil.h"


/*
 * IsBeta1Player - get 'User-Agent' request header
 * and look for string 'Version 6.0.1'
 */

HXBOOL
IsBeta1Player(IHXRequest* pRequest)
{
    HXBOOL bBeta1Player = FALSE;

    IHXValues* pRequestHeaders = NULL;
    if(HXR_OK == pRequest->GetRequestHeaders(pRequestHeaders) &&
       pRequestHeaders)
    {
	IHXBuffer* pUserAgentBuffer;
	if(HXR_OK == pRequestHeaders->GetPropertyCString("User-Agent",
	    pUserAgentBuffer))
	{
	    const char* pUserAgent = (const char*)pUserAgentBuffer->GetBuffer();
	    if(strstr(pUserAgent, "Version 6.0.1"))
	    {
		bBeta1Player = TRUE;
	    }
	    pUserAgentBuffer->Release();
	}
	pRequestHeaders->Release();
    }
    return bBeta1Player;
}

HX_RESULT
GetVersionFromString(CHXString	szVersion,
		     UINT32&	ulVersion)
{
    HX_RESULT	rc = HXR_OK;
    UINT32	i = 0;
    UINT32	j = 0;
    UINT32	ulFields = 0;
    UINT32	ulVersionInfo[4];
    CHXString	token;
    CHXString	version;

    ulVersion = 0;

    token = szVersion.NthField(' ', ++i);
    while (!token.IsEmpty())
    {
	ulFields = token.CountFields('.');
	if (4 == ulFields)
	{
	    while (ulFields && ((version = token.NthField('.', ++j)) != ""))
	    {
		version.TrimRight();
		version.TrimLeft();
		
		ulVersionInfo[--ulFields] = atoi(version);
	    }
	    break;
	}
	token = szVersion.NthField(' ', ++i);
    }
    
    ulVersion = HX_ENCODE_PROD_VERSION(ulVersionInfo[3],
				       ulVersionInfo[2],
				       ulVersionInfo[1],
				       ulVersionInfo[0]);

    return rc;
}

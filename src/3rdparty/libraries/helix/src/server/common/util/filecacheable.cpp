/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: filecacheable.cpp,v 1.4 2003/09/04 22:35:34 dcollins Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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
#include "hxcom.h"     /* IUnknown */

#include "hxresult.h"
#include "hxcomm.h"
#include "hxerror.h"  /* IHXErrorMessages */
#include "hxengin.h"  /* IHXErrorMessages */
#include "hxplugn.h"  /* IHXPlugin */
#include "ihxpckts.h"
#include "hxallow.h"
#include "hxfiles.h"
#include "hxmon.h"
#include "hxstrutl.h"

#include "filecacheable.h"

BOOL IsVBR(const char* pURL, int urllen);

#define TS_NO_CACHE_DIR	"config.NoCacheDir"

HX_RESULT IsFileCacheable(const char* pURL, IHXBuffer *pViaBuf, 
                          IHXRegistry* pRegistry)
{
    HX_RESULT theErr = HXR_OK;
    int urlLen = 0;
    int dirLen = 0;
    IHXValues *pDirList = NULL;
    IHXBuffer *pDirName = NULL;
    const char *pPropName = NULL;
    UCHAR *pBuf = NULL;
    UCHAR *pBuf2 = NULL;
    ULONG32 id;
    char *pURL2 = NULL;
    int i = 0;

    pRegistry->AddRef();

    INT32 numProps = pRegistry->GetNumPropsByName (TS_NO_CACHE_DIR);

    urlLen = strlen (pURL);
    pURL2 = new char [ urlLen + 1 ] ;
    strcpy (pURL2, pURL);
    for (i=0; i < urlLen; ++i)
    {   //normalize the slashes so string compares work
        if (pURL2[i] == '\\') pURL2[i] = '/';
    }

    if (numProps)
    {
        theErr = pRegistry->GetPropListByName (TS_NO_CACHE_DIR, pDirList);
    }

    if (numProps && SUCCEEDED (theErr) && pDirList != NULL)
    {
	theErr = pDirList->GetFirstPropertyULONG32 (pPropName, id);
	if (SUCCEEDED(theErr))
	{
	    theErr = pRegistry->GetStrById (id, pDirName);
	}

	while (SUCCEEDED (theErr) && pDirName != NULL)
	{
	    pBuf = pDirName->GetBuffer();

	    if (pBuf != NULL)
	    {   pBuf2 = pBuf;
	        while (*pBuf2)
		{   //normalize the slashes so string compares work
		    if (*pBuf2 == '\\') *pBuf2 = '/';
		    pBuf2++;
		}
	    }

            if(pBuf && strcmp((CHAR*)pBuf, "/") == 0)
            {
                theErr = HXR_NOT_AUTHORIZED;
            }

            if (SUCCEEDED (theErr))
            {
                while (*pBuf && *pBuf == '/') pBuf++;
	        if (pBuf != NULL && *pBuf != '\0')
	        {
		    dirLen = strlen ((CHAR*)pBuf);
		    if (dirLen <= urlLen)
		    {
		        if (strnicmp ((CHAR*)pBuf, pURL2, dirLen) == 0)
		        {
			    theErr = HXR_NOT_AUTHORIZED;
		        }
		    }
	        }
            }

	    HX_RELEASE (pDirName);

	    // get the next one if we haven't matched yet.
	    if (SUCCEEDED (theErr))
	    {
		theErr = pDirList->GetNextPropertyULONG32 (pPropName, id);
	    }

	    if (SUCCEEDED (theErr))
	    {
		theErr = pRegistry->GetStrById (id, pDirName);
	    }
	}

	HX_RELEASE (pDirList);
    }

    /* Hack Alert! The 8.x proxy works passing the new ".rmvb" requests
     * in the simple case but kills the client when the unsupported 
     * datatype is accessed inside a smil file because the server core 
     * sends a server alert. So we need to look for legacy proxies if 
     * this is variable bit rate content and force to pass through if 
     * found.
     */
    if (theErr != HXR_NOT_AUTHORIZED)
    {
        if (pURL2 && pViaBuf && IsVBR(pURL2, urlLen))
        {
            UINT32 vialen = pViaBuf->GetSize();
            const char* pszVia = (const char*)pViaBuf->GetBuffer();

            if ((vialen > 19) && StrNStr(pszVia, "RealProxy Version 8", vialen, 19))
            {
                // theres an 8.x RealSystem Proxy in the chain
                theErr = HXR_NOT_AUTHORIZED;
            }
            else if (vialen > 19)
            {
                // look for older (non-spec compliant) RealProxy Via entries
                UINT32 pos = 0;
                const char*  pcur = pszVia;

                while (pos < vialen)
                {
                    const char* pbegin;
                    const char* pend;

                    // Trim leading WS
                    while (pos < vialen && isspace(*pcur))
                    {
                        pcur++;
                        pos++;
                    }
                    pbegin = pcur;

                    // Find separator
                    BOOL bInQuote = FALSE;
                    while (pos < vialen && (*pcur != ',' || bInQuote))
                    {
                        if (*pcur == '"')
                        {
                            bInQuote = !bInQuote;
                        }
                        pcur++;
                        pos++;
                    }
                    pend = pcur;
                    pcur++;
                    pos++;

                    // Trim trailing WS
                    while (pend > pbegin && isspace(*(pend-1)))
                    {
                        pend--;
                    }

                    // Format example: "30/Nov/2002:00:10:15:05"
                    if ((pend - pbegin) == 20)
                    {
                        if ((pbegin[2] == '/') && (pbegin[6] == '/') &&
                            (pbegin[11] == ':') && (pbegin[14] == ':') &&
                            (pbegin[17] == ':'))
                        {
                            theErr = HXR_NOT_AUTHORIZED;
                            break;
                        }
                    }

                }
            }
        }
    }

    HX_RELEASE (pRegistry);

    if (pURL2) delete [] pURL2;

    // return one of two values:
    if (theErr != HXR_NOT_AUTHORIZED) theErr = HXR_OK;

    return theErr;
}

BOOL IsVBR(const char* pURL, int urllen)
{
    const char* pend = pURL+urllen;
    const char* pcur = pURL;
    const char* ptemp = pURL;
    const char* pext = NULL;

    /*
     * figure out which server we are using
     */
    if (urllen > 6 && !memcmp(pcur, "pnm://", 6))
    {
        pcur += 6;
        urllen -= 6;

        ptemp = (char*)memchr((const void*)pcur, '/', urllen);
        if (ptemp)
        {
            urllen = urllen - (ptemp - pcur);
            pcur = ptemp;
        }
    }
    else if(urllen > 7 && !strncasecmp(pcur, "rtsp://", 7))
    {
        pcur += 7;
        urllen -= 7;

        ptemp = (char*)memchr((const void*)pcur, '/', urllen);
        if (ptemp)
        {
            urllen = urllen - (ptemp - pcur);
            pcur = ptemp;
        }
    }
    
    /*
     * Separate parameters if any
     */
    pend = (char*)memchr((const void*)pcur, '?', urllen);
    if (pend)
    {
        urllen = pend-pcur;
    }

    // If there is no path length after separating the parameters, 
    // then the URL was JUST parameters, and is not valid!
    if(!urllen)
    {
	return FALSE;
    }

    /*
     * Get segment if any
     */
    pend = (char*)memchr((const void*)pcur, '=', urllen);
    if (pend)
    {
	// find beginning of segment
	const char* pseg = pend;
	while(pseg != pcur)
	{
	    pseg--;
	    if(*pseg == '/')
	    {
		urllen = pseg - pcur;
                break;
	    }
	}
    }
    else
    {
	if(pcur[urllen-1] == '/')
	{
            urllen--;
	}
    }

    // return fail if there is nothing left after separating the segment, 
    if(!urllen)
    {
	return FALSE;
    }

    pend = pcur + urllen - 1;
    while (pend > pcur)
    {
	if (*pend == '.')
        {
	    pext = pend + 1;
            break;
        }

	pend--;
    }

    pend = pcur + urllen;
    if (pext && ((pend-pext) == 4) && !memcmp(pext, "rmvb", 4))
    {
        return TRUE;
    }

    return FALSE;
}


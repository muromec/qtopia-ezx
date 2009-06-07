/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: clone_values.cpp,v 1.3 2004/07/09 18:23:51 hubbe Exp $
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
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxcomm.h"
#include "clone_values.h"

HX_RESULT CloneIHXValues(IHXValues*             pSrc,
                         IHXCommonClassFactory* pCCF,
                         REF(IHXValues*)        rpDst)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pSrc && pCCF)
    {
        // Create an IHXValues
        IHXValues* pDst = NULL;
        retVal = pCCF->CreateInstance(CLSID_IHXValues, (void**) &pDst);
        if (SUCCEEDED(retVal))
        {
            // Copy the ULONG32 values
            const char* pszName = NULL;
            ULONG32     ulValue = 0;
            HX_RESULT   rv      = pSrc->GetFirstPropertyULONG32(pszName, ulValue);
            while (SUCCEEDED(rv) && SUCCEEDED(retVal))
            {
                retVal = pDst->SetPropertyULONG32(pszName, ulValue);
                if (SUCCEEDED(retVal))
                {
                    rv = pSrc->GetNextPropertyULONG32(pszName, ulValue);
                }
            }
            if (SUCCEEDED(retVal))
            {
                // Copy the buffer properties
                IHXBuffer* pSrcBuf = NULL;
                rv = pSrc->GetFirstPropertyBuffer(pszName, pSrcBuf);
                while (SUCCEEDED(rv) && SUCCEEDED(retVal))
                {
                    retVal = CloneAndSetIHXBuffer(pSrcBuf, pCCF, pDst, pszName,
                                                  IHXVALUES_PROP_TYPE_BUFFER);
                    if (SUCCEEDED(retVal))
                    {
                        HX_RELEASE(pSrcBuf);
                        rv = pSrc->GetNextPropertyBuffer(pszName, pSrcBuf);
                    }
                }
                HX_RELEASE(pSrcBuf);
                if (SUCCEEDED(retVal))
                {
                    // Copy the CString properties
                    rv = pSrc->GetFirstPropertyCString(pszName, pSrcBuf);
                    while (SUCCEEDED(rv) && SUCCEEDED(retVal))
                    {
                        retVal = CloneAndSetIHXBuffer(pSrcBuf, pCCF, pDst, pszName,
                                                      IHXVALUES_PROP_TYPE_CSTRING);
                        if (SUCCEEDED(retVal))
                        {
                            HX_RELEASE(pSrcBuf);
                            rv = pSrc->GetNextPropertyBuffer(pszName, pSrcBuf);
                        }
                    }
                    HX_RELEASE(pSrcBuf);
                    if (SUCCEEDED(retVal))
                    {
                        // Copy the out parameter
                        HX_RELEASE(rpDst);
                        rpDst = pDst;
                        rpDst->AddRef();
                    }
                }
            }
        }
        HX_RELEASE(pDst);
    }

    return retVal;
}

HX_RESULT CloneAndSetIHXBuffer(IHXBuffer*             pSrc,
                               IHXCommonClassFactory* pCCF,
                               IHXValues*             pValues,
                               const char*            pszName,
                               UINT32                 ulType)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pSrc && pCCF && pValues && pszName &&
        (ulType == IHXVALUES_PROP_TYPE_BUFFER ||
         ulType == IHXVALUES_PROP_TYPE_CSTRING))
    {
        // Clone the IHXBuffer
        IHXBuffer* pDst = NULL;
        retVal = CloneIHXBuffer(pSrc, pCCF, pDst);
        if (SUCCEEDED(retVal))
        {
            // Set the cloned buffer into the IHXValues
            if (ulType == IHXVALUES_PROP_TYPE_BUFFER)
            {
                retVal = pValues->SetPropertyBuffer(pszName, pDst);
            }
            else
            {
                retVal = pValues->SetPropertyCString(pszName, pDst);
            }
        }
        HX_RELEASE(pDst);
    }

    return retVal;
}

HX_RESULT CloneIHXBuffer(IHXBuffer*             pSrc,
                         IHXCommonClassFactory* pCCF,
                         REF(IHXBuffer*)        rpDst)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pSrc && pCCF)
    {
        // Create an IHXBuffer
        IHXBuffer* pDst = NULL;
        retVal = pCCF->CreateInstance(CLSID_IHXBuffer, (void**) &pDst);
        if (SUCCEEDED(retVal))
        {
            // Copy the buffer
            retVal = pDst->Set(pSrc->GetBuffer(), pSrc->GetSize());
            if (SUCCEEDED(retVal))
            {
                // Assign the out parameter
                HX_RELEASE(rpDst);
                rpDst = pDst;
                rpDst->AddRef();
            }
        }
        HX_RELEASE(pDst);
    }

    return retVal;
}


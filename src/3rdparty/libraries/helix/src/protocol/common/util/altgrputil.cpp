/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: altgrputil.cpp,v 1.5 2006/12/15 04:46:32 gbajaj Exp $
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
#include "altgrputil.h"

#include "altgrpselect.h"
#include "langaltfilt.h"
#include "maxbwaltfilt.h"
#include "highbwaltfilt.h"
#include "mimealtfilt.h"

#include "hxprefs.h"
#include "hxprefutil.h"
#include "hxengin.h"

HX_RESULT 
CHXAltGroupUtil::SelectAltGroups(IUnknown* pContext,
                                 UINT16 nValues, IHXValues** ppValues)
{
    HX_RESULT res = HXR_OK;
    
    if (pContext && ppValues && nValues)
    {
        ULONG32 ulMaxBW = 0;
        CHXString languages;
        IHXPreferences* pPrefs = NULL;
        
        if (HXR_OK == pContext->QueryInterface(IID_IHXPreferences, (void**)&pPrefs))
        {
            ReadPrefUINT32(pPrefs, "MaxBandwidth", ulMaxBW);
            ReadPrefCSTRING(pPrefs, "Language", languages);

            res = SelectAltGroups(pContext, ulMaxBW, languages, nValues,
                                  ppValues);
        }
        HX_RELEASE(pPrefs);
    }

    return res;
}

HX_RESULT 
CHXAltGroupUtil::SelectAltGroups(IUnknown* pContext,
                                 ULONG32 ulMaxBW, const char* pLanguages,
                                 UINT16 nValues, IHXValues** ppValues)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;
    
    if (ulMaxBW == 0 && pContext)
    {
        IHXConnectionBWInfo* pConnBWInfo = NULL;
        if (HXR_OK == pContext->QueryInterface(IID_IHXConnectionBWInfo, (void**)&pConnBWInfo))
        {
            pConnBWInfo->GetConnectionBW(ulMaxBW, FALSE);
            HX_RELEASE(pConnBWInfo);
        }
    }

    if (pContext && ppValues && nValues)
    {
        IHXValues** ppNewValues = new IHXValues*[nValues];
        
        if (ppNewValues)
        {
            CHXAltGroupSelector selector;
            
            res = selector.Init(pContext);
            
            if (HXR_OK == res)
            {
                res = AddMimetypeFilter(selector, pContext);
            }

            if ((HXR_OK == res) && pLanguages && strlen(pLanguages))
            {
                res = AddLanguageFilter(selector, pLanguages);
            }
                
            if ((HXR_OK == res) && ulMaxBW)
            {
                res = AddMaxBWFilter(selector, ulMaxBW);
            }
                
            if (HXR_OK == res)
            {
                res = AddHighBWFilter(selector);
            }
                
            if (HXR_OK == res)
            {
                res = selector.Select(nValues, ppValues, ppNewValues);
                    
                if (HXR_OK == res)
                {
                    for (UINT16 i = 0; i < nValues; i++)
                    {
                        // Release reference to old header
                        HX_RELEASE(ppValues[i]);
                        
                        // Transfer ownership of new header
                        ppValues[i] = ppNewValues[i];
                        ppNewValues[i] = NULL;
                    }
                }
            }
            HX_VECTOR_DELETE(ppNewValues);
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }

    return res;
}

HX_RESULT 
CHXAltGroupUtil::AddLanguageFilter(CHXAltGroupSelector& selector,
                                   const char* pLanguages)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pLanguages)
    {
        CHXLanguageAltIDFilter* pLangFilt = new CHXLanguageAltIDFilter;
    
        if (pLangFilt)
        {
            res = pLangFilt->Init(pLanguages);
            
            if (HXR_OK == res)
            {
                res = selector.AddFilter(pLangFilt);
                
                if (HXR_OK == res)
                {
                    // clear pointer since ownership was transfered
                    pLangFilt = NULL;
                }
            }
                HX_DELETE(pLangFilt);
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }

    return res;
}

HX_RESULT 
CHXAltGroupUtil::AddMaxBWFilter(CHXAltGroupSelector& selector,
                                UINT32 uMaxBW)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (uMaxBW)
    {
        CHXMaxBWAltIDFilter* pMaxBWFilt = new CHXMaxBWAltIDFilter;
        
        if (pMaxBWFilt)
        {
            res = pMaxBWFilt->Init(uMaxBW);
            
            if (HXR_OK == res)
            {
                res = selector.AddFilter(pMaxBWFilt);

                if (HXR_OK == res)
                {
                    // clear pointer since ownership was transfered
                    pMaxBWFilt = NULL;
                }
            }
            
            HX_DELETE(pMaxBWFilt);
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }

    return res;
}

HX_RESULT 
CHXAltGroupUtil::AddHighBWFilter(CHXAltGroupSelector& selector)
{
    HX_RESULT res = HXR_OUTOFMEMORY;

    CHXHighestBWAltIDFilter* pHighBWFilt = new CHXHighestBWAltIDFilter;
    
    if (pHighBWFilt)
    {
        res = selector.AddFilter(pHighBWFilt);
        
        if (HXR_OK == res)
        {
            // clear pointer since ownership was transfered
            pHighBWFilt = NULL;
        }
        HX_DELETE(pHighBWFilt);
    }

    return res;
}

HX_RESULT 
CHXAltGroupUtil::AddMimetypeFilter(CHXAltGroupSelector& selector,
                                   IUnknown* pContext)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pContext)
    {
        CHXMimetypeAltIDFilter* pMimetypeFilt = new CHXMimetypeAltIDFilter;
        
        if (pMimetypeFilt)
        {
            res = pMimetypeFilt->Init(pContext);
            
            if (HXR_OK == res)
            {
                res = selector.AddFilter(pMimetypeFilt);

                if (HXR_OK == res)
                {
                    // clear pointer since ownership was transfered
                    pMimetypeFilt = NULL;
                }
            }
            
            HX_DELETE(pMimetypeFilt);
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }

    return res;
}

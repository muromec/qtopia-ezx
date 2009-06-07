/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mimealtfilt.cpp,v 1.3 2007/02/07 18:11:11 cybette Exp $
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
#include "mimealtfilt.h"

#include "altidset.h"
#include "altidmap.h"
#include "hxval2util.h"
#include "ihxpckts.h"
#include "pckunpck.h"

CHXMimetypeAltIDFilter::CHXMimetypeAltIDFilter() :
    m_pPluginHdlr(NULL),
    m_pContext(NULL)
{}

CHXMimetypeAltIDFilter::~CHXMimetypeAltIDFilter()
{
    HX_RELEASE(m_pPluginHdlr);
    HX_RELEASE(m_pContext);
}

HX_RESULT 
CHXMimetypeAltIDFilter::Init(IUnknown* pContext)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pContext)
    {
        m_pContext = pContext;
        HX_ADDREF(m_pContext);

        res = pContext->QueryInterface(IID_IHXPlugin2Handler,
                                       (void**)&m_pPluginHdlr);
    }

    return res;
}

HX_RESULT 
CHXMimetypeAltIDFilter::Filter(UINT32 uHdrCount, IHXValues** pInHdrs,
                               const CHXAltIDMap& altIDMap,
                               CHXAltIDSet& /* in/out */ altIDSet)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if ((uHdrCount > 1) && pInHdrs)
    {
        CHXAltIDSet newIDSet;

        res = HXR_OK;
        for (UINT32 i = 0; (HXR_OK == res) && (i < altIDSet.GetCount()); i++)
        {
            UINT32 uAltID;

            res = altIDSet.GetAltID(i, uAltID);

            if (HXR_OK == res)
            {
                UINT32 uStreamID;
                res = altIDMap.GetStreamID(uAltID, uStreamID);

                if (HXR_OK == res)
                {
                    UINT32 uHdrIndex = uStreamID + 1;
                    if (uHdrIndex < uHdrCount)
                    {
                        HXBOOL bSupportedMimetype = FALSE;
                        
                        res = isSupported(uAltID, pInHdrs[uHdrIndex],
                                          bSupportedMimetype);
                        
                        if ((HXR_OK == res) && bSupportedMimetype)
                        {
                            newIDSet.AddID(uAltID);
                        }
                    }
                    else
                    {
                        res = HXR_UNEXPECTED;
                    }
                }
            }
        }
   
        if (HXR_OK == res)
        {
            res = altIDSet.Copy(newIDSet);
        }
    }

    return res;
}

HX_RESULT 
CHXMimetypeAltIDFilter::isSupported(UINT32 uAltID,
                                    IHXValues* pStreamHdr,
                                    HXBOOL& bIsSupported) const
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pStreamHdr)
    {
        IHXBuffer* pMimetype = NULL;

        res = pStreamHdr->GetPropertyCString("MimeType", pMimetype);

        if (HXR_OK == res)
        {
            ULONG32 ulDefaultID;

            if ((HXR_OK != pStreamHdr->GetPropertyULONG32("alt-default-id",
                                                          ulDefaultID)) ||
                (ulDefaultID != uAltID))
            {
                IHXValues2* pAlt = NULL;
                res = CHXIHXValues2Util::GetIHXValues2Property(pStreamHdr,
                                                               "Alt",
                                                               pAlt);
                if (HXR_OK == res)
                {
                    CHXString key;
                    key.AppendULONG(uAltID);

                    IHXValues2* pAltInfo = NULL;
                    res = CHXIHXValues2Util::GetIHXValues2Property(pAlt,
                                                                   key,
                                                                   pAltInfo);

                    if (HXR_OK == res)
                    {
                        IHXBuffer* pAltMimetype = NULL;

                        if (HXR_OK == pAltInfo->GetPropertyCString("MimeType",
                                                                   pAltMimetype))
                        {
                            HX_RELEASE(pMimetype);
                            pMimetype = pAltMimetype;
                            pMimetype->AddRef();
                        }
                        HX_RELEASE(pAltMimetype);
                    }
                    HX_RELEASE(pAltInfo);
                }
                HX_RELEASE(pAlt);
            }
        }

        if (HXR_OK == res)
        {
            res = isMimetypeSupported((const char*)pMimetype->GetBuffer(),
                                      bIsSupported);
        }

        HX_RELEASE(pMimetype);
    }

    return res;
}

HX_RESULT 
CHXMimetypeAltIDFilter::isMimetypeSupported(const char* pMimetype,
                                            HXBOOL& bIsSupported) const
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (!m_pPluginHdlr)
    {
        res = HXR_UNEXPECTED;
    }
    else if (pMimetype)
    {
        IHXValues* pValues = NULL;
        res = CreateValuesCCF(pValues, m_pContext);
        if (SUCCEEDED(res))
        {
            res = SetCStringPropertyCCF(pValues, PLUGIN_RENDERER_MIME,
                                        pMimetype, m_pContext);

            if (SUCCEEDED(res))
            {
                IUnknown* pUnk = NULL;
                res = m_pPluginHdlr->FindPluginUsingValues(pValues, pUnk);
                if (SUCCEEDED(res))
                {
                    bIsSupported = TRUE;
                }
                else
                {
                    bIsSupported = FALSE;
                }
                HX_RELEASE(pUnk);
            }

            HX_RELEASE(pValues);
        }

        res = HXR_OK;
    }

    return res;
}

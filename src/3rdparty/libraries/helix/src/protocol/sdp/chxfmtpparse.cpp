/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxfmtpparse.cpp,v 1.9 2005/03/10 20:59:19 bobclark Exp $
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

#include "chxfmtpparse.h"
#include "chxcharstack.h"
#include "hlxclib/string.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/errno.h"
#include "hlxclib/limits.h"

static const char FMTP_PREFIX[] = "FMTP";

CHXFMTPParser::CHXFMTPParser(IUnknown* pUnk) :
    m_pCCF(0)
{
    if (pUnk)
    {
        pUnk->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCCF);
    }
}

CHXFMTPParser::~CHXFMTPParser()
{
    HX_RELEASE(m_pCCF);
}

HX_RESULT CHXFMTPParser::Parse(const char* pFMTPStr, IHXValues* pHeaders)
{
    HX_RESULT res = HXR_UNEXPECTED;

    // e.g. a=fmtp:101 emphasis=50/15;foo=bar;flag
    
    if (m_pCCF)
    {
        const char* pCur = pFMTPStr;
            
        // Skip payload type
        //for (;*pCur && (*pCur != ' '); pCur++)
        //;
        
        res = HXR_OK;
        
        IHXBuffer* pFieldName = 0;
        IHXBuffer* pFieldValue = 0;
        UINT32 ulState = 0;
        const char* pDelims = 0;
        while ((HXR_OK == res) && *pCur)
        {
            // Skip whitespace
            for (;*pCur && (*pCur == ' '); pCur++)
                ;
            
            HXBOOL bCollectValue = FALSE;
            HXBOOL bAddValue = FALSE;
            IHXBuffer** ppTokDest = 0;
            switch (ulState) {
            case 0:
                pDelims = " ;=";
                if (strchr(pDelims, *pCur))
                {
                    pCur++; // Skip delimiter
                }
                else
                {
                    ulState = 1;
                    bCollectValue = TRUE;
                    ppTokDest = &pFieldName;
                }
                break;
            case 1:
                if (*pCur == '=')
                {
                    pCur++; // Skip delimiter
                    pDelims = " ;";
                    ulState = 2;
                }
                else
                {
                    bAddValue = TRUE;

                    if (strchr(pDelims, *pCur))
                        pCur++; // Skip delimiter
                }               
                break;
            case 2:
                ppTokDest = &pFieldValue;
                bCollectValue = TRUE;
                ulState = 3;
                break;
            case 3:
                if (*pCur && strchr(pDelims, *pCur))
                    pCur++; // Skip delimiter

                bAddValue = TRUE;
                break;
            }

            if (bCollectValue)
            {
                res = CollectToken(pCur, pDelims, ppTokDest, bAddValue);
            }

            if (bAddValue)
            {
                res = AddParam(pFieldName, pFieldValue, pHeaders);
                HX_RELEASE(pFieldName);
                HX_RELEASE(pFieldValue);

                ulState = 0;
            }
        }
        
        if (HXR_OK == res)
        {
            // Add last parameter
            res = AddParam(pFieldName, pFieldValue, pHeaders);
        }

        HX_RELEASE(pFieldName);
        HX_RELEASE(pFieldValue);
    }

    return res;
}

HX_RESULT CHXFMTPParser::CollectToken(const char*& pBuf, const char* pDelims,
                                      IHXBuffer** ppTokDest, HXBOOL& bAddValue)
{
    HX_RESULT res = HXR_OK;

    CHXCharStack tok(m_pCCF);
    
    // Collect value
    while((HXR_OK == res) && *pBuf && !strchr(pDelims, *pBuf))
    {
        res = tok.AddChar(*pBuf++);
    }
    
    
    IHXBuffer* pTok = 0;
    
    if ((HXR_OK == res) && 
        (HXR_OK == (res = tok.Finish(pTok))))
    {
        // Make sure we dont have an empty string
        if (pTok->GetSize() > 1)
        {
            *ppTokDest = pTok;
        }
        else
        {
            bAddValue = TRUE;
            HX_RELEASE(pTok);
        }
    }

    return res;
}

HX_RESULT CHXFMTPParser::AddParam(IHXBuffer* pFieldName, 
                                  IHXBuffer* pFieldValue,
                                  IHXValues* pHeaders)
{
    HX_RESULT res = HXR_OK;

    if (pFieldName)
    {
        IHXBuffer* pParamName = 0;
        
        res = ContructParamName(pFieldName, pParamName);

        if (HXR_OK == res)
        {
            char* pNameStr = (char*)pParamName->GetBuffer();
            if (pFieldValue)
            {
                ULONG32 ulValue;

                if (HXR_OK == ConvertToULONG32(pFieldValue, ulValue))
                {
                    // Insert as a numeric field
                    res = pHeaders->SetPropertyULONG32(pNameStr, ulValue);
                }
                else
                {
                    res = pHeaders->SetPropertyCString(pNameStr, pFieldValue);
                }
            }
            else
            {
                // Assume this is a flag field
                res = pHeaders->SetPropertyULONG32(pNameStr, 1);
            }
        }

	HX_RELEASE(pParamName);
    }

    return res;
}

HX_RESULT CHXFMTPParser::ContructParamName(IHXBuffer* pFieldName, 
                                           REF(IHXBuffer*) pParamName)
{
    HX_RESULT res = HXR_UNEXPECTED;
    pParamName = 0;

    ULONG32 ulSize = 
        strlen((char*)pFieldName->GetBuffer()) + strlen(FMTP_PREFIX) + 1;

    res = m_pCCF->CreateInstance(IID_IHXBuffer, (void**)&pParamName);

    if ((HXR_OK == res) &&
        (HXR_OK == (res = pParamName->SetSize(ulSize))))
    {
        // Construct parameter name. This will be something
        // like FMTPconfig
        strcpy((char*)pParamName->GetBuffer(), FMTP_PREFIX);
        strcat((char*)pParamName->GetBuffer(), (char*)pFieldName->GetBuffer());
    }

    return res;
}

HX_RESULT CHXFMTPParser::ConvertToULONG32(IHXBuffer* pValue, 
                                          REF(ULONG32) ulValue)
{
    HX_RESULT res = HXR_FAILED;

    char* pValueStr = (char*)pValue->GetBuffer();
    char* pEnd = 0;
    ulValue = ::strtoul(pValueStr, &pEnd, 10);

    if (*pValueStr && (*pEnd == '\0') && 
        ((ulValue != ULONG_MAX) || (errno != ERANGE)))
    {
        res = HXR_OK;
    }

    return res;
}

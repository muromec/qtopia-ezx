/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: adapthdr.cpp,v 1.7 2007/07/06 20:51:35 jfinnecy Exp $
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
#include "adapthdr.h"

#include "hlxclib/string.h"
#include "pckunpck.h"

const char* const CHXAdaptationHeader::zm_pDefaultRequiredParams[] = {"url", 
                                                                NULL};

const char* const CHXAdaptationHeader::zm_pDefaultParamOrder[] = {"url", 
                                                            NULL};

CHXAdaptationHeader::CHXAdaptationHeader() :
    m_pCCF(NULL),
    m_pValues(NULL)
{}

CHXAdaptationHeader::~CHXAdaptationHeader()
{
    HX_RELEASE(m_pCCF);
    HX_RELEASE(m_pValues);
}

bool CHXAdaptationHeader::operator==(const CHXAdaptationHeader& rhs)
{
    return (CompareIHXValues(m_pValues, rhs.m_pValues) &&
            CompareIHXValues(rhs.m_pValues, m_pValues));
}

HX_RESULT CHXAdaptationHeader::Init(IUnknown* pContext)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    HX_RELEASE(m_pCCF);

    if (pContext)
    {
        res = pContext->QueryInterface(IID_IHXCommonClassFactory,
                                       (void**)&m_pCCF);
    }

    return res;
}

HX_RESULT 
CHXAdaptationHeader::Parse(const char* pBuf, UINT32 ulSize)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (!pBuf) // minimum valid string is url=""
    {
        res = HXR_INVALID_PARAMETER;
    }
    else if (!m_pCCF)
    {
        res = HXR_UNEXPECTED;
    }
    else
    {
        const char* pEnd = pBuf + ulSize;

        HX_RELEASE(m_pValues);

        res = GetURL(pBuf, pEnd);

        if (HXR_OK == res)
        {
            while((HXR_OK == res) && (pBuf != pEnd))
            {
                res = GetAdaptParam(pBuf, pEnd);
            }
        }

        if ((HXR_OK == res) && !AreValuesValid(m_pValues))
        {
            res = HXR_INVALID_PARAMETER;
        }
        
        if (HXR_OK != res)
        {
            HX_RELEASE(m_pValues);
        }
    }

    return res;
}

HX_RESULT 
CHXAdaptationHeader::GetValues(REF(IHXValues*) pValues)
{
    HX_RESULT res = HXR_FAILED;

    if (m_pValues)
    {
        pValues = m_pValues;
        pValues->AddRef();
        res = HXR_OK;
    }
    
    return res;
}

HX_RESULT CHXAdaptationHeader::SetValues(IHXValues* pValues)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pValues && AreValuesValid(pValues))
    {
        HX_RELEASE(m_pValues);
        m_pValues = pValues;
        m_pValues->AddRef();
        
        res = HXR_OK;
    }

    return res;
}

HX_RESULT CHXAdaptationHeader::GetString(REF(IHXBuffer*) pBuf)
{
    HX_RESULT res = HXR_UNEXPECTED;

    if (m_pCCF && m_pValues)
    {
        res = CreateBufferCCF(pBuf, m_pCCF);

        const char* const* pParamOrder = paramOrder();

        // Add all the ordered parameters first
        for (; (HXR_OK == res) && *pParamOrder; pParamOrder++)
        {
            const char* pParamName = *pParamOrder;

            if (IsIntParam(pParamName))
            {
                ULONG32 ulValue;
                if (HXR_OK == m_pValues->GetPropertyULONG32(pParamName, 
                                                            ulValue))
                {
                    res = AddULONG32Param(pBuf, pParamName, ulValue);
                }
            }
            else if (IsStringParam(pParamName))
            {
                IHXBuffer* pValue = NULL;

                if (HXR_OK == m_pValues->GetPropertyCString(pParamName,
                                                            pValue))
                {
                    if (!strcasecmp(pParamName, "url"))
                    {
                        res = AddURL(pBuf, pValue);
                    }
                    else
                    {
                        res = AddStringParam(pBuf, pParamName, pValue);
                    }

                    HX_RELEASE(pValue);
                }
            }
            else
            {
                // This shouldn't happen. If it does, then it means that
                // a parameter is in the paramOrder() list, but is not in
                // the intParams() or stringParams() lists.
                res = HXR_UNEXPECTED;
            }
        }

        if (HXR_OK == res)
        {
            // Add ULONG32 properties
            const char* pKey = NULL;
            ULONG32 ulValue;
            HX_RESULT tmpRes = m_pValues->GetFirstPropertyULONG32(pKey, 
                                                                  ulValue);

            while((HXR_OK == res) && (HXR_OK == tmpRes))
            {
                if (!InKeyList(pKey, paramOrder()))
                {
                    // Only add parameters that are not in the paramOrder()
                    // list, because those parameters were added in the
                    // code above.
                    res = AddULONG32Param(pBuf, pKey, ulValue);
                }

                tmpRes = m_pValues->GetNextPropertyULONG32(pKey, ulValue);
            }
        }

        if (HXR_OK == res)
        {
            // Add CString properties
            const char* pKey = NULL;
            IHXBuffer* pValue = NULL;
            HX_RESULT tmpRes = m_pValues->GetFirstPropertyCString(pKey, 
                                                                  pValue);

            while((HXR_OK == res) && (HXR_OK == tmpRes))
            {
                if (!InKeyList(pKey, paramOrder()))
                {
                    // Only add parameters that are not in the paramOrder()
                    // list, because those parameters were added in the
                    // code above.
                    res = AddStringParam(pBuf, pKey, pValue);
                }

                HX_RELEASE(pValue);
                tmpRes = m_pValues->GetNextPropertyCString(pKey, pValue);
            }

            HX_RELEASE(pValue);
        }

        if (HXR_OK != res)
        {
            HX_RELEASE(pBuf);
        }
    }

    HX_ASSERT(HXR_OK == res);
    return res;
}

const char* const* 
CHXAdaptationHeader::requiredParams() const
{
    return zm_pDefaultRequiredParams;
}

const char* const* 
CHXAdaptationHeader::paramOrder() const
{
    return zm_pDefaultParamOrder;
}

HX_RESULT 
CHXAdaptationHeader::GetURL(REF(const char*) pCur, const char* pEnd)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pCur && (pCur < pEnd) && !strncmp("url=\"", pCur, 5))
    {
        pCur += 5; // skip url="

        const char* pURLStart = pCur;
                
        // Search for the end quote
        for(;(pCur < pEnd) && *pCur && (*pCur != '\"'); pCur++)
            ;

        if (*pCur == '\"')
        {
            // We found the end quote
            UINT32 uURLSize = (UINT32)(pCur - pURLStart);

            if ((uURLSize > 0) &&
                (HXR_OK == m_pCCF->CreateInstance(CLSID_IHXValues, 
                                                  (void**)&m_pValues)))
            {
                res = SetCStringPropertyCCFWithNullTerm(m_pValues,
                                                        "url",
                                                        (BYTE*)pURLStart,
                                                        uURLSize,
                                                        m_pCCF);
            }

            if (HXR_OK != res)
            {
                HX_RELEASE(m_pValues);
            }
            
            pCur++; // skip '\"'
        }
    }

    return res;
}

HX_RESULT 
CHXAdaptationHeader::GetAdaptParam(REF(const char*) pCur, const char* pEnd)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pCur && (pCur < pEnd) && (*pCur == ';'))
    {
        pCur++; // skip ';'

        const char* pNameStart = pCur;

        // Search for the '='
        for(;(pCur < pEnd) && *pCur && (*pCur != '='); pCur++)
            ;

        if (*pCur == '=')
        {
            // We found the '='
            UINT32 uNameSize = (UINT32)(pCur - pNameStart);
            char* pName = new char[1 + uNameSize];
                        
            if (pName)
            {
                // Copy name
                memcpy(pName, pNameStart, uNameSize);

                // null terminate
                pName[uNameSize] = '\0';

                pCur++; // skip '='

                if (IsIntParam(pName))
                {
                    res = GetIntParam(pName, pCur, pEnd);
                }
                else
                {
                    res = GetStringParam(pName, pCur, pEnd);
                }

                HX_VECTOR_DELETE(pName);
            }
            else
            {
                res = HXR_OUTOFMEMORY;
            }
        }
    }

    return res;
}

HX_RESULT CHXAdaptationHeader::GetIntParam(const char* pName, 
                                            REF(const char*) pCur, 
                                            const char* pEnd)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pName && pCur && (pCur < pEnd) && (*pCur >= '0') && (*pCur <= '9'))
    {
        const char* pIntStart = pCur;

        // Search for the end of the string or a ';' and make
        // sure that all the characters are digits
        for (;(pCur < pEnd) && *pCur && (*pCur != ';') && 
                 (*pCur >= '0') && (*pCur <= '9'); pCur++)
            ;

        if (!*pCur || (*pCur == ';'))
        {
            UINT32 uDigitCount = (UINT32)(pCur - pIntStart);

            if (uDigitCount <= 9)
            {
                char* pEnd = NULL;
                ULONG32 uTmp = strtoul(pIntStart, &pEnd, 10);

                if ((pEnd == pCur)  && m_pValues)
                {
                    res = m_pValues->SetPropertyULONG32(pName, uTmp);
                }
            }
        }
    }

    return res;
}

HX_RESULT CHXAdaptationHeader::GetStringParam(const char* pName, 
                                               REF(const char*) pCur, 
                                               const char* pEnd)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pName && pCur && (pCur < pEnd) && *pCur)
    {
        const char* pStrStart = pCur;

        // Search for the end of the string or a ';'
        for (;(pCur < pEnd) && *pCur && (*pCur != ';'); pCur++)
            ;

        if (!*pCur || (*pCur == ';') && m_pCCF && m_pValues)
        {
            res = SetCStringPropertyCCFWithNullTerm(m_pValues,
                                                    pName,
                                                    (BYTE*)pStrStart,
                                                    (UINT32)(pCur - pStrStart),
                                                    m_pCCF);
        }
    }

    return res;
}

HXBOOL CHXAdaptationHeader::IsIntParam(const char* pName)
{
    return InKeyList(pName, intParams());
}

HXBOOL CHXAdaptationHeader::IsStringParam(const char* pName)
{
    return InKeyList(pName, stringParams());
}

HXBOOL CHXAdaptationHeader::InKeyList(const char* pName, const char* const* pList)
{
    HXBOOL bRet = FALSE;

    if (pName && pList)
    {
        for (UINT32 i = 0; !bRet && pList[i]; i++)
        {
            if (!strcasecmp(pName, pList[i]))
            {
                // We found a match
                bRet = TRUE;
            }
        }
    }

    return bRet;
}

bool CHXAdaptationHeader::CompareIHXValues(IHXValues* pA, IHXValues* pB)
{
    bool ret = true;

    if (pA && pB)
    {
        const char* pKey = NULL;
        IHXBuffer* pValueA = NULL;

        HX_RESULT res = pA->GetFirstPropertyCString(pKey, pValueA);
        while (ret && (HXR_OK == res))
        {
            IHXBuffer* pValueB = NULL;

            if ((HXR_OK != pB->GetPropertyCString(pKey, pValueB)) ||
                (pValueA->GetSize() != pValueB->GetSize()) ||
                (memcmp(pValueA->GetBuffer(), pValueB->GetBuffer(),
                        pValueA->GetSize())))
            {
                ret = false;
            }

            HX_RELEASE(pValueA);
            HX_RELEASE(pValueB);

            res = pA->GetNextPropertyCString(pKey, pValueA);
        }

        if (ret)
        {
            ULONG32 ulValueA;
            HX_RESULT res = pA->GetFirstPropertyULONG32(pKey, ulValueA);

            while (ret && (HXR_OK == res))
            {
                ULONG32 ulValueB;

                if ((HXR_OK != pB->GetPropertyULONG32(pKey, ulValueB)) ||
                    (ulValueA != ulValueB))
                {
                    ret = false;
                }

                res = pA->GetNextPropertyULONG32(pKey, ulValueA);
            }
        }
    }
    else
    {
        ret = false;
    }

    return ret;
}

HX_RESULT CHXAdaptationHeader::CopyIHXValues(IHXValues* pA)
{
    HX_RESULT res = HXR_UNEXPECTED;

    HX_RELEASE(m_pValues);

    if (!pA)
    {
        res = HXR_OK;
    }
    else if (m_pCCF)
    {
        res = m_pCCF->CreateInstance(CLSID_IHXValues, (void**)&m_pValues);
        
        if (HXR_OK == res)
        {
            const char* pKey = NULL;
            IHXBuffer* pValue = NULL;
            
            HX_RESULT tmpRes = pA->GetFirstPropertyCString(pKey, pValue);
            
            while ((HXR_OK == res) && (HXR_OK == tmpRes))
            {
                res = SetCStringPropertyCCF(m_pValues,
                                            pKey,
                                            (const char*)pValue->GetBuffer(),
                                            m_pCCF);
                HX_RELEASE(pValue);
                tmpRes = pA->GetNextPropertyCString(pKey, pValue);
            }
            
            if (HXR_OK == res)
            {
                ULONG32 ulValue;
                HX_RESULT tmpRes = pA->GetFirstPropertyULONG32(pKey, ulValue);
                
                while ((HXR_OK == res) && (HXR_OK == tmpRes))
                {
                    res = m_pValues->SetPropertyULONG32(pKey, ulValue);
                    tmpRes = pA->GetNextPropertyCString(pKey, pValue);
                }
            }
        }
    }

    return res;
}

HXBOOL CHXAdaptationHeader::AreValuesValid(IHXValues* pA)
{
    HXBOOL bRet = TRUE;
    
    if (pA)
    {
        bRet = HasRequiredParams(pA);

        // Check for and verify URL property
        IHXBuffer* pURL = NULL;

        if (bRet &&
            ((HXR_OK != pA->GetPropertyCString("url", pURL)) ||
             (pURL->GetSize() <= 1) || 
             !IsValidURL((const char*)pURL->GetBuffer())))
        {
            // The url property must always be present
            bRet = FALSE;
        }
        HX_RELEASE(pURL);

        if (bRet)
        {
            // Check ULONG32 properties
            const char* pKey = NULL;
            ULONG32 ulValue;
            HX_RESULT res = pA->GetFirstPropertyULONG32(pKey, ulValue);
            
            while(bRet && (HXR_OK == res))
            {
                if (IsStringParam(pKey))
                {
                    // This is a string parameter passed as a
                    // ULONG32
                    bRet = FALSE;
                }

                res = pA->GetNextPropertyULONG32(pKey, ulValue);
            }
        }

        if (bRet)
        {
            // Check CString properties
            const char* pKey = NULL;
            IHXBuffer* pValue = NULL;
            HX_RESULT res = pA->GetFirstPropertyCString(pKey, pValue);
            
            while(bRet && (HXR_OK == res))
            {
                if (IsIntParam(pKey))
                {
                    // This is a ULONG32 parameter passed as a
                    // string
                    bRet = FALSE;
                }

                HX_RELEASE(pValue);
                res = pA->GetNextPropertyCString(pKey, pValue);
            }
        }
    }

    return bRet;
}

HXBOOL CHXAdaptationHeader::IsValidURL(const char* pURL)
{
    return TRUE;
}

HXBOOL CHXAdaptationHeader::HasRequiredParams(IHXValues* pA) const
{
    HXBOOL bRet = (pA) ? TRUE : FALSE;
    
    const char* const* pRequiredParams = requiredParams();
    
    if (pRequiredParams)
    {
        for(;bRet && *pRequiredParams; pRequiredParams++)
        {
            const char* pParamName = *pRequiredParams;

            IHXBuffer* pBuf = NULL;
            ULONG32 ulTmp;
            if ((HXR_OK != pA->GetPropertyCString(pParamName, pBuf)) &&
                (HXR_OK != pA->GetPropertyULONG32(pParamName, ulTmp)))
            {
                bRet = FALSE;
            }
            HX_RELEASE(pBuf);
        }
    }

    return bRet;
}

HX_RESULT CHXAdaptationHeader::AddURL(IHXBuffer* pBuf, 
                                       IHXBuffer* pURL)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pBuf && (pBuf->GetSize() == 0) && pURL)
    {
        // We need to add url=" to the beginning and an ending "
        UINT32 ulURLLength = pURL->GetSize() - 1 ; // w/o null terminator
        res = pBuf->SetSize(ulURLLength + 
                            6 + // 6 is for the 'url="' and '"'
                            1); // null terminator

        if (HXR_OK == res)
        {
            char* pCur = (char*)pBuf->GetBuffer();
            memcpy(pCur, "url=\"", 5);
            pCur += 5;

            memcpy(pCur, (char*)pURL->GetBuffer(), ulURLLength);
            pCur += ulURLLength;
            
            *pCur++ = '\"';
            *pCur++ = '\0';
        }
    }

    return res;
}

HX_RESULT CHXAdaptationHeader::AddStringParam(IHXBuffer* pBuf,
                                               const char* pKey, 
                                               IHXBuffer* pValue)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pBuf && pKey && pValue && (pValue->GetSize() > 1))
    {
        UINT32 uOldSize = pBuf->GetSize() - 1; // size w/o null terminator
        UINT32 uKeySize = strlen(pKey);
        UINT32 uValueSize = pValue->GetSize() - 1; // size w/o null terminator

        res = pBuf->SetSize(uOldSize + uKeySize + uValueSize + 
                            3); // 3 for ';' & '=' & null terminator

        if (HXR_OK == res)
        {
            char* pCur = (char*)pBuf->GetBuffer() + uOldSize;

            *pCur++ = ';';

            memcpy(pCur, pKey, uKeySize);
            pCur += uKeySize;
                
            *pCur++ = '=';
                
            memcpy(pCur, pValue->GetBuffer(), uValueSize);
            pCur += uValueSize;

            *pCur++ = '\0';
        }
    }

    return res;
}

HX_RESULT CHXAdaptationHeader::AddULONG32Param(IHXBuffer* pBuf,
                                                const char* pKey, 
                                                ULONG32 ulValue)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pBuf && pKey)
    {
        const UINT32 CCH_MAXUINT32 = 10;
        char buf[CCH_MAXUINT32 + 1];
        buf[CCH_MAXUINT32] = '\0';
        int numberSize = snprintf(buf, CCH_MAXUINT32, "%lu", ulValue);
        HX_ASSERT(numberSize > 0);
        if (numberSize > 0)
        {
            UINT32 uOldSize = pBuf->GetSize(); // Includes a null terminator
            UINT32 uKeySize = strlen(pKey);
            res = pBuf->SetSize(uOldSize + uKeySize + 
                                numberSize + 2); // 2 for ';' & '='

            if (HXR_OK == res)
            {
                char* pCur = (char*)pBuf->GetBuffer() + uOldSize - 1;

                *pCur++ = ';';

                memcpy(pCur, pKey, uKeySize);
                pCur += uKeySize;
                
                *pCur++ = '=';
                
                memcpy(pCur, buf, (size_t)numberSize);
                pCur += numberSize;

                *pCur++ = '\0';
            }
        }
    }

    return res;
}


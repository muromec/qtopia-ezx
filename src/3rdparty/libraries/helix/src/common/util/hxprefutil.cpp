/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxprefutil.cpp,v 1.15 2006/08/16 15:46:16 ehyche Exp $
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
#include "hxresult.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxccf.h"
#include "hxprefs.h"
#include "hxprefutil.h"
#include "hxstring.h"
#include "pckunpck.h"
#include "hlxclib/stdlib.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

HX_RESULT
ReadPrefUINT32(IHXPreferences* pPreferences, const char* pszName, UINT32& ulValue)
{
    HX_RESULT   rc = HXR_OK;
    IHXBuffer*  pBuffer = NULL;

    if (pPreferences && HXR_OK == pPreferences->ReadPref(pszName, pBuffer))
    {
        ulValue = atol((const char*) pBuffer->GetBuffer());
        HX_RELEASE(pBuffer);
    }
    else
    {
        rc = HXR_FAILED;
    }

    return rc;
}

HX_RESULT
ReadPrefUINT16(IHXPreferences* pPreferences, const char* pszName, UINT16& ulValue)
{
    HX_RESULT   rc = HXR_OK;
    IHXBuffer*  pBuffer = NULL;

    if (pPreferences && HXR_OK == pPreferences->ReadPref(pszName, pBuffer))
    {
        ulValue = atoi((const char*) pBuffer->GetBuffer());
        HX_RELEASE(pBuffer);
    }
    else
    {
        rc = HXR_FAILED;
    }

    return rc;
}

HX_RESULT
ReadPrefUINT8(IHXPreferences* pPreferences, const char* pszName, UINT8& nValue)
{
    HX_RESULT   rc = HXR_OK;
    IHXBuffer*  pBuffer = NULL;

    if (pPreferences && HXR_OK == pPreferences->ReadPref(pszName, pBuffer))
    {
        nValue = atoi((const char*) pBuffer->GetBuffer());
        HX_RELEASE(pBuffer);
    }
    else
    {
        rc = HXR_FAILED;
    }

    return rc;
}

HX_RESULT
ReadPrefFLOAT(IHXPreferences* pPreferences, const char* pszName, HXFLOAT& fValue)
{
    HX_RESULT   rc = HXR_OK;
    IHXBuffer*  pBuffer = NULL;

    if (pPreferences && HXR_OK == pPreferences->ReadPref(pszName, pBuffer))
    {
        fValue = (HXFLOAT) atof((const char*) pBuffer->GetBuffer());
        HX_RELEASE(pBuffer);
    }
    else
    {
        rc = HXR_FAILED;
    }

    return rc;
}

HX_RESULT   
ReadPrefBOOL(IHXPreferences* pPreferences, const char* pszName, HXBOOL& ulValue)
{
    HX_RESULT   rc = HXR_OK;
    IHXBuffer*  pBuffer = NULL;

    if (pPreferences && HXR_OK == pPreferences->ReadPref(pszName, pBuffer))
    {
        ulValue = (atol((const char*) pBuffer->GetBuffer()))? TRUE : FALSE;
        HX_RELEASE(pBuffer);
    }
    else
    {
        rc = HXR_FAILED;
    }

    return rc;
}

HX_RESULT   
ReadPrefCSTRING(IHXPreferences* pPreferences, const char* pszName, CHXString& strValue)
{
    HX_RESULT   rc = HXR_OK;
    IHXBuffer*  pBuffer = NULL;

    if (pPreferences && HXR_OK == pPreferences->ReadPref(pszName, pBuffer))
    {
        strValue = (const char*) pBuffer->GetBuffer();
        HX_RELEASE(pBuffer);
    }
    else
    {
        rc = HXR_FAILED;
    }

    return rc;
}

HX_RESULT ReadPrefUINT32Array(IHXPreferences* pPrefs,
                              const char*     pszBaseName,
                              UINT32          ulNumValues,
                              UINT32*         pulValue)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pPrefs && pszBaseName && ulNumValues && pulValue)
    {
        // Maximum UINT32 is 4294967295, which as a string
        // is 10 characters long. One more for the NULL terminator
        // and we get 11.
        UINT32 ulMaxLen = strlen(pszBaseName) + 11;
        char* pszTmp = new char [ulMaxLen];
        if (pszTmp)
        {
            // Clear the return value
            retVal = HXR_OK;
            // Loop through constructing the name
            // and reading the value
            for (UINT32 i = 0; i < ulNumValues && SUCCEEDED(retVal); i++)
            {
                // Create the name
                sprintf(pszTmp, "%s%lu", pszBaseName, i); /* Flawfinder: ignore */
                // Read the value from the preferences
                UINT32 ulTmp = 0;
                retVal = ReadPrefUINT32(pPrefs, (const char*) pszTmp, ulTmp);
                if (SUCCEEDED(retVal))
                {
                    pulValue[i] = ulTmp;
                }
            }
        }
        HX_VECTOR_DELETE(pszTmp);
    }

    return retVal;
}


HX_RESULT 
ReadPrefBOOL(IUnknown* pUnk, const char* pszName, HXBOOL& bValue)
{
    HX_RESULT hr = HXR_FAIL;
    IHXPreferences* pPrefs = NULL;
    if (pUnk && HXR_OK == pUnk->QueryInterface(IID_IHXPreferences, (void**) &pPrefs))
    {
        hr = ReadPrefBOOL(pPrefs, pszName, bValue);
        HX_RELEASE(pPrefs);
    }
    return hr;
}

HX_RESULT 
ReadPrefUINT32(IUnknown* pUnk, const char* pszName, UINT32& unValue)
{
    HX_RESULT hr = HXR_FAIL;
    IHXPreferences* pPrefs = NULL;
    if (pUnk && HXR_OK == pUnk->QueryInterface(IID_IHXPreferences, (void**) &pPrefs))
    {
        hr = ReadPrefUINT32(pPrefs, pszName, unValue);
        HX_RELEASE(pPrefs);
    }
    return hr;
}

HX_RESULT 
ReadPrefUINT16(IUnknown* pUnk, const char* pszName, UINT16& unValue)
{
    HX_RESULT hr = HXR_FAIL;
    IHXPreferences* pPrefs = NULL;
    if (pUnk && HXR_OK == pUnk->QueryInterface(IID_IHXPreferences, (void**) &pPrefs))
    {
        hr = ReadPrefUINT16(pPrefs, pszName, unValue);
        HX_RELEASE(pPrefs);
    }
    return hr;
}

HX_RESULT ReadPrefUINT8(IUnknown* pUnk, const char* pszName, UINT8& nValue)
{
    HX_RESULT hr = HXR_FAIL;
    IHXPreferences* pPrefs = NULL;
    if (pUnk && HXR_OK == pUnk->QueryInterface(IID_IHXPreferences, (void**) &pPrefs))
    {
        hr = ReadPrefUINT8(pPrefs, pszName, nValue);
        HX_RELEASE(pPrefs);
    }
    return hr; 
}

HX_RESULT ReadPrefFLOAT(IUnknown* pUnk, const char* pszName, HXFLOAT& fValue)
{
    HX_RESULT hr = HXR_FAIL;
    IHXPreferences* pPrefs = NULL;
    if (pUnk && HXR_OK == pUnk->QueryInterface(IID_IHXPreferences, (void**) &pPrefs))
    {
        hr = ReadPrefFLOAT(pPrefs, pszName, fValue);
        HX_RELEASE(pPrefs);
    }
    return hr; 
}

HX_RESULT ReadPrefCSTRING(IUnknown* pUnk, const char* pszName, CHXString& strValue)
{
    HX_RESULT hr = HXR_FAIL;
    IHXPreferences* pPrefs = NULL;
    if (pUnk && HXR_OK == pUnk->QueryInterface(IID_IHXPreferences, (void**) &pPrefs))
    {
        hr = ReadPrefCSTRING(pPrefs, pszName, strValue);
        HX_RELEASE(pPrefs);
    }
    return hr;
}

HX_RESULT ReadPrefStringBuffer(IUnknown* pUnk, const char* pszName, IHXBuffer*& rpStr)
{
    HX_RESULT retVal = HXR_FAIL;

    HX_ASSERT(rpStr == NULL);

    if (pUnk && pszName)
    {
        // Read the preference into a CHXString
        CHXString strValue;
        retVal = ReadPrefCSTRING(pUnk, pszName, strValue);
        if (SUCCEEDED(retVal))
        {
            // Create a string buffer
            retVal = CreateStringBufferCCF(rpStr, (const char*) strValue, pUnk);
        }
    }

    return retVal;
}

HX_RESULT ReadPrefUINT32Array(IUnknown* pUnk,
                              const char*     pszBaseName,
                              UINT32          ulNumValues,
                              UINT32*         pulValue)
{
    HX_RESULT hr = HXR_FAIL;
    IHXPreferences* pPrefs = NULL;
    if (pUnk && HXR_OK == pUnk->QueryInterface(IID_IHXPreferences, (void**) &pPrefs))
    {
        hr = ReadPrefUINT32Array(pPrefs, pszBaseName, ulNumValues, pulValue);
        HX_RELEASE(pPrefs);
    }
    return hr;
}

HX_RESULT WritePrefUINT32(IUnknown* pUnk, const char* pszName, UINT32 uValue)
{
    CHXString tmp;
    tmp.AppendULONG(uValue);

    return WritePrefCSTRING(pUnk, pszName, tmp);
}

HX_RESULT WritePrefCSTRING(IUnknown* pUnk, const char* pszName, 
                           const CHXString& strValue)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pUnk && pszName)
    {
        IHXCommonClassFactory* pCCF = NULL;
        IHXPreferences* pPrefs = NULL;
        IHXBuffer* pBuf = NULL;

        res = pUnk->QueryInterface(IID_IHXCommonClassFactory,
                                   (void**)&pCCF);
        if (HXR_OK == res)
        {
            res = pUnk->QueryInterface(IID_IHXPreferences, (void**)&pPrefs);
        }

        if (HXR_OK == res)
        {
            res = pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuf);
        }

        if (HXR_OK == res)
        {
            res = pBuf->Set((UCHAR*)(const char*)strValue,
                            strValue.GetLength() + 1);
        }

        if (HXR_OK == res)
        {
            res = pPrefs->WritePref(pszName, pBuf);
        }
        
        HX_RELEASE(pBuf);
        HX_RELEASE(pCCF);
        HX_RELEASE(pPrefs);
    }

    return res;
}




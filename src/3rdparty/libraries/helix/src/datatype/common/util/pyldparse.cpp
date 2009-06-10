/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: pyldparse.cpp,v 1.5 2008/01/24 12:20:56 vkathuria Exp $
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

#include "pyldparse.h"
#include "char_stack.h"

#include "hlxclib/stdlib.h"
#include "hlxclib/string.h"

CPayloadParamParser::CPayloadParamParser() :
    m_pCCF(0),
    m_pValues(0)
{}

CPayloadParamParser::~CPayloadParamParser()
{
    HX_RELEASE(m_pCCF);
    HX_RELEASE(m_pValues);
}

HX_RESULT CPayloadParamParser::Init(IHXCommonClassFactory* pCCF)
{
    HX_RESULT res = HXR_FAILED;
    
    if (pCCF)
	res = pCCF->QueryInterface(IID_IHXCommonClassFactory,
				   (void**)&m_pCCF);

    return res;
}

HX_RESULT CPayloadParamParser::Parse(const char* pParamString)
{
    HX_RESULT res = HXR_FAILED;

    HX_RELEASE(m_pValues);

    if (m_pCCF &&
	SUCCEEDED(res = m_pCCF->CreateInstance(IID_IHXValues,
					       (void**)&m_pValues)))
    {
	CharStack key;
	CharStack value;
	
	const char* pCur = pParamString;
	
	while(*pCur && SUCCEEDED(res))
	{
	    // Get rid of leading whitespace
	    for(;*pCur && *pCur == ' '; pCur++) 
		;
	    
	    // Get key
	    key.Reset();
	    while(*pCur && (*pCur != ' ') && (*pCur != '='))
	    {
		*key = *pCur++;
		key++;
	    }
	    
	    if (strlen(key.Finish()) &&
		(*pCur == '='))
	    {
		// This is a key=value pair
		pCur++; // skip '='
		
		value.Reset();
		
		// Get value
		while(*pCur && (*pCur != ' ') && (*pCur != ';'))
		{
		    *value = *pCur++;
		    value++;
		}
		
		if (strlen(value.Finish()))
		{
		    if (*pCur)
			pCur++; // skip ' ' or ';'
		    
		    res = AddParameter(key.Finish(), value.Finish());
		}
		else
		{
		    // We got an empty value
		    res = HXR_UNEXPECTED;
		}
	    }
	    else
	    {
		// We got an empty or invalid key
		res = HXR_UNEXPECTED;
	    }
	}
	
	if (!SUCCEEDED(res))
	    HX_RELEASE(m_pValues);
    }

    return res;
}

HX_RESULT CPayloadParamParser::AddParameter(const char* pKey, 
					    const char* pValue)
{
    HX_RESULT res = HXR_FAILED;

    IHXBuffer* pBuf = 0;

    if (m_pCCF &&
	SUCCEEDED(res = m_pCCF->CreateInstance(IID_IHXBuffer, 
					       (void**)&pBuf))&&
	SUCCEEDED(res = pBuf->SetSize(strlen(pValue) + 1)))
    {
	strcpy((char*)pBuf->GetBuffer(), pValue); /* Flawfinder: ignore */
	m_pValues->SetPropertyCString(pKey, pBuf);
    }

    HX_RELEASE(pBuf);

    return res;
}

HX_RESULT CPayloadParamParser::GetULONG32(const char* pKey, 
					  REF(ULONG32) ulValue)
{
    HX_RESULT res = HXR_FAILED;
    IHXBuffer* pBuf = 0;

    if (m_pValues &&
	SUCCEEDED(m_pValues->GetPropertyCString(pKey, pBuf)))
    {
	const char* pValue = (const char*)pBuf->GetBuffer();
	char* pEnd = 0;

	ULONG32 ulTmp = strtoul(pValue, &pEnd, 10);

	if (*pValue && pEnd && !*pEnd)
	{
	    ulValue = ulTmp;
	    res = HXR_OK;
	}
    }
    HX_RELEASE(pBuf);

    return res;
}

static HX_RESULT FromHex(char ch, UINT8& val)
{
    HX_RESULT res = HXR_OK;

    if ((ch >= '0') && (ch <= '9'))
	val += ch - '0';
    else if ((ch >= 'a') && (ch <= 'f'))
	val += 10 + ch - 'a';
    else if ((ch >= 'A') && (ch <= 'F'))
	val += 10 + ch - 'A';
    else
	res = HXR_FAILED;

    return res;
}

HX_RESULT CPayloadParamParser::GetBuffer(const char* pKey, 
					 REF(IHXBuffer*) pValue)
{
    HX_RESULT res = HXR_FAILED;
        
    IHXBuffer* pBuf = 0;

    if (m_pCCF && m_pValues &&
	SUCCEEDED(m_pValues->GetPropertyCString(pKey, pBuf)))
    {
	const char* pStr = (const char*)pBuf->GetBuffer();
	ULONG32 ulStringSize = strlen(pStr);
	ULONG32 ulBufferSize = ulStringSize / 2;

	// make sure the string length is even
	if (!(ulStringSize & 0x1)) 
	{
	    res = m_pCCF->CreateInstance(IID_IHXBuffer, (void**)&pValue);

	    if (SUCCEEDED(res) &&
		SUCCEEDED(res = pValue->SetSize(ulBufferSize)))
	    {
		UINT8* pDest = pValue->GetBuffer();
		
		while(SUCCEEDED(res) && *pStr)
		{
		    UINT8 val = 0;
		    
		    res = FromHex(*pStr++, val);
		    
		    if (SUCCEEDED(res))
		    {
			val <<= 4;
			res = FromHex(*pStr++, val);
			
			if (SUCCEEDED(res))
			    *pDest++ = val;
		    }
		}
	    }

	    if (!SUCCEEDED(res))
		HX_RELEASE(pValue);
	}
    }
    HX_RELEASE(pBuf);
    
    return res;
}

HX_RESULT CPayloadParamParser::GetString(const char* pKey, 
					 REF(IHXBuffer*) pValue)
{
    HX_RESULT res = HXR_FAILED;
        
    IHXBuffer* pBuf = 0;

    if (m_pValues &&
	SUCCEEDED(m_pValues->GetPropertyCString(pKey, pBuf)))
    {
	pValue = pBuf;
	pValue->AddRef();
    }
    
    HX_RELEASE(pBuf);

    return res;
}

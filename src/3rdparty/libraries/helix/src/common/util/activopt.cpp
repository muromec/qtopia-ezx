/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: activopt.cpp,v 1.6 2007/07/06 20:39:16 jfinnecy Exp $
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

#include "hlxclib/stdlib.h"
#ifdef _BENG_DEBUG
#include "hlxclib/stdio.h"
#endif /* _BENG_DEBUG */
#include "hxtypes.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "hxplugn.h"
#include "hxformt.h"
#include "ihxpckts.h"
#include "hxmon.h"
#include "hxallow.h"
#include "hxerror.h"
#include "hxengin.h"
#include "hxslist.h"
#include "hxstrutl.h"
#include "hxbuffer.h"

#include "activewrap.h"
#include "activopt.h"


#ifdef _BENG_DEBUG
    #define DUMP Dump()
#else
    #define DUMP 
#endif

CActiveOptions::CActiveOptions()
    : m_lRefCount(0)
    , m_pContext(0)
    , m_pOptions(0)
{
}

CActiveOptions::~CActiveOptions()
{
    HX_RELEASE(m_pOptions);
    HX_RELEASE(m_pContext);
}


STDMETHODIMP_(ULONG32)
CActiveOptions::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
CActiveOptions::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


HX_RESULT
CActiveOptions::Init(IUnknown* pContext, IHXValues* pOptions)
{
    if (!(pContext || pOptions))
	return HXR_FAIL;

    HX_RELEASE(m_pContext);
    m_pContext = pContext;
    m_pContext->AddRef();

    HX_RELEASE(m_pOptions);
    m_pOptions = pOptions;
    pOptions->AddRef();

    IHXBuffer* pLongName = 0;
    CActivePropWrapper* pActiveReg = 0;

    HX_RESULT res = pOptions->GetPropertyBuffer("LongName", pLongName);
    if (HXR_OK != res)
    {
	goto cleanup;
    }
    
    //printf( "CActiveOptions::Init(): Setting %s options active\n", GetLongName());

    pActiveReg = new CActivePropWrapper();
    if (!pActiveReg)
    {
    	res = HXR_FAILED ;
	goto cleanup;
    }
    pActiveReg->AddRef();

    res = pActiveReg->Init(pContext, (CActivePropWrapperUser*) this);
    if (HXR_OK != res)
    {
	goto cleanup;
    }

    res = pActiveReg->SetAsActive((const char*)pLongName->GetBuffer());
    if (res != HXR_OK)
    {
	goto cleanup;
    }

cleanup:
    HX_RELEASE(pLongName);
    HX_RELEASE(pActiveReg);
    return res;
}


/*
    CActivePropWrapperUser methods
*/

HX_RESULT
CActiveOptions::PropUpdated (REF(CHXString) strName, REF(UINT32) ulVal, REF(CHXString) strErr)
{
    CHXString strProp;
    HX_RESULT res = GetPropName(strName, strProp);
    if ((HXR_OK != res) || strProp.IsEmpty())
    {
	return res;
    }

    m_pOptions->SetPropertyULONG32((const char*)strProp, ulVal);

    //DUMP;
    return HXR_OK ;
}

HX_RESULT
CActiveOptions::PropUpdated (REF(CHXString) strName, REF(CHXString) strVal, REF(CHXString) strErr)
{
    CHXString strProp;
    HX_RESULT res = GetPropName(strName, strProp);
    if ((HXR_OK != res) || strProp.IsEmpty())
    {
	return res;
    }

    IHXBuffer* pBuf = 0;
    IHXCommonClassFactory* pClassFactory = 0;
    res = m_pContext->QueryInterface(IID_IHXCommonClassFactory,
				     (void**)&pClassFactory);
    if (HXR_OK != res)
    {
	goto cleanup;
    }

    res = pClassFactory->CreateInstance(CLSID_IHXBuffer,(void**)&pBuf);
    if (HXR_OK != res)
    {
	goto cleanup;
    }
    pBuf->Set((const unsigned char*)(const char*)strVal, strVal.GetLength() + 1);
    res = m_pOptions->SetPropertyBuffer((const char*)strProp, pBuf);

    //DUMP;

cleanup:
    HX_RELEASE(pClassFactory);
    HX_RELEASE(pBuf);

    return res;
}

HX_RESULT
CActiveOptions::PropDeleted (REF(CHXString) strName, REF(CHXString) strErr)
{
    // deleting a property is not supported by IHXValues
    return HXR_OK ;
}



HX_RESULT
CActiveOptions::GetPropName(REF(CHXString) strName, REF(CHXString) strProp)
{
    strProp.Empty();

    IHXBuffer* pLongName = 0;
    HX_RESULT res = m_pOptions->GetPropertyBuffer("LongName", pLongName);
    if (HXR_OK != res)
    {
	return HXR_FAILED;
    }

    res = HXR_OK;

    // we only care about the first level of properties under the longname list
    // so strip off the last segment of strName and compare this to the LongName: 
    // they should be equal
    int pos = strName.ReverseFind('.');
    if ((pos == -1) ||
	(strName.Left(pos) != (const char*)pLongName->GetBuffer()))
    {
	goto cleanup;
    }
    strProp = strName.Right(strName.GetLength()-pos-1);

    // reject changes to the 'read-only' properties: MountPoint, LongName and ShortName
    if ((strProp == "MountPoint") ||
	(strProp == "ShortName") ||
	(strProp == "LongName"))
    {
	res = HXR_FAILED;
    }
    
cleanup:
    HX_RELEASE(pLongName);
    return res;
}


#ifdef _BENG_DEBUG
void
CActiveOptions::Dump()
{
    HX_RESULT res;
    UINT32 ulVal;
    const char* pName;
    IHXBuffer* pBuf;

    m_pOptions->GetPropertyBuffer("LongName", pBuf);
    printf( "%s options:\n", (const char*)pBuf->GetBuffer());
    HX_RELEASE(pBuf);

    res = m_pOptions->GetFirstPropertyULONG32(pName, ulVal);
    while(res == HXR_OK)
    {
	printf( "\t%s:\t%d\n", pName, ulVal );
	res = m_pOptions->GetNextPropertyULONG32(pName, ulVal);
    }
    res = m_pOptions->GetFirstPropertyBuffer(pName, pBuf);
    while(res == HXR_OK)
    {
	printf( "\t%s:\t%s\n", pName, (const char*)pBuf->GetBuffer());
	HX_RELEASE(pBuf);
	res = m_pOptions->GetNextPropertyBuffer(pName, pBuf);
    }
    res = m_pOptions->GetFirstPropertyCString(pName, pBuf);
    while(res == HXR_OK)
    {
	printf( "\t%s:\t%s\n", pName, (const char*)pBuf->GetBuffer());
	HX_RELEASE(pBuf);
	res = m_pOptions->GetNextPropertyCString(pName, pBuf);
    }
}
#endif


/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sdpplin.cpp,v 1.21 2008/08/20 21:11:31 ehyche Exp $
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

/////////////////////////////////////////////////////////////////////////////
// 
//  SDP Stream Description plugin
//
#include "hxsdp.ver"

#include "hlxclib/stdlib.h"

#define INITGUID 1

#include "hxtypes.h"
#include "hxcom.h"
#include "hxstring.h"
#include "hxslist.h"
#include "chxpckts.h"
#include "ihxfgbuf.h"
#include "hxplugn.h"
#include "hxengin.h"
#include "hxsdesc.h"
#include "hxcomm.h"
#include "hxmon.h"
#include "hxstrutl.h"        // strcasecmp
#include "hxcore.h"        // IHXPlayer
#include "hxupgrd.h"        // IHXUpgradeCollection
#include "rtsputil.h"
#include "rtspprop.h"
#include "sdpprop.h"
#include "sdpplin.h"
#include "sdptypes.h"
#include "sdpmdparse.h"
#include "sdpmdgen.h"
#include "sdppyldinfo.h"
#include "ihxtlogsystem.h"
#include "ihxtlogsystemcontext.h"
#include "hxdllaccess.h"
#undef INITGUID

#include "hxver.h"
#include "hxheap.h"

#ifdef _DEBUG
#undef HX_THIS_FILE                
static const char HX_THIS_FILE[] = __FILE__;
#endif

#ifdef _AIX
#include "hxtbuf.h"
#include "dllpath.h"
ENABLE_MULTILOAD_DLLACCESS_PATHS(Sdpplin);
#elif !defined(HELIX_FEATURE_DLLACCESS_CLIENT)
#include "dllpath.h"
ENABLE_DLLACCESS_PATHS(Sdpplin);
#endif	// HELIX_FEATURE_DLLACCESS_CLIENT

#if defined(HELIX_FEATURE_SERVER)
#define SDP_MAJOR   6L
#define SDP_MINOR   0L
#define SDP_RELEASE 7L
#define SDP_BUILD   4248L
#define SDP_MEDIA_DESC_GENERATOR_VERSION HX_ENCODE_PROD_VERSION(SDP_MAJOR,SDP_MINOR,SDP_RELEASE,SDP_BUILD)
#endif


#if defined(HELIX_CONFIG_NOSTATICS)
#include "globals/hxglobalint.h"
#define DLLRefType const INT32
#else
#define DLLRefType INT32
#endif

static DLLRefType g_nRefCount_sdpp = 0 ;

INT32& RefCountSDPP()
{
#if defined(HELIX_CONFIG_NOSTATICS)
    return HXGlobalInt32::Get((GlobalID)&g_nRefCount_sdpp);
#else
    return g_nRefCount_sdpp;
#endif
}

/****************************************************************************
 * 
 *  Function:
 * 
 *        HXCreateInstance()
 * 
 *  Purpose:
 * 
 *        Function implemented by all plugin DLL's to create an instance of 
 *        any of the objects supported by the DLL. This method is similar to 
 *        Window's CoCreateInstance() in its purpose, except that it only 
 *        creates objects from this plugin DLL.
 *
 *        NOTE: Aggregation is never used. Therefore and outer unknown is
 *        not passed to this function, and you do not need to code for this
 *        situation.
 * 
 */
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(HXCREATEINSTANCE)
(
    IUnknown**  /*OUT*/        ppIUnknown
)
{
    *ppIUnknown = (IUnknown*)(IHXPlugin*)new CSDPStreamDescription;
    if (*ppIUnknown)
    {
        (*ppIUnknown)->AddRef();
        return HXR_OK;
    }
    return HXR_OUTOFMEMORY;
}

/****************************************************************************
 * 
 *  Function:
 * 
 *        CanUnload()
 * 
 *  Purpose:
 * 
 *        Function implemented by all plugin DLL's if it returns HXR_OK 
 *        then the pluginhandler can unload the DLL
 *
 */
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(CanUnload2)(void)
{
    return (RefCountSDPP() ? HXR_FAIL : HXR_OK);
}

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(CanUnload)(void)
{
    return ENTRYPOINT(CanUnload2)();
}

const char* const CSDPStreamDescription::zm_pDescription    = "RealNetworks SDP Stream Description Plugin";
const char* const CSDPStreamDescription::zm_pCopyright      = HXVER_COPYRIGHT;
const char* const CSDPStreamDescription::zm_pMoreInfoURL    = HXVER_MOREINFO;

const char* const CSDPStreamDescription::zm_pStreamDescriptionMimeType  = 
    "application/sdp";

/************************************************************************
 *  Method:
 *    IHXPlugin::InitPlugin
 *  Purpose:
 *    Initializes the plugin for use. This interface must always be
 *    called before any other method is called. This is primarily needed 
 *    so that the plugin can have access to the context for creation of
 *    IHXBuffers and IMalloc.
 */
STDMETHODIMP CSDPStreamDescription::InitPlugin(IUnknown* /*IN*/ pContext)
{
    HX_RESULT res = HXR_FAILED;

    m_pContext = pContext;
    m_pContext->AddRef();
    m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCCF);
    if (m_pCCF != NULL)
    {
        res = HXR_OK;
    }

    if (SUCCEEDED(res) && m_pDescParser)
    {
        res = m_pDescParser->Init(pContext);
    }

    if (SUCCEEDED(res) && m_pDescGenerator)
    {
        res = m_pDescGenerator->Init(pContext);
    }

    return res;
}

/************************************************************************
 *  Method:
 *    IHXPlugin::GetPluginInfo
 *  Purpose:
 *    Returns the basic information about this plugin. Including:
 *
 *    bLoadMultiple        whether or not this plugin DLL can be loaded
 *                        multiple times. All File Formats must set
 *                        this value to TRUE.
 *    pDescription        which is used in about UIs (can be NULL)
 *    pCopyright        which is used in about UIs (can be NULL)
 *    pMoreInfoURL        which is used in about UIs (can be NULL)
 */
STDMETHODIMP CSDPStreamDescription::GetPluginInfo
(
    REF(HXBOOL)        /*OUT*/ bLoadMultiple,
    REF(const char*) /*OUT*/ pDescription,
    REF(const char*) /*OUT*/ pCopyright,
    REF(const char*) /*OUT*/ pMoreInfoURL,
    REF(ULONG32)     /*OUT*/ ulVersionNumber
)
{
    bLoadMultiple = TRUE;   // Must be true for file formats.

    pDescription    = zm_pDescription;
    pCopyright      = zm_pCopyright;
    pMoreInfoURL    = zm_pMoreInfoURL;
    ulVersionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXPlugin::GetStreamDescriptionInfo
 *  Purpose:
 *    If this object is a stream description object this method returns
 *    information vital to the instantiation of stream description plugins.
 *    If this object is not a stream description object, it should return
 *    HXR_UNEXPECTED.
 */
STDMETHODIMP CSDPStreamDescription::GetStreamDescriptionInfo
(
    REF(const char*) /*OUT*/ pStreamDescriptionMimeType
)
{
    pStreamDescriptionMimeType  = zm_pStreamDescriptionMimeType;

    return HXR_OK;
}

// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//        IUnknown::QueryInterface
//  Purpose:
//        Implement this to export the interfaces supported by your 
//        object.
//
STDMETHODIMP CSDPStreamDescription::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPlugin))
    {
        AddRef();
        *ppvObj = (IHXPlugin*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXStreamDescription))
    {
        AddRef();
        *ppvObj = (IHXStreamDescription*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXStreamDescriptionSettings))
    {
        AddRef();
        *ppvObj = (IHXStreamDescriptionSettings*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTPPayloadInfo))
    {
        AddRef();
        *ppvObj = (IHXRTPPayloadInfo*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//        IUnknown::AddRef
//  Purpose:
//        Everyone usually implements this the same... feel free to use
//        this implementation.
//
STDMETHODIMP_(ULONG32) CSDPStreamDescription::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//        IUnknown::Release
//  Purpose:
//        Everyone usually implements this the same... feel free to use
//        this implementation.
//
STDMETHODIMP_(ULONG32) CSDPStreamDescription::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return (ULONG32)m_lRefCount;
    }

    delete this;
    return 0;
}

// *** IHXStreamDescription methods ***

CSDPStreamDescription::CSDPStreamDescription()
                : m_lRefCount(0)
                , m_pDescParser(0)
                , m_pDescGenerator(0)
                , m_pContext(0)
                , m_pCCF(0)
{
    RefCountSDPP()++;

    m_pDescParser = new SDPMediaDescParser(TARVER_ULONG32_VERSION);

#if defined(HELIX_FEATURE_SERVER)
    m_pDescGenerator = new SDPMediaDescGenerator(SDP_MEDIA_DESC_GENERATOR_VERSION);
#endif /* defined(HELIX_FEATURE_SERVER) */
}

CSDPStreamDescription::~CSDPStreamDescription()
{
    RefCountSDPP()--;

    delete m_pDescParser;
    HX_DELETE(m_pDescGenerator);
    HX_RELEASE(m_pCCF);
    HX_RELEASE(m_pContext);
}

HX_RESULT
CSDPStreamDescription::Update()
{
    IHXPlayer* pPlayer = NULL;
    if (HXR_OK == m_pContext->QueryInterface(IID_IHXPlayer, (void**)&pPlayer))
    {            
        IHXUpgradeCollection* pUpgradeCollection = NULL;

        if (HXR_OK == pPlayer->QueryInterface(IID_IHXUpgradeCollection, 
                (void**)&pUpgradeCollection))
        {
            IHXBuffer* pBuf = NULL;
            m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuf);
            if (!pBuf)
            {
                HX_RELEASE(pUpgradeCollection);
                HX_RELEASE(pPlayer);                
                return HXR_OUTOFMEMORY;
            }
            pBuf->Set((const BYTE*)zm_pStreamDescriptionMimeType, 
                    strlen(zm_pStreamDescriptionMimeType) + 1);
            pUpgradeCollection->Add(eUT_Required, pBuf, 0, 0);
            HX_RELEASE(pBuf);
        }
        HX_RELEASE(pUpgradeCollection);    

    }
    HX_RELEASE(pPlayer);                

    return HXR_OK;
}

STDMETHODIMP
CSDPStreamDescription::GetValues(IHXBuffer* pDescription,
    REF(UINT16) nValues, REF(IHXValues**) pValueArray)
{
    HX_RESULT res = HXR_UNEXPECTED;

    if (m_pDescParser)
        res = m_pDescParser->Parse(pDescription, nValues, pValueArray);

    if (HXR_REQUEST_UPGRADE == res)
    {
        res = Update();
        if (HXR_OK == res)
            res = HXR_REQUEST_UPGRADE;
    }

    return res;
}

STDMETHODIMP
CSDPStreamDescription::GetDescription(UINT16 nValues, IHXValues** pValueArray,
    REF(IHXBuffer*) pDescription)
{
    HX_RESULT res = HXR_UNEXPECTED;

    if (m_pDescGenerator)
        res = m_pDescGenerator->Generate(nValues, pValueArray, pDescription);

    return res;
}

// *** IHXStreamDescriptionSettings methods ***

STDMETHODIMP
CSDPStreamDescription::SetOption(const char* pKey, IHXBuffer* pVal)
{
    if (pKey == NULL || pVal == NULL)
    {
        return HXR_POINTER;
    }
    const char* pszVal = (const char*)pVal->GetBuffer();
    if (pszVal == NULL || *(pszVal+pVal->GetSize()-1) != '\0')
    {
        return HXR_UNEXPECTED;
    }

    HX_RESULT pnr = HXR_FAIL;
    if ((strcasecmp(pKey, "UseOldEOL") == 0) && m_pDescGenerator)
    {
        if (strcasecmp(pszVal, "true") == 0)
        {
            m_pDescGenerator->SetUseOldEOL(TRUE);
            pnr = HXR_OK;
        }
        else if (strcasecmp(pszVal, "false") == 0)
        {
            m_pDescGenerator->SetUseOldEOL(FALSE);
            pnr = HXR_OK;
        }
    }
    else if (strcasecmp(pKey, "AbsoluteBaseURL") == 0)
    {
        if (*pszVal == '0')
        {
                m_pDescGenerator->SetUseAbsoluteURL(FALSE);
            pnr = HXR_OK;
        }
        else if  (*pszVal == '1')
        {
                m_pDescGenerator->SetUseAbsoluteURL(TRUE);
            pnr = HXR_OK;
        }
    }
    else if (strcasecmp(pKey, "SessionGUID") == 0)
    {
        if (*pszVal == '0')
        {
                m_pDescGenerator->SetUseSessionGUID(FALSE);
            pnr = HXR_OK;
        }
        else if  (*pszVal == '1')
        {
                m_pDescGenerator->SetUseSessionGUID(TRUE);
        }
    }
    // add new options here

    return pnr;
}

STDMETHODIMP
CSDPStreamDescription::GetOption(const char* pKey, REF(IHXBuffer*) pVal)
{
    if (pKey == NULL)
    {
        return HXR_POINTER;
    }
    if (m_pCCF == NULL)
    {
        return HXR_UNEXPECTED;
    }
    m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pVal);
    if (pVal == NULL)
    {
        return HXR_OUTOFMEMORY;
    }

    HX_RESULT pnr = HXR_FAIL;
    char* pszVal;
    if ((strcasecmp(pKey, "UseOldEOL") == 0) && m_pDescGenerator)
    {
        pVal->SetSize(5+1);
        pszVal = (char*)pVal->GetBuffer();
        strcpy(pszVal, (m_pDescGenerator->GetUseOldEOL()?"true":"false")); /* Flawfinder: ignore */
        pnr = HXR_OK;
    }
    else
    {
        HXBOOL bIsOptionSet = FALSE;

        if (strcasecmp(pKey, "AbsoluteBaseURL") == 0)
        {
            bIsOptionSet = m_pDescGenerator->GetUseAbsoluteURL();
            pnr = HXR_OK;
        }
        else if (strcasecmp(pKey, "SessionGUID") == 0)
        {
            bIsOptionSet = m_pDescGenerator->GetUseSessionGUID();
            pnr = HXR_OK;
        }

        if (HXR_OK == pnr)
        {
            pVal->SetSize(5+1);
            pszVal = (char*)pVal->GetBuffer();
            strcpy(pszVal, (bIsOptionSet ? "true":"false")); /* Flawfinder: ignore */
        }
    }

    if (pnr != HXR_OK)
    {
        HX_RELEASE(pVal);
    }

    return pnr;
}

// *** IHXRTPPayloadInfo methods ***

/************************************************************************
 *        Method:
 *            IHXRTPPayloadInfo::PayloadSupported
 *        Purpose:
 *            Returns TRUE if this payload type is handled by this interface
 */
 
HXBOOL
CSDPStreamDescription::IsPayloadSupported(UINT32 ulRTPPayloadType)
{
    // make sure it's not one of reserved/unassigned ones
    if (SDPIsStaticPayload(ulRTPPayloadType) &&
        SDPMapPayloadToEncodingName(ulRTPPayloadType))
    {
        return TRUE;        
    }

    return FALSE;
}

/************************************************************************
 *        Method:
 *            IHXRTPPayloadInfo::GetTimestampConversionFactors
 *        Purpose:
 *            Retrieves the RTP and RMA factors for RTP to RMA timestamp ratio.
 *      RTP->RMA is RTPTimestamp * RTPFactor / HXFactor
 *      RMA->RTP is HXTimestamp * HXFactor / RTPFactor
 *        
 *        Note: does not check if the payload type is supported
 */
 
STDMETHODIMP
CSDPStreamDescription::GetTimestampConversionFactors
(                                
    UINT32 ulRTPPayloadType,
    REF(UINT32) /*OUT*/ ulRTPFactor,
    REF(UINT32) /*OUT*/ ulHXFactor
)
{
    HX_ASSERT(IsPayloadSupported(ulRTPPayloadType));
    ulRTPFactor = SDPMapPayloadToRTPFactor(ulRTPPayloadType);
    ulHXFactor = SDPMapPayloadToRMAFactor(ulRTPPayloadType);
    return HXR_OK;
}

/************************************************************************
 *        Method:
 *            IHXRTPPayloadInfo::IsTimestampDeliverable
 *        Purpose:
 *            Returns TRUE if this payload type is timestamp deliverable
 */
 
HXBOOL
CSDPStreamDescription::IsTimestampDeliverable(UINT32 ulRTPPayloadType)
{
    HX_ASSERT(IsPayloadSupported(ulRTPPayloadType));
    return SDPIsTimestampDeliverable(ulRTPPayloadType);    
}


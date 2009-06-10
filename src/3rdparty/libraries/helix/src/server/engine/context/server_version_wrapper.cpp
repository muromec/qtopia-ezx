/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: server_version_wrapper.cpp,v 1.3 2003/01/23 23:42:53 damonlan Exp $ 
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

/*
 * Implementation of IHXProductVersion.
 *
 * I believe this is called rarely enough that I don't cache any of the
 * data or make persistent objects.  That makes it pretty straightforward.
 *
 * Similarly, it's generally better to get serverbuffers from the context,
 * which allows them to be cached by the context; but again, this will
 * be called so rarely it doesn't seem worth it.
 *
 * From the IHXProductVersion definition:
     *  EXAMPLES:
     *    ProductName:          "Helix Server"
     *    ReleaseName:          "9.0 Beta"
     *    FullProductName:      "Helix Server 9.0 Beta"
     *    BuildName:            ""
     *    VersionString:        "9.0.2.588"
     *    MajorMinorString:     "9.0"
     *    PlatformName:         "freebsd-4.0-i386"
     *    MajorVersion:         9
     *    MinorVersion:         0
     *    GetVersion:           9,0,2,588
     *    GetVersion32:         0x9000224c
 */

#include <string.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "ihxpckts.h"  //IHXBuffer

#include "servbuffer.h"
#include "server_version.h"
#include "server_version_wrapper.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static char HX_THIS_FILE[] = __FILE__;
#endif

ServerVersionWrapper::ServerVersionWrapper()
    : m_lRefCount(0)
{
}

ServerVersionWrapper::~ServerVersionWrapper()
{
}
    /*
     *	IHXProductVersion methods
     */

STDMETHODIMP
ServerVersionWrapper::ProductName(REF(IHXBuffer*) pProductName)
{
    const char* name = ServerVersion::ProductName();

    pProductName = new ServerBuffer(TRUE);
    pProductName->Set((const unsigned char*)name, strlen(name) + 1);

    return HXR_OK;
}

STDMETHODIMP
ServerVersionWrapper::ReleaseName(REF(IHXBuffer*) pReleaseName)
{
    const char* name = ServerVersion::ReleaseName();

    pReleaseName = new ServerBuffer(TRUE);
    pReleaseName->Set((const unsigned char*)name, strlen(name) + 1);

    return HXR_OK;
}

STDMETHODIMP
ServerVersionWrapper::FullProductName(REF(IHXBuffer*) pFullName)
{
    const char* name = ServerVersion::FullProductName();

    pFullName = new ServerBuffer(TRUE);
    pFullName->Set((const unsigned char*)name, strlen(name) + 1);

    return HXR_OK;
}

STDMETHODIMP
ServerVersionWrapper::BuildName(REF(IHXBuffer*) pBuildName)
{
    const char* name = ServerVersion::BuildName();

    pBuildName = new ServerBuffer(TRUE);
    pBuildName->Set((const unsigned char*)name, strlen(name) + 1);

    return HXR_OK;
}

STDMETHODIMP
ServerVersionWrapper::ExecutableName(REF(IHXBuffer*) pExecutableName)
{
    const char* name = ServerVersion::ExecutableName();

    pExecutableName = new ServerBuffer(TRUE);
    pExecutableName->Set((const unsigned char*)name, strlen(name) + 1);

    return HXR_OK;
}

STDMETHODIMP
ServerVersionWrapper::VersionString(REF(IHXBuffer*) pVersionString)
{
    const char* name = ServerVersion::VersionString();

    pVersionString = new ServerBuffer(TRUE);
    pVersionString->Set((const unsigned char*)name, strlen(name) + 1);

    return HXR_OK;
}

STDMETHODIMP
ServerVersionWrapper::MajorMinorString(REF(IHXBuffer*) pMajorMinorString)
{
    const char* name = ServerVersion::MajorMinorString();

    pMajorMinorString = new ServerBuffer(TRUE);
    pMajorMinorString->Set((const unsigned char*)name, strlen(name) + 1);

    return HXR_OK;
}

STDMETHODIMP
ServerVersionWrapper::PlatformName(REF(IHXBuffer*) pPlatformName)
{
    const char* name = ServerVersion::Platform();

    pPlatformName = new ServerBuffer(TRUE);
    pPlatformName->Set((const unsigned char*)name, strlen(name) + 1);

    return HXR_OK;
}

STDMETHODIMP_(UINT32)
ServerVersionWrapper::MajorVersion()
{
    return ServerVersion::MajorVersion();
}

STDMETHODIMP_(UINT32)
ServerVersionWrapper::MinorVersion()
{
    return ServerVersion::MinorVersion();
}

STDMETHODIMP
ServerVersionWrapper::GetVersion(REF(UINT32) ulMajor,
                                 REF(UINT32) ulMinor,
                                 REF(UINT32) ulSubMinor,
                                 REF(UINT32) ulSubSubMinor)
{
    // like 9.0.2.236
    const char* version = ServerVersion::VersionString();
    const char* pos;
    UINT32 subminor;

    pos = strchr(version, '.');
    if (pos) pos = strchr(pos+1, '.');

    if (pos)
    {
        ++pos;
        subminor = atol(pos);
        pos = strchr(pos, '.');
    }

    if (!pos) return HXR_FAIL;

    ulMajor = ServerVersion::MajorVersion();
    ulMinor = ServerVersion::MinorVersion();
    ulSubMinor = subminor;
    ulSubSubMinor = atol(pos+1);

    return HXR_OK;
}

STDMETHODIMP_(UINT32)
ServerVersionWrapper::GetVersion32()
{
    HX_RESULT status;
    UINT32 ulMajor;
    UINT32 ulMinor;
    UINT32 ulSubMinor;
    UINT32 ulSubSubMinor;
    UINT32 value = 0;

    status = GetVersion(ulMajor, ulMinor, ulSubMinor, ulSubSubMinor);

    if (SUCCEEDED(status))
    {
        value = (ulMajor << 28) | (ulMinor << 20) | (ulSubMinor << 12) |
            ulSubSubMinor;
    }

    return value;
}

    /*
     *	IUnknown methods
     */
STDMETHODIMP
ServerVersionWrapper::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXProductVersion))
    {
        AddRef();
        *ppvObj = (IHXProductVersion*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
ServerVersionWrapper::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
ServerVersionWrapper::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}

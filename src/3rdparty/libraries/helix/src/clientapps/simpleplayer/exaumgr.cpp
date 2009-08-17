/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#include <stdio.h>
#include "hxtypes.h"
#include "hxcom.h"
#include "hxauth.h"
#include "hxstrutl.h"
#include "exaumgr.h"
#include <ctype.h>
#include "print.h"


#include "globals.h"
struct _stGlobals*& GetGlobal(); //in main.cpp



ExampleAuthenticationManager::ExampleAuthenticationManager() :
    m_lRefCount(0),
    m_bSentPassword(FALSE)
{
}

ExampleAuthenticationManager::~ExampleAuthenticationManager()
{
}

STDMETHODIMP
ExampleAuthenticationManager::QueryInterface(REFIID riid, void**ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXAuthenticationManager*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXAuthenticationManager))
    {
        AddRef();
        *ppvObj = (IHXAuthenticationManager*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
ExampleAuthenticationManager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
ExampleAuthenticationManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
ExampleAuthenticationManager::HandleAuthenticationRequest(IHXAuthenticationManagerResponse* pResponse)
{
    char      username[1024] = ""; /* Flawfinder: ignore */
    char      password[1024] = ""; /* Flawfinder: ignore */
    HX_RESULT res = HXR_FAIL;
    
    if( !m_bSentPassword )
    {
        res = HXR_OK;
        if (GetGlobal()->bEnableVerboseMode)
            STDOUT("\nSending Username and Password...\n");

        SafeStrCpy(username,  GetGlobal()->g_pszUsername, 1024);
        SafeStrCpy(password,  GetGlobal()->g_pszPassword, 1024);

        //strip trailing whitespace
        char* c;
        for(c = username + strlen(username) - 1; 
            c > username && isspace(*c);
            c--)
            ;
        *(c+1) = 0;
    
        for(c = password + strlen(password) - 1; 
            c > password && isspace(*c);
            c--)
            ;
        *(c+1) = 0;
        
        m_bSentPassword = TRUE;
    }

    if (GetGlobal()->bEnableVerboseMode && FAILED(res) )
        STDOUT("\nInvalid Username and/or Password.\n");
    
    pResponse->AuthenticationRequestDone(res, username, password);
    return res;
}


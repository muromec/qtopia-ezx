/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: crdcache.h,v 1.8 2007/07/06 20:39:23 jfinnecy Exp $
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

#ifndef _CRDCACHE_H_
#define _CRDCACHE_H_

#include "hxcredc.h"
#include "pckunpck.h"

struct CredentialEntry
{
    CredentialEntry(char* pUserName, char* pPassword, IUnknown* pContext)
    {
	m_pUserName = NULL;
	m_pPassword = NULL;

	if (pUserName)
	{
	    CreateAndSetBufferCCF(m_pUserName, (UCHAR*)pUserName, strlen(pUserName)+1, pContext);
	}

	if (pPassword)
	{	    
	    CreateAndSetBufferCCF(m_pPassword, (UCHAR*)pPassword, strlen(pPassword)+1, pContext);
	}
    }

    ~CredentialEntry()
    {
	HX_RELEASE(m_pUserName);
	HX_RELEASE(m_pPassword);
    }

    IHXBuffer* m_pUserName;
    IHXBuffer* m_pPassword;
};

class CHXCredentialsCache : public IHXCredentialsCache
{   
private:
    IUnknown* m_pContext;
    INT32 m_lRefCount;
    CHXMapStringToOb m_credentialMap;

    ~CHXCredentialsCache();

  
public:
    CHXCredentialsCache(IUnknown* pContext);

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     *	IID_IHXCredentialsCache Methods
     */

    /************************************************************************
     *	Method:
     *	    IHXCredentialsCache::IsEmpty
     *
     *	Purpose:    
     *	    This function is used by the client core to determine whether the
     *	    credential info. has been saved already
     *
     */

    STDMETHOD_(HXBOOL, IsEmpty) (THIS_ IHXBuffer* pBuffer);

    /************************************************************************
     *	Method:
     *	    IHXCredentialsCache::Empty
     *
     *	Purpose:    
     *	    This function is used by the client core to remove the saved
     *	    credential info.
     *
     */
    STDMETHOD(Empty) (THIS_ IHXBuffer* pBuffer);

    /************************************************************************
     *	Method:
     *	    IHXCredentialsCache::FillCredentials
     *
     *	Purpose:    
     *	    This function is used by the client core to retrieve saved
     *	    credential info.
     *
     */
    STDMETHOD(FillCredentials) (THIS_ REF(IHXValues*) pValues);

    /************************************************************************
     *	Method:
     *	    IHXCredentialsCache::SetCredentials
     *
     *	Purpose:    
     *	    This function is used by the client core to store
     *	    credential info.
     *
     */
    STDMETHOD(SetCredentials) (THIS_ IHXValues* pValues);

    void Close();
};

#endif /* _CRDCACHE_H_ */

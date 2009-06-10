/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: dllaccesserver.cpp,v 1.5 2007/07/06 20:41:55 jfinnecy Exp $
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

/************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "dllaccesserver.h"
#include "hxassert.h"


/************************************************************************
 *  DLLAccessServer
 */
/************************************************************************
 *  Constuctore/Destructor
 */
DLLAccessServer::DLLAccessServer()
    : m_lRefCount(0)
{
    ;
}

DLLAccessServer::~DLLAccessServer()
{
    ;
}


/************************************************************************
 *  IHXDllAccess methods
 */
STDMETHODIMP DLLAccessServer::Open(const char* pDllName,
				   UINT16 uLibType)
{
    return (mDllAccess.open(pDllName, uLibType) == 0) ? HXR_OK : HXR_FAIL;
}

STDMETHODIMP DLLAccessServer::Close()
{
    return (mDllAccess.close() == DLLAccess::DLL_OK) ? HXR_OK : HXR_FAIL;
}

STDMETHODIMP_(void*) DLLAccessServer::GetSymbol(const char* pSymbolName)
{
    return mDllAccess.getSymbol(pSymbolName);
}

STDMETHODIMP_(ULONG32) DLLAccessServer::GetError()
{
    return mDllAccess.getError();
}

STDMETHODIMP_(const char*) DLLAccessServer::GetErrorString()
{
    return mDllAccess.getErrorString();
}

STDMETHODIMP_(const char*) DLLAccessServer::GetDllName()
{
    return mDllAccess.getDLLName();
}

STDMETHODIMP_(const char*) DLLAccessServer::GetDllVersion()
{
    return mDllAccess.getVersion();
}

STDMETHODIMP_(HXBOOL) DLLAccessServer::IsOpen()
{
    return mDllAccess.isOpen();
}

/************************************************************************
 *  IUnknown methods
 */
STDMETHODIMP DLLAccessServer::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
    {
	{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*) (IHXDllAccess*) this},
	{ GET_IIDHANDLE(IID_IHXDllAccess), (IHXDllAccess*) this},
    };

    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) DLLAccessServer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) DLLAccessServer::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

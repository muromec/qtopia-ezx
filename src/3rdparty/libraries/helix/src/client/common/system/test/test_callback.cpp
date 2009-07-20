/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: test_callback.cpp,v 1.3 2007/07/06 21:58:05 jfinnecy Exp $
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

#include "./test_callback.h"

#include "hx_ut_debug.h"
#include "./test_op.h"
#include "scheduler_test.h"

HXSchedulerTestCB::HXSchedulerTestCB(UINT32 operationID,
				     HXSchedulerTest* pTest,
				     TestOperation* pTestOp) :
    m_lRefCount(0),
    m_operationID(operationID),
    m_pTest(pTest),
    m_pTestOp(pTestOp)
{}

HXSchedulerTestCB::~HXSchedulerTestCB()
{
    delete m_pTestOp;
    m_pTestOp = 0;
}

STDMETHODIMP HXSchedulerTestCB::QueryInterface(THIS_ REFIID riid, 
					       void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXCallback))
    {
	AddRef();
	*ppvObj = (IHXCallback*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) HXSchedulerTestCB::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXSchedulerTestCB::Release(THIS)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP HXSchedulerTestCB::Func()
{
    HX_RESULT res = HXR_FAILED;

    DPRINTF(D_INFO,("HXSchedulerTestCB::Func()\n"));

    if (m_pTest->OnOperation(m_operationID))
    {
	if (m_pTestOp->Run(m_pTest))
	    res = HXR_OK;
	else
	{
	    DPRINTF(D_ERROR, ("HXSchedulerTestCB::Func() : test operation failed\n"));
	    m_pTest->OperationFailed();
	}
    }

    return res;
}

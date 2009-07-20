/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxminiccf.cpp,v 1.10 2005/06/27 14:22:28 ehyche Exp $
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

#include "chxminiccf.h"
#include "chxpckts.h"
#include "hxbuffer.h"
#include "hxtbuf.h"
#include "timebuff.h"
#include "hxlistp.h"

CHXMiniCCF::CHXMiniCCF() :
    m_lRefCount(0)
{}

CHXMiniCCF::~CHXMiniCCF()
{}

STDMETHODIMP CHXMiniCCF::QueryInterface(THIS_
				       REFIID riid,
				       void** ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), this },
		{ GET_IIDHANDLE(IID_IHXCommonClassFactory), (IHXCommonClassFactory*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) CHXMiniCCF::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXMiniCCF::Release(THIS)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP CHXMiniCCF::CreateInstance(THIS_
				       REFCLSID    /*IN*/  rclsid,
				       void**	    /*OUT*/ ppUnknown)
{
    HX_RESULT res = HXR_OUTOFMEMORY;
    *ppUnknown = NULL;

    if (IsEqualCLSID(rclsid, CLSID_IHXBuffer))
    {
	*ppUnknown = (IUnknown*)(IHXBuffer*)(new CHXBuffer());
    }
#if defined(HELIX_FEATURE_PLAYBACK_NET)
    else if (IsEqualCLSID(rclsid, CLSID_IHXTimeStampedBuffer))
    {
	*ppUnknown = (IUnknown*)(IHXTimeStampedBuffer*)(new CHXTimeStampedBuffer());
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_NET) */
    else if (IsEqualCLSID(rclsid, CLSID_IHXValues))
    {
	*ppUnknown = (IUnknown*)(IHXValues*)(new CHXHeader());
    }
    else if (IsEqualCLSID(rclsid, CLSID_IHXValues2))
    {
	*ppUnknown = (IUnknown*)(IHXValues2*)(new CHXHeader());
    }
#if defined(HELIX_FEATURE_PLAYBACK_NET)
    else if (IsEqualCLSID(rclsid, CLSID_IHXList))
    {
	*ppUnknown = (IUnknown*)(IHXList*)(new CHXList(NULL));
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_NET) */
    else
    {
	res = HXR_NOINTERFACE;
    }
    
    if (*ppUnknown)
    {
	((IUnknown*)*ppUnknown)->AddRef();
	res = HXR_OK;
    }

    return res;
}

STDMETHODIMP CHXMiniCCF::CreateInstanceAggregatable
				    (THIS_
				    REFCLSID	    /*IN*/  rclsid,
				    REF(IUnknown*)  /*OUT*/ ppUnknown,
				    IUnknown*	    /*IN*/  pUnkOuter)
{
    HX_RESULT res = HXR_NOINTERFACE;
    ppUnknown = NULL;

    if (IsEqualCLSID(rclsid, CLSID_IHXValues))
    {
        res = CHXHeader::CreateInstance(pUnkOuter, &ppUnknown);
    }

    return res;
}


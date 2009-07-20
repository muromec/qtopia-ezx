/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxaudvol.cpp,v 1.8 2005/03/14 20:32:05 bobclark Exp $
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

#include "hlxclib/stdio.h"
#include "hlxclib/string.h"

#include "hxresult.h"
#include "hxtypes.h"

#include "hxcom.h"
#include "hxausvc.h"
#include "hxaudvol.h"
#include "hxslist.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

/************************************************************************
 *  Method:
 *		IHXVolume::CHXVolume()
 *	Purpose:
 *		Constructor. Clean up and set free.
 */
CHXVolume::CHXVolume()
:	m_uVolume(HX_INIT_VOLUME)
,	m_lRefCount(0)
,	m_bMute(FALSE)
,	m_pSinkList(0)
{
    Init();
}

/************************************************************************
 *  Method:
 *		IHXVolume::~CHXVolume()
 *	Purpose:
 *		Destructor. Clean up and set free.
 */
CHXVolume::~CHXVolume()
{
    /* Delete items in lists */
    if ( m_pSinkList )
    {
	IHXVolumeAdviseSink* s = 0;
        CHXSimpleList::Iterator lIter = m_pSinkList->Begin();
        for (; lIter != m_pSinkList->End(); ++lIter)
        {
            s = (IHXVolumeAdviseSink*) (*lIter);
            if ( s )
	    {
                s->Release();
		s = 0;
	    }
        }
 	delete m_pSinkList;
	m_pSinkList = 0;
    }
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP CHXVolume::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXVolume), (IHXVolume*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXVolume*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32) CHXVolume::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32) CHXVolume::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *  IHXVolume methods
 */
/************************************************************************
 *  Method:
 *		IHXVolume::Init
 *	Purpose:
 *	
 */
STDMETHODIMP CHXVolume::Init()
{
    HX_RESULT theErr = HXR_OK;
    /* create sink lists. */
    m_pSinkList = new CHXSimpleList;
    if ( !m_pSinkList )
	theErr = HXR_OUTOFMEMORY;
    if ( !theErr )
    {
	if(!m_pSinkList->IsPtrListValid())
	    theErr = HXR_OUTOFMEMORY;
    }
    if ( !theErr )
	return HXR_OK;
    else
	return HXR_FAILED;
}

/************************************************************************
 *  Method:
 *		IHXVolume::SetVolume
 *	Purpose:
 *	
 */
STDMETHODIMP CHXVolume::SetVolume
(
    const	UINT16	uVolume
)
{
    m_uVolume   = uVolume;

    /* Call all advise sinks in list. */
    if ( m_pSinkList )
    {
	IHXVolumeAdviseSink* s = 0;
        CHXSimpleList::Iterator lIter = m_pSinkList->Begin();
        for (; lIter != m_pSinkList->End(); ++lIter)
        {
            s = (IHXVolumeAdviseSink*) (*lIter);
            if ( s )
                s->OnVolumeChange(uVolume);
        }
    }
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *		IHXVolume::GetVolume
 *	Purpose:
 *	
 */
STDMETHODIMP_(UINT16) CHXVolume::GetVolume()
{
    return m_uVolume;
}

/************************************************************************
 *  Method:
 *		IHXVolume::SetMute
 *	Purpose:
 *	
 */
STDMETHODIMP CHXVolume::SetMute
(
	const		HXBOOL	bMute									
)
{
    m_bMute = bMute;

    /* Call all advise sinks in list. */
    if ( m_pSinkList )
    {
	IHXVolumeAdviseSink* s = 0;
        CHXSimpleList::Iterator lIter = m_pSinkList->Begin();
        for (; lIter != m_pSinkList->End(); ++lIter)
        {
            s = (IHXVolumeAdviseSink*) (*lIter);
            if ( s )
                s->OnMuteChange(bMute);
        }
    }
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *		IHXVolume::GetMute
 *	Purpose:
 *	
 */
STDMETHODIMP_(HXBOOL) CHXVolume::GetMute()
{
    return m_bMute;
}

/************************************************************************
 *  Method:
 *		IHXVolume::AddAdviseSink
 *	Purpose:
 *	
 */
STDMETHODIMP CHXVolume::AddAdviseSink
(
    IHXVolumeAdviseSink*	pSink
)
{
    if (!pSink || !m_pSinkList)
    {
	return HXR_FAILED;	
    }

    /* Check if this one already exists */
    LISTPOSITION lPos = m_pSinkList->Find(pSink);
    if (lPos)
    {
	return HXR_FAILED;
    }

    pSink->AddRef();
    m_pSinkList->AddTail((void*) pSink);
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *		IHXVolume::RemoveAdviseSink
 *	Purpose:
 *	
 */
STDMETHODIMP CHXVolume::RemoveAdviseSink
(
    IHXVolumeAdviseSink*	pSink
)
{
    if (!pSink || !m_pSinkList)
    {
	return HXR_FAILED;
    }

    LISTPOSITION lpos = m_pSinkList->Find(pSink);
    if (lpos)
    {
 	m_pSinkList->RemoveAt(lpos);
	pSink->Release();
	return HXR_OK;
    }
    
    return HXR_FAILED;
}

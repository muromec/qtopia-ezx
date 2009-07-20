/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: wmbufctl.cpp,v 1.5 2007/07/06 21:58:12 jfinnecy Exp $
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

#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"

#include "wmbufctl.h"

#include "hxstring.h"
#include "hxbsrc.h" // HXSource

#include "hxtick.h"
#include "debug.h"
#define D_WM_BUF_CTL 0x10000000

/* Added these constants temporarily to tell ASM when to slow down
 * Accelerated buffering
 */
#define MAX_BUFFERING_INMS              20000
#define MAX_BUFFERING_INBYTES           4000000

/*
 * These constants are to tell ASM to slow down below the stated clip rate
 * if we have buffered an insane amount of data.
 * This can happen if the bitrate of a stream is signficantly lower at some
 * points then the actual clip rate (i.e. high bit-rate video).  These values
 * need to be VERY aggressive so we don't cut off things like flash which
 * will get WAY ahead and then spend the bits later on in the clip.
 */
#define MAX_BUFFERING_SLOW_HALF_INMS            300000
#define MAX_BUFFERING_SLOW_HALF_INBYTES         5000000

#define MAX_BUFFERING_SLOW_ONE_HUNDRETH_INMS    400000
#define MAX_BUFFERING_SLOW_ONE_HUNDRETH_INBYTES 6000000


HXWatermarkBufferControl::HXWatermarkBufferControl() :
    m_lRefCount(0),
    m_pSource(NULL),
    m_ChillState(HX_NONE)
{
    DPRINTF(D_WM_BUF_CTL, ("HXWatermarkBufferControl()\n"));
}
    
HXWatermarkBufferControl::~HXWatermarkBufferControl()
{
    HX_RELEASE(m_pSource);
}

/*
 *	IUnknown methods
 */
STDMETHODIMP HXWatermarkBufferControl::QueryInterface(THIS_
						      REFIID riid,
						      void** ppvObj)
{
    QInterfaceList qiList[] =
    {
	{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*)this }, 
	{ GET_IIDHANDLE(IID_IHXBufferControl), (IHXBufferControl*)this },
	{ GET_IIDHANDLE(IID_IHXWatermarkBufferControl), (IHXWatermarkBufferControl*)this }
    };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXWatermarkBufferControl::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXWatermarkBufferControl::Release(THIS)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 * IHXBufferControl method
 */

/************************************************************************
 *	Method:
 *	    IHXBufferControl::Init
 *	Purpose:
 *          Initialize the buffer control object with a context
 *          so it can find the interfaces it needs to do buffer
 *          control
 */
STDMETHODIMP HXWatermarkBufferControl::Init(THIS_ IUnknown* pContext)
{
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXBufferControl::OnBuffering
 *	Purpose:
 *          Called while buffering
 */
STDMETHODIMP HXWatermarkBufferControl::OnBuffering(UINT32 ulRemainingInMs,
						   UINT32 ulRemainingInBytes)
{
    return ResetAccelState();
}


/************************************************************************
 *	Method:
 *	    IHXBufferControl::OnBufferingDone
 *	Purpose:
 *      Called when buffering is done
 */
STDMETHODIMP HXWatermarkBufferControl::OnBufferingDone(THIS)
{
    return HXR_OK;
}


/************************************************************************
 *	Method:
 *	    IHXBufferControl::OnResume
 *	Purpose:
 *          Called when playback is resumed
 */
STDMETHODIMP HXWatermarkBufferControl::OnResume(THIS)
{
    return ResetAccelState();
}
    
/************************************************************************
 *	Method:
 *	    IHXBufferControl::OnPause
 *	Purpose:
 *          Called when playback is paused
 */
STDMETHODIMP HXWatermarkBufferControl::OnPause(THIS)
{
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXBufferControl::OnSeek
 *	Purpose:
 *          Called when a seek occurs
 */
STDMETHODIMP HXWatermarkBufferControl::OnSeek(THIS)
{
    return ResetAccelState();
}

/************************************************************************
 *	Method:
 *	    IHXBufferControl::OnClipEnd
 *	Purpose:
 *          Called when we get the last packet in the clip
 */
STDMETHODIMP HXWatermarkBufferControl::OnClipEnd(THIS)
{
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXBufferControl::Close()
 *	Purpose:
 *          Called when the owner of this object wishes to shutdown
 *          and destroy this object. This call causes the buffer control
 *          object to release all it's interfaces references.
 */
STDMETHODIMP HXWatermarkBufferControl::Close(THIS)
{
    ClearChillState();

    HX_RELEASE(m_pSource);

    return HXR_OK;
}

/*
 * IHXWatermarkBufferControl method
 */

/************************************************************************
 *	Method:
 *	    IHXWatermarkBufferControl::SetSource
 *	Purpose:
 *          Tells the object what HXSource object it is associated with.
 */
STDMETHODIMP HXWatermarkBufferControl::SetSource(THIS_ HXSource* pSource)
{
    HX_RESULT res = HXR_FAILED;

    HX_RELEASE(m_pSource);
    m_pSource = pSource;

    if( m_pSource)
    {
	m_pSource->AddRef();
	res = HXR_OK;
    }

    return res;
}

/************************************************************************
 *	Method:
 *	    IHXWatermarkBufferControl::OnBufferReport
 *	Purpose:
 *          Initiates control operations based on buffering information.
 *      
 */
STDMETHODIMP HXWatermarkBufferControl::OnBufferReport(THIS_ 
						      UINT32 ulBufferInMs,
						      UINT32 ulBuffer)
{
    DPRINTF(D_WM_BUF_CTL, ("WBC::OBR %u %u %u %u %s\n",
			   HX_GET_TICKCOUNT(),
			   ulBufferInMs,
			   ulBuffer,
			   m_ChillState,
			   (m_ChillState == HX_NONE) ? "NONE" :
			   (m_ChillState == CHILL) ? "CHILL" :
			   (m_ChillState == HALF) ? "HALF" :
			   (m_ChillState == HUNDRETH) ? "HUNDRETH" :
			   "UNKNOWN"));

    if (m_ChillState == HALF && 
	(ulBufferInMs > MAX_BUFFERING_SLOW_ONE_HUNDRETH_INMS ||
	 ulBuffer > MAX_BUFFERING_SLOW_ONE_HUNDRETH_INBYTES))
    {
	m_ChillState = HUNDRETH;
	ChangeAccelerationStatus(FALSE, TRUE, 1);
    }
    else if (m_ChillState == CHILL && 
	     (ulBufferInMs > MAX_BUFFERING_SLOW_HALF_INMS ||
	      ulBuffer > MAX_BUFFERING_SLOW_HALF_INBYTES))
    {
	m_ChillState = HALF;
	ChangeAccelerationStatus(FALSE, TRUE, 50);
    }
    /*
     * XXXSMP  Perhaps we want to stop at MAX_BUFFERING on the way back
     * to MAX_BUFFERING / 2 (In the case where we are at HALF or HUNDRETH.
     * This will prevent switch to CHILL and then HALF again on the way
     * back down.  No big deal for now.
     */
    else if (m_ChillState == HX_NONE &&
	     (ulBufferInMs > MAX_BUFFERING_INMS ||
	      ulBuffer > MAX_BUFFERING_INBYTES))
    {
	m_ChillState = CHILL;
	ChangeAccelerationStatus(FALSE, FALSE, 0);
    }
    else if (m_ChillState != HX_NONE &&
	     (ulBufferInMs < MAX_BUFFERING_INMS/2 &&
	      ulBuffer < MAX_BUFFERING_INBYTES/2))
    {
	m_ChillState = HX_NONE;
	ChangeAccelerationStatus(TRUE, FALSE, 0);
    }

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXWatermarkBufferControl::ClearChillState
 *	Purpose:
 *          Sets the chill state to NONE
 *      
 */
STDMETHODIMP HXWatermarkBufferControl::ClearChillState(THIS)
{
    m_ChillState = HX_NONE;

    return HXR_OK;
}


HX_RESULT
HXWatermarkBufferControl::ChangeAccelerationStatus(HXBOOL bMayBeAccelerated,
						   HXBOOL bUseAccelerationFactor,
						   UINT32 ulAccelerationFactor)
{
    IHXBandwidthManager* pMgr = 0;
    
    HX_RESULT res = m_pSource->QueryInterface(IID_IHXBandwidthManager, 
					      (void **)&pMgr);
    if (pMgr)
    {
	pMgr->ChangeAccelerationStatus(m_pSource, bMayBeAccelerated,
				       bUseAccelerationFactor, 
				       ulAccelerationFactor);
	pMgr->Release();
    }

    return res;
}

HX_RESULT HXWatermarkBufferControl::ResetAccelState()
{
    if (m_ChillState != HX_NONE)
    {
	m_ChillState = HX_NONE;
	ChangeAccelerationStatus(TRUE, FALSE, 0);
    }

    return HXR_OK;
}


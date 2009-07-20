/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxbufctl.h,v 1.6 2007/07/06 20:43:41 jfinnecy Exp $
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

#ifndef HXBUFCTL_H
#define HXBUFCTL_H

class HXSource;

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXBufferControl
 * 
 *  Purpose:
 *      Common interface for buffer control objects. These objects attempt
 *      to control a data source so that the amount of data buffered is 
 *      bounded.
 * 
 * 
 *  IID_IHXBufferControl:
 *  
 *	{68B2AEF9-1384-46ec-A4D0-00680A7DBBAE}
 *
 */

DEFINE_GUID(IID_IHXBufferControl, 0x68b2aef9, 0x1384, 0x46ec, 0xa4, 0xd0, 0x0,
	    0x68, 0xa, 0x7d, 0xbb, 0xae);

#undef  INTERFACE
#define INTERFACE   IHXBufferControl

DECLARE_INTERFACE_(IHXBufferControl, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXBufferControl methods
     */

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::Init
     *	Purpose:
     *      Initialize the buffer control object with a context
     *      so it can find the interfaces it needs to do buffer
     *      control
     */
    STDMETHOD(Init) (THIS_ IUnknown* pContext) PURE;

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::OnBuffering
     *	Purpose:
     *      Called while buffering
     */
    STDMETHOD(OnBuffering) (THIS_ UINT32 ulRemainingInMs,
			    UINT32 ulRemainingInBytes) PURE;

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::OnBufferingDone
     *	Purpose:
     *      Called when buffering is done
     */
    STDMETHOD(OnBufferingDone)(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::OnResume
     *	Purpose:
     *      Called when playback is resumed
     */
    STDMETHOD(OnResume) (THIS) PURE;
    
    /************************************************************************
     *	Method:
     *	    IHXBufferControl::OnPause
     *	Purpose:
     *      Called when playback is paused
     */
    STDMETHOD(OnPause) (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::OnSeek
     *	Purpose:
     *      Called when a seek occurs
     */
    STDMETHOD(OnSeek) (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::OnClipEnd
     *	Purpose:
     *      Called when we get the last packet in the clip
     */
    STDMETHOD(OnClipEnd) (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::Close()
     *	Purpose:
     *      Called when the owner of this object wishes to shutdown
     *      and destroy this object. This call causes the buffer control
     *      object to release all it's interfaces references.
     */
    STDMETHOD(Close)(THIS) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXWatermarkBufferControl
 * 
 *  Purpose:
 *      This buffer control object uses a set of watermarks to determine
 *      when it should adjust the source bandwidth. This is the buffer
 *      control algorithm historically used by the Helix Engine.
 * 
 * 
 *  IID_IHXWatermarkBufferControl:
 *  
 *	{68B2AEF9-1384-46ec-A4D0-00680A7DBBAF}
 *
 */

DEFINE_GUID(IID_IHXWatermarkBufferControl, 0x68b2aef9, 0x1384, 0x46ec, 0xa4, 
	    0xd0, 0x0, 0x68, 0xa, 0x7d, 0xbb, 0xaf);

#undef  INTERFACE
#define INTERFACE   IHXWatermarkBufferControl

DECLARE_INTERFACE_(IHXWatermarkBufferControl, IHXBufferControl)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXBufferControl methods
     */

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::Init
     *	Purpose:
     *      Initialize the buffer control object with a context
     *      so it can find the interfaces it needs to do buffer
     *      control
     */
    STDMETHOD(Init) (THIS_ IUnknown* pContext) PURE;

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::OnBuffering
     *	Purpose:
     *      Called while buffering
     */
    STDMETHOD(OnBuffering) (THIS_ UINT32 ulRemainingInMs,
			    UINT32 ulRemainingInBytes) PURE;

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::OnBufferingDone
     *	Purpose:
     *      Called when buffering is done
     */
    STDMETHOD(OnBufferingDone)(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::OnResume
     *	Purpose:
     *      Called when playback is resumed
     */
    STDMETHOD(OnResume) (THIS) PURE;
    
    /************************************************************************
     *	Method:
     *	    IHXBufferControl::OnPause
     *	Purpose:
     *      Called when playback is paused
     */
    STDMETHOD(OnPause) (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::OnSeek
     *	Purpose:
     *      Called when a seek occurs
     */
    STDMETHOD(OnSeek) (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::OnClipEnd
     *	Purpose:
     *      Called when we get the last packet in the clip
     */
    STDMETHOD(OnClipEnd) (THIS) PURE;
    
    /************************************************************************
     *	Method:
     *	    IHXBufferControl::Close()
     *	Purpose:
     *      Called when the owner of this object wishes to shutdown
     *      and destroy this object. This call causes the buffer control
     *      object to release all it's interfaces references.
     */
    STDMETHOD(Close)(THIS) PURE;

    /*
     * IHXWatermarkBufferControl methods
     */

    /************************************************************************
     *	Method:
     *	    IHXWatermarkBufferControl::SetSource
     *	Purpose:
     *      Tells the object what HXSource object it is associated with.
     */
    STDMETHOD(SetSource)(THIS_ HXSource* pSource) PURE;

    /************************************************************************
     *	Method:
     *	    IHXWatermarkBufferControl::OnBufferReport
     *	Purpose:
     *      Initiates control operations based on buffering information.
     *      
     */
    STDMETHOD(OnBufferReport)(THIS_ UINT32 ulBufferInMs,
			      UINT32 ulBuffer) PURE;

    /************************************************************************
     *	Method:
     *	    IHXWatermarkBufferControl::ClearChillState
     *	Purpose:
     *      Sets the chill state to NONE
     *      
     */
    STDMETHOD(ClearChillState)(THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXTransportBufferLimit
 * 
 *  Purpose:
 *      This interface allows you to control the number of bytes that
 *      are allowed to be buffered in the transport buffers. If the
 *      number of bytes exceeds the set limit, the packets will be replaced
 *      with loss packets.
 * 
 * 
 *  IID_IHXTransportBufferLimit:
 *  
 *	{68B2AEF9-1384-46ec-A4D0-00680A7DBBB0}
 *
 */

DEFINE_GUID(IID_IHXTransportBufferLimit, 0x68b2aef9, 0x1384, 0x46ec, 0xa4, 
	    0xd0, 0x0, 0x68, 0xa, 0x7d, 0xbb, 0xb0);

#undef  INTERFACE
#define INTERFACE   IHXTransportBufferLimit

DECLARE_INTERFACE_(IHXTransportBufferLimit, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXTransportBufferLimit methods
     */

    /************************************************************************
     *	Method:
     *	    IHXTransportBufferLimit::SetByteLimit
     *	Purpose:
     *      Sets the maximum number of bytes that can be buffered in the
     *      transport buffer. If incomming packets would put us over this
     *      limit, then they are replaced with lost packets. A byte limit
     *      of 0 means unlimited buffering.
     */
    STDMETHOD(SetByteLimit) (THIS_ UINT16 uStreamNumber, 
			     UINT32 uByteLimit) PURE;

    /************************************************************************
     *	Method:
     *	    IHXTransportBufferLimit::GetByteLimit
     *	Purpose:
     *      Returns the current byte limit in effect. A value of 0 means
     *      unlimited buffering is allowed
     */
    STDMETHOD_(UINT32,GetByteLimit) (THIS_ UINT16 uStreamNumber) PURE;
};

#endif /* HXBUFCTL_H */

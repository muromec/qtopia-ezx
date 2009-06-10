/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxasm.h,v 1.10 2007/07/06 20:43:41 jfinnecy Exp $
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

#ifndef _HXASM_H_
#define _HXASM_H_

/*
 * Forward declarations of some interfaces defined here-in.
 */
typedef _INTERFACE	IHXPacket			IHXPacket;
typedef _INTERFACE	IHXBackChannel			IHXBackChannel;
typedef _INTERFACE	IHXASMSource			IHXASMSource;
typedef _INTERFACE	IHXASMStreamSink		IHXASMStreamSink;
typedef _INTERFACE	IHXASMStream2			IHXASMStream2;
typedef _INTERFACE	IHXASMStream			IHXASMStream;


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXBackChannel
 * 
 *  Purpose:
 * 
 *      Backchannel interface to be used by renderers and implemented by
 *	FileFormat Plugins
 * 
 *  IID_IHXBackChannel:
 *  
 *	{00001500-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXBackChannel, 0x00001500, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
				 0xa0, 0x24, 0x40, 0x6d, 0x59);
#undef  INTERFACE
#define INTERFACE   IHXBackChannel

DECLARE_INTERFACE_(IHXBackChannel, IUnknown)
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
     * IHXBackChannel method
     */

    /************************************************************************
     *	Method:
     *	    IHXBackChannel::PacketReady
     *	Purpose:
     *      A back channel packet sent from Renderer to FileFormat plugin.
     */
    STDMETHOD(PacketReady)	(THIS_
				IHXPacket* pPacket) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXASMSource
 *
 *  Purpose:
 *
 *      This interface is implemented by file formats so that they can
 *	act on ASM Subscribe and Unsubscribe actions.
 *
 *  IID_IHXASMSource:
 *
 *	{00001501-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXASMSource, 0x00001501, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
				 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXASMSource

DECLARE_INTERFACE_(IHXASMSource, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXASMSource methods
     */

    /************************************************************************
     *	Method:
     *	    IHXASMSource::Subscribe
     *	Purpose:
     *      Called to inform a file format that a subscription has occurred,
     *	    to rule number uRuleNumber, for stream uStreamNumber.
     */
    STDMETHOD(Subscribe)	(THIS_
				UINT16	uStreamNumber,
				UINT16	uRuleNumber) PURE;

    /************************************************************************
     *	Method:
     *	    IHXASMSource::Unsubscribe
     *	Purpose:
     *      Called to inform a file format that a unsubscription has occurred,
     *	    to rule number uRuleNumber, for stream uStreamNumber.
     */
    STDMETHOD(Unsubscribe)	(THIS_
				UINT16	uStreamNumber,
				UINT16	uRuleNumber) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXASMStream
 *
 *  Purpose:
 *	This interface is implemented by the client core.  Top level clients
 *	renderers, etc can query for this interface off of IHXStream.  This
 *	interface allows you to subscribe and unsubscribe to certain rules,
 *	and it also allows you to add a advise sink for these events. 
 *
 *  IID_IHXASMStream:
 *
 *	{00001502-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXASMStream, 0x00001502, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
				 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXASMStream

DECLARE_INTERFACE_(IHXASMStream, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXASMStream methods
     */

    /************************************************************************
     *	Method:
     *	    IHXASMStream::AddASMStreamSink
     *	Purpose:
     *	    Add an advise sink for getting subscribe and unsubscribe
     *	    notifications.
     */
    STDMETHOD(AddStreamSink)	(THIS_
				IHXASMStreamSink*	pASMStreamSink) PURE;

    /************************************************************************
     *	Method:
     *	    IHXASMStream::RemoveStreamSink
     *	Purpose:
     *	    Remove an advise sink for getting subscribe and unsubscribe
     *	    notifications.
     */
    STDMETHOD(RemoveStreamSink)	(THIS_
				IHXASMStreamSink*	pASMStreamSink) PURE;

    /************************************************************************
     *	Method:
     *	    IHXASMStream::Subscribe
     *	Purpose:
     *	    Called by renderers and possibly even top level clients to
     *	    inform the core to subscribe to a particular rule number for
     *	    this stream.
     */
    STDMETHOD(Subscribe)	(THIS_
				UINT16	uRuleNumber) PURE;

    /************************************************************************
     *	Method:
     *	    IHXASMStream::Unsubscribe
     *	Purpose:
     *	    Called by renderers and possibly even top level clients to
     *	    inform the core to unsubscribe to a particular rule number for
     *	    this stream.
     */
    STDMETHOD (Unsubscribe)	(THIS_
				UINT16	uRuleNumber) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXASMStream2
 *
 *  Purpose:
 *	This interface is implemented by the client core.  Top level clients
 *	renderers, etc can query for this interface off of IHXStream.  This
 *	interface allows you to disable and re-enable certain rules.
 *
 *  IID_IHXASMStream2:
 *
 *	{00001504-0901-11d1-8b06-00a024406d59}
 *
 */
DEFINE_GUID(IID_IHXASMStream2, 0x00001504, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
				 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXASMStream2

DECLARE_INTERFACE_(IHXASMStream2, IHXASMStream)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXASMStream2 methods
     */

    /************************************************************************
     *	Method:
     *	    IHXASMStream2::Disable 
     *	Purpose:
     *	    Called by RA renderer to inform the core to disable a 
     *	    particular rule for this stream.
     */
    STDMETHOD(Disable)	(THIS_
				UINT16	uRuleNumber) PURE;
    /************************************************************************
     *	Method:
     *	    IHXASMStream2::Enable
     *	Purpose:
     *	    Called by RA renderer to inform the core to enable a 
     *	    particular rule for this stream.
     */
    STDMETHOD (Enable)	(THIS_
				UINT16	uRuleNumber) PURE;

    STDMETHOD (ReCompute)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXASMStream2::IsEnabled
     *	Purpose:
     *      Allows a user to determine if a rule is enabled or not
     */
    STDMETHOD_(HXBOOL,IsEnabled) (THIS_
				UINT16	uRuleNumber) PURE;

    /************************************************************************
     *	Method:
     *	    IHXASMStream2::LockSubscriptions
     *	Purpose:
     *      Locks the stream to the current subscriptions. This has the
     *      effect of disabling the client side rate adaptation for this
     *      stream.
     */
    STDMETHOD (LockSubscriptions)(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXASMStream2::UnlockSubscriptions
     *	Purpose:
     *      Unlocks the stream so that the enabled streams can be selected
     *      by the ASM code. This has the effect of reenabling the client side
     *      rate adaptation for this stream.
     */
    STDMETHOD (UnlockSubscriptions)(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXASMStream2::AreSubscriptionsLocked
     *	Purpose:
     *      Lets you determine whether the stream subscriptions are locked
     *      or not.
     */
    STDMETHOD_(HXBOOL, AreSubscriptionsLocked)(THIS) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXASMStreamSink
 *
 *  Purpose:
 *	This is a advise sink for getting notification about subscriptions
 *	and unsubscriptions for a stream.
 *
 *  IID_IHXASMStream:
 *
 *	{00001503-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXASMStreamSink, 0x00001503, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
				 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXASMStreamSink

DECLARE_INTERFACE_(IHXASMStreamSink, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXASMStreamSink methods
     */

    /************************************************************************
     *	Method:
     *	    IHXASMStreamSink::OnSubscribe
     *	Purpose:
     *	    Called to inform you that a subscribe has occurred.
     */
    STDMETHOD (OnSubscribe)	(THIS_
				UINT16	uRuleNumber) PURE;

    /************************************************************************
     *	Method:
     *	    IHXASMStreamSink::OnUnsubscribe
     *	Purpose:
     *	    Called to inform you that a unsubscribe has occurred.
     */
    STDMETHOD (OnUnsubscribe)	(THIS_
				UINT16	uRuleNumber) PURE;
};

#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXBackChannel)
DEFINE_SMART_PTR(IHXASMSource)
DEFINE_SMART_PTR(IHXASMStream)
DEFINE_SMART_PTR(IHXASMStream2)
DEFINE_SMART_PTR(IHXASMStreamSink)

#endif /*_HXASM_H_*/

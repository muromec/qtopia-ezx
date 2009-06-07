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

/*
 *
 *  XXXJHUG, this interface exists soly for the Image Map renderer to be able
 *  to cleanly get events from the RealVideo renderer.
 *
 *  The perfered method is to use smil.  When smil is used, a renderer can 
 *  register it self as an IHXGroupSink to be notified of the playto region's
 *  and url's being started.  Once it has the name of the site, it can create
 *  a IHXEventHook, with the IHXEventHookMgr, using the name of the "playto"
 *  it wants to recieve events for.
 *
 */

#ifndef _RVSINK_H_
#define _RVSINK_H_

typedef _INTERFACE  IHXBuffer			IHXBuffer;

/****************************************************************************
 * 
 *  Interface:
 *	IRVSink
 *  
 *	IID_IRVSink:
 *
 * {A57FD430-4599-11d4-B423-009027430F4C}
 *
 */
DEFINE_GUID(IID_IRVSink, 
0xa57fd430, 0x4599, 0x11d4, 0xb4, 0x23, 0x0, 0x90, 0x27, 0x43, 0xf, 0x4c);

#undef INTERFACE
#define INTERFACE IRVSink

DECLARE_INTERFACE_(IRVSink, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_
					 REFIID riid,
					 void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    /*
     *	IRVSink methods
     */

    STDMETHOD(OnEvent)(HXxEvent* /*IN*/ pEvent) PURE;
    STDMETHOD(Close)(THIS) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *	IRVSinkMgr
 *  
 *	IID_IRVSinkMgr:
 *
 * {A57FD431-4599-11d4-B423-009027430F4C}
 *
 */
DEFINE_GUID(IID_IRVSinkMgr, 
0xa57fd431, 0x4599, 0x11d4, 0xb4, 0x23, 0x0, 0x90, 0x27, 0x43, 0xf, 0x4c);

#undef INTERFACE
#define INTERFACE IRVSinkMgr

DECLARE_INTERFACE_(IRVSinkMgr, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_
					 REFIID riid,
					 void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    /*
     *	IRVSinkMgr methods
     */

    STDMETHOD(AddRVEventSink)(IRVSink* pSink) PURE;

    STDMETHOD(GetSiteSize)(REF(HXxRect) size) PURE;
};

#endif // _RVSINK_H_


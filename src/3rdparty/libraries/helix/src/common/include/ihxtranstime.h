/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: ihxtranstime.h,v 1.1 2004/06/24 00:09:10 acolwell Exp $ 
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
#ifndef IHXTRANSTIME_H
#define IHXTRANSTIME_H

/****************************************************************************
 * 
 *  Interface:
 *
 *	IID_IHXTransportTimeSink
 *
 *  Purpose:
 *
 *	This interface is used to send OnTimeSync() calls down
 *      to the transport layer. Note that the value passed to
 *      OnTransportTime() MUST be in the same units and have the same
 *      offset as the packets coming from the transport. This means
 *      that any offets caused by SMIL or live playback must be
 *      removed before calling OnTransportTime() on this interface
 *
 *      IID_IHXTransportTimeSink
 *
 *	{DB838AB3-4637-41e0-BC3F-ED03B6A684C1}
 *
 */
DEFINE_GUID(IID_IHXTransportTimeSink, 
0xdb838ab3, 0x4637, 0x41e0, 0xbc, 0x3f, 0xed, 0x3, 0xb6, 0xa6, 0x84, 0xc1);

DECLARE_INTERFACE_(IHXTransportTimeSink, IUnknown)
{
    /*
     *	IUnknown methods
     */

    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXTransportTimeSink methods
     */
    STDMETHOD(OnTransportTime) (THIS_ UINT32 ulCurrentTime) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IID_IHXTransportTimeManager
 *
 *  Purpose:
 *
 *      This interface is used to register IHXTransportTimeSink interfaces
 *      with a source of transport time.
 *
 *      IID_IHXTransportTimeManager
 *
 *	{D0A5BA01-EDFB-4741-B709-884E685D2D08}
 *
 */
DEFINE_GUID(IID_IHXTransportTimeManager, 
0xd0a5ba01, 0xedfb, 0x4741, 0xb7, 0x9, 0x88, 0x4e, 0x68, 0x5d, 0x2d, 0x8);


DECLARE_INTERFACE_(IHXTransportTimeManager, IUnknown)
{
    /*
     *	IUnknown methods
     */

    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXTransportTimeManager methods
     */
    STDMETHOD(AddSink) (THIS_ IHXTransportTimeSink* pSink) PURE;
    STDMETHOD(RemoveSink) (THIS_ IHXTransportTimeSink* pSink) PURE;
};

#endif /* IHXTRANSTIME_H */

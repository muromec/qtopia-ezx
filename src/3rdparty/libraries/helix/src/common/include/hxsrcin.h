/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsrcin.h,v 1.6 2008/11/12 13:32:27 ehyche Exp $
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

#ifndef _HXSRCIN_H_
#define _HXSRCIN_H_

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxcom.h"

#include "ihxpckts.h"

#define DTDR_TAG "DTDR"
#define APPLICATION_DATA "ApplicationData"

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSourceInput
 *
 *  Purpose:
 *
 *	Media Source Flow Bridge Interface:  conveys source accross components
 *  (Source Handlers).
 *
 *  IID_IHXSourceInput
 *
 *	{EBF8D220-40F7-11d6-AB3F-00010251B340}
 *
 */
DEFINE_GUID(IID_IHXSourceInput, 0xebf8d220, 0x40f7, 0x11d6, 0xab, 0x3f, 
	    0x0, 0x1, 0x2, 0x51, 0xb3, 0x40);

#undef  INTERFACE
#define INTERFACE   IHXSourceInput

DECLARE_INTERFACE_(IHXSourceInput, IUnknown)
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
     *	IHXSourceInput methods
     */

    /************************************************************************
     *	Method:
     *	    IHXSourceInput::OnFileHeader
     *	Purpose:
     *	    Called to send file header information
     *
     */
    STDMETHOD(OnFileHeader)	(THIS_
				HX_RESULT   status,
				IHXValues* pValues) PURE;

    /************************************************************************
     *	Method:
     *	    IHXSourceInput::OnStreamHeader
     *	Purpose:
     *	    Called to send stream header information	
     *
     */
    STDMETHOD(OnStreamHeader)	(THIS_
				HX_RESULT   status,
				IHXValues* pValues) PURE;

    /************************************************************************
     *	Method:
     *	    IHXSourceInput:OnStreamDone
     *	Purpose:
     *	    Called to signal the end of particular stream from the source.
     */
    STDMETHOD(OnStreamDone)	(THIS_
				HX_RESULT   status,
				UINT16	    unStreamNumber) PURE;

    /************************************************************************
     *	Method:
     *	    IHXSourceInput:OnPacket
     *	Purpose:
     *	    Called to send packet information.
     *
     */
    STDMETHOD(OnPacket)		(THIS_
				HX_RESULT   status,
				IHXPacket* pPacket) PURE;

    /************************************************************************
     *	Method:
     *	    IHXSourceInput:OnTermination
     *	Purpose:
     *	    Called to indicate the source is terminated and receiver is
     *	to release all its resources required for processing source input.
     *  Any input performed after this call is to be ignored.
     *
     */
    STDMETHOD(OnTermination)	(THIS_
				HX_RESULT status) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSourceHandler
 *
 *  Purpose:
 *
 *	Media Source Flow Handler Interface: process media source streams
 *
 *  IID_IHXSourceInput
 *
 *	{B2646DA0-410A-11d6-AB3F-00010251B340}
 *
 */
DEFINE_GUID(IID_IHXSourceHandler, 
0xb2646da0, 0x410a, 0x11d6, 0xab, 0x3f, 0x0, 0x1, 0x2, 0x51, 0xb3, 0x40);

#define CLSID_IHXSourceHandler IID_IHXSourceHandler

#undef  INTERFACE
#define INTERFACE   IHXSourceHandler

DECLARE_INTERFACE_(IHXSourceHandler, IHXSourceInput)
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
     *	IHXSourceInput methods
     */

    /************************************************************************
     *	Method:
     *	    IHXSourceInput::OnFileHeader
     *	Purpose:
     *	    Called to send file header information
     *
     */
    STDMETHOD(OnFileHeader)	(THIS_
				HX_RESULT   status,
				IHXValues* pValues) PURE;

    /************************************************************************
     *	Method:
     *	    IHXSourceInput::OnStreamHeader
     *	Purpose:
     *	    Called to send stream header information	
     *
     */
    STDMETHOD(OnStreamHeader)	(THIS_
				HX_RESULT   status,
				IHXValues* pValues) PURE;

    /************************************************************************
     *	Method:
     *	    IHXSourceInput:OnStreamDone
     *	Purpose:
     *	    Called to signal the end of particular stream from the source.
     */
    STDMETHOD(OnStreamDone)	(THIS_
				HX_RESULT   status,
				UINT16	    unStreamNumber) PURE;

    /************************************************************************
     *	Method:
     *	    IHXSourceInput:OnPacket
     *	Purpose:
     *	    Called to send packet information.
     *
     */
    STDMETHOD(OnPacket)		(THIS_
				HX_RESULT   status,
				IHXPacket* pPacket) PURE;

    /************************************************************************
     *	Method:
     *	    IHXSourceInput:OnTermination
     *	Purpose:
     *	    Called to indicate the source is terminated and receiver is
     *	to release all its resources required for processing source input.
     *  Any input performed after this call is to be ignored.
     *
     */
    STDMETHOD(OnTermination)	(THIS_
				HX_RESULT status) PURE;

    /*
     *	IHXSourceHandler methods
     */

    /************************************************************************
     *	Method:
     *	    InitSourceHandler:InitSourceHandler
     *	Purpose:
     *	    Called to provide configuration options and processed source sink.
     *
     */
    STDMETHOD(InitSourceHandler)(THIS_
				IHXSourceInput* pSourceSink,
				IHXValues* pOptions) PURE;

    /************************************************************************
     *	Method:
     *	    InitSourceHandler:Close
     *	Purpose:
     *	    Called to relase all resources used by the source handler.
     *
     */
    STDMETHOD(Close)		(THIS) PURE;
};

#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXSourceInput)
DEFINE_SMART_PTR(IHXSourceHandler)

#endif  // _HXSRCIN_H_

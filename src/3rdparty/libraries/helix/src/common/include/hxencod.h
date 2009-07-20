/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxencod.h,v 1.4 2007/07/06 20:43:41 jfinnecy Exp $
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

#ifndef _HXENCOD_H_
#define _HXENCOD_H_

typedef _INTERFACE IUnknown			IUnknown;
typedef _INTERFACE IHXValues			IHXValues;
typedef _INTERFACE IHXPacket			IHXPacket;
typedef _INTERFACE IHXRequest			IHXRequest;
typedef _INTERFACE IHXEncoder			IHXEncoder;
typedef _INTERFACE IHXEncoderResponse		IHXEncoderResponse;
typedef _INTERFACE IHXEncoderCompletion	IHXEncoderCompletion;
typedef _INTERFACE IHXEncoderResponseCompletion IHXEncoderResponseCompletion;
typedef _INTERFACE IHXConnectionlessControl	IHXConnectionlessControl;
typedef _INTERFACE IHXTransportControl		IHXTransportControl;

#ifndef _MACINTOSH
STDAPI_(IUnknown*) CreateContext();
#else
#pragma export on
STDAPI_(IUnknown*) CreateContext();
#pragma export off
#endif


DEFINE_GUID(IID_IHXEncoderResponse, 	0x00001600, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef INTERFACE
#define INTERFACE IHXEncoderResponse

DECLARE_INTERFACE_(IHXEncoderResponse, IUnknown)
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
     * IHXEncoderResponse methods
     */
    STDMETHOD(InitEncoderResponse) (THIS_
				    const char* pHost,
				    UINT16 unPort, 
				    IHXRequest* pRequest,
				    const char* pUsername,
				    const char* pPassword,
				    IHXEncoder* pEncoder) PURE;

    STDMETHOD(FileHeaderReady)     (THIS_ 
				    HX_RESULT   result, 
				    IHXValues* pHeader) PURE;

    STDMETHOD(StreamHeaderReady)   (THIS_ 
				    HX_RESULT   result, 
				    IHXValues* pHeader) PURE;

    STDMETHOD(PacketReady)         (THIS_ 
				    HX_RESULT   result,
				    IHXPacket* pPacket) PURE;

    STDMETHOD(StreamDone)          (THIS_ 
				    UINT16 unStream) PURE;

    STDMETHOD(Process)             (THIS) PURE;

    STDMETHOD_(UINT32,GetTime)     (THIS) PURE;
};

DEFINE_GUID(IID_IHXEncoder,		0x00001601, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef INTERFACE
#define INTERFACE IHXEncoder

DECLARE_INTERFACE_(IHXEncoder, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					 REFIID riid,
					 void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     * IHXEncoder methods
     */
    STDMETHOD(InitEncoderResponseDone) (THIS_ 
					HX_RESULT result) PURE;

    STDMETHOD(GetFileHeader)           (THIS) PURE;

    STDMETHOD(GetStreamHeader)         (THIS_ 
					UINT16 unStream) PURE;

    STDMETHOD(StartPackets)            (THIS_ 
					UINT16 unStream) PURE;

    STDMETHOD(StopPackets)             (THIS_ 
					UINT16 unStream) PURE;
};

DEFINE_GUID(IID_IHXEncoderCompletion,	0x00001602, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef INTERFACE
#define INTERFACE IHXEncoderCompletion

DECLARE_INTERFACE_(IHXEncoderCompletion, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					 REFIID riid,
					 void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     * IHXEncoderCompletion methods
     */

    STDMETHOD(EncoderDone)		(THIS_ 
					 HX_RESULT result) PURE;
};

DEFINE_GUID(IID_IHXConnectionlessControl,	0x00001603, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef INTERFACE
#define INTERFACE IHXConnectionlessControl

DECLARE_INTERFACE_(IHXConnectionlessControl, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					 REFIID riid,
					 void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     * IHXConnectionlessControl methods
     */

    STDMETHOD(EnableConnectionlessControl)
					(THIS) PURE;

    STDMETHOD(ConnectionCheckFailed)	(THIS_
					HX_RESULT status) PURE;

    STDMETHOD(SetConnectionTimeout)	(THIS_
					UINT32 uSeconds) PURE;
};

DEFINE_GUID(IID_IHXEncoderResponseCompletion,	0x00001604, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef INTERFACE
#define INTERFACE IHXEncoderResponseCompletion

DECLARE_INTERFACE_(IHXEncoderResponseCompletion, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					 REFIID riid,
					 void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     * IHXEncoderResponseCompletion methods
     */

    STDMETHOD(EncoderResponseDone)	(THIS) PURE;
};

/*
 * The only 2 encoder transport types supported are:
 * "x-pn-tng/udp"
 * "x-pn-tng/tcp"
 */

DEFINE_GUID(IID_IHXTransportControl,	0x00001605, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef INTERFACE
#define INTERFACE IHXTransportControl

DECLARE_INTERFACE_(IHXTransportControl, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					 REFIID riid,
					 void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     * IHXTransportControl methods
     */

    STDMETHOD(SetTransportType)	(const char* pTransportType) PURE;
};

#endif /* _HXENCOD_H_ */

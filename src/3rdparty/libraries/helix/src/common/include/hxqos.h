/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxqos.h,v 1.13 2008/02/27 09:19:57 dsingh Exp $
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

#ifndef _HX_QOS_H_
#define _HX_QOS_H_

/* All below RR/RS values are accordign to 3GPP spec "3GPP TS 26.234 V6.10.0 (2006-12)"
 * Section 5.3.3.1
 */
#define MAX_3GPP_RTCP_RR 5000 // in bits/sec
#define MAX_3GPP_RTCP_RS 4000 // in bits/sec
#define DEFAULT_3GPP_RTCP_RR_PERCENTAGE 3.75
#define DEFAULT_3GPP_RTCP_RS_PERCENTAGE 1.25

#include "chxmapstringtoob.h" //This is required because forward declaration CHXMapStringToOb::Iterator is not possible. See IHXQoSProfileSelector.

/* Foreward Declaration of Interfaces */
typedef _INTERFACE IHXBuffer                 IHXBuffer;
typedef _INTERFACE IHXSocket                 IHXSocket;
typedef _INTERFACE IHXSetSocketOption        IHXSetSocketOption;
typedef _INTERFACE IHXQoSSignalBusController IHXQoSSignalBusController;
typedef _INTERFACE IHXQoSSignalSource        IHXQoSSignalSource;
typedef _INTERFACE IHXQoSSignalSink          IHXQoSSignalSink;
typedef _INTERFACE IHXQoSSignalBus           IHXQoSSignalBus;
typedef _INTERFACE IHXQoSSignalFilterTool    IHXQoSSignalFilterTool;
typedef _INTERFACE IHXQoSSignal              IHXQoSSignal;
typedef _INTERFACE IHXQoSDiffServConfigurator IHXQoSDiffServConfigurator;

typedef UINT16 HX_QOS_SIGNAL;

typedef enum _HX_QOS_DIFFSERV_CLASS
{
    HX_QOS_DIFFSERV_CLASS_MEDIA,
    HX_QOS_DIFFSERV_CLASS_CONTROL,
    HX_QOS_DIFFSERV_CLASS_ADMIN,
    HX_QOS_DIFFSERV_CLASS_DIST,
    HX_QOS_DIFFSERV_CLASS_COUNT
} HX_QOS_DIFFSERV_CLASS;

/*
 * IHXUserAgentSettings
 * {75FE922B-3C26-41b3-AAAA-58244D1B7503}
 */
DEFINE_GUID(IID_IHXUserAgentSettings,
            0x75fe922b, 0x3c26, 0x41b3, 0xaa, 0xaa, 0x58, 0x24, 0x4d, 0x1b, 0x75, 0x3);
#undef  INTERFACE
#define INTERFACE IHXUserAgentSettings
DECLARE_INTERFACE_(IHXUserAgentSettings, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)   ( THIS_ REFIID riid, void** ppvObj ) PURE;
    STDMETHOD_(ULONG32,AddRef)  ( THIS ) PURE;
    STDMETHOD_(ULONG32,Release) ( THIS ) PURE;

    // IHXUserAgentSettings
    STDMETHOD_(UINT32,GetPropertyID) (THIS_ const char* pszPropPath) PURE;
};


/*
 * IHXQoSProfileSelector
 * {75DB043B-C5A8-49b2-8D3F-8CF99F9E6444}
 */
DEFINE_GUID(IID_IHXQoSProfileSelector,
            0x75db043b, 0xc5a8, 0x49b2, 0x8d, 0x3f, 0x8c, 0xf9, 0x9f, 0x9e, 0x64, 0x44);

#undef  INTERFACE
#define INTERFACE   IHXQoSProfileSelector
DECLARE_INTERFACE_(IHXQoSProfileSelector, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)   ( THIS_ REFIID riid, void** ppvObj ) PURE;
    STDMETHOD_(ULONG32,AddRef)  ( THIS ) PURE;
    STDMETHOD_(ULONG32,Release) ( THIS ) PURE;

    // IHXQoSProfileSelector
    STDMETHOD (SelectProfile)   (THIS_ IHXBuffer* pUserAgent,
                                 IHXBuffer* pTransportMime,
                                 IHXBuffer* pMediaMime,
                                 REF(IHXUserAgentSettings*) /*OUT*/ pUAS) PURE;

    //Dereferencing the iterator returned by GetBegin method gives a pointer 
    //to IHXUserAgentSettings interface. This method can be used for iterating over
    //all UAS objects in the UASConfigTree. 
    STDMETHOD_(CHXMapStringToOb::Iterator,GetBegin)( THIS ) PURE;

    //The iterator returned by GetEnd method should only be used for 
    //testing whether or not the iterator returned by GetBegin has reached
    //the end of the list of UAS objects from the UASConfigTree.
    //It should never be dereferenced.
    STDMETHOD_(CHXMapStringToOb::Iterator,GetEnd)( THIS ) PURE;
};

/*
 * IHXQoSProfileConfigurator
 * {75DB043B-C5A8-49b2-8D3F-8CF99F9E6448}
 */
DEFINE_GUID(IID_IHXQoSProfileConfigurator,
            0x75db043b, 0xc5a8, 0x49b2, 0x8d, 0x3f, 0x8c, 0xf9, 0x9f, 0x9e, 0x64, 0x48);

#undef  INTERFACE
#define INTERFACE   IHXQoSProfileConfigurator
DECLARE_INTERFACE_(IHXQoSProfileConfigurator, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)   ( THIS_ REFIID riid, void** ppvObj ) PURE;
    STDMETHOD_(ULONG32,AddRef)  ( THIS ) PURE;
    STDMETHOD_(ULONG32,Release) ( THIS ) PURE;

    // IHXQoSProfileConfigurator
    STDMETHOD (SetUserAgentSettings)(THIS_ IHXUserAgentSettings* pUAS) PURE;
    STDMETHOD (GetUserAgentSettings)(THIS_ REF(IHXUserAgentSettings*) /*OUT*/ pUAS) PURE;
    STDMETHOD (GetConfigInt)    (THIS_ const char* pName, REF(INT32) /*OUT*/ lValue) PURE;
    STDMETHOD (GetConfigBuffer) (THIS_ const char* pName, REF(IHXBuffer*) /*OUT*/ pValue) PURE;
};

/*
 * IHXQoSSignalSourceResponse
 * {B6154B09-BBC3-4239-BE8B-81607CA0BE09}
 */
DEFINE_GUID(IID_IHXQoSSignalSourceResponse,
            0xb6154b09, 0xbbc3, 0x4239, 0xbe, 0x8b, 0x81, 0x60, 0x7c, 0xa0, 0xbe, 0x9);

#undef  INTERFACE
#define INTERFACE   IHXQoSSignalSourceResponse
DECLARE_INTERFACE_(IHXQoSSignalSourceResponse, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)           ( THIS_ REFIID riid, void** ppvObj ) PURE;
    STDMETHOD_(ULONG32,AddRef)          ( THIS ) PURE;
    STDMETHOD_(ULONG32,Release)         ( THIS ) PURE;

    // IHXQoSSignalSourceResponse
    STDMETHOD (SignalBusReady)(THIS_ HX_RESULT hResult, IHXQoSSignalBus* pBus,
                               IHXBuffer* pSessionId) PURE;
};

/*
 * IHXQoSSignalSource
 * {42AEDDAE-3C4A-498c-863E-E6EDEED402A5}
 */

DEFINE_GUID(IID_IHXQoSSignalSource,
            0x42aeddae, 0x3c4a, 0x498c, 0x86, 0x3e, 0xe6, 0xed, 0xee, 0xd4, 0x2, 0xa5);

#undef  INTERFACE
#define INTERFACE   IHXQoSSignalSource
DECLARE_INTERFACE_(IHXQoSSignalSource, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)           ( THIS_ REFIID riid, void** ppvObj ) PURE;
    STDMETHOD_(ULONG32,AddRef)          ( THIS ) PURE;
    STDMETHOD_(ULONG32,Release)         ( THIS ) PURE;

    // IHXQoSSignalSource
    STDMETHOD (GetSignalBus) (THIS_ IHXBuffer* pSessionId,
                              IHXQoSSignalSourceResponse* pResp) PURE;
    STDMETHOD (ReleaseResponseObject) (THIS_ IHXBuffer* pSessionId,
                              IHXQoSSignalSourceResponse* pResp) PURE;
};

/*
 * IHXQoSSignalSink
 * {8B94C9CF-48E2-4384-BC39-701B924F556F}
 */

DEFINE_GUID(IID_IHXQoSSignalSink,
            0x8b94c9cf, 0x48e2, 0x4384, 0xbc, 0x39, 0x70, 0x1b, 0x92, 0x4f, 0x55, 0x6f);

#undef  INTERFACE
#define INTERFACE   IHXQoSSignalSink
DECLARE_INTERFACE_(IHXQoSSignalSink, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)           ( THIS_ REFIID riid, void** ppvObj ) PURE;
    STDMETHOD_(ULONG32,AddRef)          ( THIS ) PURE;
    STDMETHOD_(ULONG32,Release)         ( THIS ) PURE;

    // IHXQoSSignalSink
    STDMETHOD (Signal)(THIS_ IHXQoSSignal* pSignal, IHXBuffer* pSessionId) PURE;
    STDMETHOD (ChannelClosed)(THIS_ IHXBuffer* pSessionId) PURE;
};

/*
 * IHXQoSSignalBus
 * {8003507E-453F-4439-BF08-7F8A0E083D9E}
 */

DEFINE_GUID(IID_IHXQoSSignalBus,
            0x8003507e, 0x453f, 0x4439, 0xbf, 0x8, 0x7f, 0x8a, 0xe, 0x8, 0x3d, 0x9e);

#undef  INTERFACE
#define INTERFACE   IHXQosSignalBus
DECLARE_INTERFACE_(IHXQoSSignalBus, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)           ( THIS_ REFIID riid, void** ppvObj ) PURE;
    STDMETHOD_(ULONG32,AddRef)          ( THIS ) PURE;
    STDMETHOD_(ULONG32,Release)         ( THIS ) PURE;

    // IHXQoSSignalBus
    STDMETHOD (Init) (THIS_ IHXBuffer* pSessionId) PURE;
    STDMETHOD (Close) (THIS) PURE;
    STDMETHOD (Send) (THIS_ IHXQoSSignal* pSignal) PURE;
    STDMETHOD (AttachListener)  (THIS_ HX_QOS_SIGNAL ulListenFilter, IHXQoSSignalSink* pListener) PURE;
    STDMETHOD (DettachListener) (THIS_ HX_QOS_SIGNAL ulListenFilter, IHXQoSSignalSink* pListener) PURE;
};

/*
 * IHXQoSignal
 * {32126BDC-0074-4f43-8D2C-65D76D60B5CB}
 */

DEFINE_GUID(IID_IHXQoSSignal,
            0x32126bdc, 0x74, 0x4f43, 0x8d, 0x2c, 0x65, 0xd7, 0x6d, 0x60, 0xb5, 0xcb);

#undef  INTERFACE
#define INTERFACE   IHXQoSSignal

//A class factory may create this object
#define CLSID_IHXQoSSignal IID_IHXQoSSignal

DECLARE_INTERFACE_(IHXQoSSignal, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;
    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    //IHXQoSSignal
    STDMETHOD (GetId) (THIS_ REF(HX_QOS_SIGNAL) ulSignalId) PURE;
    STDMETHOD (SetId) (THIS_ HX_QOS_SIGNAL ulSignalId) PURE;
    STDMETHOD (GetValueUINT32) (THIS_ REF(UINT32) ulValue) PURE;
    STDMETHOD (SetValueUINT32) (THIS_ UINT32 ulValue) PURE;
    STDMETHOD (GetValue) (THIS_ REF(IHXBuffer*) pBuffer) PURE;
    STDMETHOD (SetValue) (THIS_ IHXBuffer* pBuffer) PURE;
    STDMETHOD (WriteSignal) (THIS_ UINT32 ulFileDescriptor) PURE;
};

/****************************************************************************
 *
 *  Interface:
 *
 *      IHXQOSClassFactory
 *
 *  Purpose:
 *
 *      HX interface that manages the creation of HX QoS classes.
 *
 *  IID_IHXQoSClassFactory:
 *
 *      {C1316A78-2960-4f5c-A2A9-46D9C925E88F}
 *
 */

DEFINE_GUID(IID_IHXQoSClassFactory,
            0xc1316a78, 0x2960, 0x4f5c, 0xa2, 0xa9, 0x46, 0xd9, 0xc9, 0x25, 0xe8, 0x8f);

#undef  INTERFACE
#define INTERFACE   IHXQoSClassFactory

DECLARE_INTERFACE_(IHXQoSClassFactory, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    /*
     *  IHXQOSClassFactory methods
     */

    /************************************************************************
     *  Method:
     *      IHXCommonClassFactory::CreateInstance
     *  Purpose:
     *      Creates instances of QoS classes. The first argument is the
     *      profile for which you are creating the objects.
     */
    STDMETHOD(CreateInstance)           (THIS_
                                         IHXQoSSignalBus* pSignalBus,
                                         REFCLSID    /*IN*/  rclsid,
                                         void**     /*OUT*/ ppUnknown) PURE;
};

/*
 * IHXQoSDiffServConfigurator
 * {5FB79E3A-9BEF-4676-8977-08FF6B2D7270}
 */
DEFINE_GUID(IID_IHXQoSDiffServConfigurator,
            0x5fb79e3a, 0x9bef, 0x4676, 0x89, 0x77, 0x8, 0xff, 0x6b, 0x2d, 0x72, 0x70);

#undef  INTERFACE
#define INTERFACE  IHXQoSDiffServConfigurator
DECLARE_INTERFACE_(IHXQoSDiffServConfigurator, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)           ( THIS_ REFIID riid, void** ppvObj ) PURE;
    STDMETHOD_(ULONG32,AddRef)          ( THIS ) PURE;
    STDMETHOD_(ULONG32,Release)         ( THIS ) PURE;

    // IHXQoSDiffServConfigurator
    STDMETHOD(ConfigureSocket) (THIS_ IHXSocket* pSock,
                                HX_QOS_DIFFSERV_CLASS cClass) PURE;
};

#endif /* _HX_QOS_H_ */

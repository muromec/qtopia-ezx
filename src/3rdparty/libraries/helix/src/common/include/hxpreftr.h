/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxpreftr.h,v 1.10 2007/04/27 20:12:16 ping Exp $
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

#ifndef _HXPREFTR_H_
#define _HXPREFTR_H_

// the following entries are critical to the
// smartER networking(client/common/netio/preftran.cpp)
// please modify with extra caution.
#define MAX_TRANSPORT_MODE  4

typedef enum
{
    UnknownMode   = -1
   ,MulticastMode = 0
   ,UDPMode       = 1
   ,TCPMode       = 2
   ,HTTPCloakMode = 3
} TransportMode;

typedef enum
{
  PTC_UNKNOWN,
  PTC_INTERNAL,
  PTC_EXTERNAL
} PreferredTransportClass;

typedef enum 
{
  PTP_UNKNOWN,
  PTP_PNM,
  PTP_RTSP
} PreferredTransportProtocol;

typedef enum
{
  PTS_UNKNOWN,
  PTS_CREATE,
  PTS_PENDING,
  PTS_READY
} PreferredTransportState;

#if defined(HELIX_FEATURE_TCP_OVER_UDP)
#define ATTEMPT_AUTOTRANSPORT	0x1C
#else
#define ATTEMPT_AUTOTRANSPORT	0x1F
#endif /* HELIX_FEATURE_TCP_OVER_UDP */

#define ATTEMPT_MULTICAST	0x01
#define ATTEMPT_UDP		0x02
#define ATTEMPT_TCP		0x04
#define ATTEMPT_HTTPCLOAK	0x08

typedef _INTERFACE	IHXPreferredTransportManager	IHXPreferredTransportManager;
typedef _INTERFACE	IHXPreferredTransport		IHXPreferredTransport;
typedef _INTERFACE	IHXPreferredTransportSink	IHXPreferredTransportSink;
typedef _INTERFACE	IHXProxyManager		        IHXProxyManager;

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPreferredTransportManager
 *
 *  Purpose:
 *
 *	Interface to select preferred transport
 *
 *  IHXPreferredTransportManager:
 *
 *	{00003700-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXPreferredTransportManager, 0x00003700, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
					       0xa0, 0x24, 0x40, 0x6d, 0x59);

DECLARE_INTERFACE_(IHXPreferredTransportManager, IUnknown)
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
     * IHXPreferredTransportManager methods
     */

    /************************************************************************
     *	Method:
     *	    IHXPreferredTransportManager::Initialize
     *	Purpose:
     *	    Initialize the transport manager such as re-reading the preferences
     */
    STDMETHOD(Initialize)		(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPreferredTransportManager::GetPrefTransport
     *	Purpose:
     *	    Get preferred host transport
     */
    STDMETHOD(GetPrefTransport)		(const char*			/* IN  */ pszHostName,
					PreferredTransportProtocol	/* IN  */ prefTransportProtocol,
					REF(IHXPreferredTransport*)	/* OUT */ pPrefTransport) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPreferredTransportManager::RemovePrefTransport
     *	Purpose:
     *	    Remove preferred host transport
     */
    STDMETHOD(RemovePrefTransport)	(IHXPreferredTransport*	/* IN  */ pPrefTransort) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPreferredTransportManager::GetTransportPreference
     *	Purpose:
     *	    Get transport preference set by the user
     */
    STDMETHOD(GetTransportPreference)	(PreferredTransportProtocol	/* IN  */ prefTransportProtocol,
					 REF(UINT32)			/* OUT */ ulPreferenceMask) PURE;

    STDMETHOD(SetAutoBWDetectionValue)  (THIS_
                                         UINT32         ulBW) PURE;

    STDMETHOD(GetAutoBWDetectionValue)  (THIS_
                                         REF(UINT32)    ulBW) PURE;    
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPreferredTransport
 *
 *  Purpose:
 *
 *	Interface to preferred transport
 *
 *  IHXPreferredTransport:
 *
 *	{00003701-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXPreferredTransport, 0x00003701, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
					0xa0, 0x24, 0x40, 0x6d, 0x59);

DECLARE_INTERFACE_(IHXPreferredTransport, IUnknown)
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
     * IHXPreferredTransport methods
     */
    STDMETHOD_(PreferredTransportState, GetState) (THIS) PURE;

    STDMETHOD_(PreferredTransportClass, GetClass) (THIS) PURE;
	STDMETHOD_(HXBOOL, ValidateTransport) (TransportMode mode) PURE;

    STDMETHOD(GetTransport)	    (REF(TransportMode)			/* OUT */   prefTransportType,
				    REF(UINT16)				/* OUT */   uCloakPort) PURE;
    
    STDMETHOD(SetTransport)	    (TransportMode			/* IN  */   prefTransportType,
				    UINT16				/* IN  */   uCloakPort) PURE;

    STDMETHOD(SwitchTransport)	    (HX_RESULT				/* IN  */   error,
				    REF(TransportMode)			/* INOUT */ prefTransportType) PURE;

    STDMETHOD(RemoveTransport)	    (THIS) PURE;

    STDMETHOD(AbortTransport)       (THIS) PURE;

    STDMETHOD(AddTransportSink)	    (IHXPreferredTransportSink*	/* IN  */   pPrefTransportSink) PURE;

    STDMETHOD(RemoveTransportSink)  (IHXPreferredTransportSink*	/* IN  */   pPrefTransportSink) PURE;

    STDMETHOD_(HXBOOL, GetHTTPNG)	    (THIS) PURE;

    STDMETHOD(SetHTTPNG)	    (HXBOOL bHTTPNG) PURE;

    STDMETHOD(SetAutoBWDetectionValue)  (THIS_
                                         UINT32         ulBW) PURE;

    STDMETHOD(GetAutoBWDetectionValue)  (THIS_
                                         REF(UINT32)    ulBW) PURE;    
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPreferredTransportSink
 *
 *  Purpose:
 *
 *	Sinker to preferred transport
 *
 *  IHXPreferredTransportSink:
 *
 *	{00003702-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXPreferredTransportSink, 0x00003702, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
					    0xa0, 0x24, 0x40, 0x6d, 0x59);

DECLARE_INTERFACE_(IHXPreferredTransportSink, IUnknown)
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
     * IHXPreferredTransportSink methods
     */
    STDMETHOD(TransportSucceeded)   (TransportMode			/* IN */   prefTransportType,
				     UINT16				/* IN */   uCloakPort) PURE;

    STDMETHOD(TransportFailed)	    (THIS) PURE;

    STDMETHOD(TransportAborted)     (THIS) PURE;
};

#endif /* _HXPREFTR_H_ */

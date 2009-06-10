/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxslta.h,v 1.4 2004/10/12 18:46:39 jc Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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

#ifndef _HXSLTA_H
#define _HXSLTA_H

typedef _INTERFACE  IHXBuffer           IHXBuffer;

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXSLTA
 * 
 *  Purpose:
 * 
 *	Slta that works with RMA.  Simulates a live stream from a file.
 * 
 *  IID_IHXSLTA
 * 
 *	{00000D00-b4c8-11d0-9995-00a0248da5f0}
 * 
 */
DEFINE_GUID(IID_IHXSLTA,   0x00000D00, 0xb4c8, 0x11d0, 0x99, 
			    0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

DECLARE_INTERFACE_(IHXSLTA, IUnknown)
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
     *	IHXSLTA methods
     */

    /************************************************************************
     *	Method:
     *	    IHXSLTA::Connect
     *	Purpose:
     *	    Connects the slta to a server.
     */
    STDMETHOD(Connect)	(THIS_
			    const char* host,
			    UINT16 uPort,
			    const char* username,
			    const char* passwd,
			    const char* livefile
			) PURE;

    /************************************************************************
    * Method:
    *	    IHXSLTA::SetTAC
    * Purpose:
    *	    Set the TAC info for the stream. This method MUST be called
    *	before Encode to have any effect.
    */
    STDMETHOD(SetTAC)	(THIS_
			    const char* Title,
			    const char* Author,
			    const char* Copyright) PURE;

    /************************************************************************
    *	Method:
    *	    IHXSLTA:Encode
    *	Purpose:
    *	    Start encoding the file to the server.
    */
    STDMETHOD(Encode)	
			(THIS_
			    const char* filename
			) PURE;

    /************************************************************************
    *	Method:
    *	    IHXSLTA:Disconnect
    *	Purpose:
    *	    Disconnect the slta from the server.
    */
    STDMETHOD(Disconnect)   (THIS) PURE;


    /************************************************************************
    *	Method:
    *	    IHXSLTA::SetTargetBandwidth
    *	Purpose:
    *	    Sets the target bw for rule subscription.
    */
    STDMETHOD(SetTargetBandwidth)   (THIS_
					UINT32 ulTargetBW)  PURE;


};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXiQSLTA
 * 
 *  Purpose:
 * 
 *	Slta that works with Remote Broadcast Services.  
 *      Simulates a live stream from a file.
 * 
 *  IID_IHXiQSLTA
 * 
 *      {1D110F56-73AA-44fd-9FE6-F091486F5ED9}
 */

DEFINE_GUID(IID_IHXiQSLTA, 0x1d110f56, 0x73aa, 0x44fd, 0x9f, 
            0xe6, 0xf0, 0x91, 0x48, 0x6f, 0x5e, 0xd9);

DECLARE_INTERFACE_(IHXiQSLTA, IUnknown)
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
     *	IHXSLTA methods
     */

    /************************************************************************
     *	Method:
     *	    IHXiQSLTA::Init - Advanced mode
     *	Purpose:
     *	    Initializes the remote broadcast library.
     */
    STDMETHOD(Init)	(THIS_
			 IHXBuffer* pConfigFilePath,
                         IHXBuffer* pSessionName
			) PURE;

    /************************************************************************
     *  Method:
     *      IHXiQSLTA::Init - Basic mode
     *  Purpose:
     *      Initializes the remote broadcast library.
     */
    STDMETHOD(Init)     (THIS_
                         const char* host,
                         UINT16 httpPort,
                         const char* user,
                         const char* password,
                         IHXBuffer* pSessionName,
                         BOOL bTCP
                        ) PURE;

    /************************************************************************
    * Method:
    *	    IHXiQSLTA::SetTAC
    * Purpose:
    *	    Set the TAC info for the stream. This method MUST be called
    *	before Encode to have any effect.
    */
    STDMETHOD(SetTAC)	(THIS_
			    const char* Title,
			    const char* Author,
			    const char* Copyright) PURE;

    /************************************************************************
    *	Method:
    *	    IHXiQSLTA::BeginTransmission
    *	Purpose:
    *	    Start transmitting the file to the server(s).
    */
    STDMETHOD(BeginTransmission)	
			(THIS_
			 IHXBuffer* pMediaName,
			 IHXBuffer* pSessionName
			) PURE;

    /************************************************************************
    *	Method:
    *	    IHXiQSLTA::EndTransmission
    *	Purpose:
    *	    End a transmission of a file to the server(s).
    */
    STDMETHOD(EndTransmission)   (THIS) PURE;

    /************************************************************************
    *	Method:
    *	    IHXiQSLTA::Close()
    *	Purpose:
    *	    Cleanup SLTA implementation's resources.
    */
    STDMETHOD(Close)   (THIS) PURE;

    /************************************************************************
    *	Method:
    *	    IHXiQSLTA::SetTargetBandwidth
    *	Purpose:
    *	    Sets the target bw for rule subscription.
    */
    STDMETHOD(SetTargetBandwidth)   (THIS_
					UINT32 ulTargetBW)  PURE;

    /************************************************************************
    *	Method:
    *	    IHXiQSLTA::SetLatencyMode
    *	Purpose:
    *	    Sets the latency mode for the broadcast. 0 is normal mode (the 
    *       default. 1 and 2 are inceasingly stringent latency requirements.
    */
    STDMETHOD(SetLatencyMode)   (THIS_
					UINT32 ulLatencyMode)  PURE;

};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXSltaEvent
 * 
 *  Purpose:
 * 
 *	Allows events to be sent through an SLTA stream
 * 
 *  IID_IHXSltaEvent
 * 
 *	{00000D01-b4c8-11d0-9995-00a0248da5f0}
 * 
 */
DEFINE_GUID(IID_IHXSltaEvent,   0x00000D01, 0xb4c8, 0x11d0, 0x99, 
			    0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

/* 
 * Valid RMA event IDs.
 */

#define HX_EVENT_TITLE             0x0000
#define HX_EVENT_AUTHOR            0x0001
#define HX_EVENT_COPYRIGHT         0x0002
#define HX_EVENT_SERVER_ALERT      0x0003
#define HX_EVENT_PROGRESS_MESSAGE  0x0004
#define HX_EVENT_TEXT_SIZE         0x0010
#define HX_EVENT_TEXT              0x0011
#define HX_EVENT_TEXT_ANCHOR       0x0012
#define HX_EVENT_BROWSER_OPEN_URL  0x0020
#define HX_EVENT_TOPIC             0x0030
#define HX_EVENT_EMPTY             0x0200
#define HX_EVENT_CUSTOM_BEGIN      0x0400

DECLARE_INTERFACE_(IHXSltaEvent, IUnknown)
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
     *	IHXSltaEvent methods
     */

   /************************************************************************
    * Method:
    *	    IHXSltaEvent::SetEvent
    * Purpose:
    *	    Sends an event to the slta stream. 
    *       nEventID must be one of the valid event IDs defined above.
    */
    STDMETHOD(SetEvent)             (THIS_
                                     UINT16 nEventID,
                                     const char* szEventText) PURE;

   /************************************************************************
    * Method:
    *	    IHXSltaEvent::SetRepeatedEvent
    * Purpose:
    *	    Sets an event to be repeated every ulFrequency milliseconds.
    *       nEventID must be one of the valid event IDs defined above.
    */

    STDMETHOD(SetRepeatedEvent)     (THIS_
                                     UINT16 nEventID,
                                     const char* szEventText,
                                     UINT32 ulFrequency) PURE;
};

#endif

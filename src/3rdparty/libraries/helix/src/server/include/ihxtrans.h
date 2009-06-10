/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: ihxtrans.h,v 1.1 2006/12/21 20:02:06 tknox Exp $ 
 *   
 * Portions Copyright (c) 1995-2007 RealNetworks, Inc. All Rights Reserved.  
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

#ifndef _IHX_TRANSPORT_H_
#define _IHX_TRANSPORT_H_

#include "hxcom.h"
#include "hxcomm.h"

DEFINE_GUID(IID_IHXTransport, 0x9231108c, 0x0279, 0x4774, 0x8c, 0x4e,
                    0x43, 0xcd, 0x13, 0xf3, 0xc9, 0x11);

_INTERFACE IHXServerPacketSource;
_INTERFACE IHXBuffer;
_INTERFACE IHXValues;

class ServerPacket;
class Timeval;

#define RULE_TABLE_WIDTH  256
#define RULE_TABLE_HEIGHT 64

/**
 * \brief IHXTransport is the base interface for all transports.
 *
 * IHXTransport has methods that all transports must implement.
 * \note IID_IHXTransport {9231108C-0279-4774-8C4E-43CD13F3C911}
 */
//DECLARE_INTERFACE_(IHXTransport, IHXStatistics, IHXServerPacketSink)
DECLARE_INTERFACE_(IHXTransport, IHXStatistics)
{
    /**
     * \brief Pass registry ID to the caller
     * \param ulRegistryID IN registry ID
     * \return HXR_NOTIMPL in RTSPServerMulticastTransport, WMTHttpTransport
     * \return HXR_FAIL|HXR_OK in MMSTransport
     * \ref IHXStatistics
     *
     */
    STDMETHOD(InitializeStatistics)     (THIS_
                                        UINT32  /*IN*/  ulRegistryID) PURE;

    /**
     * \brief Notify the client to update its statistics stored in the registry
     * \return HXR_NOTIMPL in RTSPServerMulticastTransport, WMTHttpTransport
     * \return HXR_OK|HXR_UNEXPECTED|Registry::GetPropName in MMSTransport
     * \sa Registry::GetPropName
     * \ref IHXStatistics
     *
     */
    STDMETHOD(UpdateStatistics)         (THIS) PURE;

    /**
     * \brief Init is there for some initialisation stuff.
     * \param pContext The process context.
     * \return HXR_OK|Context::QueryInterface in RTSPTransport
     */
    STDMETHOD(Init) (THIS_ IUnknown* pContext) PURE;

    /**
     * \brief Done allows for any requisite cleanup.
     * \param None
     * \return None.
     */
    STDMETHOD_(void, Done) (THIS) PURE;

    /**
     * \brief GetTransportHeader gets the transport header that created this transport.
     *
     * When the factory creates a new transport, it uses the transport MIME
     * header, and saves a copy in the transport.
     *
     * \param None
     * \return IHXBuffer* Pointer to the buffer containing the transport MIME header.
     * \note This is a new method.
     */
    STDMETHOD_(IHXBuffer*,GetTransportHeader) (THIS) PURE;

    /**
     * \brief IsInitialized checks whether the transport is initialized.
     * \param None
     * \return Is the transport initialized?
     */
    STDMETHOD_(HXBOOL,IsInitialized) (THIS) PURE;

    /**
     * \brief AddStreamInfo sets up some member variables based on the pStreamInfo.
     * \param pStreamInfo Pointer to the stream info IHXValues to add from.
     * \param ulBufferDepth The buffer depth to use.
     * \return None.
     */
    STDMETHOD_(void,AddStreamInfo) (THIS_ IHXValues* pStreamInfo, UINT32 ulBufferDepth) PURE;

    /**
     * \brief SetSessionID stashes the sessionID for later use.
     * \param pSessionID Pointer to the sessionID.
     * \return None.
     */
    STDMETHOD_(void,SetSessionID) (THIS_ const char* pszSessionID) PURE;

    /**
     * \brief GetCapabilities returns a bitmask of special transport capabilities.
     *
     * It will be used in place of calls to SupportsPacketAggregation or isReflector,
     * and to handle future capability queries that are orthogonal to
     * the type of transport.
     *
     * \param None
     * \return A bitmask of all special transport capabilities.
     */
    STDMETHOD_(UINT32,GetCapabilities) (THIS) PURE;

    /**
     * \brief HandlePacket determines what sort of packet this is, and handles appropriately.
     *
     * Determine what kind of handling this packet needs (i.e. is
     * it an ACK, or StreamEnd, or whatever).
     *
     * Basically, HandlePacket is the way a transport currently receives
     * an inbound packet for processing. Outbound packets, however, still
     * use the socket directly. Why the difference? Because in the inbound
     * case, the transport doesn't always own the socket the packet came
     * from, and so there must be a way to inject it.
     *
     * \param pBuffer The buffer containing the packet.
     * \return HX_RESULT Was the packet handled successfully?
     */
    STDMETHOD(HandlePacket) (THIS_ IHXBuffer* pBuffer) PURE;

    /**
     * \brief SetFirstPlayTime responds to a setFirstPlayTime event.
     *
     * \param pTv Pointer to the time value.
     * \return HX_RESULT Did we set the first play time correctly?
     */
    STDMETHOD(SetFirstPlayTime) (THIS_ Timeval* pTv) PURE;

};

#endif /* _IHX_TRANSPORT_H_ */


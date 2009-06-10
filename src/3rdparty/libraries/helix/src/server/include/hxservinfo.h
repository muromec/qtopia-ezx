/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxservinfo.h,v 1.2 2007/03/30 19:08:38 tknox Exp $ 
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

#ifndef _IHX_SERVER_INFO_H_
#define _IHX_SERVER_INFO_H_

#include "hxcom.h"

/****************************************************************************
 *
 *  Interface:
 *
 *      IHXServerInfo
 *
 *  Purpose:
 *
 *      Abstract server statistics gathering.
 *
 *  IID_IHXServerInfo:
 *
 *      {E3B2CE09-52B4-457A-B79A-C9CC937357C7}
 *
 */
DEFINE_GUID(IID_IHXServerInfo, 0xe3b2ce09, 0x52b4, 0x457a, 0xb7, 0x9a, 0xc9,
                        0xcc, 0x93, 0x73, 0x57, 0xc7);

#undef  INTERFACE
#define INTERFACE   IHXServerInfo

enum ServerInfoCounters
{
    SIC_STREAM_COUNT,
    SIC_MIDBOX_COUNT,
    SIC_RTSPCLIENT_COUNT,
    SIC_HTTPCLIENT_COUNT,
    SIC_MMSCLIENT_COUNT,
    SIC_TCPTRANSPORT_COUNT,
    SIC_UDPTRANSPORT_COUNT,
    SIC_MULTICASTTRANSPORT_COUNT,
    SIC_CLOAKED_COUNT,
    SIC_LAST_COUNT,
};

enum ServerGlobalSelector
{
    SG_BYTES_SERVED,
    SG_PPS,
    SG_LAST_GLOBAL,
};

/**
 * \brief IHXServerInfo abstracts some server statistics gathering
 */
DECLARE_INTERFACE_(IHXServerInfo, IUnknown)
{
public:
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     *  IHXServerInfo methods
     */

    /**
     * \brief Increment the chosen count.
     * \param sicSelector One of the ServerInfoCounters enumeration.
     */
    STDMETHOD_(void, IncrementServerInfoCounter)    (THIS_ ServerInfoCounters sicSelector) PURE;

    /**
     * \brief Decrement the chosen count.
     * \param sicSelector One of the ServerInfoCounters enumeration.
     */
    STDMETHOD_(void, DecrementServerInfoCounter)    (THIS_ ServerInfoCounters sicSelector) PURE;

    /**
     * \brief Increment the total client count.
     * \param None
     * \return The total client count.
     */
    STDMETHOD_(UINT32, IncrementTotalClientCount)   (THIS) PURE;

    /**
     * \brief Add the bandwidth specified to the total in use.
     * \param lBandwidth The bandwidth to add.
     */
    STDMETHOD_(void, ChangeBandwidthUsage)          (THIS_ INT32 lBandwidth) PURE;

    /**
     * \brief Get a pointer to the selected server global.
     * \param sgSelector One of the ServerGlobalSelector enumeration.
     * \return The address of the server global.
     */
    STDMETHOD_(UINT32*, GetServerGlobalPointer) (THIS_ ServerGlobalSelector sgSelector) PURE;
};

#endif /* _IHX_SERVER_INFO_H_ */

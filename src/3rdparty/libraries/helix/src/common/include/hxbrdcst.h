/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxbrdcst.h,v 1.10 2008/08/20 21:05:49 ehyche Exp $
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

#ifndef _HXBRDCST_H_
#define _HXBRDCST_H_

/*
 * Forward declarations of some interfaces defined here-in.
 */
typedef _INTERFACE  IHXRemoteBroadcastServices	              IHXRemoteBroadcastServices;
typedef _INTERFACE  IHXRemoteBroadcastConfiguration          IHXRemoteBroadcastConfiguration;
typedef _INTERFACE  IHXRemoteBroadcastConfigurationResponse  IHXRemoteBroadcastConfigurationResponse;
typedef _INTERFACE  IHXAuthResponse  IHXAuthResponse;
typedef _INTERFACE  IHXRemoteBroadcastStatisticsReport       IHXRemoteBroadcastStatisticsReport;

typedef _INTERFACE  IHXRemoteBroadcastServices2               IHXRemoteBroadcastServices2;

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXRemoteBroadcastServices
 * 
 *  Purpose:
 * 
 *       To act as the interface to the RealSystem iQ broadcast engine in
 *  contexts that exist outside of the Helix Server process space.     
 * 
 *  IID_IHXRemoteBroadcastServices:
 * 
 *  {8F933081-27B6-11d5-9569-00902742E832}
 */

DEFINE_GUID(IID_IHXRemoteBroadcastServices, 0x8f933081, 	 
             0x27b6, 0x11d5, 0x95, 0x69, 0x0, 0x90, 0x27, 0x42, 0xe8, 0x32);

#undef  INTERFACE
#define INTERFACE   IHXRemoteBroadcastServices

DECLARE_INTERFACE_(IHXRemoteBroadcastServices, IUnknown)
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
     * IHXRemoteBroadcastServices Methods
     */

    /************************************************************************
     *	Method:
     *	    IHXRemoteBroadcastServices::InitRemoteBroadcast
     *	Purpose:
     *	     Load an initial configuration for the remote broadcast plugin
     *
     */
    STDMETHOD(InitRemoteBroadcast)	(THIS_	
					IHXBuffer* pConfgFilePath) PURE;

    /************************************************************************
     *  Method:
     *      IHXRemoteBroadcastServices::InitRemoteBroadcast
     *  Purpose:
     *       Load an initial configuration for the remote broadcast plugin
     *       : the RBS Authentication version
     */
    STDMETHOD(InitRemoteBroadcast)      (THIS_
                                        IHXBuffer* pConfgFilePath,
                                        IHXAuthResponse *pAuthResponse,
                                        IHXBuffer* pSessionName) PURE;
    
    /************************************************************************
     *	Method:
     *	    IHXRemoteBroadcastServices::NewRemoteBroadcastSession
     *	Purpose:
     *	     Initiate a new broadcast session
     *
     */
    STDMETHOD(NewRemoteBroadcastSession)  (THIS_
					   IHXBuffer*          pSessionName,
					   IUnknown* pSessionSource) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRemoteBroadcastServices::Process
     *	Purpose:
     *	     Yeild processing time to the scheduler
     *
     */
    STDMETHOD(Process)             (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRemoteBroadcastServices::Close
     *	Purpose:
     *	     Cleanup Remote Broadcast Services. 
     *
     */
    STDMETHOD(Close)             (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRemoteBroadcastServices::GetTime
     *	Purpose:
     *	     Get current scheduler time
     *
     */
    STDMETHOD_(UINT32,GetTime)     (THIS) PURE;
    
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXRemoteBroadcastServices2
 * 
 *  Purpose:
 *       Derives from IHXRemoteBroadcastServices providing additional 
 *  functionality to know whether the configuration has any pull setting
 *   
 *  IID_IHXRemoteBroadcastServices2:
 * 
 *    {A0A86867-EE8A-4888-A65E-62AA33D0E9BB}
 */

DEFINE_GUID(IID_IHXRemoteBroadcastServices2,0xa0a86867, 0xee8a, 0x4888, 0xa6, 0x5e, 0x62, 0xaa, 0x33, 0xd0, 0xe9, 0xbb);

#undef  INTERFACE
#define INTERFACE   IHXRemoteBroadcastServices2

DECLARE_INTERFACE_(IHXRemoteBroadcastServices2, IHXRemoteBroadcastServices)
{ 
      /************************************************************************
     *	Method:
     *	    IHXRemoteBroadcastServices2::IsPullConfigured
     *	Purpose:
     *	    Returns true if the configuration consists of pull setting(s)
     */

    STDMETHOD_(HXBOOL, IsPullConfigured) (THIS) PURE; //returns true if SLTA is started with pull configuration
};

// {C88D3530-1455-11d6-93D7-0002B31090EC}
DEFINE_GUID(IID_IHXAuthResponse, 
0xc88d3530, 0x1455, 0x11d6, 0x93, 0xd7, 0x0, 0x2, 0xb3, 0x10, 0x90, 0xec);
#undef  INTERFACE
#define INTERFACE   IHXAuthResponse

DECLARE_INTERFACE_(IHXAuthResponse, IUnknown)
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
     * IHXAuthResponse Methods
     */
    STDMETHOD(AuthDone) 	(THIS_
				HXBOOL bAuthSucceeded) PURE;
};



/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXRemoteBroadcastConfiguration
 * 
 *  Purpose:
 * 
 *     To provide a simple interface to the XML based config
 * 
 *  IID_IHXRemoteBroadcastConfiguration:
 *
 *  {8F933083-27B6-11d5-9569-00902742E832}
 * 
 */

DEFINE_GUID(IID_IHXRemoteBroadcastConfiguration, 
	    0x8f933083, 0x27b6, 0x11d5, 0x95, 0x69, 0x0, 0x90, 0x27, 0x42, 0xe8, 0x32);

#undef  INTERFACE
#define INTERFACE   IHXRemoteBroadcastConfiguration

DECLARE_INTERFACE_(IHXRemoteBroadcastConfiguration, IUnknown)
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
     * IHXRemoteBroadcastServices Methods
     */
    STDMETHOD(Init) 	(THIS_
			 IHXRemoteBroadcastConfigurationResponse* pResponse) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRemoteBroadcastConfiguration::AddDestination
     *	Purpose:
     *
     *    Add a new destination to which to broadcast.  Destinations
     *	 are defined as an IHXValues struct with the following fields
     *	 
     *	 Name             <string>      unique user defined name of destination 
     *	 RelayMode        <int>         TRUE or FALSE, should always be FALSE for remote
     *	 Protocol         <string>      protocol string: <udp/unicast>, <udp/multicast>, <tcp>
     *	 Password         <string>      user defined
     *	 SecurityType     <string>      'None' or 'Basic' 
     *	 Address          <string>      hostname or IP of remote receiver
     *	 PortRange        <string/int>  outbound port range to use (should match receiver)
     *	 PathPrefix       <string>      prefix of media to broadcast (can be * for wildcard)
     *	 ResendSupported  <int>         TRUE or FALSE
     *	 FECLevel         <int>         % redundancy to use for FEC    
     *   TTL              <int>         multicast packet time to live.
     *
     *   TCPReconectTimeout      <int>     
     *   TCPWouldBlockedTimeout  <int>
     *   RedundancySendInterval  <int>
     *   AcquisitionDataInterval <int>  rate (in seconds) at which to send header blocks
     *                                  for stream (re)establishment.
     */

    STDMETHOD(AddDestination)   	(THIS_
					 IHXValues*   pDestination,
					 HXBOOL          bOverwrite) PURE;
    
    STDMETHOD(UpdateDestination)        (THIS_
					 IHXValues*   pDestination) PURE;
    
    STDMETHOD(RemoveDestination) 	(THIS_
					 const char*   pDestinationName) PURE;
    
    STDMETHOD(RetrieveDestination)      (THIS_
					 const char*   pDestinationName, 
					 REF(IHXValues*) /*OUT*/ pDestination) PURE;

     /************************************************************************
     *	Method:
     *	    IHXRemoteBroadcastConfiguration::AddPullSplit
     *	Purpose:
     *
     *    Add a new pullsplit to which to broadcast.  PullSplits
     *	 are defined as an IHXValues struct with the following fields
     *	 
     *	 Name             <string>      unique user defined name of destination 
     *	 ListenPort       <int>         port on which pull split connections are accepted
     *	 LocalAddress     <string>      address of interface to which to bind
     *   PathPrefix       <string>      prefix of media to broadcast (can be * for wildcard)
     *	 Password         <string>      user defined
     *	 SecurityType     <string>      'None' or 'Basic' 
     *
     *   TCPReconectTimeout      <int>     
     *   TCPWouldBlockedTimeout  <int>
     */

    STDMETHOD(AddPullSplit)   	        (THIS_
					 IHXValues*   pPullSplit,
					 HXBOOL          bOverwrite) PURE;
    
    STDMETHOD(UpdatePullSplit)          (THIS_
					 IHXValues*   pPullSplit) PURE;
    
    STDMETHOD(RemovePullSplit) 	        (THIS_
					 const char*   pPullSplitName) PURE;
    
    STDMETHOD(RetrievePullSplit)         (THIS_
					 const char*   pPullSplitName, 
					 REF(IHXValues*) /*OUT*/ pPullSplit) PURE;

    STDMETHOD(Commit) 	(THIS) PURE;
				
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXRemoteBroadcastConfigurationResponse
 * 
 *  Purpose:
 * 
 *     To notify the user of IHXRemoteBroadcastConfiguration of the status
 * asychronous configuration modifications.
 * 
 *  IID_IHXRemoteBroadcastConfigurationResponse:
 *
 *  {67C1BA10-39BC-11d5-956A-00902742E832}
 * 
 */


DEFINE_GUID(IID_IHXRemoteBroadcastConfigurationResponse, 
0x67c1ba10, 0x39bc, 0x11d5, 0x95, 0x6a, 0x0, 0x90, 0x27, 0x42, 0xe8, 0x32);

#undef  INTERFACE
#define INTERFACE   IHXRemoteBroadcastConfigurationResponse

DECLARE_INTERFACE_(IHXRemoteBroadcastConfigurationResponse, IUnknown)
{
    STDMETHOD(CommitDone) 	(THIS_
				 HX_RESULT hResult) PURE;
};



/****************************************************************************
 *
 *  Interface:
 *
 *      IHXRemoteBroadcastStatisticsReport
 *
 *  Purpose:
 *
 *       To provide users of the RealSystem iQ broadcast engine with a way to
 *  download statistics to the remote server.
 *
 *  IID_IHXRemoteBroadcastStatisticsReport:
 *
 *    {8F933081-27B6-11d5-9569-00902742E832}
 */
DEFINE_GUID(IID_IHXRemoteBroadcastStatisticsReport, 0xcee9cc1e, 0xdcdf, 0x4159, 0xa3, 0x15, 0x8a, 0x3b, 0x86, 0xee, 0x9a, 0x86);

#undef  INTERFACE
#define INTERFACE   IHXRemoteBroadcastStatisticsReport

DECLARE_INTERFACE_(IHXRemoteBroadcastStatisticsReport, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXRemoteBroadcastStatisticsReport Methods
     */

    /************************************************************************
     *  Method:
     *      IHXRemoteBroadcastStatisticsReport::ReportStatistics
     *  Purpose:
     *       Send an opaque string containing encoder statistics to the
     *     broadcast server(s) to be logged
     *
     */
    STDMETHOD(ReportStatistics)         (THIS_
                                        IHXBuffer* pStatistics) PURE;
};

#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXRemoteBroadcastServices)
DEFINE_SMART_PTR(IHXRemoteBroadcastServices2)
DEFINE_SMART_PTR(IHXAuthResponse)
DEFINE_SMART_PTR(IHXRemoteBroadcastConfiguration)
DEFINE_SMART_PTR(IHXRemoteBroadcastConfigurationResponse)
DEFINE_SMART_PTR(IHXRemoteBroadcastStatisticsReport)

#endif /* _HXBRDCST_H_ */











/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdtdr.h,v 1.5 2006/05/04 00:09:10 milko Exp $
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

#ifndef _HXDTDR_H_
#define _HXDTDR_H_

/****************************************************************************
 * Defines
 */

typedef _INTERFACE      IHXPacket                  IHXPacket;
typedef _INTERFACE      IHXValues                  IHXValues;
typedef _INTERFACE      IHXSourceInput             IHXSourceInput;
typedef _INTERFACE      IHXFileWriter		   IHXFileWriter;
typedef _INTERFACE      IHXSourceHandler           IHXSourceHandler;


/****************************************************************************
 * Options
 */

#define MEMORY_OPTION_NAME	    "Memory"
#define HEADER_OPTION_NAME	    "Header"
#define PACKET_OPTION_NAME	    "Packet"
#define VERBOSE_OPTION_NAME	    "Verbose"
#define LEAKANALYSIS_OPTION_NAME    "LeakAnalysis"
#define LEAKDETAILS_OPTION_NAME	    "LeakDetails"
#define MEMORYDETAILS_OPTION_NAME   "MemoryDetails"
#define SERVER_OPTION_NAME	    "Server"
#define RATE_OPTION_NAME	    "Rate"
#define TIMER_OPTION_NAME	    "Timer"
#define EVENT_OPTION_NAME	    "Event"
#define DURATION_OPTION_NAME	    "Duration"
#define PREROLL_OPTION_NAME	    "RecomputePreroll"
#define BLASTFILE_OPTION_NAME	    "BlastFiles"
#define ROTATIONSIZE_OPTION_NAME    "RotationSize"
#define RELATIVETS_OPTION_NAME	    "RelativeTS"
#define DECRYPT_OPTION_NAME	    "DecryptSource"
#define DECODE_OPTION_NAME	    "DecodeSource"
#define BLOCK_OPTION_NAME	    "BlockFilter"
#define MUX_OPTION_NAME         "MuxAudioVideo"

#define SUPPRESS_WRITER_OPTION	    "SuppressWriter"
#define OUTPUT_FILENAME_OPTION	    "OutputFileName"

#define TRANSFER_CERTIFICATE_OPTION "TransferCertificate"

//e1ec2c5f-7f0c-4348-b3b0c4af5eae9778
#define SHID_DECODER   {0xe1ec2c5f, 0x7f0c, 0x4348, 0xb3, 0xb0, 0xc4, 0xaf, 0x5e, 0xae, 0x97, 0x78}


/****************************************************************************
 * 
 *  Interface:
 * 
 * IHXDataTypeDriverResponse
 * 
 *  Purpose:
 * 
 * Interface used to check the state of plugins.
 * 
 *  IHXDataTypeDriverResponse:
 * 
 *  {9D65E58D-9B4B-40f2-8FF0-5CFF673F0BD3}
 * 
 */

DEFINE_GUID(IID_IHXDataTypeDriverResponse, 0x9d65e58d, 0x9b4b, 0x40f2, 0x8f, 0xf0, 0x5c, 0xff, 0x67, 0x3f, 0xb, 0xd3);
 
#undef  INTERFACE
#define INTERFACE   IHXDataTypeDriverResponse
 
DECLARE_INTERFACE_(IHXDataTypeDriverResponse, IUnknown)
{
    /*
     * IHXDataTypeDriverResponse methods
     */
 
    /************************************************************************
     * Method:
     *     IHXDataTypeDriverResponse::OnProgress(UINT32);
     * Purpose:
     *	    Called to notify the status of the transfer
     *     
     */
    STDMETHOD(OnProgress)(THIS_ UINT32 progress) PURE;

    /************************************************************************
     * Method:
     *     IHXDataTypeDriverResponse::OnTerminate(HX_RESULT);
     * Purpose:
     *	    Called when the datatype driver is done, with the appropriate 
     *	    error condition.
     */
    STDMETHOD(OnTerminate)(THIS_ HX_RESULT result) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 * IHXDataTypeDriver2
 * 
 *  Purpose:
 * 
 *  New interface for datatype driver.
 * 
 *  IHXDataTypeDriver2:
 * 
 *  {9D65E58E-9B4B-40f2-8FF0-5CFF673F0BD3}
 * 
 */

DEFINE_GUID(IID_IHXDataTypeDriver2, 0x9d65e58e, 0x9b4b, 0x40f2, 0x8f, 0xf0, 0x5c, 0xff, 0x67, 0x3f, 0xb, 0xd3);
 
#undef  INTERFACE
#define INTERFACE   IHXDataTypeDriver2

#define CLSID_IHXDataTypeDriver IID_IHXDataTypeDriver2

DECLARE_INTERFACE_(IHXDataTypeDriver2, IUnknown)
{
    /*
     * IHXDataTypeDriver2 methods
     */
 
    /************************************************************************
     * Method:
     *     IHXDataTypeDriver2::Open()
     * Purpose:
     *     Called to initialize the driver.
     *     Note: all callbacks on the pResponse will be on the same thread 
     *     that called function.
     *	  
     *	    the data output from the last source handler will be provided to an
     *	    IHXSourceInput. IHXDataTypeDriverResponse is useful for 
     *	    monitoring transfer/decode/decrypt progress and error conditions.	    
     *    
     *	    the source handler list used is the concatenation of handlers from 
     *	    pCertificate, pSourceHandlerGUIDs and pSourceHandlerList, in that order.
     *	    any of the above three can be null
     *
     *	    pOption can be used to pass option values to both FFDRIVER and source
     *	    handlers
     *
     *	    'decrypt' can only be signaled by the presence of a pCertificate
     *	    'decode'  can be signaled by setting the value in pOptions
     *		      but if pCertificate is present, it overwrites 
     *		      'decode' setting in pOptions
     *
     */
    STDMETHOD(Open) (THIS_ 
		     const char*	pCertificate,
		     int		nSourceHandlerGUIDs,
		     GUID*  		pSourceHandlerGUIDs,
		     int		nSourceHandlerList,
		     IHXSourceHandler**	pSourceHandlerList,
		     IHXValues*		pOptions,

		     IHXDataTypeDriverResponse* pResponse,
		     IHXSourceInput* pSourceInput
		    ) PURE;
 
    /************************************************************************
     * Method:
     *	    IHXDataTypeDriver2::Drive()
     * Purpose:
     *	    Called to start processing. This will start the conversion in
     *      a separate thread and return immediately. The client will
     *	    receive callbacks through pResponse's IHXSourceInput and/or 
     *	    IHXDataTypeDriverResponse interfaces (see Open()).
     *
     *	    Drive() may be called multiple times without calling Close()
     *	    to process multiple files with the same driver settings.
     *     
     */
    STDMETHOD(Drive)(THIS_ const char* pInFileName,
		     const char* pOutFileName
		     ) PURE;

    /************************************************************************
     * Method:
     *      IHXDataTypeDriver2::Stop()
     * Purpose:
     *	    Called to stop Drive. Can be called from a different thread, or 
     *	    called from a call back.
     */
    STDMETHOD(Stop)(THIS_ ) PURE;

    /************************************************************************
     * Method:
     *	    IHXDataTypeDriver2::Close()
     * Purpose:
     *	    Called to clean up the driver process. If called while still
     *	    processing a file, the operation will be terminated. The driver
     *	    will need to be reinitialized by calling Open() before it can
     *	    be used again.
     *
     */
    STDMETHOD(Close)(THIS) PURE;

    /************************************************************************
     * Method:
     *     IHXDataTypeDriver2::Pause()
     * Purpose:
     *     Called to throttle delivery. No more callbacks will be made
     *	   until Resume() is called. 
     */
    STDMETHOD(Pause)(THIS) PURE;

    /************************************************************************
     * Method:
     *     IHXDataTypeDriver2::Resume()
     * Purpose:
     *     Called to resume delivery
     */
    STDMETHOD(Resume)(THIS) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXSetClientContext
 * 
 *  Purpose:
 * 
 *  to set a client context
 * 
 *  IHXSetClientContext
 * 
 *  {9D65E58F-9B4B-40f2-8FF0-5CFF673F0BD3}
 * 
 */

DEFINE_GUID(IID_IHXSetClientContext, 0x9d65e58f, 0x9b4b, 0x40f2, 0x8f, 0xf0, 0x5c, 0xff, 0x67, 0x3f, 0xb, 0xd3);
 
#undef  INTERFACE
#define INTERFACE   IHXSetClientContext

DECLARE_INTERFACE_(IHXSetClientContext, IUnknown)
{
    /*
     * IHXSetClientContext methods
     */

     /************************************************************************
     *	Method:
     *	    IHXSetClientContext::SetClientContext
     *	Purpose:
     *	    Called by the client to install itself as the provider of client
     *	    services. 
     *
     *	Datatype driver client can implement IHXUpgradeCollection in the client context 
     *  to get plugin components upgrade notification.
     *
     *  Can also be used as means for custom source handler to commnuication with client through custom interface
     *  implemnted in the ClientContext.
     */
    STDMETHOD(SetClientContext)	(THIS_
				IUnknown* pUnknown) PURE;
};

#endif //_HXDTDR_H_


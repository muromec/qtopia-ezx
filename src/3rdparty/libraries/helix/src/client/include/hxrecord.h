/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxrecord.h,v 1.5 2005/12/02 18:48:35 ping Exp $
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

#ifndef _IHXRECORD_H_
#define _IHXRECORD_H_

#include "hxcom.h"
#include "hxcomptr.h"
#include "hxformt.h"

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE	IHXRecordManager		IHXRecordManager;
typedef _INTERFACE	IHXRecordService		IHXRecordService;
typedef _INTERFACE	IHXRecordSource			IHXRecordSource;

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXRecordManager
 *
 *  Purpose:
 *
 *	Interface implemented by the Core (HXPlayer) for connection of
 *      RMA sources to record sources.
 *
 *  IID_IHXRecordManager:
 *
 *	{9B7854DD-92C8-42c6-936C-565EC373E2AD}
 *
 */
DEFINE_GUID(IID_IHXRecordManager, 
0x9b7854dd, 0x92c8, 0x42c6, 0x93, 0x6c, 0x56, 0x5e, 0xc3, 0x73, 0xe2, 0xad);


DECLARE_INTERFACE_(IHXRecordManager, IUnknown)
{
    /************************************************************************
     *	Method:
     *	    IHXRecordManager::LoadRecordService
     *	Purpose:
     *	    Called by TLC to supply the Core with record service.
     */
    STDMETHOD(LoadRecordService) (THIS_ IHXRecordService* pRecordService) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRecordManager::GetRecordService
     *	Purpose:
     *	    return current record service for the Player.
     */
    STDMETHOD(GetRecordService) (THIS_ REF(IHXRecordService*) pRecordService) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRecordManager::UnloadRecordService
     *	Purpose:
     *	    Called by TLC to ask the Core to stop using record service.
     */
    STDMETHOD(UnloadRecordService) (THIS) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXRecordService
 *
 *  Purpose:
 *
 *	Interface implemented by TLC to allow Core create record sources.
 *
 *  IID_IHXRecordService
 *
 *	{F2F8C09A-A607-40c9-9C26-483BBBC4A086}
 */
DEFINE_GUID(IID_IHXRecordService, 
0xf2f8c09a, 0xa607, 0x40c9, 0x9c, 0x26, 0x48, 0x3b, 0xbb, 0xc4, 0xa0, 0x86);


DECLARE_INTERFACE_(IHXRecordService, IUnknown)
{
    /************************************************************************
     *	Method:
     *	    IHXRecordService::Init
     *	Purpose:
     *	    Initializes record service according to context.
     */
    STDMETHOD(Init) (THIS_ IUnknown* pContext, HXBOOL bDiskIO) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRecordService::CreateRecordSource
     *	Purpose:
     *	    creates a record source for a given.
     */
    STDMETHOD(CreateRecordSource) (THIS_ IUnknown* pUnkSource, 
					 REF(IHXRecordSource*) pRecordSource) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRecordService::OpenRecordSource
     *	Purpose:
     *	    open recorded source.
     */
    STDMETHOD(OpenRecordSource) (THIS_ const char* szURL, IUnknown* pUnkFileObject, 
				       REF(IHXRecordSource*) pRecordSource) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRecordService::CloseRecordSource
     *	Purpose:
     *	    closes record source.
     */
    STDMETHOD(CloseRecordSource) (THIS_ IHXRecordSource* pRecordSource) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXRecordSource
 * 
 *  Purpose:
 * 
 *	provide methods for RMA source recording.
 * 
 *  IID_IHXRecordSource:
 * 
 *	{E007F531-4EC9-4555-8EC5-1D58499904DF}
 * 
 */
DEFINE_GUID(IID_IHXRecordSource, 
0xe007f531, 0x4ec9, 0x4555, 0x8e, 0xc5, 0x1d, 0x58, 0x49, 0x99, 0x4, 0xdf);


DECLARE_INTERFACE_(IHXRecordSource, IUnknown)
{
    /******************************************************************
     * Method:
     *     IHXRecordSource::OnFileHeader
     *
     * Purpose:
     *	   send file header to record source
     *
     */
    STDMETHOD(OnFileHeader) (THIS_ IHXValues* pValues) PURE;

    /******************************************************************
     * Method:
     *    IHXRecordSource::OnStreamHeader
     *
     * Purpose:
     *	   send stream headers to record source
     */
    STDMETHOD(OnStreamHeader) (THIS_ IHXValues* pValues) PURE;

    /******************************************************************
     * Method:
     *    IHXRecordSource::OnPacket
     *
     * Purpose:
     *	   send packets to record source
     */
    STDMETHOD(OnPacket) (THIS_ IHXPacket* pPacket, INT32 nTimeOffset) PURE;

    /******************************************************************
     * Method:
     *    IHXRecordSource::OnEndOfPackets
     *
     * Purpose:
     *	   notification of source completion.
     */
    STDMETHOD(OnEndOfPackets) (THIS) PURE;

    /******************************************************************
     * Method:
     *    IHXRecordSource::Flush
     *
     * Purpose:
     *	   removes all previously recorded packets.
     */
    STDMETHOD(Flush) (THIS) PURE;

    /******************************************************************
     * Method:
     *     IHXRecordSource::SetFormatResponse
     *
     * Purpose:
     *	   sets formant response interface
     *
     */
    STDMETHOD(SetFormatResponse) (THIS_ IHXFormatResponse* pFormatResponse) PURE;

    /******************************************************************
     * Method:
     *     IHXRecordSource::GetFormatResponse
     *
     * Purpose:
     *	   gets current formant response interface
     *
     */
    STDMETHOD(GetFormatResponse) (THIS_ REF(IHXFormatResponse*) pFormatResponse) PURE;

    /******************************************************************
     * Method:
     *     IHXRecordSource::GetFileHeader
     *
     * Purpose:
     *	   asks for a file header
     *
     */
    STDMETHOD(GetFileHeader)	(THIS) PURE;

    /******************************************************************
     * Method:
     *    IHXRecordSource::GetStreamHeader
     *
     * Purpose:
     *	   asks for a stream headers
     *
     */
    STDMETHOD(GetStreamHeader)(UINT32 uStreamNumber) PURE;

    /******************************************************************
     * Method:
     *    IHXRecordSource::GetPacket
     *
     * Purpose:
     *	   asks for a packet 
     *
     */
    STDMETHOD(GetPacket) (THIS_ UINT16 nStreamNumber) PURE;

    /******************************************************************
     * Method:
     *    IHXRecordSource::Seek
     *
     * Purpose:
     *	   seeks to a certain position inside recorded content
     *
     */
    STDMETHOD(Seek) (THIS_ UINT32 nPosition) PURE;

    /******************************************************************
     * Method:
     *    IHXRecordSource::Pause
     *
     * Purpose:
     *	   notification about pausing 
     *
     */
    STDMETHOD(Pause) (THIS) PURE;

    /******************************************************************
     * Method:
     *     IHXRecordSource::SetSource
     *
     * Purpose:
     *	   passes stream source to the record source.
     *
     */
    STDMETHOD(SetSource) (THIS_ IUnknown* pUnkSource) PURE;
};


DEFINE_SMART_PTR( IHXRecordManager );
DEFINE_SMART_PTR( IHXRecordService );
DEFINE_SMART_PTR( IHXRecordSource );


#endif /* _IHXRECORD_H_ */

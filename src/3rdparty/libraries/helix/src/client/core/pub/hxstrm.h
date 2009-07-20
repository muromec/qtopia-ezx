/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxstrm.h,v 1.8 2007/07/06 21:58:16 jfinnecy Exp $
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

#ifndef _HXSTREAM_
#define _HXSTREAM_

class  CHXMapPtrToPtr;
class  CHXHeader;
class  HXPlayer;
class  HXSource;
class  HXASMStream;
struct IHXStream;
struct IHXStream3;
struct IHXStreamSource;
struct IHXASMStreamSink;
struct IHXASMSource;
struct IHXASMStream;
struct IHXRegistryID;

class HXStream :   public IHXStream3,
                   public IHXRegistryID,
		   public IHXLayoutStream	    
{
protected:
    LONG32			m_lRefCount;

public:

	    HXStream();
	    ~HXStream();

    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXStream methods
     */

    /************************************************************************
     *	Method:
     *	    IHXStream::GetSource
     *	Purpose:
     *	    Get the interface to the source object of which the stream is
     *	    a part of.
     *
     */
    STDMETHOD(GetSource)		    (THIS_
					    REF(IHXStreamSource*)	pSource);
    

    /************************************************************************
     *	Method:
     *	    IHXStream::GetStreamNumber
     *	Purpose:
     *	    Get the stream number for this stream relative to the source 
     *	    object of which the stream is a part of.
     *
     */
    STDMETHOD_(UINT16,GetStreamNumber)	    (THIS);

    /************************************************************************
     *	Method:
     *	    IHXStream::GetStreamType
     *	Purpose:
     *	    Get the MIME type for this stream. NOTE: The returned string is
     *	    assumed to be valid for the life of the IHXStream from which it
     *	    was returned.
     *
     */
    STDMETHOD_(const char*,GetStreamType)   (THIS);

    /************************************************************************
     *	Method:
     *	    IHXStream::GetHeader
     *	Purpose:
     *      Get the header for this stream.
     *
     */
    STDMETHOD_(IHXValues*,GetHeader)       (THIS);

    /************************************************************************
     *	Method:
     *	    IHXStream::ReportQualityOfService
     *	Purpose:
     *	    Call this method to report to the playback context that the 
     *	    quality of service for this stream has changed. The unQuality
     *	    should be on a scale of 0 to 100, where 100 is the best possible
     *	    quality for this stream. Although the transport engine can 
     *	    determine lost packets and report these through the user
     *	    interface, only the renderer of this stream can determine the 
     *	    "real" perceived damage associated with this loss.
     *
     *	    NOTE: The playback context may use this value to indicate loss
     *	    in quality to the user interface. When the effects of a lost
     *	    packet are eliminated the renderer should call this method with
     *	    a unQuality of 100.
     *
     */
    STDMETHOD(ReportQualityOfService)	    (THIS_
					    UINT8   unQuality);

    /************************************************************************
     *	Method:
     *	    IHXStream::ReportRebufferStatus
     *	Purpose:
     *	    Call this method to report to the playback context that the
     *	    available data has dropped to a critically low level, and that
     *	    rebuffering should occur. The renderer should call back into this
     *	    interface as it receives additional data packets to indicate the
     *	    status of its rebuffering effort.
     *
     *	    NOTE: The values of unNeeded and unAvailable are used to indicate
     *	    the general status of the rebuffering effort. For example, if a
     *	    renderer has "run dry" and needs 5 data packets to play smoothly
     *	    again, it should call ReportRebufferStatus() with 5,0 then as
     *	    packet arrive it should call again with 5,1; 5,2... and eventually
     *	    5,5.
     *
     */
    STDMETHOD(ReportRebufferStatus)	    (THIS_
					    UINT8   unNeeded,
					    UINT8   unAvailable);

    /************************************************************************
     *	Method:
     *	    IHXStream::SetGranularity
     *	Purpose:
     *	    Sets the desired Granularity for this stream. The actual 
     *	    granularity will be the lowest granularity of all streams.
     */
    STDMETHOD(SetGranularity)		    (THIS_
					    ULONG32 ulGranularity);

    /************************************************************************
     *	Method:
     *	    IHXStream::GetRendererCount
     *	Purpose:
     *	    Returns the current number of renderer instances supported by
     *	    this stream instance.
     */
    STDMETHOD_(UINT16, GetRendererCount)(THIS);

    /************************************************************************
     *	Method:
     *	    IHXStream::GetRenderer
     *	Purpose:
     *	    Returns the Nth renderer instance supported by this stream.
     */
    STDMETHOD(GetRenderer)	(THIS_
				UINT16		nIndex,
				REF(IUnknown*)	pUnknown);

    /************************************************************************
     *	Method:
     *	    IHXStream2::ReportAudioRebufferStatus
     *	Purpose:
     *      For audio only, when it's called, the rebuffer will only occur when
     *      there aren't any packets in the transport and the amount of audio in
     *      audio device falls below the minimum startup audio pushdown(1000ms
     *      by default)
     *      
     *      Non-audio renderers should still call ReportRebufferStatus(), the 
     *      rebuffer will occur when the core drains out all the packets from
     *      the transport buffer
     *
     *      The rest semantic are the same between the 2 calls.
     */
    STDMETHOD(ReportAudioRebufferStatus)    (THIS_
					    UINT8   unNeeded,
					    UINT8   unAvailable);

    /************************************************************************
     *  Method:
     *      IHXStream3::GetMinimumBufferingInMs
     *  Purpose:
     *      Provides the minimum buffering in ms that is being enforced by
     *	    the player for a particualr stream.  Media packets are dispatched
     *	    to the renderer the "minimum buffering in ms" ahead of playback
     *	    timeline. Determination of "minimum buffering in ms" is typically
     *	    based on required preroll associated with the media stream,
     *	    post decode delay associated with the media stream rendering and
     *	    additional delays imposed by the system due to explict user setting
     *	    of minimum preroll or dynamic discovery of network conditions 
     *	    causing playback disruptions.
     */
    STDMETHOD_(UINT32, GetMinimumBufferingInMs) (THIS);

    /************************************************************************
     *	Method:
     *	    IHXRegistryID::GetID
     *	Purpose:
     *	    Get registry ID(hash_key) of the objects(player, source and stream)
     *
     */
    STDMETHOD(GetID)	(THIS_
                        REF(UINT32) /*OUT*/  ulRegistryID);

    /*
     * IHXLayoutStream methods
     */
    /************************************************************************
     *	Method:
     *	    IHXLayoutStream::GetProperty
     *	Purpose:
     *	    Get a layout stream property
     *
     */
    STDMETHOD(GetProperties)	(THIS_
                        	REF(IHXValues*) /*OUT*/  pProps);

    /************************************************************************
     *	Method:
     *	    IHXLayoutStream::SetProperty
     *	Purpose:
     *	    Set a layout stream property
     *
     */
    STDMETHOD(SetProperties)	(THIS_
                        	IHXValues* /*IN*/  pProps);

    
    // other methods

    HX_RESULT	Init(HXPlayer* pPlayer, HXSource* pSource, IHXValues* pHeader, IUnknown* pUnkRenderer = NULL);
    HX_RESULT	ResetASMSource(IHXASMSource* pASMSource);
    HX_RESULT	SetRenderer(IUnknown* pUnkRenderer);
    void	SetBufferingFullfilled(void);
    HXBOOL	IsTimeStampDelivery();

    void	PostEndTimePacket(IHXPacket* pPacket, HXBOOL& bSentMe, HXBOOL& bEndMe);
    void	ResetASMRuleState(void);
    HXSource*	GetHXSource();
    HXBOOL	IsSureStream(void);

    HX_BITFIELD	m_bPostSeekToBeSent : 1;

protected:

    HXSource*	    m_pSource;
    IHXValues*	    m_pHeader;
    IUnknown*	    m_pUnkRenderer;
    UINT16	    m_uStreamNumber;
#if defined(HELIX_FEATURE_ASM)
    HXASMStream*    m_pASMStream;
#endif /* HELIX_FEATURE_ASM */
    UINT32          m_ulRegistryID;
};

#endif //_HXSTREAM_

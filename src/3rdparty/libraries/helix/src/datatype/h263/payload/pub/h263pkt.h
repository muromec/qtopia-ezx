/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#ifndef H263PKT_H
#define H263PKT_H

#include "hxcom.h"
#include "hxformt.h"
#include "hxccf.h"
#include "hxslist.h"
#include "hxwintyp.h"

#include "baseobj.h"
#include "hxplugn.h"
#include "hxplgns.h"

#define H263_MIME_TYPE "video/H263-2000"
 

struct IHXRTPPacket;
class CQT_TrackInfo_Manager;

/*
 * H263+ Packetizer - currently implementing only RFC2429
 */
class CH263Packetizer	: public IHXPayloadFormatObject,
                          public IHXPlugin,
                          public IHXPluginProperties,
                          public CHXBaseCountingObject
{
public:
    CH263Packetizer(CQT_TrackInfo_Manager* pTrackInfo = NULL);
    ~CH263Packetizer();

    	
    static HX_RESULT STDAPICALLTYPE HXCreateInstance(IUnknown** ppIUnknown);
    static HX_RESULT STDAPICALLTYPE CanUnload(void);
    static HX_RESULT STDAPICALLTYPE CanUnload2(void);
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)		(THIS);

    STDMETHOD_(ULONG32,Release)		(THIS);

    /*
     *	IHXPayloadFormatObject methods
     */
    STDMETHOD(Init)		(THIS_
				IUnknown* pContext,
				BOOL bPacketize);
    STDMETHOD(Close)		(THIS);
    STDMETHOD(Reset)		(THIS);
    STDMETHOD(SetStreamHeader)	(THIS_
				IHXValues* pHeader);
    STDMETHOD(GetStreamHeader)	(THIS_
				REF(IHXValues*) pHeader);
    STDMETHOD(SetPacket)	(THIS_
				IHXPacket* pPacket);
    STDMETHOD(GetPacket)	(THIS_
				REF(IHXPacket*) pPacket);
    STDMETHOD(Flush)		(THIS);

    	
 /*
    *	IHXPlugin methods
    */

    STDMETHOD(InitPlugin)	(THIS_
				IUnknown*   /*IN*/  pContext);

    STDMETHOD(GetPluginInfo)	(THIS_
				REF(HXBOOL) bLoadMultiple,
				REF(const char*) pDescription,
				REF(const char*) pCopyright,
				REF(const char*) pMoreInfoURL,
				REF(ULONG32) ulVersionNumber
				);
 
    /*
     *	IHXPluginProperties
     */
    STDMETHOD(GetProperties)  (THIS_
				REF(IHXValues*) pIHXValuesProperties
				);		

 
protected:
    void	FlushOutput(void);
    UINT8*	MakeBuffer(REF(IHXBuffer*)pOutBuf, UINT32 ulSize);

    // Out Packet routine
    typedef HX_RESULT (CH263Packetizer::*AddOutPkt)(IHXBuffer*, IHXPacket*, BOOL, UINT32, HXBOOL); 
    HX_RESULT AddOutHXPkt   (IHXBuffer* pBuf, IHXPacket* pInPkt, BOOL bMBit, UINT32 uiIterations, HXBOOL bIterationOver);    
    HX_RESULT AddOutHXRTPPkt(IHXBuffer* pBuf, IHXPacket* pInPkt, BOOL bMBit, UINT32 uiIterations, HXBOOL bIterationOver);

    /*
     * RFC2429 specific implementation
     */
    enum RTPMarkerBit
    {
	MARKER_TRUE,
	MARKER_FALSE,
	MARKER_UNKNOWN
    };
    HX_RESULT	PacketizeRFC2429(IHXPacket* pInHXPkt);
    HX_RESULT	GeneratePacket(UINT8* pData, UINT32 ulDataSize, IHXPacket* pInHXPkt, RTPMarkerBit mBit, UINT32 uiIterations, HXBOOL bIterationOver);
    UINT8*	WriteRTP263PlusPayloadHdr(UINT8* pc, BOOL bPBit);

    UINT32 PrepareOutPktASMFlags(IHXPacket* pInPkt, UINT32 uiIterations, HXBOOL bIterationOver);

    /*
     * Stream Header Routines
     */
    HX_RESULT	HandleMaxPacketSize(IHXValues* pHeader);     
    HX_RESULT	HandleSDP(IHXBuffer* pOpaque);
    void	HandleBitRates(IHXBuffer* pOpaque);
    HX_RESULT	HandleMimeType(void);

    
private:
    
    ULONG32 m_lRefCount;
    IUnknown*		   m_pContext;
    IHXCommonClassFactory* m_pCCF;

    IHXValues*	    m_pStreamHeader;
    UINT32	    m_ulMaxPktSize;
    UINT32	    m_ulCurMaxPktSize;

    AddOutPkt	    m_pOutPktQueue;
    CHXSimpleList   m_outputQueue;

    CQT_TrackInfo_Manager* m_pTrackInfo;

    enum
    {
	H263PKT_STATE_CLOSE = 0,
	H263PKT_STATE_READY = 1	
    }		    m_state;
    
    static const UINT32 zm_ulRTP263PlusHdrSize;
    static const char* zm_pDescription;
    static const char* zm_pCopyright;
    static const char* zm_pMoreInfoURL;
};

#endif /* H263PKT_H */

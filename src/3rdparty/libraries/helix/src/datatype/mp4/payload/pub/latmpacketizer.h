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

#ifndef _LATMPACKETIZER_H_
#define _LATMPACKETIZER_H_
#include "baseobj.h"
#include "hxplugn.h"
#include "hxplgns.h"

#define LATM_MIME_TYPE "audio/MP4A-LATM"
#define LATM_SDP_SIZE 128

class CAudioSpecificConfig;
class CTSConverter;

class LATMPacketizer : public IHXPlugin,
		    public IHXPluginProperties,
		    public IHXPayloadFormatObject,
			public CHXBaseCountingObject
{   
public:
    static HX_RESULT STDAPICALLTYPE HXCreateInstance(IUnknown** ppIUnknown);
    static HX_RESULT STDAPICALLTYPE CanUnload(void);
    static HX_RESULT STDAPICALLTYPE CanUnload2(void);
	
public:
    LATMPacketizer(UINT32 ulChannels = 0, UINT32 ulTimeScale = 0);
    ~LATMPacketizer();
  /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);
   
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
				  
  /*
     *	IHXPayloadFormatObject methods
     */
    STDMETHOD(Init)             (THIS_ IUnknown* pContext, BOOL bPacketize);
    STDMETHOD(Close)            (THIS);
    STDMETHOD(Reset)            (THIS);
    STDMETHOD(SetStreamHeader)  (THIS_ IHXValues* pHeader);
    STDMETHOD(GetStreamHeader)  (THIS_ REF(IHXValues*) pHeader);
    STDMETHOD(SetPacket)        (THIS_ IHXPacket* pPacket);
    STDMETHOD(GetPacket)        (THIS_ REF(IHXPacket*) pPacket);
    STDMETHOD(Flush)            (THIS);
   
 	
private:
    inline static UINT32 SampleHeaderSize  (UINT32 ulSize) 
        { return ulSize/0xFF + 1; }
    static void CreateSampleHeader  (UINT8 *pHeader, UINT32 ulSampleSize);

    HX_RESULT GetHeaderProperties   ();
    HX_RESULT AddHeaderMimeType     ();
    HX_RESULT AddHeaderSDPData      ();
    HX_RESULT InitializeResampling  ();

    static int WriteFMTP            (char* pBuf,
                                     UINT32 ulSize, 
                                     UINT8* pConfig, 
                                     UINT32 ulConfigSize,
                                     UINT32 ulRTPPayloadType,
                                     CAudioSpecificConfig& audioConfig);
    static int FormatStreamMuxConfig(char* pConfigString, 
                                     UINT32 ulSize,
                                     UINT8* pConfig, 
                                     UINT32 ulConfigSize,
                                     UINT32 ulFramesPerPacket);
    static void BitBufferImportBytes(UINT8* pTarget,
                                     UINT32 ulTargetBitOffset,
                                     UINT8* pSource,
                                     UINT32 ulSourceSize);
									 
    static const char* zm_pDescription;
    static const char* zm_pCopyright;
    static const char* zm_pMoreInfoURL;

    INT32                   m_lRefCount;
    IUnknown*               m_pContext;
    IHXCommonClassFactory*  m_pClassFactory;
    IHXValues*              m_pStreamHeader;
    CHXSimpleList           m_InputPackets;
    HXBOOL                  m_bUsesRTPPackets;
    HXBOOL                  m_bRTPPacketTested;
    HXBOOL                  m_bFlushed;
    UINT32                  m_ulChannels;
    UINT32                  m_ulInputTimeScale;
    UINT32                  m_ulOutTimeScale;
    CTSConverter*           m_pTSConverter;
};

#endif /* _LATMPACKETIZER_H_ */

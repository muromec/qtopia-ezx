/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: inputsrc_push.h,v 1.5 2003/09/04 22:39:08 dcollins Exp $ 
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

#ifndef _STATIC_PUSH_SOURCE_H_
#define _STATIC_PUSH_SOURCE_H_

#include "hxtypes.h"
#include "hxcom.h"
#include "hxfiles.h"
#include "hxformt.h"
#include "hxsrc.h"

#include "sink.h"
#include "source.h"
#include "client.h"

class Process;
class CHXMapLongToObj;

class StreamPacketState 
{ 
 public:
    StreamPacketState ();
    ~StreamPacketState ();

    void SetPacket(ServerPacket* pPacket);
    ServerPacket* GetPacket();

    BOOL       m_bBlocked; 
    BOOL       m_bDone;

 private:
    ServerPacket* m_pPacket; 
};

class StaticPushSource: public IHXPSourceControl,
			public IHXServerPacketSource,
			public IHXFormatResponse
{
public:
    StaticPushSource(Process* proc,
		     IHXFileFormatObject* file_format,
		     IUnknown* pFileObject,
		     IHXRequest* pRequest);
    ~StaticPushSource();

    /* XXXSMP */

    HX_RESULT Init(IHXPSinkControl* pSink, IHXValues* values);

    /* IUnknown Interfaces */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    /* IHXPSourceControl Interfaces */

    STDMETHOD(Init)             (THIS_
                                IHXPSinkControl*             pSink);
    STDMETHOD(Done)		(THIS);
    STDMETHOD(GetFileHeader)    (THIS_
                                IHXPSinkControl*             pSink);
    STDMETHOD(GetStreamHeader)  (THIS_
                                IHXPSinkControl*             pSink,
                                UINT16 unStreamNumber);
    STDMETHOD(Seek)             (THIS_
                                UINT32          ulSeekTime);
    STDMETHOD_(BOOL,IsLive)     (THIS);
    STDMETHOD(SetLatencyParams) (THIS_
                                 UINT32 ulLatency,
                                 BOOL bStartAtTail,
                                 BOOL bStartAtHead) { return HXR_OK; }

    /* IHXServerPacketSource */
    STDMETHOD(SetSink)(THIS_ IHXServerPacketSink* pSink);
    STDMETHOD(StartPackets) (THIS);
    STDMETHOD(GetPacket) (THIS);
    STDMETHOD(SinkBlockCleared)(THIS_ UINT32 ulStream);
    STDMETHOD(EnableTCPMode) (THIS) {return HXR_OK;}

    /* IHXFormatResponse Interfaces */
    STDMETHOD(InitDone)		(THIS_
				HX_RESULT	status);
    STDMETHOD(FileHeaderReady)	(THIS_
				HX_RESULT	status,
				IHXValues*	pHeader);
    STDMETHOD(StreamHeaderReady)
				(THIS_
				HX_RESULT status, 
				IHXValues* header);
    STDMETHOD(PacketReady)	(THIS_
				HX_RESULT status, 
				IHXPacket* packet);
    STDMETHOD(SeekDone)	    	(THIS_
				HX_RESULT status);
    STDMETHOD(StreamDone)	(THIS_
				UINT16 stream_number);
private:
    Process*			m_proc;

    IUnknown*                   m_pFileObject;
    IHXFileFormatObject*	m_pFileFormat;
    IHXPSinkControl*		m_pSinkControl;
    IHXServerPacketSink*	m_pSink;

    StreamPacketState*          m_pStreams;
    IHXValues*                  m_pBWEValues;
    IHXRequest*                 m_pRequest;
    CHXMapLongToObj*		m_streams;

    UINT16                      m_nCurrentStream;
    UINT32			m_lNumStreams;
    LONG32			m_lRefCount;

    UINT32			m_ulRecursionCounter;

    void                 RequestPacket();
    void                        TransmitPacket(ServerPacket* pPacket);
    //XXXDWL Duplicate from BasicPacketSource, clean this up...
    ServerPacket*               HXPacketToServerPacket(IHXPacket* pPacket);
};

#endif /* _STATIC_PUSH_SOURCE_H_ */



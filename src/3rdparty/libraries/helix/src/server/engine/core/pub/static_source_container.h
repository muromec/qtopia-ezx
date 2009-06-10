/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: static_source_container.h,v 1.3 2004/05/03 19:02:48 tmarshall Exp $ 
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

#ifndef STATIC_SOURCE_CONTAINER_H_
#define STATIC_SOURCE_CONTAINER_H_

#include "hxcom.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxformt.h"
#include "hxsrc.h"
#include "sink_container.h"
#include "source.h"
#include "timeval.h"

class GetPacketCallback;

class StaticSourceContainer : public IHXRawSourceObject,
                              public IHXPSinkPackets
{
public:
   

    StaticSourceContainer(IUnknown* pContext, IHXPSourceControl* pSource);

	/* IHXUnknown methods */

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    /* IHXRawSourceObject methods */

    STDMETHOD(Init)		(THIS_
				IUnknown* pUnknown);

    STDMETHOD(Done)		(THIS);

    STDMETHOD(GetFileHeader)	(THIS);

    STDMETHOD(GetStreamHeader)	(THIS_
				UINT16 unStreamNumber);

    STDMETHOD(StartPackets)	(THIS_
				UINT16 unStreamNumber);

    STDMETHOD(StopPackets)	(THIS_
				UINT16 unStreamNumber);

    /* IHXPSinkPackets methods */

    STDMETHOD(PacketReady)      (THIS_
                                HX_RESULT               ulStatus,
                                IHXPacket*             pPacket);
 
private:
    BOOL                    m_bReady;
    BOOL                    m_bPacketsStarted; 
    BOOL                    m_bAdvanceSchedulerTime;
    BOOL                    m_bOneStreamStarted;
    BOOL		    m_bGetPacketOutstanding;    
    BOOL                    m_bDone;

    UINT16                  m_nStreamCount;

    LONG32		    m_lRefCount;
   
    UINT32		    m_uGetPacketCallbackHandle;

    ULONG32                 m_ulStopPacketsSeen;
   
    Timeval                 m_StartTime;
    Timeval                 m_SchedulerTimeOffset;

    IUnknown*			m_pContext;
    IHXPSourceControl*		m_pSourceControl;
    IHXPSourcePackets*		m_pSourcePackets;
    IHXScheduler*		m_pScheduler;
    IHXThreadSafeScheduler*    m_pThreadSafeScheduler;

    GetPacketCallback**		m_ppPacketStreams;
    SinkContainer*		m_pSink;

    ~StaticSourceContainer();

    friend class GetPacketCallback;
};


/* 
 *  Class for handling GetPacket scheduling
 */

class GetPacketCallback : public IHXCallback
{
 public:
    GetPacketCallback(StaticSourceContainer* pOwner, UINT16 unStreamNumber);
    ~GetPacketCallback();

    /* IUnknown Interfaces */

    STDMETHOD(QueryInterface)	(THIS_
				 REFIID riid,
				 void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /* IHXCallback method */

    STDMETHOD(Func)			(THIS);

    /* GetPacketCallback methods */
	
    void   ScheduleNextPacket();

 private:
    LONG32			  m_lRefCount;
    StaticSourceContainer* 	  m_pOwner;
	
    CallbackHandle		  m_Handle;
    HXTimeval		          m_pTimeToSchedule;
	
    BOOL	                  m_bPacketsStarted;
    BOOL                          m_bThreadSafeGetPacket;
	
    UINT16	                  m_unStreamNo;
    
    friend class StaticSourceContainer;
};



#endif /* _SOURCE_CONTAINER_H_ */



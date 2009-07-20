/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: ff_source.h,v 1.3 2003/09/04 22:39:08 dcollins Exp $ 
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

#ifndef _FF_SOURCE_H_
#define _FF_SOURCE_H_

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

class FileFormatSource: public IHXPSourceControl,
			public IHXPSourcePackets,
			public IHXFormatResponse
{
public:
    FileFormatSource(Process* proc,
		     IHXFileFormatObject* file_format,
		     UINT32 mount_point_len,
		     IUnknown* pFileObject,
		     IHXRequest* pRequest,
		     BOOL bIsLive);
    ~FileFormatSource();

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

    /* IHXPSourcePackets Interfaces */

    STDMETHOD(Init)             (THIS_
                                IHXPSinkPackets*             pSink);

    STDMETHOD(GetPacket)        (THIS_
                                UINT16 unStreamNumber);

    STDMETHOD_(UINT32, IsThreadSafe)     (THIS);

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
    UINT32			m_lNumStreams;
    LONG32			m_lRefCount;
    Process*			m_proc;
    IHXFileFormatObject*	m_file_format;
    BOOL                    m_bFileFormatInitialized;
    BOOL			m_is_ready;
    BOOL                        m_is_live;
    IHXPSinkControl*		m_pSinkControl;
    IHXPSinkPackets*		m_pSinkPackets;
    CHXMapLongToObj*		m_streams;
    IUnknown*                   m_pFileObject;
    UINT32                      m_mount_point_len;
    IHXRequest*		m_pRequest;
    IHXValues*                 m_bwe_values;
    char*			m_pURL;
    UINT32			m_ulThreadSafeFlags;

    void                        FixHeaderForLive(IHXValues*& pHeader);
};

#endif /* _FF_SOURCE_H_ */



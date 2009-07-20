/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: dtcvtcon.h,v 1.2 2003/01/23 23:42:57 damonlan Exp $ 
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

#include "ts_overlord.h"
class Process;

class DataConvertShim : public IHXDataConvertResponse,
			public ThreadSafeOverlord
{
public:
    DataConvertShim(IHXDataConvert* pConvert, Process* proc);
    ~DataConvertShim();
    
    /*
     * IUnknown
     */
    STDMETHOD(QueryInterface) (THIS_
			       REFIID riid,
			       void** ppvObj);
    STDMETHOD_(ULONG32, AddRef)         (THIS);
    STDMETHOD_(ULONG32, Release)        (THIS);

    /************************************************************************
     *  IHXDataConvertResponse
     */
    STDMETHOD(DataConvertInitDone) (THIS_ HX_RESULT status);
    
    STDMETHOD(ConvertedFileHeaderReady) (THIS_
			    HX_RESULT status, IHXValues* pFileHeader);
    
    STDMETHOD(ConvertedStreamHeaderReady) (THIS_
			    HX_RESULT status, IHXValues* pStreamHeader);

    STDMETHOD(ConvertedDataReady) (THIS_ HX_RESULT status,
	    				IHXPacket* pPacket);

    STDMETHOD(SendControlBuffer) (THIS_ IHXBuffer* pBuffer);


    
    void SetControlResponse(IHXDataConvertResponse* pResp);
    void SetDataResponse(IHXDataConvertResponse* pResp);
    void DataConvertInit(IUnknown* pContext);
    void ConvertFileHeader(IHXValues* pHeader);
    void ConvertStreamHeader(IHXValues* pHeader);
    void ConvertData(IHXPacket* pPacket);
    void GetConversionMimeType(REF(const char*)pConversionType);
    void SetRequest(IHXRequest* pRequest);
    void ControlBufferReady(IHXBuffer* pBuffer);
    void Done();
    void SetMulticastTransport(DataConvertShim*);
    void SetMulticastTransportFor(IHXDataConvert*);
    void AddMulticastControl(DataConvertShim*);
    void AddMulticastControlFor(IHXDataConvert*);
					
private:
    IHXDataConvertResponse* m_pControlResponse;
    IHXDataConvertResponse* m_pDataResponse;
    IHXDataConvert* m_pConverter;
    BOOL m_bWaitForInitDone;
    INT32 m_lRefCount;
    CHXSimpleList* m_pCachePacketList;
};


class DataConvertController
{
public:
    DataConvertController();
    ~DataConvertController();
    
    IHXDataConvert* GetConverter(Process* proc, IHXBuffer* pURL);
    
private:
};

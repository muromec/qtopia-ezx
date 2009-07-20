/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: dtrvtcon.h,v 1.5 2007/07/06 21:58:11 jfinnecy Exp $
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

#include "hxdtcvt.h"
#include "packfilt.h"
#include "hxplugn.h"
#include "hxslist.h"

class DataRevertControllerResponse
{
public:
    virtual void RevertHeadersDone(IHXValues* pFileHeader,
				   CHXSimpleList* pStreamHeaders, 
				   IHXValues* pResponseHeaders,
				   HXBOOL bUseReverter) = 0;
    virtual void SendControlBuffer(IHXBuffer* pBuffer) = 0;
};

class DataRevertController : public IHXDataRevertResponse,
				public RawPacketFilter
{
public:
    DataRevertController(IUnknown* pContext);
    ~DataRevertController();

    void SetControlResponse(DataRevertControllerResponse* pResp);
    void RevertHeaders(IHXValues* pFileHeader,
	    CHXSimpleList* pStreamHeaders,
	    IHXValues* pResponseHeaders);
    void ControlBufferReady(IHXBuffer* pBuffer);

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)          (THIS);

    STDMETHOD_(ULONG32,Release)         (THIS);

    /************************************************************************
     *  IHXDataRevertResponse
     */
    STDMETHOD(DataRevertInitDone) (THIS_ HX_RESULT status);
    
    STDMETHOD(RevertedFileHeaderReady)  (THIS_ 
                                HX_RESULT status, IHXValues* pHeader);
    
    STDMETHOD(RevertedStreamHeaderReady) (THIS_
                                HX_RESULT status, IHXValues* pHeader);

    STDMETHOD(RevertedDataReady) (THIS_ HX_RESULT status,
                                        IHXPacket* pPacket);

    STDMETHOD(SendControlBuffer) (THIS_ IHXBuffer* pBuffer);
    
    
    /*
     * RawPacketFilter methods
     */
    void FilterPacket(IHXPacket* pPacket);
    void SetFilterResponse(RawPacketFilter*);
    
    void Done(void);

private:
    IHXValues* InflateConvertHeader(IHXBuffer* pBuffer);
    IUnknown* m_pContext;
    IHXPlugin2Handler* m_pPlugin2Handler;
    DataRevertControllerResponse* m_pControlResp;
    INT32 m_lRefCount;
    IHXDataRevert* m_pDataRevert;
    
    void CleanStreamHeaders();
    void CleanControlBuffers();
    CHXSimpleList* m_pStreamHeaders;
    CHXSimpleList* m_pRevertedStreamHeaders;
    IHXValues* m_pFileHeaders;
    IHXValues* m_pResponseHeaders;
    IHXValues* m_pCurrentStreamHeader;
    RawPacketFilter* m_pDataResp;
    CHXSimpleList* m_pPacketList;
    CHXSimpleList* m_pControlBufferList;
    HXBOOL m_bInited;
};


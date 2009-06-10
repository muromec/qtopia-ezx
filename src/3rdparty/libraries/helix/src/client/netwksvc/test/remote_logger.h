/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: remote_logger.h,v 1.5 2007/07/06 21:58:25 jfinnecy Exp $
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

#include "hxtypes.h"
#include "hxcom.h"

#include "hxengin.h"  // IHXNetworkServices
#include "hxccf.h"    // IHXCommonClassFactory
#include "ihxpckts.h" // IHXBuffer

#include "hxslist.h" // CHXSimpleList

typedef enum {HXLogOpen = 0xf1,
	      HXLogEntry = 0xf2,
	      HXLogClose = 0xf3} HXLogMesgType;

typedef enum {HXLogClosed,
	      HXLogConnecting,
	      HXLogConnected,
	      HXLogError} HXLoggerState;


class HXRemoteLogger : public IHXTCPResponse
{
public:
    HXRemoteLogger();
    ~HXRemoteLogger();

    HX_RESULT Init(IUnknown* pContext, 
		   const char* pRemoteHost,
		   UINT16 nRemotePort,
		   const char* pLogFilename);

    void Log(const char* pLogStr);
    void Close();

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)		(THIS);

    STDMETHOD_(ULONG32,Release)		(THIS);

    /*
     *	IHXTCPResponse methods
     */

    /************************************************************************
     *	Method:
     *	    IHXTCPResponse::ConnectDone
     *	Purpose:
     *	    A Connect operation has been completed or an error has occurred.
     */
    STDMETHOD(ConnectDone)	(THIS_
				HX_RESULT		status);

    /************************************************************************
     *	Method:
     *	    IHXTCPResponse::ReadDone
     *	Purpose:
     *	    A Read operation has been completed or an error has occurred.
     *	    The data is returned in the IHXBuffer.
     */
    STDMETHOD(ReadDone)		(THIS_
				HX_RESULT		status,
				IHXBuffer*		pBuffer);

    /************************************************************************
     *	Method:
     *	    IHXTCPResponse::WriteReady
     *	Purpose:
     *	    This is the response method for WantWrite.
     *	    If HX_RESULT is ok, then the TCP channel is ok to Write to.
     */
    STDMETHOD(WriteReady)	(THIS_
    				HX_RESULT		status);

    /************************************************************************
     *	Method:
     *	    IHXTCPResponse::Closed
     *	Purpose:
     *	    This method is called to inform you that the TCP channel has
     *	    been closed by the peer or closed due to error.
     */
    STDMETHOD(Closed)		(THIS_
				HX_RESULT		status);
private:
    IHXBuffer* CopyBuffer(const char* pStr, UINT32 ulLength) const;
    
    void ClearMesgQueue();

    void OnError();
 
    HX_RESULT QueueMesg(HXLogMesgType type, IHXBuffer* pPayload);

    void SendQueuedMesgs();
    
    void PackUInt32(UCHAR* pBuf, UINT32 ulVal) const;
    HX_RESULT SendMesg(HXLogMesgType type, IHXBuffer* pPayload);
   
    ULONG32 m_lRefCount;

    IHXNetworkServices* m_pNetSvc;
    IHXCommonClassFactory* m_pCCF;
    IHXTCPSocket* m_pSocket;

    IHXBuffer* m_pRemoteHost;
    UINT16 m_remotePort;
    CHXSimpleList m_mesgList;

    HXLoggerState m_state;
    UINT32 m_seq;
};

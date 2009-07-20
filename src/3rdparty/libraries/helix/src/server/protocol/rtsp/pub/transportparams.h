/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: transportparams.h,v 1.4 2007/02/14 18:50:45 tknox Exp $
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

#ifndef _TRANSPORTPARAMETERS_H_
#define _TRANSPORTPARAMETERS_H_

#include "hxnet.h"
#include "bcngtypes.h"

/* Get _RTSPTransportTypeEnum and _RTSPTransportModeEnum from rtspif.h
 * along with all the macros for determining the type of transport 
 * (IS_CLIENT_TRANSPORT, IS_RDT_TRANSPORT, etc)
 */

#include "rtspif.h"

#ifndef MAX_HOST_LEN
#define MAX_HOST_LEN 256
#endif

typedef HXCOMPtr<RTSPTransportParams> SPRTSPTransportParams;

class RTSPServerProtocol;

class RTSPTransportParams : public IUnknown
{
public:
    RTSPTransportParams(void);

    bool operator==(const RTSPTransportParams& b)
    {
        if ((m_lTransportType == b.m_lTransportType) &&
            (m_Mode == b.m_Mode) &&
            (m_sPort == b.m_sPort))
        {
            return TRUE;
        }

        return FALSE;
    }

    // IUnknown methods.

    STDMETHODIMP QueryInterface(REFIID ID, void** ppInterfaceObj);
    STDMETHODIMP_(UINT32) AddRef();
    STDMETHODIMP_(UINT32) Release();

private:
    ~RTSPTransportParams();

    UINT32                       m_ulRefCount;

public:
    RTSPTransportTypeEnum       m_lTransportType;
    RTSPTransportModeEnum       m_Mode;
    UINT16                      m_sPort;
    UINT16                      m_streamNumber;
    UINT32                      m_ulBufferDepth;
    IHXSockAddr*                m_pDestAddr;
    UINT32                      m_ulStreamID;
    BOOL                        m_bAggregateTransport;
    IHXSocket*                  m_pSockets[2];

    /// BCNG (live broadcast through a Real proxy) specific vars
    UINT32                      m_ulPullSplitID;
    ProtocolType::__PType       m_Protocol;
    BOOL                        m_bResendSupported;
    UINT8                       m_ucFECLevel;
    UINT8                       m_ucTTL;
    UINT8                       m_ucSecurityType;
    IHXBuffer*                  m_pAuthenticationDataField;

    /** More BCNG - but these paramters all come from the BCNG transport
      * itself (they are needed in the responses "Transport" header) */
    UINT32                      m_ulBCNGStartTime;
    UINT32                      m_ulBCNGSessionID;
    UINT16                      m_usBCNGPort;
    UINT8                       m_ucBCNGTCPInterleave;
};



class RTSPTransportInstantiator : public IUnknown
{
public:
    RTSPTransportInstantiator(BOOL bAggregate);

    HX_RESULT Init(IUnknown* pContext,
                   RTSPServerProtocol* pServerProtocol);

    // IUnknown methods.

    STDMETHODIMP QueryInterface(REFIID ID, void** ppInterfaceObj);
    STDMETHODIMP_(UINT32) AddRef();
    STDMETHODIMP_(UINT32) Release();

    /**
     * \brief IsAggregate tells whether this is an aggregate transport.
     *
     * \param None.
     * \return HXBOOL Is this an Aggregate-Transport?
     */
    BOOL IsAggregateTransport (void) { return m_bAggregateTransport; }

    BOOL PlayCapableTransportExists (void) { return TRUE; }
    BOOL DataCapableTransportExists (void);
 
    HX_RESULT parseTransportHeader (IHXMIMEHeader* pHeader,
                                    UINT32 ulStrmCtrlID = 0);

    BOOL matchSelected (IHXMIMEHeader* pHeader);

    BOOL CanUseTransport     (RTSPTransportParams* pParams,
                              BOOL bAllowRARTP,
                              BOOL bForceRTP,
                              BOOL bAllowDest,
                              BOOL bIsRealDatatype,
                              BOOL bMulticastOK,
                              BOOL bRequireMulticast,
                              BOOL bIsMidBox,
                              BOOL bIsFirstSetup);

    HX_RESULT selectTransport(RTSPServerProtocol::Session* pSession,
                              UINT16 usStreamNumber = 0);

    HX_RESULT selectTransport(BOOL bAllowRARTP,
                              BOOL bForceRTP,
                              BOOL bAllowDest,
                              BOOL bIsRealDatatype,
                              BOOL bMulticastOK,
                              BOOL bRequireMulticast,
                              BOOL bIsMidBox,
                              BOOL bIsFirstSetup,
                              RTSPTransportParams* pExistingAggregateParams,
                              UINT32 ulControlID = 0,
                              UINT16 uStreamNumber = 0);

    HX_RESULT setLocalAddress(IHXSockAddr* pLocalAddr);

    HX_RESULT SetBCNGParameters(UINT16 streamNumber, UINT32 ulSessionID, UINT32 ulStartTime, 
                                UINT16 usBCNGPort, UINT8 ucTCPInterleave);

    /**
     * \brief GetTransportParams gets the RTSPTransportParams associated with this RTSPTransportInstantiator.
     *
     * \param ppParams (OUT) Pointer to a pointer to an RTSPTransportParams to be populated.
     * \param streamNumber (IN) Optionally the stream number to get the params for.
     * \return HXR_FAIL If not m_bSelected or no RTSPTransportParams found for stream.
     */
    HX_RESULT GetTransportParams(RTSPTransportParams** ppParams,
                                 UINT16 streamNumber = 0);

    IHXMIMEHeader* MakeTransportHeader(BOOL bAggregate,
                                       UINT16 streamNumber = 0);

    /**
     * \brief CreateTransport creates a suitable transport based on the transport header.
     *
     * \param bIsAggregate (IN) Is this an Aggregate-Transport?
     * \param ppTransport (OUT) The newly created transport, or a pointer to NULL on failure.
     * \return HXR_OK or failure codes (TBD).
     */
    STDMETHOD(CreateTransport) (THIS_ HXBOOL /*IN*/ bIsAggregate,
                                        void** /*OUT*/ ppTransport);

    HX_RESULT SetupTransportTNGTcpRDTTcp(RTSPServerProtocol::Session* pSession,
        RTSPStreamInfo* pStreamInfo,
        UINT16 usStreamNumber);
    HX_RESULT SetupTransportTNGUdpRDTUdpMcast(
        RTSPServerProtocol::Session* pSession,
        RTSPStreamInfo* pStreamInfo,
        RTSPTransportParams* pTransParams, UINT16 usStreamNumber);
    HX_RESULT SetupTransportRTPUdp(RTSPServerProtocol::Session* pSession,
        RTSPStreamInfo* pStreamInfo,
        RTSPTransportParams* pTransParams, UINT16 usStreamNumber);
    HX_RESULT SetupTransportRTPTcp(RTSPServerProtocol::Session* pSession,
        RTSPStreamInfo* pStreamInfo,
        UINT16 usStreamNumber);
    HX_RESULT SetupTransportNULLSet(RTSPServerProtocol::Session* pSession,
        RTSPStreamInfo* pStreamInfo,
        UINT16 usStreamNumber);
    HX_RESULT SetupTransportBCNGUdpTcpMcast(
        RTSPServerProtocol::Session* pSession,
        RTSPStreamInfo* pStreamInfo,
        RTSPTransportParams* pTransParams, UINT16 usStreamNumber);

    IHXSockAddr* getPeerAddress(RTSPTransportParams* pTransParams);

private:
    ~RTSPTransportInstantiator(void);

    void clearTransportParamsList(void);

    void GetUdpPortRange(UINT16& nMin, UINT16& nMax);

    HX_RESULT createUDPSockets (RTSPTransportParams* pTransParams);

    RTSPTransportParams* parseTransportField (IHXMIMEField* pField,
                                              UINT32 ulStrmCtrlID = 0);

    IUnknown*                    m_pContext;
    UINT32                       m_ulRefCount;

    CHXSimpleList                m_transportParamsList;
    BOOL                         m_bSelected;
    BOOL                         m_bAggregateTransport;

    IHXSockAddr*                 m_pLocalAddr;

    RTSPServerProtocol*          m_pServProt;
    UINT16                       m_usFirstStream;
};


#endif /* _TRANSPORTPARAMETERS_H_ */


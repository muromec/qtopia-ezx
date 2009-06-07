 /* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_prof_mgr.h,v 1.6 2007/04/24 05:11:08 npatil Exp $ 
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
#ifndef _QOS_PROF_MGR_H_
#define _QOS_PROF_MGR_H_

/* Example Config: 
<List Name="MediaDelivery"> 
   <List Name="UserAgentProfiles"> 
      <List Name="Helix DNA"> 
         <Var UserAgent="HelixDNAClient"/> 
         <Var StaticPush="0"/> 
         <Var UseFlowManager="0"/> 
         <List Name="Transport"> 
            <Var RtcpRRratio="200"/> 
            <Var RtcpRSratio="100"/> 
            <List Name="CongestionControl"> 
               <Var UDPType="TFRC"/> 
               <Var MaxBurst="5"/> 
               <List Name="TFRC"> 
                  <Var MaxOversendRate="125"/> 
                  <Var EnableIIRForRTT="1"/> 
               </List> 
            </List> 
         </List> 
        <List Name="Session"> 
           <List Name="RateManager"> 
              <Var BufferModel="ANNEXG"/> 
              <Var Type="ANNEXG"/> 
              <Var LowMark="200"/> 
              <Var HighMark="200"/> 
           </List> 
        </List> 
      </List> 
      <!-- If there is no matching User-Agent: --> 
      <List Name="Default"> 
         <Var StaticPush="0"/> 
         <Var UseFlowManager="0"/> 
     </List> 
   </List> 
 </List> 
*/

class UserAgentProfile
{
 public:
  UserAgentProfile (const char* pUserAgent, INT32 ulConfifId);
  ~UserAgentProfile ();

  char*      m_pUserAgent;
  UINT32     m_ulUserAgentLen;
  INT32      m_ulConfigId;
};

class QoSProfileSelector : public IHXQoSProfileSelector
{
 public:
    QoSProfileSelector (Process* pProc);
    ~QoSProfileSelector ();
    
    /* IHXUnknown methods */
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXQoSProfileSelector
    STDMETHOD (SelectProfile)   (THIS_ IHXBuffer* pUserAgent,
				 IHXBuffer* pTransportMime,
				 IHXBuffer* pMediaMime,
				 REF(INT32) /*OUT*/ ulConfigID);
 private:
    void                         UpdateProfiles();

    LONG32                        m_lRefCount;
    Process*                      m_pProc;

    UserAgentProfile**            m_pProfiles;
    UINT32                        m_ulNumProfiles;
    UINT32                        m_ulProfileRoot;
    UINT32                        m_ulDefaultProfile;
};

#endif /*_QOS_PROF_MGR_H_ */

/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_signal.h,v 1.4 2005/06/29 14:59:06 dcollins Exp $ 
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

#ifndef _QOS_SIGNAL_H_
#define _QOS_SIGNAL_H_

class QoSSignal : public IHXQoSSignal
{
 public:
    QoSSignal();
    ~QoSSignal();

    QoSSignal (BOOL bRef, IHXBuffer* pBuffer, HX_QOS_SIGNAL ulSignalId);

    inline HX_QOS_SIGNAL GetSignalId()   { return m_ulId; }
    inline IHXBuffer* GetSignalValue()   { (m_pValue ? m_pValue->AddRef(): 0); return m_pValue; }
    inline UINT32 GetSignalValueUINT32() { return m_ulValue; }

    inline void SetSignalId(HX_QOS_SIGNAL ulId)      { m_ulId = ulId; }
    inline void SetSignalValue(IHXBuffer* pBuffer)   { pBuffer->AddRef(); m_pValue = pBuffer; }
    inline void SetSignalValueUINT32(UINT32 ulValue) { m_ulValue = ulValue; }

    
    /* IHXUnknown methods */
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    /* IHXQoSSignal */
    STDMETHOD (GetId)           (THIS_ REF(HX_QOS_SIGNAL) ulSignalId);
    STDMETHOD (SetId)           (THIS_ HX_QOS_SIGNAL ulSignalId);
    STDMETHOD (GetValueUINT32)  (THIS_ REF(UINT32) ulValue);
    STDMETHOD (SetValueUINT32)  (THIS_ UINT32 ulValue);
    STDMETHOD (GetValue)        (THIS_ REF(IHXBuffer*) pBuffer);
    STDMETHOD (SetValue)        (THIS_ IHXBuffer* pBuffer);
    STDMETHOD (WriteSignal)     (THIS_ UINT32 ulFileDescriptor);

 private:
    LONG32                      m_lRefCount;
    HX_QOS_SIGNAL               m_ulId;
    UINT32                      m_ulValue;
    IHXBuffer*                  m_pValue;
};

#endif /* _QOS_SIGNAL_H_*/

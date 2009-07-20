/* ***** BEGIN LICENSE BLOCK *****
 *
 * Copyright Notices: 
 *  
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved. 
 *  
 * Patent Notices: This file may contain technology protected by one or  
 * more of the patents listed at www.helixcommunity.org 
 *  
 * 1.   The contents of this file, and the files included with this file, 
 * are protected by copyright controlled by RealNetworks and its  
 * licensors, and made available by RealNetworks subject to the current  
 * version of the RealNetworks Public Source License (the "RPSL")  
 * available at  http://www.helixcommunity.org/content/rpsl unless  
 * you have licensed the file under the current version of the  
 * RealNetworks Community Source License (the "RCSL") available at 
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
 * will apply.  You may also obtain the license terms directly from 
 * RealNetworks.  You may not use this file except in compliance with 
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable 
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
 * the rights, obligations and limitations governing use of the 
 * contents of the file. 
 *  
 * 2.  Alternatively, the contents of this file may be used under the 
 * terms of the GNU General Public License Version 2 (the 
 * "GPL") in which case the provisions of the GPL are applicable 
 * instead of those above.  Please note that RealNetworks and its  
 * licensors disclaim any implied patent license under the GPL.   
 * If you wish to allow use of your version of this file only under  
 * the terms of the GPL, and not to allow others 
 * to use your version of this file under the terms of either the RPSL 
 * or RCSL, indicate your decision by deleting Paragraph 1 above 
 * and replace them with the notice and other provisions required by 
 * the GPL. If you do not delete Paragraph 1 above, a recipient may 
 * use your version of this file under the terms of any one of the 
 * RPSL, the RCSL or the GPL. 
 *  
 * This file is part of the Helix DNA Technology.  RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the 
 * portions it created.   Copying, including reproducing, storing,  
 * adapting or translating, any or all of this material other than  
 * pursuant to the license terms referred to above requires the prior  
 * written consent of RealNetworks and its licensors 
 *  
 * This file, and the files included with this file, is distributed 
 * and made available by RealNetworks on an 'AS IS' basis, WITHOUT  
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS  
 * AND ITS LICENSORS HEREBY DISCLAIM  ALL SUCH WARRANTIES, INCLUDING  
 * WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS  
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 *  
 * Technology Compatibility Kit Test Suite(s) Location:  
 *    http://www.helixcommunity.org/content/tck 
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef _IHXDOWNLOADEVENTOBSERVER_H_
#define _IHXDOWNLOADEVENTOBSERVER_H_


/****************************************************************************
 *
 *  Interface:
 *
 *  IHXDownloadEventObserver
 *
 *  Purpose:
 *
 *  Provides an interface for informing an interested party of events 
 *  relating to a downloaded file.
 *
 *  IID_IHXDownloadEventObserver
 *
 *  {6D2F6C25-9BE1-4EE1-8DF5-AADFA755D040}
 */


DEFINE_GUID(IID_IHXDownloadEventObserver, 0x6d2f6c25, 0x9be1, 0x4ee1, 0x8d, 0xf5, 0xaa, 0xdf, 0xa7, 0x55, 0xd0, 0x40);

#undef  INTERFACE
#define INTERFACE   IHXDownloadEventObserver

#define DEFAULT_UNDERFLOW_TIMEOUT_IN_MILLIS   60000

DECLARE_INTERFACE_(IHXDownloadEventObserver, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)            (THIS_
                                          REFIID riid,
                                          void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)           (THIS) PURE;
    STDMETHOD_(ULONG32,Release)          (THIS) PURE;

    /*
     *  DownloadComplete Event
     */
    STDMETHOD_(UINT32,IsDownloadComplete) (THIS) PURE;
    STDMETHOD_(UINT32,GetDownloadSize)    (THIS) PURE;
    STDMETHOD_(UINT32,GetUnderflowTimeoutInMillis) (THIS) PURE;
    STDMETHOD(SetDownloadComplete)       (THIS_
                                          UINT32 downloadComplete) PURE;
    STDMETHOD(SetDownloadSize)           (THIS_
                                          UINT32 downloadSize) PURE;
    STDMETHOD(SetUnderflowTimeoutInMillis)(THIS_
                                          UINT32 timeout) PURE;
};

#endif // _IHXDOWNLOADEVENTOBSERVER_H_


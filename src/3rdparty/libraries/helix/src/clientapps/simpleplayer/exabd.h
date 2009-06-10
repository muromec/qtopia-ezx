/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#ifndef _EXAMPLEABD_
#define _EXAMPLEABD_

struct IHXAutoBWCalibrationAdviseSink;
struct IUnknown;

class ExampleABD : public IHXAutoBWCalibrationAdviseSink
{
  private:
    LONG32          m_lRefCount;    
    IUnknown*       m_pUnknown;    
    UINT32          m_ulABDStartTime;
    UINT32          m_ulABDResult;
    BOOL            m_bABDDone;
    
    ~ExampleABD();

  public:

    ExampleABD(IUnknown* /*IN*/ pUnknown);

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface) (THIS_
                               REFIID riid,
                               void** ppvObj);
    
    STDMETHOD_(ULONG32,AddRef) (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     *	IHXAutoBWCalibrationAdviseSink methods
     */
    STDMETHOD(AutoBWCalibrationStarted) (THIS_
                                         const char* pszServer);
    STDMETHOD(AutoBWCalibrationDone)	(THIS_
                                         HX_RESULT  status,
                                         UINT32     ulBW);

    HX_RESULT   DoABD(const char*   pszServer,
                      UINT32        mode,
                      UINT32        packetSize,
                      UINT32        packetNum);

    UINT32      GetABD(void) { return m_ulABDResult; };
};

#endif /* _EXAMPLEABDSNK_ */

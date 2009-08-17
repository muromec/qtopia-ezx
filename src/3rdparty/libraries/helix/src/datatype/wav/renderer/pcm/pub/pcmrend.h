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

#ifndef PCMREND_H
#define PCMREND_H

// Forward declarations
typedef _INTERFACE IHXValues IHXValues;
class CAudioFormat;


class CPCMAudioRenderer : public CAudioRenderer,
                          public CHXBaseCountingObject          
{
public:
    CPCMAudioRenderer(void);
    virtual ~CPCMAudioRenderer();

    // IHXPlugin methods
    STDMETHOD(GetPluginInfo) (THIS_ REF(HXBOOL)        bLoadMultiple,
                                    REF(const char*) pDescription,
                                    REF(const char*) pCopyright,
                                    REF(const char*) pMoreInfoURL,
                                    REF(ULONG32)     ulVersionNumber);

    // IHXRenderer methods
    STDMETHOD(GetRendererInfo) (THIS_ REF(const char**) pStreamMimeTypes,
                                      REF(UINT32)       unInitialGranularity);

    // CPCMAudioRenderer methods
    static HX_RESULT STDAPICALLTYPE HXCreateInstance(IUnknown** ppIUnknown);
    static HX_RESULT STDAPICALLTYPE CanUnload2();
protected:
    virtual const char*   GetRendererName(void);
    virtual CAudioFormat* CreateFormatObject(IHXValues* pHeader);

    static const char* const m_pszDescription;
    static const char* const m_pszRendererName;
    static const char* const m_ppszStreamMimeTypes[];
};

#endif /* #ifndef PCMREND_H */


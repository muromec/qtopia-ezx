/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0 and Exhibits. 
 * REALNETWORKS CONFIDENTIAL--NOT FOR DISTRIBUTION IN SOURCE CODE FORM 
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. 
 * All Rights Reserved. 
 * 
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Community Source 
 * License Version 1.0 (the "RCSL"), including Attachments A though H, 
 * all available at http://www.helixcommunity.org/content/rcsl. 
 * You may also obtain the license terms directly from RealNetworks. 
 * You may not use this file except in compliance with the RCSL and 
 * its Attachments. There are no redistribution rights for the source 
 * code of this file. Please see the applicable RCSL for the rights, 
 * obligations and limitations governing use of the contents of the file. 
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
 * https://rarvcode-tck.helixcommunity.org 
 * 
 * Contributor(s): 
 * 
 * ***** END LICENSE BLOCK ***** */ 


#ifndef __MP4VIDEO_H__
#define __MP4VIDEO_H__

/****************************************************************************
 *  Includes
 */
#include "vidrend.h"
#include "baseobj.h"
#include "hxalloc.h"

class CMP4VideoFormat;

/****************************************************************************
 *  CMP4VideoRenderer
 */
class CMP4VideoRenderer : public CVideoRenderer,
                          public CHXBaseCountingObject		
{
private:
    static const char* const zm_pDescription;
    static const char* const zm_pStreamMimeTypes[];

protected:
    virtual CVideoFormat* CreateFormatObject(IHXValues* pHeader);
    const char* CMP4VideoRenderer::GetRendererName(void);

    void SetupBitmapDefaults(IHXValues* pHeader,
			     HXBitmapInfoHeader &bitmapInfoHeader);

    CMP4VideoFormat*	m_pMP4VideoFormat;

public:
    static HX_RESULT STDAPICALLTYPE HXCreateInstance(IUnknown** ppIUnknown);
    static HX_RESULT STDAPICALLTYPE CanUnload(void);
    static HX_RESULT STDAPICALLTYPE CanUnload2(void);

    /*
     *	Constructor/Destructor
     */
    CMP4VideoRenderer(void);
    ~CMP4VideoRenderer();

    /*
     *	IHXPlugin methods
     */
    /************************************************************************
     *	Method:
     *	    IHXPlugin::GetPluginInfo
     *	Purpose:
     *	    Returns the basic information about this plugin. Including:
     *
     *	    bLoadMultiple	whether or not this plugin DLL can be loaded
     *				multiple times. All File Formats must set
     *				this value to TRUE.
     *	    pDescription	which is used in about UIs (can be NULL)
     *	    pCopyright		which is used in about UIs (can be NULL)
     *	    pMoreInfoURL	which is used in about UIs (can be NULL)
     */
    STDMETHOD(GetPluginInfo)	(THIS_
				REF(HXBOOL)	 /*OUT*/ bLoadMultiple,
				REF(const char*) /*OUT*/ pDescription,
				REF(const char*) /*OUT*/ pCopyright,
				REF(const char*) /*OUT*/ pMoreInfoURL,
				REF(ULONG32)	 /*OUT*/ ulVersionNumber
				);

    /*
     *	IHXRenderer methods
     */
    /************************************************************************
     *	Method:
     *	    IHXRenderer::GetRendererInfo
     *	Purpose:
     *	    Returns information vital to the instantiation of rendering 
     *	    plugins.
     */
    STDMETHOD(GetRendererInfo)	(THIS_
				REF(const char**) /*OUT*/ pStreamMimeTypes,
				REF(UINT32)       /*OUT*/ unInitialGranularity
				);

    virtual HX_RESULT UntimedModeNotice(HXBOOL bUntimedRendering);
    
    CHXMemoryAllocator* m_pOutputAllocator;
};

#endif //__MP4VIDEO_H__


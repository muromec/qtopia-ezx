/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: h263video.h,v 1.5 2007/07/06 22:00:35 jfinnecy Exp $
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

#ifndef __H263VIDEO_H__
#define __H263VIDEO_H__

/****************************************************************************
 *  Includes
 */
#include "vidrend.h"
#include "baseobj.h"


/****************************************************************************
 *  CH263VideoRenderer
 */
class CH263VideoRenderer : public CVideoRenderer,
			   public CHXBaseCountingObject		
{
private:
    static const char* const zm_pDescription;
    static const char* const zm_pStreamMimeTypes[];

protected:
    CVideoFormat* CreateFormatObject(IHXValues* pHeader);
    const char* GetRendererName(void);
    const char* GetCodecFourCC(void);

    void SetupBitmapDefaults(IHXValues* pHeader,
			     HXBitmapInfoHeader &bitmapInfoHeader);

public:
    static HX_RESULT STDAPICALLTYPE HXCreateInstance(IUnknown** ppIUnknown);
    static HX_RESULT STDAPICALLTYPE CanUnload(void);
    static HX_RESULT STDAPICALLTYPE CanUnload2(void);

    /*
     *	Constructor/Destructor
     */
    CH263VideoRenderer(void);
    ~CH263VideoRenderer();

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


};

#endif //__h263VIDEO_H__


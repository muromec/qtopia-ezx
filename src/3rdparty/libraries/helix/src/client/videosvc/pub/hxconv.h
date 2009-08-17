/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxconv.h,v 1.2 2004/07/09 18:45:32 hubbe Exp $
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

#ifndef _HXCONV_H_
#define _HXCONV_H_

#include "hxwintyp.h"
#include "dllacces.h"
#include "coloracc.h"
#include "hxwin.h"
#include "hxvsurf.h"

class HXColorConverterManager: public IHXColorConverterManager
{
protected:
    LONG32		    m_lRefCount;
    ColorFuncAccess*	    m_pColorAcc;
    IUnknown*		    m_pContext;
 
public:

    HXColorConverterManager(IUnknown* pContext);
    ~HXColorConverterManager(void);

    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

		
    /*
     *	IHXColorConverterManager methods
     */

    /************************************************************************
     *	Method:
     *	    IHXColorConverterManager::GetColorConverter
     *	Purpose:
     *	    Get ColorConverter is called to obtain a color converter to convert 
     *	    from a particular bitmap to another bitmap.
     *	    
     *
     */
    STDMETHOD(GetColorConverter)	(   THIS_ 
				    HXBitmapInfoHeader*  pInfoSrc,
				    HXBitmapInfoHeader*  pInfoDest,
				    REF(IHXColorConverter*) /*OUT*/ pConverter);

private:
    void Init();
};

class HXColorConverter: public IHXColorConverter
{
protected:
    LONG32		    m_lRefCount;
    int			    m_nPitchIn;
    int			    m_nPitchOut;
    HXxSize		    m_sizeIn;
    HXxSize		    m_sizeOut;
    LPHXCOLORCONVERTER	    m_fpColorConverter;
    
public:

    HXColorConverter();
    ~HXColorConverter(void);

    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

		
    /*
     *	IHXColorConverter methods
     */

    /************************************************************************
     *	Method:
     *	    IHXColorConverter::ColorConvert
     *	Purpose:
     *	    Get ColorConverter is called to obtain a color converter to convert 
     *	    from a particular bitmap to another bitmap.
     *	    
     *
     */
    STDMETHOD(ColorConvert)	(   THIS_ 
				    UCHAR*	pBitsIn,
				    UCHAR*	pBitsOut,
				    HXxRect*    pRectIn,
				    HXxRect*    pRectOut);

    void SetColorConverter(LPHXCOLORCONVERTER fpColorConverter) {m_fpColorConverter = fpColorConverter;}
    void SetPitchIn(int pitch) {m_nPitchIn = pitch;}
    void SetPitchOut(int pitch) {m_nPitchOut = pitch;}
    void SetSizeIn(int height, int width) {m_sizeIn.cx = width; m_sizeIn.cy = height;}
    void SetSizeOut(int height, int width) {m_sizeOut.cx = width; m_sizeOut.cy = height;}
};


#endif // _HXCONV_H_

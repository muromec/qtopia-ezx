/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxcfg.h,v 1.3 2007/07/06 20:43:41 jfinnecy Exp $
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

#ifndef _HXCFG_H_
#define _HXCFG_H_

typedef _INTERFACE  IHXBuffer			IHXBuffer;

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXConfigFile
 *
 *  Purpose:
 *
 *  IID_IHXConfigFile:
 *
 *	{00001c00-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXConfigFile, 0x00001c00, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
				0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef INTERFACE
#define INTERFACE IHXConfigFile

DECLARE_INTERFACE_(IHXConfigFile, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_
					 REFIID riid,
					 void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    /*
     *	IHXConfigFile methods
     */
    /************************************************************************
     *	Method:
     *	    IHXConfigFile::LoadFrom
     *	Purpose:
     *
     *	    LoadFrom tells the server to load the config file specified,
     *	    and sets that file as the default for future Reloads and Saves
     */
    STDMETHOD(LoadFrom)                 (THIS_
					 IHXBuffer* filename) PURE;

    /************************************************************************
     *	Method:
     *	    IHXConfigFile::Reload
     *	Purpose:
     *
     *	    Reload causes the current default config file to be reloaded.
     */
    STDMETHOD(Reload)                   (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXConfigFile::Save
     *	Purpose:
     *
     *	    Save causes the current configuration to be written to the
     *	    current default file.
     */
    STDMETHOD(Save)                     (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXConfigFile::SaveAs
     *	Purpose:
     *
     *	    SaveAs writes the configuration to the named file, and sets that
     *	    file as the default.
     */
    STDMETHOD(SaveAs)                   (THIS_
					 IHXBuffer* pFilename) PURE;

    /************************************************************************
     *	Method:
     *	    IHXConfigFile::GetFilename
     *	Purpose:
     *
     *	    GetFilename returns the current default file
     */
    STDMETHOD(GetFilename)              (THIS_
					 REF(IHXBuffer*) pFilename) PURE;

    /************************************************************************
     *	Method:
     *	    IHXConfigFile::SetFilename
     *	Purpose:
     *
     *	    SetFilename sets the current default file, but does not read it
     *	    or change its contents.
     */
    STDMETHOD(SetFilename)              (THIS_
					 IHXBuffer* pFilename) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXRegConfig
 *
 *  Purpose:
 *
 *  IID_IHXRegConfig:
 *
 *	{00001c01-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXRegConfig, 0x00001c01, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
				0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef INTERFACE
#define INTERFACE IHXRegConfig

DECLARE_INTERFACE_(IHXRegConfig, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_
					 REFIID riid,
					 void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRegConfig::WriteKey
     *	Purpose:
     *
     *	    Write out the registry from the passed in keyname to the 
     *  currently active permanent config storage area (ex. config file,
     *  registry).
     */
    STDMETHOD(WriteKey)              (THIS_
					const char* pKeyName) PURE;

};

#endif /* _HXCFG_H_ */

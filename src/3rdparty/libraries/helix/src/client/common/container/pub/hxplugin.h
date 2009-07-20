/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxplugin.h,v 1.5 2006/04/12 20:54:38 ehyche Exp $
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

#ifndef HXPLUGIN_H__
#define HXPLUGIN_H__

#include "hxstring.h"
#include "hxtypes.h"
#include "hxcom.h"
#include "unkimp.h"


// Plugin Types.
#define	PLUGIN_FILESYSTEM_TYPE	    "PLUGIN_FILE_SYSTEM"
#define	PLUGIN_FILEFORMAT_TYPE	    "PLUGIN_FILE_FORMAT"
#define	PLUGIN_FILEWRITER_TYPE	    "PLUGIN_FILE_WRITER"
#define	PLUGIN_METAFILEFORMAT_TYPE  "PLUGIN_METAFILE_FORMAT"
#define	PLUGIN_RENDERER_TYPE	    "PLUGIN_RENDERER"
#define PLUGIN_DEPACKER_TYPE        "PLUGIN_DEPACKER"
#define PLUGIN_STREAM_DESC_TYPE	    "PLUGIN_STREAM_DESC"
#define PLUGIN_CLASS_FACTORY_TYPE   "PLUGIN_CLASS_FACT"
#define PLUGIN_PAC_TYPE		    "PLUGIN_PAC"

#define	PLUGIN_CLASS		    "PluginType"
#define PLUGIN_FILENAME		    "PluginFilename"
#define PLUGIN_REGKEY_ROOT	    "PluginHandlerData"
#define PLUGIN_PLUGININFO	    "PluginInfo"
#define PLUGIN_GUIDINFO		    "GUIDInfo"
#define PLUGIN_NONHXINFO	    "NonHXDLLs"
#define PLUGIN_DESCRIPTION2	    "Description"
#define PLUGIN_FILE_HASH	    "FileHash"
#define PLUGIN_INDEX		    "IndexNumber"
#define PLUGIN_FILENAMES	    "FileInfo"
#define PLUGIN_COPYRIGHT2	    "Copyright"
#define PLUGIN_LOADMULTIPLE	    "LoadMultiple"
#define PLUGIN_VERSION		    "Version"
#define PLUGIN_FILESYSTEMSHORT	    "FileShort"
#define PLUGIN_FILESYSTEMPROTOCOL   "FileProtocol"
#define PLUGIN_FILEMIMETYPES	    "FileMime"
#define PLUGIN_FILEEXTENSIONS	    "FileExtensions"
#define PLUGIN_FILEOPENNAMES	    "FileOpenNames"
#define PLUGIN_RENDERER_MIME	    "RendererMime"
#define PLUGIN_RENDERER_GRANULARITY "Renderer_Granularity"
#define PLUGIN_DEPACKER_MIME        "DepackerMime"
#define PLUGIN_STREAMDESCRIPTION    "StreamDescription"

#define PLUGIN_NUM_PLUGINS	    "NumPlugins"
#define PLUGIN_FILE_CHECKSUM	    "DLLCheckSum"
#define PLUGIN_DLL_SIZE		    "DLLSize"
#define PLUGIN_HAS_FACTORY	    "DLLHasFactory"

class HXPluginArchiveReader;
class HXPluginArchiveWriter;
class HXPluginDLL;

_INTERFACE IHXValues;
_INTERFACE IHXPlugin;

class HXPlugin 
: public CUnknownIMP
{
public:
    DECLARE_UNKNOWN_NOCREATE(HXPlugin)
public:

    HXPlugin(IUnknown* pContext);
    HXPlugin(IUnknown* pContext, HXPluginArchiveReader& ar);

    void Archive(HXPluginArchiveWriter& ar);

    ~HXPlugin();

    HX_RESULT Init(HXPluginDLL* pDll, UINT16 idxPlugin);
 
    HXBOOL DoesMatch(IHXValues* pValues);
    HX_RESULT GetValuesFromDLL(IHXPlugin* pHXPlugin);
    HX_RESULT GetPlugin(IUnknown*& pUnknown );
    HX_RESULT GetInstance(IUnknown*& pUnknown, IUnknown* pIUnkOuter = NULL );
    HX_RESULT GetPluginInfo(IHXValues*&);
    UINT16 GetIndex();

    HX_RESULT AddComponentInfo(IHXValues* pVal);


private:
    UINT16		    m_idxPlugin;
    HXPluginDLL*	    m_pDll;
    IHXValues*		    m_pValues;
    bool                    m_bCanUnload;
    IUnknown*		    m_pContext;
    IHXCommonClassFactory*  m_pClassFactory;

private:
    void SetPluginProperty(const char * pszPluginType);
    HX_RESULT   GetBasicValues(IHXPlugin* pHXPlugin);
    HX_RESULT	GetExtendedValues(IHXPlugin* pHXPlugin);
    HXBOOL        AreBufferEqual(IHXBuffer* pBigBuff, IHXBuffer* pSmallBuff);
};


inline
UINT16 HXPlugin::GetIndex()
{
    return m_idxPlugin;
}

#endif /* HXPLUGIN_H__ */

/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: minifilesys.h,v 1.6 2007/07/06 20:48:14 jfinnecy Exp $
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
#ifndef _MINIFILESYS_H_
#define _MINIFILESYS_H_


/****************************************************************************
 * Includes
 */
#include "hlxclib/stdio.h" /* FILE */
#include "hlxclib/io.h" /* struct _finddata_t, etc. */
#include "hxplugn.h"  /* IHXPlugin */
#include "hxfiles.h"  /* IHXFileSystemObject */
#include "baseobj.h"  /* IHXFileSystemObject */


/****************************************************************************
 * Constants
 */
#define FILE_SYS_PROTOCOL "file"

/****************************************************************************
 *
 *  CHXMiniFileSystem Class
 *
 *  This class inherits the interfaces required to create a File System
 *  plug-in. The IHXFileSystemObject interface contains methods for
 *  initializing the File System object, and creating the File Object which
 *  handles the actual low level file access. All plug-ins also require the
 *  IHXPlugin interface in order to handle fundamental plug-in operations.
 *  Since this is also a COM object, this class also inherits COM's IUnknown
 *  interface to handle reference counting and interface query.
 */
class CHXMiniFileSystem :  public IHXFileSystemObject,
			   public IHXPlugin,
			   public CHXBaseCountingObject
{
public:
    CHXMiniFileSystem(void);
    ~CHXMiniFileSystem(void);

    /************************************************************************
     *  IHXFileSystemObject Interface Methods
     */
    STDMETHOD(GetFileSystemInfo)(THIS_ REF(const char*) pShortName,
				 REF(const char*) pProtocol);
    STDMETHOD(InitFileSystem) (THIS_ IHXValues* pOptions);
    STDMETHOD(CreateFile    ) (THIS_ IUnknown** ppFileObject);
    STDMETHOD(CreateDir     ) (THIS_ IUnknown**	ppDirObject);


    /************************************************************************
     *  IHXPlugin Interface Methods
     */
    STDMETHOD(GetPluginInfo)(THIS_ REF(HXBOOL)  bLoadMultiple,
			     REF(const char*) pDescription,
			     REF(const char*) pCopyright,
			     REF(const char*) pMoreInfoURL,
			     REF(UINT32)      versionNumber);
    STDMETHOD(InitPlugin) (THIS_ IUnknown* pContext);


    /************************************************************************
     *  IUnknown COM Interface Methods
     */
    STDMETHOD (QueryInterface ) (THIS_ REFIID ID, void** ppInterfaceObj);
    STDMETHOD_(UINT32, AddRef ) (THIS);
    STDMETHOD_(UINT32, Release) (THIS);


private:
    /****** Private Class Variables ****************************************/
    INT32                    m_RefCount;       // Object's reference count
    IHXCommonClassFactory*   m_pClassFactory;  // Creates common Helix classes
    char*                    m_pBasePath;      // Platform's root path
	IUnknown*                m_pContext;
    
    /****** Private Static Class Variables *********************************/
    static const char* const zm_pDescription;
    static const char* const zm_pCopyright;
    static const char* const zm_pMoreInfoURL;
    static const char* const zm_pShortName;
    static const char* const zm_pProtocol;
};

#endif  /* _MINIFILESYS_H_ */

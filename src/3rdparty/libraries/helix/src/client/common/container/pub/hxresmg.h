/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxresmg.h,v 1.6 2007/07/06 21:57:57 jfinnecy Exp $
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

#ifndef _HXRESMG_H_
#define _HXRESMG_H_

struct IHXXResFile;

class HXExternalResourceManager: public IHXExternalResourceManager
{
private:
    LONG32		    m_lRefCount;
    CHXSimpleList*	    m_pResourceList;
    IUnknown*		    m_pContext;
    IUnknown*		    m_pHXXResPlugin;
    IHXPreferences*	    m_pPrefs;
    IHXRegistry*	    m_pRegistry;
    UINT32		    m_ulLanguageID;
    char*		    m_pExternalResDir;

#if !defined(HELIX_CONFIG_NOSTATICS)
    static HXExternalResourceManager*	
			    m_pResourceManager; // single instance
#else
    static const HXExternalResourceManager* const
			    m_pResourceManager; // single instance
#endif
    
	    HXExternalResourceManager	    (IUnknown* pContext);
    virtual ~HXExternalResourceManager	    ();

    HX_RESULT GetHXXResPlugin		    ();
    HX_RESULT LoadResourceFiles		    ();
    HX_RESULT LoadResourceFile		    (const char* pPath);
    IHXBuffer*	ChecksumFile		    (const char* pPath);
    IHXBuffer*	ConvertToAsciiString	    (char* pBuffer, 
					    UINT32 nBuffLen);
    HX_RESULT	Stat			    (const char* pPath, 
					    struct stat* pStatBuffer);
    HXBOOL	FileInfoCurrent		    (const char* pFileName,
					    const char* pPath);
    HXBOOL	ContainsCurrentLanguage	    (const char* pFileName,
					    const char* pPath);
    HXBOOL	ContainsCurrentLanguage	    (IHXXResFile* pResFile);
    HX_RESULT	SaveFileInfo		    (const char* pFileName,
					    const char* pPath);


public:
    static HXExternalResourceManager*	// single instance
			    Instance(IUnknown* pContext);
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)      (THIS);

    STDMETHOD_(ULONG32,Release)     (THIS);


    /*
     * IHXExternalResouceManager methods
     */
    /************************************************************************
    *  Method:
    *      IHXExternalResourceManager::Init
    *  Purpose:
    *		Reads current language preference and
    *		initializes resource cache
    *		
    */
    STDMETHOD(Init)		    (THIS);

    /************************************************************************
    *  Method:
    *      IHXExternalResourceManager::CreateExternalResourceReader
    *  Purpose:
    *		Create an instance of a resource reader for the
    *		"short name" resources in the system
    */
    STDMETHOD(CreateExternalResourceReader) (THIS_  
				    const char* pShortName,
				    REF(IHXExternalResourceReader*) pReader);

    /*
     * HXExternalResourceManager methods
     */
    IHXXResFile*	    MakeResFileObject(const char* pPath);
};

class HXExternalResourceReader: public IHXExternalResourceReader
{
private:
    LONG32			m_lRefCount;
    CHXSimpleList*		m_pResourceList;
    HXExternalResourceManager*	m_pResManager;
    IHXXResFile*		m_pDefaultRes;
    
    virtual			~HXExternalResourceReader();

public:
				HXExternalResourceReader(
				    HXExternalResourceManager* pMgr);
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)      (THIS);

    STDMETHOD_(ULONG32,Release)     (THIS);


    /*
     *  IHXExternalResourceReader methods
     */
    /************************************************************************
    *  Method:
    *      IHXExternalResourceReader::SetDefaultResourceFile
    *  Purpose:
    *		Resource file used if XRS files do not contain
    *		requested resource.
    *		
    */
    STDMETHOD(SetDefaultResourceFile)
				    (THIS_
				    const char* pResourcePath);

    /************************************************************************
    *  Method:
    *      IHXExternalResourceReader::GetResource
    *  Purpose:
    *		Create an instance of a resource reader for the
    *		"short name" resources in the system
    */
    STDMETHOD_(IHXXResource*, GetResource)   
				    (THIS_
				    UINT32 ulResourceType,
				    UINT32 ulResourceID);


    /*
     * HXExternalResourceReader methods
     */

    HX_RESULT Init		    (const char* pShortName,
				    CHXSimpleList* pResList);

    IHXXResource* GetResource	    (IHXXResFile* pResFile,
				    UINT32 ulResourceType,
				    UINT32 ulResourceID);
};

#endif	/* _HXRESMG_H_ */

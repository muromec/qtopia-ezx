/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: testplgnhdlr.h,v 1.2 2007/07/06 20:51:33 jfinnecy Exp $
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

#ifndef TESTPLGNHDLR_H
#define TESTPLGNHDLR_H

#include "hxcom.h"
#include "hxplugn.h"
#include "hxslist.h"

class CHXTestPluginHdlr : public IHXPlugin2Handler
{
public:
    CHXTestPluginHdlr();
    ~CHXTestPluginHdlr();

    HX_RESULT SetMimetypes(const char* pMimetypes);

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	IHXPlugin2Handler Methods
     */

     /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::Init
     *
     *	Purpose:    
     *	    Specifies the context and sets the pluginhandler in motion.
     *
     */
    STDMETHOD(Init)    (THIS_ IUnknown* pContext);
     
     /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::GetNumPlugins2
     *
     *	Purpose:    
     *	    Gets the info of a particular plugin.
     *
     */
    STDMETHOD_(ULONG32,GetNumOfPlugins2)    (THIS);
    
    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::GetPluginInfo
     *
     *	Purpose:    
     *	    Gets the info of a particular plugin.
     *
     */
    STDMETHOD(GetPluginInfo)	(THIS_ 
				UINT32 unIndex, 
				REF(IHXValues*) /*OUT*/ Values);

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FlushCache()
     *
     *	Purpose:    
     *	    Flushes the LRU cache -- Unloads all DLLs from memory 
     *	    which currenltly have a refcount of 0.
     */

    STDMETHOD(FlushCache)	(THIS);

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::SetCacheSize
     *
     *	Purpose:    
     *	    This function sets the size of the Cache. The cache is 
     *	    initally set to 1000KB. To disable the cache simply set
     *	    the size to 0.If the cache is disabled a DLL will be 
     *	    unloaded whenever it's refcount becomes zero. Which MAY
     *	    cause performance problems.
     */

    STDMETHOD(SetCacheSize)	(THIS_ ULONG32 nSizeKB);

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::GetInstance
     *
     *	Purpose:    
     *	    
     *	    This function will return a plugin instance given a plugin index.
     *		
     */

    STDMETHOD(GetInstance) (THIS_ UINT32 index, REF(IUnknown*) pUnknown); 

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FindIndexUsingValues
     *
     *	Purpose:    
     *	    Finds a plugin  which matches the set of values given. An index
     *	    is returned which can be used to either get the values (using 
     *	    GetPluginInfo) or an instance can be created using GetPluing(). 	
     * 
     */

    STDMETHOD(FindIndexUsingValues)	    (THIS_ IHXValues*, 
						    REF(UINT32) unIndex);

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FindPluginUsingValues
     *
     *	Purpose:    
     *	    Finds a plugin  which matches the set of values given. A Plugin
     *	    instance is returned. 
     *	    
     */

    STDMETHOD(FindPluginUsingValues)	    (THIS_ IHXValues*, 
						    REF(IUnknown*) pUnk);
    
    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FindIndexUsingStrings
     *
     *	Purpose:    
     *	    Finds a plugin  which matches the set of values given. An index
     *	    is returned which can be used to either get the values (using 
     *	    GetPluginInfo) or an instance can be created using GetPluing(). 	
     *	    NOTE: that a max of two values may be given.
     */

    STDMETHOD(FindIndexUsingStrings)	    (THIS_ char* PropName1, 
						    char* PropVal1, 
						    char* PropName2, 
						    char* PropVal2, 
						    char* PropName3, 
						    char* PropVal3, 
						    REF(UINT32) unIndex);

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FindPluginUsingStrings
     *
     *	Purpose:    
     *	    Finds a plugin  which matches the set of values given. A Plugin
     *	    instance is returned. 
     *	    NOTE: that a max of two values may be given.
     */

    STDMETHOD(FindPluginUsingStrings)	    (THIS_ char* PropName1, 
						    char* PropVal1, 
						    char* PropName2, 
						    char* PropVal2, 
						    char* PropName3, 
						    char* PropVal3, 
						    REF(IUnknown*) pUnk);

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FindImplementationFromClassID
     *
     *	Purpose:    
     *	    Finds a CommonClassFactory plugin which supports the 
     *	    ClassID given. An instance of the Class is returned. 
     */

    STDMETHOD(FindImplementationFromClassID)
    (
	THIS_ 
	REFGUID GUIDClassID, 
	REF(IUnknown*) pIUnknownInstance
    );

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::Close
     *
     *	Purpose:    
     *	    A function which performs all of the functions of delete.
     *	    
     *
     */
    
    STDMETHOD(Close)		(THIS); 

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::SetRequiredPlugins
     *
     *	Purpose:    
     *	    This function sets the required plugin list
     *	    
     *
     */

    STDMETHOD(SetRequiredPlugins) (THIS_ const char** ppszRequiredPlugins);

private:
    void clearList();
    HX_RESULT addString(const char* pStr, UINT32 uLength);

    ULONG32 m_lRefCount;
    CHXSimpleList m_list;
};
#endif /* TESTPLGNHDLR_H */

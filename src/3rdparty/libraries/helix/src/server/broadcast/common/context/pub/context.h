/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: context.h,v 1.6 2009/02/09 21:23:26 dcollins Exp $
 *
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.
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
#ifndef _Context_H__
#define _Context_H__

typedef _INTERFACE  IHXSapManager       IHXSapManager;

class RBSCommonClassFactory;
class HXScheduler;
class HXPreferences;
class HXNetworkServices;
class CSapClass;
class MulticastAddressPool;
class PluginHandler;
class ConfigRegistry;
class CErrorcontroller;
class CRemoteNetServicesContext;

class Context : public IUnknown
{
public:
    Context(void);
    virtual ~Context(void);

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32, AddRef) (THIS);
    STDMETHOD_(ULONG32, Release)(THIS);

    STDMETHOD(Close) (THIS);

    HX_RESULT Process();
    static void DestroyGlobals(void);

private:
    LONG32                  m_lRefCount;
    CallbackContainer       callbacks;

    HXScheduler*                m_pScheduler;
    RBSCommonClassFactory*      m_pCommonClassFactory;
    PluginHandler*              m_plugin_handler;
    IHXSapManager*              m_pSapManager;
    MulticastAddressPool*       m_pMulticastAddressPool;
    HXPreferences*              m_pPrefs;
    ConfigRegistry*             m_pRegistry;
    CErrorcontroller*           m_pErrorController;
    IHXErrorMessages*           m_pErrorMessages;
    CRemoteNetServicesContext*  m_pNetServices;
};

class DestructContext
{
public:
    DestructContext() {};
    ~DestructContext() {Context::DestroyGlobals();}
};

#endif

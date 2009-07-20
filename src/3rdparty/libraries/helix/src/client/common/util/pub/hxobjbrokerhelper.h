/* ***** BEGIN LICENSE BLOCK *****
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

#if defined(HELIX_CONFIG_NIMBUS)
#include "hxobjbrokerhelper2.h"
#else
#ifndef _HXOBJBROKERHELPER_H_
#define _HXOBJBROKERHELPER_H_

#include "hxcom.h"
#include "dllacces.h"
#include "hxsingleton.h"
#include "ihxcontext.h"

_INTERFACE IHXCommonClassFactory;

class CHXObjBrokerWrapper_
{
public:
    HX_RESULT InitObjectBroker (IUnknown** ppIContext);
    HX_RESULT TerminateObjectBroker ();

    HX_RESULT GetRootFactory (IHXCommonClassFactory** ppIContext);

    // SetRootContext allows use of Gemini functions without the
    // object broker being available
    HX_RESULT SetRootContext (IUnknown* pIContext);
		
    // XXXHP - this is far less than ideal. the problem is that the
    // Release/Acquire methods should have a different meaning from within the context 
    // of the object broker DLL itself. If we attempt to load/unload when the static object
    // gets destroyed in the object broker DLL then the object broker will never get unloaded.
    // As a quick solution I've gone ahead and made ReleaseObjectBroker_ public but left
    // it underscored to indicate that it should not be used and it's exposition is 
    // an artifact of a convoluted design (the design of the DLL loading, object broker access, etc).
    HX_RESULT ReleaseObjectBroker_ ();

private:
    HX_RESULT CreateRootContext_ ();
    HX_RESULT AcquireObjectBroker_ ();
    
    DLLAccess* m_pLibObjBroker;
    IUnknown* m_pRootContext;
    SPIHXContext m_spGenericContext;
    UINT32    m_ulRefCount;


    CHXObjBrokerWrapper_ ();
    ~CHXObjBrokerWrapper_ (); 

    HX_CREATE_AS_SINGLETON (HXObjBrokerHelper, CHXObjBrokerWrapper_);
};

HX_DECLARE_SINGLETON (HXObjBrokerHelper, CHXObjBrokerWrapper_);

#endif
#endif /* HELIX_CONFIG_NIMBUS */
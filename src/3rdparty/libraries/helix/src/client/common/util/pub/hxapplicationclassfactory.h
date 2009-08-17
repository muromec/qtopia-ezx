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

#ifndef _HXAPPLICATIONCLASSFACTORY_H_
#define _HXAPPLICATIONCLASSFACTORY_H_

/*
/////////////////////////////////////////////////////////////////////////////
//
//  Function:
//
//      SetApplication()
//
//  Purpose:
//
//	Allows an application to provide a class factory capable of
//	creating simple COM objects such as IHXBuffer, IHXString, ...
//
//
*/

#include "hxresult.h"

_INTERFACE IHXCommonClassFactory;

namespace HXApplicationClassFactory
{
    // Pass this method a class factory pointer capable of creating simple COM types
    // (objects that can be created by the class factory without loading any other DLLs)
    // The method only needs to be called once when the application starts- the class
    // factory will automatically be propagated to all DLLs loaded by the app.
    // It is up to the app to ensure that the class factory is available for the life of the app.
    // See TerminateApplication for shutdown requirements
    HX_RESULT Init(IHXCommonClassFactory *pIHXCommonClassFactory);


    // Upon app shutdown, TerminateApplication should be called before the
    // classfactory DLL (if implemented in a DLL) is unloaded. Failing to perform this
    // step will cause a crash when the ref-counted Class Factory object tries to call
    // its destructor.
    HX_RESULT Terminate();

    // Retrieves a ref-counted copy of the class factory specified above.
    HX_RESULT Get(IHXCommonClassFactory **ppIHXCommonClassFactory);

}; //namespace HXApplicationClassFactory

#endif // _HXAPPLICATIONCLASSFACTORY_H_

//Leave a CR/LF before EOF to prevent CVS from getting angry


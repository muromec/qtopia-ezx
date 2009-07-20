/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: dllaccesserver.h,v 1.4 2005/03/14 19:35:27 bobclark Exp $
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

#ifndef _DLLACCESSERVER_H_
#define _DLLACCESSERVER_H_

/************************************************************************
 *  Includes
 */
#include "hxcom.h"
#include "hxdllaccess.h"
#include "dllacces.h"

class DLLAccessServer : public IHXDllAccess
{
public:
    /*
     *	Constructor/Destructor
     */
    DLLAccessServer();

    ~DLLAccessServer();

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	IHXDllAccess methods
     */
    /************************************************************************
     * Method:
     *	    IHXDllAccess::Open()
     * Purpose:
     *	    Open a Dll of the specified name and type.  
     */
    STDMETHOD(Open)		(THIS_
				const char* pDllName,
				UINT16 uLibType
				);

    /************************************************************************
     * Method:
     *	    IHXDllAccess::Close()
     * Purpose:
     *	    Close opened Dll.
     */
    STDMETHOD(Close)		(THIS);

    /************************************************************************
     * Method:
     *	    IHXDllAccess::GetSymbol()
     * Purpose:
     *	    Obtain pointer to symbol in opened Dll.
     */
    STDMETHOD_(void*, GetSymbol)	(THIS_
					const char* pSymbolName
					);

    /************************************************************************
     * Method:
     *	    IHXDllAccess::GetError()
     * Purpose:
     *	    Get service specific error code (0 = no error).
     */
    STDMETHOD_(ULONG32,GetError)	(THIS);

    /************************************************************************
     * Method:
     *	    IHXDllAccess::GetErrorString()
     * Purpose:
     *	    Get last error string.
     */
    STDMETHOD_(const char*, GetErrorString)	(THIS);

    /************************************************************************
     * Method:
     *	    IHXDllAccess::GetDllName()
     * Purpose:
     *	    Get opened Dll name
     */
    STDMETHOD_(const char*, GetDllName)		(THIS);

    /************************************************************************
     * Method:
     *	    IHXDllAccess::GetDllVersion()
     * Purpose:
     *	    Get opened Dll vesion
     */
    STDMETHOD_(const char*, GetDllVersion)	(THIS);
    
    /************************************************************************
     * Method:
     *	    IHXDllAccess::IsOpen()
     * Purpose:
     *	    Tells if Dll is opened:
     *		TRUE  = dll is opened
     *		FALSE = dll is not opened	
     */
    STDMETHOD_(HXBOOL,IsOpen)	(THIS);
    
private:
    INT32 m_lRefCount;

    DLLAccess mDllAccess;
};

#endif	// _DLLACCESSERVER_H_

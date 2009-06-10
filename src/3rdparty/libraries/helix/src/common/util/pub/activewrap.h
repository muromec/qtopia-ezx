/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: activewrap.h,v 1.5 2007/07/06 20:39:23 jfinnecy Exp $
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

#ifndef _ACTIVEWRAP_H_
#define _ACTIVEWRAP_H_

DECLARE_INTERFACE(CActivePropWrapperUser)
{
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	Active prop change callbacks
     */
    STDMETHOD(PropUpdated) (THIS_ REF(CHXString) strName, 
    							  REF(UINT32) strVal, 
    							  REF(CHXString) strErr) PURE;
    							  
    STDMETHOD(PropUpdated) (THIS_ REF(CHXString) strName, 
    							  REF(CHXString) ulVal, 
    							  REF(CHXString) strErr) PURE;
    							  
    STDMETHOD(PropDeleted) (THIS_ REF(CHXString) strName, 
    							  REF(CHXString) strErr) PURE;
};

/****************************************************************************
 * Forward declarations
 */
struct IUnknown;

/****************************************************************************
 *
 *  CActiveBase Class
 *
 *  An application level bitsaving proxy
 */
class CActivePropWrapper : public IHXActivePropUser
{


public:
    /****** Public Class Methods ******************************************/
    CActivePropWrapper();
    ~CActivePropWrapper();
    
    HX_RESULT Init(IUnknown* /*IN*/ pContext, CActivePropWrapperUser* pUser);
    
    HX_RESULT SetAsActive(const char* pPropName);
    HX_RESULT Done();

    /************************************************************************
     *  IUnknown COM Interface Methods                          ref:  hxcom.h
     */
    STDMETHOD (QueryInterface) (THIS_ REFIID ID, void** ppInterfaceObj);
    STDMETHOD_(UINT32, AddRef) (THIS);
    STDMETHOD_(UINT32, Release) (THIS);

    /************************************************************************
     *  IHXActivePropUser COM Interface Methods                 ref:hxmon.h
     */
    /************************************************************************
    * IHXActivePropUser::SetActiveInt
    *
    *    Async request to set int pName to ul.
    */
    STDMETHOD(SetActiveInt) (THIS_
                            const char* pName,
                            UINT32 ul,
                            IHXActivePropUserResponse* pResponse);

    /************************************************************************
    * IHXActivePropUser::SetActiveStr
    *
    *    Async request to set string pName to string in pBuffer.
    */
    STDMETHOD(SetActiveStr) (THIS_
                            const char* pName,
                            IHXBuffer* pBuffer,
                            IHXActivePropUserResponse* pResponse);

    /************************************************************************
    * IHXActivePropUser::SetActiveBuf
    *
    *    Async request to set buffer pName to buffer in pBuffer.
    */
    STDMETHOD(SetActiveBuf)     (THIS_
                                const char* pName,
                                IHXBuffer* pBuffer,
                                IHXActivePropUserResponse* pResponse);

    /************************************************************************
    * IHXActivePropUser::DeleteActiveProp
    *
    *   Async request to delete the active property.
    */
    STDMETHOD(DeleteActiveProp) (THIS_
                                const char* pName,
                                IHXActivePropUserResponse* pResponse);
                                
private:
	
    /****** Private Class Variables ****************************************/
    INT32                       m_lRefCount;
    IHXCommonClassFactory*	m_pClassFactory;
    IUnknown*			m_pContext;
    CActivePropWrapperUser*	m_pUser;

    /****** Private Functions **********************************************/
    IHXBuffer* MakeBufString(const char* pText);
};

#endif // _ACTIVEWRAP_H_

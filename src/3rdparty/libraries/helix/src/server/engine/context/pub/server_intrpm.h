/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: server_intrpm.h,v 1.2 2003/01/23 23:42:53 damonlan Exp $ 
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

class PluginMessageCallback : public SimpleCallback
{
public:
    MEM_CACHE_MEM

    PluginMessageCallback(IHXCallback*);
    virtual ~PluginMessageCallback();
    void func(Process* proc);

public:
    IHXCallback* m_pCallback;

};

class PluginMessageCallbackUserDeleted : public PluginMessageCallback
{
public:
    PluginMessageCallbackUserDeleted(IHXCallback* pCB) :
        PluginMessageCallback(pCB) {}
    void func(Process* proc);
};


class ServerInterPluginMessenger : public IHXInterPluginMessenger2
{
public:
    ServerInterPluginMessenger(Process*, MemCache*);
    ~ServerInterPluginMessenger();

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	IHXInterPluginMessenger methods
     */

    /************************************************************************
     *	Method:
     *	    IHXInterPluginMessenger::GetPluginContextID
     *	Purpose:
     *	    Get a HXPluginContextID for the current server process/thread
     *	for use in the call to IHXInterPluginMessenger::PostCallback.
     */
    STDMETHOD_(HXPluginContextID, GetPluginContextID) (THIS);

    /************************************************************************
     *	Method:
     *	    IHXInterPluginMessenger::PostCallback
     *	Purpose:
     *	    Request a callback to be fired in the server space coresponding
     *	to the HXPluginContextID.
     */
    STDMETHOD(PostCallback) (THIS_ HXPluginContextID,
                            IHXCallback*);    

    /************************************************************************
     *	Method:
     *	    IHXInterPluginMessenger::PostCallbackWithHandle
     *	Purpose:
     *	    Request a callback to be fired in the server space coresponding
     *	to the HXPluginContextID. If pHandle is NULL a new Handle will be
     *  created and it is up to the caller to delete it.  Note that in
     *  subsequent calls to PostCallback a Handle created by a previous call
     *  can be reused for added efficiency.
     */
    STDMETHOD(PostCallbackWithHandle) (THIS_
                                       HXPluginContextID,
                                       IHXCallback* pCallback,
                                       void** pHandle);

    STDMETHOD(FreeHandle) (THIS_ void** pHandle);
                           
private:

    INT32 m_lRefCount;
    Process*	m_pProc;
    MemCache*   m_pMalloc;
};

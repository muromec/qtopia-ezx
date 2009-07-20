/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: casynthr.h,v 1.7 2007/07/06 20:44:00 jfinnecy Exp $
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

#if !defined( __CASYNNETTHREAD_H )
#define	__CASYNNETTHREAD_H 

class CHXMapPtrToPtr;
class ThreadedConn;
class HXMutex;

class CAsyncNetThread 
{
public:
    static CAsyncNetThread* GetCAsyncNetThreadNotifier(IUnknown* pContext, HINSTANCE hInst, 
						       HXBOOL Create = FALSE);

    HX_RESULT	AttachSocket(ThreadedConn* pSocket);

    HX_RESULT	DetachSocket(ThreadedConn* pSocket);

    HWND GetWindowHandle(void) {return m_hWnd;};

protected:

    static ULONG32  GetSession();

    CAsyncNetThread(IUnknown* pContext, HINSTANCE hInst);

    HXBOOL	    Create();

    ~CAsyncNetThread();

    HXBOOL IsValid()   {return m_bValid;};

    void DecrementSocketsClientCount()
    {
	if (m_nNumSocketsClients > 0)
	{
	    m_nNumSocketsClients--;
	}
    }

    void IncrementSocketsClientCount()
    {
	m_nNumSocketsClients++;
    }

    //	WndProc for our notifier	
    static LRESULT CALLBACK AsyncNotifyProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    LRESULT		OnAsyncDNS(WPARAM wParam, LPARAM lParam);
    LRESULT		OnAsyncConnect(WPARAM wParam, LPARAM lParam);
    LRESULT		OnAsyncRead(WPARAM wParam, LPARAM lParam);
    LRESULT		OnAsyncWrite(WPARAM wParam, LPARAM lParam);
    LRESULT		OnAsyncAccept(WPARAM wParam, LPARAM lParam);

    // cleanup if class is not used by any clients
    LRESULT		CheckClients();

    //	Linked list of US management functions
    //	Inserts us at the head of the list
    static void	LinkUs(CAsyncNetThread* pUs);

    //	Removes us from the list
    static void	UnlinkUs(CAsyncNetThread* pUs);

    //	Finds a notifier for the current thread in the linked list
    static CAsyncNetThread* FindNetThreadNotifier();

    /*
     *	Class private data members
     */
    //	Don't try to register multiple times
    static HXBOOL		zm_bClassRegistered;

    static CAsyncNetThread*	zm_pSockNotifiers;

    //	Store the link in a nonstatic member
    CAsyncNetThread*	m_pNextNotifier;

    HWND		m_hWnd;
    HXBOOL		m_bValid;

    //	Win32 this is the thread ID, Win16 this is the current task
    ULONG32		m_ulSession;

    HINSTANCE		m_hInst;


    //	Map that holds our client objects mapped from Socket Handles
    CHXMapPtrToPtr*	m_ClientSocketsMap;
    UINT16		m_nNumSocketsClients;
    HXBOOL		m_bInCancelMode;
    IUnknown*		m_pContext;
    IHXMutex*		m_pThreadEngineMutex;

private:
#ifdef _WIN32
    static UINT		zm_uDestroyMessage;
#endif
};

#endif	/*__CASYNNETTHREAD_H */

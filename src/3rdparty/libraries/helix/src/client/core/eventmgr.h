/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: eventmgr.h,v 1.5 2007/07/06 21:58:11 jfinnecy Exp $
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

#if defined(HELIX_FEATURE_EVENTMANAGER)
#ifndef EVENTMGR_H
#define EVENTMGR_H

// Forward declarations
typedef _INTERFACE IHXScheduler    IHXScheduler;
typedef _INTERFACE IHXEventSink    IHXEventSink;
typedef _INTERFACE IHXEventManager IHXEventManager;
typedef _INTERFACE IHXValues       IHXValues;
typedef _INTERFACE IHXBuffer       IHXBuffer;
typedef _INTERFACE IHXMutex        IHXMutex;
class CRendererEventManager;
class CEventSinkFilterRule;
class CRendererEventCallback;
class CRendererEvent;
class CHXSimpleList;
class CHXMapPtrToPtr;
class CHXGenericCallback;

class CRendererEventManager : public IHXEventManager,
                              public IHXCallback
{
public:
    CRendererEventManager(IUnknown* pContext);
    virtual ~CRendererEventManager();

    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXEventManager methods
    STDMETHOD(AddEventSink)              (THIS_ IHXEventSink* pSink);
    STDMETHOD(RemoveEventSink)           (THIS_ IHXEventSink* pSink);
    STDMETHOD(AddEventSinkFilterRule)    (THIS_ IHXEventSink* pSink,
                                                IHXValues*    pRule);
    STDMETHOD(RemoveEventSinkFilterRule) (THIS_ IHXEventSink* pSink,
                                                IHXValues*    pRule);
    STDMETHOD(FireEvent)                 (THIS_ IHXBuffer* pURLStr,
                                                IHXBuffer* pFragmentStr,
                                                IHXBuffer* pEventNameStr,
                                                IHXValues* pOtherValues);

    // IHXCallback methods
    STDMETHOD(Func) (THIS);

    // CRendererEventManager methods
    STDMETHOD(Close) (THIS);

    static void RendererEventCallback(void* pParam);
protected:
    INT32                   m_lRefCount;
    IUnknown*               m_pContext;
    IHXScheduler*          m_pScheduler;
    CHXSimpleList*          m_pSinkList;
    CHXMapPtrToPtr*         m_pSinkToFilterMap;
    CHXGenericCallback*     m_pCallback;
    CHXSimpleList*          m_pEventQueue;
    IHXMutex*              m_pEventQueueMutex;

    void ClearSinks();
    HXBOOL IsSinkInList(IHXEventSink* pSink);
    void ClearEventQueue();
    HXBOOL ShouldSinkGetEvent(IHXEventSink* pSink, CRendererEvent* pEvent);
    HXBOOL PassFilter(CHXSimpleList* pFilter, CRendererEvent* pEvent);
    HXBOOL PassFilterRule(CEventSinkFilterRule* pRule,
                        CRendererEvent*       pEvent);
    HXBOOL PassFilterRuleString(const char* pszRule, const char* pszEvent);
    void ClearSinkFilterList(CHXSimpleList* pList);
};

class CEventSinkFilterRule
{
public:
    CEventSinkFilterRule(IHXValues* pRule);
    ~CEventSinkFilterRule();

    HXBOOL        Same(CEventSinkFilterRule* pRule);
    const char* GetURL() const;
    const char* GetFragment() const;
    const char* GetEventName() const;
protected:
    IHXValues* m_pRule;

    static HXBOOL        SameString(const char* pszA, const char* pszB);
    static const char* GetString(IHXValues* pValues, const char* pszName);
};

class CRendererEvent
{
public:
    CRendererEvent(IHXBuffer* pURLStr,
                   IHXBuffer* pFragmentStr,
                   IHXBuffer* pEventNameStr,
                   IHXValues* pOtherValues);
    ~CRendererEvent();

    HX_RESULT   Fire(IHXEventSink* pSink);
    const char* GetURL() const;
    const char* GetFragment() const;
    const char* GetEventName() const;
protected:
    IHXBuffer* m_pURLStr;
    IHXBuffer* m_pFragmentStr;
    IHXBuffer* m_pEventNameStr;
    IHXValues* m_pOtherValues;
};

#endif /* #ifdef EVENTMGR_H */

#endif /* #if defined(HELIX_FEATURE_EVENTMANAGER) */

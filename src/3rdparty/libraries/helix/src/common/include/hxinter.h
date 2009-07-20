/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxinter.h,v 1.4 2007/07/06 20:43:41 jfinnecy Exp $
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

#ifndef HXINTER_H
#define HXINTER_H

/*
 * Forward declarations of some interfaces defined here
 */
typedef _INTERFACE IHXEventManager IHXEventManager;
typedef _INTERFACE IHXEventSink    IHXEventSink;
/*
 * Forward declarations of interfaces used here
 */
typedef _INTERFACE IHXValues       IHXValues;
typedef _INTERFACE IHXBuffer       IHXBuffer;

/*
 * Definitions of filter rule key strings
 */
#define FILTER_RULE_KEY_URL       "url"
#define FILTER_RULE_KEY_FRAGMENT  "fragment"
#define FILTER_RULE_KEY_EVENTNAME "eventName"

/****************************************************************************
 * 
 *  Interface:
 *
 *      IHXEventManager
 *
 *  Purpose:
 *
 *      Interface to manage inter-datatype events
 *
 *  IID_IHXEventManager:
 *
 *      {F932B582-0517-4ca4-844C-26A4E8E96983}
 *
 */
DEFINE_GUID(IID_IHXEventManager, 0xf932b582, 0x517, 0x4ca4, 0x84, 0x4c,
            0x26, 0xa4, 0xe8, 0xe9, 0x69, 0x83);

#undef  INTERFACE
#define INTERFACE IHXEventManager

DECLARE_INTERFACE_(IHXEventManager, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXEventManager methods
     */

    /************************************************************************
     *  Method:
     *      IHXEventManager::AddEventSink
     *  Purpose:
     *      Register an event sink
     */
    STDMETHOD(AddEventSink) (THIS_ IHXEventSink* pSink) PURE;

    /************************************************************************
     *  Method:
     *      IHXEventManager::RemoveEventSink
     *  Purpose:
     *      Unregister an event sink
     */
    STDMETHOD(RemoveEventSink) (THIS_ IHXEventSink* pSink) PURE;

    /************************************************************************
     *  Method:
     *      IHXEventManager::AddEventSinkFilterRule
     *  Purpose:
     *      Adds a rule to the event filter.
     *
     *      Filter rules are IHXValues with the following
     *      well-known CString properties:
     *
     *      "url"       - URL generating the event
     *      "fragment"  - object in the URL generating the event
     *      "eventName" - name of event
     *
     *      The filter rule is an implicit AND across the
     *      *specified* parameters of the rule. There is an implicit
     *      OR between different calls to AddEventSinkFilterRule(). For
     *      example, if a sink make two calls to AddEventSinkFilterRule()
     *      with the following two IHXValues:
     *
     *      { "url"       = "rtsp://www.foo.com/bar.svg",
     *        "fragment"  = "my_circle" }
     *
     *      { "eventName" = "ev:change_ads" }
     *
     *      Then this filter would pass:
     *      a) ANY events from the "my_circle" object
     *         in "rtsp://www.foo.com/bar.svg"; OR
     *      b) The "ev:change_ads" event from ANY fragment in ANY URL.
     *
     */
    STDMETHOD(AddEventSinkFilterRule) (THIS_ IHXEventSink* pSink,
                                             IHXValues*    pRule) PURE;

    /************************************************************************
     *  Method:
     *      IHXEventManager::RemoveEventSinkFilterRule
     *  Purpose:
     *      Removes a rule to the event filter. See the Purpose of
     *      AddEventSinkFilterRule() for a description of the rule logic.
     *      Note that the rule is removed based on the *content* of the
     *      IHXValues, and not on the pointer value itself.
     */
    STDMETHOD(RemoveEventSinkFilterRule) (THIS_ IHXEventSink* pSink,
                                                IHXValues*    pRule) PURE;

    /************************************************************************
     *  Method:
     *      IHXEventManager::FireEvent
     *  Purpose:
     *      Send an event to the manager
     *
     *      pURLStr       - URL of the event source (required, error if NULL)
     *      pFragmentStr  - "object" within source that fired event (optional)
     *      pEventNameStr - event name (required, error if NULL) 
     *      pOtherValues  - properties associated with this event (optional)
     */
     STDMETHOD(FireEvent) (THIS_ IHXBuffer* pURLStr,
                                 IHXBuffer* pFragmentStr,
                                 IHXBuffer* pEventNameStr,
                                 IHXValues* pOtherValues) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *      IHXEventSink
 *
 *  Purpose:
 *
 *      Interface to receive events
 *
 *  IID_IHXEventSink
 *
 *      {76CF54BC-9FCE-45e7-90D1-034605F8DD14}
 */
DEFINE_GUID(IID_IHXEventSink, 0x76cf54bc, 0x9fce, 0x45e7, 0x90, 0xd1,
            0x3, 0x46, 0x5, 0xf8, 0xdd, 0x14);

#undef  INTERFACE
#define INTERFACE IHXEventSink

DECLARE_INTERFACE_(IHXEventSink, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXEventSink methods
     */

    /************************************************************************
     *  Method:
     *      IHXEventSink::EventFired
     *  Purpose:
     *      Fire an event that the sink has requested
     *
     *      pURLStr       - URL of the event source
     *      pFragmentStr  - "object" within source that fired event
     *      pEventNameStr - event name
     *      pOtherValues  - properties associated with this event
     */
    STDMETHOD(EventFired) (THIS_ IHXBuffer* pURLStr,
                                 IHXBuffer* pFragmentStr,
                                 IHXBuffer* pEventNameStr,
                                 IHXValues* pOtherValues) PURE;
};

#endif /* #ifndef HXINTER_H */

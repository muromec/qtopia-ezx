/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxcmenu.h,v 1.4 2007/07/06 21:58:18 jfinnecy Exp $
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

#ifndef _HXCMENU_H_
#define _HXCMENU_H_

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE  IHXContextMenu          IHXContextMenu;
typedef _INTERFACE  IHXContextMenuResponse  IHXContextMenuResponse;


/****************************************************************************
 * 
 *  Interface:
 *
 *  IHXContextMenu
 *
 *  Purpose:
 *
 *  Interface implemented by top level clients and provided to renderers.
 *  Allows the renderer to show a context menu and the top level client
 *  to add client specitic commands unknown to the renderer to that menu.
 *
 *  IID_IHXContextMenu:
 *
 *  {00001f00-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXContextMenu, 0x00001f00, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXContextMenu

DECLARE_INTERFACE_(IHXContextMenu, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                REFIID riid,
                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXContextMenu methods
     */

    /************************************************************************
     *  Method:
     *      IHXContextMenu::InitContextMenu
     *  Purpose:
     *      Initializes the context menu to a blank menu, and sets the name
     *      of the "sub menu" for the renderer if appropriate. This will 
     *      clear any previously added menu items and sub menus.
     */
    STDMETHOD(InitContextMenu)  (THIS_
                                 const char* pMenuText
                                 ) PURE;
    
    /************************************************************************
     *  Method:
     *      IHXContextMenu::AddMenuItem
     *  Purpose:
     *      Returns information vital to the instantiation of rendering 
     *      plugins.
     */
    STDMETHOD(AddMenuItem)  (THIS_
                             UINT16      commandID, 
                             const char* pMenuItemText, 
                             HXBOOL        bChecked,
                             HXBOOL        bRadioOn, 
                             HXBOOL        bDisabled
                             ) PURE;
    
    /************************************************************************
     *  Method:
     *      IHXContextMenu::AddMenuItem
     *  Purpose:
     *      Returns information vital to the instantiation of rendering 
     *      plugins.
     */
    STDMETHOD(AddSeparator) (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXContextMenu::AddChildMenu
     *  Purpose:
     *      Returns information vital to the instantiation of rendering 
     *      plugins.
     */
    STDMETHOD(AddChildMenu) (THIS_
                             const char* pMenuText
                             ) PURE;
    
    /************************************************************************
     *  Method:
     *      IHXContextMenu::EndChildMenu
     *  Purpose:
     *      Returns information vital to the instantiation of rendering 
     *      plugins.
     */
    STDMETHOD(EndChildMenu) (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXContextMenu::ChangeMenuItem
     *  Purpose:
     *      Returns information vital to the instantiation of rendering 
     *      plugins.
     */
    STDMETHOD(ChangeMenuItem)   (THIS_
                                 UINT16      commandID, 
                                 const char* pMenuItemText, 
                                 HXBOOL        bChecked,
                                 HXBOOL        bRadioOn, 
                                 HXBOOL        bDisabled
                                 ) PURE;

    /************************************************************************
     *  Method:
     *      IHXContextMenu::ShowMenu
     *  Purpose:
     *      Shows the setup context menu at the specified point.
     */
    STDMETHOD(ShowMenu)     (THIS_
                             IHXContextMenuResponse* pResonse,
                             HXxPoint ptPopup
                             ) PURE;
    
};

/****************************************************************************
 * 
 *  Interface:
 *
 *  IHXContextMenuResponse
 *
 *  Purpose:
 *
 *  Interface implemented by renderers that use the context menut.
 *  Is called to inform the renderer that a particular menu item was
 *  chosen.
 *
 *  IHXContextMenuResponse:
 *
 *  {00001f01-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXContextMenuResponse, 0x00001f01, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
            0xa0, 0x24, 0x40, 0x6d, 0x59);


#undef  INTERFACE
#define INTERFACE   IHXContextMenuResponse

DECLARE_INTERFACE_(IHXContextMenuResponse, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                 REFIID riid,
                                 void** ppvObj) PURE;
    
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXContextMenuResponse methods
     */

    /************************************************************************
     *  Method:
     *      IHXContextMenuResponse::OnCommand
     *  Purpose:
     *      Called to inform the renderer that a command was chosen from
     *      the context menu.
     */
    STDMETHOD(OnCommand)    (THIS_
                             UINT16 commandID
                             ) PURE;
    

    /************************************************************************
     *  Method:
     *      IHXContextMenuResponse::OnCanceled
     *  Purpose:
     *      Called to inform the renderer that the context menu was closed
     *      without a command being chosen from the renders set of commands.
     */
    STDMETHOD(OnCanceled)   (THIS) PURE;

};

#endif /* _HXCMENU_H_ */

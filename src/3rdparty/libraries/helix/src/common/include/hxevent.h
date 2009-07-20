/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxevent.h,v 1.11 2007/08/14 00:43:10 milko Exp $
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

#if defined(USE_XWINDOWS)
#include "X11/keysymdef.h" //for the virtual key definitions below.
#endif

#ifndef _HXEVENT_H_
#define _HXEVENT_H_

#define HX_BASE_EVENT  0x00001000UL

// --------------------------- SURFACE EVENTS --------------------------

// This class of events are events sent to site users of windowless
// sites to notify them of events on the site with platform independent
// messages.
#define HX_SURFACE_EVENTS      (HX_BASE_EVENT + 0x00001000)
#define HX_SURFACE_UPDATE      (HX_SURFACE_EVENTS + 1)
#define HX_SURFACE_MODE_CHANGE (HX_SURFACE_EVENTS + 2)
#define HX_SURFACE_UPDATE2     (HX_SURFACE_EVENTS + 3)
#define HX_SURFACE_NEXT_EVENT  (HX_SURFACE_EVENTS + 4)
#define HX_SURFACE_DAMAGED     (HX_SURFACE_EVENTS + 5)

// HX_SURFACE_DAMAGED is sent by external rendering engine to a
// Helix object representing the surface to notify of the damage
// occuring to the rendering surface and thus needing to update
// it.
//    ULONG32 event;      HX_SURFACE_DAMAGED
//    void*   window;     Native Window - may be null if no window is
//                        associated with the site
//    void*   param1;     NULL = entire surface is damaged
//			  HXREGION* = surface sub-region damaged 
//    void*   param2;     NULL = unused
//    void*   result;     HRESULT result code of message handling
//    HXBOOL    handled;    TRUE if handled, FALSE if not handled

// HX_SURFACE_UPDATE is sent by the site to the renderer when the
// surface has damage and needs to be updated.  The event struct is
// filled out as follows:
//
//    ULONG32 event;      HX_SURFACE_UPDATE
//    void*   window;     Native Window - may be null if no window is
//                        associated with the site
//    void*   param1;     IHXVideoSurface*
//    void*   param2;     UNIX - HXxWindow
//    void*   result;     HRESULT result code of message handling
//    HXBOOL    handled;    TRUE if handled, FALSE if not handled

// HX_SURFACE_UPDATE2
// --------------------
//
// This event is like HX_SURFACE_UPDATE except that it contains info
// on the dirty rects/region assiciated with this site.
// This event is passed before HX_SURFACE_UPDATE. If this event is not
// handled it is converted into a HX_SURFACE_UPDATE and sent again.
//
// window  -- Native Window handle of the nearest parent window. May be NULL
// param1  -- IHXVideoSurface* associated with this site.
// param2  -- HXxExposeInfo* associated with this event. Defined in hxwintyp.h
// result  -- Result code os message handling.
// handled -- TRUE if handled, FALSE if not. If renderer returns TRUE for
//            handled then the system will automatically validate the 
//            entire client area associated with this video surface.


// HX_SURFACE_MODE_CHANGE is sent by the site to the renderer when the
// surface mode should be changed.  This event is optional, but for
// best playback quality it should be processed.  The event struct is
// filled out as follows:
//
//    ULONG32   event;      HX_SURFACE_MODE_CHANGE
//    void*     window;     null
//    void*     param1;     null
//    void*     param2;     HX_VIDEOSURFACE1_RECOMMENDED or
//                          HX_VIDEOSURFACE1_NOT_RECOMMENDED
//    void*     result;     HRESULT result code of message handling
//    HXBOOL      handled;    TRUE if handled, FALSE if not handled
#define HX_VIDEOSURFACE1_RECOMMENDED       1         
#define HX_VIDEOSURFACE1_NOT_RECOMMENDED   2


//------------------------- MOUSE EVENTS ----------------------------

// This class of events are sent to site users to
// notify them of mouse events.
// All mouse events have the event structure filled out as follows:
//
//    UINT32    event;
//    void*     window;
//    void*     param1;     HXxPoint struct with mouse position local to the renderer
//    void*     param2;     UINT32 of flags for modifier keys
//       BITS      DESCRIPTION
//     -------   -------------------------------
//        0       Shift key down while moving or clicking
//        1       Control key down while moving or clicking
//        2       Alt key donw while moving or clicking
//        3       Primary mouse button down while moving.
//        4       Context mouse button down while moving.
//        5       Third mouse button down while moving.
//    void*     result;     HRESULT result code of message handling
//    HXBOOL      handled;    TRUE if handled, FALSE if not handled
//
#define HX_MOUSE_EVENTS        (HX_BASE_EVENT + 0x00002000)

#define HX_SHIFT_KEY           (1<<0) //is the Shift key down while moving?
#define HX_CTRL_KEY            (1<<1) //is the Control key down while moving?
#define HX_ALT_COMMAND_KEY     (1<<2) //is the  Apple/Splat or PC/ALT key down?
#define HX_PRIMARY_BUTTON      (1<<3) //Is the primary button down while moving?
#define HX_CONTEXT_BUTTON      (1<<4) //is the context button down while moving?
#define HX_THIRD_BUTTON        (1<<5) //is the third button down while moving?

#define HX_PRIMARY_BUTTON_DOWN (HX_MOUSE_EVENTS + 1)
#define HX_PRIMARY_BUTTON_UP   (HX_MOUSE_EVENTS + 2)
#define HX_CONTEXT_BUTTON_DOWN (HX_MOUSE_EVENTS + 3)
#define HX_CONTEXT_BUTTON_UP   (HX_MOUSE_EVENTS + 4)
#define HX_MOUSE_MOVE          (HX_MOUSE_EVENTS + 5)
#define HX_MOUSE_ENTER         (HX_MOUSE_EVENTS + 6)
#define HX_MOUSE_LEAVE         (HX_MOUSE_EVENTS + 7)
#define HX_THIRD_BUTTON_DOWN   (HX_MOUSE_EVENTS + 8)
#define HX_THIRD_BUTTON_UP     (HX_MOUSE_EVENTS + 9)
#define HX_SET_CURSOR          (HX_MOUSE_EVENTS + 10)
#define HX_SET_STATUS          (HX_MOUSE_EVENTS + 11)
#define HX_PRIMARY_DBLCLK      (HX_MOUSE_EVENTS + 12)
#define HX_CONTEXT_DBLCLK      (HX_MOUSE_EVENTS + 13)
#define HX_THIRD_DBLCLK        (HX_MOUSE_EVENTS + 14)

// This class of events are sent to renderers to
// notify them of the validation of the window
// All window events have the event structure filled out as follows:
//
//    UINT32    event;
//    void*     window;
//    void*     UNUSED;
//    void*     UNUSED;
//    void*     result;     HRESULT result code of message handling
//    HXBOOL      handled;    TRUE if handled, FALSE if not handled
//
#define HX_WINDOW_EVENTS       HX_BASE_EVENT + 0x00003000

#define HX_ATTACH_WINDOW       HX_WINDOW_EVENTS + 1
#define HX_DETACH_WINDOW       HX_WINDOW_EVENTS + 2

// This class of events are sent to site users to
// notify them of keyboard events.
// All keyboard events have the event structure filled out as follows:
//
//    UINT32    event;
//    void*     window;
//    HXBOOL      handled;    TRUE if handled, FALSE if not handled
//    void*     result;     HRESULT result code of message handling
//    void *    param1;     Contents depends on keyboard event:
//
//HX_CHAR event.
//    param1   Translated ASCII Char Code.
//   --------  HX_CHAR events will have this as the translated char
//             of the key acted upon and the result of any modifiers
//             like the shift key, control key, caps lock, etc. If a
//             virtual key has been pressed (like an arrow key) then
//             param1 will be set to a HX_VK code representing the
//             virtual key pressed and the bit-field in param2 will
//             indicate that a virtual key was pressed.
//HX_KEY_DOWN or HX_KEY_UP
//    param1   Non-translated ASCII Char Code of the key pressed or
//             released. 
//   --------  
//             This is the same as HX_CHAR except that the ASCII char
//             has not been translated by the modifiers.
//
//    void *    param2;     Description bit field.
//       BITS      DESCRIPTION
//     -------   -------------------------------
//     
// WIN   0-7     OEM specific scan code.
// UNIX  0-7     keycode. For 1-0x58 they equal scancode.
//       8       Shift key down
//       9       Control key down
//       10      ALT key down or Apple/Splat key
//       11      Caps-Lock on.
//       12      Scroll-Lock on.
//       13      Num-Lock on.
//       14      1 if event represents a virtual key. 0 if not.
//       15      1 if key came from the extended part of the keyboard.
//               (ie right cntrl, right alt, keypad, etc).
//       
#define HX_KEYBOARD    HX_BASE_EVENT + 0x00004000
#define HX_CHAR        HX_KEYBOARD + 1 //The translated key event
#define HX_KEY_UP      HX_KEYBOARD + 2 //raw key release non-translated.
#define HX_KEY_DOWN    HX_KEYBOARD + 3 //raw key down non-translated event.
#define HX_SET_FOCUS   HX_KEYBOARD + 4
#define HX_LOSE_FOCUS  HX_KEYBOARD + 5

//
// Keyboard event modifiers. Must correspond to the structure in
// param2 above.
//
#define HX_NO_MODIFIERS      0
#define HX_SHIFT_MASK        (1<<8)
#define HX_CTRL_MASK         (1<<9)
#define HX_ALT_MASK          (1<<10) //Also the apple spat key.
#define HX_APPLE_SPLAT_MASK  HX_ALT_MASK
#define HX_CAPS_LOCK_MASK    (1<<11)
#define HX_NUM_LOCK_MASK     (1<<12)
#define HX_SCROLL_LOCK_MASK  (1<<13)
#define HX_VIRTUAL_KEY_MASK  (1<<14)
#define HX_EXTENDED_KEY_MASK (1<<15)

//
// RMA virtual key definitions....
//requires inclusion of the platform specific header files defining
//these keys, if used. (ie, keysymdef.h on UNIX and winresrc.h on windows.
//
#if defined( _WINDOWS )
#define HX_VK_LBUTTON        VK_LBUTTON        
#define HX_VK_RBUTTON        VK_RBUTTON        
#define HX_VK_CANCEL         VK_CANCEL         
#define HX_VK_MBUTTON        VK_MBUTTON        
#define HX_VK_BACK           VK_BACK           
#define HX_VK_TAB            VK_TAB            
#define HX_VK_CLEAR          VK_CLEAR          
#define HX_VK_RETURN         VK_RETURN         
#define HX_VK_SHIFT          VK_SHIFT          
#define HX_VK_CONTROL        VK_CONTROL        
#define HX_VK_MENU           VK_MENU           
#define HX_VK_PAUSE          VK_PAUSE          
#define HX_VK_CAPITAL        VK_CAPITAL        
#define HX_VK_ESCAPE         VK_ESCAPE         
#define HX_VK_SPACE          VK_SPACE          
#define HX_VK_PRIOR          VK_PRIOR          
#define HX_VK_NEXT           VK_NEXT           
#define HX_VK_END            VK_END            
#define HX_VK_HOME           VK_HOME           
#define HX_VK_LEFT           VK_LEFT           
#define HX_VK_UP             VK_UP             
#define HX_VK_RIGHT          VK_RIGHT          
#define HX_VK_DOWN           VK_DOWN           
#define HX_VK_SELECT         VK_SELECT
#define HX_VK_EXECUTE        VK_EXECUTE        
#define HX_VK_SNAPSHOT       VK_SNAPSHOT       
#define HX_VK_INSERT         VK_INSERT         
#define HX_VK_DELETE         VK_DELETE         
#define HX_VK_HELP           VK_HELP           
#define HX_VK_LWIN           VK_LWIN           
#define HX_VK_RWIN           VK_RWIN           
#define HX_VK_APPS           VK_APPS           
#define HX_VK_NUMPAD0        VK_NUMPAD0        
#define HX_VK_NUMPAD1        VK_NUMPAD1        
#define HX_VK_NUMPAD2        VK_NUMPAD2        
#define HX_VK_NUMPAD3        VK_NUMPAD3        
#define HX_VK_NUMPAD4        VK_NUMPAD4        
#define HX_VK_NUMPAD5        VK_NUMPAD5        
#define HX_VK_NUMPAD6        VK_NUMPAD6        
#define HX_VK_NUMPAD7        VK_NUMPAD7        
#define HX_VK_NUMPAD8        VK_NUMPAD8        
#define HX_VK_NUMPAD9        VK_NUMPAD9        
#define HX_VK_MULTIPLY       VK_MULTIPLY       
#define HX_VK_ADD            VK_ADD            
#define HX_VK_SEPARATOR      VK_SEPARATOR      
#define HX_VK_SUBTRACT       VK_SUBTRACT       
#define HX_VK_DECIMAL        VK_DECIMAL        
#define HX_VK_DIVIDE         VK_DIVIDE         
#define HX_VK_F1             VK_F1             
#define HX_VK_F2             VK_F2             
#define HX_VK_F3             VK_F3             
#define HX_VK_F4             VK_F4             
#define HX_VK_F5             VK_F5             
#define HX_VK_F6             VK_F6             
#define HX_VK_F7             VK_F7             
#define HX_VK_F8             VK_F8             
#define HX_VK_F9             VK_F9             
#define HX_VK_F10            VK_F10            
#define HX_VK_F11            VK_F11            
#define HX_VK_F12            VK_F12            
#define HX_VK_F13            VK_F13            
#define HX_VK_F14            VK_F14            
#define HX_VK_F15            VK_F15            
#define HX_VK_F16            VK_F16            
#define HX_VK_F17            VK_F17            
#define HX_VK_F18            VK_F18            
#define HX_VK_F19            VK_F19            
#define HX_VK_F20            VK_F20            
#define HX_VK_F21            VK_F21            
#define HX_VK_F22            VK_F22            
#define HX_VK_F23            VK_F23            
#define HX_VK_F24            VK_F24            
#define HX_VK_NUMLOCK        VK_NUMLOCK        
#define HX_VK_SCROLL         VK_SCROLL         
#elif defined(_UNIX) && !defined(_MAC_UNIX)
#define HX_VK_LBUTTON        XK_Pointer_Button1
#define HX_VK_RBUTTON        XK_Pointer_Button2
#define HX_VK_CANCEL         XK_Cancel
#define HX_VK_MBUTTON        XK_Pointer_Button3
#define HX_VK_BACK           XK_BackSpace
#define HX_VK_TAB            XK_Tab
#define HX_VK_CLEAR          XK_Begin     //Usually '5' on the keypad.
#define HX_VK_RETURN         XK_Return    //XK_KP_Enter will be mapped to this.
#define HX_VK_SHIFT          XK_Shift_L   //XK_Shift_R will be translated to _L
#define HX_VK_CONTROL        XK_Control_L //XK_Control_R will be mapped to _L
#define HX_VK_MENU           XK_Alt_L     //XK_Alt_R will be mapped to _L
#define HX_VK_PAUSE          XK_Pause
#define HX_VK_CAPITAL        XK_Caps_Lock
#define HX_VK_ESCAPE         XK_Escape
#define HX_VK_SPACE          XK_space
#define HX_VK_PRIOR          XK_Prior     //XK_KP_Prior wil be mapped to this.
#define HX_VK_NEXT           XK_Next      //XK_KP_Next wil be mapped to this.
#define HX_VK_END            XK_End       //XK_KP_End wil be mapped to this.
#define HX_VK_HOME           XK_Home      //XK_KP_Home will be mapped to this.
#define HX_VK_LEFT           XK_Left      //XK_KP_Left will be mapped to this.
#define HX_VK_UP             XK_Up        //XK_KP_Up will be mapped to this.
#define HX_VK_RIGHT          XK_Right     //XK_KP_Right will be mapped to this.
#define HX_VK_DOWN           XK_Down      //XK_KP_Down will be mapped to this.
#define HX_VK_SELECT         XK_Select
#define HX_VK_EXECUTE        XK_Execute
#define HX_VK_SNAPSHOT       XK_Print     //Not supported.
#define HX_VK_INSERT         XK_Insert    //XK_KP_Insert will be mapped to this.
#define HX_VK_DELETE         XK_Delete    //XK_KP_Delete will be mapped to this.
#define HX_VK_HELP           XK_Help
#define HX_VK_LWIN           XK_Meta_L
#define HX_VK_RWIN           XK_Meta_R
#define HX_VK_APPS           XK_VoidSymbol //Not used.......
#define HX_VK_NUMPAD0        XK_KP_0
#define HX_VK_NUMPAD1        XK_KP_1
#define HX_VK_NUMPAD2        XK_KP_2
#define HX_VK_NUMPAD3        XK_KP_3
#define HX_VK_NUMPAD4        XK_KP_4
#define HX_VK_NUMPAD5        XK_KP_5
#define HX_VK_NUMPAD6        XK_KP_6
#define HX_VK_NUMPAD7        XK_KP_7
#define HX_VK_NUMPAD8        XK_KP_8
#define HX_VK_NUMPAD9        XK_KP_9
#define HX_VK_MULTIPLY       XK_KP_Multiply
#define HX_VK_ADD            XK_KP_Add
#define HX_VK_SEPARATOR      XK_KP_Separator
#define HX_VK_SUBTRACT       XK_KP_Subtract
#define HX_VK_DECIMAL        XK_KP_Decimal
#define HX_VK_DIVIDE         XK_KP_Divide
#define HX_VK_F1             XK_F1
#define HX_VK_F2             XK_F2
#define HX_VK_F3             XK_F3
#define HX_VK_F4             XK_F4
#define HX_VK_F5             XK_F5
#define HX_VK_F6             XK_F6
#define HX_VK_F7             XK_F7
#define HX_VK_F8             XK_F8
#define HX_VK_F9             XK_F9
#define HX_VK_F10            XK_F10
#define HX_VK_F11            XK_F11
#define HX_VK_F12            XK_F12
#define HX_VK_F13            XK_F13
#define HX_VK_F14            XK_F14
#define HX_VK_F15            XK_F15
#define HX_VK_F16            XK_F16
#define HX_VK_F17            XK_F17
#define HX_VK_F18            XK_F18
#define HX_VK_F19            XK_F19
#define HX_VK_F20            XK_F20
#define HX_VK_F21            XK_F21
#define HX_VK_F22            XK_F22
#define HX_VK_F23            XK_F23
#define HX_VK_F24            XK_F24
#define HX_VK_NUMLOCK        XK_Num_Lock
#define HX_VK_SCROLL         XK_Scroll_Lock
#endif

// --------------------------- CONFIGURATION EVENTS --------------------------
// This class of events are sent to either promote and inquire about
// set of conditions relevant to configuration of the components. 
#define HX_CONFIG_EVENTS	HX_BASE_EVENT + 0x00005000

#define HX_RETRIEVE_CONFIG	(HX_CONFIG_EVENTS + 1)
// HX_RETRIEVE_CONFIG is sent to retrieve the
// configuration information from the object on the event network.  
// Each objet receiving this event can insert globally applicable
// configuration information. On return, the calling object
// can examine the global configuration provided and act
// accordingly.
// On one specific case, this event by top level site at the time of
// initialization to a delegated site to retrive confguration
// information for the site.
//    ULONG32 event;      HX_RETRIEVE_CONFIG
//    void*   window;     *HXxWindow - may be null if no window is
//                        associated with the site or event is not
//			  dispatched by the site or if window specific
//			  configuration information is not needed.
//    void*   param1;     IHXValues* interface to used to insert
//			  the configuration information.
//			  The provided interface is no longer valid
//			  after the event notification returns.
//    void*   param2;     IHXValues* interface used to inidcate type
//			  of configuration needed.  If NULL, the
//			  request is for all known type of configuration
//			  values.  When the interface is provided, the
//			  callee may optionally restrict the configuration
//			  provided to the ndicated set.
//    void*   result;     HXRESULT result code of message handling
//    HXBOOL  handled;    TRUE if handled an thus at least one piece
//			  of configuration was inserted or no configuration
//			  is known to be available. FALSE if configuration
//			  is known to exist but could not be obtained.

// Each event class should have a comment describing the kinds
// of events that belong to this class
// The next event class should use this base:
#define HX_NEXT_EVENT_CLASS    HX_BASE_EVENT + 0x00006000

// $Private:
#ifdef _WINDOWS
// NH: embeded players need this to tell our IHXSiteWindowed implementation 
// when the site is moved in the browser
#define MSG_EMBEDEDSITEMOVING   "EmbededSiteWindowMovingMsg"
#endif
// $EndPrivate.

#endif // _HXEVENT_H_

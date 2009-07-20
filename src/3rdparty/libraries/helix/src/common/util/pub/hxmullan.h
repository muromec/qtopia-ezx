/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxmullan.h,v 1.4 2005/03/14 19:36:40 bobclark Exp $
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

#ifdef __cplusplus
extern "C" {
#endif

void		HXSetupMulLang(HINSTANCE hInstBase, HXBOOL bWin31);
void		HXCleanupMulLang();

int			HXLoadString(UINT idResource, LPSTR lpszBuffer, int cbBuffer);
HBITMAP		HXLoadBitmap(LPCSTR pszName);
HICON		HXLoadIcon(LPCSTR pszName);
HCURSOR		HXLoadCursor(LPCSTR pszName);
#ifndef WIN32_PLATFORM_PSPC
HMENU		HXLoadMenu(LPCSTR pszName);
HMENU 		HXLoadPopupMenu(LPCSTR pMenuName);
HFONT		HXCreateFont (int nHeight,
						  int nWidth,
						  int nEscapement,
						  int nOrientation,
						  int fnWeight,
						  DWORD fdwItalic,
						  DWORD fdwUnderline,
						  DWORD fdwStrikeOut,
						  DWORD fdwCharSet,
						  DWORD fdwOutputPrecision,
						  DWORD fdwClipPrecision,
						  DWORD fdwQuality,
						  DWORD fdwPitchAndFamily,
						  LPCSTR lpszFace);
HFONT		HXGetStockFont (int stockFont);
#endif
int			HXDialogBox(LPCSTR pszName, HWND hwndOwner, DLGPROC dlgprc);
int			HXDialogBoxParam(LPCSTR pszName, HWND hwndOwner, DLGPROC dlgprc, LPARAM lParamInit);
HWND		HXCreateDialog(LPCSTR pszName, HWND hwndOwner, DLGPROC dlgprc);
HWND		HXCreateDialogParam(LPCSTR pszName, HWND hwndOwner, DLGPROC dlgprc, LPARAM lParamInit);

void *      HXLoadDialogBoxTemplate(LPCSTR pszName, HGLOBAL * pHMem);

// Helpful for resetting a windows menu after a Locale change
void 		HXResetWindowMenu(HWND hwnd, LPCSTR pszName);

// Helpful for resetting a windows text after a Locale change
void 		HXResetWindowText(HWND hwnd, UINT idResource);

// This function loads satalite DLL list.
void 		HXSetLocale(LCID locale);
// This function returns current locale global.
LCID 		HXGetLocale();
// This function checks current locale global against 
// preference, if they are different, calls HXSetLocale().
LCID 		HXCheckLocale();
// Fills in listbox with all installed languages
void		HXLoadLocaleListbox(HWND hwndListBox);

// Maximum number of satellite DLLs per language
#define MAX_LANG_DLLS	10

#ifdef _WIN32
#define HX_MAKELCID(langid)	(MAKELCID(langid,SORT_DEFAULT))
#else
#define HX_MAKELCID(langid)	(MAKELCID(langid))
#endif

#define HX_DEFAULT_LOCALE (HX_MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)))

#ifdef __cplusplus
} // extern "C"
#endif


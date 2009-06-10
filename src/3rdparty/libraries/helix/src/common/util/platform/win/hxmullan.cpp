/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxmullan.cpp,v 1.8 2007/07/06 20:39:21 jfinnecy Exp $
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

#include "hxtypes.h"
#ifdef _WINDOWS
#include "hlxclib/windows.h"
#endif
#include "hxcom.h"
#include "hxver.h"
#include "hxmullan.h"		// The API we are implementing
#include <string.h>			// Needed for strchr()
#include <stdlib.h>			// Needed for atol()
#include <stdio.h>			// Needed for sprintf()
#include "hxstrutl.h"

#ifndef _WIN32
#include <shellapi.h>		// Needed for Registry Stuff
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

//
// Function prototypes for load and create font from satellite DLL
//
#define NAME_CREATE_FONT "Create_Font"
#define NAME_LOAD_FONT   "Load_Font"

static char Name_Create_Font [] = NAME_CREATE_FONT;
static char Name_Load_Font [] = NAME_LOAD_FONT;

typedef HFONT     (FAR PASCAL * CREATE_FONT)(int, int ,	int , int ,
											int , DWORD , DWORD , DWORD ,
											DWORD , DWORD , DWORD , DWORD ,
                                                                                        DWORD , LPCSTR );
typedef HFONT	  (FAR PASCAL * LOAD_FONT) (int);



HXBOOL		g_bWindows31 = FALSE;	// We need to know about Win3.1 
									// since it's registry is kinda
									// messed up.
HINSTANCE	g_hInstBase = NULL;
LCID		g_locale = HX_DEFAULT_LOCALE;
HINSTANCE	g_hLangDLLs[MAX_LANG_DLLS];
int			g_nLangDLLs = 0;


////////////////////////////////////////////
//
// Quick Class to dynamically load the National Language support
// under Win 16 and Win 32.
//
////////////////////////////////////////////
typedef		int (HXEXPORT_PTR FPGETLOCALEINFOA)(LCID LCID, LCTYPE LCType, LPSTR lpLCData, int cchData);

class CDynamicLoadLanguageService
{
private:
	HINSTANCE m_hInstance;
	FPGETLOCALEINFOA fpGetLocaleInfoA;
public:
	CDynamicLoadLanguageService();
	~CDynamicLoadLanguageService();
	HXBOOL IsValidCodePage(UINT CodePage) const;
	int GetLocaleInfoA(LCID LCID, LCTYPE LCType, LPSTR lpLCData, 
		int cchData) const;
	HXBOOL IsValid() const { return (m_hInstance > (HINSTANCE)32 && fpGetLocaleInfoA != NULL);};
};

CDynamicLoadLanguageService::CDynamicLoadLanguageService()
{
#ifdef _WIN32
	m_hInstance = LoadLibrary(OS_STRING("Kernel32.dll"));
#else
	m_hInstance = LoadLibrary(OS_STRING("ole2nls.dll"));
#endif
	if (m_hInstance > (HINSTANCE)32)
	{
		fpGetLocaleInfoA = (FPGETLOCALEINFOA)GetProcAddress(m_hInstance, OS_STRING("GetLocaleInfoA"));
	}
}

CDynamicLoadLanguageService::~CDynamicLoadLanguageService()
{
	FreeLibrary(m_hInstance);
	fpGetLocaleInfoA = NULL;
}

HXBOOL CDynamicLoadLanguageService::IsValidCodePage(UINT CodePage) const
{
	// There doesn't appear to be a good way to determine in a 16bit application
	// whether or not a particular code page is supported, therefore, we will
	// just assume it is for 16bit apps, and only check for 32bit apps
#ifdef _WIN32
	return ::IsValidCodePage(CodePage);
#else
	return TRUE;
#endif
}

int CDynamicLoadLanguageService::GetLocaleInfoA(LCID theLCID, LCTYPE LCType, LPSTR lpLCData, 
		int cchData) const
{
	LCID lLCID = theLCID;
	LCTYPE lLCType = LCType;
	LPSTR llpLCData = lpLCData;
	int lcchData = cchData;
	int retVal = fpGetLocaleInfoA(lLCID, lLCType, llpLCData, lcchData);

	return retVal;
}
/////////////////////////////////////////
//
// Forward Declarations of some local functions!
//
/////////////////////////////////////////
void WriteLocaleToRegistry(LCID locale);
void ChangeSpaces(char* pString);
LCID ReadLocaleFromRegistry();
void UnloadLanguageDLLs();

void HXSetupMulLang(HINSTANCE hInstBase, HXBOOL bWin31)
{
	g_hInstBase = hInstBase;
	g_bWindows31 = bWin31;
}

void HXCleanupMulLang()
{
	UnloadLanguageDLLs();
}

void UnloadLanguageDLLs()
{
	// First unload any and all previously loaded DLLs!
	for (int ndx = 0; ndx < g_nLangDLLs; ndx++)
	{
		if (g_hLangDLLs[ndx])
		{
			FreeLibrary(g_hLangDLLs[ndx]);
			g_hLangDLLs[ndx] = NULL;
		}
	}
	g_nLangDLLs = 0;
}

// This function loads satalite DLL list.
void HXSetLocale(LCID locale)
{
	if (locale != g_locale)
	{
		// First unload any and all previously loaded DLLs!
		UnloadLanguageDLLs();

		// Now search registry and load DLLs for this locale!
		HKEY	hKey;
		char*	pKeyName = new char[256];
		char	szLocaleCode[20]; /* Flawfinder: ignore */
		char	szDLLPath[256]; /* Flawfinder: ignore */
		int		nRegCount = 0;
			
		sprintf(szLocaleCode,"%d",locale); /* Flawfinder: ignore */
		//GetLocaleInfoA(locale,LOCALE_SABBREVLANGNAME,szSLang,4);
			
		SafeSprintf(pKeyName, 256, "Software\\%s\\Languages\\", HXVER_COMMUNITY);
#ifdef _WIN32
		SafeStrCat(pKeyName, "32bit\\", 256);
#else
		SafeStrCat(pKeyName, "16bit\\", 256);
#endif
		SafeStrCat(pKeyName, szLocaleCode, 256);

		ChangeSpaces(pKeyName);
	
		long lResult;
		if(RegOpenKey(HKEY_CLASSES_ROOT, pKeyName, &hKey) == ERROR_SUCCESS)
		{
			lResult = RegEnumKey(hKey, nRegCount, szDLLPath, sizeof(szDLLPath));
			while(lResult == ERROR_SUCCESS)
			{
				g_hLangDLLs[g_nLangDLLs] = LoadLibrary(OS_STRING(szDLLPath));
				if (g_hLangDLLs[g_nLangDLLs] > (HINSTANCE)32)
				{
					g_nLangDLLs++;
				}
				nRegCount++;
				lResult = RegEnumKey(hKey, nRegCount, szDLLPath, sizeof(szDLLPath));
			}
			RegCloseKey(hKey);
		}
		delete[] pKeyName;

		// Now, we need to set our stored locale setting
		g_locale = locale;

		// We should also set the preferred locale in the registry!
		WriteLocaleToRegistry(locale);
	}
}

void WriteLocaleToRegistry(LCID locale)
{
	HKEY	hKey;
	char*	pKeyName = new char[256];
	char	szLocaleCode[20]; /* Flawfinder: ignore */
	
	sprintf(szLocaleCode,"%d",locale); /* Flawfinder: ignore */

	SafeSprintf(pKeyName, 256, "Software\\%s\\Languages\\Locale", HXVER_COMMUNITY);	
	ChangeSpaces(pKeyName);
	
	long lResult;
	if(RegCreateKey(HKEY_CLASSES_ROOT, pKeyName, &hKey) == ERROR_SUCCESS)
	{
		lResult = RegSetValue(hKey, "", REG_SZ, szLocaleCode, strlen(szLocaleCode));
		RegCloseKey(hKey);
	}
	delete[] pKeyName;
}

LCID ReadLocaleFromRegistry()
{
	HKEY	hKey;
	char*	pKeyName = new char[256];
	char	szLocaleCode[20]; /* Flawfinder: ignore */
	long    lSize;
	int		nRegCount = 1;
	LCID	locale = HX_DEFAULT_LOCALE;

	SafeSprintf(pKeyName, 256, "Software\\%s\\Languages\\Locale", HXVER_COMMUNITY);	
	ChangeSpaces(pKeyName);
	
	long lResult;
	if(RegOpenKey(HKEY_CLASSES_ROOT, pKeyName, &hKey) == ERROR_SUCCESS)
	{
		lSize = 20;
		lResult = RegQueryValue(hKey, "", szLocaleCode, &lSize);
		if (lResult == ERROR_SUCCESS)
		{
			locale = atol(szLocaleCode);
		}
		RegCloseKey(hKey);
	}
	delete[] pKeyName;
	return locale;
}

void ChangeSpaces(char* pString)
{
	// For Windows 3.1 we are using '_' instead of spaces
	if (g_bWindows31)
	{
	    char * pSpace;
	    while((pSpace = strchr(pString, ' ')) != NULL)
	        *pSpace = '_';
	}
}


// This function returns current locale global.
LCID HXGetLocale()
{
	return g_locale;
}

// This function checks current locale global against 
// preference, if they are different, calls HXSetLocale().
LCID HXCheckLocale()
{
	LCID localeFromPrefs = ReadLocaleFromRegistry();
	if (localeFromPrefs != g_locale)
	{
		HXSetLocale(localeFromPrefs);
	}
	return g_locale;
}

static const char LOCALE_FORMAT_STRING[] = "%s (%s)";


// returns TRUE if the current item is selected!
HXBOOL AddLocaleListbox(HWND hwndListBox, LCID locale)
{
	char*	pLocaleName = new char[256];
	char*	pEnglishName = new char[256];
	HXBOOL	bSelected = FALSE;
	CDynamicLoadLanguageService LanguageService;

	if(LanguageService.IsValid())
	{
		// First we want to make sure this machine can handle this
		// language at all. To do this it needs to have the code page
		// correctly installed.
		
		char szCodePage[7]; /* Flawfinder: ignore */

		// Ask NLS for the Country Name!
		if (LanguageService.GetLocaleInfoA(locale, LOCALE_IDEFAULTCODEPAGE, szCodePage, sizeof(szCodePage)-1))
		{
			int nCodePage = atoi(szCodePage);

			if (LanguageService.IsValidCodePage(nCodePage))
			{
				// Ask NLS for the Country Name!
				if (LanguageService.GetLocaleInfoA(locale, LOCALE_SNATIVELANGNAME, pLocaleName, 255) &&
					LanguageService.GetLocaleInfoA(locale, LOCALE_SENGLANGUAGE, pEnglishName, 255))
				{
					// Compose the string; it will look like "NATIVELANGNAME (ENGLANGUAGE)"
                                        UINT32 ulFullDescBufLen =  strlen (pLocaleName) +
                                                                   strlen (pEnglishName) +
                                                                   strlen (LOCALE_FORMAT_STRING) + 1;
					char*	pFullDescription = new char [ulFullDescBufLen];

					SafeSprintf (pFullDescription, ulFullDescBufLen, LOCALE_FORMAT_STRING, pLocaleName, pEnglishName);

					// First Add the String...
					int nLBIndex = (int)SendMessage(hwndListBox,LB_ADDSTRING,0,(LPARAM)pFullDescription);
								
					// Then set the item data...
					SendMessage(hwndListBox,LB_SETITEMDATA,nLBIndex,(LPARAM)locale);
								
					// If this is the current locale, then select it!
					if (locale == g_locale)
					{
						SendMessage(hwndListBox,LB_SETCURSEL,nLBIndex,0);
						bSelected = TRUE;
					}
	
					delete [] pFullDescription;
				}
			}
		}
	}
	
	delete [] pLocaleName;
	delete [] pEnglishName;

	return bSelected;
}

// Fills in listbox with all installed languages
void HXLoadLocaleListbox(HWND hwndListBox)
{
	// Now search registry and load DLLs for this locale!
	HKEY	hKey;
	char*	pKeyName = new char[256];
	char	szLocaleCode[20]; /* Flawfinder: ignore */
	int		nRegCount = 0;
	LCID	thisLocale;
	HXBOOL	bSomethingSelected = FALSE;
	LCID	englishLocale = HX_MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_NEUTRAL));
				
	SafeSprintf(pKeyName, 256, "Software\\%s\\Languages\\", HXVER_COMMUNITY);	
#ifdef _WIN32
	SafeStrCat(pKeyName, "32bit", 256);
#else
	SafeStrCat(pKeyName, "16bit", 256);
#endif
	
	ChangeSpaces(pKeyName);
		
	long lResult;
	if(RegOpenKey(HKEY_CLASSES_ROOT, pKeyName, &hKey) == ERROR_SUCCESS)
	{
		lResult = RegEnumKey(hKey, nRegCount, szLocaleCode, sizeof(szLocaleCode));
		while(lResult == ERROR_SUCCESS)
		{
			// The Locale is coded in the key name!
			thisLocale = (LCID)(atol(szLocaleCode));

			// If this is english, then don't add it yet, but remember 
			// so we can add it at the end. We do this so that we will
			// be able to set english as the selected language.
			if (thisLocale != englishLocale)
			{
				// Add it, this will check for validity, current settings, etc.
				if (AddLocaleListbox(hwndListBox,thisLocale))
				{
					bSomethingSelected = TRUE;
				}
			}
			
			nRegCount++;
			lResult = RegEnumKey(hKey, nRegCount, szLocaleCode, sizeof(szLocaleCode));
		}
		RegCloseKey(hKey);
	}

	//////////////////////////////////////////////////
	// Last, but not least, don't forget English. 
	// We always support English! <g>
	//////////////////////////////////////////////////
	
	// If something else hasn't yet been selected, then
	// we will make sure English is selected!
	if (!bSomethingSelected)
	{
		g_locale = englishLocale;
	}
	AddLocaleListbox(hwndListBox,englishLocale);

	// Finally, clean up any mess we made!
	delete[] pKeyName;
}

////////////////////////////////////////////////////////////////////////////////
//
//
//
//
int HXLoadString(UINT idResource, LPSTR lpszBuffer, int cbBuffer)
{
	HXBOOL	bLoaded = FALSE;
	int	output = 0;

	HXCheckLocale();
	
	HINSTANCE hTempDLL;
	// First Check in Language DLLs
	for (int ndx = 0; ndx < g_nLangDLLs; ndx++)
	{
		if (g_hLangDLLs[ndx])
		{
			hTempDLL = g_hLangDLLs[ndx];
			output = LoadString(hTempDLL, idResource, 
					    OS_STRING2(lpszBuffer, cbBuffer), 
					    cbBuffer);
			if (output)
			{
				bLoaded = TRUE;
				break; // for
				
			} // if output
			
		} // if some handle
		
	} // for

	if (!bLoaded)
	{
		output = LoadString(g_hInstBase, idResource, 
				    OS_STRING2(lpszBuffer, cbBuffer), 
				    cbBuffer);
	}
	return output;
}


HBITMAP HXLoadBitmap(LPCSTR pszName)
{
	HXBOOL	bLoaded = FALSE;
	HBITMAP	output = NULL;

	HXCheckLocale();

	HINSTANCE hTempDLL;
	// First Check in Language DLLs
	for (int ndx = 0; ndx < g_nLangDLLs; ndx++)
	{
		if (g_hLangDLLs[ndx])
		{
			hTempDLL = g_hLangDLLs[ndx];
			output = LoadBitmap(hTempDLL, OS_STRING(pszName));
			if (output)
			{
				bLoaded = TRUE;
				break; // for
				
			} // if output
			
		} // if some handle
		
	} // for

	if (!bLoaded)
	{
		output = LoadBitmap(g_hInstBase, OS_STRING(pszName));
	}
	return output;
}


HICON HXLoadIcon(LPCSTR pszName)
{
	HXBOOL	bLoaded = FALSE;
	HICON	output = NULL;

	HXCheckLocale();

	HINSTANCE hTempDLL;
	// First Check in Language DLLs
	for (int ndx = 0; ndx < g_nLangDLLs; ndx++)
	{
		if (g_hLangDLLs[ndx])
		{
			hTempDLL = g_hLangDLLs[ndx];
			output = LoadIcon(hTempDLL, OS_STRING(pszName));
			if (output)
			{
				bLoaded = TRUE;
				break; // for
				
			} // if output
			
		} // if some handle
		
	} // for
	if (!bLoaded)
	{
		output = LoadIcon(g_hInstBase, OS_STRING(pszName));
	}
	return output;
}

HCURSOR HXLoadCursor(LPCSTR pszName)
{
	HXBOOL	bLoaded = FALSE;
	HCURSOR	output = NULL;

	HXCheckLocale();

	HINSTANCE hTempDLL;
	// First Check in Language DLLs
	for (int ndx = 0; ndx < g_nLangDLLs; ndx++)
	{
		if (g_hLangDLLs[ndx])
		{
			hTempDLL = g_hLangDLLs[ndx];
			output = LoadCursor(hTempDLL, OS_STRING(pszName));
			if (output)
			{
				bLoaded = TRUE;
				break; // for
				
			} // if output
			
		} // if some handle
		
	} // for
	if (!bLoaded)
	{
		output = LoadCursor(g_hInstBase, OS_STRING(pszName));
	}
	return output;
}

HMENU HXLoadMenu(LPCSTR pszName)
{
	HXBOOL	bLoaded = FALSE;
	HMENU	output = NULL;

	HXCheckLocale();

	HINSTANCE hTempDLL;
	// First Check in Language DLLs
	for (int ndx = 0; ndx < g_nLangDLLs; ndx++)
	{
		if (g_hLangDLLs[ndx])
		{
			hTempDLL = g_hLangDLLs[ndx];
			output = LoadMenu(hTempDLL, OS_STRING(pszName));
			if (output)
			{
				bLoaded = TRUE;
				break; // for
				
			} // if output
			
		} // if some handle
		
	} // for
	if (!bLoaded)
	{
		output = LoadMenu(g_hInstBase, OS_STRING(pszName));
	}
	return output;
}

void * LoadDialogBoxTemplate(HINSTANCE hInst, LPCSTR pszName, HGLOBAL * pHMem)
{
	*pHMem = NULL;
	
	HRSRC hRsrc = FindResource(hInst, OS_STRING(pszName),RT_DIALOG);

	if (hRsrc)
	{
		*pHMem = LoadResource(hInst,hRsrc);
		return LockResource(*pHMem);
	}

	return NULL;
}

void * HXLoadDialogBoxTemplate(LPCSTR pszName, HGLOBAL * pHMem)
{
	HXBOOL	bLoaded = FALSE;
	void *  pTemplate = NULL;
	*pHMem = NULL;

	HXCheckLocale();

	HINSTANCE hTempDLL;
	// First Check in Language DLLs
	for (int ndx = 0; ndx < g_nLangDLLs; ndx++)
	{
		if (g_hLangDLLs[ndx])
		{
			hTempDLL = g_hLangDLLs[ndx];
			pTemplate = LoadDialogBoxTemplate(hTempDLL, pszName, pHMem);
			if (pTemplate)
			{
				bLoaded = TRUE;
				break; // for
				
			} // if output
			
		} // if some handle
		
	} // for
	if (!bLoaded)
	{
		pTemplate = LoadDialogBoxTemplate(g_hInstBase, pszName, pHMem);
	}
	return pTemplate;
}

int HXDialogBox(LPCSTR pszName, HWND hwndOwner, DLGPROC dlgprc)
{
	int	output = 0;                                                   
	HGLOBAL hMem;
#ifdef _WIN32
	LPDLGTEMPLATE pDlgTemp = (LPDLGTEMPLATE)HXLoadDialogBoxTemplate(pszName, &hMem);
#else
	void * pDlgTemp = HXLoadDialogBoxTemplate(pszName, &hMem);
#endif

	if (pDlgTemp && hMem)
	{
#ifdef _WIN32	
		output = DialogBoxIndirect(g_hInstBase, pDlgTemp, hwndOwner, dlgprc);
#else		                                                                 
      	output = DialogBoxIndirect(g_hInstBase, hMem, hwndOwner, dlgprc);
		UnlockResource(hMem);
		FreeResource(hMem);
#endif        		
	}
	return output;
}

int HXDialogBoxParam(LPCSTR pszName, HWND hwndOwner, DLGPROC dlgprc, LPARAM lParamInit)
{
	int	output = 0;   
	HGLOBAL hMem;
#ifdef _WIN32
	LPDLGTEMPLATE pDlgTemp = (LPDLGTEMPLATE)HXLoadDialogBoxTemplate(pszName, &hMem);
#else
	void * pDlgTemp = HXLoadDialogBoxTemplate(pszName, &hMem);
#endif

	if (pDlgTemp && hMem)
	{
#ifdef _WIN32	
		output = DialogBoxIndirectParam(g_hInstBase, pDlgTemp, hwndOwner, dlgprc, lParamInit);
#else		                                                                 
      	output = DialogBoxIndirectParam(g_hInstBase, hMem, hwndOwner, dlgprc, lParamInit);
		UnlockResource(hMem);
		FreeResource(hMem);
#endif        		
	}
	return output;
}

HWND HXCreateDialog(LPCSTR pszName, HWND hwndOwner, DLGPROC dlgprc)
{
	HWND output = 0;  
	HGLOBAL hMem;
#ifdef _WIN32
	LPDLGTEMPLATE pDlgTemp = (LPDLGTEMPLATE)HXLoadDialogBoxTemplate(pszName, &hMem);
#else
	void * pDlgTemp = HXLoadDialogBoxTemplate(pszName, &hMem);
#endif


	if (pDlgTemp && hMem)
	{
		output = CreateDialogIndirect(g_hInstBase, pDlgTemp, hwndOwner, dlgprc);
#ifdef _WIN16
                UnlockResource(hMem);
		FreeResource(hMem);
#endif
	}
	return output;
}

HWND HXCreateDialogParam(LPCSTR pszName, HWND hwndOwner, DLGPROC dlgprc, LPARAM lParamInit)
{
	HWND output = 0;
	HGLOBAL hMem;
#ifdef _WIN32
	LPDLGTEMPLATE pDlgTemp = (LPDLGTEMPLATE)HXLoadDialogBoxTemplate(pszName, &hMem);
#else
	void * pDlgTemp = HXLoadDialogBoxTemplate(pszName, &hMem);
#endif

	if (pDlgTemp && hMem)
	{
		output = CreateDialogIndirectParam(g_hInstBase, pDlgTemp, hwndOwner, dlgprc, lParamInit);
#ifdef _WIN16
		UnlockResource(hMem);
		FreeResource(hMem);
#endif
	}
	return output;
}

void HXResetWindowText(HWND hwnd, UINT idResource)
{
	char szText[256]; /* Flawfinder: ignore */
	int nCharCount = HXLoadString(idResource,szText,255);

	if (nCharCount > 0)
	{
		// Try and set the new text
		SetWindowText(hwnd, OS_STRING(szText));
	}
	
}

#ifndef WIN32_PLATFORM_PSPC

void HXResetWindowMenu(HWND hwnd, LPCSTR pszName)
{
	HMENU hNewMenu = HXLoadMenu(pszName);
	HMENU hOldMenu = GetMenu(hwnd);

	// Try and set the new menu
	if (SetMenu(hwnd,hNewMenu))
	{
		hNewMenu = NULL;
	}
	
	// If we succesfully set the new menu than it
	// will be NULL, otherwise we need to clean it
	// up correctly.
	if (hNewMenu)
	{
		DestroyMenu(hNewMenu);
	}
	
	// Normally we got the old menu and we need to
	// clean it up. Otherwise we don't need to do
	// anything.
	if (hOldMenu)
	{
		DestroyMenu(hOldMenu);
	}
	
}

HMENU HXLoadPopupMenu(LPCSTR pMenuName)
{
	HMENU hPopup = NULL;
	HMENU hMenu = HXLoadMenu(pMenuName);

	if (hMenu)
	{
		hPopup = CreatePopupMenu();
		int nItemCount = GetMenuItemCount(hMenu);

		for (int nItem = 0; nItem < nItemCount; nItem++)
		{
			char buffer[256]; /* Flawfinder: ignore */
			UINT unID = GetMenuItemID(hMenu,nItem);
			UINT unState = GetMenuState(hMenu,nItem,MF_BYPOSITION);
			GetMenuString(hMenu,nItem,buffer,sizeof(buffer)-1,MF_BYPOSITION);
			AppendMenu(hPopup,unState | MF_STRING,unID,buffer);
   		}
	}
	return hPopup;
}

HFONT HXCreateFont (int nHeight,					
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
					LPCSTR lpszFace)
{
	HINSTANCE hTempDLL;
	HFONT rtnFont;
	CREATE_FONT Create_Font_Func = NULL;
	HXBOOL	bLoaded = FALSE;
	int		output = 0;

	HXCheckLocale();
	
	// First Check in Language DLLs
	for (int ndx = 0; ndx < g_nLangDLLs; ndx++)
	{
		if (g_hLangDLLs[ndx])
		{
			hTempDLL = g_hLangDLLs[ndx];
			Create_Font_Func = (CREATE_FONT)GetProcAddress((HINSTANCE)hTempDLL, OS_STRING(Name_Create_Font));  
			if (Create_Font_Func)
			{
				bLoaded = TRUE;
				rtnFont = (* Create_Font_Func) (nHeight, nWidth, nEscapement,
												nOrientation, fnWeight, fdwItalic,
												fdwUnderline, fdwStrikeOut, fdwCharSet,
												fdwOutputPrecision, fdwClipPrecision,
												fdwQuality, fdwPitchAndFamily,
												lpszFace);
				break; // for				
			} // if output
			
		} // if some handle
		
	} // for

	if (!bLoaded)
	{
		// NOTE: Win32 and Win16 versions take different sized parameters!
		rtnFont = CreateFont(nHeight, nWidth, nEscapement, nOrientation, fnWeight, 
#ifdef _WIN32
							fdwItalic, fdwUnderline, fdwStrikeOut, fdwCharSet, 
							fdwOutputPrecision, fdwClipPrecision, fdwQuality, 
							fdwPitchAndFamily,
#else
							(BYTE)fdwItalic, (BYTE)fdwUnderline, (BYTE)fdwStrikeOut, 
							(BYTE)fdwCharSet, (BYTE)fdwOutputPrecision, (BYTE)fdwClipPrecision,
							(BYTE)fdwQuality, (BYTE)fdwPitchAndFamily,
#endif
							lpszFace);
	}
	return rtnFont;

}

HFONT HXGetStockFont ( int stockFontId )
{
	HINSTANCE hTempDLL;
	HFONT rtnFont;
	LOAD_FONT Load_Font_Func = NULL;
	HXBOOL	bLoaded = FALSE;
	int		output = 0;

	HXCheckLocale();
	

	// First Check in Language DLLs
	for (int ndx = 0; ndx < g_nLangDLLs; ndx++)
	{
		if (g_hLangDLLs[ndx])
		{
			hTempDLL = g_hLangDLLs[ndx];
		    Load_Font_Func = (LOAD_FONT)GetProcAddress((HINSTANCE)hTempDLL, OS_STRING(Name_Load_Font));  
			if (Load_Font_Func)
			{
				bLoaded = TRUE;
				rtnFont = (* Load_Font_Func) ( stockFontId );
				break;
			}
		}
	}

	if (!bLoaded)
	{
		return (HFONT) GetStockObject ( stockFontId );
	}

	return rtnFont;
}

#endif

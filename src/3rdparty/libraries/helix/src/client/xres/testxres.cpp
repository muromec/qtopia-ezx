/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: testxres.cpp,v 1.8 2007/07/06 21:58:51 jfinnecy Exp $
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

#ifndef _UNIX
#include "hlxclib/windows.h"
#endif

#include "hxtypes.h"
#include "hxresult.h"
#include "pnpeff.h"
#include "dllacces.h"
#include "dllpath.h"
#include "hxslist.h"
#include "pnxres.h"
#include "pnxbmp.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/string.h"
#include "hlxclib/stdio.h"
#include "chxdataf.h"

#include "hxassert.h"
#include "pnxdlog.h"
#include "pnxmenu.h"
#include "pnxmbar.h"
#include "pnxcrsr.h"
#include "pnxicon.h"

ENABLE_DLLACCESS_PATHS(Testxres);

#ifdef _MACINTOSH
#include <console.h>
#include "macfd.h"
#include "resource.h"


void    AddSubMenu(MenuHandle   menu, CHXXMenu* pMenu,UINT16*  index);

#endif


#ifdef _UNIX
#include <fcntl.h>	// for O_CREAT, O_WRONLY, etc.
#include <Xm/PushB.h>
#include "xpm.h"
//#include <X11/xpm.h>
#endif

ULONG32         gSubMenuID=2;
ULONG32         gSubLevel=1;

typedef HX_RESULT (HXEXPORT_PTR FPCREATEINSTANCE) (IUnknown** /*OUT*/ ppIUnknown);

HXBOOL OpenLibPNCreateInstance(char* dllName, UINT32 uDLLNameLen, const char* szShortName,const char* szLongName,DLLAccess& lib,IUnknown** pObj)
{
    
    DLLAccess::CreateName(szShortName,szLongName, dllName, uDLLNameLen);
    if (DLLAccess::DLL_OK != lib.open(dllName))
	return(FALSE);
    FPCREATEINSTANCE fpCreateInstance = (FPCREATEINSTANCE)lib.getSymbol("HXCreateInstance");
    fpCreateInstance(pObj);

    HX_ASSERT(*pObj);
    if (!*pObj)
	return FALSE;

    return TRUE;
}



extern "C" int main (int argc,char** argv)

{
  IHXXResource*  rsrc;

#ifdef _MACINTOSH

	InitGraf(&qd.thePort);
	InitFonts();
	FlushEvents(everyEvent, 0);
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(nil);
	InitCursor();
	DrawMenuBar();  

	argc = ccommand(&argv);
#endif




	DLLAccess           PNXResLib;
	
//
//      SIMPLE TEST
//

	

	
	IHXXResFile*           theFile=NULL;
	UINT16          ID;

    // used for DLL name creation
    const UINT32 MAX_DLL_NAME_LEN = 256;
    char dllName[MAX_DLL_NAME_LEN]; /* Flawfinder: ignore */
    UINT32 uDLLNameLen = MAX_DLL_NAME_LEN;

	if (!OpenLibPNCreateInstance(dllName,uDLLNameLen,"pnxr","pnxres",PNXResLib,(IUnknown**)&theFile))
	{
	
		printf("ERROR: Could not find %s\n",dllName);
		return 0;
	
	}

	

	if (strcmp(argv[1],"CACHETEST")==0)
	{

		if (HXR_OK != theFile->Open(argv[2]))
		{
			printf("ERROR: file could not be opened.\n");
		}
		
		for (int x=101; x<=115; x++)
		{

			if (HXR_OK==theFile->GetResource(HX_RT_BITMAP,x,&rsrc))
			{
				rsrc->Release();
			}

		}

		theFile->Close();

		return 0;
	}

	if (strcmp(argv[1],"DIALOG")==0)
	{
		if (HXR_OK != theFile->Open(argv[2]))
		{
			printf("ERROR: File could not be openend.\n");
		}

		ULONG32 id=atoi(argv[3]);

		IHXXResource*  rsrc;

		if (HXR_OK == theFile->GetResource(HX_RT_DIALOG,id,&rsrc))
		{
			CHXXDialog*     theDialog=CHXXDialog::create(rsrc);
			
			rsrc->Release();
			
			//
			//      Draw out the things in the dialog, into a Window so that we can see how
			//      it is layed out.
			//
			
#ifdef _MACINTOSH
		
			HX_DialogBoxHeader*       dlgh;
			HX_ControlData*           c=NULL;
			short                             index=0;
		
			GrafPtr         saveport; GetPort(&saveport);
			
			HX_RESULT       GetDialogHeader(HX_DialogBoxHeader**            h);
			
			theDialog->GetDialogHeader(&dlgh);
			
			Rect            wRect={50,50,dlgh->cy+50,dlgh->cx+50};
				
			WindowPtr       myWind=NewWindow(NULL,&wRect,"\pStringParsing",TRUE,2,(WindowPtr)-1L, FALSE,NULL); 
			
			ShowWindow(myWind);
			SetPort(myWind);
			
			//
			//      Draw my dialog's items
			//
			
			while (HXR_OK==theDialog->GetNthControl(&c,index))
			{
				Rect  iRect={c->y,c->x,c->cy+c->y,c->cx+c->x};
				
				FrameRect(&iRect);
				
				index++;
			}
			
			while (!Button())
			{
				EventRecord     theEvent;
			
				WaitNextEvent(everyEvent,&theEvent,0xFF,NULL);
			}
			
			DisposeWindow(myWind);
			
			SetPort(saveport);
#endif  
			
		}
		else
		{
			printf("ERROR: Dialog resource %d not found.",id);
		}


		return 0;
	}


//
//      Test the ability to display a bitmap.
//

	if (strcmp(argv[1],"BITMAP")==0)
	{
		if (HXR_OK != theFile->Open(argv[2]))
		{
			printf("ERROR: File could not be openend.\n");
		}

		ULONG32 id=atoi(argv[3]);

		IHXXResource*  rsrc=NULL;

		if (HXR_OK == theFile->GetResource(HX_RT_BITMAP,id,&rsrc))
		{
			
#ifdef _MACINTOSH
		
			short                             index=0;
		
			GrafPtr         saveport; GetPort(&saveport);
			
			Rect            wRect={50,50,600,800};
				
			WindowPtr       myWind=NewWindow(NULL,&wRect,"\pStringParsing",TRUE,2,(WindowPtr)-1L, FALSE,NULL); 
			
			ShowWindow(myWind);
			SetPort(myWind);
			
		
			//
			//      Okay make a CHXXBmp object, and feed the resource data we got into it.
			//              
	
			CHXXBitmap*        bmp;
			
			bmp=CHXXBitmap::create(rsrc);
			
			rsrc->Release();
			
			if (bmp)
			{
			
				Rect*   rect=*bmp;
			
				PixMapHandle thePixMap = GetGWorldPixMap( ( GWorldPtr ) *bmp );
				CopyBits((const BitMap*)*thePixMap,(const BitMap*)&myWind->portBits,*bmp,*bmp,srcCopy,NULL);
				ValidRect((Rect*)bmp);
			}
	
	
			
			while (!Button())
			{
				EventRecord     theEvent;
			
				WaitNextEvent(everyEvent,&theEvent,0xFF,NULL);
			}
			
			DisposeWindow(myWind);
			
			SetPort(saveport);
			
			bmp->Release();
#endif  


#ifdef _UNIX

		//
		//      Generate an XPM file.
		//                      

		CHXXBitmap* bmp;
		bmp=CHXXBitmap::create(rsrc);
		
		rsrc->Release();
		
		CHXDataFile*    outputfile;
		
		outputfile=CHXDataFile::Construct();
		
		if (outputfile)
		{               
			printf ("Width=%d Height=%d\nconverteddata=0x%x converteddatasize=%d\ncommondata=0x%x commondatasize=%d\n",
				bmp->m_Width,
				bmp->m_Height,
				bmp->m_ConvertedData,
				bmp->m_ConvertedDataSize,
				bmp->m_CommonData,
				bmp->m_CommonDataSize);

			outputfile->Open(argv[4],O_RDWR|O_CREAT);
			
			//outputfile->Write((char*)*bmp,strlen(*bmp));
			outputfile->Write((char*)(bmp->m_ConvertedData),bmp->m_ConvertedDataSize);
			outputfile->Close();
			
			delete outputfile;
		}
		else
		{
			printf("Could not create file object.\r");
		}


#if 1
		XtAppContext app;
		Widget top, button;
		int status;
		Pixmap pixmap;
		XpmImage image;
		top = XtAppInitialize(&app, "TestButton", NULL, 0, &argc, argv,
				NULL, NULL, 0);
		button = XmCreatePushButton(top, "button", NULL, 0);
		status = XpmCreateXpmImageFromData ((CHAR **)(bmp->m_ConvertedData),
				&image, NULL);
		status = XpmCreatePixmapFromXpmImage (XtDisplay(top),
				XRootWindowOfScreen(XtScreen(top)),
				&image, &pixmap, NULL, NULL);
		if (status != XpmSuccess)
		{
			fprintf (stderr, "XpmError:  %s\n", XpmGetErrorString(status));
			exit(1);
		}
		XtVaSetValues(button,
				XmNlabelType, XmPIXMAP,
				XmNlabelPixmap, pixmap,
				NULL);
		XtManageChild(button);
		XtRealizeWidget(top);
		XtAppMainLoop(app);
#endif


		delete bmp;

#endif

			
		}
		else
		{
			printf("ERROR: Dialog resource %d not found.",id);
		}

		return 0;
	}


if (strcmp(argv[1],"BITMAPFILE")==0)
{
	{

		ULONG32 id=atoi(argv[3]);

			
#ifdef _MACINTOSH


			c2pstr(argv[2]);
		
			FSSpec  theFileSpec;
			short   fileRef=0;
			long    eof=0;
		
			FSMakeFSSpec(0,0,(StringPtr)argv[2],&theFileSpec);
			
			FSpOpenDF(&theFileSpec,fsRdPerm,&fileRef);
			GetEOF(fileRef,&eof);
			
			BYTE*   buffer=new BYTE[eof];
			BYTE*   p=buffer+14;
			
			HX_ASSERT(buffer);
			
			FSRead(fileRef,&eof,buffer);
			
			FSClose(fileRef);

		
			short                             index=0;
		
			GrafPtr         saveport; GetPort(&saveport);
			
			Rect            wRect={50,50,600,800};
				
			WindowPtr       myWind=NewWindow(NULL,&wRect,"\pStringParsing",TRUE,2,(WindowPtr)-1L, FALSE,NULL); 
			
			ShowWindow(myWind);
			SetPort(myWind);
			
		
			//
			//      Okay make a CHXXBmp object, and feed the resource data we got into it.
			//              
	
			CHXXBitmap*        bmp;
			
			bmp=CHXXBitmap::create((HX_BITMAPINFO*)p);
			
			
			if (bmp)
			{
			
				Rect*   rect=*bmp;
			
				PixMapHandle thePixMap = GetGWorldPixMap( ( GWorldPtr ) *bmp );
				CopyBits((const BitMap*)*thePixMap,(const BitMap*)&myWind->portBits,*bmp,*bmp,srcCopy,NULL);
				ValidRect((Rect*)bmp);
			}
	
	
			
			while (!Button())
			{
				EventRecord     theEvent;
			
				WaitNextEvent(everyEvent,&theEvent,0xFF,NULL);
			}
			
			DisposeWindow(myWind);
			
			SetPort(saveport);
			
			bmp->Release();
#endif  


#ifdef _UNIX

		//
		//      Generate an XPM file.
		//                      

		CHXXBitmap* bmp;
		bmp=CHXXBitmap::create(rsrc);
		
		rsrc->Release();
		
		CHXDataFile*    outputfile;
		
		outputfile=CHXDataFile::Construct();
		
		if (outputfile)
		{               
			outputfile->Create(argv[4],O_CREAT);
			outputfile->Open(argv[4],O_WRONLY);
			
			outputfile->Write((char*)*bmp,strlen(*bmp));
			outputfile->Close();
			
			delete outputfile;
		}
		else
		{
			printf("Could not create file object.\r");
		}
		
		delete bmp;

#endif

			
		}

		return 0;
	}



if (strcmp(argv[1],"ICON")==0)
{

	if (HXR_OK != theFile->Open(argv[2]))
	{
		printf("ERROR: file could not be opened.\n");
	}
	else
	{

		ULONG32 id=atoi(argv[3]);

			
#ifdef _MACINTOSH		
		
			short                             index=0;
		
			GrafPtr         saveport; GetPort(&saveport);
			
			Rect            wRect={50,50,600,800};
				
			WindowPtr       myWind=NewWindow(NULL,&wRect,"\pStringParsing",TRUE,2,(WindowPtr)-1L, FALSE,NULL); 
			
			ShowWindow(myWind);
			SetPort(myWind);
			
			//
			//	Load the icon first
			//
			
			IHXXResource*	   iconresource;
			
			if (HXR_OK==theFile->GetResource(HX_RT_GROUPICON,id,&rsrc))
			{
				
				//
				//	Create an Icon parser object
				//
				
				CHXXIcon*		   icon;
				icon=CHXXIcon::create(rsrc);

#if 0 /* needs to be re-worked with new icon drawing code */
				if (icon)
				{
					
					CIconHandle        macicon;
					
					
					macicon=icon->GetIconHandle();
					
					Rect	aRect={0,0,31,31};
					FillRect(&aRect,&qd.ltGray);
					PlotCIcon(&aRect,macicon);
					
					aRect.top+=32;
					aRect.bottom+=32;
					FillRect(&aRect,&qd.gray);
					PlotCIcon(&aRect,macicon);
	
					aRect.top+=32;
					aRect.bottom+=32;
					FillRect(&aRect,&qd.dkGray);
					PlotCIcon(&aRect,macicon);	
					
					aRect.top+=32;
					aRect.bottom+=32;
					FillRect(&aRect,&qd.white);
					PlotCIcon(&aRect,macicon);					


					aRect.top+=32;
					aRect.bottom+=32;
					FillRect(&aRect,&qd.black);
					PlotCIcon(&aRect,macicon);					


										
					while (!Button())
					{
						EventRecord     theEvent;
					
						WaitNextEvent(everyEvent,&theEvent,0xFF,NULL);
					}
					
					HX_DELETE(icon);
					
				}	
#endif //		
				HX_RELEASE(rsrc);

			}			
			DisposeWindow(myWind);
			
			SetPort(saveport);
			
#endif  

			
		}

		return 0;
	}



//
//      Menu test
//
	
	
	if (strcmp(argv[1],"MENU")==0)
	{
		if (HXR_OK != theFile->Open(argv[2]))
		{
			printf("ERROR: File could not be openend.\n");
		}

		ULONG32 id=atoi(argv[3]);

		IHXXResource*  rsrc=NULL;


		if (HXR_OK == theFile->GetResource(HX_RT_MENU,id,&rsrc))
		{               
			
#ifdef _MACINTOSH

			CHXXMenu*        pMenu=CHXXMenu::create(rsrc);
			
			rsrc->Release();
			
			if (!pMenu)
			{
				return;
			}
			
			UINT16                          menuid=atoi(argv[3]);
			UINT16                          index=0;
			MenuHandle                      mainmenu;
			MenuHandle                      submenus[10];
			HX_MenuItem*            mi=NULL;
			ConstStr255Param        pstring;
			UINT16                          curitem=1;
			
			memset(submenus,0,sizeof(submenus));
			
			if (HXR_OK==pMenu->GetNthMenuItem(&mi,0))
			{
				CHXString       thestring;
				thestring=mi->szItemText;
				pstring=thestring;
			
				mainmenu=NewMenu(menuid,pstring);
				
				index=1;
				
				while (HXR_OK==pMenu->GetNthMenuItem(&mi,index))
				{
					pstring=mi->szItemText;
					
					
					if (!IS_POPUP_MENUITEM(mi))
					{
						if (IS_SEPARATOR_MENUITEM(mi))
						{
							AppendMenu(mainmenu,"\p-");
						}
											
						AppendMenu(mainmenu,pstring);
						
						if (IS_DISABLED_MENUITEM(mi))
						{
							DisableItem(mainmenu,curitem);                                                  
						}
						
						if (IS_CHECKED_MENUITEM(mi))
						{
							CheckItem(mainmenu,curitem,TRUE);                                                       
						}                                               

						if (mi->wKey && (mi->fKeyFlags & HX_FCONTROL))
						{
							SetItemCmd(mainmenu,curitem,mi->wKey);
						}       

					}
					else
					{
						AddSubMenu(mainmenu,pMenu,&index);      
					}
					
					index++;
					curitem++;
				}
				
				
				InsertMenu(mainmenu,0);
				
				EventRecord     theEvent;
				
				
				DrawMenuBar();                          
				
				while (true)
				{
					if (WaitNextEvent(everyEvent,&theEvent,0xFF,NULL))
					{
						if (theEvent.what==autoKey)
						{
							break;
						}
						
						if (theEvent.what==mouseDown)
						{
							WindowPtr       whichWin;
						
							short part = FindWindow(theEvent.where,&whichWin);
							
							if (part == inMenuBar)
							{
								MenuSelect(theEvent.where);
							}
							
						}       
					
					}
					
				}
				
			}

#endif
		


#if 0
//#ifdef _UNIX

			CHXXMenu*        pMenu=CHXXMenu::create(rsrc);
			HX_MenuItem*            mi=NULL;
			if (HXR_OK==pMenu->GetNthMenuItem(&mi,0))
			printf ("Menu: 0x%x\nItem: %s %s %s\n", pMenu, mi->szItemText, mi->szAcceleratorText, mi->szStatusText);

#endif




		}
	}
	
	
	if (strcmp(argv[1],"MENUBAR")==0)
	{

		if (HXR_OK != theFile->Open(argv[2]))
		{
			printf("ERROR: file could not be opened.\n");
		}
		
		ULONG32 id=atoi(argv[3]);

		IHXXResource*  rsrc=NULL;


		if (HXR_OK == theFile->GetResource(HX_RT_MENU,id,&rsrc))
		{               

#ifdef _MACINTOSH
			CHXXMenuBar*    menubar=CHXXMenuBar::create(rsrc);
			
			ULONG32 index=0;
			CHXXMenu*               menu=NULL;
			
			gSubLevel=0;
			
			while (HXR_OK==menubar->GetMenuByIndex(&menu,index))
			{
			
				gSubLevel=0;
			
				UINT16  itemindex=0;
				
				AddSubMenu(NULL,menu,&itemindex);
				
				index++;
			}
			
			
			EventRecord     theEvent;
			
			DrawMenuBar();                          
			
			while (true)
			{
				if (WaitNextEvent(everyEvent,&theEvent,0xFF,NULL))
				{
					if (theEvent.what==autoKey)
					{
						break;
					}
					
					if (theEvent.what==mouseDown)
					{
						WindowPtr       whichWin;
					
						short part = FindWindow(theEvent.where,&whichWin);
						
						if (part == inMenuBar)
						{
							MenuSelect(theEvent.where);
						}
						
					}       
				
				}
				
			}
		
#endif                  
			



#if 0
//#ifdef _UNIX

            CHXXMenuBar*    menubar=CHXXMenuBar::create(rsrc);
#endif                  



		}

		theFile->Close();

		return 0;
	}       
	

	if (strcmp(argv[1],"CURSOR")==0)
	{

		if (HXR_OK != theFile->Open(argv[2]))
		{
			printf("ERROR: file could not be opened.\n");
		}
		
		ULONG32 id=atoi(argv[3]);

		IHXXResource*  rsrc=NULL;


		if (HXR_OK == theFile->GetResource(HX_RT_GROUPCURSOR,id,&rsrc))
		{               

			CHXXCursor*             theCursor=CHXXCursor::create(rsrc);
#ifdef _MACINTOSH                
			HX_ASSERT(theCursor);
		
			if (theCursor)
			{
				SetCursor(*theCursor);
			}
		
			while (!Button())
			{
				EventRecord     theEvent;
			
				WaitNextEvent(everyEvent,&theEvent,0xFF,NULL);
			}
#endif                
			
		}
		
		

		theFile->Close();

		return 0;
	}       

	if (strcmp(argv[1],"STRING")==0)
	{

		if (HXR_OK != theFile->Open(argv[2]))
		{
			printf("ERROR: File could not be opened.\n");
		}

		IHXXResource*   res;

		ID= atoi(argv[3]);
		
		res=theFile->GetString(ID);
		
		if (res)
		{
			printf("%s",(char*)res->ResourceData());
		
			res->Release();
		}

		theFile->Close();

		return 0;
	}
	return 0;
}


#ifdef _MACINTOSH

	void    AddSubMenu(MenuHandle   menu, CHXXMenu* pMenu,UINT16*  index)
	{
	
		HX_MenuItem*    mi=NULL;
		
		
		pMenu->GetNthMenuItem(&mi,*index);

		MenuHandle      submenu=NewMenu(gSubMenuID++,mi->szItemText);
		ULONG32         curitem=1;
		
		if (menu)
		{
			InsertMenu  (submenu,-1);
			
			AppendMenu(menu,mi->szItemText);
			UINT16  thisitem=CountMenuItems(menu);
			SetItemCmd(menu,thisitem,hMenuCmd);
			SetItemMark(menu,thisitem,(char)(gSubMenuID-1));
		}
		else
		{
			InsertMenu (submenu,0);
		}               
	
		gSubLevel++;    
		
		(*index)++;
		while (HXR_OK==pMenu->GetNthMenuItem(&mi,*index))
		{
		
			if (IS_SEPARATOR_MENUITEM(mi))
			{
				AppendMenu(submenu,"\p-");
			}
		
		
			if (mi->wSubLevel < gSubLevel)
			{
				(*index)--;
				gSubLevel--;
				return;
			}
				
			
			if (IS_POPUP_MENUITEM(mi))
			{
				AddSubMenu(submenu,pMenu,index);        
			}
			else
			{
				AppendMenu(submenu,mi->szItemText);     
				
				if (IS_DISABLED_MENUITEM(mi))
				{
					DisableItem(submenu,curitem);                                                   
				}
				
				if (IS_CHECKED_MENUITEM(mi))
				{
					CheckItem(submenu,curitem,TRUE);                                                        
				}
				
				if (mi->wKey && (mi->fKeyFlags & HX_FCONTROL))
				{
					SetItemCmd(submenu,curitem,mi->wKey);
				}               
				
			}
					
			curitem++;
			(*index)++;

			
		}
	}

#endif

/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: reg.cpp,v 1.4 2004/05/13 18:57:48 tmarshall Exp $ 
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

#include "hxtypes.h"
#include "hxcom.h"
#include "proc.h"
#include "hxreg.h"
#include "reg.h"
#include "chxpckts.h"
#include "hxstrutl.h"
#include "hxmon.h"	// for the property type
#include "servreg.h"
#include "server_version.h"
#include "servbuffer.h"
#include "hlxclib/windows.h"
#include <stdio.h>

WinRegistry::WinRegistry(Process* proc, ServerRegistry* registry)
{
    m_proc = proc;
    m_registry = registry;
    sprintf(RootKeyName, "Software\\RealNetworks\\%s\\%d.%d",
	    ServerVersion::ProductName(),
	    ServerVersion::MajorVersion(),
            ServerVersion::MinorVersion());
}

HX_RESULT
WinRegistry::Read(const char* key, const char* destkey)
{
    HKEY hrootkey;
    char pTopKeyName[1024];
    char pSubKeyName[1024];
    char pDstKeyName[1024];
    char pKeyName[1024];
    char pClassName[1024];
    LONG index;
    DWORD namelen;
    DWORD classlen;
    HXRegistry* hxreg = new HXRegistry(m_registry, m_proc);
    char* p;

    hxreg->AddRef();

    strcpy(pTopKeyName, RootKeyName);

    if(key)
    {
	strcat(pTopKeyName, "\\");
	strcat(pTopKeyName, key);
    }

    if(RegOpenKey(HKEY_CLASSES_ROOT, pTopKeyName, &hrootkey) == ERROR_SUCCESS)
    {
	sprintf(pSubKeyName, "%s", key);
	sprintf(pDstKeyName, "%s", destkey);
	for(p = pSubKeyName; *p; p++)
	    if(*p == '\\')
		*p = '.';
	for(p = pDstKeyName; *p; p++)
	    if(*p == '\\')
		*p = '.';
	if(m_registry->GetType(pSubKeyName, m_proc) != PT_COMPOSITE)
	{
	    m_registry->AddComp(pDstKeyName, m_proc);
	}
	namelen = 1024;
	classlen = 1024;
	index = 0;
	while(RegEnumKeyEx(hrootkey,
			   index++,
			   pKeyName,
			   &namelen,
			   0,
			   pClassName,
			   &classlen,
			   NULL) == ERROR_SUCCESS)
	{
	    HKEY hsubkey;
	    DWORD keytype;
	    DWORD datasize;

	    sprintf(pSubKeyName, "%s\\%s", pTopKeyName, pKeyName);
	    if(RegOpenKey(HKEY_CLASSES_ROOT, 
			  pSubKeyName, 
			  &hsubkey) == ERROR_SUCCESS)
	    {
		/*
		 * Check first to see if this key has subkeys.
		 * If it does, handle the subkeys. -paulm
		 */
		DWORD subKeys;
		if(RegQueryInfoKey(hsubkey,
				   NULL, NULL,
				   NULL,
				   &subKeys,
				   NULL,
				   NULL,
				   NULL,
				   NULL,
				   NULL,
				   NULL,
				   NULL) == ERROR_SUCCESS)
		{
		    if(subKeys > 0)
		    {
			sprintf(pSubKeyName, "%s\\%s", key, pKeyName);
			sprintf(pDstKeyName, "%s\\%s", destkey, pKeyName);
			Read(pSubKeyName, pDstKeyName);
		    }
		}

		/*
		 * Now get the data of the key.
		 */
		if(RegQueryValueEx(hsubkey,
				   "",
				   NULL,
				   &keytype,
				   NULL,
				   &datasize) == ERROR_SUCCESS)
		{
		    IHXBuffer* pBuffer = new ServerBuffer(TRUE);
		    pBuffer->SetSize(datasize);
		    RegQueryValueEx(hsubkey,
				    "",
				    NULL,
				    &keytype,
				    pBuffer->GetBuffer(),
				    &datasize);

		    /*
		     *  This condition will only make a difference in
		     *  win95.  In win95, if there is no value set for
		     *  a key, then RegQueryValueEx will return a
		     *  datasize of 1 and a buffer of "\0".  If this key
		     *  has subkeys && datasize of 1 && buffer of "\0",
		     *  assume it was just a COMPOSITE with no value. -paulm
		     */
		    if(!subKeys || 
			datasize != 1 || 
			*(pBuffer->GetBuffer()) != 0)
		    {
			sprintf(pSubKeyName, "%s\\%s", key, pKeyName);
			sprintf(pDstKeyName, "%s\\%s", destkey, pKeyName);
			char* p;
			for(p = pDstKeyName; *p; p++)
			    if(*p == '\\')
				*p = '.';
			for(p = (char*)pBuffer->GetBuffer(); *p && isdigit(*p); p++)
			    ;
			HXPropType val_type = m_registry->GetType(pDstKeyName, m_proc);
			if(*p == 0 && 
			   ((val_type == PT_UNKNOWN) || 
			    (val_type == PT_INTEGER)))
			{
			    
			    if(val_type == PT_UNKNOWN)
			    {
				m_registry->AddInt(pDstKeyName,
						   atol((char*)pBuffer->GetBuffer()),
						   m_proc);
			    }
			    else
			    {
				m_registry->SetInt(pDstKeyName,
						   atol((char*)pBuffer->GetBuffer()),
						   m_proc);
			    }
			}
			else
			{
			    if(val_type == PT_UNKNOWN)
			    {
				m_registry->AddStr(pDstKeyName, pBuffer, m_proc);
			    }
			    else
			    {
				m_registry->SetStr(pDstKeyName, pBuffer, m_proc);
			    }
			}
		    }
		    pBuffer->Release();
		}
		RegCloseKey(hsubkey);
		
	    }
	    classlen = namelen = 1024;
	}
	RegCloseKey(hrootkey);
    }
    else
    {
	return HXR_FAIL;
    }

    return HXR_OK;
}

HX_RESULT
WinRegistry::Import(const char* name,
		    const char* key,
		    const char* path,
		    HXRegistry* hxreg)
{
    IHXValues* pValues;
    const char* propName;
    HX_RESULT res;
    UINT32 prop_id;
    char regname[1024];
    char regval[1024];
    HKEY hkey;

    // name and key are required
    HX_ASSERT(name && key);
    if (!name || !key)
    {
	return HXR_FAIL;
    }

    if(!hxreg)
    {
	hxreg = new HXRegistry(m_registry, m_proc);
    }
    hxreg->AddRef();
    
    if(!path)
    {
	path = RootKeyName;
    }

    if(HXR_OK != hxreg->GetPropListByName(name, pValues))
	return HXR_FAIL;

    // We only create the lowest key here. This code does not
    // handle the case when key == "x.y.z". In this case, only
    // the z key is created and values are placed under it,
    // effectively ignoring the "x.y" keys under which z should
    // be nested. -XXXDPS
    const char* point = strrchr(key, '.');
    if(!point)
    {
	point = key;
    }
    else
    {
	point++;
    }

    if(strcasecmp(point, "XXXBADLIST") == 0)
	return HXR_FAIL;
    
    sprintf(regname, "%s\\%s", path, point);
    if(RegCreateKeyEx(HKEY_CLASSES_ROOT, 
		      regname, 
		      NULL,
		      NULL,
		      REG_OPTION_NON_VOLATILE,
		      KEY_ALL_ACCESS,
		      NULL,
		      &hkey,
		      NULL) == ERROR_SUCCESS)
    {
	RegCloseKey(hkey);
    }
    
    res = pValues->GetFirstPropertyULONG32(propName, prop_id);
    while(res == HXR_OK)
    {
	HXPropType type = hxreg->GetTypeById(prop_id);
	if(type != PT_COMPOSITE)
	{
	    const char* proppoint = strrchr(propName, '.');
	    if(!proppoint)
		proppoint = propName;
	    else
		proppoint++;

	    sprintf(regname, "%s\\%s\\%s", path, point, proppoint);
	    if(RegCreateKeyEx(HKEY_CLASSES_ROOT, 
			      regname, 
			      NULL,
			      NULL,
			      REG_OPTION_NON_VOLATILE,
			      KEY_ALL_ACCESS,
			      NULL,
			      &hkey,
			      NULL) == ERROR_SUCCESS)
	    {
	    }
	}

	// Initialize a blank value, in case we can't find one
	sprintf(regval, "");

	switch(type)
	{
	    case PT_COMPOSITE:
	    {
		sprintf(regname, "%s\\%s", path, point);
		Write(propName, regname, hxreg);
		break;
	    }
	    case PT_INTEGER:
	    {
		INT32 val;
		if(HXR_OK == hxreg->GetIntById(prop_id, val))
		{		    
		    sprintf(regval, "%d", val);
		}
		break;
	    }
	    case PT_INTREF:
		break;
	    case PT_STRING:
	    {
		IHXBuffer* pBuffer;
		if(HXR_OK == hxreg->GetStrById(prop_id, pBuffer) && pBuffer)
		{
		    sprintf(regval, "%s", pBuffer->GetBuffer());
		    pBuffer->Release();
		}
		break;
	    }
	    case PT_BUFFER:
	    {
		IHXBuffer* pBuffer;
		hxreg->GetBufById(prop_id, pBuffer);
		if(pBuffer)
		{
		    sprintf(regval, "%s", pBuffer->GetBuffer());
		    pBuffer->Release();
		}
		break;
	    }
	    case PT_UNKNOWN:
	    default:
		break;
	}
	if(type != PT_COMPOSITE)
	{
	    RegSetValue(hkey,
			"",
			REG_SZ,
			regval,
			strlen(regval));
	    RegCloseKey(hkey);
	}

	res = pValues->GetNextPropertyULONG32(propName, prop_id);
    }
    pValues->Release();
    hxreg->Release();
    return HXR_OK;
}

HX_RESULT
WinRegistry::Write(const char* name,
		   const char* path,
		   HXRegistry* hxreg)
{
    return Import(name, name, path, hxreg);
}

HX_RESULT
WinRegistry::Nuke(const char* key)
{
    char pKeyname[1024];

    if (key)
    {
	// Construct full key name
	sprintf(pKeyname, "%s\\%s", RootKeyName, key);

	return Erase(pKeyname);
    }
    else
    {
	return Erase(RootKeyName);
    }
}


HX_RESULT
WinRegistry::Erase(const char keyname[])
{
    HKEY hKey;
    DWORD dwIndex = 0;
    DWORD dwRet;
    char subkeyname[256];
    char combinedkeyname[512];
    DWORD dwSubKeySize;
    int appendindex;
    FILETIME ft; //this is not used but the docs don't say it can
                 //be NULL.
    /*
     * Open the base key
     */
    if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_CLASSES_ROOT,
			keyname,
			0,
			KEY_ALL_ACCESS,
			&hKey))
    {
	return HXR_FAIL;
    }

    /*
     *  Enumerate subkeys and recursively call erase for each. Always
     *  ask for key of index 0.  When we delete key 0, key 1 becomes
     *  key 0...
     */
    strcpy(combinedkeyname, keyname);
    appendindex = strlen(combinedkeyname);
    dwSubKeySize = 256;
    dwRet = RegEnumKeyEx(hKey, dwIndex, subkeyname,
	&dwSubKeySize, 0, NULL, NULL, &ft);
    while(dwRet == ERROR_SUCCESS)
    {
	sprintf(&(combinedkeyname[appendindex]), "\\%s",
	    subkeyname);
	Erase(combinedkeyname);
	dwSubKeySize = 256;
	dwRet = RegEnumKeyEx(hKey, dwIndex, subkeyname,
	    &dwSubKeySize, 0, NULL, NULL, &ft);
    }

    /*
     *  Erase the current key.
     */
    RegCloseKey(hKey);
    if(ERROR_SUCCESS != RegDeleteKey(HKEY_CLASSES_ROOT, keyname))
    {
	return HXR_FAIL;
    }

    return HXR_OK;
}

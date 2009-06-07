/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: replacefn.cpp,v 1.4 2004/05/13 18:57:48 tmarshall Exp $ 
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
/*  replacefn.cpp
 *
 *  A function for replacing any imported function call on windows.
 */

#include "hlxclib/windows.h"

BOOL
ReplaceFunction(char* pszFunctionName, char* pszModuleName, void* fpFunction)
{
    // get the address of the function
    unsigned char* procMem;
    HINSTANCE inst = LoadLibrary(pszModuleName);
    if (inst)
        procMem = (unsigned char*)GetProcAddress(inst, pszFunctionName);
    else
	return FALSE;

    if (!procMem)
        return FALSE;

    // create the jump instruction (remember, jumps are relative)
    UCHAR buffer[5];
    buffer[0] = 0xE9;
    *((int*)(&buffer[1])) = (int)fpFunction - (int)procMem - 5;
    
    // now do the OS stuff to actually write the above value
    DWORD len = 0;
    if (!WriteProcessMemory(GetCurrentProcess(), procMem, buffer, 5, &len))
        return FALSE;

    return TRUE;
}



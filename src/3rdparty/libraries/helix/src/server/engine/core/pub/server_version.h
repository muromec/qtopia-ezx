/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: server_version.h,v 1.7 2006/10/11 03:03:17 tknox Exp $ 
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

#ifndef _SERVER_VERSION_H_
#define _SERVER_VERSION_H_

class ServerVersion
{
public:
    // names
    static const char* ProductName();
    static const char* FullProductName();
    static const char* ReleaseName();
    static const char* BuildName();
    static const char* ExecutableName();

    // version strings
    static const char* VersionString();
    static const char* MajorString();
    static const char* MinorString();
    static const char* MajorMinorString();

    // version numbers
    static const UINT32 MajorVersion();
    static const UINT32 MinorVersion();

    // build info
    static const char*  Platform();
    static const UINT32 BuildID();
    static const char*  BuildTimestamp();
    static const char*  BuildTag();
    static const UINT32 BuildTagID();
    static const char*  Branch();
};

#endif //_SERVER_VERSION_H_

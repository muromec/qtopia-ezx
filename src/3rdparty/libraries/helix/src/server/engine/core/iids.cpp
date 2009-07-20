/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: iids.cpp,v 1.4 2005/02/21 18:52:28 dcollins Exp $ 
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
/*
 *  Definition of RMA IIDs for this application
 */

// define all guids here once...
#include "hxtypes.h"

#define INITGUID
#ifdef _STATICALLY_LINKED
#define _STATICALLY_LINKED_SAVE 1
#undef _STATICALLY_LINKED
#endif
#include "hxcom.h"
#ifdef _STATICALLY_LINKED_SAVE
#include "hxsdesc.h"
//#include "pxffmcod.h"
//#include "ihxperplex.h"
#include "../../../server/log/tmplgpln/iLogTemplate.h"
#define _STATICALLY_LINKED  1
#undef _STATICALLY_LINKED_SAVE
#endif
#include "hxiids.h"
#include "hxpiids.h"
#include "server_piids.h"

#ifdef PAULM_CLIENTAR
#include "objdbg.h"
#endif

#include "servrtsp_piids.h"

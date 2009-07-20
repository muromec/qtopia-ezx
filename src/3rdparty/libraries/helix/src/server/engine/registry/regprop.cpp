/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: regprop.cpp,v 1.3 2009/05/30 19:09:56 atin Exp $ 
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
#include <string.h>
#include "hxcom.h"
#include "debug.h"
#include "watchlst.h"
#include "regkey.h"
#include "regprop.h"
#include "regdb_dict.h"
#include "regmem.h"

HX_RESULT
ServRegProperty::key_str(char* s)
{
    _prop_name = new ServRegKey((const char*)s);
    if (_prop_name)
	return HXR_OK;
    else
	return HXR_FAIL;
}

HX_RESULT 
ServRegProperty::db_val(ServRegDB_dict* dbi)
{
    if (dbi)
    {
	_prop_val.t_imp = dbi;
	return HXR_OK;
    }
    else
    {
	DPRINTF(D_INFO, ("trying to CREATE Property(%s) with a NULL value\n", 
	                 _prop_name->get_key_str()));
	return HXR_FAIL;
    }
}

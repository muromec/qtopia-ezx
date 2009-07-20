/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: packedvalues.h,v 1.5 2007/07/06 20:39:23 jfinnecy Exp $
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

#ifndef _PACKEDVALUES_H_
#define _PACKEDVALUES_H_

#define PROP_TYPE_ULONG32 0
#define PROP_TYPE_CSTRING 1
#define PROP_TYPE_BUFFER  2

struct PackedValuesHeader {
    UINT8    fourCC[4];	    //fourCC to identify the buffer
    UINT32   size;	    //size of the data

    void pack()
    {
	size = ((size&0xff)<<24) + (((size>>8)&0xff)<<16)
	      + (((size>>16)&0xff)<<8) + ((size>>24)&0xff);
    }
    void unpack()
    {
	pack();
    }
};

/*
    packed buffer format: fourCC:size:num_items:<item1>:<item2>:....
    item format: type(8):name_len(8):name:data_len(16):data
*/
class PackedValues {
public:
    static UINT32 size(IHXValues* pValues);
    static HX_RESULT pack(UINT8* buf, UINT32 &len, IHXValues* pValues, UINT8 fourCC[4]);

    HX_RESULT unpack(UINT8* buf, UINT32 len, IHXValues* pValues, IHXCommonClassFactory* pCCF = NULL);
    UINT8 m_fourCC[4];

private:
    static UINT32 packone(UINT8* buf, UINT8 type, UINT8 name_length, UINT8* name, UINT16 value_length, UINT8* value_data);
};

#endif

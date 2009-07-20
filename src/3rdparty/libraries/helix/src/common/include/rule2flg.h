/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rule2flg.h,v 1.5 2005/03/17 19:24:33 ping Exp $
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

#ifndef _RULE2FLG_H_
#define _RULE2FLG_H_

#include "hxtypes.h"


#define HX_3GPPTT_RULE_TO_FLAG_MAP_PROPERTY "3GPP-TT 1.0 Flags"
#define RULE_TO_FLAG_MAP_PROPERTY "RMFF 1.0 Flags"
class RuleToFlagMap
{
public:
    ~RuleToFlagMap()
    {
        HX_VECTOR_DELETE(rule_to_flag_map);
    };

    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 2;}

    UINT16	num_rules;
    UINT16*	rule_to_flag_map;
};

inline UINT8*
RuleToFlagMap::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {*off++ = (UINT8) (num_rules>>8); *off++ = (UINT8) (num_rules);}
    {for (int i = 0;  i < num_rules; i++)
	{*off++ = (UINT8) (rule_to_flag_map[i]>>8); *off++ = (UINT8) (rule_to_flag_map[i]);}
    }
    len = off-buf;
    return off;
}

inline UINT8*
RuleToFlagMap::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    {num_rules = *off++<<8; num_rules |= *off++;}
    {
	rule_to_flag_map = new UINT16[num_rules];
	for (int i = 0;  i < num_rules; i++)
	{rule_to_flag_map[i] = *off++<<8; rule_to_flag_map[i] |= *off++;}
    }
    return off;
}



/*
class C3GPPTTRuleToFlagMap
{
  public:
    UINT8*      pack(UINT8* buf, UINT32 &len);
    UINT8*      unpack(UINT8* buf, UINT32 len);
    const UINT32 static_size() {return 2;}

    UINT16      m_uiNumRules;
    UINT16*     m_pRuleToFlagMap;
};

inline UINT8*
C3GPPTTRuleToFlagMap::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {*off++ = (UINT8) (m_uiNumRules>>8); *off++ = (UINT8) (m_uiNumRules);}
    {for (int i = 0;  i < m_uiNumRules; i++)
	{*off++ = (UINT8) (m_pRuleToFlagMap[i]>>8); *off++ = (UINT8) (m_pRuleToFlagMap[i]);}
    }
    len = off-buf;
    return off;
}

inline UINT8*
C3GPPTTRuleToFlagMap::unpack(UINT8* buf, UINT32 len)
{
    if (!buf || !len)
	return 0;
    UINT8* off = buf;

    {m_uiNumRules = *off++<<8; m_uiNumRules |= *off++;}
    {
	m_pRuleToFlagMap = new UINT16[m_uiNumRules];
	for (int i = 0;  i < m_uiNumRules; i++)
	{m_pRuleToFlagMap[i] = *off++<<8; m_pRuleToFlagMap[i] |= *off++;}
    }
    return off;
}
*/

#endif /* _RULE2FLG_H_ */


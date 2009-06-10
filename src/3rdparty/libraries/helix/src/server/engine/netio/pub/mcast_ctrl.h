/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mcast_ctrl.h,v 1.3 2004/06/02 21:59:21 tmarshall Exp $
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

#ifndef _MCAST_CTRL_H_
#define _MCAST_CTRL_H_

struct sockaddr_in;
class ServerRegistry;

struct MulticastACRule
{
    MulticastACRule();
    ~MulticastACRule();
    BOOL SubNetMatch(ULONG32 interfaceId);

    INT32 nRuleNum;
    IHXBuffer* pFromAddr;
    IHXBuffer* pNetmask;
    ULONG32 ulFromAddrLowerLimit;
    ULONG32 ulFromAddrUpperLimit;
    ULONG32 ulFromAddr;
    ULONG32 ulFromNetmask;
    BOOL bFromAddrIsAny;
};

class MulticastRuleChain
{
public:
    MulticastRuleChain();
    ~MulticastRuleChain();
    HX_RESULT   Init(Process* proc, ServerRegistry* registry);
    int         GetNumOfRules();

    // overloaded operator helpers
    MulticastACRule* operator[](int nIndex) const;

    class Iterator
    {
    public:
                        Iterator(MulticastRuleChain* chain);
            MulticastACRule*    operator++(int);        // postfix increment
            BOOL        operator==(const Iterator& iter) const;
            BOOL        operator!=(const Iterator& iter) const;
            MulticastACRule*    operator*();

    private:
            MulticastRuleChain* _chain;
            int         _idx;
    };

    friend class Iterator;

private:
    void        _clearRules();
    BOOL        _isValidHostAddr(ULONG32 addr);
    HX_RESULT   _parseAddr(const char* addrStr, int strLen,
                    ULONG32& addr, ULONG32& netmask);
    HX_RESULT   _parseToAddr(const char* addrStr, int strLen,
                    ULONG32& addr);
    HX_RESULT   _createMulticastRuleChain();
    MulticastACRule*    _parseRule(const char* ruleNumStr, ULONG32 regId);

    // for ControlList default
    BOOL        _findRule(ULONG32 propID);
    HX_RESULT   _makeDefaultRule(void);
    HX_RESULT   _addDefaultRule(const char* pName);

    MulticastACRule**   m_rules;
    int         m_numRules;
    int         m_beginIdx;
    int         m_endIdx;

    Process*            m_proc;
    ServerRegistry*     m_registry;
    IHXErrorMessages*   m_pErrorHandler;
    ULONG32*            m_hostAddrs;
    INT32               m_numOfAddrs;
};


class MulticastAccessControl
{
public:
                        MulticastAccessControl();
                        ~MulticastAccessControl();
    HX_RESULT           Init(Process* proc);
    BOOL                ValidateAddress(ULONG32 incomingAddr);
    BOOL                IsValidAddress(ULONG32 ulClientAddr);
    BOOL                IsValidAddress(const char* incomingAddr);

    HX_RESULT           Reload() { return HXR_NOTIMPL; }
    BOOL                RulesArePresent() { return m_rulesArePresent; }

private:

    Process*            m_proc;
    MulticastRuleChain* m_rules;
    ServerRegistry*     m_registry;
    BOOL                m_rulesArePresent;
};

inline MulticastACRule*
MulticastRuleChain::operator[](int nIdx) const
{
    return (nIdx >= 0 || nIdx < m_numRules ? m_rules[nIdx] : 0);
}

inline
MulticastRuleChain::Iterator::Iterator(MulticastRuleChain* chain)
    : _chain(chain)
    , _idx(0)
{
}

// postfix increment
inline MulticastACRule*
MulticastRuleChain::Iterator::operator++(int)
{
    return (_idx >= 0 && _idx <= _chain->m_numRules
        ? _chain->m_rules[_idx++] : 0);
}

inline BOOL
MulticastRuleChain::Iterator::operator==(const MulticastRuleChain::Iterator& rhs) const
{
    if (_idx >= 0 && _idx < _chain->m_numRules &&
        rhs._idx >= 0 && rhs._idx < rhs._chain->m_numRules)
    {
        return (_chain->m_rules[_idx] == rhs._chain->m_rules[_idx]
            ? TRUE : FALSE);
    }
    else
        return FALSE;
}

inline BOOL
MulticastRuleChain::Iterator::operator!=(const MulticastRuleChain::Iterator& rhs) const
{
    if (_idx >= 0 && _idx < _chain->m_numRules &&
        rhs._idx >= 0 && rhs._idx < rhs._chain->m_numRules)
    {
        return (_chain->m_rules[_idx] != rhs._chain->m_rules[_idx]
            ? TRUE : FALSE);
    }
    else
        return TRUE;
}

inline MulticastACRule*
MulticastRuleChain::Iterator::operator*()
{
    return (_idx >= 0 && _idx < _chain->m_numRules
        ? _chain->m_rules[_idx] : 0);
}

#endif /* _MCAST_CTRL_H_ */

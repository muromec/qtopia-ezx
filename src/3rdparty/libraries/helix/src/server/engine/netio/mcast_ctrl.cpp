/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mcast_ctrl.cpp,v 1.6 2005/06/30 00:20:25 dcollins Exp $
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
 *  given below is the access control structure in the server's registry
 *  AccessControl               COMPOSITE
 *    100                       COMPOSITE
 *       TCP                    COMPOSITE
 *          Allow               COMPOSITE
 *              To      STRING value
 *              From    STRING value
 *              Ports           COMPOSITE
 *                  elem0 NUMERIC value
 *                  elem1 NUMERIC value
 *    101
 * ...
 */

#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "ihxpckts.h"
#include "chxpckts.h"
#include "hxstrutl.h"
#include "netbyte.h"    /* for IsNumericAddr() */
#include "fio.h"
#include "sio.h"
#include "bufio.h"
#include "fsio.h"
#include "udpio.h"
#include "servreg.h"
#include "debug.h"
#include "proc.h"
#include "servreg.h"
#include "access_ctrl.h"
#include "hxerror.h"            //      For HXLOG_... Defines
#include "mcast_ctrl.h"
#include "servbuffer.h"

#if defined _WINDOWS
#define snprintf _snprintf
#endif

static const char* classCUpperLimit = "223.255.255.255";
static ULONG32 netLongClassCULimit = 4294967263U;
static const int len = 15;

static const char* g_zcszInvalidAddressMsg =
        "The Multicast Allow address %.*s is not valid.  Defaulting to allow Any.";

static int
_mcastRuleCompare(const void* rule1, const void* rule2)
{
    const MulticastACRule* r1 = *((const MulticastACRule **)rule1);
    const MulticastACRule* r2 = *((const MulticastACRule **)rule2);

    if (r1->nRuleNum > r2->nRuleNum)
        return 1;
    else if (r1->nRuleNum < r2->nRuleNum)
        return -1;
    else
        return 0;
}

/*
 *  Copyright (c) 1996, 1997 Real Networks
 *
 *  Function Name:      MulticastRuleChain::_convertStrToNetLongAddr
 *  Input Params:       const char* addrStr, int strLen
 *  Return Value:       ULONG32
 *  Description:
 *      Converts a string of the type "172.16.2.40" or "foo.bar.com"
 *  into the equivalent "unsigned long" IP address in the Network byte
 *  order.
 */
static ULONG32
_convertStrToNetLongAddr(const char* addrStr, int strLen)
{
    // if the string IS null terminated, then reduce the len by 1
    if (*(addrStr+strLen) == '\0')
        strLen--;

    if (::IsNumericAddr(addrStr, strLen))
    {
        return (ULONG32)HXinet_addr(addrStr);
    }

    struct hostent* hent = ::gethostbyname(addrStr);
    return (hent ? *(ULONG32*)hent->h_addr : INADDR_NONE);
}

/*
 *  Copyright (c) 1996, 1997 Real Networks
 *
 *  Function Name:      MulticastRuleChain::_convertNumBitsToNetLongNetmask
 *  Input Params:       ULONG32 num
 *  Return Value:       ULONG32
 *  Description:
 *      This method converts a number which indicates how many bits
 *  are set to '1', into a netmask in the Network byte order.
 *  e.g.
 *      if num is equal to 24
 *  then the netmask will be 255.255.255.0
 *  which effectively is 11111111.11111111.11111111.00000000
 */
static ULONG32
_convertNumBitsToNetLongNetmask(ULONG32 num)
{
    if (num > 32)
        return 0xffffffff;

    ULONG32 mask = 0x80000000;
    ULONG32 netmask = 0L;
    for (ULONG32 i = 0; i < num; i++)
        netmask |= (mask >> i);

    return DwToNet(netmask);
}

MulticastACRule::MulticastACRule()
        : nRuleNum(0)
        , pFromAddr(0)
        , pNetmask(0)
        , ulFromAddrLowerLimit(0)
        , ulFromAddrUpperLimit(0)
        , ulFromAddr(0)
        , ulFromNetmask(0)
        , bFromAddrIsAny(FALSE)
{
}

MulticastACRule::~MulticastACRule()
{
    HX_RELEASE(pFromAddr);
    HX_RELEASE(pNetmask);
}

/*
 *  Copyright (c) 1996, 1997 Real Networks
 *
 *  Function Name:      MulticastACRule::SubNetMatch
 *  Input Params:       ULONG32 interfaceID
 *  Return Value:       BOOL
 *  Description:
 *      Checks if the subnet's of the rule match the interfaceID's
 *  returns TRUE if they match and FALSE otherwise.
 */
BOOL
MulticastACRule::SubNetMatch(ULONG32 interfaceID)
{
    if ((ulFromAddr & ulFromNetmask) == (interfaceID & ulFromNetmask))
    {
        // printf("SubNetMatch found for %ld\n", interfaceID);
        return TRUE;
    }

    return FALSE;
}

MulticastRuleChain::MulticastRuleChain()
          : m_rules(0)
          , m_numRules(0)
          , m_beginIdx(0)
          , m_endIdx(0)
          , m_proc(0)
          , m_registry(0)
          , m_pErrorHandler(0)
          , m_hostAddrs(0)
          , m_numOfAddrs(0)
{
}

MulticastRuleChain::~MulticastRuleChain()
{
    _clearRules();
    if (m_rules)
    {
        delete [] m_rules;
        m_rules = 0;
    }
    if (m_hostAddrs)
    {
        delete [] m_hostAddrs;
        m_hostAddrs = 0;
    }
}

HX_RESULT
MulticastRuleChain::_createMulticastRuleChain()
{
    HX_RESULT res = HXR_OK;
    IHXValues* props = 0;
    const char* propName = 0;   // here it represents the rule number
    ULONG32 propID = 0;
    MulticastACRule* rule = 0;
    int i = 0;

    // if we need to, make a default rule in the registry, so the rest of the
    // process can take place.
    _makeDefaultRule();

    res = m_registry->GetPropList("config.Multicast.ControlList", props,
        m_proc);

    /*
     * XXXAAK -- remember to change ServerRegistry::_getPropList()
     * to return NULL (IHXValues *) in case the property does not exist
     */
    if (SUCCEEDED(res) && props)
    {
        props->GetFirstPropertyULONG32(propName, propID);
        if (!propName || !propID)
        {
            res = HXR_FAIL;
            goto endInit;
        }

        // printf("creating rule chain\n");
        while(propName && propID)
        {
            // printf("rule %d detected\n", m_numRules);
            rule = _parseRule(propName, propID);
            if (rule)
                m_rules[m_numRules++] = rule;
            propName = 0;
            propID = 0;
            props->GetNextPropertyULONG32(propName, propID);
        }
        // printf("total %d rules\n", m_numRules);
        if (m_numRules)
        {
            ::qsort(m_rules, m_numRules, sizeof(MulticastACRule *),
                _mcastRuleCompare);
            m_beginIdx = m_numRules;
            m_endIdx = 0;
        }
    }
    else
    {
        // now that we are making a default, it shouldn't get here
        HX_ASSERT(!"ControlList missing...");
        res = HXR_FAIL;
    }

endInit:
    // printf("_createMulticastRuleChain() returns %lu\n", res);
    HX_RELEASE(props);
    return res;
}

HX_RESULT
MulticastRuleChain::Init(Process* proc, ServerRegistry* registry)
{
    m_rules = new MulticastACRule*[MAX_ACCESS_RULES+1];
    memset(m_rules, 0, sizeof(MulticastACRule *) * MAX_ACCESS_RULES+1);
    m_proc = proc;
    m_registry = registry;
    m_pErrorHandler = proc->pc->error_handler;

    if (!m_rules || !m_registry)
    {
        // printf("could not create MulticastACRule array\n");
        return HXR_UNEXPECTED;
    }

    return _createMulticastRuleChain();
}

int
MulticastRuleChain::GetNumOfRules()
{
    return m_numRules;
}

/*
 *  Copyright (c) 1996, 1997 Real Networks
 *
 *  Function Name:      MulticastRuleChain::_isValidHostAddr
 *  Input Params:       ULONG32 addr
 *  Return Value:       BOOL
 *  Description:
 *      Its job is to match the addr passed in as a parameter with the
 *  known IP addresses specified in the IP binding list.
 *  XXXAAK --
 *      VERY INEFFICIENT method. does a LINEAR search.
 */
BOOL
MulticastRuleChain::_isValidHostAddr(ULONG32 addr)
{
    if (!m_hostAddrs || !m_numOfAddrs)
        return TRUE;

    ULONG32* basePtr = m_hostAddrs;
    ULONG32* arrayPtr = m_hostAddrs;
    BOOL match = FALSE;

    while (*arrayPtr != 0xefefefef)
    {
        if (*arrayPtr == addr)
        {
            match = TRUE;
            break;
        }
        arrayPtr++;
    }
    return match;
}

void
MulticastRuleChain::_clearRules()
{
    MulticastACRule* rule = 0;

    // clean out all the access rules
    for (int i = 0; i < m_numRules; i++)
    {
        rule = m_rules[i];
        if (rule)
            HX_DELETE(rule);
    }

}

HX_RESULT
MulticastRuleChain::_parseAddr(const char* addrStr, int strLen,
    ULONG32& netLongAddr, ULONG32& netLongNetmask)
{
    HX_RESULT res = HXR_OK;
    char* tmpStr = new char[strLen];
    char* ptr = 0;
    strncpy(tmpStr, addrStr, strLen);

    netLongAddr = netLongNetmask = 0L;

    if ((ptr = strchr(tmpStr, ':')))
    {
        *ptr = '\0';
        netLongAddr = _convertStrToNetLongAddr(tmpStr, ptr-tmpStr);
        ptr++;
        netLongNetmask = _convertStrToNetLongAddr(ptr, strLen-(ptr-tmpStr));
    }
    else if ((ptr = strchr(tmpStr, '/')))
    {
        *ptr = '\0';
        netLongAddr = _convertStrToNetLongAddr(tmpStr, ptr-tmpStr);
        int netmaskBits = atoi(ptr+1);  // to get past the '/'
        netLongNetmask = _convertNumBitsToNetLongNetmask(netmaskBits);
    }
    else
    {
        netLongAddr = _convertStrToNetLongAddr(tmpStr, strLen);
        netLongNetmask = 0xffffffff;
    }

        //      INADDR_NONE returned if gethostbyname or inet_addr fails
        //      this means an invalid address failure.
        if(netLongAddr == INADDR_NONE)
        {
                res = HXR_FAIL;
        }

    HX_VECTOR_DELETE(tmpStr);

    return res;
}

/*
 *  Copyright (c) 1996, 1997, 1998 Real Networks
 *
 *  Function Name:      MulticastRuleChain::_parseRule
 *  Input Params:       const char* ruleNumStr, ULONG32 regId
 *  Return Value:       MulticastACRule*
 *  Description:
 *      This method takes a rule number (registry Composite property)
 *  and the id and retrieves all the properties under it. it then
 *  parses the info and puts it into a access rule structure which
 *  gets returned back to the caller.
 *      In case an error occurs while parsing, a NULL pointer is returned.
 */
MulticastACRule*
MulticastRuleChain::_parseRule(const char* ruleNumStr,
    ULONG32 regId)
{
    IHXValues* props = 0;
    const char* propName = 0;   // here it represents the rule number
    ULONG32 propID = 0;
    MulticastACRule* rule = 0;
    char* ruleVar = 0;
    INT32 ruleNum = 0;
    char* strValue = 0;
    IHXBuffer* fromAddr = 0;
    IHXBuffer* netmask = 0;
    IHXBuffer* value = 0;
    ULONG32 netLongFromAddr = 0L;
    ULONG32 netLongFromNetmask = 0L;
    ULONG32 netLongFromLowerLimit = 0L;
    ULONG32 netLongFromUpperLimit = 0L;
    BOOL fromAddrIsAny = FALSE;
    int numProps = 0;

    ruleVar = (char*)strrchr(ruleNumStr, '.');
    if (!ruleVar || !*ruleVar)
    {
        // printf("invalid Access rule # -- %s\n", ruleNumStr);
        goto fin;
    }
    ruleVar++;  // advance past the '.'
    ruleNum = atoi(ruleVar);

    m_registry->GetPropList(regId, props, m_proc);
    if (!props)
    {
        // printf("invalid Access rule # -- %s - !props\n", ruleNumStr);
        goto fin;
    }

    props->GetFirstPropertyULONG32(propName, propID);
    if (!propName || !propID)
    {
        // printf("invalid Access rule # -- %s - !propName||!propID\n",
            // ruleNumStr);
        goto fin;
    }

    while (propName && propID)
    {
        ruleVar = (char*)strrchr(propName, '.');
        if (!ruleVar || !*ruleVar)
        {
            // printf("invalid Access rule # -- %s - !ruleVar||!*ruleVar\n", ruleNumStr);
            break;
        }
        ruleVar++;      // advance past the '.'

        // printf("%s ", ruleVar);
        if (!strcasecmp(ruleVar, "Allow"))
        {
            if (HXR_OK != m_registry->GetStr(propID, fromAddr, m_proc))
                break;

            // printf("= %s", (const char *)fromAddr->GetBuffer());

            if (!strcasecmp("any", (const char *)fromAddr->GetBuffer()))
            {
                netLongFromNetmask = 0xffffffff;
                netLongFromLowerLimit = 0L;
                netLongFromUpperLimit = netLongClassCULimit;
                fromAddrIsAny = TRUE;
            }
            else
            {
                //      This is an error, INADDR_NONE was returned from parsing
                //      the address and this caused a fail result to be returned
                //      To handle gracefully, log an error to let know that the
                //      error has occurred and specify all addresses allowed
                if (HXR_OK != _parseAddr(
                    (const char *)fromAddr->GetBuffer(), fromAddr->GetSize(),
                    netLongFromAddr, netLongFromNetmask))
                {
                        char szMsg[128];
                        snprintf(szMsg, 128, g_zcszInvalidAddressMsg, fromAddr->GetSize(), (const char*) fromAddr->GetBuffer());
                        //      If this is not a valid ip address, then
                        //      report an error and default to any
                        m_pErrorHandler->Report(HXLOG_ERR, 0, 0, szMsg, 0);
                        netLongFromNetmask = 0xffffffff;
                        netLongFromLowerLimit = 0L;
                        netLongFromUpperLimit = netLongClassCULimit;
                        fromAddrIsAny = TRUE;
                }
                else
                {
                        //      If INADDR_ANY '0.0.0.0' then should be the same as specifying Any
                        //      and this is not considered an error
                        if(netLongFromAddr == INADDR_ANY)
                        {
                                netLongFromNetmask = 0xffffffff;
                                netLongFromLowerLimit = 0L;
                                netLongFromUpperLimit = netLongClassCULimit;
                                fromAddrIsAny = TRUE;
                        }
                        else
                        {
                                /*
                                 * ToAddr ==> 172.16.2.0
                                 * Netmask => 255.255.255.0
                                 * UpperLimit => 172.16.2.255
                                 */
                                netLongFromLowerLimit = netLongFromAddr;
                                netLongFromUpperLimit = netLongFromAddr | ~netLongFromNetmask;
                        }
                }
            }

            /****
            printf(" -- addr(%lu), lower(%lu), upper(%lu)\n",
                   netLongFromAddr, netLongFromLowerLimit,
                   netLongFromUpperLimit);
            ****/
            numProps++;
        }
        else
        {
            // printf("erroneous property under rule # %s\n", ruleNumStr);
        }

        propName = 0;
        propID = 0;
        props->GetNextPropertyULONG32(propName, propID);
    }

    // printf("\n");

    if (numProps == 1)
    {
        rule = new MulticastACRule;
        rule->nRuleNum = ruleNum;
        rule->pFromAddr = fromAddr;
        rule->pFromAddr->AddRef();
        if (netmask)
        {
            rule->pNetmask = netmask;
            rule->pNetmask->AddRef();
        }
        rule->ulFromAddr = netLongFromAddr;
        rule->ulFromNetmask = netLongFromNetmask;
        rule->ulFromAddrLowerLimit = netLongFromLowerLimit;
        rule->ulFromAddrUpperLimit = netLongFromUpperLimit;
        rule->bFromAddrIsAny = fromAddrIsAny;

        // printf("all of 1 rule parts detected\n");
    }

fin:
    return rule;
}

/*
*   returns TRUE if it finds the rule under propID
*   propID should be the ID for "config.Multicast.ControlList.???"
*/
BOOL
MulticastRuleChain::_findRule(ULONG32 propID)
{
    IHXValues* props = NULL;
    const char* propName = NULL;

    if (SUCCEEDED(m_registry->GetPropList(propID, props, m_proc)))
    {
        if (SUCCEEDED(props->GetFirstPropertyULONG32(propName, propID)))
        {
            // no need for a default
            HX_RELEASE(props);
            return TRUE;
        }
        else
        {
            // <Var Allow="???"/> is missing...
        }
    }

    HX_RELEASE(props);
    return FALSE;
}

/*
*  Make a default rule of "any" if necessary.
*  It assumes "config.Multicast.ControlList" is present in the registy
*/
HX_RESULT
MulticastRuleChain::_makeDefaultRule()
{
    HX_RESULT res = HXR_OK;
    IHXValues* props = 0;
    const char* propName = 0;
    ULONG32     propID = 0;
    IHXBuffer* pBuf = 0;

    UINT32 ul = 0;
    CHXString str;

    /*
    *   This is how it looks like...
    <List Name="Multicast">
        <List Name="ControlList">
            <List Name="100">
                <Var Allow="any"/>
            </List>
        </List>
    </List>
    */

    // create the right registry entry if we need to.  Figoure out if we need to
    // do this at all as well!
    if (SUCCEEDED(m_registry->GetPropList("config.Multicast.ControlList",
        props, m_proc)))
    {
        // we need to make sure there is at least one good entry in the list
        if (SUCCEEDED(props->GetFirstPropertyULONG32(propName, propID)))
        {
            if (_findRule(propID))
            {
                // found a rule...no need for a default.
                HX_RELEASE(props);
                res = HXR_OK;
                goto bail;
            }
            else
            {
                // try to see if there is any other entry under ControlList
                while (SUCCEEDED(props->GetNextPropertyULONG32(propName, propID)))
                {
                    if (_findRule(propID))
                    {
                        HX_RELEASE(props);
                        res = HXR_OK;
                        goto bail;
                    }
                }
            }
        }

        HX_RELEASE(props);
    }
    else
    {
        // need to see what's in the registry
        if (FAILED(m_registry->GetPropList("config.Multicast", props, m_proc)))
        {
            HX_ASSERT(!props);

            // there is no Multicast section....Add it
            if (!m_registry->AddComp("config.Multicast", m_proc))
            {
                // there is nothing we can do...
                res = HXR_FAIL;
                goto bail;
            }
        }
        HX_RELEASE(props);

        if (FAILED(m_registry->GetPropList("config.Multicast.ControlList",
            props, m_proc)))
        {
            HX_ASSERT(!props);

            if (!m_registry->AddComp("config.Multicast.ControlList", m_proc))
            {
                // there is nothing we can do...
                res = HXR_FAIL;
                goto bail;
            }
        }
        HX_RELEASE(props);
    }

    // shouldn't make a default if there is a list
    HX_ASSERT(!m_numRules);

    // put a default entry in a registry, so the rest of the process can take
    // place.  Try at most 100 times
    do
    {
        ul += 100;
        str.Format("config.Multicast.ControlList.%u", ul);
    } while (!m_registry->AddComp(str, m_proc) && ul <= 10000);

    str += ".Allow";
    res = _addDefaultRule((const char*)str);
bail:
    HX_RELEASE(props);
    HX_RELEASE(pBuf);
    return res;
}

/*  add "any" at pName */
HX_RESULT
MulticastRuleChain::_addDefaultRule(const char* pName)
{
    IHXBuffer* pBuf = new ServerBuffer(TRUE);
    pBuf->Set((const UCHAR*)"any", sizeof("any"));
    m_registry->AddStr(pName, pBuf, m_proc);
    pBuf->Release();

    return HXR_OK;
}

MulticastAccessControl::MulticastAccessControl()
                   : m_proc(0)
                   , m_registry(0)
                   , m_rules(0)
                   , m_rulesArePresent(FALSE)
{
}

MulticastAccessControl::~MulticastAccessControl()
{
    HX_DELETE(m_rules);
}

HX_RESULT
MulticastAccessControl::Init(Process* proc)
{
    m_proc = proc;
    m_registry = proc->pc->registry;
    m_rules = new MulticastRuleChain;
    HX_RESULT res = m_rules->Init(m_proc, m_registry);
    if (res == HXR_OK)
        m_rulesArePresent = TRUE;
    /*
     * XXXAAK -- just test the process
     * put two entries into the config file at least one of which
     * has the same subnet as these addresses in case u want to test
     */
    // printf("172.16.2.99 = %s\n",
        // IsValidAddress("172.16.2.99") == TRUE ? "IsValid" : "NotValid");
    // printf("172.16.3.99 = %s\n",
        // IsValidAddress("172.16.3.99") == TRUE ? "IsValid" : "NotValid");
    // fflush(0);

    return res;
}

/*
 * check the validity of the address against the multicast control list
 * loop thru the rules
 *    if (incomingAddr is NOT within the range of rule->FromAddr/Netmask)
 *          skip to the next rule
 *    else
 *          return TRUE
 *    fi
 *    break;
 * endloop
 * return FALSE
 */
BOOL
MulticastAccessControl::ValidateAddress(ULONG32 incomingAddr)
{
    BOOL access = FALSE;
    MulticastACRule* rule = 0;
    ULONG32 clientAddr = incomingAddr;
    int numRules = m_rules->GetNumOfRules();

    DPRINTF(0x02000000, ("validating clientAddr(%lu)...\n", clientAddr));

    for (MulticastRuleChain::Iterator i(m_rules);
        *i != 0 && numRules > 0;
        numRules--, i++, rule = 0)
    {
        rule = *i;

        DPRINTF(0x02000000, ("rule %d being checked\n", numRules));
        if (!rule)
            continue;

        DPRINTF(0x02000000, ("%d: ruleFrom(%d) & ruleMask(0x%x) "
                "== clientAddr(%d) & ruleMask(0x%x)\n",
                rule->nRuleNum, rule->ulFromAddr, rule->ulFromNetmask,
                clientAddr, rule->ulFromNetmask));

        if (rule->bFromAddrIsAny || rule->SubNetMatch(clientAddr))
        {
            DPRINTF(0x02000000, ("match found...\n"));
            return TRUE;
        }
    }

    return access;
}

BOOL
MulticastAccessControl::IsValidAddress(ULONG32 clientAddr)
{
    return ValidateAddress(clientAddr);
}

BOOL
MulticastAccessControl::IsValidAddress(const char* incomingAddr)
{
    ULONG32 clientAddr = _convertStrToNetLongAddr(incomingAddr,
        strlen(incomingAddr));
    return ValidateAddress(clientAddr);
}

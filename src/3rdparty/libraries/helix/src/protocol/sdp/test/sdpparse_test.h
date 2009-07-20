/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sdpparse_test.h,v 1.6 2007/07/06 20:51:39 jfinnecy Exp $
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

#ifndef SDPPARSE_TEST_H
#define SDPPARSE_TEST_H

#include "hx_cmd_based_test.h"

#include "sdpmdparse.h"
#include "ihxlist.h"

class CHXListContext;

class SDPMediaDescParserTest : public HLXCmdBasedTest
{
public:
    SDPMediaDescParserTest();
    ~SDPMediaDescParserTest();

    virtual const char* DefaultCommandLine() const;
    virtual void GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds);
    virtual HLXCmdBasedTest* Clone() const;

protected:
    bool HandleInitCmd(const UTVector<UTString>& info);
    bool HandleParseCmd(const UTVector<UTString>& info);
    bool HandleSetClientContextCmd(const UTVector<UTString>& info);

    bool HandleIntCountCmd(const UTVector<UTString>& info);
    bool HandleStringCountCmd(const UTVector<UTString>& info);
    bool HandleBufferCountCmd(const UTVector<UTString>& info);
    bool HandleObjectCountCmd(const UTVector<UTString>& info);

    bool HandleGetIntCmd(const UTVector<UTString>& info);
    bool HandleGetStringCmd(const UTVector<UTString>& info);
    bool HandleGetBufferCmd(const UTVector<UTString>& info);
    bool HandleGetIHXValuesCmd(const UTVector<UTString>& info);
    bool HandleGetIHXListCmd(const UTVector<UTString>& info);

    bool HandleListGetCountCmd(const UTVector<UTString>& info);
    bool HandleListItrBeginCmd(const UTVector<UTString>& info);
    bool HandleListItrGetBufferCmd(const UTVector<UTString>& info);
    bool HandleListItrNextCmd(const UTVector<UTString>& info);

private:
    bool GetHXResult(const char* pStr, HX_RESULT& res) const;
    bool GetBuffer(const char* pStr, IHXBuffer*& pValue) const;
    void PrintBuffer(const char* pLabel, IHXBuffer* pBuf) const;

    void DestroyValues();
    
    HX_RESULT getIHXValues(const char* pID, REF(IHXValues*) pHdr);
    HX_RESULT getGroupIDBuffer(IHXValues* pHdr, const char* pGroupType,
                               const char* pGroupID,
                               REF(IHXBuffer*) pIDBuffer) const;

    HX_RESULT getIHXValuesProp(IHXValues* pHdr, const char* pKey,
                               REF(IHXValues*) pValue) const;

    HX_RESULT addCCF(CHXListContext* pContext);
    HX_RESULT addClientEngine(CHXListContext* pContext);
    HX_RESULT createContext(HXBOOL bIsClient);

    IUnknown* m_pContext;
    IHXCommonClassFactory* m_pCCF;
    SDPMediaDescParser* m_pParser;
    UINT16 m_nValues;
    IHXValues** m_ppValues;
    IHXValues* m_pA;
    IHXValues* m_pB;
    IHXList* m_pList;
    IHXListIterator* m_pListItr;
};

#endif // SDPPARSE_TEST_H

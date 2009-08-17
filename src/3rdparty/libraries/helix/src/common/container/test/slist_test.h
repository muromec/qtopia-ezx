/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: slist_test.h,v 1.4 2004/07/09 18:21:31 hubbe Exp $
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

#ifndef SIMPLE_LIST_TEST_H
#define SIMPLE_LIST_TEST_H

#include "hx_cmd_based_test.h"

#include "hxslist.h"

class HLXSListTest : public HLXCmdBasedTest
{
public:
    HLXSListTest();
    ~HLXSListTest();

    virtual const char* DefaultCommandLine() const;
    virtual void GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds);
    virtual HLXCmdBasedTest* Clone() const;

protected:
    void* CreateValue(int val);
    void DestroyValue(void* ptr);
    bool GetValue(void* ptr, int& val);
    bool FindValue(int value, LISTPOSITION pos, void*& pValue);

    bool HandleGetCountCmd(const UTVector<UTString>& info);
    bool HandleIsEmptyCmd(const UTVector<UTString>& info);
    bool HandleGetHeadCmd(const UTVector<UTString>& info);
    bool HandleReplaceHeadCmd(const UTVector<UTString>& info);
    bool HandleGetTailCmd(const UTVector<UTString>& info);
    bool HandleReplaceTailCmd(const UTVector<UTString>& info);
    bool HandleRemoveHeadCmd(const UTVector<UTString>& info);
    bool HandleRemoveTailCmd(const UTVector<UTString>& info);
    bool HandleAddHeadCmd(const UTVector<UTString>& info);
    bool HandleAddHeadListCmd(const UTVector<UTString>& info);
    bool HandleAddTailCmd(const UTVector<UTString>& info);
    bool HandleAddTailListCmd(const UTVector<UTString>& info);
    bool HandleRemoveAllCmd(const UTVector<UTString>& info);
    bool HandleGetHeadPosCmd(const UTVector<UTString>& info);
    bool HandleGetTailPosCmd(const UTVector<UTString>& info);
    bool HandleGetNextCmd(const UTVector<UTString>& info);
    bool HandleReplaceNextCmd(const UTVector<UTString>& info);
    bool HandleGetPrevCmd(const UTVector<UTString>& info);
    bool HandleReplacePrevCmd(const UTVector<UTString>& info);
    bool HandleGetAtNextCmd(const UTVector<UTString>& info);
    bool HandleGetAtPrevCmd(const UTVector<UTString>& info);
    bool HandleReplaceAtPrevCmd(const UTVector<UTString>& info);
    bool HandleGetAtCmd(const UTVector<UTString>& info);
    bool HandleReplaceAtCmd(const UTVector<UTString>& info);
    bool HandleSetAtCmd(const UTVector<UTString>& info);
    bool HandleRemoveAtCmd(const UTVector<UTString>& info);
    bool HandleInsertBeforeCmd(const UTVector<UTString>& info);
    bool HandleInsertAfterCmd(const UTVector<UTString>& info);
    bool HandleFindCmd(const UTVector<UTString>& info);
    bool HandleFindIndexCmd(const UTVector<UTString>& info);
    bool HandleIsPosValidCmd(const UTVector<UTString>& info);
    bool HandleClearPosCmd(const UTVector<UTString>& info);
    bool HandleTestIteratorCmd(const UTVector<UTString>& info);
    bool HandleDumpCmd(const UTVector<UTString>& info);

private:
    CHXSimpleList m_list;
    LISTPOSITION m_pos;
};

#endif // SIMPLE_LIST_TEST_H

/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ihxfiletest.h,v 1.6 2007/07/06 20:35:23 jfinnecy Exp $
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

#ifndef IHXFILETEST_H
#define IHXFILETEST_H

#include "hx_cmd_based_test.h"

#include "hxcom.h"
#include "ihxpckts.h"
#include "hxdataf.h"
#include "hxdir.h"

class CHXDataFile;

class HLXIHXFileTest : public HLXCmdBasedTest
{
public:
    HLXIHXFileTest(const char* pModeString = NULL);
    ~HLXIHXFileTest();
    
    virtual void GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds);
    virtual HLXCmdBasedTest* Clone() const;

protected:
    // DataFile Commands
    bool HandleBindCmd(const UTVector<UTString>& info);
    bool HandleCreateCmd(const UTVector<UTString>& info);
    bool HandleOpenCmd(const UTVector<UTString>& info);
    bool HandleCloseCmd(const UTVector<UTString>& info);
    bool HandleNameCmd(const UTVector<UTString>& info);
    bool HandleIsOpenCmd(const UTVector<UTString>& info);
    bool HandleWriteCmd(const UTVector<UTString>& info);
    bool HandleReadCmd(const UTVector<UTString>& info);
    bool HandleFlushCmd(const UTVector<UTString>& info);
    bool HandleSeekCmd(const UTVector<UTString>& info);
    bool HandleRewindCmd(const UTVector<UTString>& info);
    bool HandleTellCmd(const UTVector<UTString>& info);
    bool HandleStatCmd(const UTVector<UTString>& info);
    bool HandleDeleteCmd(const UTVector<UTString>& info);
    bool HandleGetFdCmd(const UTVector<UTString>& info);
    bool HandleGetLastErrorCodeCmd(const UTVector<UTString>& info);
    bool HandleGetLastErrorStringCmd(const UTVector<UTString>& info);
    bool HandleGetTempNameCmd(const UTVector<UTString>& info);
    
    // Directory Commands
    bool HandleSetDirPathCmd(const UTVector<UTString>& info);
    bool HandleGetDirPathCmd(const UTVector<UTString>& info);
    bool HandleCreateDirCmd(const UTVector<UTString>& info);
    bool HandleIsDirValidCmd(const UTVector<UTString>& info);
    bool HandleDestroyDirCmd(const UTVector<UTString>& info);
    bool HandleDeleteDirFileCmd(const UTVector<UTString>& info);
    bool HandleMakeCurrentDirCmd(const UTVector<UTString>& info);
    bool HandleFindFirstInDirCmd(const UTVector<UTString>& info);
    bool HandleFindNextInDirCmd(const UTVector<UTString>& info);
    bool HandleRenameCmd(const UTVector<UTString>& info);

private:
    IHXDataFile* m_pIHXDataFile;
    CHXDataFile* m_pCHXDataFile;
    CHXDirectory* m_pCHXDirectory;

    char m_DataFileMode;
    bool m_bTestDir;
};

#endif // IHXFILETEST_H

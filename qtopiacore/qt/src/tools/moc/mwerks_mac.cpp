/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the tools applications of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifdef MOC_MWERKS_PLUGIN

#include "mwerks_mac.h"
#include "qt_mac.h"

/* compiler headers */
#include "DropInCompilerLinker.h"
#include "CompilerMapping.h"
#include "CWPluginErrors.h"

/* standard headers */
#include <stdio.h>
#include <string.h>

//qglobal.cpp
const unsigned char * p_str(const char * c);
QCString pstring2qstring(const unsigned char *c);

#if CW_USE_PRAGMA_EXPORT
#pragma export on
#endif

CWPLUGIN_ENTRY(CWPlugin_GetDropInFlags)(const DropInFlags** flags, long* flagsSize)
{
        static const DropInFlags sFlags = {
                kCurrentDropInFlagsVersion,
                CWDROPINCOMPILERTYPE,
                DROPINCOMPILERLINKERAPIVERSION_7,
                kCompAlwaysReload|kCompRequiresProjectBuildStartedMsg,
                Lang_C_CPP,
                DROPINCOMPILERLINKERAPIVERSION
        };
        *flags = &sFlags;
        *flagsSize = sizeof(sFlags);
        return cwNoErr;
}



CWPLUGIN_ENTRY(CWPlugin_GetDropInName)(const char** dropinName)
{
        static const char sDropInName[] = "McMoc";
        *dropinName = sDropInName;
        return cwNoErr;
}

CWPLUGIN_ENTRY(CWPlugin_GetDisplayName)(const char** displayName)
{
        static const char sDisplayName[] = "McMoc";
        *displayName = sDisplayName;
        return cwNoErr;
}

CWPLUGIN_ENTRY(CWPlugin_GetTargetList)(const CWTargetList** targetList)
{
        static CWDataType sCPU = targetCPUAny;
        static CWDataType sOS = targetOSMacintosh;
        static CWTargetList sTargetList = {kCurrentCWTargetListVersion, 1, &sCPU, 1, &sOS};
        *targetList = &sTargetList;
        return cwNoErr;
}

CWPLUGIN_ENTRY(CWPlugin_GetDefaultMappingList)(const CWExtMapList** defaultMappingList)
{
        static CWExtensionMapping sExtension[] = { {'TEXT', ".mocs", kPrecompile } };
        static CWExtMapList sExtensionMapList = {kCurrentCWExtMapListVersion, 3, sExtension};
        *defaultMappingList = &sExtensionMapList;
        return cwNoErr;
}

#if CW_USE_PRAGMA_EXPORT
#pragma export off
#endif
typedef short CWFileRef;

static int line_count = 0;
moc_status do_moc(CWPluginContext, const QCString &, const QCString &, CWFileSpec *, bool);

static CWResult        mocify(CWPluginContext context, const QCString &source)
{
    CWDisplayLines(context, line_count++);

    source.stripWhiteSpace();

    CWResult err;
        bool            dotmoc=false;
        QCString stem = source, ext;
        int dotpos = stem.findRev('.');
    if(dotpos != -1) {
        ext = stem.right(stem.length() - (dotpos+1));
        stem = stem.left(dotpos);
        if(ext == "cpp")
            dotmoc = true;
    } else {
        //whoa!
    }
    QCString dest;
    if(dotmoc)
        dest = stem + ".moc";
    else
        dest = "moc_" + stem + ".cpp";

    //moc it
    CWFileSpec destSpec;
        moc_status mocd = do_moc(context, source, dest, &destSpec, dotmoc);

#if 0
    QCString derr = "Weird";
    switch(mocd) {
    case moc_success: derr = "Success"; break;
    case moc_parse_error: derr = "Parser Error"; break;
    case moc_no_qobject:derr = "No QOBJECT"; break;
    case moc_not_time: derr = "Not Time"; break;
    case moc_no_source: derr = "No Source"; break;
    case moc_general_error: derr = "General Error"; break;
    }
        char        dmsg[200];
        sprintf(dmsg, "\"%s\" %s", source.data(), derr.data());
        CWReportMessage(context, NULL, dmsg, NULL, messagetypeError, 0);
#endif

    //handle project
    if(mocd == moc_no_qobject) {
        char        msg[400];
                sprintf(msg, "\"%s\" No relevant classes found. No output generated.", source.data());
                CWReportMessage(context, NULL, msg, NULL, messagetypeWarning, 0);
        } else if ((mocd == moc_success || mocd == moc_not_time) && !dotmoc)
        {
                long                        whichFile;
                CWNewProjectEntryInfo ei;
                memset(&ei, '\0', sizeof(ei));
                ei.groupPath = "QtGenerated";
                    err = CWAddProjectEntry(context, &destSpec, true, &ei, &whichFile);
                    if (!CWSUCCESS(err))
                    {
                            char        msg[200];
                            sprintf(msg, "\"%s\" not added", dest.data());
                            CWReportMessage(context, NULL, msg, NULL, messagetypeWarning, 0);
                    }
                    if(mocd == moc_success)
                        CWSetModDate(context, &destSpec, NULL, true);
        }
        return cwNoErr;
}

pascal short main(CWPluginContext context)
{
        short                result;
        long                request;

        if (CWGetPluginRequest(context, &request) != cwNoErr)
                return cwErrRequestFailed;
        result = cwErrInvalidParameter;

        /* dispatch on compiler request */
        switch (request)
        {
        case reqInitCompiler:
        case reqTermCompiler:
            result = cwNoErr;
        break;

        case reqCompile:
        {
            line_count = 0;
            const char *files = NULL;
            long filelen;
            CWGetMainFileText(context, &files, &filelen);
            const char *beg = files;
            for(int x = 0; x < filelen; x++) {
                if(*(files++) == '\r') {
                    char file[1024];
                    memcpy(file, beg, files - beg);
                    file[(files-beg)-1] = '\0';
                    mocify(context, file);
                beg = files;
            }
        }
        if(beg != files) {
                char file[1024];
                memcpy(file, beg, files - beg);
                file[(files-beg)] = '\0';
                mocify(context, file);
        }

        result = cwNoErr;
                break;
        }
        }

        /* return result code */
        return result;
}

#endif

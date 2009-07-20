# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: umake_codegen.py,v 1.7 2006/04/24 23:34:04 jfinnecy Exp $ 
#   
#  Copyright Notices: 
#   
#  Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved. 
#   
#  Patent Notices: This file may contain technology protected by one or  
#  more of the patents listed at www.helixcommunity.org 
#   
#  1.   The contents of this file, and the files included with this file, 
#  are protected by copyright controlled by RealNetworks and its  
#  licensors, and made available by RealNetworks subject to the current  
#  version of the RealNetworks Public Source License (the "RPSL")  
#  available at  * http://www.helixcommunity.org/content/rpsl unless  
#  you have licensed the file under the current version of the  
#  RealNetworks Community Source License (the "RCSL") available at 
#  http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
#  will apply.  You may also obtain the license terms directly from 
#  RealNetworks.  You may not use this file except in compliance with 
#  the RPSL or, if you have a valid RCSL with RealNetworks applicable 
#  to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
#  the rights, obligations and limitations governing use of the 
#  contents of the file. 
#   
#  2.  Alternatively, the contents of this file may be used under the 
#  terms of the GNU General Public License Version 2 (the 
#  "GPL") in which case the provisions of the GPL are applicable 
#  instead of those above.  Please note that RealNetworks and its  
#  licensors disclaim any implied patent license under the GPL.   
#  If you wish to allow use of your version of this file only under  
#  the terms of the GPL, and not to allow others 
#  to use your version of this file under the terms of either the RPSL 
#  or RCSL, indicate your decision by deleting Paragraph 1 above 
#  and replace them with the notice and other provisions required by 
#  the GPL. If you do not delete Paragraph 1 above, a recipient may 
#  use your version of this file under the terms of any one of the 
#  RPSL, the RCSL or the GPL. 
#   
#  This file is part of the Helix DNA Technology.  RealNetworks is the 
#  developer of the Original Code and owns the copyrights in the 
#  portions it created.   Copying, including reproducing, storing,  
#  adapting or translating, any or all of this material other than  
#  pursuant to the license terms referred to above requires the prior  
#  written consent of RealNetworks and its licensors 
#   
#  This file, and the files included with this file, is distributed 
#  and made available by RealNetworks on an 'AS IS' basis, WITHOUT  
#  WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS  
#  AND ITS LICENSORS HEREBY DISCLAIM  ALL SUCH WARRANTIES, INCLUDING  
#  WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS  
#  FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
#   
#  Technology Compatibility Kit Test Suite(s) Location:  
#     http://www.helixcommunity.org/content/tck 
#   
# Contributor(s):
# 
# ***** END LICENSE BLOCK *****
# 
"""Functions which generate C/C++ code."""

import os
import string
import bldreg
import umake_lib
import types


def WriteDLLTab(platform, project, plugin_list):
    """Write the dlltab.cpp file, and include it in the project source list.
    The dlltab.cpp file defines a global function table used by
    pnmisc/dllaccess.cpp for loading staticly linked plugins."""

    externsection = []
    tablesection = []
    dlltypesection = []


    includes = [ ]
    
    header1 = """
/* This file is generated automatically.  Please do not edit. */
"""
    
    structs = """

typedef struct DLLMAP {
        const char * dllName;
        const char * entryPoint;
        int          pluginType;
        void *       funcptr;
} DLLMAP;

extern const DLLMAP g_dllMap [];

typedef struct DLLTYPEMAP {
        const char * dllName;
        int          pluginType;
} DLLTYPEMAP;

extern const DLLTYPEMAP g_dllTypeMap[];

    """

    for target in plugin_list:
        ## retrieve the dll type from the registry
        try:
            dll_type = bldreg.get_value('dll_type', target)
        except KeyError:
            umake_lib.fatal('cannot find [dll_type] for target %s in registry' % (target))
    
        if dll_type == 'codec':
            my_type = 'DLLTYPE_CODEC'
        elif dll_type == 'common':
            my_type = 'DLLTYPE_COMMON'
        else:
            my_type = 'DLLTYPE_PLUGIN'

        dlltypesection.append('\t{ "%s", %s },' % (target, my_type))


        ## retrieve the dll entrypoints from the registry
        try:
            exported_functions = bldreg.get_value('export', target)
        except KeyError:
            umake_lib.fatal('cannot find exported functions for target %s in registry' % (target))

        exported_function_list = string.split(exported_functions, ',')

        for symbol in exported_function_list:
            tmp =  bldreg.get("export_protos",
                              target+"::"+symbol,
                              ["", None, None])

            if type(tmp) == types.StringType:
                tmp = [ tmp, None, None ]

            args, include, path = tmp

            if include and include not in includes:
                includes.append(include)

            if path:
                project.AddModuleIncludes(path)
                
            externsection.append('STDAPI entrypoint_for_%s_%s (%s);' % (target, symbol, args))
            
            tablesection.append(
                '\t{"%s", "%s", %s, (void*)entrypoint_for_%s_%s},' % (
                    target, symbol, my_type, target, symbol))

        ## add the static target to the library list
        try:
            target_lib = bldreg.get_value('targets', target)
        except KeyError:
            umake_lib.fatal('cannot rfind [targets] path for target %s in registry' % (target))

        handle = bldreg.get("file_to_handle", target_lib, None)
        if handle:
            project.AddModuleLibraries(handle)
        else:
            umake_lib.warning("codegen couldn't find which module created '%s'" % target_lib)
            target_lib = os.path.join(project.src_root_path, target_lib)
            project.AddLibraries(target_lib)
            

    ## FIXME: these should not be hardcoded!
    includes.append("dllacces.h")
    includes.append("dllpath.h")

    dlltab = open("dlltab.cpp", "w")
    def emit(x = "", tab = dlltab) :
        tab.write(x + "\n")
                        
    emit(header1)
    for i in includes:
        emit('#include "%s"' % i)
    emit(structs) 
    emit()
    emit(string.joinfields(externsection, "\n"))
    emit()
    emit()
    emit("const DLLTYPEMAP g_dllTypeMap[] = {")
    emit(string.joinfields(dlltypesection, "\n"))
    emit("\t{NULL, 0}\n};")
    emit("const DLLMAP g_dllMap[] = {")
    emit(string.joinfields(tablesection, "\n"))
    emit("\t{NULL, NULL, 0, NULL}\n};\n")

    ## have dlltab.cpp automagicly added to the source list
    project.AddSources("dlltab.cpp")

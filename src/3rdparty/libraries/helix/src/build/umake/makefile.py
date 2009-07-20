# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: makefile.py,v 1.6 2007/02/08 01:52:39 jfinnecy Exp $ 
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
"""This file implements a Makefile "sanitizer".  The input is a Makefile
which may or may not be valid.  It is parsed into the Makefile class,
which generates a valid Makefile.  This is here because there are many
Umakefil/*.pcf files which used project.write/project.writeln to write
lines directly to the Makefile.  These lines are not necessarily valid,
and may contain targets with the same name as Umake generates (like
the "all" target).  This takes its best guess at merging, and re-naming
those targets so they continue to work."""

import string
import re

class Text:
    """Stores a line of raw text. """
    def __init__( self , text ):
        self.value = text
        
        
class Macro:
    """Stores a name/value pair for a Makefile macro(variable)."""
    def __init__(self):
        self.name = ""
        self.value = ""


class Target:
    """Stores a target name, list of dependancies, and a list of commands
    for a Makefile target."""
    def __init__(self):
        self.name = ""
        self.depend_list = []
        self.command_list = []
        

class Makefile:
    """Stores a list of Makefile macros and targets.  When referenced as
    a string, the output is a valid Makefile from its macro and target
    lists."""
    
    def __init__(self):
        self.target_list = []
        self.macro_list = []
        self.preList = []
        self.postList = []

    def __str__(self):
        line_list = []

        ## write pre lines
        for line in self.preList:
            line_list.append( line.value )
            line_list.append("")
            
        ## write macros
        for macro in self.macro_list:
            line_list.append("%s=%s" % (macro.name, macro.value))
            line_list.append("")

        ## write targets
        for target in self.target_list:
            line_list.append("%s: %s" % (
                target.name, string.join(target.depend_list)))
            for command in target.command_list:
                line_list.append("\t%s" % (command))
            line_list.append("")
            
        ## write post lines
        for line in self.postList:
            line_list.append( line.value )
            line_list.append("")

        return string.join(line_list, "\n") + "\n"


    def get_variables(self):
        vars={}
        for m in self.macro_list:
            vars[m.name]=m.value
        return vars


def ParseMakefile(buff):
    """Given a buffer containing a Makefile, parse it into a Makefile()
    class.""" 
    
    mfile = Makefile()

    _re_macro = re.compile("\s*(\w+)\s*=\s*(.*)$")

    ## Nmake cruft
    _re_target0 = re.compile("\s*([A-Za-z]:[^:]*)\s*:\s*(.*)$")

    _re_target1 = re.compile("\s*([^:]+)\s*::\s*(.*)$")
    _re_target2 = re.compile("\s*([^:]+)\s*:\s*(.*)$")

    _state_pre_body = "pre body"
    _state_in_body = "in body"
    _state_in_target = "in target"
    _state_post_body = "post body"

    state = _state_pre_body
    continue_line = ""
    current_target = None
    
    line_list = string.split(buff, "\n")
    for line in line_list:
        line = continue_line + string.rstrip(line)

        ## ignore blank lines and comments
        if len(line) == 0 or line[0] == "#":
            continue

        ## handle backslash line continuations
        if line[-1] == "\\":
            continue_line = continue_line + line[:-1]
            continue
        else:
            continue_line = ""

        ## process
        if state == _state_pre_body:
            # The only valid pre_body lines at this time are !includes to enable
            # manifest embedding in VC8 per the MS recommended approach:
            # http://msdn2.microsoft.com/en-us/library/ms235591(VS.80).aspx
            if line[0:8] == "!include":
                preText = Text( line )
                mfile.preList.append( preText )
                continue
            else:
                # If line is anything else, change state to _state_in_body for
                # further evaluation.
                state = _state_in_body

        if state == _state_in_target:
            if line[0] == "\t":
                line = string.lstrip(line)
                current_target.command_list.append(line)
                continue
            else:
                state = _state_in_body
        
        if state == _state_in_body:            
            m = _re_macro.match(line)
            if m:
                macro = Macro()
                mfile.macro_list.append(macro)
                (macro.name, macro.value) = m.groups()
                continue
            else:
                m = _re_target0.match(line) or _re_target1.match(line) or _re_target2.match(line)
                if m:
                    state = _state_in_target
                    current_target = Target()
                    mfile.target_list.append(current_target)
                    (current_target.name, deps) = m.groups()
                    current_target.depend_list = string.split(deps)
                    continue
                elif line[0:8] == "!include":
                    state = _state_post_body

        if state == _state_post_body:
            if line[0:8] == "!include":
                postText = Text( line )
                mfile.postList.append( postText )
                continue
        
        # Any other result is an error.
        print "INTERNAL ERROR makefile.py(%s):%s" % (state, line)

    return mfile

def expand_variables(text, variables, empty=0):
    """Expand variables in makefile-style, use makefile.get_vars() for 'variables'"""
    # print "Expanding %s ..." % text
    arr = string.split(text,"$")
    if len(arr) < 2:
        return text

    expanded = {}
    i=1
    while i < len(arr):
        item = arr[i]
        if item[0] == '$':
            pass
        elif item[0] == '(':
            pos = string.index(item,')')
            vname = item[1:pos]

            if expanded.has_key(vname):
                xpand=expanded[vname]
            elif variables.has_key(vname):
                xpand=variables[vname]
                tmp = variables.copy()
                del tmp[vname]
                xpand=expand_variables(xpand,tmp, empty)
            elif empty:
                xpand=""
            else:
                xpand="$("+vname+")"

            expanded[vname]=xpand
            arr[i]=xpand+item[pos+1:]
        else:
            arr[i]="$"+item

        i = i + 1

    # print "Expanding %s to %s " % (text, string.join(arr, ''))
    return string.join(arr, '')

    
## testing
if __name__ == "__main__":
    import sys
    
    try:
        path = sys.argv[1]
    except IndexError:
        print "makefile.py <filename>"
        sys.exit(1)

    buff = open(path, "r").read()
    mfile = ParseMakefile(buff)
    print mfile

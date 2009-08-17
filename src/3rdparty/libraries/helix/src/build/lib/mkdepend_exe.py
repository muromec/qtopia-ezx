#!/usr/bin/env python
# -*- Mode: Python -*-
# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: mkdepend_exe.py,v 1.2 2006/07/06 19:28:05 jfinnecy Exp $ 
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
#  Contributor(s):  
#  
#  ***** END LICENSE BLOCK ***** 
# 

import os
import sys
import string
import getopt
import stat

import utils

define_list = []
include_list = []
prefix_includes = []

dependency_cache = {}

gnumake = 0

## NMAKE
def quote_dep_nmake(dep):
    return '"%s"' % dep

def unquote_dep_nmake(dep):
    if dep[0] == '"':
        return dep[1:-1]
    return dep

## UNIX make

def quote_dep_make(dep):
    dep=string.replace(dep," ","\\ ")
    return dep

def unquote_dep_make(dep):
    dep=string.replace(dep,"\\ "," ")
    return dep


def init_platform():
    global quote_dep
    global unquote_dep
    global object_ext
    if (sys.platform == "win32" or sys.platform == "win16") and not gnumake:
        object_ext="obj"
        quote_dep=quote_dep_nmake
        unquote_dep=unquote_dep_nmake
    else:
        object_ext="o"
        quote_dep=quote_dep_make
        unquote_dep=unquote_dep_make



def multifind(haystack, needles, start):
    best=len(haystack)
    for needle in needles:
        f = string.find(haystack, needle, start, best + len(needle))
        if f != -1 and f < best:
            best = f
    if best >= len(haystack):
        return -1

    return best


class Finder:
    def __init__(self, token, callback):
        self.token=token
        self.cb=callback
        self.pos=-1


class FileParser:
    pos = 0

    def SkipSpace(self):
        while self.pos < len(self.data) and self.data[self.pos] in [ " ", "\t" ]:
            self.pos = self.pos + 1

    def SkipStr(self):
        """ Skip over a C-style string """
        p = self.pos + 1
        while 1:
            p = multifind( self.data, [ "\"" , "\\" ], p)
            if p == -1:
                p = len(self.data) # Actually an error
                break
            elif self.data[p] == "\\":
                p = p + 2
            else:
                break
        self.pos = p + 1


    def SkipSingleQuotedChar(self):
        """ Skip over a C-style single quoted character. Remember that we have to deal with things like '\n' and '\x4f' """
        p = self.pos + 1
        while 1:
            p = multifind( self.data, [ "'", "\\" ], p)
            if p == -1:
                p = len(self.data) # Actually an error
                break
            elif self.data[p] == "\\":
                p = p + 2
            else:
                break
        self.pos = p + 1

    def SkiptoEOL(self):
        """ Skip to end of line """
        p = multifind(self.data, [ "\n", "\r" ], self.pos)
        if p != -1:
            p = p + 1
        else:
            p=len(self.data)
        self.pos = p


    def SkipCComment(self):
        """ Skip to */ """
        p = string.find(self.data, "*/", self.pos + 1)
        if p != -1:
            p = p + 2
        self.pos = p
        

    def ReadString(self):
        """Read a string from data"""
        # FIXME: Should really understand \n \t \" etc.
        begin = self.pos + 1
        SkipStr(self)
        end = self.pos - 1
        str = self.data[begin:end]
        str = replace(str,"\\\\","\\")
        return str

    def ParsePP(self):
        p=self.pos
        self.SkiptoEOL()
        block = self.data[p+1:self.pos]
        block = string.split(block,"//",1)[0]
        block = string.split(block,"/*",1)[0]

        block=string.strip(block)
        if block[:7] == "include":
            self.includes.append(string.lstrip(block[7:]))

    def skipinclude(self):
        self.pos = self.pos + 7


    def __init__ (self, d, name):
        self.data=d
        self.name = name

    def ParseCFile(self):

        self.pos=0
        self.includes=[]

        finders = [
            Finder("include",self.skipinclude),
            Finder("#",self.ParsePP),
            Finder('"',self.SkipStr),
            Finder("'",self.SkipSingleQuotedChar),
            Finder("//",self.SkiptoEOL),
            Finder("/*",self.SkipCComment)
            ]

        while finders[0].pos < len(self.data):
            bestfinder = None
            bestpos = len(self.data)

            for f in finders:
                if f.pos < self.pos:
                    f.pos = string.find(self.data, f.token, self.pos)
                    if f.pos == -1:
                        f.pos = len(self.data)
                if f.pos < bestpos:
                    bestfinder = f
                    bestpos = f.pos

            if not bestfinder:
                break
            self.pos = bestpos
            bestfinder.cb()
            if self.pos == -1:
                print "mkdepend: Parse error (%s) in %s byte %d" % \
                      (bestfinder.token, self.name, bestpos)
                sys.exit(1)

        return self.includes
        

#
# This function needs more error checking
#
def ReadSourceFile(filename):
    """Read a c/c++/h file and return a list of files included"""
    global dependency_cache

    try:
        deps = dependency_cache[filename];

    except KeyError:
        #print "ReadSourceFile(%s)" % filename

        deps = [ filename ]
        dependency_cache[filename] = deps

        try:
            file=open(filename,"r")
        except IOError:
            print "mkdepend: Failed to open file %s\n" % filename
            return deps

        for inc in FileParser( file.read(), filename ).ParseCFile():
            for f in ReadIncludeFile(inc, filename):
                if not f in deps:
                    deps.append(f)

        dependency_cache[filename] = deps
        
    return deps

def ReadIncludeFile(file, from_file):
    global include_list
    if len(file) < 1:
        return []
    if file[0] == '"':
        filename = string.split(file,'"')[1]
    elif file[0] == "<":
        #return []
        filename = string.split(file,'<')[1]
        filename = string.split(filename,'>')[0]
    else:
        return []

    for dir in [os.path.dirname(from_file) ] + include_list:
        f = os.path.join(dir, filename)
        if os.path.isfile(f):
            return ReadSourceFile(f)

    #print "%s: Include file %s not found" % (from_file, file)
    return []


def MakefileRuleList(obj_path, hf_list):
    mfr_list = []
    for hf in hf_list:
        mfr_list.append('%s: %s\n' % (obj_path, quote_dep(hf)))
    return mfr_list


mtime_cache = {}
def mtime(filename):
    if mtime_cache.has_key(filename):
        return mtime_cache[filename]
    try:
        ret=os.stat(filename)[stat.ST_MTIME]
    except OSError:
        ret=0

    mtime_cache[filename]=ret
    return ret


def parse_args(argv):
    global include_list
    global prefix_includes
    global define_list
    global object_ext

    targ = ""
    marg = None
    arg_list = []

    state = ""
    for arg in argv:
        if state == "define":
            define_list.append(arg)
            state = ""
            continue

        if state == "include":
            include_list.append(arg)
            state = ""
            continue

        if state == "preinc":
            prefix_includes.append(arg)
            state = ""
            continue

        if state == "targ":
            targ = arg
            continue

        if state == "marg":
            marg = arg
            continue

        if state == "oarg":
            object_ext = arg
            continue

        if arg == "--gnumake":
            global gnumake
            gnumake = 1
            continue

        if arg[:2] in ["-D", "/D"]:
            if len(arg) > 2:
                define_list.append(arg[2:])
            else:
                state = "define"

        elif arg[:2] in ["-I", "/I", "-J", "/J"]:
            if len(arg) > 2:
                include_list.append(arg[2:])
            else:
                state = "include"
        
        elif arg[:2] in ["-t", "/t"]:
            if len(arg) > 2:
                targ = arg[2:]
            else:
                state = "targ"

        elif arg[:2] in ["-m", "/m"]:
            if len(arg) > 2:
                marg = arg[2:]
            else:
                state = "marg"

        elif arg[:2] in ["-o", "/o"]:
            if len(arg) > 2:
                object_ext = arg[2:]
            else:
                state = "marg"

        elif arg[:3] in ["-FI", "/FI"]:
            if len(arg) > 3:
                prefix_includes.append(arg[3:])
            else:
                state = "preinc"

        elif arg == "-include":
            state = "preinc"

        else:
            arg_list.append(arg)

    return targ, marg, arg_list

def run():
    global include_list
    global gnumake

    ## Que?
    sys.setcheckinterval(50)

    magic = "# Dependency magic by $Id: mkdepend_exe.py,v 1.2 2006/07/06 19:28:05 jfinnecy Exp $\n"
    obj_path, makefile, source_list = parse_args(sys.argv[1:])

    if makefile:
        if not os.path.isfile(makefile):
            print "Makefile=\"%s\" not found." % (makefile)
            sys.exit(1)
    else:
        if not gnumake:
            print "No Makefile specified."
            sys.exit(1)

    init_platform()
    
    if makefile:
        makefile_data = open(makefile, "r").read()
        if gnumake and string.find(makefile_data,magic)!=-1:
            print "Dependencies have already been updated automagically."
            sys.exit(0)
        
    # Get system include path (Windows way)
    #incpath=os.environ.get("include")
    #if incpath:
    #    include_list = include_list + string.split(incpath,';')

    #incpath=os.environ.get("C_INCLUDE_PATH")
    # Get system include path (UNIX way)
    #if incpath:
    #    include_list = include_list + string.split(incpath,':')

    mrlist = [
        "# DO NOT DELETE -- mkdepend depends on this line\n"
        ]
    extentions={".c":1, ".cc":1, ".cxx":1 }
    all_files={}
    for source in source_list:
        (base, ext) = os.path.splitext(source)
        extentions[ext]=1

        base = os.path.normpath(base)
        if os.path.isabs(base):
            base = string.replace(base,"\\","_")
            base = string.replace(base,":","_")
            base = string.replace(base,"/","_")
        else:
            ## Trim off any ../ from the beginning of the path
            base = string.replace(base,"\\","/")
            tmp = string.split(base, "/")
            x = 0
            while len(tmp) > x + 1:
                if tmp[x] in [ os.curdir, os.pardir ]:
                    x = x + 1
                else:
                    break
            if x:
                tmp = [ "par%d" % x ] + tmp[x:]
            base = string.join(tmp,os.sep)

        path = os.path.join(obj_path, base)

        if string.lower(ext) == ".rc":
            obj = "%s.%s" % (path, "res")
        else:
            obj = "%s.%s" % (path, object_ext)

        dep = "%s.%s" % (path, "dep")

        deptime = mtime(dep)

        if deptime:
            tmp_rules=open(dep,"r").readlines()

            mt=mtime(dep+"-flist")
            if mt and mt <= deptime:
                hlist=string.split(open(dep+"-flist","r").read(),"\n")
            else:
                hlist = []
                for l in tmp_rules:
                    l = string.split(l,":")

                    ## Patchlet for stupid windows path names
                    if len(l)>1 and len(l[0]) == 1:
                        l = [ l[0] +":"+l[1] ] + l[2:]

                    if len(l):
                        file = string.strip(string.join(l[1:],":"))
                        file = unquote_dep(file)
                        hlist.append(file)

            for file in hlist:
                #print "Checking times for %s (%s >? %s)" % (repr(file), repr(mtime(file)), repr(deptime))
                mt = mtime(file)
                if mt > deptime or mt == 0:
                    deptime = 0
                    break


        if not deptime:
            hlist = ReadSourceFile(source)
            tmp_rules=None

        global prefix_includes
        for pi in prefix_includes:
            if pi not in hlist:
                hlist.extend( ReadSourceFile(pi) )
                tmp_rules=None
    
        if gnumake:
            obj = "%s %s.dep" % (obj, obj_path)

        if tmp_rules:
            tmp=tmp_rules
        else:
            tmp = MakefileRuleList(obj, hlist)

        if not deptime:
            utils.mkdirTree(path)
            open(dep+"-flist", "w").write(string.join(hlist, "\n"))
            open(dep, "w").write(string.join(tmp, ""))

        if not gnumake:
            mrlist.extend(tmp)

        for file in hlist:
            all_files[file]=1

    if not makefile:
        sys.exit(0)
        
    if gnumake:
        tmplist=[]
        p = obj_path
        while p not in [ "", ".", "..", ":" ]:
            tmplist.append('\t@test -d %s || mkdir %s\n' % (p,p))
            p = os.path.dirname(p)

        tmplist.reverse()
        tmplist.append('\t%s %s --gnumake -t%s $?\n' % (sys.executable, sys.argv[0], obj_path))

        mrlist.append(magic)
        mrlist.append('ifneq ($MAKECMDGOALS),clean)\n')
        mrlist.append('-include $(COMPILED_OBJS:.%s=.dep)\n' % object_ext)
        mrlist.append('endif\n')
        for ext in extentions.keys():
            mrlist.append('\n')
            mrlist.append('%s/%%.dep: %%%s\n' % (obj_path, ext))
            mrlist.extend(tmplist)
        mrlist.append('\n')

    mrlist.append('\n')

    fil = open(makefile, "w")
    fil.write(makefile_data[:string.find(makefile_data,"# DO NOT DELETE")])
    fil.write(string.join(mrlist,""))
    fil.close()

    fil = open(makefile+"-mkdep.o","w")
    fil.write(string.join(all_files.keys(),"\n"))
    fil.close()

    sys.setcheckinterval(10)
    sys.exit(0)

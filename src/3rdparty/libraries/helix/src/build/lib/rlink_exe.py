#!/usr/bin/env python
# -*- Mode: Python -*-
# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: rlink_exe.py,v 1.3 2007/04/30 22:51:13 jfinnecy Exp $ 
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


##
## What it does:
##   Rlink emulates the broken --gc-sections ld option and allows
##   for some fairly significant code size reductions by eliminating
##   any functions you don't actually use from the final binary.
##   You will need to give the flags -ffunction-sections and
##   -fdata-sections for this to have any noticable effect.
##
## How it works:
##   To achive this, rlink runs objdump on all object files (even
##   those embedded in archives). From the objdump output, information
##   about relocations, symbols and sections is extracted and used to
##   find out which sections aren't needed. It then proceeds to use
##   objcopy to remove those sections.
##
## Control & Debugging
##   There are two environment variables which can be used to control
##   Rlink's behaviour:
##     RLINK_DEBUG: if this is set, rlink will print *lots* of debug
##     RLINK_DISABLE: if this is set, rlink will do nothing 
##
## TODO
##   Make rlink still perform library merging when RLINK_DISABLE is on
##   so that it can replace armerge.
##
##  -Hubbe
##

import os
import sys
import string
import re
import types
import stat
import getopt

import shell


global_symbols={}
all_objects=[]
output_name = "a.out"
platform_prefix=""

if os.environ.get("RLINK_DEBUG"):
    def debug(s,*args):
        print s % args
else:
    def debug(*args):
        pass

    
if os.name == "posix":
    maxcmdlen=100000

    ## FIXME
    def quote(s):
        if re.match("^[-a-zA-Z0-9./_=]+$", s):
            return s
        return '"' + s + '"'
else:
    maxcmdlen=4000

    def quote(s):
        if re.match("^[-a-zA-Z0-9./_=$\\\\]+$", s):
            return s
        return '"' + s + '"'


def run1(cmd, no_prefix=False):
    if not no_prefix:
        cmd = "%s%s" % (platform_prefix, cmd)
    debug("rlink, running: %s" % cmd)
    (e, out) = shell.run(cmd)
    if string.strip(out) != "":
        print out
    if e:
        print "Cmd '%s' failed with error code %d" % (cmd, e)
        sys.exit(1)

def run_noexit(cmd):
    cmd = "%s%s" % (platform_prefix, cmd)
    debug("rlink, running: %s" % cmd)
    (e, out) = shell.run(cmd)
    if string.strip(out) != "":
        print out
    if e:
        ## Only print verbose message if debug is enabled, since
        ## it typically isn't an error
        debug("Cmd '%s' failed with error code %d" % (cmd, e))
    
def run2(args):
    run1(string.join(map(quote,args)), True)

class Section:
    def __init__(self, name, o=None):
        self.mark=0
        self.name=name
        self.relocs=[]  ## list of symbol names
        self.object=o

class Symbol:
    def __init__(self, name):
        self.done=0
        self.name=name
        self.sections=[] ## List of all sections where this is defined


class Object:
    def __init__(self, name):
        debug("rlink, parsing %s",name)
        self.name=name
        self.sections=[]
        self.sections_byname={}
        self.symbols={}
        self.parse()


    def translate(self, to):
        debug("rlink, translating %s => %s", self.name, to)
        cnt=1
        cmd="objcopy"
        from_to=' %s %s' % (quote(self.name),quote(to))
        mlen=maxcmdlen - len(from_to)
        for x in self.sections:
            if not x.mark:
                ncmd=cmd+' %s' % quote("-R"+x.name)
                if len(ncmd) >= mlen:
                    tmp='%s.%d.o' % (to, cnt)
                    cnt = cnt+1
                    run_noexit('%s %s' % (cmd, from_to));
                    ncmd='objcopy %s' % quote("-R"+x.name)
                    from_to=' %s' % (quote(to))
                    mlen=maxcmdlen - len(from_to)
                cmd=ncmd

        run_noexit('%s %s' % (cmd, from_to));

        if not os.path.exists(to):
            print "Translation of %s failed (file ignored)" % self.name
            return 0
        return 1

    def parse(self):
        data=os.popen('objdump -w -h -r -t %s' % quote(self.name), "r").read()
        data=string.replace(data,"\r\n","\n")

        secstart=string.find(data,"\nIdx")
        secstart=string.find(data,"\n",secstart+1)+1
        secend=string.find(data,"\nSYMBOL TABLE:",secstart)

        symstart=string.find(data,"\n",secend+1)+1
        symend=string.find(data,"\nRELOCATION RECORDS FOR")

        self.read_sections(data[secstart:secend])
        self.read_symbols(data[symstart:symend])
        self.read_relocs(data[symend:])


    def read_sections(self, data):
        for l in string.split(data,"\n"):
            #debug("read_sections: %s", l)
            w=string.split(l)
            s=Section(w[1], self)
            self.sections.append(s)
            self.sections_byname[w[1]]=s

    def read_symbols(self, data):
        for l in string.split(data,"\n"):
            debug("read_symbols2: %s", l)

            if l == 'no symbols':
                return;

            w=string.split(l)

            if len(w) < 2:
                continue

            sec=None
            symname=None
            status=None
            
            if l[0] == '[':
                # PE STYLE
                sec_num=string.split(l,"(sec")[1]
                sec_num=string.split(sec_num,")")[0]
                sec_num=int(sec_num)

                scl=string.split(l,"(scl")[1]
                scl=string.split(scl,")")[0]
                scl=int(scl)

                # C_EXT, C_WEAKEXT, C_THUMBEXT, C_THUMBEXTFUNC,
                # C_HIDEXT, C_SYSTEM
                if scl in [ 2, 127, 130, 150, 107, 23 ]:
                    status="global"

                # C_STAT, C_LEAFSTAT, C_LABEL
                # C_THUMBSTAT, C_THUMBLABEL, C_THUMBSTATFUNC
                if scl == [ 3, 6,  113, 131, 134, 151]:
                    status = "local"
                    
                symname=string.split(l,")")[-1]
                symname=string.split(symname)[-1]

                if sec_num:
                    sec=self.sections[sec_num-1]
                else:
                    status="undefined"
            else:
                # ELF style
                tmp=string.split(l[17:])
                sec_name=tmp[0]
                sec=self.sections_byname.get(sec_name)
                if len(tmp) >= 3:
                    symname=tmp[2]
                else:
                    symname=sec_name
                if l[9] == 'l':
                    status="local"
                if l[9] == 'g' or l[10] == 'w':
                    status="global"
                if sec_name == "*UND*":
                    status="undefined"

            if symname and ' ' in symname:
                print "BOGUS SYMBOL NAME: %s" % repr(symname)
                sys.exit(3)

            ## Work around weirdness
            if symname[:14] == ".gnu.linkonce.":
                symname=symname[16:]

            debug("section: %s symbol: %s status: %s", repr(sec and sec.name), repr(symname), repr(status))

            if sec:
                if status == "global":
                    s=global_symbols.get(symname)
                    if not s:
                        s=Symbol(symname)
                        global_symbols[symname]=s
                        
                    self.symbols[symname]=s
                    s.sections.append(sec)
                else:
                    s=Symbol(symname)
                    self.symbols[symname]=s
                    self.symbols[symname].sections.append(sec)


    def read_relocs(self, data):
        for l in string.split(data,"\n"):
            debug("read_relocs: %s", l)
            w=string.split(l)
            if len(w) < 3:
                continue

            if w[0] == "RELOCATION":
                sec=w[3][1:-2]
                current_sec=self.sections_byname[sec]
                continue
            
            if w[0] == "OFFSET":
                continue

            if not current_sec:
                continue

            sym=w[2]
            sym=string.split(sym,"+")[0]
            current_sec.relocs.append(self.symbols.get(sym,sym))

        

def mark(exported_symbols):
    next_batch = exported_symbols[:]

    while len(next_batch):
        batch=next_batch
        next_batch=[]
        for sym in batch:
            if type(sym) == types.StringType:
                symbol=global_symbols.get(sym)
                if symbol:
                    debug("GSYM: %s",repr(sym))
                else:
                    debug("USYM: %s",repr(sym))
            else:
                symbol = sym
                debug("LSYM: %s",repr(symbol.name))

            if not symbol or symbol.done:
                continue

            for sec in symbol.sections:
                debug("%s => %s:%s", symbol.name, sec.object.name, sec.name)
                if not sec.mark:
                    sec.mark=1
                    next_batch.extend(sec.relocs)

            symbol.done=1

def mangle(filename):
    filename=string.replace(filename,":","_")
    filename=string.replace(filename,"/","_")
    filename=string.replace(filename,"\\","_")
    return filename
    

def mangle_obj(filename):
    return "rlink/"+mangle("@OUTPUT_NAME@_"+ filename)

def mangle2(s):
    s=string.replace(s,"@OUTPUT_NAME@",mangle(output_name))
    return s

def rdel(f):
    #debug("rdel(%s)",f)
    if os.path.isdir(f):
        for l in os.listdir(f):
            rdel(os.path.join(f,l))
        os.rmdir(f)
    elif os.path.exists(f):
        os.unlink(f)


##
## This is a rather stupid link script reader
##
def read_linker_script(file):
    syms=[]
    data=open(file,"r").read()

    if data[:7] == "VERSION":
        tmp=string.split(data,"global:",1)[1]
        tmp=string.split(tmp,"local:",1)[0]
        for t in string.split(tmp,";"):
            t = string.strip(t)
            if t != "":
                syms.append(t)
    else:
        tmp=string.split("EXTERN")
        for x in tmp[1:]:
            i1=string.find(x,"(")
            if i1==-1:
                continue
            i2=string.find(x,")",i1)
            if i2==-1:
                continue
            syms.append(string.split(x[i1+1:i2-1]))

    return syms

def usage():
    print "rlink.py [-x platform_prefix] linker_command linker_arguments "
    sys.exit(1)

def juggle_args(args):
    global output_name
    
    if not os.path.exists('rlink'):
        os.mkdir('rlink')

    objects = []
    archives = []
    expfiles = []
    ofiles = []
    symbols = [ "_start", "E32Dll__F10TDllReason","main","__init","__exit" ]
    newargs = args[:1]

    state = None

    for a in args[1:]:
        if state:
            if state == "SYMBOL":
                symbols.append(a)
            
            if state == "OUTPUT":
                output_name = a

            state = None
            newargs.append(a)
            continue

        if a[0] == '-':
            if a[:2] == '-u':
                if a[2:]:
                    symbols.append(a[2:])
                else:
                    state="SYMBOL"

            if a[:2] == '-o':
                debug("-o found!")
                if a[2:]:
                    output_name=a[2:]
                else:
                    state="OUTPUT"
                debug("state=%s" % repr(state))

            if a[:12] == '--undefined=':
                symbols.append(a[12:])

            newargs.append(a)
            continue


        al=string.lower(a)
        ext=string.split(al,".")[-1]

        if ext == 'o' or ext == 'obj':
            objects.append(a)
            newargs.append(mangle_obj(a))
            ofiles.append(mangle_obj(a))
            continue

        if ext == 'a' or ext == 'lib':
            ## FIXME: This isabs is actually not supposed to be here,
            ## without it, this program becomes extremely painfully slow
            ## when linking symbian binaries. A neater solution would be
            ## very handy..
            if not os.path.isabs(a):
                archives.append(a)
                xa="rlink/@OUTPUT_NAME@_archives.a"
                try:
                    newargs.remove(xa)
                except ValueError:
                    ofiles.append(xa)
                    pass
                newargs.append(xa)
                continue

        ## Read all relocs in an .exp file
        ## Hopt this is a reasonably correct way to handle these files..
        if ext == 'exp':
            expfiles.append(a)


        if ext == 'lnk':
            ## Linker script
            symbols.extend(read_linker_script(a))

        newargs.append(a)

    print "RLINK: Reading objects..."

    for a in expfiles:
        if open(a,"r").read(7) == "VERSION":
            symbols.extend(read_linker_script(a))
        else:
            for sec in Object(a).sections:
                sec.mark=1
                symbols.extend(sec.relocs)

    ## Read objects
    for o in objects:
        all_objects.append(Object(o))

    print "RLINK: Reading archives..."

    ## Read archives
    aobj = []
    cdir=os.getcwd()
    for a in archives:
        tmpdir=mangle2("rlink/rlink_@OUTPUT_NAME@_"+mangle(a)+"_dir")
        debug("PWD=%s", os.getcwd())
        rdel(tmpdir)
        os.mkdir(tmpdir)
        os.chdir(tmpdir)
        archive_path = os.path.join(cdir, a)
        run1("ar x %s" % quote(archive_path))
        os.chdir(cdir)
        for x in os.listdir(tmpdir):
            oname=os.path.join(tmpdir, x)
            all_objects.append(Object(oname))
            aobj.append(mangle_obj(oname))

    print "RLINK: Computing dependencies..."

    for o in all_objects:
        for sec in o.sections:
            if sec.name in [".ctors",".dtors"]:
                sec.mark=1
                symbols.extend(sec.relocs)

    mark(symbols)

    # sweep
    print "RLINK: Translating object files..."

    for o in all_objects:
        if not o.translate(mangle2(mangle_obj(o.name))):
            try:
                aobj.remove(mangle_obj(o.name))
            except:
                newargs.remove(mangle_obj(o.name))

    ## re-create archives
    atmp=mangle2("rlink/@OUTPUT_NAME@_archives.a")
    rdel(atmp)
    xcmd="ar cq %s" % quote(atmp)
    cmd=xcmd
    for f in aobj:
        f=quote(mangle2(f))
        if len(cmd) + len(f) + 1 >= maxcmdlen:
            run1(cmd)
            cmd=xcmd
        cmd = cmd + " " + f

    run1(cmd)
    run1("ranlib %s" % quote(atmp))
    #newargs.append(atmp)

    return map(mangle2, newargs)
    

def run():
    global platform_prefix
    args=sys.argv[1:]

    try:
        (opts, args) = getopt.getopt(sys.argv[1:], ":x:")
    except getopt.error:
        usage()
    if opts: 
        platform_prefix = opts[0][1]

    if not os.environ.get("RLINK_DISABLE"):
        args = juggle_args(args)
    run2(args)
    ## Delete temp files?
    sys.exit(0)

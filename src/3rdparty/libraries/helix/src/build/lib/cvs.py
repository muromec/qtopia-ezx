# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: cvs.py,v 1.53 2007/04/30 22:51:13 jfinnecy Exp $ 
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
"""Multi-platform classes to checkout source code from CVS.  This module
automatically determines the platform and instances the correct platform
specific CVS class.  Only the entry points Checkout and Update are meant to
be used."""

import os
import sys
import string
import types
import time

import err
import shell
import outmsg
import ascript
import outmsg

import re

import log
log.debug( 'Imported: $Id: cvs.py,v 1.53 2007/04/30 22:51:13 jfinnecy Exp $' )
import utils

num_cvs_retries=10

def fix_tag(str):
    """Return (tag, timestamp) given a tag or timestamp"""
    if " " in str:
        return ( "", str )
    if "/" in str:
        return ( "", str +" 00:00:00" )

    return (str, "")
        
def listify(string_or_list):
    """Takes a string or list of strings as input, and returns a list
    of strings."""

    list = None

    if type(string_or_list) == types.ListType:
        list = string_or_list
    else:
        list = [string_or_list]

    ## filter list to have only unique items
    for item in list:
        while list.count(item) > 1:
            list.remove(item)

    return list


class CVS:
    """Abstract CVS class."""

    def update_checkout_list(self, tag, module_list, az, dir):
        update_list = []
        checkout_list = []

        if tag == "HEAD":
            tag = ""

        ## separate the modules which need to be checked out
        ## from the modules which need to be updated
        ## and go through the list of modules to be updated, and
        ## check that the current module's CVS/Tag entry is the same
        ## before updating; if it's not, then throw a error and let
        ## the developer handle it
        update_error_list = []
        for module in module_list:
            opath = module

            if dir:
                opath = os.path.join(dir, module)

            if az:
                opath = az

            if (not os.path.isdir(opath) or
                not os.path.isdir(os.path.join(opath, "CVS"))):
                checkout_list.append(module)
                continue

            tag_path = os.path.join(opath, "CVS", "Tag")
            try:
                current_tag = string.strip(open(tag_path).read())
            except IOError:
                # Special case
                if module == ".":
	            checkout_list.append(module)
	            continue
                current_tag = ""

            update_list.append(module)
                
            if current_tag in [  "HEAD", "THEAD", "NHEAD" ]:
                current_tag = ""

            tag_match = 0

            ## some versions of CVS put a "N" or "T" before the actual tag
            if re.match(r'^[NT]?' + tag, current_tag):
                tag_match = 1

            if not tag_match:
                update_error_list.append(
                    "module=\"%s\" current tag=\"%s\" dir=\"%s\"" % (module, current_tag, opath))

        ## now error out if there were conflicts
        if update_error_list:
            e = err.Error()
            e.Set("There are CVS modules in your source tree which "\
                  "were originally checked out from a different CVS "\
                  "branch than the current .bif file is requesting.  "\
                  "This is most likely because of a change in the .bif "\
                  "file.  You will need to remove or move these modules "\
                  "by hand before the build system can continue.\n%s" % (
                string.join(update_error_list, "\n")))
            raise err.error, e

        return update_list, checkout_list

    def Checkout(self, tag, module_list, az = None, timestamp = None, nonrecursive = 0, dir = None):
        """Given a CVS tag and a list of CVS modules, check them out."""
        log.trace( 'entry' , [ tag , module_list , az , timestamp , nonrecursive , dir ] )
        #print "cvs.Checkout(%s,%s,az=%s,timestamp=%s, %s, %s)" % (
        #    repr(tag),repr(module_list),
        #    repr(az),repr(timestamp), repr(nonrecursive), repr(dir))

        module_list = listify(module_list)

        if az:
            if len(module_list) > 1:
                print "Cannot checkout two modules as the same name"
                sys.exit(1)
            
        update_list,checkout_list = self.update_checkout_list(tag, module_list, az, dir)

        #print "UPDATE_LIST=%s" % repr(update_list)
        #print "CHECKOUT_LIST=%s" % repr(checkout_list)

        if update_list:
            self.update(tag, update_list, az, timestamp, nonrecursive, dir)
        if checkout_list:
            self.checkout(tag, checkout_list, az, timestamp, nonrecursive, dir)

        log.trace( 'exit' )
        
        
    def get_viewcvs_url(self, path):
        if not self.viewcvs:
            return None
        return self.viewcvs + path


class UNIXCVS(CVS):

    def __init__(self, root, shadow = None, viewcvs = None):
        self.root = root
        self.shadow = shadow
        self.viewcvs = viewcvs

    def get_root(self, module):
        return self.root

    def Cmd(self, cmd, path, dir = None):
        log.trace( 'entry' , [ cmd ] )
        cmd='cvs -d "%s" %s "%s"' % (self.root, cmd, path)
        log.info("running %s in %s + %s" % (repr(cmd), repr(os.getcwd()), repr(dir)))
        results = shell.run(cmd, dir = dir, timeout_seconds = 1800)
        resList = []
	for item in results:
	    resList.append(item)
        log.trace( 'exit' , [ resList ]  )
        return results

    def Status(self, path, dir=None):
        return self.Cmd("status",path, dir)

    def Commit(self, path, message, dir=None):
        return self.Cmd('commit -m "%s"' % message, path, dir)

    def Tag(self, path, tag, dir=None):
        return self.Cmd('tag "%s"' % tag, path, dir)

    def update(self, tag, module_list, az = None, timestamp = None, nonrecursive = 0, checkout_dir = None):
        # print "TAG = %s" % tag
        # print "TIMESTAMP = %s" % timestamp

        if checkout_dir == None:
            checkout_dir = os.curdir
            
        cmd = "cvs"
        
        if self.root:
            if self.shadow:
                cmd = "%s -d %s" % (cmd, self.shadow)
            else:
                cmd = "%s -d %s" % (cmd, self.root)

        cmd = "%s checkout" % cmd

        if nonrecursive:
            cmd = cmd + " -l"

        if len(tag):
            if tag == "HEAD":
                cmd = "%s -A" % cmd
            else:
                cmd = '%s -r "%s"' % (cmd, tag)

        if timestamp:
            cmd = '%s -D "%s"' % (cmd, timestamp)

        out_dirs = []
        for x in module_list:
            #print "%s" % repr( [checkout_dir] + string.split(x,"/"))
            out_dirs.append(apply(os.path.join, [checkout_dir] +
                                  string.split(x,"/")))

        as_arg = ""
        if az:
            dir, base = os.path.split(az)
            if dir:
                utils.mkdirTree(dir)
                checkout_dir = dir
            cmd = "%s -d %s" % (cmd , base)
            out_dirs = [az]

        cmd = "%s %s" % (cmd, string.join(module_list))
        outmsg.verbose("running %s in %s + %s (as = %s)" % (repr(cmd), repr(os.getcwd()), repr(checkout_dir), repr(az)))
        def line_cb(line):
            outmsg.verbose(string.strip(line))

        # Tenacious cvs checkout loop, tries <num_cvs_retries> times before
        # failing.
        for N in range(0, num_cvs_retries):
            retcode, output = shell.run(cmd, line_cb, dir = checkout_dir)
            if retcode:
                # If we have a fatal error as determined by error output 
                # matching any of the following strings, then we will abort out 
                # of the retry loop.
                if string.find(output,"cannot find module") != -1:
                    break
                if string.find(output,"cannot expand modules")!=-1:
                    break
                if string.find(output,"cannot open CVS/Entries for reading")!=-1:
                    break
                if string.find(output,"Cannot access ")!=-1:
                    break
                if string.find(output,"Can't parse date/time")!=-1:
                    break
                if string.find(output,"is modified but no longer in the repository")!=-1:
                    break
                if string.find(output,"\nC ")!=-1:
                    break
                # We didn't abort, so we'll assume non-fatal error, log the
                # error, and try again.
                log.info( "CVS command failed with error code %d, trying again in a few seconds." % retcode )
                time.sleep(1 + N*5)
            else:
                break

        dirs = out_dirs[:]
        for dir in dirs:
            if os.path.isdir(dir):

                ## Create a timestamp file
                timestamp=os.path.join(dir, "CVS", "timestamp")
                shell.rm(timestamp)
                try:
                    open(timestamp,"w").write(str(int(time.time())))
                except IOError:
                    continue

                if self.shadow:
                    for subdir in os.listdir(dir):
                        if string.lower(subdir) != "cvs":
                            subdir=os.path.join(dir, subdir)
                            if os.path.isdir(subdir):
                                dirs.append(subdir)

                    rootfile = os.path.join(dir, "CVS", "Root")
                    open(rootfile,"w").write("%s\n" % self.root)

        #print "ODIRS: %s" % repr(out_dirs)
        return out_dirs

    ## Thread safe checkout (hopefully)
    def checkout(self, tag, module_list, az = None, timestamp = None, nonrecursive = 0, checkout_dir = None):
        log.trace( 'entry' , [ tag , module_list , az , timestamp , nonrecursive , checkout_dir ] )
        import distributions
               
        if not checkout_dir:
            checkout_dir="."

        if az:
            tmpdirbase=os.path.join(checkout_dir, os.path.dirname(az), "cvs")
        else:    
            tmpdirbase=os.path.join(checkout_dir, "cvs")
        
        distributions.setup(tmpdirbase)
        tmpdir=distributions.tmpdir(tmpdirbase)

        if az:
            t=os.path.join(tmpdir,"tmp")
            self.update( tag, module_list,t , timestamp, nonrecursive)
            if os.path.exists(t):
                utils.mkdirTree(os.path.dirname(az))
                outmsg.verbose("Moving %s to %s" % (t,az))
                shell.move(t, az)
        else:
            out_dirs=self.update( tag, module_list, az, timestamp, nonrecursive, tmpdir)
            #print out_dirs
            for d in out_dirs:
                rl=os.path.join(checkout_dir, d[len(tmpdir)+1:])
                if os.path.exists(d):
                    outmsg.verbose("Moving %s to %s" % (d, rl))
                    utils.mkdirTree(os.path.dirname(rl))
                    if d[len(tmpdir)+1:] == ".":
                        for tmp in os.listdir(d):
                            outmsg.verbose("Moving %s to %s" % (os.path.join(d,tmp), os.path.join(rl, tmp)))
                            shell.move(os.path.join(d,tmp), os.path.join(rl, tmp))
                    else:
                        shell.move(d, rl)
                else:
                    print "%s does not exists" % d

        distributions.cleanup(tmpdirbase)
        log.trace( 'exit' )

class UNIXWinCVS(UNIXCVS):
    def update(self, tag, module_list, az = None, timestamp = None, nonrecursive = 0, dir = None):
        step=20
        ret=[]
        for d in range(0, len(module_list), step):
            ret.extend(UNIXCVS.update(self,tag,module_list[d:d+step],az,timestamp,nonrecursive, dir))
        return ret

class Win9xCVS(UNIXCVS):
    def update(self, tag, module_list, az = None, timestamp = None, nonrecursive = 0, dir = None):
        step=3
        ret=[]
        for d in range(0, len(module_list), step):
            ret.extend(UNIXCVS.update(self,tag,module_list[d:d+step],az,timestamp,nonrecursive, dir))
        return ret


class MacCVS(CVS):
    def __init__(self, cvssession, shadow = None, viewcvs = None):
        self.viewcvs=viewcvs
        if os.environ.has_key('MACCVS_PATH'):
            self.cvs_path = os.environ['MACCVS_PATH']
        else:
            e = err.Error()
            e.Set("You need to set the MACCVS_PATH environment variable "\
                  "to the path of MacCVS.")
            raise err.error, e

        if cvssession:
            self.cvs_session_path = cvssession
        else:
            e = err.Error()
            e.Set("You need to set the CVSSESSION_PATH environment variable "\
                  "to the path of the MacCVS session file.")
            raise err.error, e

        if os.environ.has_key('CVSSCRIPT_PATH'):
            self.script_save_path = os.environ['CVSSCRIPT_PATH']
        else:
            self.script_save_path = ''

    def update(self, tag, module_list, az = None, timestamp = None, nonrecursive = 0, dir = None):

        ### FIXME:
        ### test this!

        if dir:
            odir=os.getcwd()
            try:
                os.chdir(dir)
                ret=self.checkout(tag, module_list, az, timestamp, nonrecursive)
            finally:
                os.chdir(odir)

            return ret

        if az:
            odir=os.getcwd()
            shell.mkdir("cvs_temp")
            os.chdir("cvs_temp")
            self.checkout(tag,module_list, None, timestamp, nonrecursive)
            os.chdir(odir)
            for m in module_list:
                shell.cp(os.path.join(odir, "cvs_temp", m),
                         os.path.join(odir, az))

            shell.rm("cvs_temp")
            return
        
        if nonrecursive:
            e = err.Error()
            e.Set("Nonrecursive not supported by MacCVS")
            raise err.error, e
        
        session_name = self.cvs_session_path[1:-1]
        session_name = os.path.basename(session_name)

        script = ascript.CreateAppleScript(
            'tell application %s' % (self.cvs_path),
            '  activate',
            '  open alias %s' % (self.cvs_session_path),
            '  set thesession to session named "%s"' % (session_name),
            '  set local root of thesession to alias "%s"' % (os.getcwd()),
            '  with timeout of 99999 seconds')

        for module in module_list:
            cmd = 'check out thesession module "%s"' % (module)
            if tag:
                cmd = cmd +' revision "%s"' % (tag)

            if timestamp:
                cmd = cmd +' date "%s"' % (timestamp)

            script.Append(cmd)

        script.Append(
            '  end timeout',
            '  quit',
            'end tell')

        if self.script_save_path == '':
            result = script.CompileAndExecute()
        else:
            script.CompileAndSave(self.script_save_path)

            launch_script = ascript.CreateAppleScript(
                'set scriptobj to (load script file "%s")' % (
                self.script_save_path),
                'tell scriptobj',
                '  run',
                'end tell')

            result = launch_script.CompileAndExecute()

        if result and result != '':
            outmsg.error('cvs checkout error %s' % (result))



class MultiCVS:
    def __init__(self, root, shadow = None, viewcvs = None):
        self.root = root
        if self.root[-1] != '/':
            self.root = self.root + "/"
        self.shadow = shadow
        self.viewcvs=viewcvs

    def fix(self, path, dir = None):
        tmp=string.split(path,"/")
        base=tmp[0]
        path=string.join(tmp[1:],"/")
        if path == "":
            path = "."

        if dir:
            dir=os.path.join(dir,base)
        else:
            dir=base

        return (base, path, dir)

    def get_root(self, module):
        tmp=string.split(module,"/")
        base=tmp[0]
        return self.root + base

    def Status(self, path, dir=None):
        base, path, dir = self.fix(path, dir)
        return _Cvs(self.root + base).Status(path,dir)

    def Commit(self, path, message, dir=None):
        base, path, dir = self.fix(path, dir)
        return _Cvs(self.root + base).Commit(path,message,dir)

    def Tag(self, path, message, dir=None):
        base, path, dir = self.fix(path, dir)
        return _Cvs(self.root + base).Tag(path,message,dir)

    def Checkout(self, tag, module_list, az=None, timestamp=None, nonrecursive=0):
        tmp={}


        for m in module_list:
            base, mod, dir = self.fix(m, ".")

            if tmp.has_key(base):
                tmp[base].append( mod )
            else:
                tmp[base]=[mod]

        for base in tmp.keys():
            ns=None
            if self.shadow:
                ns=self.shadow + base

            if az:
                dir = None
            else:
                dir = base
                shell.mkdir(dir)

            _Cvs(self.root + base, ns).Checkout(tag,
                                                tmp[base],
                                                az,
                                                timestamp,
                                                nonrecursive,
                                                dir)

    def get_viewcvs_url(self, path):
        if not self.viewcvs:
            return None
        try:
            project, path = string.split(path,"/",1)
        except ValueError:
            project = path
            path = ""
        return string.replace(self.viewcvs,"$project", project) + path

## create CVS class instance
_Cvs = None
_cvs = None

if os.name == "posix" or (os.name == 'nt' and \
    os.environ.get('OS') == 'Windows_NT'):
    _Cvs = UNIXWinCVS

elif os.name == "dos" or \
    string.find(os.environ.get("winbootdir", ""), "WINDOWS") >= 0:
    _Cvs = Win9xCVS

elif os.name == "mac":
    if os.environ.get("CVSROOT","") != "":
        _Cvs=UNIXWinCVS
    else:
        _Cvs = MacCVS

else:
    e = err.Error()
    e.Set("Unsupported OS for cvs.py.")
    raise err.error, e


cvs_class_cache = {}

cvs_error = "cvs error"


def Get(name):
    log.trace( 'entry' , [ name ] )
    if cvs_class_cache.has_key(name):
        ret = cvs_class_cache[name]
        log.trace( 'exit' , [ ret ] )
        return ret

    ret = None
    if name == "":
        if _Cvs == MacCVS:
            ret = _Cvs(os.environ.get("CVSSESSION_PATH"))
        else:
            ret = _Cvs(os.environ.get("CVSROOT"))

    if not ret:
        print "Failed to find CVS repository '%s' in your .buildrc." % name
        print "Please refer to the documentation to find out how to"
        print "add cvs repositories to your buildrc file."
        raise cvs_error

    cvs_class_cache[name] = ret
    
    log.trace( 'exit' , [ ret ] )
    return ret


def Add(name, root, shadow = None, viewcvs = None):
    cvs_class_cache[name] = _Cvs(root, shadow, viewcvs)


def AddMulti(name, root, shadow = None, viewcvs = None):
    cvs_class_cache[name] = MultiCVS(root, shadow, viewcvs)


def AddMacCVS(name, session_file, shadow_session = None, viewcvs = None):
    cvs_class_cache[name] = MacCvs(session_file, shadow_session, viewcvs)


def AddCmdCVS(name, root, shadow = None, viewcvs = None):
    if _Cvs != MacCVS:
        Add(name, root, shadow, viewcvs)
    else:
        cvs_class_cache[name] = UNIXWinCVS(root, shadow, viewcvs)

get_reverse_cache={}
def GetReverse(root, path):
    if not get_reverse_cache.has_key(root):
        for (name, cl) in cvs_class_cache.items():
            if cl.root == root[:len(cl.root)]:
                get_reverse_cache[root]=name
                break

    return get_reverse_cache[root]
    

cvs_checkout_hook = None

## entrypoints
def Checkout(tag,
             module_list,
             repository_name = "",
             az = None,
             timestamp = None,
             nonrecursive = 0,
             zap = None):

    log.trace( 'entry' , [ module_list , repository_name , az , timestamp , nonrecursive , zap ] )
    module_list = listify(module_list)

    if cvs_checkout_hook:
        module_list = cvs_checkout_hook(tag, module_list, repository_name, az, timestamp, nonrecursive)

    if not module_list:
        return

    if zap:
        for mod in module_list:
            if az:
                mod = az
            if os.path.exists(mod):
                import thread
                print "DELETING: %s [%d] START" % (mod, thread.get_ident())
                shell.rm(mod)
                print "DELETING: %s [%d] END" % (mod, thread.get_ident())
    
    log.trace( 'exit' )
    return Get(repository_name).Checkout(tag, module_list, az, timestamp, nonrecursive)


def Status(repository, path, dir = None):
    return Get(repository).Status(path, dir)

def Commit(repository, path, message, dir = None):
    return Get(repository).Commit(path, message, dir)

def Tag(repository, path, tag, dir=None):
    return Get(repository).Tag(path, tag, dir)


def GetViewCVSUrl(repository, path):
    return Get(repository).get_viewcvs_url(path)


import posixpath

class MagicFixDir:
    
    def is_new(self):
        return self.new_cvs_dir

    def get_root(self):
        if not self.cvsdir:
            return None
        return open(os.path.join(self.cvsdir,"Root"),"r").read()

    def get_repository(self):
        return open(os.path.join(self.cvsdir,"Repository"),"r").read()

    def get_parent_repository(self):
        rep_all=self.get_repository()
        rep=string.rstrip(rep_all)
        dname=posixpath.dirname(rep)
        if not dname:
            dname="."
        return dname + rep_all[ len(rep) - len(rep_all) : ]

    def get_tag(self):
        try:
            return open(string.join(self.cvsdir,"Tag"),"r").read()
        except IOError:
            return None


    def mkparent(self):
        return (
            self.get_root(),
            self.get_parent_repository(),
            self.get_tag()
            )

    def get_entry(self):
        return "D/%s////\n" % os.path.basename(self.dir)

    def __init__(self, dir="."):
        self.dir=dir
        self.new_cvs_dir=None
        self.cvsdir = os.path.join(dir, "CVS")
        subdirs=[]
        entries=[]
        self.use_fake=0


        try:
            files=os.listdir(dir)
        except:
            return

        for e in files:
            if e in ["CVS"]:
                continue

            path = os.path.join(dir, e)
            if os.path.isdir(path):
                ndir = MagicFixDir(path)
                if ndir.cvsdir:
                    subdirs.append(ndir)

        if not os.path.isdir(self.cvsdir):
            if subdirs:
                self.new_cvs_dir=1

                q=subdirs[0].mkparent()
                for n in subdirs:
                    entries.append(n.get_entry())
                    #print "==========="
                    #print n.cvsdir
                    #print n.mkparent()
                    #print q
                    #print "==========="
                    if n.use_fake or n.mkparent() != q:
                        self.use_fake=1

                shell.mkdir(self.cvsdir)
                if self.use_fake:
                    print "Adding faux CVS dir: %s" % self.cvsdir

                    faux=os.path.join(os.environ["BUILD_ROOT"],"lib","faux-cvs","CVS")
                    if os.path.isdir(faux):
                        shell.mkdir(self.cvsdir)
                        shell.cp(os.path.join(faux,"Root"), os.path.join(self.cvsdir,"Root"))
                        shell.cp(os.path.join(faux,"Repository"),
                                 os.path.join(self.cvsdir,"Repository"))
                    else:
                        print "Faux cvs dir missing, skipping"
                        self.cvsdir=None
                        return
                else:
                    print "Adding PARENT CVS dir: %s" % self.cvsdir
                    shell.mkdir(self.cvsdir)
                    open(os.path.join(self.cvsdir,"Root"),"w").write(q[0])
                    open(os.path.join(self.cvsdir,"Repository"),"w").write(q[1])
                    if q[2]:
                        open(os.path.join(self.cvsdir,"Tag"),"w").write(q[2])
                    
                open(os.path.join(self.cvsdir,"Entries"),"w").write(string.join(entries,""))
            else:
                self.cvsdir=None
        else:
            for n in subdirs:
                if n.is_new():
                    entries.append(n.get_entry())

            if entries:
                print "Adding CVS entries in %s" % self.cvsdir
                e=open(os.path.join(self.cvsdir,"Entries"),"a")
                e.write(string.join(entries,""))
        

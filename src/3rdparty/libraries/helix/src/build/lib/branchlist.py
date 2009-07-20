# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: branchlist.py,v 1.22 2007/07/17 00:17:48 jfinnecy Exp $ 
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
"""Functions, classes to search a directory full of *.bif files and extract
the <build id="xxx"> strings from all of them.  It does not do full XML
parsing because that would be too slow.  Instead, it just uses a regular
expression."""

import os
import sys
import re
import string
import glob
import err
import outmsg
import types
import time
import chaingang
import cvs
import log
import utils

## List of (name, path)
## where path may be a normal filesystem directory,
## or [cvsrepository]/path/to/dir
## where 'cvsrepository' must be added with cvs.Add()
## 
branch_search_path = []

_re_branch_id = re.compile("^\s*<\s*build\s*id=\"([^\"]+)\"")

def neuter(s):
    s = string.replace(s,":","%3a")
    s = string.replace(s,"%","%25")
    s = string.replace(s,"/","%2f")
    s = string.replace(s," ","%20")
    s = string.replace(s,"\\","%5c")
    s = string.replace(s,".","%2e")
    return s


class PathList:
    """Go through the build information files and yank out the branch
    id's for quick display."""

    def do_checkout(self,
                    prefix, base,
                    tag, cvspath, repository, local_path, timestamp):
                        
        outmsg.send("Updating [%s] %s files..." % (prefix, base))
        cvs.Checkout(tag, cvspath, repository, local_path, timestamp)
        

    ## FIXME: Add a command line option to NOT cvs update BIF files ?
    def __init__(self, paths, base, ext_list, tag = "", timestamp = "",
                 noupdate=0):
        
        log.trace( 'entry' , [ paths , base , ext_list , tag , timestamp ] )
        self.path_list = paths

        if type(ext_list) == types.StringType:
            ext_list = [ ext_list ]

        ## Find bif files in */build/{BIF,umakecf}
        if base:
            log.debug( 'Checking %s for build subdirs.' % base )
            for dir in os.listdir(os.getcwd()):
                p=os.path.join(os.getcwd(), dir, "build", base)
                if os.path.isdir(p):
                    ## Destructive!!
                    p=("./"+dir, p)
                    if p not in paths:
                        paths.append( p )


        working_path = os.getcwd()
        log.debug( 'Going to work in %s' % working_path )
        self.name_hash = {}
        self.name_reverse_hash = {}
        self.list=[]
        self.list2=[]
        self.paths={}

        jobs=[]
        
        for (prefix, path) in paths:
            log.debug( 'Checking out %s,%s' % ( prefix , path ) )
            if path[0] == '[':
                ## CVS PATH!!!
                ## (Check if this works on MAC!)
                tmp=string.split(path[1:],"]")
                repository=tmp[0]
                cvspath=string.join(tmp[1:],"]")

                if cvspath and cvspath[0]=='/':
                    cvspath=cvspath[1:]

                base = os.path.basename(cvspath)

                
                checkoutdir = os.path.join(os.environ["BUILD_ROOT"] , string.lower(base + "-cvs"))
                if not os.path.isdir(checkoutdir):
                    os.mkdir(checkoutdir)

                checkoutdir = os.path.join(checkoutdir, repository)
                if not os.path.isdir(checkoutdir):
                    os.mkdir(checkoutdir)

                if tag:
                    checkoutdir = os.path.join(checkoutdir, neuter(tag))
                    if not os.path.isdir(checkoutdir):
                        os.mkdir(checkoutdir)

                if timestamp:
                    checkoutdir = os.path.join(checkoutdir, neuter(timestamp))
                    if not os.path.isdir(checkoutdir):
                        os.mkdir(checkoutdir)

                local_path = os.path.join(checkoutdir, cvspath)

                ## noupdate = 0: update always
                ## noupdate = 1: update if not updated within 30 seconds
                ## noupdate = 2: update if dir doesn't exist
                ## noupdate = 3: never update

                do_update=1

                if noupdate >= 3:
                    do_update=0
                elif noupdate and os.path.exists(local_path):
                    if noupdate >= 2:
                        do_update=0
                    else:
                        t=0
                        try:
                            timefile=os.path.join(local_path,"CVS","timestamp")
                            t=int(open(timefile).read())
                        except:
                            pass

                        if t + 30 > int(time.time()):
                            if noupdate >= 1:
                                do_update=0
                            

                if do_update:
                    log.debug( 'Adding job to chaingang for %s' % path )
                    jobs.append(chaingang.ChainGangJob(
                        "[%s]%s" % (prefix, base),
                        self.do_checkout,
                        [ prefix, base,
                          tag, cvspath, repository, local_path, timestamp]))
                                                  
            else:
                # Regular path
                local_path = path

            self.paths[prefix] = local_path
            
            log.debug( 'Done creating checkout job for %s.' % path )

        t=time.time()
        log.info( 'Submitting all jobs to chaingang.' )
        chaingang.ChainGang(jobs,3).run()
        log.info( "Time used for updates: %f" % (time.time()-t) )

        log.info( 'Building filename lists.' )
        for (prefix, path) in paths:
            log.debug( 'Checking %s, %s.' % ( prefix , path ) )

            local_path = self.paths[prefix]
            
            for ext in ext_list:
                file_list = utils.globNoMetaPath(os.path.join(local_path, "*.%s" % ext))

                for file in file_list:
                    full_path=os.path.join(local_path, file)
                    file_name = self.file_name(full_path)
                    full_path=self.fix_file_name(full_path)
                    file_name2 = "[%s]%s" % (prefix, file_name)

                    if self.name_hash.has_key(file_name):
                        if self.name_hash.has_key(file_name2):
                            print "ERROR: .%s files %s and %s have the same name (%s)" % (
                                ext,
                                self.name_hash[file_name2],
                                file,
                                file_name2
                                )
                            del self.name_hash[file_name]
                            del self.name_hash[file_name2]
                            if file_name in self.list:
                                self.list.remove(file_name)
                            if file_name2 in self.list:
                                self.list.remove(file_name2)
                            continue
                                  
                        self.list.append(file_name2)
                        self.name_reverse_hash[full_path]=file_name2
                    else:
                        self.name_hash[file_name] = full_path
                        self.name_reverse_hash[full_path]=file_name
                        self.list.append(file_name)

                    self.list2.append(file_name2)
                    self.name_hash[file_name2] = full_path
        log.trace( 'exit' )

    def fix_file_name(self, fname):
        return fname

    def file(self, file_name):
        #print "(%s)\n" %branch_tag
        if self.name_hash.has_key(file_name):
            return self.name_hash[file_name]
        return None

    def name(self, file_name):
        if self.name_reverse_hash.has_key(file_name):
            return self.name_reverse_hash[file_name]
        return file_name

    def get_list(self):
        return self.list



class BranchList(PathList):
    """Go through the build information files and yank out the branch
    id's for quick display."""
    

    ## FIXME: Add a command line option to NOT cvs update BIF files ?
    def __init__(self, tag = "", timestamp = "", noupdate=1):
        global branch_search_path

        PathList.__init__(self, branch_search_path, "BIF", [ "bif", "biif"], tag, timestamp, noupdate)

        if not branch_search_path:
            print
            print "You have no BIF paths defined in your .buildrc file."
            print "(or you have no buildrc file) Please refer to the"
            print "documentation for how to set up your .buildrc file:"
            print os.path.join(os.environ["BUILD_ROOT"], "doc", "buildrc.html")
            print
            sys.exit(1)

    def get_bif_list(self):
        """Return a list of .bif files, .cif, and .bifi files are excluded."""
        ret = []
        for id in self.get_list():
            if string.lower(self.file(id)[-4:])==".bif":
                ret.append(id)
        return ret


    def file_name(self, path):
        fil = open(path, "r")

        while 1:
            line = fil.readline()
            if not line:
                break

            match = _re_branch_id.match(line)
            if match:
                return match.group(1)

        return None


    def load(self, branch_tag):
        if self.name_hash.has_key(branch_tag):
            import bif
            return bif.load_bif_data(self.name_hash[branch_tag], self)
        return None



## print out the branch list
if __name__ == "__main__":
    branch_list = BranchList()
    for branch_name in branch_list.list:
        print branch_name


def AddBIFPath(prefix, path):
    if not (prefix, path) in branch_search_path:
        branch_search_path.append( (prefix, path) )

### This doesn't really belong here, but it was easier to stick it here...

profile_search_path=[ ("helix", os.path.join(os.environ.get("BUILD_ROOT"),"umakepf") ) ]

class ProfileList(PathList):
    def __init__(self, tag = "", timestamp = "", noupdate=1):
        global profile_search_path

        PathList.__init__(self, profile_search_path, "umakepf", "pf", tag, timestamp, noupdate)

    def fix_file_name(self, fname):
        x=string.split(fname,".")
        if x[-1] == "pf":
            x=x[:-1]
        return string.join(x,".")

    def file_name(self, f):
        f=os.path.basename(f)
        x=string.split(f,".")
        if x[-1] == "pf":
            x=x[:-1]
        return string.join(x,".")

    def file(self, file_name):
        f=PathList.file(self, file_name)
        if f:
            return f
        return file_name


def AddProfilePath(prefix, path):
    global profile_search_path
    if not (prefix, path) in profile_search_path:
        profile_search_path.append( (prefix, path) )

def RemoveProfilePath(prefix):
    global profile_search_path
    n=[]
    for p in profile_search_path:
        if p[0] != prefix:
            n.append(p)
    profile_search_path=n

    
    
    

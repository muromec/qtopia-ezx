#!/usr/bin/env python
# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: diff.py,v 1.15 2007/04/30 22:51:13 jfinnecy Exp $ 
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
"""Not part of the build system, but a really useful script for diffing
the code base. """

import os
import sys
import string
import getopt

import bif
import dependlist
import branchlist
import copy
import cvs

import types
import chaingang
import distributions
import shell
import module

import err
import time
import posixpath
import re
import timestamp

def htmlquote(str):
    str=string.strip(str)
    str=string.replace(str,"&","&amp;")
    str=string.replace(str,"<","&lt;")
    str=string.replace(str,">","&gt;")
    str=string.replace(str,"\n","<br>")
    return str

def dec_rev(rev):
    tmp=string.split(rev,".")
    tmp[-1]=str(int(tmp[-1])-1)
    # Check for new branch revision
    if (0 == int(tmp[-1])) and (len(tmp) > 2):
        tmp = tmp[:-2]
    return string.join(tmp, ".")

class File:
    def __init__(self, localname, rev, root, cvsfile, viewcvs_url):
        self.localname=localname
        self.rev=rev
        self.root=root
        self.cvsfile=cvsfile
        self.viewcvs_url=viewcvs_url
    

    def text(self):
        return '    %s: %s' % (self.localname, self.rev)

    def html(self):
        if self.viewcvs_url:
            # Our two servers use two different ViewCVS models now. :P
            if re.search( "cvs\.helixcommunity\.org" , self.root ):
                repository = self.root.split("/")[-1:][0]
                return '<a href="%s?cvsroot=%s">%s</a>&nbsp;<a href="%s?annotate=%s;cvsroot=%s">%s</a>&nbsp;(<a href="%s.diff?r1=%s;r2=%s;cvsroot=%s">diff</a>)' % (
                    self.viewcvs_url,
                    repository,
                    htmlquote(self.localname),
                    self.viewcvs_url,
                    self.rev,
                    repository,
                    self.rev,
                    self.viewcvs_url,
                    dec_rev(self.rev),
                    self.rev,
                    repository
                    )
            else:
                return '<a href="%s">%s</a>&nbsp;<a href="%s?annotate=%s">%s</a>&nbsp;(<a href="%s.diff?r1=%s&r2=%s">diff</a>)' % (
                    self.viewcvs_url,
                    htmlquote(self.localname),
                    self.viewcvs_url,
                    self.rev,
                    self.rev,
                    self.viewcvs_url,
                    dec_rev(self.rev),
                    self.rev,
                    )
            
        return "%s&nbsp;%s" % (htmlquote(self.localname), self.rev)

class Checkin:

    def __init__(self, author, date, comment):
        self.begin_date=date
        self.end_date=date
        self.files=[]
        self.comment=comment
        self.author=author

    def add(self, file):
        self.files.append(file)

    def __repr__(self):
        ret=[]
        ret.append("===================Checkin======================");
        ret.append("%s - %s, %s" % (
            time.ctime(self.begin_date),
            time.ctime(self.end_date),
            self.author))
            
        for (localname, rev, root, cvsfile) in self.files:
            ret.append('  %s: %s (%s <> %s))' % (localname, rev, root, cvsfile))

        ret.append(self.comment)

        return string.join(ret,"\n")

    def text(self):
        ret=[]
        ret.append("Checkin by %s, %s" % (self.author,
                                          time.ctime(self.begin_date)))
            
        for f in self.files:
            ret.append(f.text())

        ret.append("")

        ret.append(self.comment)

        ret.append("---------------------------------------------------")
        return string.join(ret,"\n")


    def html(self, blue):
        if blue:
            td="<td valign=top bgcolor='#ccccee'"
        else:
            td="<td valign=top bgcolor='#ffffff'"
        
        ret=[]
        ret.append("<tr align=top>")
        t=string.replace(timestamp.Timestamp(self.begin_date).getUTCString(),
                         " ","&nbsp;")
        ret.append("%s>%s</td>" % (td,t))
        ret.append("%s>%s</td>" % (td, htmlquote(self.author)))
        ret.append("%s>%s</td>" % (td, htmlquote(self.comment)))

        files=[]
        for f in self.files:
            files.append(f.html())

        ret.append("%s>%s</td>" % (td, string.join(files,"<br>")))

        return string.join(ret,"")
        

class Diff:
    def process_module(self, imodule):

        if imodule.type in ["cvs", "distribution"]:
            revisions=0
                
            tmpdir="diff_tmp_"+str(distributions.my_get_thread_ident())
            shell.rm(tmpdir)

            try:

                if imodule.type == "cvs":
                    imodule.checkout(
                        date = self.source_date,
                        tag = self.source_tag,
                        as = tmpdir)
                else:
                    cvs.Checkout(self.source_tag or imodule.cvs_tag,
                                 "distribution",
                                 imodule.cvs_root,
                                 tmpdir,
                                 self.source_date or imodule.cvs_date)

                tag = "-b"
                try:
                    tag=string.strip(open(os.path.join(tmpdir,"CVS","Tag"),"r").read())

                    ## Fixme: what if the sticky tag is a date??
                    
                    if tag and tag[0] == "T":
                        tag = "-r"+tag[1:]
                    else:
                        tag = "-b"
                            
                except IOError:
                    pass

                cmd='cvs log%s -N %s >cvs-log-temp-file' %( self.date_range,
                                                            tag )
                print "Running: %s (in %s + %s)" % (cmd, os.getcwd(), tmpdir)
                status, output = shell.run(cmd, dir = tmpdir)

                print output

                output=open(os.path.join(tmpdir,"cvs-log-temp-file"),"r").read()

                output=string.replace(output,"\r\n","\n")

                root="UNKNOWN"
                try:
                    root=open(os.path.join(tmpdir,"CVS","Root"),"r").read()
                    root=string.strip(root)
                except IOError:
                    pass

                base_repository="UNKNOWN"
                try:
                    base_repository=open(os.path.join(tmpdir,"CVS","Repository"),"r").read()
                    base_repository=string.strip(base_repository)
                except IOError:
                    pass

                
                viewcvs_base = cvs.GetViewCVSUrl(imodule.cvs_root,
                                                 imodule.cvs_path or
                                                 imodule.name)

                blocks=string.split(output,
                                    "=============================================================================")
                for file in blocks[:-2]:
                    data=string.split(file,"----------------------------\nrevision ")
                    wf=string.split(data[0],"Working file: ",1)[1]
                    wf=string.split(wf,"\n",1)[0]
                    local_name=os.path.join(imodule.name, wf)
                    print " - %s, %d revisions" % (wf, len(data)-1)

                    viewcvs_url=None
                    if viewcvs_base:
                        viewcvs_url=posixpath.join(viewcvs_base,wf)

                    cvs_file=os.path.join(base_repository, wf)
                    for revision in data[1:]:
                        revdata=string.split(revision,";",3)
                        author=string.split(revdata[1])[1]

                        ## Ignore build farm checkins
                        if author in [ "mserver", "drmbuild", "buildq", "codecbuild" ]:
                            continue

                        tmp=string.split(revdata[0])
                        rev=tmp[0]

                        t="%s %s" % (tmp[2],tmp[3])
                        t=time.strptime(t,"%Y/%m/%d %H:%M:%S")
                        t=time.mktime(t)-time.timezone
                        author=string.split(revdata[1])[1]
                        comment=string.split(revdata[3],"\n",1)[1]
                        if string.split(comment,":",1)[0] == "branches":
                            comment=string.split(comment,"\n",1)[1]

                                                    
                        ci=Checkin(author, t, comment)
                        ci.add(File(local_name, rev,
                                    root, cvs_file,
                                    viewcvs_url))

                        self.checkins.append(ci)
                        revisions = revisions + 1
                
                if status:
                    print "Diff module %s failed\n" % imodule.id

            except cvs.cvs_error:
                print "Diff module %s failed\n" % imodule.id

            shell.rm(tmpdir)
            print "%d revisions in %s" % (revisions, imodule.id)

    def cmpfun(self, x, y):
        if x.begin_date != y.begin_date:
            if x.begin_date < y.begin_date:
                return 1
            if x.begin_date > y.begin_date:
                return -1

        if x.author != y.author:
            return x.author < y.author

        return x.files[0].localname < y.files[0].localname

    def merge_checkins(self):
        if not len(self.checkins):
            return
        self.checkins.sort(self.cmpfun)
        c=[ self.checkins[0] ]
        for x in self.checkins[1:]:
            if x.comment == c[-1].comment and x.author == c[-1].author:
                c[-1].end_date=x.end_date
                c[-1].files.extend(x.files)
            else:
                c.append(x)

        self.checkins=c

    
    def __init__(self,
                 module_id,
                 source_branch,
                 source_tag,
                 source_date,
                 start_date,
                 end_date):

        if start_date:
            if not re.match("^[-a-zA-Z0-9:/ ]*$",start_date):
                raise "Invalid start date"

        if end_date:
            if not re.match("^[-a-zA-Z0-9:/ ]*$",end_date):
                raise "Invalid start date"

        self.source_tag = source_tag
        self.source_date = source_date

        self.checkins = []

        todo={}

        if type(module_id) != types.ListType:
            module_id=[ module_id ]

        self.module_id = module_id
        self.source_branch = source_branch
        self.start_date = start_date
        self.end_date = end_date

        if self.start_date:
            self.start_date_local=self.start_date
            # FIXME: this was heinously broken. This call converts ticks
            # to gmticks with the date_to_gmticks() call. Then converts that
            # time a second time with the cvs_server_time() call.
            # This will probably require deep investigation to make sure that
            # diff reports are all using the correct times. No surprise that
            # there is an open bug on the diff report timestamps.
            #self.start_date = datelib.cvs_server_time(datelib.date_to_gmticks(self.start_date))
            
            # Notice we are still propagating raw strings here instead of
            # moving Timestamp objects around. Going to take a long time to
            # clean this up completely.
            startTS = timestamp.Timestamp( self.start_date )
            self.start_date = startTS.getUTCString()

        if self.end_date:
            self.end_date_local=self.end_date
            endTS = timestamp.Timestamp( self.end_date )
            self.end_date = endTS.getUTCString()
            #self.end_date = datelib.cvs_server_time(datelib.date_to_gmticks(self.end_date))

        self.date_range = " -d '%s<%s'" % (self.start_date or "",
                                           self.end_date or "")
        if self.date_range == " -d '<'":
            self.date_range = ""

        ## find the BIF file for the branch name
        branch_list = branchlist.BranchList(source_tag, source_date)
        old_filename = branch_list.file(source_branch)
        if not old_filename:
            raise "no BIF file for branch=\"%s\"" % (source_branch)

        ## parse old BIF file, and create the new BIF file
        ## Do not include shadows
        print "parsing file=\"%s\"" % (old_filename)
        bdata1 = bif.load_bif_data(old_filename, branch_list, 0)

        ## compute the dependancy list for the target

        if module_id == [ "ALL" ]:
            print "diffing all modules" % (string.capitalize(branch_or_tag))
            deplist1_list = bdata1.module_hash.values()
        else:
            print "computing dependancy tree for target=%s" % (repr(module_id))
            deplist1_list = dependlist.DependList(bdata1, module_id).list()

        modules_to_diff = []


        dists_done={}
        for mod in deplist1_list:
            if mod.type not in ["cvs", "distribution" ]:
                continue

            if mod.type == "distribution":
                if dists_done.has_key(mod.cvs_root):
                    continue
                dists_done[mod.cvs_root]=1

            modules_to_diff.append(mod)

        self.modules_to_diff = modules_to_diff

    def processModules( self ):
        chaingang.ProcessModules_anyorder( self.modules_to_diff, self.process_module)
        self.merge_checkins()

    def getModuleList( self ):
        """getModuleList() --> list

        Returns the list of modules that the Diff class is interested in.
        """
        # This stuff should all be in BIF, but for some reason it's here.
        # Prime candidate for a later refactoring.
        return self.modules_to_diff

    def html(self):
        rownum=0
        ret=[]
        ret.append("<table width='100%'>")
        ret.append("<tr>")
        ret.append('<th bgcolor="#88ff88">Date</th>')
        ret.append('<th bgcolor="#88ff88">Author</th>')
        ret.append('<th bgcolor="#88ff88">Comment</th>')
        ret.append('<th bgcolor="#88ff88">Files</th>')
        ret.append("</tr>\n")

        for x in self.checkins:
            ret.append("<tr>")
            ret.append(x.html(rownum & 1))
            ret.append("</tr>\n")
            rownum = rownum + 1

        if not self.checkins:
            ret.append("<tr>")
            ret.append('<td align=center bgcolor="#ffffff" colspan=4>no changes</td>')
            ret.append("</tr>\n")

        ret.append("</table>")
        return ret


    def html2(self):
        ret=[]
        ret.append("<table width=100% bgcolor='#e6e6e6'><tr><td>")
        ret.append("<table bgcolor='#e6e6e6'>")
        ret.append("<tr><td align=right><b>BIF:</b></td><td>%s</td>\n" %
                   htmlquote(self.source_branch))
        ret.append("<tr><td align=right><b>Target:</b></td><td>%s</td>\n" %
                   htmlquote(string.join(self.module_id,", ")))

        if self.source_tag:
            ret.append("<tr><td align=right><b>CVS Tag:</b></td><td>%s</td>\n" %
                       htmlquote(self.source_tag))

        if self.source_date:
            ret.append("<tr><td align=right><b>CVS Date:</b></td><td>%s</td>\n" %
                       htmlquote(self.source_date))

        if self.start_date:
            ret.append("<tr><td align=right><b>History Start:</b></td><td>%s</td>\n" %
                       htmlquote(self.start_date_local))
        if self.end_date:
            ret.append("<tr><td align=right><b>History End:</b></td><td>%s</td>\n" %
                       htmlquote(self.end_date_local))

        ret.append("</table>")
        ret.append("</table>")
        ret.extend(self.html())

        return ret


    def html3(self):
        ret=[]
        ret.append("<html>")
        ret.append("<header><title>Diff report</title></header>")
        ret.append("<body>")
        ret.append("<center><h2>Diff report</h2></center>")
        ret.extend(self.html2())
        ret.append("</body>")
        ret.append("</html>")
        return ret
        


## print usage information
def usage():
    print 'Usage: diff.py [-r cvs-tag-to-branch-from] [-D cvs-date-to-read-bif-from]'
    print '               [-S start-date] [-E end-date]'
    print '               [-o output.html] -b BIF-file-id TARGET'
    print 'If target is "ALL", all targets will be diffed.'
    print 'default output filename is diff.html'
    sys.exit(1)


## MAIN
if __name__ == '__main__':
    make_cmd="make"
    opt_list, arg_list = getopt.getopt(sys.argv[1:], 'r:b:D:o:S:E:')

    if len(arg_list) != 1:
        usage()

    ## create a hash from the opt_list
    opt_hash = {}
    for opt in opt_list: opt_hash[opt[0]] = opt[1]

    source_tag = ''
    source_branch = ''
    source_date=''
    ribosome=None
    cvs_flags=''
    bif_dir=None
    output_filename="diff.html"
    start_date=None
    end_date=None

    if opt_hash.has_key('-r'):
        source_tag = opt_hash['-r']

    if opt_hash.has_key('-b'):
        source_branch = opt_hash['-b']

    if opt_hash.has_key('-D'):
        source_date = opt_hash['-D']

    if opt_hash.has_key('-o'):
        output_filename = opt_hash['-o']

    if opt_hash.has_key('-S'):
        start_date = opt_hash['-S']

    if opt_hash.has_key('-E'):
        end_date = opt_hash['-E']

    target_module_id = arg_list[0]

    if not source_branch:
        usage()

    import build_exe
    build_exe.call_buildrc()

    try:
        diff=Diff(target_module_id,
                  source_branch,
                  source_tag,
                  source_date,
                  start_date,
                  end_date)
        diff.processModules()
    except err.error, e:
        print "Something went wrong:"
        print e.Text()

    open(output_filename,"w").write(string.join(diff.html3(),""))
    

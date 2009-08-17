import outmsg
import string
import cvs
import os
import shell
import log

def dummy_get_thread_ident():
    return 1

my_get_thread_ident = dummy_get_thread_ident

try:
    import thread
    if thread.get_ident():
        my_get_thread_ident = thread.get_ident
except:
    pass


def tmpdir(xtra=""):
    return "%stmpdir_%d" % (xtra,my_get_thread_ident())


def cleanup(xtra=""):
    #print "CLEANUP: %s" % tmpdir(xtra)
    try:
        shell.rm(tmpdir(xtra))
    except:
        for l in os.listdir(tmpdir(xtra)):
            shell.rm(os.path.join(tmpdir(xtra),l))

def setup(xtra=""):
    cleanup(xtra)
    if not os.path.isdir(tmpdir(xtra)):
        #print "MKDIR: %s" % tmpdir(xtra)
        shell.mkdir(tmpdir(xtra))

cvs_directory_caches={}        

class DistFinder:
    def __init__(self,
                 module_name,
                 cvs_root,
                 cvs_tag,
                 cvs_timestamp = "",
                 cvs_path = None,
                 cvs_base = "distribution"):
        self.module_name = module_name
        self.cvs_root = cvs_root
        self.cvs_tag = cvs_tag
        self.cvs_timestamp = cvs_timestamp
        self.cvs_path = cvs_path or module_name
        self.cvs_base = cvs_base

        if not cvs_directory_caches.has_key(cvs_root):
            cvs_directory_caches[cvs_root]={}

        self.cvs_dir_cache=cvs_directory_caches[cvs_root]


    ## FIXME: Allow str to be an array
    def possible_locations(self, str):
        str = "/" + str
        ret = [ str ]
        parts = string.split(str,"-")

        if len(parts) > 1:
            parts.append("")

        while len(parts) > 1:
            parts[-1] = ""
            ret.append(string.join(parts,"-"))
            parts=parts[:-1]
        ret.append("")
        return ret

    def natify_path(self, path):
        return apply(os.path.join, [ os.curdir ] + string.split(path,"/"))


    def cvs_dir_check(self, loc):
        log.trace( 'entry' , [ loc ] )
        global cvs_directory_cache

        parts=string.split(loc, "/")
        log.debug( 'Parts = %s' % parts )
        x=len(parts)-2
        while x > 0:
            part = string.join(parts[:x],"/")
            log.debug("CHECKING %s" % part )
            # cvs_dir_cache holds the result of prior dir_check results.
            # Thus, a key with a value of 0 means prior failure, which forces
            # the early exit rather than a re-check.
            if not self.cvs_dir_cache.get(part,1):
                log.debug( 'Prior check reported failure on %s' % part )
                log.trace( 'exit' , [ 0 ] )
                return 0

            # Here, prior success of the cache check means prior success, so
            # we abort the loop and start the check.
            if self.cvs_dir_cache.get(part,0):
                log.debug('Prior check reported success on %s' % part )
                break

            x = x - 1

        y=(len(parts) + x - 1)/2
        if y <= x:
            log.debug( 'y <= x - early exit' )
            log.trace( 'exit' , [1] )
            return 1
        part = string.join(parts[:y],"/")

        log.info( "TESTING %s" % part )

        ## Nonrecursive checkout
        p=os.path.join(tmpdir(), "dir_cache_test")
        try:
            cvs.Checkout("", part, self.cvs_root,p, None, 1)
        except cvs.cvs_error:
            pass

        
        # FIFME: This relies on the cvs client implementation. On Linux,
        # checking out a dir with no files still puts a local dir and a CVS
        # subdir. On Windows, it doesn't touch the local filesystem, so 
        # this returns True on Linux, and False on Windows.
        ret=os.path.exists(p)
        if ret:
            shell.rm(p)
            
        # This is where we store the results of the check into a cache to
        # cut down on CVS server load.
        self.cvs_dir_cache[part]=ret
        log.info( "RESULT => %s" % repr(ret) )
        log.trace( 'exit' , [ ret ] )
        return ret
                     

    ## Locate an RNA distribution in cvs
    ## Return a native path to the distribution if found, otherwise None
    def find_distribution_rna(self, locations):

        for dir in locations:
            if not self.cvs_dir_check(dir):
                continue
            dir = dir +".rna"
            outmsg.verbose("Looking for %s in %s (don't worry if this fails)" % (self.cvs_path, dir))

            try:
                cvs.Checkout(self.cvs_tag,
                             dir,
                             self.cvs_root,
                             os.path.dirname(os.path.join(tmpdir(),dir)),
                             self.cvs_timestamp)
            except cvs.cvs_error: ## Ignore cvs errors, report them as missing distributions instead
                return


            dir = os.path.join(os.curdir, tmpdir(), dir)

            try:
                print "DOES %s exist????" % dir
                if os.path.isfile(dir):
                    return dir
            except OSError:
                pass

        return None

    ## Locate a distribution in cvs
    ## Return a native path to the distribution if found, otherwise None
    def find_distribution_cvs(self, locations):
        log.trace( 'entry' , [ locations ] )
        for dir in locations:
            log.debug( 'Trying to find %s' % dir )
            if not self.cvs_dir_check(dir):
                log.debug( 'Failed cvs_dir_check on "%s" - skipping.' % dir )
                continue
                
            # Once we have a dir, let's do something with it.
            message = "Looking for %s in %s (don't worry if this fails)" % (self.cvs_path, dir)
            log.debug( message )
            outmsg.verbose( message )

            try:
                cvs.Checkout(self.cvs_tag,
                             dir,
                             self.cvs_root,
                             os.path.join(tmpdir(),dir),
                             self.cvs_timestamp)
            except cvs.cvs_error: ## Ignore cvs errors, report them as missing distributions instead
                log.warn( 'Failed to check out %s - skipping' % dir )
                return

            dir = os.path.join(os.curdir, tmpdir(), dir)

            try:
                for f in os.listdir(dir):
                    if string.lower(f) not in ["cvs"]:
                        return dir
            except OSError:
                pass

        log.trace( 'exit' )
        return None


    ## Locate a distribution already on disk
    ## Return a native path to the distribution if found, otherwise None
    def find_distribution_filesystem(self, locations):
        if not os.path.exists(self.natify_path(self.cvs_base)):
            return None

        for dir in locations:
            dir=self.natify_path(dir)
            outmsg.verbose("Looking for %s in %s" % (self.cvs_path, dir))

            if os.path.isdir(dir) and os.listdir(dir):
                return dir

            if os.path.isfile(dir+".rna"):
                return dir+".rna"

        return None

    def find_distribution(self, dist_id, profile_id, build_type, search_local = 1, search_cvs = 1):
        ## Profile may be a full path!
        profile_id = os.path.basename(profile_id)
        
        ## .rna module names are magical, we cannot find them!
        if self.module_name[-4:] == ".rna":
            return None

        setup()

        message = "getting distribution=\"%s\" (cvs tag=\"%s\", root=\"%s\", dist_id=\"%s\")" % (
            self.module_name, self.cvs_tag or "HEAD", self.cvs_root, dist_id)
        log.debug( message )
        outmsg.send( message )

        locations = []
        for a in self.possible_locations(dist_id):
            for b in self.possible_locations(profile_id):
                for c in self.possible_locations(build_type):
                    locations.append( "%s%s%s%s/%s" % ( self.cvs_base, a,b,c,self.cvs_path))

        if search_local:
            location = self.find_distribution_filesystem(locations)
            if location:
                return location

        if not search_cvs:
            return None
        
        if string.count(dist_id, "macos"):
            location = self.find_distribution_rna(locations)
            if location:
                return location
                
        return  self.find_distribution_cvs(locations)


    ## This function is not currently used by the Ribosome build system
    def download_distribution(self,
                              dist_id,
                              profile_id,
                              build_type):
        """Download the approperiate distribution into ./distribution so that it can be easily
        found by find_distribution_filesystem"""
        location = self.find_distribution(dist_id, profile_id, build_type, 0)

        if location:
            if string.count(location, tmpdir()):
                p = string.index(location,tmpdir())
                dest = location[:p]+location[p+len(tmpdir())+1:]
                shell.mkdir(os.path.dirname(dest))
                shell.cp(location, dest)
        else:
            outmsg.send("distribution=\"%s\" not found." % ( self.module_name ))

        cleanup()


    ## Find/download/unpack/copy distribution to proper place.
    ## Returns the module in a list if successful, [] otherwise.
    def get_distribution(self,
                         dist_id,
                         profile_id,
                         build_type,
                         search_cvs=1):

        location = self.find_distribution(dist_id, profile_id, build_type, 1, search_cvs)

        path = self.natify_path(self.module_name)
        if location:
            if location[-4:] == ".rna":
                import archive
                archive.Extract(location)
            else:
                shell.mkdir(os.path.dirname(path))
                shell.cp(location, path)

        cleanup()

        if not os.path.exists(path):
            # FIXME: This error message needs to point to the
            # (so far nonexistant) documentation
            outmsg.send("distribution=\"%s\" not found." % ( self.module_name ))

        return [ self.module_name ]




class DistCheckin:

    def lcb(self, f):
        if f[-1:] == '\n':
            f=f[:-1]
        if f[-1:] == '\r':
            f=f[:-1]
        print f

    def run(self, cmd, nowarn=0):
        print "Running '%s' in %s" % (cmd, tmpdir())
        status, output=shell.run(cmd, self.lcb, 1800, tmpdir())
        if status and not nowarn:
            print "WARNING WARNING WARNING"
            print "Command failed: %s" % cmd

        return output

    ## Root is actual cvsroot, not 'helix'
    def checkin(self, file, new_name, root, path, tag):
        print "Processing %s" % file
        if not os.path.exists(file):
            print "No such file or directory!"
            return
        setup()
        cvsdir=os.path.join(tmpdir(),"CVS")
        shell.mkdir(cvsdir)
        open(os.path.join(cvsdir,"Root"),"w").write(root+"\n")
        open(os.path.join(cvsdir,"Repository"),"w").write(path+"\n")
        open(os.path.join(cvsdir,"Entries"),"w").write("")
        if tag:
            open(os.path.join(cvsdir,"Tag"),"w").write("T"+tag+os.linesep)

        self.run('cvs update "%s"' % new_name, 1)
        new_path=os.path.join(tmpdir(),new_name)
        exists=os.path.exists(new_path)
        shell.cp(file, new_path)
        if not exists:
            self.run('cvs add -kb "%s"' % new_name, 1)
            
        self.run('cvs commit -m "distribution checkin" "%s"' % new_name)
        cleanup()

    def process_checkins(self, dir):
        import bldreg
        print "Processing distribution checkins"
        for file in bldreg.section_key_list("distribute"):
            apply(self.checkin, (os.path.join(dir, file), ) +
                  bldreg.get_value("distribute",file))
            
            

if __name__ == '__main__':
    import sys
    DistCheckin().process_checkins(sys.argv[1])

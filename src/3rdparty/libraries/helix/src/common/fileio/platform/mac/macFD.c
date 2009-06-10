/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: macFD.c,v 1.6 2007/07/06 20:35:13 jfinnecy Exp $
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

// if we're building mach-o we already have honest implementations of all this.

#ifndef _MAC_MACHO

#include <assert.h>
#include "macFD.h"
#include "platform/mac/socketerrors.h"

FD gFDs[FD_SETSIZE];
Boolean gFDInitialized = FALSE;


// init_fds initializes the FD array. This MUST be called at program startup
void init_fds(void)
{
	int i;
	
	if(gFDInitialized)
		return;
		
	for(i = 0; i < FD_SETSIZE; i++)
	{
		FD *temp = &gFDs[i];
		temp->index = i;
		temp->owner = NULL;
		temp->available = TRUE;
		temp->readReady = FALSE;	
		temp->writeReady = FALSE;
		temp->error = 0;
		temp->type = 0;
	}
	
	gFDInitialized = TRUE;
}

// fd_get_owner returns the FD object's owner
void *fd_get_owner(int fd)
{
	FD *temp;
	// validate fd
	assert(gFDInitialized);

	if(!FD_VALID(fd))
	{
		errno = EBADF;
		return NULL;
	}
		
	temp = &gFDs[fd];
	
	return (temp->owner);
}


// free_fd marks the FD object referenced by fd as free
int free_fd(int fd)
{
	FD *temp;
	
	// validate fd
	assert(gFDInitialized);

	if(!FD_VALID(fd))
	{
		errno = EBADF;
		return - 1;
	}
		
	temp = &gFDs[fd];
	
//	if(temp->owner)
//		delete temp->owner;
		
	temp->owner = NULL;
	temp->readReady = FALSE;	
	temp->writeReady = FALSE;
	temp->error = 0;
	temp->available = TRUE;
	temp->type = 0;
	
	return 0;
}

void fd_register(int fd, void *owner, int type)
{
	FD *theFD = get_fd_object(fd);
	if(theFD)
	{
		theFD->owner = owner;
		theFD->type = type;
	}
}

// get_fd_object returns a pointer to the actual FD object referenced by
// fd. returns NULL if fd is not a valid file descriptor

FD *get_fd_object(int fd)
{
	assert(gFDInitialized);
	
	// validate fd
	if(!FD_VALID(fd))
	{
		errno = EBADF;
		return NULL;
	}
	
	return &gFDs[fd];
}

// get_free_fd returns a file descriptor (fd) reference if there is a 
// free FD object in the FD array

int get_free_fd(void)
{
	int i;
	Boolean found = FALSE;

	assert(gFDInitialized);
	
	for(i = 0; i < FD_SETSIZE; i++)
	{
		if(gFDs[i].available)
		{
			gFDs[i].available = 0;	// mark FD as not available
			found = TRUE;
			break;
		}
	}
	
	if(!found)
	{
		errno = EMFILE;
		return -1;
	}
	else
		return i;
}

int close(int fd)
{
	return free_fd(fd);
}

// select scans the requested file descriptors to check if they are ready for
// reading and/or writing. NOTE: timeout is ignored. This version of select is
// only for polling the fds.

/*
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)

     select - synchronous I/O multiplexing

DESCRIPTION
     Select() examines the I/O descriptor sets whose addresses are passed in
     readfds, writefds, and exceptfds to see if some of their descriptors are
     ready for reading, are ready for writing, or have an exceptional condi-
     tion pending, respectively.  The first nfds descriptors are checked in
     each set; i.e., the descriptors from 0 through nfds-1 in the descriptor
     sets are examined.  On return, select() replaces the given descriptor
     sets with subsets consisting of those descriptors that are ready for the
     requested operation.  Select() returns the total number of ready descrip-
     tors in all the sets.

     The descriptor sets are stored as bit fields in arrays of integers.  The
     following macros are provided for manipulating such descriptor sets:
     FD_ZERO(&fdsetx) initializes a descriptor set fdset to the null set.
     FD_SET(fd, &fdset) includes a particular descriptor fd in fdset.
     FD_CLR(fd, &fdset) removes fd from fdset. FD_ISSET(fd, &fdset) is non-
     zero if fd is a member of fdset, zero otherwise.  The behavior of these
     macros is undefined if a descriptor value is less than zero or greater
     than or equal to FD_SETSIZE, which is normally at least equal to the max-
     imum number of descriptors supported by the system.

     If timeout is a non-nil pointer, it specifies a maximum interval to wait
     for the selection to complete.  If timeout is a nil pointer, the select
     blocks indefinitely.  To affect a poll, the timeout argument should be
     non-nil, pointing to a zero-valued timeval structure.

     Any of readfds, writefds, and exceptfds may be given as nil pointers if
     no descriptors are of interest.

RETURN VALUES
     Select() returns the number of ready descriptors that are contained in
     the descriptor sets, or -1 if an error occurred.  If the time limit ex-
     pires, select() returns 0.  If select() returns with an error, including
     one due to an interrupted call, the descriptor sets will be unmodified.

ERRORS
     An error return from select() indicates:

     [EBADF]       One of the descriptor sets specified an invalid descriptor.

     [EINTR]       A signal was delivered before the time limit expired and

                   before any of the selected events occurred.

     [EINVAL]      The specified time limit is invalid.  One of its components
                   is negative or too large.

SEE ALSO
     accept(2),  connect(2),  getdtablesize(2),  gettimeofday(2),  read(2),
     recv(2),  send(2),  write(2)

BUGS
     Although the provision of getdtablesize(2) was intended to allow user
     programs to be written independent of the kernel limit on the number of
     open files, the dimension of a sufficiently large bit field for select
     remains a problem.  The default size FD_SETSIZE (currently 256) is some-
     what larger than the current kernel limit to the number of open files.
     However, in order to accommodate programs which might potentially use a
     larger number of open files with select, it is possible to increase this
     size within a kernel by providing a larger definition of FD_SETSIZE in
     kernel configuration file and rebuilding a kernel.  To increase default
     limit user program must define its own FD_SETSIZE which is less or equal
     new kernel FD_SETSIZE before the inclusion of any header which includes
     <sys/types.h>.

     Select() should probably return the time remaining from the original
     timeout, if any, by modifying the time value in place.  This may be im-
     plemented in future versions of the system.  Thus, it is unwise to assume
     that the timeout value will be unmodified by the select() call.
*/

int select(int width, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
	int count = 0;
	int s;

	assert(gFDInitialized);
	
	// check for valid fds
	for (s = 0; s < width ; s++)
	{
		if ((readfds && FD_ISSET(s,readfds)) ||(writefds && FD_ISSET(s,writefds)) || (exceptfds && FD_ISSET(s,exceptfds)))
		{
			if (gFDs[s].owner == NULL)
				return EBADF;
		}	
	}

	// check for read/write/exception ready
	for (s = 0; s < width ; s++)
	{
		Boolean ready = FALSE;
		
		if(readfds)
		{
			if (gFDs[s].readReady && FD_ISSET(s,readfds))
				ready = TRUE;
			else
				FD_CLR(s,readfds);
		}

		if(writefds)
		{
			if (gFDs[s].writeReady && FD_ISSET(s,writefds))
				ready = TRUE;
			else
				FD_CLR(s,writefds);
		}

		if(exceptfds)
		{
			if (gFDs[s].error && FD_ISSET(s,exceptfds))
				ready = TRUE;
			else
				FD_CLR(s,exceptfds);
		}
		
		if(ready)
			count++;
	}

	return count;
}

#endif



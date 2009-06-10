/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: fsio.h,v 1.2 2003/01/23 23:42:50 damonlan Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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
#ifndef _FSIO_H_
#define _FSIO_H_ 

#include "sio.h"

#define FSIO HX_FSIO
#define SET_OFFSET 1

class IO;

class FSIO: public SIO {
public:
			FSIO(IO* i, int blksize);
			FSIO(const char* name, int flags, mode_t mode = 0);
			~FSIO();
	int		set_bufsize(int size);
	int		get_bufsize();
	Byte*		read_alloc(int& size, off_t seek, int whence);
	Byte*		write_alloc(int& size, off_t seek, int whence);
	off_t		read_seek(off_t off, int whence);
	off_t		write_seek(off_t off, int whence);
	int		read_pushback(Byte* buf, int len);
	char *		get_pathname() { return pathname; }
	int		get_would_block();

private:
	char		*pathname;
	mode_t		mode;
	int		bufsize;
	Byte*		_read_alloc(int& size);
	void		_read_free(Byte* buf);
	Byte*		_write_alloc(int& size);
	int		_write_free(Byte* buf);
	int		_write_flush(Region* reg);
	off_t		file_size();
	int		would_block;
};

#endif /* _FSIO_H_ */

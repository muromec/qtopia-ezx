/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 
/*
 * Fixed-point sampling rate conversion library
 * Developed by Ken Cooke (kenc@real.com)
 * May 2003
 *
 * Command-line test application.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "getopt.h"
#include "../resample.h"

#define ERROR(x)	fprintf(stderr, "\nError: %s\n", x), exit(-1)
#define WARNING(x)	fprintf(stderr, "\nWarning: %s\n", x)
#define ASSERT		assert

#define MAX(a,b) (((b) > (a)) ? (b) : (a))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

/* defaults */
#define DEF_INRATE	44100
#define DEF_OUTRATE	48000
#define DEF_NCHUNK	4096
#define DEF_QUALITY	1

static int inrate = DEF_INRATE;
static int outrate = DEF_OUTRATE;
static int nchunk = DEF_NCHUNK;
static int quality = DEF_QUALITY;
static int chans = 1;
static int outmode = 0;

void
Usage(char *name)
{
	char *s;

	if (s = strrchr(name, '\\'))	/* basename */
		name = s + 1;
	printf("                                               \n");
	printf("Usage: %s [options] infile outfile             \n", name);
	printf("       -c           chunk output mode          \n");
	printf("       -i INRATE    input samprate    (def=%d) \n", DEF_INRATE);
	printf("       -o OUTRATE   output samprate   (def=%d) \n", DEF_OUTRATE);
	printf("       -n CHUNK     chunking size     (def=%d) \n", DEF_NCHUNK);
	printf("       -q QUALITY   level from 0..3   (def=%d) \n", DEF_QUALITY);
	printf("       -s           stereo                     \n");
	exit(0);
}

/*
 * Parse any command-line switches using getopt.
 * Returns the index of first non-switch arg.
 */
int 
ParseArgs(int argc, char **argv)
{ 
    int c;

	while ((c = getopt(argc, argv, "ci:o:n:q:s")) != EOF) {
		switch(c) {
		case 'c':
			outmode = 1;
			break;
		case 'i':
			inrate = atoi(optarg);
			if (!inrate) Usage(argv[0]);
			break;
		case 'o':
			outrate = atoi(optarg);
			if (!outrate) Usage(argv[0]);
			break;
		case 'n':
			nchunk = atoi(optarg);
			if (!nchunk) Usage(argv[0]);
			break;
		case 'q':
			quality = atoi(optarg);;
			if (quality > 3) Usage(argv[0]);
			break;
		case 's':
			chans = 2;
			break;
		case '?':
		default:
			Usage(argv[0]);
		}
	}
	return optind;
}

int
main(int argc, char **argv)
{
	FILE *infile, *outfile;
	short *inbuf, *outbuf;
	int inbufsize, outbufsize;
	int argnext, incount, outcount, starttime;
	int nread, ninput, noutput, nwanted, nsaved;
	float calctime, audiotime;
	void *inst;	/* resampler instance */

	/* command-line switches */
	argnext = ParseArgs(argc, argv);
	if (argc - argnext != 2)
		Usage(argv[0]);

	/* open files */
	if ((infile = fopen(argv[argnext++], "rb")) == NULL)
		ERROR("Cannot open infile");
	if ((outfile = fopen(argv[argnext++], "wb")) == NULL)
		ERROR("Cannot open outfile");

	/* create resampler */
	inst = InitResampler(inrate, outrate, chans, quality);
	if (!inst)
		ERROR("InitResampler failed");

	/* determine buffer sizes */
	if (!outmode) {
		/* constant-input */
		inbufsize = nchunk;
		outbufsize = GetMaxOutput(nchunk, inst);
	} else {
		/* constant-output */
		inbufsize = GetMinInput(nchunk, inst);
		outbufsize = nchunk + GetMaxOutput(chans, inst);
	}

	/* allocate buffers */
	inbuf = (short *) malloc(inbufsize * sizeof(short));
	outbuf = (short *) malloc(outbufsize * sizeof(short));
	if (!inbuf || !outbuf)
		ERROR("malloc failed");

	printf("\nConverting %d %s to %d %s (quality=%d %s=%d)\n",
		inrate, chans==2 ? "STEREO" : "MONO",
		outrate, chans==2 ? "STEREO" : "MONO",
		quality, outmode ? "outchunk" : "inchunk", nchunk);

	incount = 0;
	outcount = 0;
	starttime = clock();

	if (outmode) {

/*
 * Process the file, in constant output chunks.
 */
		nsaved = 0;
		for (;;) {

			/* determine the amount of input needed */
			nwanted = MAX(nchunk - nsaved, 0);
			ninput = GetMinInput(nwanted, inst);
			ASSERT(ninput <= inbufsize);

			/* read variable input */
			nread = fread(inbuf, sizeof(short), ninput, infile);
			incount += nread;

			if (nread < ninput)
				break;	/* not enough input */

			/* resample, appending to saved output */
			noutput = Resample(inbuf, nread, outbuf + nsaved, inst);
			outcount += noutput;

			ASSERT(noutput >= nwanted);
			ASSERT((nsaved + noutput) >= nchunk);
			ASSERT((nsaved + noutput) <= outbufsize);
			ASSERT(noutput <= GetMaxOutput(ninput, inst));

			/* write constant output */
			fwrite(outbuf, sizeof(short), nchunk, outfile);

			/* save extra output */
			nsaved += noutput - nchunk;
			ASSERT(nsaved >= 0);
			memcpy(outbuf, outbuf + nchunk, nsaved * sizeof(short));
		}

		/* resample the remaining input */
		noutput = Resample(inbuf, nread, outbuf + nsaved, inst);
		outcount += noutput;

		/* write the remaining output */
		fwrite(outbuf, sizeof(short), nsaved + noutput, outfile);

	} else {

/*
 * Process the file, in constant input chunks.
 */
		for (;;) {

			/* read constant input */
			nread = fread(inbuf, sizeof(short), nchunk, infile);
			incount += nread;

			if (!nread)
				break;	/* done */

			/* resample */
			noutput = Resample(inbuf, nread, outbuf, inst);
			outcount += noutput;
			
			/* write variable output */
			fwrite(outbuf, sizeof(short), noutput, outfile);
		}
	}

	/* print timing info */
	calctime = (clock() - starttime) / (float)CLOCKS_PER_SEC;
	audiotime = outcount / ((float)outrate * chans);
	printf("Processed %0.2fs of audio in %0.2fs ", audiotime, calctime);
	printf("[%0.2f%% realtime]\n", calctime * 100.0f / audiotime);

	FreeResampler(inst);

	free(inbuf);
	free(outbuf);
	fclose(infile);
	fclose(outfile);

	return 0;
}

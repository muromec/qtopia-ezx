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

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mhead.h"     /* mpeg header structure, decode protos */
#include "mpadecobjfltpt.h"
#ifdef _WIN32
#include <conio.h>
#endif

#define BS_BUFBYTES		60000U
#define PCM_BUFBYTES	60000U

typedef struct _FileInfo {
	char *infileName;
	char *outfileName;
} FileInfo;

typedef struct _BitStream {
	unsigned char *buffer;
	unsigned char *bufptr;
	int trigger;      
	int bufbytes;
} BitStream;

typedef struct _PCM {
	unsigned char *buffer;
	unsigned int bufbytes;
	unsigned int trigger;
} PCM;

static int print_mpeg_info( MPEG_HEAD *h, int bitrate_arg)   /* info only */
{
	int bitrate;
	int samprate;
	static char *Layer_msg[] = { "INVALID", "III", "II", "I" };
	static char *mode_msg[] = { "STEREO", "JOINT", "DUAL", "MONO" };
	static int sr_table[8] =
    { 22050L, 24000L, 16000L, 1L,
	44100L, 48000L, 32000L, 1L };
	
	bitrate = bitrate_arg/1000;
	printf("\n Layer %s ", Layer_msg[h->option]);
	
	printf("  %s ", mode_msg[h->mode]);
	samprate = sr_table[4*h->id + h->sr_index];
	if( (h->sync & 1) == 0 ) 
		samprate = samprate/2;  // mpeg25
	printf(" %d ", samprate);
	printf("  %dKb ", bitrate);
	if( (h->mode == 1) && (h->option != 1) )
		printf("  %d stereo bands ", 4 + 4*h->mode_ext  );
	if( h->prot == 0 ) 
		printf(" (CRC)");
	
	return 0;
}

static int bs_fill(FILE *infile, BitStream *bs)
{
	int nread;
	
	if( bs->bufbytes < 0 ) 
		bs->bufbytes = 0;  // signed var could be negative
	
	if( bs->bufbytes < bs->trigger ) {
		memmove(bs->buffer, bs->bufptr, bs->bufbytes);
		nread = fread(bs->buffer + bs->bufbytes, 1, BS_BUFBYTES - bs->bufbytes, infile);
		if(nread == -1) {      /*-- test for -1 = error --*/
			bs->trigger = 0;
			printf("\n FILE_READ_ERROR\n");
			return 0; 
		}
		bs->bufbytes += nread;
		bs->bufptr = bs->buffer;
	}
	
	return 1;
}


int WINAPI DecodeMP3File(LPVOID lpParam)
{
	int bitrate, framebytes, nFrames, sampleCount;
	int in_bytes, out_bytes, writeFlag;
	FileInfo *fileInfo;
	FILE *infile, *outfile;
	unsigned int nwrite;
	unsigned long temp, inBytes, pcmBytes;
	MPEG_HEAD head;
	IN_OUT x;
	DEC_INFO decinfo;
	CMpaDecObj decobj;
	BitStream bs;
	PCM pcm;

	in_bytes = 0;
	out_bytes = 0;
	pcm.bufbytes = 0;
	pcm.trigger = (PCM_BUFBYTES - 2500*sizeof(short));
	
	/* open files */
	fileInfo = (FileInfo *)lpParam;

	infile = fopen(fileInfo->infileName, "rb");
	if (!infile)
		return -1;
	
	writeFlag = 0;
	if (strcmp(fileInfo->outfileName, "nul")) {
		writeFlag = 1;
		outfile = fopen(fileInfo->outfileName, "wb");
		if (!outfile)
			return -1;
	}
		
	/* allocate buffers */
	bs.buffer = new unsigned char [BS_BUFBYTES];
	pcm.buffer = new unsigned char [PCM_BUFBYTES];
	if (!bs.buffer || !pcm.buffer) {
		if (bs.buffer)
			delete [] bs.buffer;
		if (pcm.buffer)
			delete [] pcm.buffer;
		return -1;
	}
		
	/* fill bs buffer */
	bs.bufbytes = 0;
	bs.bufptr   = bs.buffer;
	bs.trigger  =  2500;
	bs_fill(infile, &bs);
	
	/* parse mpeg header */
	framebytes = head_info2(bs.buffer, bs.bufbytes, &head, &bitrate, 0);
	if (framebytes == 0) {
		printf("header parsing error\n");
		delete [] bs.buffer;
		delete [] pcm.buffer;
    }
	print_mpeg_info(&head, bitrate);
	
	decobj.Init_n(bs.buffer, bs.bufbytes);
	
	/*---- get info -------*/
	decobj.GetPCMInfo_v(temp, decinfo.channels, decinfo.bits);
	decinfo.samprate = temp;
	decinfo.type = 0;
	//printf(" output: samprate = %d, channels = %d, bits = %d, type = %d\n", 
	//	decinfo.samprate, decinfo.channels, decinfo.bits, decinfo.type);
	
	/*----- DECODE -----*/
	nFrames = 0;
	sampleCount = 0;
	while (1) {

		/* check for EOF */
		if(!bs_fill(infile, &bs) || bs.bufbytes < framebytes) 
			break;
		
		inBytes = bs.bufbytes;
		pcmBytes = PCM_BUFBYTES - pcm.bufbytes;
		
		/* you can uncomment this to see which file is being decoded, and watch the threads take turns */
		/* printf("decoding %s\n", fileInfo->infileName); */
		decobj.DecodeFrame_v(bs.bufptr, &inBytes, pcm.buffer + pcm.bufbytes, &pcmBytes);
	
		x.in_bytes = inBytes;
		x.out_bytes = pcmBytes;
		
		if( x.in_bytes <= 0 ) {
			printf("\n BAD SYNC IN MPEG FILE\n");
			break;
		}
		bs.bufptr += x.in_bytes;
		bs.bufbytes -= x.in_bytes;
		pcm.bufbytes += x.out_bytes;
		sampleCount += (x.out_bytes / sizeof(short));

		if( pcm.bufbytes > pcm.trigger ) {
			if (writeFlag) {
				nwrite = fwrite(pcm.buffer, 1, pcm.bufbytes, outfile);
				if( nwrite != pcm.bufbytes  ) {
					printf("\n FILE WRITE ERROR\n");
					break;
				}
			}
			out_bytes += pcm.bufbytes;
			pcm.bufbytes = 0;
		}

#ifdef _WIN32
		if( kbhit() ) 
			break;
#endif

		nFrames++;
		in_bytes += x.in_bytes;
	}
	
	if( writeFlag && pcm.bufbytes > 0 ) {
		if (writeFlag) {
			nwrite = fwrite(pcm.buffer, 1, pcm.bufbytes, outfile);
			if( nwrite != pcm.bufbytes  ) {
				printf("\n FILE WRITE ERROR\n");
			}
		}
		out_bytes += pcm.bufbytes;
		pcm.bufbytes = 0;
	}
	
	/* cleanup */
	fclose(infile);
	if (writeFlag)
		fclose(outfile);
	delete [] bs.buffer;
	delete [] pcm.buffer;
#ifdef _WIN32
	while(kbhit()) 
		getch();  /* purge key board */
#endif
	printf("\n");

	return 0;
}

void Usage(void)
{
	printf("Usage: towave infile1.mp3 outfile1.pcm [infile2.mp3 outfile2.pcm]\n");
	exit(-1);
}


int main(int argc,char *argv[])
{
	HANDLE hDecThread1, hDecThread2;
	DWORD  decThreadID1, decThreadID2;
	DWORD exitCode1, exitCode2;
	FileInfo fileInfo1, fileInfo2;

	if (argc != 3 && argc != 5)
		Usage();

	if (argc == 3 || argc == 5) {
		fileInfo1.infileName = argv[1];
		fileInfo1.outfileName = argv[2];
		hDecThread1 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DecodeMP3File, (LPVOID)&fileInfo1, 0, &decThreadID1 );
		if (hDecThread1)
			printf("Started decoder thread 1: infile = %16s    outfile = %16s\n", fileInfo1.infileName, fileInfo1.outfileName);
		else 
			printf("error creating decoder thread 1\n");
	} 
	
	if (argc == 5) {
		fileInfo2.infileName = argv[3];
		fileInfo2.outfileName = argv[4];
		hDecThread2 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DecodeMP3File, (LPVOID)&fileInfo2, 0, &decThreadID2 );
		if (hDecThread2)
			printf("Started decoder thread 2: infile = %16s    outfile = %16s\n", fileInfo2.infileName, fileInfo2.outfileName);
		else 
			printf("error creating decoder thread 2\n");
	}

	exitCode1 = exitCode2 = 0;
	do {
		if (hDecThread1)
			GetExitCodeThread(hDecThread1, &exitCode1);
		if (hDecThread2)
			GetExitCodeThread(hDecThread2, &exitCode2);
	} while (exitCode1 == STILL_ACTIVE || exitCode2 == STILL_ACTIVE);

	if (hDecThread1)
		CloseHandle(hDecThread1);
	if (hDecThread2)
		CloseHandle(hDecThread2);

	return 0;
}


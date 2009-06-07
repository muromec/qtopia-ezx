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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#include <conio.h>
#elif defined _LINUX
#include <unistd.h>
#define kbhit()	0
#define getch()
#define O_BINARY 0
#endif

#include <fcntl.h>     /* file open flags */
#include <sys/types.h>   /* someone wants for port */
#include <sys/stat.h>    /* forward slash for portability */
#include "mhead.h"     /* mpeg header structure, decode protos */
//#include "port.h"
//#include "DXhead.h"     /* Xing header in MPEG stream */

//#include "MpaDecL1.h"
//#include "MpaDecL2.h"
//#include "MpaDecL3.h"
#include "mpadecobjfltpt.h"

/* time test Pentium only */
//#define TIME_TEST

static char default_file[] = "TEST.MP3";
static char default_outfile[] = "TEST.WAV";

/*---- timing test ---*/
#ifdef TIME_TEST
static double tot_cycles;
static int  tot_cycles_n;
extern "C" {
    unsigned int set_clock(void);
    unsigned int get_clock(void);
}
//extern unsigned int global_cycles;
#endif

/*********  bitstream buffer */
#define BS_BUFBYTES 60000U
static unsigned char *bs_buffer = NULL;
static unsigned char *bs_bufptr;
static int bs_trigger;      
static int bs_bufbytes;
static int handle = -1;
/********************** */

/********  pcm buffer ********/
#define PCM_BUFBYTES  60000U
static unsigned char *pcm_buffer;
static unsigned int pcm_bufbytes;
static unsigned int pcm_trigger = (PCM_BUFBYTES - 2500*sizeof(short) ) ;
static int handout = -1;
/****************************/
static int bs_fill();


static int out_help();
static int out_mpeg_info( MPEG_HEAD *h, int bitrate_arg);
int ff_decode(char *filename, 
              char *fileout, 
              int reduction_code, 
              int convert_code, 
              int decode8_flag, 
              int freq_limit);

extern "C" {
	int cvt_to_wave_test();
	int write_pcm_header_wave( int handout,
        int samprate, int channels, int bits, int type);
	int write_pcm_tailer_wave( int handout, unsigned int pcm_bytes);
}

static float calc_br_con( MPEG_HEAD *h);

/*------------------------------------------*/
main(int argc,char *argv[])
{
	int i, k;
	char *filename;
	char *fileout;
	int convert_code;
	int reduction_code;
	int freq_limit;
	int decode8_flag;    /* decode to 8KHz */
	
	printf(
		"\n  file-file MPEG Layer I/II/III audio decode v3.07C/32"
		"\n Copyright 1995-99 RealNetworks Inc."
		"\n useage: MpaDec mpeg_file pcm_file" 
		);
	
	/****** process command line args */
	filename = default_file;
	fileout  = default_outfile;
	convert_code = 0;
	reduction_code = 0;
	freq_limit = 24000;
	decode8_flag = 0;    /* decode to 8KHz */
	for(k=0, i=1; i<argc; i++) {
		if( argv[i][0] != '-' ) {
			if( k==0 ) filename = argv[i];
			if( k==1 ) fileout  = argv[i];
			k++;
			continue;
		}
		switch ( argv[i][1] ) {
		case 'h':  case 'H':
			out_help();
			return 0;
		case 'c':  case 'C':
			convert_code = atoi(argv[i]+2);
			break;
		case 'r':  case 'R':
			reduction_code = atoi(argv[i]+2);
			break;
		case 'f':  case 'F':
			freq_limit = atoi(argv[i]+2);
			break;
		case 'd':  case 'D':
			if( atoi(argv[i]+2) == 8 ) decode8_flag = 1;
			break;
		}
	}
	
	
	printf("\n  <press any key to stop decoder>");
	
	/******** decode *********/
	ff_decode(filename, fileout, 
		reduction_code, convert_code, decode8_flag,
		freq_limit);
	/*************************/
	
	
	/*---- timing test --------*/
#ifdef TIME_TEST
	if( tot_cycles_n ) {
		// ms per frame
		tot_cycles = (1.0/266000.0) * tot_cycles /tot_cycles_n; 
		if( tot_cycles > 5.0 ) 
			printf("\n ave frame time ms = %8.1lf", tot_cycles);
		else 
			printf("\n ave frame time ms = %8.2lf", tot_cycles);
	}
#endif
	
	
	printf("\n");
	return 0;
}
/*-------------------------------------------------------------*/
static int out_help()
{
	
	printf("\n"
		"\n -D8 option decode8 (8Ks output) convert_code:"
		"\n    convert_code = 4*bit_code + chan_code"
		"\n        bit_code:   1 = 16 bit linear pcm"
		"\n                    2 =  8 bit (unsigned) linear pcm"
		"\n                    3 = u-law (8 bits unsigned)"
		"\n        chan_code:  0 = convert two chan to mono"
		"\n                    1 = convert two chan to mono"
		"\n                    2 = convert two chan to left chan"
		"\n                    3 = convert two chan to right chan"
		"\n decode (standard decoder) convert_code:"
		"\n              0 = two chan output"
		"\n              1 = convert two chan to mono"
		"\n              2 = convert two chan to left chan"
		"\n              3 = convert two chan to right chan"
		"\n              (higher bits ignored)"
		"\n decode (standard decoder) reduction_code:"
		"\n              0 = full sample rate output"
		"\n              1 = half rate"
		"\n              2 = quarter rate"
		"\n -I  option selects integer decoder"
		);
	
	return 1;
}
/*-------------------------------------------------------------*/
int ff_decode(char *filename, char *fileout,
			  int reduction_code, int convert_code, int decode8_flag,
			  int freq_limit)
{
	int framebytes;
	int u;
	MPEG_HEAD head;
	unsigned int nwrite;
	IN_OUT x;
	int in_bytes, out_bytes;
	DEC_INFO decinfo;
	int bitrate;
	//int seekpoint;
	//float percent;
	float br_con, br;
	//CMpaDecoderL3 decoder;
	//CMpaDecoderL3 *decoder = NULL;
	//CMpaDecoder *decoder = NULL;
	
	CMpaDecObj  decobj;
	/*------------------------------------------*/
	
	/*-----------------------*/
	printf("\nMPEG input file: %s", filename);
	printf("\n    output file: %s", fileout);
	/*-----------------------*/
	
	in_bytes = out_bytes = 0;
	/*-----------------------*/
	handout = -1;
	pcm_buffer = NULL;
	pcm_bufbytes = 0;
	/*-----------------------*/
	bs_buffer = NULL;
	handle = -1;
	bs_bufbytes = 0;
	bs_bufptr   = bs_buffer;
	bs_trigger  =  2500;
	/*------ open mpeg file --------*/
	handle = open(filename, O_RDONLY|O_BINARY);
	if( handle < 0 ) {
		printf("\n CANNOT_OPEN_INPUT_FILE\n");
		goto abort; }
	/*--- allocate bs buffer ----*/
	bs_buffer = new unsigned char [BS_BUFBYTES];
	if( bs_buffer == NULL ) {
		printf("\n CANNOT_ALLOCATE_BUFFER\n");
		goto abort; }
	/*--- fill bs buffer ----*/
	if( !bs_fill() ) goto abort;
	/*---- parse mpeg header  -------*/
	framebytes = head_info2(bs_buffer, bs_bufbytes, &head, &bitrate, 0);
	if( framebytes == 0 ) {
		printf("\n BAD OR UNSUPPORTED MPEG FILE\n");
		goto abort;
    }
	/*--- display mpeg info --*/
	out_mpeg_info(&head, bitrate);
	
	/*if( head.option == 1 )          // Layer III
    decoder = new CMpaDecoderL3;
	else if( head.option == 2 )     // Layer II
    decoder = new CMpaDecoderL2;
	else if( head.option == 3 )     // Layer I
    decoder = new CMpaDecoderL1;
	else {
    printf("\n BAD OR UNSUPPORTED MPEG OPTION\n");
    goto abort;
}*/
	
	decobj.Init_n(bs_buffer, bs_bufbytes);
	
	
	br_con = calc_br_con(&head);
	/*---- create pcm file ------*/
	handout = open(fileout, O_RDWR|O_BINARY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE);
	if( handout < 0 ) {
		printf("\n CANNOT CREATE OUTPUT FILE\n");
		goto abort;
    }
	/*---- allocate pcm buffer --------*/
	pcm_buffer = new unsigned char [PCM_BUFBYTES];
	if( pcm_buffer == NULL ) {
		printf("\n CANNOT ALLOCATE PCM BUFFER\n");
		goto abort;
    }
	
	/*---- init decoder -------*/
	//if( !decoder->audio_decode_init(&head,framebytes,
	//        reduction_code,0,convert_code,freq_limit) ) {
	//    printf("\n DECODER INIT FAIL \n");
	//    goto abort;
	//    }
	
	/*---- get info -------*/
	//decoder->audio_decode_info( &decinfo);
	unsigned long ulTemp;
	//decoder->GetPCMInfo_v(ulTemp, decinfo.channels, decinfo.bits);
	decobj.GetPCMInfo_v(ulTemp, decinfo.channels, decinfo.bits);
	decinfo.samprate = ulTemp;
	decinfo.type = 0;
	/*---- info display -------*/
	printf("\n output samprate = %6ld", decinfo.samprate);
	printf("\n output channels = %6d", decinfo.channels);
	printf("\n output bits     = %6d", decinfo.bits);
	printf("\n output type     = %6d", decinfo.type);
	
	
	
#ifdef ENABLE_WAVE_HEADER
	/*---- write pcm header -------*/
	if( !write_pcm_header_wave(handout,
        decinfo.samprate, decinfo.channels, decinfo.bits, decinfo.type ) ) {
		printf("\n FILE WRITE ERROR\n");
		goto abort;
    }
#endif
	
	unsigned long   ulBytes,
		ulPcmBytes;
	printf("\n");
	/*----- DECODE -----*/
	for(u=0;;) {
		if( !bs_fill() ) break;
		if( bs_bufbytes < framebytes ) break;  /* end of file */
#ifdef TIME_TEST
		set_clock();
#endif
		//x = decoder->audio_decode(bs_bufptr, (pcm_buffer+pcm_bufbytes));
		ulBytes = bs_bufbytes;
		ulPcmBytes = PCM_BUFBYTES - pcm_bufbytes;
		
		decobj.DecodeFrame_v(bs_bufptr, &ulBytes, pcm_buffer+pcm_bufbytes, &ulPcmBytes);
		
		x.in_bytes = ulBytes;
		x.out_bytes = ulPcmBytes;
		
#ifdef TIME_TEST
		tot_cycles += get_clock(); 
		tot_cycles_n++;
		//tot_cycles+=global_cycles; 
#endif
		if( x.in_bytes <= 0 ) {
			printf("\n BAD SYNC IN MPEG FILE\n");
			break;
		}
		bs_bufptr += x.in_bytes;
		bs_bufbytes -= x.in_bytes;
		pcm_bufbytes += x.out_bytes;
		u++;
		if( pcm_bufbytes > pcm_trigger ) {
			nwrite = write(handout, pcm_buffer, pcm_bufbytes );
			if( nwrite != pcm_bufbytes  ) {
				printf("\n FILE WRITE ERROR\n");
				break;
			}
			out_bytes += pcm_bufbytes;
			pcm_bufbytes = 0;
		}
		if( kbhit() ) break;
		in_bytes  += x.in_bytes;
		if( (u & 127) == 1 )  {
			br = br_con * in_bytes / u;
			printf("\r frames %6ld   bytes in %6ld    bytes out %6ld  %6.1f",
				u, in_bytes, out_bytes, br);
		}
	}
	/*---------------*/
	
	if( pcm_bufbytes > 0 ) {
		nwrite = write(handout, pcm_buffer, pcm_bufbytes );
		if( nwrite != pcm_bufbytes  ) {
			printf("\n FILE WRITE ERROR\n");
		}
		out_bytes += pcm_bufbytes;
		pcm_bufbytes = 0;
	}
	
	br = br_con * in_bytes / u;
	printf("\n frames %6ld   bytes in %6ld    bytes out %6ld  %6.1f",
		u, in_bytes, out_bytes, br);
	
#ifdef ENABLE_WAVE_HEADER
	/*---- write pcm tailer -------*/
	write_pcm_tailer_wave(handout, out_bytes);
#endif
	
	printf("\n    output file: %s", fileout);
	
abort:
	//delete decoder;
	close(handle);
	close(handout);
	delete [] bs_buffer;
	delete [] pcm_buffer;
	while( kbhit() ) getch();  /* purge key board */
	return 1;
}
/*-------------------------------------------------------------*/
static int bs_fill()
{
	unsigned int nread;
	
	if( bs_bufbytes < 0 ) bs_bufbytes = 0;  // signed var could be negative
	if( bs_bufbytes < bs_trigger ) {
		memmove(bs_buffer, bs_bufptr, bs_bufbytes);
		nread = read(handle,bs_buffer+bs_bufbytes, BS_BUFBYTES-bs_bufbytes);
		if( (nread+1) == 0 ) {      /*-- test for -1 = error --*/
			bs_trigger = 0;
			printf("\n FILE_READ_ERROR\n");
			return 0; }
		bs_bufbytes += nread;
		bs_bufptr = bs_buffer;
	}
	
	return 1;
}
/*------------------------------------------------------------------*/
static int out_mpeg_info( MPEG_HEAD *h, int bitrate_arg)   /* info only */
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
	if( (h->sync & 1) == 0 ) samprate = samprate/2;  // mpeg25
	printf(" %d ", samprate);
	printf("  %dKb ", bitrate);
	if( (h->mode == 1) && (h->option != 1) )
		printf("  %d stereo bands ", 4 + 4*h->mode_ext  );
	if( h->prot == 0 ) printf(" (CRC)");
	
	return 0;
}
/*------------------------------------------------------------------*/
static float calc_br_con( MPEG_HEAD *h)   /* info only */
{
	float br_con;
	int samprate;
	static int sr_table[8] =
    { 22050, 24000, 16000L, 1,
	44100, 48000, 32000L, 1 };
	
	samprate = sr_table[4*h->id + h->sr_index];
	
	if( h->option == 1 ) {      // layer III
		if( (h->sync & 1) == 0 ) samprate = samprate/2;  // mpeg25
		if( h->id == 1 ) {
			br_con = (float)(0.001*8.0*samprate/1152.0);
		}
		else {
			br_con = (float)(0.001*8.0*samprate/576.0);
		}
	}
	else if( h->option == 2 ) {      // layer II
        br_con = (float)(0.001*8.0*samprate/1152.0);
	}
	else if( h->option == 3 ) {      // layer I
        br_con = (float)(0.001*8.0*samprate/384.0);
	}
	else {
        br_con = (float)(0.001*8.0*samprate/1152.0);
	}
	
	
	return br_con;
}
/*------------------------------------------------------------------*/
int dummy()
{
    return 0;
}
/*------------------------------------------------------------------*/




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

// PN generic media stream types(audio,video,image) in network byte order
#define HX_MEDIA_NATIVE		0x4E545645L	// 'NTVE'	
#define HX_MEDIA_AUDIO		0x4155444FL 	// 'AUDO'
#define HX_MEDIA_VIDEO		0x5649444FL 	// 'VIDO'
#define HX_MEDIA_IMAGE		0x494D4147L 	// 'IMAG'
#define HX_MEDIA_IMAGEMAP 	0x494D4150L 	// 'IMAP'
#define HX_MEDIA_SYNCMM 	0x53594D4DL 	// 'SYMM'
#define HX_MEDIA_PACKETIZER	0x504B4554L	// 'PKET'
#define HX_MEDIA_SYNCAD 	0x53594144L 	// 'SYAD'
#define HX_MEDIA_IMAGE2		0x494D4742L 	// 'IMG2'

#define HX_MEDIA_OTHER	0x4F544852L	// 'OTHR'

#define HX_H263VIDEO_ID	0x52563130L	// 'RV10'
#define HX_RV20VIDEO_ID	0x52563230L	// 'RV20'
#define HX_RV30VIDEO_ID	0x52563330L	// 'RV30'
#define HX_RV4XVIDEO_ID	0x52563458L	// 'RV4X'
#define HX_RV40VIDEO_ID	0x52563430L	// 'RV40'
#define HX_RV41VIDEO_ID	0x52563431L	// 'RV41'
#define HX_RVTRVIDEO_ID 0x52565452	// 'RVTR' (for rv20 codec)
#define HX_RVTR_RV30_ID 0x52565432	// 'RVT2' (for rv30 codec)
#define HX_RGB3VIDEO_ID	0x52474256L	// 'RGBV'
#define HX_RGB3_ID	0x52474233L	// 'RGB3'
#define HX_RGB555_ID	0x52474235L	// 'RGB5'
#define HX_RGB565_ID	0x52474236L	// 'RGB6'
#define HX_RGB24_ID	0x20524742L	// ' RGB' 24 bit RGB top-down format
#define HX_8BIT_ID	0x38424954L	// '8BIT' 8 Bit dithered format

#define HX_AVC1VIDEO_ID 0x41564331 // 'AVC1'
#define HX_MP4VVIDEO_ID 0x4d503456 // 'MP4V'
#define HX_CLEARVIDEO_ID 0x434C5631 // 'CLV1'

#define HX_YUV420_ID	0x59555632L	// 'YUV2'
#define HX_YUV411_ID	0x59555631L	// 'YUV1'
#define HX_YUVRAW_ID	0x59555652L	// 'YUVR'
#define RA_WTIMAGE_ID	0x52493130L	// 'RI10'

// ICM codecs
#define ICM_CVID	0x43564944L	// 'CVID'
#define ICM_IV31	0x39563331L	// 'IV31'
#define ICM_IV32	0x39563332L	// 'IV32'
#define ICM_IV41	0x39563431L	// 'IV41'
#define ICM_MRLE	0x4D524C45L	// 'MRLE'
#define ICM_CRAM	0x4352414DL	// 'CRAM'

#ifdef _WIN16
#define ICM_CVID_NAME	"ICCVID.DLL"
#define ICM_IV31_NAME	"IR32_32.DLL"
#define ICM_IV32_NAME	"IR32_32.DLL"
#define ICM_IV41_NAME	"IR41_32.DLL"
#define ICM_MRLE_NAME	"MSVIDEO.DLL"
#define ICM_CRAM_NAME	"MSVIDEO.DLL"
#else
#define ICM_CVID_NAME	"ICCVID.DLL"
#define ICM_IV31_NAME	"IR32_32.DLL"
#define ICM_IV32_NAME	"IR32_32.DLL"
#define ICM_IV41_NAME	"IR41_32.DLL"
#define ICM_MRLE_NAME	"MSRLE32.DLL"
#define ICM_CRAM_NAME	"MSVIDC32.DLL"
#endif /* _WIN16 */

// ACM codecs
#define ACM_CODEC	0x41554443L	// 'AUDC'

#define ACM32_NAME	"MSACM32.DLL"
#define ACM16_NAME	"MSACM.DLL"


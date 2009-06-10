/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved. 
 *	
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.	You may not use this file except in 
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

#ifndef _HXAUDIODEVICEHOOK_INTERFACE_
#define _HXAUDIODEVICEHOOK_INTERFACE_


#ifdef __cplusplus
extern "C" {
#endif	/* C++ */

/* Forward Declarations */ 
#ifdef __cplusplus
typedef class CHXAudioDeviceHook CHXAudioDeviceHook;
#else
typedef struct CHXAudioDeviceHook CHXAudioDeviceHook;
#endif /* __cplusplus */

/* Interface Definition */
#undef  INTERFACE
#define INTERFACE  IHXAudioDeviceHookDMO
DECLARE_INTERFACE_(IHXAudioDeviceHookDMO, IUnknown)
{
    // IHXAudioDeviceHookDMO methods
    STDMETHOD(Init)     (THIS_ IUnknown* pContext) PURE;
};

/* GUID Definition */
DEFINE_GUID(CLSID_HXAUDIODEVICEHOOK, 0X2CFA30DA, 0X118B, 0X4CA3, 0XAA, 0XF3, 0XF4, 0X74, 0X16, 0X23, 0X2, 0XE5);
DEFINE_GUID(IID_IHXAudioDeviceHookDMO, 0x8b183d4a, 0x4e72, 0x42fe, 0xb5, 0x18, 0x30, 0x73, 0x42, 0x3d, 0xcf, 0x39);

#ifdef __cplusplus
class DECLSPEC_UUID("2cfa30da-118b-4ca3-aaf3-f474162302e5") CHXAudioDeviceHook;
#endif

#ifdef __cplusplus
};	/* extern "C" */
#endif	/* C++ */

#endif // _HXAUDIODEVICEHOOK_INTERFACE_


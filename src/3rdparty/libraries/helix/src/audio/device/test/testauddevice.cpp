#/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: testauddevice.cpp,v 1.3 2004/07/09 18:38:31 hubbe Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _SYMBIAN
#include <e32math.h>
#endif
#include "hxresult.h"
#include "hxcom.h"
#include "hxausvc.h"
#include "hxengin.h"
#include "ihxpckts.h"   
#include "hxtypes.h"
#include "hxaudev.h"
#include "hxbuffer.h"

extern "C"
{
    
int audioTestEntry(void)
{
    HX_RESULT res = HXR_OK; 
    
    //Create an audio device.
    fprintf( stderr, "Creating an audio device instance....\n" ); 
    CHXAudioDevice* pDevice = NULL;
    pDevice = CHXAudioDevice::Create();
    if( !pDevice )
    {
        fprintf( stderr, "couldn't create device. *FAILED*\n" );
        sleep(2);
        return 1;
    }

    //Set up the format...
    HXAudioFormat audioFormat;
    audioFormat.uChannels        = 2;     // 1 or 2
    audioFormat.uBitsPerSample   = 16;    // 8 or 16
    audioFormat.ulSamplesPerSec  = 44100; // 11025, 21050, 44100.

    //The block size we will make the number of bytes required for
    //1/10th of a second of audio given the above format. The block
    //size is the amount of data we write to the audio device each
    //time.
    int bytesPerSample    = audioFormat.uChannels*audioFormat.uBitsPerSample/8;  
    int samplesInOneTenth = audioFormat.ulSamplesPerSec/10;
    const int blockSize   = bytesPerSample*samplesInOneTenth;
    audioFormat.uMaxBlockSize = blockSize;
    fprintf( stderr, "Block Size set to %d\n", audioFormat.uMaxBlockSize ); 

    //It is OK to fail this as not all audio devices support all
    //formats. If it does fail the client core would choose the next
    //'lower' format until it finds one the device supports. at that
    //point it would use the resampler to downsample the pcm. We won't
    //do that here, just make sure whatever device you are testing
    //support the above format.
    fprintf( stderr, "Checking the format.....\n" );
    res = pDevice->CheckFormat( &audioFormat );
    if( res!=HXR_OK )
    {
        fprintf( stderr, "check format *FAILED*.\n" );
        sleep(2);
        return 1;
    }

    //open the device.
    fprintf( stderr, "Opening the device....\n" ); 
    res = pDevice->Open( &audioFormat, NULL );
    if( FAILED(res) )
    {
        fprintf( stderr, "*FAILED* to open audio device.\n" );
        sleep(2);
        return 1;
    }

    //Init the volume. This does not have to be supported by devices.
    if( pDevice->InitVolume(0,100))
        fprintf( stderr, "Device supports volume....\n" );
    else
        fprintf( stderr, "Device does not support volume....\n" ); 

    //Get and set the volume...
    int curVolume = pDevice->GetVolume();
    fprintf( stderr, "Current volume is %d\n", curVolume );

    int newVolume = 90;
    if( curVolume == newVolume )
        newVolume = 950;
        
    fprintf( stderr, "Setting volume to %d\n", newVolume );
    pDevice->SetVolume(newVolume);
    if( pDevice->GetVolume() != newVolume )
    {
        fprintf( stderr, "*FAILED* to set volume\n" );
        sleep(2);
        return 1;
    }


    //Pause the device until we are ready....
    fprintf( stderr, "Pausing audio device\n" );
    res = pDevice->Pause();
    if( FAILED(res) )
    {
        fprintf( stderr, "*FAILED* to pause device....\n" );
        sleep(2);
        return 1;
    }
    

    //try to play some pcm data.....

    //
    // Pump in PCM data here. Make sure we pump it in so fast that the
    // device falls behind so we can do some pause/resume stuff
    // below...
    //
    fprintf( stderr, "Pushing down 5 seconds of PCM...\n" ); 
    unsigned char* szBuff = new unsigned char[blockSize];
    HXAudioData   audioData;
    audioData.ulAudioTime=0;
    
    //Fill our 1/10th second buffer 50 times. 5 seconds of total audio.
    INT16 n = 0;
    TReal cur = 0.0;
    TReal xfade = 0.0;
    TInt  dir=1.0;
    
    for( int i=0 ; i<50 ; i++ )
    {
        TReal rad = (2.0*3.1415926)/(float)audioFormat.ulSamplesPerSec * 300.0*(float)i/5 ;
        int byte=0;
        for( int sample=0; sample<audioFormat.ulSamplesPerSec/10; sample++ )
        {
            for( int channel=0; channel<audioFormat.uChannels; channel++ )
            {
                //Fill each channel with some data
                TReal amp;
                Math::Sin(amp, cur); //got the sample.
                if( audioFormat.uBitsPerSample == 16 )
                {
                    amp = amp*((1<<15)-1);
                    INT16* pTmp = (INT16*)(szBuff+byte);
                    if( channel == 0 )
                        *pTmp = (INT16)(amp*xfade);
                   else
                       *pTmp = (INT16)(amp*(1-xfade));
                        
                    byte += 2;
                }
                else
                {
                    amp = amp*127+127;
                    szBuff[byte]=(INT8)amp;
                    byte++;
                }
            }
            cur = cur+rad;
            if( cur>=(2*3.1415926) )
                cur = 0;

            xfade = xfade + (1.0/(5.0*(float)audioFormat.ulSamplesPerSec))*10*dir;
            if(xfade>1)
            {
                dir *= -1;
                xfade=1;
            }
            if(xfade<0)
            {
                dir *= -1;
                xfade=0;
            }
            
        }
        
        //write this block of data to the audio device.
        audioData.pData = new CHXBuffer();
        if( !audioData.pData )
        {
            fprintf( stderr, "Out of memory....\n" );
            return 1;
        }
        
        audioData.pData->AddRef();
        audioData.pData->Set(szBuff, blockSize);
        res = pDevice->Write(&audioData);
        HX_RELEASE( audioData.pData );
        if( FAILED(res) )
        {
            fprintf( stderr, "*FAILED* to write to the audio device\n" );
            pDevice->Close(TRUE);
            sleep(2);
            return 1;
        }
    }

    HX_VECTOR_DELETE(szBuff);

    fprintf( stderr, "Resuming the device you should hear 5 seconds of sound...\n" );
    res = pDevice->Resume();
    if( FAILED(res) )
    {
        fprintf( stderr, "*FAILED* to resume...\n" );
        sleep(2);
        return 1;
    }

    

//     ULONG32 ulTime = 0;
//     while( pDevice->NumberOfBlocksRemainingToPlay() )
//     {
//         pDevice->Write(NULL);
//         pDevice->GetCurrentAudioTime(ulTime);
//         fprintf( stderr, "Current Audio time is: %lu\n", ulTime ); 
//     }
    

//     //Close the device
//     res = pDevice->Close(FALSE);
//     if( FAILED(res) )
//     {
//         fprintf( stderr, "*FAILED* to close the device...\n" );
//         sleep(2);
//         return 1;
//     }
    
    fprintf( stderr, "\nAudio device unit test *PASSED*. Give yourself a gold star\n" ); 
    return 0;
}
};

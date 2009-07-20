/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audlinux_alsa.cpp,v 1.8 2006/02/07 19:33:51 ping Exp $
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
 * Contributor(s):  ljp <ljp@llornkcor.com>
 * 
 * ***** END LICENSE BLOCK ***** */

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <stdio.h> 
#include <math.h>

#include "ihxpckts.h"
#include "hxtick.h"
#include "hxprefs.h"
#include "timeval.h"
#include "hxthread.h"
#include "audlinux_alsa.h"
#include "hxstrutl.h"
#include "pckunpck.h"

#include "hxtlogutil.h"
#include "ihxtlogsystem.h"

#include "dllacces.h"
#include "dllpath.h"

#include "hxbuffer.h"
#include "hxprefs.h"

extern IHXPreferences* z_pIHXPrefs;

//------------------------------------------
// Ctors and Dtors.
//------------------------------------------
CAudioOutLinuxAlsa::CAudioOutLinuxAlsa() :
    CAudioOutUNIX(),
    m_pAlsaPCMHandle (NULL),
    m_pAlsaMixerHandle (NULL),
    m_pAlsaMixerElem (NULL),

    m_pPCMDeviceName (NULL),
    m_pMixerDeviceName (NULL),
    m_pMixerElementName (NULL),

    m_bHasHardwarePauseAndResume (FALSE),
    m_nBytesPlayedBeforeLastTrigger(0),

    m_nLastBytesPlayed(0),

    m_bGotInitialTrigger(FALSE),
    m_bUseMMAPTStamps(TRUE)
{
}

CAudioOutLinuxAlsa::~CAudioOutLinuxAlsa()
{
    if(m_pPCMDeviceName)
    {
        HX_RELEASE(m_pPCMDeviceName);
    }

    if(m_pMixerDeviceName)
    {
        HX_RELEASE(m_pMixerDeviceName);
    }

    if(m_pMixerElementName)
    {
        HX_RELEASE(m_pMixerElementName);
    }
}

// These Device Specific methods must be implemented
// by the platform specific sub-classes.
INT16 CAudioOutLinuxAlsa::_Imp_GetAudioFd(void)
{
    //Not implemented.
    return -1;
}


//Device specific methods to open/close the mixer and audio devices.
HX_RESULT CAudioOutLinuxAlsa::_OpenAudio()
{
    int err = 0;
    const char* szDevice;

    HX_ASSERT (m_pAlsaPCMHandle == NULL);
    if (m_pAlsaPCMHandle)
    {
        m_wLastError = RA_AOE_BADOPEN;
        return m_wLastError;
    }

    if(z_pIHXPrefs)
    {
        HX_RELEASE(m_pPCMDeviceName);
        z_pIHXPrefs->ReadPref("AlsaPCMDeviceName", m_pPCMDeviceName);
    }

    if(!m_pPCMDeviceName)
    {
        const char szDefaultDevice[] = "default";

	CreateAndSetBufferCCF(m_pPCMDeviceName,
			     (const unsigned char*) szDefaultDevice,
			     sizeof(szDefaultDevice),
			     m_pContext);
    }

    szDevice = (const char*) m_pPCMDeviceName->GetBuffer();

    HXLOGL2 (HXLOG_ADEV, "Opening ALSA PCM device %s", 
             szDevice);
    
    err = snd_pcm_open( &m_pAlsaPCMHandle, 
                        szDevice, 
                        SND_PCM_STREAM_PLAYBACK, 
                        0);
    if(err < 0)
    {
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_open: %s",
                  szDevice, snd_strerror (err));

        m_wLastError = RA_AOE_BADOPEN;
    }

    if(err == 0)
    {
        err = snd_pcm_nonblock(m_pAlsaPCMHandle, FALSE);
        if(err < 0)
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_nonblock: %s",
                      snd_strerror (err));

            m_wLastError = RA_AOE_BADOPEN;
        }
    }

    if(err == 0)
    {
        m_wLastError = RA_AOE_NOERR;
    }
    else
    {
        if(m_pAlsaPCMHandle)
        {
            snd_pcm_close(m_pAlsaPCMHandle);
            m_pAlsaPCMHandle = NULL;
        }
    }

    return m_wLastError;
}


HX_RESULT CAudioOutLinuxAlsa::_CloseAudio()
{
    if (!m_pAlsaPCMHandle)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    HXLOGL2 (HXLOG_ADEV, "Closing ALSA PCM device");
   
    snd_pcm_close(m_pAlsaPCMHandle);
    m_pAlsaPCMHandle = NULL;
    m_wLastError = RA_AOE_NOERR;

    return m_wLastError;
}


HX_RESULT CAudioOutLinuxAlsa::_OpenMixer()
{
    int err;
    const char* szDeviceName = NULL;
    const char* szElementName = NULL;
    int nElementIndex = 0;

    HX_ASSERT (m_pAlsaMixerHandle == NULL);
    if (m_pAlsaMixerHandle != NULL)
    {
        m_wLastError = RA_AOE_BADOPEN;
        return m_wLastError;
    }

    HX_ASSERT(m_pAlsaMixerElem == NULL);
    if (m_pAlsaMixerElem != NULL)
    {
        m_wLastError = RA_AOE_BADOPEN;
        return m_wLastError;
    }

    if(z_pIHXPrefs)
    {
        HX_RELEASE(m_pMixerDeviceName);
        z_pIHXPrefs->ReadPref("AlsaMixerDeviceName", m_pMixerDeviceName);
    }

    if(!m_pMixerDeviceName)
    {
        const char szDefaultDevice[] = "default";

	CreateAndSetBufferCCF(m_pMixerDeviceName,
			     (const unsigned char*) szDefaultDevice,
			     sizeof(szDefaultDevice),
			     m_pContext);
    }

    if(z_pIHXPrefs)
    {
        HX_RELEASE(m_pMixerElementName);        
        z_pIHXPrefs->ReadPref("AlsaMixerElementName", m_pMixerElementName);
    }

    if(!m_pMixerElementName)
    {
        const char szDefaultElement[] = "PCM";

	CreateAndSetBufferCCF(m_pMixerElementName,
			      (const unsigned char*) szDefaultElement,
			      sizeof(szDefaultElement),
			      m_pContext);
    }

    if(z_pIHXPrefs)
    {
        IHXBuffer* pElementIndex = NULL;
        z_pIHXPrefs->ReadPref("AlsaMixerElementIndex", pElementIndex);
        if(pElementIndex)
        {
            const char* szElementIndex = (const char*) pElementIndex->GetBuffer();
            nElementIndex = atoi(szElementIndex);

            HX_RELEASE(pElementIndex);        
        }
    }

    szDeviceName  = (const char*) m_pMixerDeviceName->GetBuffer();;
    szElementName = (const char*) m_pMixerElementName->GetBuffer();
    
    HXLOGL2 (HXLOG_ADEV, "Opening ALSA mixer device %s", 
             szDeviceName);

    err = snd_mixer_open(&m_pAlsaMixerHandle, 0);
    if (err < 0)
    {
        HXLOGL1 ( HXLOG_ADEV, "snd_mixer_open: %s",
                  snd_strerror (err));
        m_wLastError = RA_AOE_BADOPEN;
    }
  
    if (err == 0)
    {
        err = snd_mixer_attach(m_pAlsaMixerHandle, szDeviceName);
        if (err < 0) 
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_mixer_attach: %s",
                      snd_strerror (err));
            m_wLastError = RA_AOE_BADOPEN;
        }
    }

    if (err == 0)
    {
        err = snd_mixer_selem_register(m_pAlsaMixerHandle, NULL, NULL);
        if (err < 0) 
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_mixer_selem_register: %s",
                      snd_strerror (err));
            m_wLastError = RA_AOE_BADOPEN;
        }
    }

    if (err == 0)
    {
        err = snd_mixer_load(m_pAlsaMixerHandle);
        if(err < 0 )
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_mixer_load: %s",
                      snd_strerror (err));
            m_wLastError = RA_AOE_NOTENABLED;
        }
    }

    if (err == 0)
    {
        /* Find the mixer element */
        snd_mixer_elem_t* fallback_elem = NULL;
        snd_mixer_elem_t* elem = snd_mixer_first_elem (m_pAlsaMixerHandle);
        snd_mixer_elem_type_t type;
        const char* elem_name = NULL;
        snd_mixer_selem_id_t *sid = NULL;
        int index;

        snd_mixer_selem_id_alloca(&sid);

        while (elem)
        {
            type = snd_mixer_elem_get_type(elem);
            if (type == SND_MIXER_ELEM_SIMPLE)
            {
                snd_mixer_selem_get_id(elem, sid);

                /* We're only interested in playback volume controls */
                if(snd_mixer_selem_has_playback_volume(elem) &&
                   !snd_mixer_selem_has_common_volume(elem))
                {
                    if (!fallback_elem)
                    {
                        fallback_elem = elem;
                    }

                    elem_name = snd_mixer_selem_id_get_name (sid);
                    index = snd_mixer_selem_id_get_index(sid);
                    if (strcmp(elem_name, szElementName) == 0 &&
                        index == nElementIndex)
                    {
                        break;
                    }
                }
            }
            
            elem = snd_mixer_elem_next(elem);
        }

        if (!elem && fallback_elem)
        {
            elem = fallback_elem;
            elem_name = NULL;
            type = snd_mixer_elem_get_type(elem);
            
            if (type == SND_MIXER_ELEM_SIMPLE)
            {
                snd_mixer_selem_get_id(elem, sid);
                elem_name = snd_mixer_selem_id_get_name (sid);
            }

            HXLOGL1 ( HXLOG_ADEV, "Could not find element %s, using element %s instead",
                     m_pMixerElementName,  elem_name? elem_name: "unknown");
        }
        else if (!elem)
        {
            HXLOGL1 ( HXLOG_ADEV, "Could not find a usable mixer element",
                      snd_strerror (err));
            m_wLastError = RA_AOE_BADOPEN;
            err = -1;
        }

        m_pAlsaMixerElem = elem;
    }
    
    if(err == 0)
    {
        if (m_pAlsaMixerHandle)
        {
            m_bMixerPresent = 1;
            _Imp_GetVolume();
        }
        else
        {
            m_bMixerPresent = 0;
        }

        m_wLastError = RA_AOE_NOERR;
    }
    else
    {
        if(m_pAlsaMixerHandle)
        {
            snd_mixer_close(m_pAlsaMixerHandle);
            m_pAlsaMixerHandle = NULL;
        }
    }

    return m_wLastError;
}

HX_RESULT CAudioOutLinuxAlsa::_CloseMixer()
{
    int err;
    const char* szMixerDeviceName = NULL;

    if (!m_pAlsaMixerHandle)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    if (!m_pMixerDeviceName)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;        
    }

    szMixerDeviceName = (const char*) m_pMixerDeviceName->GetBuffer();
    err = snd_mixer_detach(m_pAlsaMixerHandle, szMixerDeviceName);
    if (err < 0)
    {
        HXLOGL1 ( HXLOG_ADEV, "snd_mixer_detach: %s",
                  snd_strerror (err));
        m_wLastError = RA_AOE_GENERAL;
    }
  
    if(err == 0)
    {
        err = snd_mixer_close(m_pAlsaMixerHandle);
        if(err < 0)
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_mixer_close: %s",
                      snd_strerror (err));            
            m_wLastError = RA_AOE_GENERAL;
        }
    }

    if(err == 0)
    {
        m_pAlsaMixerHandle = NULL;
        m_pAlsaMixerElem = NULL;
        m_wLastError = RA_AOE_NOERR;
    }

    return m_wLastError;
}


//Devic specific method to set the audio device characteristics. Sample rate,
//bits-per-sample, etc.
//Method *must* set member vars. m_unSampleRate and m_unNumChannels.
HX_RESULT CAudioOutLinuxAlsa::_SetDeviceConfig( const HXAudioFormat* pFormat )
{
    snd_pcm_state_t state;

    HX_ASSERT(m_pAlsaPCMHandle != NULL);
    if (!m_pAlsaPCMHandle)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    state = snd_pcm_state(m_pAlsaPCMHandle);
    if (state != SND_PCM_STATE_OPEN)
    {
        HXLOGL1 ( HXLOG_ADEV, "Device is not in open state in CAudioOutLinuxAlsa::_SetDeviceConfig (%d)", (int) state);
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    /* Translate from HXAudioFormat to ALSA-friendly values */
    snd_pcm_format_t fmt;
    unsigned int sample_rate = 0;
    unsigned int channels = 0;
    unsigned int buffer_time = 500000;          /* 0.5 seconds */
    unsigned int period_time = buffer_time / 4; /* 4 interrupts per buffer */

    switch (pFormat->uBitsPerSample)
    {
    case 8:
        fmt = SND_PCM_FORMAT_S8;
        break;

    case 16:
        fmt = SND_PCM_FORMAT_S16_LE;        
        break;

    case 24:
        fmt = SND_PCM_FORMAT_S24_LE;
        break;

    case 32:
        fmt = SND_PCM_FORMAT_S32_LE;
        break;

    default:
        fmt = SND_PCM_FORMAT_UNKNOWN;
        break;        
    }
    
    if (fmt == SND_PCM_FORMAT_UNKNOWN)
    {
        HXLOGL1 ( HXLOG_ADEV, "Unknown bits per sample: %d", pFormat->uBitsPerSample);
        m_wLastError = RA_AOE_NOTENABLED;
        return m_wLastError;
    }
    sample_rate = pFormat->ulSamplesPerSec;
    channels = pFormat->uChannels;

    /* Apply to ALSA */
    int err = 0;
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_sw_params_t *swparams;

	snd_pcm_hw_params_alloca(&hwparams);
	snd_pcm_sw_params_alloca(&swparams);

    /* Hardware parameters */
	err = snd_pcm_hw_params_any(m_pAlsaPCMHandle, hwparams);
	if (err < 0) 
    {
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_any: %s", snd_strerror(err));
        m_wLastError = RA_AOE_NOTENABLED;
	}

    if (err == 0)
    {
        err = snd_pcm_hw_params_set_access(m_pAlsaPCMHandle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
        if (err < 0) 
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_set_access: %s", snd_strerror(err));
            m_wLastError = RA_AOE_NOTENABLED;
        }
    }

    if (err == 0)
    {
        err = snd_pcm_hw_params_set_format(m_pAlsaPCMHandle, hwparams, fmt);
        if (err < 0) 
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_set_format: %s", snd_strerror(err));
            m_wLastError = RA_AOE_NOTENABLED;
        }
    }

    if (err == 0)
    {
        err = snd_pcm_hw_params_set_channels(m_pAlsaPCMHandle, hwparams, channels);
        if (err < 0) 
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_set_channels: %s", snd_strerror(err));
            m_wLastError = RA_AOE_NOTENABLED;
        }
    }

    if (err == 0)
    {
        unsigned int sample_rate_out;
        sample_rate_out = sample_rate;

        err = snd_pcm_hw_params_set_rate_near(m_pAlsaPCMHandle, hwparams, &sample_rate_out, 0);
        if (err < 0) 
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_set_channels: %s", snd_strerror(err));
            m_wLastError = RA_AOE_NOTENABLED;
        }
        
        if (sample_rate_out != sample_rate)
        {
            HXLOGL2 ( HXLOG_ADEV, "Requested a sample rate of %d, got a rate of %d", 
                      sample_rate, sample_rate_out);

            sample_rate = sample_rate_out;
        }
    }

    if (err == 0)
    {
        unsigned int buffer_time_out;
        buffer_time_out = buffer_time;

        err = snd_pcm_hw_params_set_buffer_time_near(m_pAlsaPCMHandle, hwparams, &buffer_time_out, 0);
        if (err < 0) 
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_set_buffer_time_near: %s", 
                      snd_strerror(err));
            m_wLastError = RA_AOE_NOTENABLED;
        }

        if (buffer_time_out != buffer_time)
        {
            HXLOGL2 ( HXLOG_ADEV, "Requested a buffering time of %d, got a time of %d", 
                      buffer_time, buffer_time_out);

            buffer_time = buffer_time_out;
        }
    }

    if (err == 0)
    {
        unsigned int period_time_out;
        period_time_out = period_time;

        err = snd_pcm_hw_params_set_period_time_near(m_pAlsaPCMHandle, hwparams, &period_time_out, 0);
        if (err < 0) 
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_set_period_time_near: %s", 
                      snd_strerror(err));
            m_wLastError = RA_AOE_NOTENABLED;
        }

        if (period_time_out != period_time)
        {
            HXLOGL2 ( HXLOG_ADEV, "Requested a period time of %d, got a period of %d", 
                      period_time, period_time_out);
            period_time = period_time_out;
        }
    }

    /* Apply parameters */
	err = snd_pcm_hw_params(m_pAlsaPCMHandle, hwparams);
	if (err < 0) 
    {
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params: %s", 
                  snd_strerror(err));
        m_wLastError = RA_AOE_NOTENABLED;
	}

    /* read buffer & period sizes */
    snd_pcm_uframes_t buffer_size = 0;
    snd_pcm_uframes_t period_size = 0;

    if (err == 0)
    {
        err = snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size);
        if (err < 0) 
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_get_buffer_size: %s", 
                      snd_strerror(err));
            m_wLastError = RA_AOE_NOTENABLED;
        }
        else
        {
            HX_ASSERT (buffer_size > 0);
        }
    }

    if (err == 0)
    {
        err = snd_pcm_hw_params_get_period_size(hwparams, &period_size, 0);
        if (err < 0) 
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_get_period_size: %s", 
                      snd_strerror(err));
            m_wLastError = RA_AOE_NOTENABLED;
        }
    }

    /* Get hardware pause */
    if (err == 0)
    {
        int can_pause = 0;
        int can_resume = 0;

        can_pause = snd_pcm_hw_params_can_resume(hwparams);
        can_resume = snd_pcm_hw_params_can_pause(hwparams);

        m_bHasHardwarePauseAndResume = (can_pause && can_resume);
    }

    /* Software parameters */
    if (err == 0)
    {
        err = snd_pcm_sw_params_current(m_pAlsaPCMHandle, swparams);
        if (err < 0) 
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_sw_params_current: %s", 
                      snd_strerror(err));
            m_wLastError = RA_AOE_NOTENABLED;
        }
    }    

    snd_pcm_uframes_t start_threshold = ((buffer_size - 1) / period_size) * period_size;

    if (err == 0)
    {
        err = snd_pcm_sw_params_set_start_threshold(m_pAlsaPCMHandle, swparams, start_threshold);
        if (err < 0) 
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_sw_params_set_start_threshold: %s", 
                      snd_strerror(err));
            m_wLastError = RA_AOE_NOTENABLED;
        }
    }
    
    if (err == 0)
    {
        err = snd_pcm_sw_params_set_avail_min(m_pAlsaPCMHandle, swparams, period_size);
        if (err < 0) 
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_sw_params_set_avail_min: %s", 
                      snd_strerror(err));
            m_wLastError = RA_AOE_NOTENABLED;
        }
    }

    if (err == 0)
    {
        err = snd_pcm_sw_params_set_xfer_align(m_pAlsaPCMHandle, swparams, 1);
        if (err < 0) 
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_sw_params_set_xfer_align: %s", 
                      snd_strerror(err));
            m_wLastError = RA_AOE_NOTENABLED;
        }
    }

    if (err == 0)
    {
        err = snd_pcm_sw_params_set_tstamp_mode(m_pAlsaPCMHandle, swparams, SND_PCM_TSTAMP_MMAP);
        if (err < 0) 
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_sw_params_set_xfer_align: %s", 
                      snd_strerror(err));
            m_wLastError = RA_AOE_NOTENABLED;
        }
    }

    if (err == 0)
    {
        err = snd_pcm_sw_params_set_stop_threshold(m_pAlsaPCMHandle, swparams, ~0U);
        if (err < 0) 
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_sw_params_set_stop_threshold: %s", 
                      snd_strerror(err));
            m_wLastError = RA_AOE_NOTENABLED;
        }
    }

    if (err == 0)
    {
        err = snd_pcm_sw_params(m_pAlsaPCMHandle, swparams);
        if (err < 0) 
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_sw_params: %s", 
                      snd_strerror(err));
            m_wLastError = RA_AOE_NOTENABLED;
        }
    }

    /* If all the calls to this point have succeeded, move to the PREPARE state. 
       We will enter the RUNNING state when we've buffered enough for our start theshold. */
    if (err == 0)
    {
        err = snd_pcm_prepare (m_pAlsaPCMHandle);
        if (err < 0)
        {
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_prepare: %s", 
                      snd_strerror(err));
            m_wLastError = RA_AOE_NOTENABLED;
        }
    }

    /* Sanity check: See if we're now in the PREPARE state */
    if (err == 0)
    {
        snd_pcm_state_t state;
        state = snd_pcm_state (m_pAlsaPCMHandle);
        if (state != SND_PCM_STATE_PREPARED)
        {
            HXLOGL1 ( HXLOG_ADEV, "Expected to be in PREPARE state, actually in state %d", 
                      (int) state);
            m_wLastError = RA_AOE_NOTENABLED;
        }
    }

    /* Use avail to get the alsa buffer size, which is distinct from the hardware buffer 
       size. This will match what GetRoomOnDevice uses. */
    int alsa_buffer_size = 0;
    err = snd_pcm_avail_update(m_pAlsaPCMHandle);
    if(err < 0)
    {
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_avail_update: %s", snd_strerror(err));
    }
    else
    {
        alsa_buffer_size = snd_pcm_frames_to_bytes(m_pAlsaPCMHandle, err);
        err = 0;
    }

    if (err == 0)
    {
        m_wLastError = RA_AOE_NOERR;

        m_unSampleRate  = sample_rate;
        m_unNumChannels = channels;
        m_wBlockSize    = m_ulBytesPerGran;
        m_ulDeviceBufferSize = alsa_buffer_size;
        m_uSampFrameSize = snd_pcm_frames_to_bytes(m_pAlsaPCMHandle, 1) / channels;
        
        HXLOGL2 ( HXLOG_ADEV,  "Device Configured:\n");
        HXLOGL2 ( HXLOG_ADEV,  "         Sample Rate: %d",  m_unSampleRate);
        HXLOGL2 ( HXLOG_ADEV,  "        Sample Width: %d",  m_uSampFrameSize);
        HXLOGL2 ( HXLOG_ADEV,  "        Num channels: %d",  m_unNumChannels);
        HXLOGL2 ( HXLOG_ADEV,  "          Block size: %d",  m_wBlockSize);
        HXLOGL2 ( HXLOG_ADEV,  "  Device buffer size: %lu", m_ulDeviceBufferSize);
        HXLOGL2 ( HXLOG_ADEV,  "   Supports HW Pause: %d",  m_bHasHardwarePauseAndResume);
        HXLOGL2 ( HXLOG_ADEV,  "     Start threshold: %d",  start_threshold);


    }
    else
    {
        m_unSampleRate = 0;
        m_unNumChannels = 0;

        if (m_pAlsaPCMHandle)
        {
            _CloseAudio();            
        }
    }
    
    return m_wLastError;
}

//Device specific method to write bytes out to the audiodevice and return a
//count of bytes written.
HX_RESULT CAudioOutLinuxAlsa::_WriteBytes( UCHAR* buffer, ULONG32 ulBuffLength, LONG32& lCount )
{
    int err = 0;
    unsigned int frames_written = 0;
    snd_pcm_sframes_t num_frames;
    ULONG32 ulBytesToWrite = ulBuffLength;
    ULONG32 ulBytesWrote = 0;
    
    lCount = 0;

    HX_ASSERT(m_pAlsaPCMHandle);
    if (!m_pAlsaPCMHandle)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    m_wLastError = RA_AOE_NOERR;

    if (ulBuffLength == 0)
    {
        lCount = ulBuffLength;
        return m_wLastError;
    }


    do
    {
        num_frames = snd_pcm_bytes_to_frames(m_pAlsaPCMHandle, ulBytesToWrite);
        err = snd_pcm_writei( m_pAlsaPCMHandle, buffer, num_frames );
        
        if (err >= 0)
        {
            frames_written = err;
            ulBytesWrote = snd_pcm_frames_to_bytes (m_pAlsaPCMHandle, frames_written);
            buffer += ulBytesWrote;
            ulBytesToWrite -= ulBytesWrote;
            lCount += ulBytesWrote;
        }
        else
        {
            switch (err)
            {
            case -EAGAIN:
                break;

            case -EPIPE:
                HandleXRun();
                lCount = (LONG32) ulBuffLength;
                break;
               
            case -ESTRPIPE:
                HandleSuspend();
                lCount = (LONG32) ulBuffLength;
                break;

            default:
                HXLOGL1 ( HXLOG_ADEV, "snd_pcm_writei: %s", snd_strerror(err));
                m_wLastError = RA_AOE_DEVBUSY;
            }
        }
    } while (err == -EAGAIN || (err>0 && ulBytesToWrite>0));

    HX_ASSERT( lCount == ulBuffLength );
    return m_wLastError;
}

/* Subtract the `struct timeval' values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0.  */

int
timeval_subtract (struct timeval *result, 
                  const struct timeval *x, 
                  const struct timeval *y_orig)
{
    struct timeval y = *y_orig;
    
    /* Perform the carry for the later subtraction by updating Y. */
    if (x->tv_usec < y.tv_usec) 
    {
        int nsec = (y.tv_usec - x->tv_usec) / 1000000 + 1;
        y.tv_usec -= 1000000 * nsec;
        y.tv_sec += nsec;
    }
    if ((x->tv_usec - y.tv_usec) > 1000000) 
    {
        int nsec = (x->tv_usec - y.tv_usec) / 1000000;
        y.tv_usec += 1000000 * nsec;
        y.tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
       `tv_usec' is certainly positive. */
    result->tv_sec = x->tv_sec - y.tv_sec;
    result->tv_usec = x->tv_usec - y.tv_usec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y.tv_sec;
}

HX_RESULT CAudioOutLinuxAlsa::GetBytesActuallyPlayedUsingTStamps(UINT64 &nBytesPlayed) const
{
    HX_RESULT retVal = HXR_FAIL;

    int err = 0;

    snd_timestamp_t trigger_tstamp, now_tstamp, diff_tstamp;
    snd_pcm_status_t* status;

    snd_pcm_status_alloca(&status);
    
    err = snd_pcm_status(m_pAlsaPCMHandle, status);
    if (err < 0)
    {
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_status: %s", snd_strerror(err));
    }

    if (err == 0)
    {
        snd_pcm_status_get_tstamp(status, &now_tstamp);
        snd_pcm_status_get_trigger_tstamp(status, &trigger_tstamp);

        if(!m_bGotInitialTrigger && now_tstamp.tv_sec == 0 && now_tstamp.tv_usec == 0)
        {
            /* Our first "now" timestamp appears to be invalid (or the user is very unlucky, and
               happened to start playback as the timestamp rolls over). Fall back to using 
               snd_pcm_delay. 
              
               XXXRGG: Is there a better way to figure out if the driver supports mmap'd 
               timestamps? */

            m_bUseMMAPTStamps = FALSE;
        }
        else
        {
            /* Timestamp seems to be valid */
            if(!m_bGotInitialTrigger)
            {
                m_bGotInitialTrigger = TRUE;
                memcpy(&m_tstampLastTrigger, &trigger_tstamp, sizeof(m_tstampLastTrigger));
            }
            else
            {
                if(memcmp(&m_tstampLastTrigger, &trigger_tstamp, sizeof(m_tstampLastTrigger)) != 0)
                {
                    /* There's been a trigger since last time -- restart the timestamp counter
                       XXXRGG: What if there's been multiple triggers? */
                    m_nBytesPlayedBeforeLastTrigger = m_nLastBytesPlayed;
                    memcpy(&m_tstampLastTrigger, &trigger_tstamp, sizeof(m_tstampLastTrigger));

                    HXLOGL1 ( HXLOG_ADEV, "Retriggered...");
                }
            }

            timeval_subtract (&diff_tstamp, &now_tstamp, &m_tstampLastTrigger);

            double fTimePlayed = (double) diff_tstamp.tv_sec + 
                ((double) diff_tstamp.tv_usec / 1e6);
            
            nBytesPlayed = (UINT64) ((fTimePlayed * (double) m_unSampleRate * m_uSampFrameSize * m_unNumChannels) + m_nBytesPlayedBeforeLastTrigger);
            retVal = HXR_OK;
        }    
    }

    return retVal;
}

HX_RESULT CAudioOutLinuxAlsa::GetBytesActuallyPlayedUsingDelay (UINT64 &nBytesPlayed) const
{
    HX_RESULT retVal = HXR_FAIL;
    int err = 0;
    snd_pcm_sframes_t frame_delay = 0;

    err = snd_pcm_delay (m_pAlsaPCMHandle, &frame_delay);
    if (err < 0)
    {
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_status: %s", snd_strerror(err));        
    }
    else
    {
        int bytes_delay;
        bytes_delay = snd_pcm_frames_to_bytes (m_pAlsaPCMHandle, frame_delay);

        nBytesPlayed = m_ulTotalWritten - bytes_delay;
        retVal = HXR_OK;
    }

//    HXLOGL4 ( HXLOG_ADEV, "nBytesPlayed: %llu, m_ulTotalWritten: %llu\n", nBytesPlayed, m_ulTotalWritten);

    return retVal;
}

HX_RESULT CAudioOutLinuxAlsa::GetBytesActuallyPlayedUsingAvail(UINT64 &nBytesPlayed) const
{
    /* Try this the hwsync way. This method seems to crash & burn with dmix,
       as avail seems to come from the device, and varies depending on what other
       dmix clients are writing to the slave device. Currently not used for that reason. */

    HX_RESULT retVal = HXR_FAIL;
    int err = 0;

    err = snd_pcm_hwsync(m_pAlsaPCMHandle);
    if(err < 0)
    {
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hwsync: %s", snd_strerror(err));        
    }

    err = snd_pcm_avail_update(m_pAlsaPCMHandle);
    if(err < 0)
    {
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_avail_update: %s", snd_strerror(err));        
    }
    else
    {
        snd_pcm_sframes_t avail = err;
        int bytes_avail;
        bytes_avail = snd_pcm_frames_to_bytes (m_pAlsaPCMHandle, avail);

        nBytesPlayed = m_ulTotalWritten - (m_ulDeviceBufferSize - bytes_avail);
        retVal = HXR_OK;
    }

    return retVal;
}

HX_RESULT CAudioOutLinuxAlsa::GetBytesActuallyPlayedUsingTimer(UINT64 &nBytesPlayed) const
{
    /* Look at the alsa timer api, and how we can lock onto it as a timer source. */

    return HXR_FAIL;
}

UINT64 CAudioOutLinuxAlsa::_GetBytesActualyPlayed(void) const
{
    HX_ASSERT(m_pAlsaPCMHandle);
    if (!m_pAlsaPCMHandle)
    {
        return 0;
    }

    HX_RESULT retVal = HXR_OK;
    UINT64 nBytesPlayed = 0;
    snd_pcm_state_t state;

    for(;;)
    {
        state = snd_pcm_state(m_pAlsaPCMHandle);
        switch(state)
        {
        case SND_PCM_STATE_OPEN:
        case SND_PCM_STATE_SETUP:
        case SND_PCM_STATE_PREPARED:
            /* If we're in one of these states, written and played should match. */
            m_nLastBytesPlayed = m_ulTotalWritten;
            return m_nLastBytesPlayed;

        case SND_PCM_STATE_XRUN:
            HandleXRun();
            continue;
     
        case SND_PCM_STATE_RUNNING:
            break;

        case SND_PCM_STATE_PAUSED:
            // return m_nLastBytesPlayed;
            break;

        case SND_PCM_STATE_DRAINING:
        case SND_PCM_STATE_SUSPENDED:
        case SND_PCM_STATE_DISCONNECTED:
            HX_ASSERT(!"Not reached");
            break;            
        }

        break;
    }

    // XXXRGG: Always use the delay method for now.
    m_bUseMMAPTStamps = FALSE;

    if (m_bUseMMAPTStamps)
    {
        retVal = GetBytesActuallyPlayedUsingTStamps(nBytesPlayed);
    }
 
    if (!m_bUseMMAPTStamps || FAILED(retVal))
    {
        /* MMAP'd timestamps are fishy. Try using snd_pcm_delay. */
        retVal = GetBytesActuallyPlayedUsingDelay(nBytesPlayed);            
    }

    m_nLastBytesPlayed = nBytesPlayed;
    return nBytesPlayed;
}


//this must return the number of bytes that can be written without blocking.
HX_RESULT CAudioOutLinuxAlsa::_GetRoomOnDevice(ULONG32& ulBytes) const
{
    ulBytes = 0;

    HX_ASSERT(m_pAlsaPCMHandle);
    if (!m_pAlsaPCMHandle)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    int err = 0;
    err = snd_pcm_avail_update(m_pAlsaPCMHandle);
    if(err > 0)
    {
        ulBytes = snd_pcm_frames_to_bytes(m_pAlsaPCMHandle, err);
    }
    else
    {
        switch (err)
        {
        case -EAGAIN:
            break;

        case -EPIPE:
            HandleXRun();
            break;
               
        case -ESTRPIPE:
            HandleSuspend();
            break;

        default:
            HXLOGL1 ( HXLOG_ADEV, "snd_pcm_avail_update: %s", snd_strerror(err));
            m_wLastError = RA_AOE_DEVBUSY;
        }
    }

//    HXLOGL4 ( HXLOG_ADEV, "RoomOnDevice: %d", ulBytes);    

    return m_wLastError;
}


//Device specific method to get/set the devices current volume.
UINT16 CAudioOutLinuxAlsa::_GetVolume() const
{
    HX_ASSERT(m_pAlsaMixerElem);
    if (!m_pAlsaMixerElem)
    {
        return 0;
    }

    UINT16 nRetVolume = 0;

    snd_mixer_elem_type_t type;    
    int err = 0;
    type = snd_mixer_elem_get_type(m_pAlsaMixerElem);
            
    if (type == SND_MIXER_ELEM_SIMPLE)
    {
        long volume, min_volume, max_volume; 

        if(snd_mixer_selem_has_playback_volume(m_pAlsaMixerElem) || 
           snd_mixer_selem_has_playback_volume_joined(m_pAlsaMixerElem))
        {
            err = snd_mixer_selem_get_playback_volume(m_pAlsaMixerElem,
                                                      SND_MIXER_SCHN_MONO, 
                                                      &volume);
            if (err < 0)
            {
                HXLOGL1 ( HXLOG_ADEV, "snd_mixer_selem_get_playback_volume: %s",
                          snd_strerror (err));
            }

            if (err == 0)
            {
                snd_mixer_selem_get_playback_volume_range(m_pAlsaMixerElem,
                                                          &min_volume,
                                                          &max_volume);

                if(max_volume > min_volume)
                {
                    nRetVolume = (UINT16) (100 * ((double) volume / (max_volume - min_volume)));
                }
            }
        }        
    }

    return nRetVolume;
}


HX_RESULT CAudioOutLinuxAlsa::_SetVolume(UINT16 unVolume)
{
    m_wLastError = RA_AOE_NOERR;

    HX_ASSERT(m_pAlsaMixerElem);
    if (!m_pAlsaMixerElem)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    snd_mixer_elem_type_t type;    
    int err = 0;
    type = snd_mixer_elem_get_type(m_pAlsaMixerElem);
            
    if (type == SND_MIXER_ELEM_SIMPLE)
    {
        long volume, min_volume, max_volume, range; 

        if(snd_mixer_selem_has_playback_volume(m_pAlsaMixerElem) || 
           snd_mixer_selem_has_playback_volume_joined(m_pAlsaMixerElem))
        {
            snd_mixer_selem_get_playback_volume_range(m_pAlsaMixerElem,
                                                      &min_volume,
                                                      &max_volume);

            range = max_volume - min_volume;
            volume = (long) (((double) unVolume / 100.0) * range + min_volume);

            err = snd_mixer_selem_set_playback_volume( m_pAlsaMixerElem,
                                                       SND_MIXER_SCHN_FRONT_LEFT, 
                                                       volume);            
            if (err < 0)
            {
                HXLOGL1 ( HXLOG_ADEV, "snd_mixer_selem_set_playback_volume: %s",
                          snd_strerror (err));
                m_wLastError = RA_AOE_GENERAL;
            }

            if (!snd_mixer_selem_is_playback_mono (m_pAlsaMixerElem))
            {
                /* Set the right channel too */
                err = snd_mixer_selem_set_playback_volume( m_pAlsaMixerElem,
                                                           SND_MIXER_SCHN_FRONT_RIGHT, 
                                                           volume);            
                if (err < 0)
                {
                    HXLOGL1 ( HXLOG_ADEV, "snd_mixer_selem_set_playback_volume: %s",
                              snd_strerror (err));
                    m_wLastError = RA_AOE_GENERAL;
                }
            }
        }        
    }    

    return m_wLastError;
}

//Device specific method to drain a device. This should play the remaining
//bytes in the devices buffer and then return.
HX_RESULT CAudioOutLinuxAlsa::_Drain()
{
    m_wLastError = RA_AOE_NOERR;

    HX_ASSERT(m_pAlsaPCMHandle);
    if (!m_pAlsaPCMHandle)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    int err = 0;

    err = snd_pcm_drain(m_pAlsaPCMHandle);
    if (err < 0)
    {
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_drain: %s",
                  snd_strerror (err));
        m_wLastError = RA_AOE_GENERAL;
    }
    
    err = snd_pcm_prepare(m_pAlsaPCMHandle);
    if (err < 0)
    {
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_prepare: %s",
                  snd_strerror (err));
        m_wLastError = RA_AOE_GENERAL;
    }

    return m_wLastError;
}


//Device specific method to reset device and return it to a state that it
//can accept new sample rates, num channels, etc.
HX_RESULT CAudioOutLinuxAlsa::_Reset()
{
    if (!m_pAlsaPCMHandle)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    m_wLastError = RA_AOE_NOERR;

    m_nLastBytesPlayed = 0;

    int err = 0;

    err = snd_pcm_drop(m_pAlsaPCMHandle);
    if (err < 0)
    {
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_drop: %s",
                  snd_strerror (err));
        m_wLastError = RA_AOE_GENERAL;
    }

    err = snd_pcm_prepare(m_pAlsaPCMHandle);
    if (err < 0)
    {
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_prepare: %s",
                  snd_strerror (err));
        m_wLastError = RA_AOE_GENERAL;
    }

    return m_wLastError;
}

HX_RESULT CAudioOutLinuxAlsa::_CheckFormat( const HXAudioFormat* pFormat )
{
    HX_ASSERT(m_pAlsaPCMHandle == NULL);

    m_wLastError = _OpenAudio();
    if(m_wLastError != RA_AOE_NOERR)
    {
        return m_wLastError;
    }

    m_wLastError = RA_AOE_NOERR;

    snd_pcm_format_t fmt;
    unsigned int sample_rate = 0;
    unsigned int channels = 0;

    switch (pFormat->uBitsPerSample)
    {
    case 8:
        fmt = SND_PCM_FORMAT_S8;
        break;

    case 16:
        fmt = SND_PCM_FORMAT_S16_LE;        
        break;

    case 24:
        fmt = SND_PCM_FORMAT_S24_LE;
        break;

    case 32:
        fmt = SND_PCM_FORMAT_S32_LE;
        break;

    default:
        fmt = SND_PCM_FORMAT_UNKNOWN;
        break;        
    }
    
    if (fmt == SND_PCM_FORMAT_UNKNOWN)
    {
        HXLOGL1 ( HXLOG_ADEV, "Unknown bits per sample: %d", pFormat->uBitsPerSample);
        m_wLastError = RA_AOE_NOTENABLED;
        return m_wLastError;
    }
    sample_rate = pFormat->ulSamplesPerSec;
    channels = pFormat->uChannels;

    /* Apply to ALSA */
    int err = 0;
	snd_pcm_hw_params_t *hwparams;

	snd_pcm_hw_params_alloca(&hwparams);

	err = snd_pcm_hw_params_any(m_pAlsaPCMHandle, hwparams);
	if (err < 0) 
    {
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_any: %s", snd_strerror(err));
        m_wLastError = RA_AOE_NOTENABLED;
	}

    if (err == 0)
    {
        err = snd_pcm_hw_params_test_rate (m_pAlsaPCMHandle, hwparams, sample_rate, 0);
        if (err < 0)
        {
            m_wLastError = RA_AOE_BADFORMAT;
        }
    }

    if (err == 0)
    {
        err = snd_pcm_hw_params_test_channels (m_pAlsaPCMHandle, hwparams, channels);
        if (err < 0)
        {
            m_wLastError = RA_AOE_BADFORMAT;
        }
    }

    if (err == 0)
    {
        err = snd_pcm_hw_params_test_format (m_pAlsaPCMHandle, hwparams, fmt);
        if (err < 0)
        {
            m_wLastError = RA_AOE_BADFORMAT;
        }
    }

    _CloseAudio();

    return m_wLastError;
}


HX_RESULT CAudioOutLinuxAlsa::_CheckSampleRate( ULONG32 ulSampleRate )
{
    HX_ASSERT(m_pAlsaPCMHandle == NULL);

    m_wLastError = _OpenAudio();
    if(m_wLastError != RA_AOE_NOERR)
    {
        return m_wLastError;
    }

    int err = 0;
	snd_pcm_hw_params_t *hwparams;

	snd_pcm_hw_params_alloca(&hwparams);

    m_wLastError = RA_AOE_NOERR;

	err = snd_pcm_hw_params_any(m_pAlsaPCMHandle, hwparams);
	if (err < 0) 
    {
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_hw_params_any: %s", snd_strerror(err));
        m_wLastError = RA_AOE_NOTENABLED;
	}
    
    if (err == 0)
    {
        err = snd_pcm_hw_params_test_rate (m_pAlsaPCMHandle, hwparams, ulSampleRate, 0);
        if (err < 0)
        {
            m_wLastError = RA_AOE_BADFORMAT;
        }
    }

    _CloseAudio();

    return m_wLastError;
}


HX_RESULT CAudioOutLinuxAlsa::_Pause()
{
    HX_ASSERT(m_pAlsaPCMHandle);
    if (!m_pAlsaPCMHandle)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    if (m_bHasHardwarePauseAndResume)
    {
        snd_pcm_state_t state;

        state = snd_pcm_state(m_pAlsaPCMHandle);
        if (state == SND_PCM_STATE_RUNNING)
        {
            int err = 0;
            err = snd_pcm_pause(m_pAlsaPCMHandle, 1);
            if (err < 0)
            {
                HXLOGL1 ( HXLOG_ADEV, "snd_pcm_pause: %s",
                          snd_strerror (err));
            
                m_wLastError = RA_AOE_NOTSUPPORTED;
            }
        }
    }
    else
    {
        m_wLastError = RA_AOE_NOTSUPPORTED;
    }
    
    return m_wLastError;
}

HX_RESULT CAudioOutLinuxAlsa::_Resume()
{
    HX_ASSERT(m_pAlsaPCMHandle);
    if (!m_pAlsaPCMHandle)
    {
        m_wLastError = RA_AOE_DEVNOTOPEN;
        return m_wLastError;
    }

    if (m_bHasHardwarePauseAndResume)
    {
        snd_pcm_state_t state;

        state = snd_pcm_state(m_pAlsaPCMHandle);
        if (state == SND_PCM_STATE_PAUSED)
        {
            int err = 0;
            err = snd_pcm_pause(m_pAlsaPCMHandle, 0);

            if (err < 0)
            {
                HXLOGL1 ( HXLOG_ADEV, "snd_pcm_pause: %s",
                          snd_strerror (err));
            
                m_wLastError = RA_AOE_NOTSUPPORTED;
            }
        }
    }
    else
    {
        m_wLastError = RA_AOE_NOTSUPPORTED;
    }
    
    return m_wLastError;
}

HXBOOL CAudioOutLinuxAlsa::_HardwarePauseSupported() const
{
    HX_ASSERT(m_pAlsaPCMHandle != NULL);

    return m_bHasHardwarePauseAndResume;
}


void CAudioOutLinuxAlsa::HandleXRun(void) const
{
    int err = 0;

    HXLOGL2 ( HXLOG_ADEV, "Handling XRun");

    err = snd_pcm_prepare(m_pAlsaPCMHandle);
    if (err < 0)
    {
        HXLOGL1 ( HXLOG_ADEV, "snd_pcm_resume: %s (xrun)",
                  snd_strerror (err));
    }

    /* Catch up to the write position of the audio device so we get new data.
       XXXRGG: Is there some way we, the device, can force a rewind? */
    m_nLastBytesPlayed = m_ulTotalWritten;
}

void CAudioOutLinuxAlsa::HandleSuspend(void) const
{
    int err = 0;

    do
    {
        err = snd_pcm_resume(m_pAlsaPCMHandle);
        if (err == 0)
        {
            break;
        }
        else if (err == -EAGAIN)
        {
            usleep(1000);
        }
    } while (err == -EAGAIN);

    if (err < 0) 
    {
        HandleXRun();
    }
}

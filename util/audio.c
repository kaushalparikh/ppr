
/* System headers */
#include <stdio.h>
#include <alsa/asoundlib.h>

/* Local/project headers */
#include "types.h"
#include "util.h"

/* PCM device */
#define PLAYBACK_DEVICE    "hw:0,0"
#define CAPTURE_DEVICE     "hw:0,0"

/* HW sampling rate in Hz */
#define HW_SAMPLING_RATE   (48000)

/* SW sampling rate in Hz */
#define SW_SAMPLING_RATE   (8000)

/* Frame duration in millisec */
#define FRAME_DURATION     (20)

#define SAMPLES_PER_FRAME  ((HW_SAMPLING_RATE * FRAME_DURATION)/1000)

/* Local structures */
typedef struct
{
  snd_pcm_t *playback_handle;
  snd_pcm_t *capture_handle;
} audio_device_t;

/* File scope global variables */
static audio_device_t audio_device =
{
  .playback_handle = NULL,
  .capture_handle  = NULL
};


/* Upsample from 8kHz to 48kHz, i.e. by 6 */
static void audio_upsample (int16 *input, int16 *output, uint32 length)
{
  uint32 count;
  uint32 sample;

  for (count = 0; count < length; count++)
  {
    for (sample = 0; sample < 6; sample++)
    {
      output[2*((6*count)+sample)]     = input[2*count];
      output[(2*((6*count)+sample))+1] = input[(2*count)+1];
    }
  }
}

/* Downsample from 48kHz to 8kHz, i.e. by 6 */
static void audio_downsample (int16 *input, int16 *output, uint32 length)
{
  uint32 count;

  for (count = 0; count < length; count++)
  {
    output[2*count]     = input[2*6*count];
    output[(2*count)+1] = input[(2*6*count)+1];
  }
}

int32 audio_playback (int16 *frame_buffer, uint32 frames)
{
  int status = -1;

  /* Convert to ALSA frame defintion, i.e. 1 frame = 1 sample */
  frames *= SAMPLES_PER_FRAME;
  while (frames > 0)
  {
    status = snd_pcm_avail_update (audio_device.playback_handle);
    if (status > 0)
    {
      if ((status = snd_pcm_writei (audio_device.playback_handle,
                                    frame_buffer, frames)) > 0)
      {
        frames       -= status;
        frame_buffer += (2 * status);
      }
    }
   
    if ((status < 0) &&
        ((status = snd_pcm_prepare (audio_device.playback_handle)) < 0))
    {
      printf ("Unable to prepare playback\n");
      break;
    }
  }

  return status;
}

int32 audio_capture (int16 *frame_buffer, uint32 frames)
{
  int status = -1;

  /* Convert to ALSA frame defintion, i.e. 1 frame = 1 sample */
  frames *= SAMPLES_PER_FRAME;
  while (frames > 0)
  {
    status = snd_pcm_avail_update (audio_device.capture_handle);
    if (status > 0)
    {
      if ((status = snd_pcm_readi (audio_device.capture_handle,
                                   frame_buffer, frames)) > 0)
      {
        frames       -= status;
        frame_buffer += (2 * status);
      }
    }

    if ((status < 0) &&
        ((status = snd_pcm_prepare (audio_device.capture_handle)) < 0))
    {
      printf ("Unable to capture playback\n");
      break;
    }
  }

  return status;
}

int32 audio_init (void)
{
  int status;
  snd_pcm_hw_params_t *audio_hw = NULL;

  /* Playback configuration */
  if ((status = snd_pcm_open (&(audio_device.playback_handle), PLAYBACK_DEVICE,
                              SND_PCM_STREAM_PLAYBACK, 0)) < 0)
  {
    printf ("Unable to open PCM device for playback\n");
  }

  if ((status == 0) &&
      ((status = snd_pcm_hw_params_malloc (&audio_hw)) < 0))
  {
    printf ("Unable to allocate memory for playback configuration\n");
  }
  
  if ((status == 0) &&
      ((status = snd_pcm_hw_params_any (audio_device.playback_handle,
                                        audio_hw)) < 0))
  {
    printf ("Unable to fill default playback configuration\n");
  }

  if ((status == 0) &&
      ((status = snd_pcm_hw_params_set_access (audio_device.playback_handle,
                                               audio_hw,
                                               SND_PCM_ACCESS_RW_INTERLEAVED)) < 0))
  {
    printf ("Unable to set playback interleaved access\n");
  }

  if ((status == 0) &&
      ((status = snd_pcm_hw_params_set_format (audio_device.playback_handle,
                                               audio_hw,
                                               SND_PCM_FORMAT_S16_LE)) < 0))
  {
    printf ("Unable to set playback format\n");
  }

  if ((status == 0) &&
      ((status = snd_pcm_hw_params_set_rate (audio_device.playback_handle,
                                             audio_hw, HW_SAMPLING_RATE, 0)) < 0))
  {
    printf ("Unable to set playback sampling rate\n");
  }

  if ((status == 0) &&
      ((status = snd_pcm_hw_params_set_channels (audio_device.playback_handle,
                                                 audio_hw, 2)) < 0))
  {
    printf ("Unable to set playback channels\n");
  }

  if ((status == 0) &&
      ((status = snd_pcm_hw_params (audio_device.playback_handle, audio_hw)) < 0))
  {
    printf ("Unable to apply playback configuration\n");
  }

  if (audio_hw != NULL)
  {
    snd_pcm_hw_params_free (audio_hw);
    audio_hw = NULL;
  }
  
  /* Capture configuration */
  if ((status = snd_pcm_open (&(audio_device.capture_handle), CAPTURE_DEVICE,
                              SND_PCM_STREAM_CAPTURE, 0)) < 0)
  {
    printf ("Unable to open PCM device for capture\n");
  }

  if ((status == 0) &&
      ((status = snd_pcm_hw_params_malloc (&audio_hw)) < 0))
  {
    printf ("Unable to allocate memory for capture configuraiton\n");
  }

  if ((status == 0) &&
      ((status = snd_pcm_hw_params_any (audio_device.capture_handle,
                                        audio_hw)) < 0))
  {
    printf ("Unable to fill default capture configuration\n");
  }

  if ((status == 0) &&
      ((status = snd_pcm_hw_params_set_access (audio_device.capture_handle,
                                               audio_hw,
                                               SND_PCM_ACCESS_RW_INTERLEAVED)) < 0))
  {
    printf ("Unable to set capture interleaved access\n");
  }

  if ((status == 0) &&
      ((status = snd_pcm_hw_params_set_format (audio_device.capture_handle,
                                               audio_hw,
                                               SND_PCM_FORMAT_S16_LE)) < 0))
  {
    printf ("Unable to set capture format\n");
  }

  if ((status == 0) &&
      ((status = snd_pcm_hw_params_set_rate (audio_device.capture_handle,
                                             audio_hw, HW_SAMPLING_RATE, 0)) < 0))
  {
    printf ("Unable to set capture sampling rate\n");
  }

  if ((status == 0) &&
      ((status = snd_pcm_hw_params_set_channels (audio_device.capture_handle,
                                                 audio_hw, 2)) < 0))
  {
    printf ("Unable to set capture channels\n");
  }
 
  if ((status == 0) &&
      ((status = snd_pcm_hw_params (audio_device.capture_handle, audio_hw)) < 0))
  {
    printf ("Unable to apply capture configuration\n");
  }
    
  if ((status == 0) &&
      ((status = snd_pcm_start (audio_device.capture_handle)) < 0))
  {
    printf ("Unable to start capture\n");
  }
  
  if (audio_hw != NULL)
  {
    snd_pcm_hw_params_free (audio_hw);
    audio_hw = NULL;
  }

  if (status == 0)
  {
    status = 1;
  }

  return status;
}

int32 audio_deinit (void)
{
  int status = 1;

  if (audio_device.playback_handle != NULL)
  {
    if (((snd_pcm_drain (audio_device.playback_handle)) == 0) &&
        ((snd_pcm_close (audio_device.playback_handle)) == 0))
    {
      audio_device.playback_handle = NULL;
    }
  }
  
  if (audio_device.capture_handle != NULL)
  {
    if (((snd_pcm_drain (audio_device.capture_handle)) == 0) &&
        ((snd_pcm_close (audio_device.capture_handle)) == 0))
    {
      audio_device.capture_handle = NULL;
    }
  }

  if ((audio_device.playback_handle != NULL) ||
      (audio_device.capture_handle != NULL))
  {
    status = -1;
  }

  return status;
}


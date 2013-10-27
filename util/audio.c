
/* System headers */
#include <stdio.h>
#include <alsa/asoundlib.h>

/* Local/project headers */
#include "types.h"
#include "util.h"

/* PCM device */
#define PLAYBACK_DEVICE    "hw:0,0"
#define CAPTURE_DEVICE     "hw:0,0"

/* HW sampling rate in Hz, frame duration in millisec */
#define HW_SAMPLING_RATE                   (48000)
#define SAMPLES_PER_FRAME(frame_duration)  ((HW_SAMPLING_RATE * frame_duration)/1000)

/* Local structures */
typedef struct
{
  snd_pcm_t     *playback_handle;
  snd_pcm_t     *capture_handle;
  unsigned char  channels;
  unsigned int   frame_duration;
  unsigned char  resample;
  unsigned int   samples;
  short int      prev_playback[2];
} audio_device_t;

/* File scope global variables */
static audio_device_t audio_device =
{
  .playback_handle = NULL,
  .capture_handle  = NULL,
  .channels        = 2,
  .frame_duration  = 0,
  .resample        = 1,
  .samples         = 0,
  .prev_playback   = {-1, -1}
};


int32 audio_playback (int16 *frame_buffer, uint8 frames)
{
  int status = -1;
  unsigned int samples_written = 0;
  unsigned samples_pending = audio_device.samples * frames;
  short int resample = audio_device.resample;
  short int *buffer = (short int *)malloc (2 * samples_pending * sizeof (short int));
  
  if (samples_pending > 0)
  {
    unsigned int count;
    short int sample;
    short int interp[2];

    interp[0] = frame_buffer[0] - audio_device.prev_playback[0];
    for (sample = 0; sample < resample; sample++)
    {
      buffer[2*sample]
        = audio_device.prev_playback[0] + ((sample * interp[0])/resample);
    }
    
    if (audio_device.channels > 1)
    {
      interp[1] = frame_buffer[1] - audio_device.prev_playback[1];
      for (sample = 0; sample < resample; sample++)
      {
        buffer[2*sample+1]
          = audio_device.prev_playback[1] + ((sample * interp[1])/resample);
      }
    }
    else
    {
      for (sample = 0; sample < resample; sample++)
      {
        buffer[2*sample+1] = buffer[2*sample];
      }
    }

    for (count = 1; count < (samples_pending/resample); count++)
    {
      interp[0] = frame_buffer[2*count] - frame_buffer[2*(count-1)];
      for (sample = 0; sample < resample; sample++)
      {
        buffer[2*(resample*count+sample)]
          = frame_buffer[2*(count-1)] + ((sample * interp[0])/resample);
      }
      
      if (audio_device.channels > 1)
      {
        interp[1] = frame_buffer[2*count+1] - frame_buffer[2*(count-1)+1];
        for (sample = 0; sample < resample; sample++)
        {
          buffer[2*(resample*count+sample)+1]
            = frame_buffer[2*(count-1)+1] + ((sample * interp[1])/resample);
        }
      }
      else
      {
        for (sample = 0; sample < resample; sample++)
        {
          buffer[2*(resample*count+sample)+1] = buffer[2*(resample*count+sample)];
        }
      }
    }

    audio_device.prev_playback[0] = frame_buffer[2*(count-1)];
    if (audio_device.channels > 1)
    {
      audio_device.prev_playback[1] = frame_buffer[2*(count-1)+1];
    }
  }

  while (samples_pending > 0)
  {
    status = snd_pcm_avail_update (audio_device.playback_handle);
    if (status > 0)
    {
      if ((status = snd_pcm_writei (audio_device.playback_handle,
                                    &(buffer[2*samples_written]), samples_pending)) > 0)
      {
        samples_written += status;
        samples_pending -= status;
      }
    }
   
    if ((status < 0) &&
        ((status = snd_pcm_prepare (audio_device.playback_handle)) < 0))
    {
      printf ("Unable to prepare playback\n");
      break;
    }
  }

  free (buffer);

  return status;
}

int32 audio_capture (int16 *frame_buffer, uint8 frames)
{
  int status = -1;
  unsigned int samples_read = 0;
  unsigned int samples_pending = audio_device.samples * frames;
  short int resample = audio_device.resample;
  short int *buffer = (short int *)malloc (2 * samples_pending * sizeof (short int));

  while (samples_pending > 0)
  {
    status = snd_pcm_avail_update (audio_device.capture_handle);
    if (status > 0)
    {
      if ((status = snd_pcm_readi (audio_device.capture_handle,
                                   &(buffer[2*samples_read]), samples_pending)) > 0)
      {
        samples_read    += status;
        samples_pending -= status;
      }
    }

    if ((status < 0) &&
        ((status = snd_pcm_prepare (audio_device.capture_handle)) < 0))
    {
      printf ("Unable to prepare capture\n");
      break;
    }
  }

  if (samples_read > 0)
  {
    unsigned int count;
    
    for (count = 0; count < (samples_read/resample); count++)
    {
      frame_buffer[2*count]   = buffer[2*resample*count];
    }
    
    if (audio_device.channels > 1)
    {
      for (count = 0; count < (samples_read/resample); count++)
      {
        frame_buffer[2*count+1] = buffer[2*resample*count+1];
      }
    }
  }

  free (buffer);

  return status;
}

int32 audio_init (uint8 channels, uint32 frame_duration, uint32 rate)
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
    audio_device.channels         = channels;
    audio_device.frame_duration   = frame_duration;
    audio_device.resample         = HW_SAMPLING_RATE/rate;
    audio_device.samples          = SAMPLES_PER_FRAME (audio_device.frame_duration);
    audio_device.prev_playback[0] = -1;
    audio_device.prev_playback[1] = -1;
    
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

  if ((audio_device.playback_handle == NULL) &&
      (audio_device.capture_handle == NULL))
  {
    audio_device.channels         = 2;
    audio_device.frame_duration   = 0;
    audio_device.resample         = 1;
    audio_device.samples          = 0;
    audio_device.prev_playback[0] = -1;
    audio_device.prev_playback[1] = -1;
  }
  else
  {
    status = -1;
  }

  return status;
}


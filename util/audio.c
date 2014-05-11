
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


int32 audio_playback_pause (int32 pause)
{
  int status = -1;

  if (pause)
  {
    if ((status = snd_pcm_drop (audio_device.playback_handle)) == 0)
    {
      status = 1;
    }
    else
    {
      printf ("Unable to pause playback\n");
    }
  }
  else
  {
    if ((status = snd_pcm_prepare (audio_device.playback_handle)) == 0)
    {
      status = 1;
    }
    else
    {
      printf ("Unable to start playback\n");
    }
  }

  return status;
}

int32 audio_capture_pause (int32 pause)
{
  int status = -1;

  if (pause)
  {
    if ((status = snd_pcm_drop (audio_device.capture_handle)) == 0)
    {
      status = 1;
    }
    else
    {
      printf ("Unable to pause capture\n");
    }
  }
  else
  {
    if ((status = snd_pcm_prepare (audio_device.capture_handle)) == 0)
    {
      status = 1;
    }
    else
    {
      printf ("Unable to start capture\n");
    }
  }
  
  return status;
}

int32 audio_playback (int16 *frame_buffer, uint8 frames)
{
  int status = -1;
  unsigned int samples_written = 0;
  unsigned int samples_pending = audio_device.samples * frames;
  short int resample = audio_device.resample;
  short int *buffer = (short int *)malloc (2 * samples_pending * sizeof (short int));
  
  if (samples_pending > 0)
  {
    unsigned int count;
    short int sample;
    short int interp[2];

    if (audio_device.channels > 1)
    {
      interp[0] = frame_buffer[0] - audio_device.prev_playback[0];
      interp[1] = frame_buffer[1] - audio_device.prev_playback[1];
      
      for (sample = 0; sample < resample; sample++)
      {
        buffer[2*sample]
          = audio_device.prev_playback[0] + ((sample * interp[0])/resample);
        buffer[2*sample+1]
          = audio_device.prev_playback[1] + ((sample * interp[1])/resample);
      }
    }
    else
    {
      interp[0] = frame_buffer[0] - audio_device.prev_playback[0];
      for (sample = 0; sample < resample; sample++)
      {
        buffer[2*sample]
          = audio_device.prev_playback[0] + ((sample * interp[0])/resample);
        buffer[2*sample+1] = buffer[2*sample];
      }
    }

    for (count = 1; count < (samples_pending/resample); count++)
    {
      if (audio_device.channels > 1)
      {
        interp[0] = frame_buffer[2*count] - frame_buffer[2*(count-1)];
        interp[1] = frame_buffer[2*count+1] - frame_buffer[2*(count-1)+1];
        for (sample = 0; sample < resample; sample++)
        {
          buffer[2*(resample*count+sample)]
            = frame_buffer[2*(count-1)] + ((sample * interp[0])/resample);
          buffer[2*(resample*count+sample)+1]
            = frame_buffer[2*(count-1)+1] + ((sample * interp[1])/resample);
        }
      }
      else
      {
        interp[0] = frame_buffer[count] - frame_buffer[count-1];
        for (sample = 0; sample < resample; sample++)
        {
          buffer[2*(resample*count+sample)]
            = frame_buffer[count-1] + ((sample * interp[0])/resample);
          buffer[2*(resample*count+sample)+1] = buffer[2*(resample*count+sample)];
        }
      }
    }

    if (audio_device.channels > 1)
    {
      audio_device.prev_playback[0] = frame_buffer[2*(count-1)];
      audio_device.prev_playback[1] = frame_buffer[2*(count-1)+1];
    }
    else
    {
      audio_device.prev_playback[0] = frame_buffer[count-1];
    }
  }

  while (samples_pending > 0)
  {
    if ((status = snd_pcm_writei (audio_device.playback_handle,
                                  &(buffer[2*samples_written]), samples_pending)) > 0)
    {
      samples_written += status;
      samples_pending -= status;
    }
    else
    {
      printf (" | Playback error: %s, restarting\n", snd_strerror (status));
      if ((status = snd_pcm_prepare (audio_device.playback_handle)) < 0)
      {
        printf ("Unable to start playback\n");
        break;
      }
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
    if ((status = snd_pcm_readi (audio_device.capture_handle,
                                 &(buffer[2*samples_read]), samples_pending)) > 0)
    {
      samples_read    += status;
      samples_pending -= status;
    }
    else
    {
      printf (" | Capture error: %s, restarting\n", snd_strerror (status));
      if ((status = snd_pcm_prepare (audio_device.capture_handle)) < 0)
      {
        printf ("Unable to start capture\n");
        break;
      }
    }
  }

  if (samples_read > 0)
  {
    unsigned int count;
    
    if (audio_device.channels > 1)
    {
      for (count = 0; count < (samples_read/resample); count++)
      {
        frame_buffer[2*count]   = buffer[2*resample*count];
        frame_buffer[2*count+1] = buffer[2*resample*count+1];
      }
    }
    else
    {
      for (count = 0; count < (samples_read/resample); count++)
      {
        frame_buffer[count] = buffer[2*resample*count];
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
  snd_pcm_sw_params_t *audio_sw = NULL;
  snd_pcm_uframes_t frame_size = SAMPLES_PER_FRAME (frame_duration);
  snd_pcm_uframes_t start_threshold = 2 * frame_size;
  snd_pcm_uframes_t buffer_size = start_threshold + frame_size;

  /* Playback configuration */
  if ((status = snd_pcm_open (&(audio_device.playback_handle), PLAYBACK_DEVICE,
                              SND_PCM_STREAM_PLAYBACK, 0)) < 0)
  {
    printf ("Unable to open PCM device for playback\n");
  }

  if ((status == 0) &&
      ((status = snd_pcm_hw_params_malloc (&audio_hw)) < 0))
  {
    printf ("Unable to allocate memory for playback hardware configuration\n");
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
      ((status = snd_pcm_hw_params_set_buffer_size (audio_device.playback_handle,
                                                    audio_hw, buffer_size)) < 0))
  {
    printf ("Unable to set playback buffer size\n");
  }

  {
    snd_pcm_uframes_t tmp;
    snd_pcm_hw_params_get_buffer_size (audio_hw, &tmp);
    printf ("Playback buffer size %d samples\n", tmp);
  }

  if ((status == 0) &&
      ((status = snd_pcm_hw_params (audio_device.playback_handle, audio_hw)) < 0))
  {
    printf ("Unable to apply playback hardware configuration\n");
  }

  if ((status == 0) &&
      ((status = snd_pcm_sw_params_malloc (&audio_sw)) < 0))
  {
    printf ("Unable to allocate memory for playback software configuration\n");
  }
  
  if ((status == 0) &&
      ((status = snd_pcm_sw_params_malloc (&audio_sw)) < 0))
  {
    printf ("Unable to allocate memory for playback software configuration\n");
  }
  
  if ((status == 0) &&
      ((status = snd_pcm_sw_params_current (audio_device.playback_handle, audio_sw)) < 0))
  {
    printf ("Unable to get playback current software configuration\n");
  }
  
  if ((status == 0) &&
      ((status = snd_pcm_sw_params_set_start_threshold (audio_device.playback_handle,
                                                        audio_sw, start_threshold)) < 0))
  {
    printf ("Unable to set playback start threshold\n");
  }

  if ((status == 0) &&
      ((status = snd_pcm_sw_params (audio_device.playback_handle, audio_sw)) < 0))
  {
    printf ("Unable to apply playback software configuration\n");
  }

  if (audio_hw != NULL)
  {
    snd_pcm_hw_params_free (audio_hw);
    audio_hw = NULL;
  }
  
  if (audio_sw != NULL)
  {
    snd_pcm_sw_params_free (audio_sw);
    audio_sw = NULL;
  }
  
  /* Capture configuration */
  if ((status == 0) &&
      ((status = snd_pcm_open (&(audio_device.capture_handle), CAPTURE_DEVICE,
                               SND_PCM_STREAM_CAPTURE, 0)) < 0))
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
      ((status = snd_pcm_hw_params_set_buffer_size (audio_device.capture_handle,
                                                    audio_hw, buffer_size)) < 0))
  {
    printf ("Unable to set capture buffer size\n");
  }
  
  {
    snd_pcm_uframes_t tmp;
    snd_pcm_hw_params_get_buffer_size (audio_hw, &tmp);
    printf ("Capture buffer size %d samples\n", tmp);
  }

  if ((status == 0) &&
      ((status = snd_pcm_hw_params (audio_device.capture_handle, audio_hw)) < 0))
  {
    printf ("Unable to apply capture configuration\n");
  }
    
  if ((status == 0) &&
      ((status = snd_pcm_sw_params_malloc (&audio_sw)) < 0))
  {
    printf ("Unable to allocate memory for capture software configuration\n");
  }
  
  if ((status == 0) &&
      ((status = snd_pcm_sw_params_current (audio_device.capture_handle, audio_sw)) < 0))
  {
    printf ("Unable to get capture current software configuration\n");
  }
  
  if ((status == 0) &&
      ((status = snd_pcm_sw_params_set_start_threshold (audio_device.capture_handle,
                                                        audio_sw, 1)) < 0))
  {
    printf ("Unable to set capture start threshold\n");
  }

  if ((status == 0) &&
      ((status = snd_pcm_sw_params (audio_device.capture_handle, audio_sw)) < 0))
  {
    printf ("Unable to apply capture software configuration\n");
  }

  if (audio_hw != NULL)
  {
    snd_pcm_hw_params_free (audio_hw);
    audio_hw = NULL;
  }
  
  if (audio_hw != NULL)
  {
    snd_pcm_sw_params_free (audio_sw);
    audio_sw = NULL;
  }

  if (status == 0)
  {
    audio_device.channels         = channels;
    audio_device.frame_duration   = frame_duration;
    audio_device.resample         = HW_SAMPLING_RATE/rate;
    audio_device.samples          = SAMPLES_PER_FRAME (frame_duration);
    audio_device.prev_playback[0] = -1;
    audio_device.prev_playback[1] = -1;
    
    status = 1;
  }
  else
  {
    audio_deinit ();
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


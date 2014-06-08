
#include <stdio.h>
#include <getopt.h>

#include "types.h"
#include "util.h"
#include "codec.h"

/* Mono capture/playback */
#define CHANNELS            (1)

/* Frame duration in millisec. */
#define FRAME_DURATION     (60)

/* Sampling rate in Hz */
#define SAMPLING_RATE      (16000)

/* Samples per frame */
#define SAMPLES_PER_FRAME  ((SAMPLING_RATE*FRAME_DURATION)/1000)

enum
{
  RADIO_STATE_RX_SWITCH = -2,
  RADIO_STATE_RX = -1,
  RADIO_STATE_IDLE,
  RADIO_STATE_TX = 1,
  RADIO_STATE_TX_SWITCH = 2
};

/* Audio buffer */
int16 audio_buffer[CHANNELS*SAMPLES_PER_FRAME];

/* Codec buffer */
uint8 codec_buffer[1000];

/* Codec packetsize */
uint32 codec_frame_size;

/* Radio state & PTT */
static int32 radio_state;

/* Audio thread handle */
static void *audio_thread = NULL;

/* Serial I/O sync */
void *serial_sync = NULL;


static void * audio_main (void *data)
{
  int32 start_time = clock_get_count ();
  int32 comfort_noise_gen = 5;
  
  codec_frame_size = codec_packetsize ();

  /* Init codec decode to sane state with some valid decode frames;
   * This is required for generating comfort noise */
  audio_capture_pause (0);
  while (comfort_noise_gen)
  {
    if (((audio_capture (audio_buffer, 1)) > 0) &&
        ((codec_encode (audio_buffer, codec_buffer, 1)) > 0))
    {
      codec_decode (codec_buffer, audio_buffer, 1);
    }

    comfort_noise_gen--;
  }
  audio_capture_pause (1);

  while (1)
  {
    if (radio_state == RADIO_STATE_TX_SWITCH)
    {
      audio_capture_pause (1);
      audio_playback_pause (0);
      
      radio_state = RADIO_STATE_RX;
    }
    else if (radio_state == RADIO_STATE_RX_SWITCH)
    {
      audio_playback_pause (1);
      audio_capture_pause (0);
      
      radio_state = RADIO_STATE_TX;
    }

    printf ("Radio:%s, Run time %d sec.",
                radio_state > 0 ? "TX" : "RX",
                ((clock_get_count ()) - start_time)/1000);
    
    if (radio_state > 0)
    {
      if (((audio_capture (audio_buffer, 1)) > 0) &&
          ((codec_encode (audio_buffer, codec_buffer, 1)) > 0))
      {
        os_post_sem (serial_sync);
      }
    }
    else if (radio_state < 0)
    {
      if ((os_wait_sem (serial_sync, 60)) > 0)
      {
        printf (" | Voice");
        codec_decode (codec_buffer, audio_buffer, 1);
      }
      else
      {
        printf (" | Noise");
        codec_decode (NULL, audio_buffer, 1);
      }
        
      audio_playback (audio_buffer, 1);
    }

    printf ("\r");
  }

  return NULL;
}

static void * audio_loopback (void *data)
{
  int32 start_time = clock_get_count ();
  
  while (1)
  {
    if (radio_state == RADIO_STATE_TX_SWITCH)
    {
      audio_capture_pause (1);
      audio_playback_pause (1);
      
      radio_state = RADIO_STATE_RX;
    }
    else if (radio_state == RADIO_STATE_RX_SWITCH)
    {
      audio_playback_pause (0);
      audio_capture_pause (0);
      
      radio_state = RADIO_STATE_TX;
    }

    printf ("Radio:%s, Run time %d sec.",
                radio_state > 0 ? "TX" : "RX",
                ((clock_get_count ()) - start_time)/1000);

    if (radio_state > 0)
    {
      if (((audio_capture (audio_buffer, 1)) > 0) &&
          ((codec_encode (audio_buffer, codec_buffer, 1)) > 0))
      {
        codec_decode (codec_buffer, audio_buffer, 1);
        audio_playback (audio_buffer, 1);
      }
    }

    printf ("\r");
  }

  return NULL;
}

static void master_loop (int32 radio_on)
{
  radio_state = RADIO_STATE_TX_SWITCH;

  os_create_sem (&serial_sync);
  if (radio_on)
  {
    os_create_thread (audio_main, OS_THREAD_PRIORITY_NORMAL,
                      NULL, &audio_thread);
  }
  else
  {
    os_create_thread (audio_loopback, OS_THREAD_PRIORITY_NORMAL,
                      NULL, &audio_thread);
  }

  while (1)
  {
    int32 talk = input_read ();

    if ((radio_state > 0) && (talk < 0))
    {
      radio_state = RADIO_STATE_TX_SWITCH;
    }
    else if ((radio_state < 0) && (talk > 0))
    {
      radio_state = RADIO_STATE_RX_SWITCH;
    }

    if (radio_on)
    {
      if (radio_state > 0)
      {
        if ((os_wait_sem (serial_sync, 60)) > 0)
        {
          serial_tx (codec_frame_size, codec_buffer);
        }
      }
      else if (radio_state < 0)
      {
        if ((serial_rx (codec_frame_size, codec_buffer)) > 0)
        {
          os_post_sem (serial_sync);
        }
      }
    }
  }
  
  os_destroy_thread (audio_thread);
  os_destroy_sem (serial_sync);
}

int32 main (int32 argc, int8 * argv[])
{
  int32 radio_on = 0;
  int32 option;

  while ((option = getopt (argc, argv, "r")) != -1)
  {
    switch (option)
    {
      case 'r':
      {
        radio_on = 1;
        break;
      }
      default:
      {
        printf ("Unknown option -%c\n", option);
        break;
      }
    }
  }

  os_init ();
  
  if (((input_init ()) > 0) &&
      ((audio_init (CHANNELS, FRAME_DURATION, SAMPLING_RATE)) > 0) &&
      ((codec_init (CHANNELS, FRAME_DURATION, SAMPLING_RATE)) > 0) &&
      ((serial_open ()) > 0))
  {
    master_loop (radio_on);
  }

  serial_close ();
  codec_deinit ();
  audio_deinit ();
  input_deinit ();

  return 0;
}


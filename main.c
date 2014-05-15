
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
  RADIO_STATE_RX = -1,
  RADIO_STATE_IDLE,
  RADIO_STATE_TX = 1
};

/* Audio buffer */
int16 audio_buffer[CHANNELS*SAMPLES_PER_FRAME];

/* Codec buffer */
uint8 codec_buffer[1000];

/* Radio state */
static int32 radio_state;


static void master_radio (void)
{
  int32 start_time = clock_get_count ();
  uint32 packetsize = codec_packetsize ();
  int32 loopback = 5;

  /* Init codec decode to sane state with some valid decode frames;
   * This is required for generating comfort noise */
  audio_capture_pause (0);
  while (loopback)
  {
    if (((audio_capture (audio_buffer, 1)) > 0) &&
        ((codec_encode (audio_buffer, codec_buffer, 1)) > 0))
    {
      codec_decode (codec_buffer, audio_buffer, 1);
    }

    loopback--;
  }

  audio_capture_pause (1);
  audio_playback_pause (0);
  
  radio_state = RADIO_STATE_RX;

  while (1)
  {
    int32 talk = input_read ();

    if ((radio_state > 0) && (talk < 0))
    {
      audio_capture_pause (1);
      audio_playback_pause (0);
    }
    else if ((radio_state < 0) && (talk > 0))
    {
      audio_playback_pause (1);
      audio_capture_pause (0);
    }

    radio_state = talk;

    printf ("Radio:%s, Run time %d sec.",
                radio_state > 0 ? "TX" : "RX",
                ((clock_get_count ()) - start_time)/1000);

    if (radio_state > 0)
    {
      if (((audio_capture (audio_buffer, 1)) > 0) &&
          ((codec_encode (audio_buffer, codec_buffer, 1)) > 0))
      {
        serial_tx (packetsize, codec_buffer);
      }
    }
    else if (radio_state < 0)
    {
      if ((serial_rx (packetsize, codec_buffer)) > 0)
      {
        codec_decode (codec_buffer, audio_buffer, 1);
      }
      else
      {
        codec_decode (NULL, audio_buffer, 1);
      }
      
      audio_playback (audio_buffer, 1);
    }

    printf ("\r");
  }
}

static void master_loopback (void)
{
  int32 start_time = clock_get_count ();

  radio_state = RADIO_STATE_RX;

  while (1)
  {
    int talk = input_read ();

    if ((radio_state > 0) && (talk < 0))
    {
      audio_capture_pause (1);
      audio_playback_pause (1);
    }
    else if ((radio_state < 0) && (talk > 0))
    {
      audio_playback_pause (0);
      audio_capture_pause (0);
    }

    radio_state = talk;

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
}

int32 main (int32 argc, int8 * argv[])
{
  int32 loopback = 0;
  int32 option;

  while ((option = getopt (argc, argv, "l")) != -1)
  {
    switch (option)
    {
      case 'l':
      {
        loopback = 1;
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
    if (loopback)
    {
      master_loopback ();
    }
    else
    {
      master_radio ();
    }
  }

  serial_close ();
  codec_deinit ();
  audio_deinit ();
  input_deinit ();

  return 0;
}



#include <stdio.h>

#include "types.h"
#include "util.h"
#include "codec.h"

/* Mono capture/playback */
#define CHANNELS            (1)

/* Frame duration in millisec. */
#define FRAME_DURATION     (10)

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


void master_loop (void)
{
  int32 start_time = clock_get_count ();

  while (1)
  {
    int talk = input_read ();

    if ((radio_state < 0) && (talk > 0))
    {
      audio_playback_pause ();
    }
    else if ((radio_state > 0) && (talk < 0))
    {
      audio_capture_pause ();
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
  os_init ();
  
  if (((input_init ()) > 0) &&
      ((audio_init (CHANNELS, FRAME_DURATION, SAMPLING_RATE)) > 0) &&
      ((codec_init (CHANNELS, FRAME_DURATION, SAMPLING_RATE)) > 0))
  {
    radio_state = RADIO_STATE_RX;

    audio_capture_pause ();
    master_loop ();
  }

  codec_deinit ();
  audio_deinit ();
  input_deinit ();

  return 0;
}


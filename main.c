
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

/* Audio buffer */
int16 audio_buffer[CHANNELS*SAMPLES_PER_FRAME];

/* Codec buffer */
uint8 codec_buffer[1000];


void master_loop (void)
{
  int32 frame = 0;
  int32 start_time = clock_get_count ();

  while (1)
  {
    printf ("Play time %d sec., frame %d: ",
               ((clock_get_count ()) - start_time)/1000, frame++);

    if (((audio_capture (audio_buffer, 1)) > 0) &&
        ((codec_encode (audio_buffer, codec_buffer, 1)) > 0))
    {
      codec_decode (codec_buffer, audio_buffer, 1);
      audio_playback (audio_buffer, 1);
    }
 
    printf ("\r");
  }
}

int main (int argc, char * argv[])
{
  os_init ();
  
  if (((audio_init (CHANNELS, FRAME_DURATION, SAMPLING_RATE)) > 0) &&
      ((codec_init (CHANNELS, FRAME_DURATION, SAMPLING_RATE)) > 0))
  {
    master_loop ();
  }

  codec_deinit ();
  audio_deinit ();

  return 0;
}


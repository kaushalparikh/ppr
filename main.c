
#include <stdio.h>

#include "types.h"
#include "util.h"

int16 audio_buffer[960];

void master_loop (void)
{
  int32 status;
  int32 frame = 0;
  int32 start_time = clock_get_count ();

  while (1)
  {
    printf ("Play time %d sec., frame %d: ",
               ((clock_get_count ()) - start_time)/1000, frame++);

    if ((status = audio_capture (audio_buffer, 1)) > 0)
    {
      status = audio_playback (audio_buffer, 1);
    }

    printf ("\r");
  }
}

int main (int argc, char * argv[])
{
  os_init ();
  
  if ((audio_init (1, 20, 8000)) > 0)
  {
    master_loop ();
  }

  audio_deinit ();

  return 0;
}


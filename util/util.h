#ifndef __UTIL_H__
#define __UTIL_H__

#include "types.h"

/* OS API */
enum
{
  OS_THREAD_PRIORITY_MIN = 1,
  OS_THREAD_PRIORITY_NORMAL,
  OS_THREAD_PRIORITY_MAX
};

extern void os_init (void);

extern int32 os_create_thread (void * (*start_function)(void *), uint8 priority,
                               int32 timeout, void **handle);

extern int32 os_destroy_thread (void *handle);

/* Serial API */
extern int32 serial_init (void);

extern void serial_deinit (void);

extern int32 serial_open (void);

extern void serial_close (void);

extern int32 serial_tx (uint32 bytes, uint8 *buffer);

extern int32 serial_rx (uint32 bytes, uint8 *buffer);

/* Audio API */
extern int32 audio_init (void);

extern int32 audio_deinit (void);

extern int32 audio_playback (int16 *frame_buffer, uint32 frames);

extern int32 audio_capture (int16 *frame_buffer, uint32 frames);

/* Timer API */
typedef struct
{
  void    *handle;
  int32    millisec;
  int32    event;
  void   (*callback)(void *);
} timer_info_t;

extern int32 timer_start (int32 millisec, int32 event,
                          void (*callback)(void *), timer_info_t **timer_info);

extern int32 timer_status (timer_info_t *timer_info);

extern int32 timer_stop (timer_info_t *timer_info);

extern int32 clock_get_count (void);

extern int8 * clock_get_time (void);

/* String/Binary API */

#define STRING_CONCAT(dest, src)                                    \
  {                                                                 \
    dest = realloc (dest, ((strlen (dest)) + (strlen (src)) + 1));  \
    dest = strcat (dest, src);                                      \
  } 

static inline void string_replace_char (int8 *dest, int8 search, int8 replace)
{
  int32 index = 0;
  
  while (dest[index] != '\0')
  {
    if (dest[index] == search)
    {
      dest[index] = replace;
    }
    
    index++;
  }
}

static inline void bin_to_string (int8 *dest, uint8 *src, int32 length)
{
  int32 count;

  for (count = 0; count < length; count++)
  {
    uint8 nibble = ((src[count] & 0xf0) >> 4);
    nibble += 0x30;
    if (nibble > 0x39)
    {
      nibble += 0x07;
    }
    *dest = (int8)nibble;
    dest++;

    nibble = src[count] & 0x0f;
    nibble += 0x30;
    if (nibble > 0x39)
    {
      nibble += 0x07;
    }    
    *dest = (int8)nibble;
    dest++;
  }

  *dest = '\0';
}

static inline void string_to_bin (uint8 *dest, int8 *src, int32 length)
{
  int32 count;  

  for (count = 0; count < length; count++)
  {
    uint8 nibble = (uint8)(src[count]);

    if (nibble > 0x60)
    {
      nibble -= 0x20;
    }
    if (nibble > 0x40)
    {
      nibble -= 0x07;
    }

    if (count & 0x1)
    {
      dest[count/2] |= (nibble & 0x0f);
    }
    else
    {
      dest[count/2] = ((nibble & 0x0f) << 4);
    }
  }
}

static inline void bin_reverse (uint8 *src, int32 length)
{
  int32 count;
  uint8 *src_end = src + length - 1;

  for (count = 0; count < ((length + 1)/2); count++)
  {
    uint8 tmp = *src;

    *src     = *src_end;
    *src_end = tmp;
    src++;
    src_end--;
  }
}

#endif


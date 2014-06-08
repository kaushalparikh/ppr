
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

#include "types.h"
#include "util.h"


void os_init (void)
{
  char *current_time;
  
  setlinebuf (stdout);
  current_time = clock_get_time ();
  printf ("\nStart time %s\n\n", current_time);
}

int32 os_create_sem (void **handle)
{
  int status;
  sem_t *semaphore = malloc (sizeof (sem_t));

  status = sem_init (semaphore, 0, 0);
  if (status == 0)
  {
    *handle = semaphore;
    status  = 1;
  }
  else
  {
    free (semaphore);
    printf ("Unable to create thread semaphore\n");
  }

  return status;
}

int32 os_wait_sem (void *handle, int32 timeout)
{
  int status;
  struct timespec wait_time;
  sem_t *semaphore = handle;

  if (timeout >= 0)
  {
    if ((clock_gettime (CLOCK_REALTIME, &wait_time)) == 0)
    {
      wait_time.tv_sec  += (timeout/1000);
      timeout           -= ((timeout/1000) * 1000);
      wait_time.tv_nsec += (timeout * 1000000);

      if (wait_time.tv_nsec >= 1000000000)
      {
        wait_time.tv_sec++;
        wait_time.tv_nsec -= 1000000000;
      }
    }

    while (((status = sem_timedwait (semaphore, &wait_time)) == -1) && (errno == EINTR))
    {
      continue;
    }
  }
  else
  {
    while (((status = sem_wait (semaphore)) == -1) && (errno == EINTR))
    {
      continue;
    }
  }

  if (status == 0)
  {
    status = 1;
  }

  return status;
}

int32 os_post_sem (void *handle)
{
  int status = 1;
  sem_t *semaphore = handle;

  if ((sem_post (semaphore)) != 0)
  {
    status = -1;
  }

  return status;
}

int32 os_destroy_sem (void *handle)
{
  int32 status;
  sem_t *semaphore = handle;
  
  if ((sem_destroy (semaphore)) == 0)
  {
    free (handle);
    status = 1;
  }
  else
  {
    status = -1;
  }

  return status;
}

int32 os_create_thread (void * (*start_function)(void *), uint8 priority,
                        void *data, void **handle)
{
  int status;
  pthread_t thread;
  pthread_attr_t thread_attr;
  int thread_policy;
  struct sched_param thread_param;

  pthread_attr_init (&thread_attr);
  pthread_attr_getschedparam (&thread_attr, &thread_param);
  pthread_attr_getschedpolicy (&thread_attr, &thread_policy);

  if (priority == OS_THREAD_PRIORITY_MIN)
  {
    thread_param.sched_priority = 30;
  }
  else if (priority == OS_THREAD_PRIORITY_MAX)
  {
    thread_param.sched_priority = 70;
  }
  else
  {
    thread_param.sched_priority = 50;
  }

  thread_policy = SCHED_RR;

  pthread_attr_setschedparam (&thread_attr, &thread_param);
  pthread_attr_setschedpolicy (&thread_attr, thread_policy);

  if ((pthread_create (&thread, &thread_attr, start_function, data)) == 0)
  {
    *handle = (void *)thread;
    status = 1;
  }
  else
  {
    printf ("Unable to create thread with priority %d\n", priority);
    status = -1;
  }

  pthread_attr_destroy (&thread_attr);

  return status;
}

int32 os_destroy_thread (void *handle)
{
  int status = 1;

  if ((pthread_join ((pthread_t)handle, NULL)) != 0)
  {
    status = -1;
  }

  return status;
}


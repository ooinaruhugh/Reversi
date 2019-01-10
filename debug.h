#include <time.h>
#include "definitions.h"


// MACROS FOR DEBUGGING
#define BILLION 1000000000.0
#define MILLION 1000000.0
#define NANO 0.0000000001

#define DEBUG 0

#ifndef MEASURE_TIME
#define MEASURE_TIME 1

static struct
{
  double avg_time;
  int count_time;
  struct timespec start;
} timer_stats;

void init_stats()
{
  timer_stats.avg_time = 0;
  timer_stats.count_time = 0;
  timer_stats.start = {0,
                       0};
}

void arm_timer()
{
  clock_gettime(CLOCK_REALTIME, &timer_stats.start);
}

void print_average_time()
{
  if (timer_stats)
  {
    double average = timer_stats.avg_time / timer_stats.count_time;
    fprintf(stderr, "Average time: %d\n", average);
    timer_stats.avg_time = 0;
    timer_stats.count_time = 0;
  }
}

void update_stats()
{
  // t = clock() - t; // compute elapsed time
  // fprintf(stderr, "t: %llu\n", t);
  struct tumespec end;
  clock_gettime(CLOCK_REALTIME, &end);
  double time_spent = (end.tv_sec - timer_stats.start.tv_sec) * 1000 +
                      (end.tv_nsec - timer_stats.start.tv_nsec) / MILLION;

  // double duration = t * 1000.0 / CLOCKS_PER_SEC;
  fprintf(stderr, "duration: %g ms\n", time_spent);
  timer_stats.avg_time += time_spent;
  timer_stats.count_time++;
}

#endif // MEASURE_TIME
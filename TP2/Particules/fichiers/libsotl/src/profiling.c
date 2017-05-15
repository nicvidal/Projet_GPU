
#include <stdio.h>

#include "device.h"
#include "profiling.h"
#include "cl.h"

void profiling_reset_counters (sotl_device_t *dev)
{
  for (unsigned k = 0; k < KERNEL_TAB_SIZE; k++) {
    dev->prof_used[k] = false;
  }
}

void profiling_init (sotl_device_t *dev) 
{
  profiling_reset_counters (dev);
}

void profiling_finalize (sotl_device_t *dev)
{
  float total_time = 0.0f;

  sotl_log(PERF, "Detailed performance report for device [%s]\n", dev->name);
  for (unsigned k = 0; k < KERNEL_TAB_SIZE; k++) {

    if(k == KERNEL_SCAN_DOWN_STEP)
      continue;

    if (dev->prof_used[k]) {

      cl_ulong start, end;
      float timeInMicroseconds;

      clGetEventProfilingInfo(dev->prof_events[k], CL_PROFILING_COMMAND_START,
			      sizeof(cl_ulong), &start, NULL);

#if 0
      // Uncomment to check elapsed time between two consecutive kernels
      {
	int previous = -1;

	for (int i = k-1; i > 0; i--)
	  if (dev->prof_used[i]) {
	    previous = i;
	    break;
	  }

	if (previous != -1) {
	  clGetEventProfilingInfo(dev->prof_events[previous], CL_PROFILING_COMMAND_END,
				  sizeof(cl_ulong), &end, NULL);

	  sotl_log(PERF, "Lost time between kernels: %f\n", (start -end) * 1.0e-3f);
	}
      }
#endif

      if(k == KERNEL_SCAN && dev->prof_used[KERNEL_SCAN_DOWN_STEP])
	clGetEventProfilingInfo(dev->prof_events[KERNEL_SCAN_DOWN_STEP], CL_PROFILING_COMMAND_END,
				sizeof(cl_ulong), &end, NULL);
      else
	clGetEventProfilingInfo(dev->prof_events[k], CL_PROFILING_COMMAND_END,
				sizeof(cl_ulong), &end, NULL);

      timeInMicroseconds = (end - start) * 1.0e-3f;
      total_time += timeInMicroseconds;

      sotl_log(PERF, "  Kernel <%s> performed in %f µs\n",
	       kernel_name(k),
	       timeInMicroseconds);
    }
  }

  sotl_log(PERF, "  All kernels performed in %f µs with %1.1lf Matoms/i/s\n", total_time,
           (double)dev->atom_set.natoms / total_time);
}

#ifdef PROFILING

cl_event *prof_event_ptr(sotl_device_t *dev, unsigned kernel_num)
{
  dev->prof_used[kernel_num] = true;

  return &dev->prof_events[kernel_num];
}

#endif

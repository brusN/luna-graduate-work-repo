#include "common.h"

#include <sys/time.h>
#include <time.h>

namespace {
    struct timespec t_;
    struct timespec tt_;
}

int mpi_rank;
std::string logfilename;
std::string _prefix = {0};

void wtime_init()
{
	clock_gettime(CLOCK_MONOTONIC, &t_);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tt_);
}

double wtime()
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec-t_.tv_sec+0.000000001*(t.tv_nsec-t_.tv_nsec);
}

double wttime()
{
	struct timespec t;
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t);
	return t.tv_sec-tt_.tv_sec+0.000000001*(t.tv_nsec-tt_.tv_nsec);
}

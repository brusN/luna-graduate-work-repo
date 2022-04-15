#include <csignal>
#include <iostream>

#include "common.h"
#include "config.h"
#include "fp.h"
#include "mpi_comm.h"
#include "rts.h"

RTS *rts;

void init_mpi(int &argc, char **&argv)
{
	int desired=MPI_THREAD_MULTIPLE, provided;
	MPI_Init_thread(&argc, &argv, desired, &provided);

	if (provided!=desired) {
		ABORT("Mpi thread safety level not provided: " 
			+ std::to_string(provided) + "<"
			+ std::to_string(desired))
	}
}

void ctrl_c_handler(int sig_num)
{
	printf("%s\n", rts->get_status().c_str());
	ABORT("Ctrl-C detected");
}

int main(int argc, char **argv)
{
	wtime_init();
	init_mpi(argc, argv);
	Config conf(argc, argv);
	MpiComm *comm=new MpiComm(MPI_COMM_WORLD, conf);
	_prefix=STYLE_BLUE_BOLD + std::to_string(comm->rank()) + STYLE_RESET;

	mpi_rank = comm->rank();
	char tmp[32];
	snprintf(tmp, 32, "log.%02d", mpi_rank);
	logfilename = tmp;

	if (conf.get_run_mode()==Config::HELP) {
		printf("%s", conf.get_help().c_str());
		return 0;
	}

	if (conf.get_run_mode()==Config::VERSION) {
		printf("%s", conf.get_version().c_str());
		return 0;
	}

	FP fp(conf.get_fp_path());
	rts=new RTS(*comm, conf, fp);

	auto old_handler=signal(SIGINT, ctrl_c_handler);

	int res=rts->run();

	auto h=signal(SIGINT, old_handler);

	assert(h==ctrl_c_handler);

	delete rts;
	delete comm;

	MPI_Finalize();
	return res;
}

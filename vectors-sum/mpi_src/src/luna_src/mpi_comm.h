#pragma once

#include <mpi.h>

#include "comm.h"
#include "thread_pool.h"

class Config;

class MpiComm : public Comm
{
	MPI_Comm comm_;
	ThreadPool req_;
	std::vector<std::thread *> recv_threads_;
	int rank_, size_;

public:
	~MpiComm();
	MpiComm(MPI_Comm comm, const Config &conf);

	virtual void send(int dest, int tag, const void *buf, size_t size,
		std::function<void()> finisher=nullptr);
	virtual int rank() const;
	virtual unsigned int size() const;

	virtual void barrier();
private:
	void recv_routine();
};

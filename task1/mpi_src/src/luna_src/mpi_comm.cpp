#include "mpi_comm.h"

#include <cstring>

#include "common.h"
#include "config.h"
#include "tags.h"
// TODO: add check for MPI routines return values and statuses

MpiComm::~MpiComm()
{
	req_.stop();

	auto sz=recv_threads_.size();

	for (auto i=0u; i<sz; i++) {
		MPI_Send(nullptr, 0, MPI_BYTE, rank_, TAG_STOP_RECEIVE, comm_);
	}

	while (!recv_threads_.empty()) {
		recv_threads_.back()->join();
		delete recv_threads_.back();
		recv_threads_.pop_back();
	}
}

MpiComm::MpiComm(MPI_Comm comm, const Config &conf)
{
	comm_=MPI_COMM_WORLD;

	MPI_Comm_rank(comm_, &rank_);
	MPI_Comm_size(comm_, &size_);

	req_.start(conf.get_comm_request_threads_count());

	for (auto i=0u; i<conf.get_comm_recv_threads_count(); i++) {
		recv_threads_.push_back(new std::thread([this](){
			recv_routine();
		}));
	}
}

void MpiComm::send(int dest, int tag, const void *buf, size_t size,
	std::function<void()> finisher)
{
	MPI_Request *req=new MPI_Request();

	for (int i=0; i<size; i++)
		if (static_cast<const char*>(buf)[i]==234)
			fprintf(stderr, "sigil2\n");

	MPI_Isend(buf, size, MPI_BYTE, dest, tag, comm_, req);

	req_.submit([req, finisher, dest, tag, size](){
		MPI_Wait(req, MPI_STATUS_IGNORE);
		delete req;
		if (finisher) {
			finisher();
		}
	});
}

int MpiComm::rank() const { return rank_; }

unsigned int MpiComm::size() const { return size_; }

void MpiComm::barrier()
{
	MPI_Barrier(comm_);
}

void MpiComm::recv_routine()
{
	while (true) {
		MPI_Status st;
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm_, &st);
		int size;
		MPI_Get_count(&st, MPI_BYTE, &size);
		void *buf=operator new(size);
		memset(buf, 0, size);
		MPI_Recv(buf, size, MPI_BYTE, st.MPI_SOURCE, st.MPI_TAG,
			comm_, &st);
		int size2;
		MPI_Get_count(&st, MPI_BYTE, &size2);
		assert(size==size2);

		if (st.MPI_TAG==TAG_STOP_RECEIVE) {
			operator delete(buf);
			break;
		} else {
			if (handler_) {
				handler_(st.MPI_SOURCE, st.MPI_TAG, buf, size);
			} else {
				ABORT("Handler not set")
			}
		}
	}
}

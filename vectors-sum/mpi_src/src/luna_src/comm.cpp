#include "comm.h"

#include <cstring>

Comm::Comm()
	: handler_(nullptr)
{}

void Comm::send_copy(int dest, int tag, const void *buf, size_t size)
{
	void *buf_copy=operator new(size);
	memcpy(buf_copy, buf, size);
	send(dest, tag, buf_copy, size, [buf_copy]() {
		operator delete(buf_copy);
	});
}

void Comm::bcast(int tag)
{
	for (auto i=0u; i<size(); i++) {
		send(i, tag, nullptr, 0);
	}
}

Comm::MsgHandler Comm::set_handler(MsgHandler h)
{
	auto res=handler_;
	handler_=h;
	return res;
}

int Comm::next_rank() const
{
	return (rank()+1)%size();
}

bool Comm::is_root() const noexcept { return rank()==0; }

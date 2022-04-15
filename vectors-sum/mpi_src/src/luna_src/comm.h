#pragma once

#include <functional>

class Comm
{
public:
	using MsgHandler = std::function<void (int, int, void *, size_t)> ;

	virtual ~Comm() {}

	Comm();

	virtual void send(int dest, int tag, const void *buf, size_t size,
		std::function<void()> finisher=nullptr)=0;

	void send_copy(int dest, int tag, const void *buf, size_t size);

	template <typename T>
	void send(int dest, int tag, const T &val)
	{
		send_copy(dest, tag, &val, sizeof(T));
	}

	void bcast(int tag);

	MsgHandler set_handler(MsgHandler);

	virtual int rank() const=0;

	virtual unsigned int size() const=0;

	virtual int next_rank() const;

	virtual bool is_root() const noexcept;

	virtual void barrier()=0;

protected:
	MsgHandler handler_;
};

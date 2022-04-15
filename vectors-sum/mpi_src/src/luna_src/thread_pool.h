#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

class ThreadPool
{
public:
	ThreadPool();

	// start (more) threads
	// Note: must not start while stop() is in progress
	void start(size_t threads_num=1);

	// request threads stop and join them (also waits for the queue to
	// become empty)
	void stop();

	// add job
	void submit(std::function<void()>);

	void on_empty(std::function<void()>);

	void on_submit(std::function<void()>);

	bool has_jobs();

	virtual std::string to_string() const;

private:
	mutable std::mutex m_;
	std::condition_variable cv_;
	std::vector<std::thread*> threads_;
	std::queue<std::function<void()> > jobs_;
	std::function<void()> on_empty_handler_;
	std::function<void()> on_submit_handler_;
	size_t running_jobs_;
	bool stop_flag_;

	void routine();
};

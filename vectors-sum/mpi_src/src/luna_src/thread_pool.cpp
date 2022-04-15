#include "thread_pool.h"

#include <cassert>

#include "common.h"

ThreadPool::ThreadPool()
	: on_empty_handler_([](){}), on_submit_handler_([](){}), running_jobs_(0), stop_flag_(false)
{
	stop();
}

void ThreadPool::start(size_t threads_num)
{
	std::lock_guard<std::mutex> lk(m_);
	if (stop_flag_) {
		throw std::runtime_error("start while stopping ThreadPool");
	}

	for (auto i=0u; i<threads_num; i++) {
		threads_.push_back(new std::thread([this](){
			this->routine();
		}));
	}
}

void ThreadPool::stop()
{
	std::unique_lock<std::mutex> lk(m_);
	if (stop_flag_) {
		throw std::runtime_error("stop while stopping ThreadPool");
	}

	stop_flag_=true;
	cv_.notify_all();

	while (!threads_.empty()) {

		std::thread *t=threads_.back();
		threads_.pop_back();

		lk.unlock();

		t->join();
		delete t;

		lk.lock();
	}

	stop_flag_=false;
}

void ThreadPool::submit(std::function<void()> job)
{
	std::lock_guard<std::mutex> lk(m_);
	jobs_.push(job);

    on_submit_handler_();

	cv_.notify_one();
}

void ThreadPool::on_empty(std::function<void()> on_empty_handler)
{
    std::lock_guard<std::mutex> lk(m_);
    on_empty_handler_ = on_empty_handler;
}

void ThreadPool::on_submit(std::function<void()> on_submit_handler)
{
    std::lock_guard<std::mutex> lk(m_);
    on_submit_handler_ = on_submit_handler;
}

std::string ThreadPool::to_string() const
{
	std::lock_guard<std::mutex> lk(m_);
	return std::to_string(threads_.size()) + "Th "
		+ std::to_string(jobs_.size()) + "Jb "
		+ std::to_string(running_jobs_) + " RJ "
		+ (stop_flag_? "S": "");
}

bool ThreadPool::has_jobs()
{
    std::lock_guard<std::mutex> lk(m_);
    return !jobs_.empty();
}

void ThreadPool::routine()
{
	std::unique_lock<std::mutex> lk(m_);
	while (!stop_flag_ || !jobs_.empty() || running_jobs_>0) {
		if (jobs_.empty()) {
            on_empty_handler_();
			cv_.wait(lk);
			continue;
		}
		
		auto job=jobs_.front();
		jobs_.pop();
		running_jobs_++;
		
		lk.unlock();

		job();

		lk.lock();

		assert(running_jobs_>0);
		running_jobs_--;
		if (running_jobs_==0 && stop_flag_) {
			cv_.notify_all();
		}
	}
}

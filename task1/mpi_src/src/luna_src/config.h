#pragma once

#include <string>
#include <vector>

constexpr unsigned int
	DEFAULT_WORKER_THREADS_COUNT=4,
	DEFAULT_STEAL_PROC_COUNT=2,
	DEFAULT_COMM_REQUEST_THREADS_COUNT=1,
	DEFAULT_COMM_RECEIVE_THREADS_COUNT=1;


class Config
{
public:
	enum RunMode {
		UNSET,
		VERSION,
		NORMAL,
		HELP,
		ERROR
	};

	Config(int argc, char **argv);

	RunMode get_run_mode() const noexcept ;
	const std::string &argv(unsigned int) const;

	const std::string &get_fp_path() const;

	std::string get_help() const;
	std::string get_version() const;
	bool dynamic_balance() const;

	unsigned int get_steal_proc_count() const noexcept;
	unsigned int get_worker_threads_count() const noexcept;
	unsigned int get_comm_request_threads_count() const noexcept;
	unsigned int get_comm_recv_threads_count() const noexcept;
private:
	std::string program_name_;
	RunMode mode_;
	std::string fp_path_;
	std::vector<std::string> argv_;
	bool dynamic_balance_;
	unsigned int worker_threads_count_=DEFAULT_WORKER_THREADS_COUNT;
};

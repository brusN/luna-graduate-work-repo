#include "config.h"

#include <cassert>
#include <sstream>

#include "common.h"

Config::Config(int argc, char **argv)
	: mode_(UNSET), dynamic_balance_(false)
{
	assert(argc>=1);
	program_name_=argv[0];

	auto i=1;

	while (i<argc) {
		std::string arg(argv[i]);

        if (arg=="--help") {
            mode_=HELP;
            break;
        } else if (arg=="--version") {
            mode_=VERSION;
            break;
        } else if (arg=="-b") {
            dynamic_balance_ = true;
        } else if (arg.rfind("--worker-threads-count=", 0)==0) {
            try
            {
                worker_threads_count_ = std::stoi(arg.substr(23));
                if (worker_threads_count_ < 1) {
                    throw new std::invalid_argument("Invalid threads count: argument must be positive");
                }
            }
            catch (const std::invalid_argument& invalid_argument_exception)
            {
                ABORT(invalid_argument_exception.what())
            }
            catch (const std::out_of_range& out_of_range_exception)
            {
                ABORT(out_of_range_exception.what())
            }
        } else {
            if (mode_==UNSET) {
                mode_=NORMAL;
                fp_path_=arg;
            } else {
                argv_.push_back(arg);
            }
        }
        
		i++;
	}

	if (mode_==UNSET) {
		mode_=ERROR;
		ERROR("Not enough command-line arguments")
	}

	if (mode_==ERROR) {
		ABORT("Invalid command-line arguments, run with '--help' for info.")
	}
}

Config::RunMode Config::get_run_mode() const noexcept { return mode_; }

const std::string &Config::argv(unsigned int idx) const
{
	if (idx>=argv_.size()) {
		ABORT("Argv index out of range: " + std::to_string(idx) + ">="
			+ std::to_string(argv_.size()))
	}

	return argv_.at(idx);
}

const std::string &Config::get_fp_path() const
{
	assert(mode_==NORMAL);

	return fp_path_;
}

std::string Config::get_help() const
{
	std::ostringstream os;

	os << std::endl;
	os << "Usage:" << std::endl;
	os << "\t" << program_name_ << " path/to/program.so" << std::endl;
	os << "\t" << program_name_ << " --version" << std::endl;
	os << "\t" << program_name_ << " --help" << std::endl;
	os << std::endl;

	return os.str();
}

std::string Config::get_version() const
{
	std::ostringstream os;

	os << std::endl;
	os << "LuNA version: " << VERSION_RTS << std::endl;
	os << std::endl;

	return os.str();
}

bool Config::dynamic_balance() const
{
    return dynamic_balance_;
}

unsigned int Config::get_steal_proc_count() const noexcept {
    return DEFAULT_STEAL_PROC_COUNT;
}

unsigned int Config::get_worker_threads_count() const noexcept
{
	return worker_threads_count_;
}

unsigned int Config::get_comm_request_threads_count() const noexcept
{
	return DEFAULT_COMM_REQUEST_THREADS_COUNT;
}

unsigned int Config::get_comm_recv_threads_count() const noexcept
{
	return DEFAULT_COMM_RECEIVE_THREADS_COUNT;
}



#pragma once

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

#define STYLE_RESET       "\033[0m"     // default color
#define STYLE_BLUE_BOLD   "\033[34;1m"  // blue color with bold text
#define STYLE_GREEN_BOLD  "\033[1;32m"  // green color with bold text
#define STYLE_BOLD        "\033[1m"     // default color with bold text
#define STYLE_GREY        "\033[2m"     // semi bright color
#define STYLE_YELLOW_BOLD "\033[1;33m"  // yellow color with bold text
#define STYLE_RED_BOLD    "\033[1;31m"  // red color with bold text

#define VERSION_RTS "0.6"
#define SHOW_EXEC_TIME if(false)

#define TRACE(msg) if (false) { \
	printf("TRACE\t%s\n", std::string(msg).c_str()); \
}

#define LOG(msg) if (false) {\
	assert(!_prefix.empty());\
	FILE *f = fopen(logfilename.c_str(),"a");\
	fprintf(f,"%d | %lf | %s | %s:%d\n", \
		mpi_rank, wtime(), std::string(msg).c_str(), __FILE__, __LINE__); \
	fclose(f);\
}

#define NOTE(msg) {\
	assert(!_prefix.empty());\
	fprintf(stderr, "%s " STYLE_GREEN_BOLD "NOTE:" STYLE_RESET STYLE_BOLD " %s " STYLE_RESET \
		STYLE_BOLD "%s:%d\n" STYLE_RESET, _prefix.c_str(), \
		std::string(msg).c_str(), \
		__FILE__, __LINE__);\
}

#define WARN(msg) {\
	assert(!_prefix.empty());\
	fprintf(stderr, "%s " STYLE_YELLOW_BOLD "WARNING: " STYLE_RESET STYLE_BOLD " %s " STYLE_RESET \
		STYLE_BOLD "%s:%d\n" STYLE_RESET, _prefix.c_str(), \
		std::string(msg).c_str(), \
		__FILE__, __LINE__);\
}

#define ERROR(msg) {\
	assert(!_prefix.empty());\
	fprintf(stderr, "%s " STYLE_RED_BOLD "ERROR: " STYLE_RESET STYLE_BOLD " %s " STYLE_RESET \
		STYLE_BOLD "%s:%d\n" STYLE_RESET, _prefix.c_str(), \
		std::string(msg).c_str(), \
		__FILE__, __LINE__);\
}

#define ABORT(msg) {\
	assert(!_prefix.empty());\
	ERROR(msg) \
	fprintf(stderr, "%s " STYLE_RED_BOLD "ABORT \n" STYLE_RESET, _prefix.c_str()); \
	throw RuntimeError(__FILE__, __LINE__, msg); \
	abort();\
}

#define NIMPL ABORT("Code not implemented")

void wtime_init();
double wtime();
double wttime();

extern std::string _prefix;
extern int mpi_rank;
extern std::string logfilename;

class RuntimeError : public std::exception
{
	std::string file_, msg_;
	std::string full_msg_;
	int line_;
public:
	RuntimeError(const std::string &file, int line,
		const std::string &msg)
		: file_(file), msg_(msg), line_(line)
	{
		full_msg_=msg_ + " at " + file_ + ":" + std::to_string(line_);
	}

	const char *what() { return full_msg_.c_str(); }
};

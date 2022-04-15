#pragma once

#include <functional>
#include <string>
#include <vector>

#include "ucenv.h"

class CF;

class FP
{
public:
	~FP() noexcept (false);
	FP(const std::string &path);

	Block operator[](unsigned int block_idx) const;

private:
	std::string path_;
	void *so_;
	std::vector<Block> blocks_;
};

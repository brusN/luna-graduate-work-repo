#include "fp.h"

#include <dlfcn.h>

#include "cf.h"
#include "common.h"
#include "rts.h"

typedef void (*BlocksInitializer)(BlocksAppender);

FP::~FP() noexcept(false)
{
	blocks_.clear();

	if (dlclose(so_)) {
		ABORT(dlerror());
	}
}

FP::FP(const std::string &path)
	: path_(path)
{
	so_=dlopen(path.c_str(), RTLD_NOW);

	if (!so_) {
		ABORT(dlerror());
	}

	void *vinit_blocks=dlsym(so_, "init_blocks");

	if (!vinit_blocks) {
		ABORT(dlerror());
	}

	BlocksInitializer init_blocks;

	*(void**)(&init_blocks)=vinit_blocks; // to silence pedantic
		// warnings with direct cast from object ptr to function ptr:
		// init_blocks=(BlocksInitializer)vinit_blocks;
	
	init_blocks([this](Block block){
		blocks_.push_back(block);
		return blocks_.size()-1;
	});
}

Block FP::operator[](unsigned int block_idx) const
{
	if (block_idx>=blocks_.size()) {
		ABORT("Block index out of range: " + std::to_string(block_idx)
			+ " >= " + std::to_string(blocks_.size()));
	}

	return blocks_[block_idx];
}

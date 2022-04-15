#pragma once

#include <functional>
#include <typeinfo>

#include "cf.h"
#include "cyclic_locator.h"

enum BlockRetStatus
{
	UNDEFINED=0,
	CONTINUE,
	MIGRATE,
	WAIT,
	EXIT,
	STEAL
};

typedef std::function<BlockRetStatus (CF &)> Block;
typedef std::function<int (Block)> BlocksAppender;

typedef DF InputDF;
typedef DF OutputDF;

namespace luna { namespace ucenv {
	typedef ::DF OutputDF;
	typedef ::DF InputDF;
}}

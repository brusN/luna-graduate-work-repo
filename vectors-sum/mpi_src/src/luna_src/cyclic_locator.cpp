#include "cyclic_locator.h"

#include "comm.h"

CyclicLocator::CyclicLocator(int pos)
	: pos_(pos)
{
}

int CyclicLocator::get_next_rank(Comm &comm) const noexcept
{
	if (pos_>=0) {
		return pos_%comm.size();
	} else {
		int rest=(-pos_)%comm.size();
		return (comm.size()-rest)%comm.size();
	}
}

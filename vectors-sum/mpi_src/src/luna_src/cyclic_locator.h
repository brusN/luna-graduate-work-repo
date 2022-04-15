#pragma once

#include "locator.h"

class CyclicLocator : public Locator
{
	int pos_;
public:
	virtual ~CyclicLocator() {}

	CyclicLocator(int pos);

	virtual int get_next_rank(Comm &) const noexcept;
};

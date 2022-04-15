#pragma once

class Comm;

class Locator
{
public:
	virtual ~Locator() {}

	virtual int get_next_rank(Comm &) const=0;
};

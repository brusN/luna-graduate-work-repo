#pragma once

#include "serializable.h"

#include <vector>

class Id
{
	std::vector<int> idx_;
public:
	Id();
	Id(const std::vector<int> &);

	bool operator<(const Id &) const;
	
	operator bool() const;

	Id operator[](int) const;

	std::string to_string() const;

	virtual size_t get_serialization_size() const noexcept;
	virtual size_t serialize(void *buf, size_t buf_size) const;
	virtual size_t deserialize(const void *buf, size_t buf_size);

};

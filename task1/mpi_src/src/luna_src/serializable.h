#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>

class Serializable
{
public:
	~Serializable() {}

	virtual size_t get_serialization_size() const=0;
	virtual size_t serialize(void *buf, size_t buf_size) const=0;
	virtual size_t deserialize(const void *buf, size_t buf_size)=0;
};

// Helper routines

// Shift (buf, buf_size) pair by sizeof(val)
void shift(void *&buf, size_t &buf_size, size_t offset);

// Shift (buf, buf_size) pair by sizeof(val)
void shift(const void *&buf, size_t &buf_size, size_t offset);

// Store & shift
template <class T>
void put(void *&buf, size_t &buf_size, const T &val)
{
	assert(buf_size>=sizeof(val));
	memcpy(buf, &val, sizeof(val));
	shift(buf, buf_size, sizeof(val));
}

// Restore & shift
template <class T>
const T &get(const void *&buf, size_t &buf_size)
{
	assert(buf_size>=sizeof(T));
	const T &res=*static_cast<const T*>(buf);
	shift(buf, buf_size, sizeof(T));
	return res;
}

size_t get_serialization_size(const std::string &);
size_t serialize(void *buf, size_t buf_size, const std::string &);
size_t deserialize(std::string &s, const void *buf, size_t buf_size);

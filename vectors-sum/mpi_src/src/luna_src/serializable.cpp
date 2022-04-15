#include "serializable.h"

void shift(void *&buf, size_t &buf_size, size_t offset)
{
	assert(buf_size>=offset);
	char *ptr=static_cast<char *>(buf);
	buf=static_cast<void *>(ptr+offset);
	buf_size-=offset;
}

// Shift (buf, buf_size) pair by sizeof(val)
void shift(const void *&buf, size_t &buf_size, size_t offset)
{
	assert(buf_size>=offset);
	const char *ptr=static_cast<const char *>(buf);
	buf=static_cast<const void *>(ptr+offset);
	buf_size-=offset;
}

size_t get_serialization_size(const std::string &s)
{
	return sizeof(size_t)+s.size();
}

size_t serialize(void *buf, size_t buf_size, const std::string &s)
{
	put<size_t>(buf, buf_size, s.size());

	assert(buf_size>=s.size());

	memcpy(buf, s.c_str(), s.size());

	return sizeof(size_t)+s.size();
}

size_t deserialize(std::string &s, const void *buf, size_t buf_size)
{
	size_t size=get<size_t>(buf, buf_size);

	s=std::string(static_cast<const char *>(buf), size);

	return sizeof(size_t)+size;
}

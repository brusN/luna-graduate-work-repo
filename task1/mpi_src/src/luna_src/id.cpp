#include "id.h"

#include <cstring>
#include <sstream>

Id::Id() {}

Id::Id(const std::vector<int> &idx)
	: idx_(idx)
{}

bool Id::operator<(const Id &id) const
{
	if (idx_.size()<id.idx_.size()) { return true; }
	else if (idx_.size()>id.idx_.size()) { return false; }

	for (auto i=0u; i<idx_.size(); i++) {
		if (idx_[i]<id.idx_[i]) { return true; }
		else if (idx_[i]>id.idx_[i]) { return false; }
	}

	return false;
}

Id::operator bool() const
{
	return !idx_.empty();
}

Id Id::operator[](int idx) const
{
	Id res(*this);
	res.idx_.push_back(idx);
	return res;
}

std::string Id::to_string() const
{
	if (idx_.empty()) {
		return "ID<nil>";
	}

	std::ostringstream os;

	os << "ID<";
    os << idx_[0];
	for (auto i = 1u; i<idx_.size(); i++) {
			os << ", " << idx_[i];
	}
	os << ">";

	return os.str();
}

size_t Id::get_serialization_size() const noexcept 
{
	return sizeof(size_t)+idx_.size()*sizeof(int);
}

size_t Id::serialize(void *buf, size_t buf_size) const
{
	auto orig_buf_size=buf_size;
	assert(buf_size>=get_serialization_size());

	put<size_t>(buf, buf_size, idx_.size());

	for (auto idx : idx_) {
		put<int>(buf, buf_size, idx);
	}

	return orig_buf_size-buf_size;
}

size_t Id::deserialize(const void *buf, size_t buf_size)
{
	size_t orig_buf_size=buf_size;

	assert(idx_.empty());

	size_t size=get<size_t>(buf, buf_size);

	for (auto i=0u; i<size; i++) {
		idx_.push_back(get<int>(buf, buf_size));
	}

	return orig_buf_size-buf_size;
}

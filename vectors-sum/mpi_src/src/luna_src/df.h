#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <sstream>
#include <typeinfo>
#include <vector>

#include "common.h"
#include "serializable.h"

enum ValueType {
	TYPE_UNDEFINED=0,
	TYPE_INT,
	TYPE_REAL,
	TYPE_STRING,
	TYPE_VALUE,
	TYPE_NAME
};

class DF : public Serializable
{
protected:
	void *ptr_;
	size_t size_;
	std::atomic_int *refs_;
	ValueType type_;
public:
	~DF();

	DF();
	DF(const DF &);

	DF(int);
	DF(double);
	DF(const std::string &);
	DF(const void *, size_t);

	DF &operator=(int);
	DF &operator=(double);
	DF &operator=(const std::string &);
	DF &operator=(const DF &);

	DF operator+(const DF &) const;
	DF operator-(const DF &) const;
	DF operator*(const DF &) const;
	DF operator/(const DF &) const;
	DF operator%(const DF &) const;
	DF operator<(const DF &) const;
	DF operator<=(const DF &) const;
	DF operator&&(const DF &) const;
	DF operator==(const DF &) const;
	DF operator!=(const DF &) const;
	DF operator>=(const DF &) const;
	DF operator>(const DF &) const;

	bool is_set() const noexcept;
	bool is_unset() const noexcept;

	ValueType get_type() const { return type_; }

	int get_int() const;
	double get_real() const;
	std::string get_string() const;

	size_t get_size() const;
	const void *get_data() const;
	void *get_data();

	int cast_int() const;
	double cast_real() const;
	std::string cast_string() const;

	std::string to_string() const;

	virtual size_t get_serialization_size() const;
	virtual size_t serialize(void *buf, size_t buf_size) const;
	virtual size_t deserialize(const void *buf, size_t buf_size);

#ifdef ADD_DEBUG_INFO
	std::string DBG_INFO;
	std::string NAME;
#endif
protected:
	void _unlink();
	void _link(const DF &);
	void _grab(void *ptr, size_t size);
	void _copy(const void *ptr, size_t size);
	void _create(size_t size);

	// Compatibility members
public:
	template<class T>
	const T * getData() const
	{
		assert(size_%sizeof(T)==0);

		return static_cast<const T *>(get_data());
	}

	std::string getName() const;

	size_t getSize() const;

	void *create(size_t size);

	template<class T>
	T *create(size_t el_count)
	{
		return static_cast<T *>(create(el_count*sizeof(T)));
	}

	template<class T>
	T *create(size_t el_count, const T &init_val)
	{
		T *res=static_cast<T *>(create(el_count*sizeof(T)));

		for (auto i=0u; i<el_count; i++) {
			res[i]=init_val;
		}

		return res;
	}

	template<class T>
	T *getData()
	{
		assert(size_%sizeof(T)==0);

		return static_cast<T *>(get_data());
	}

	template<class T>
	void setValue(const T& val)
	{
		if (std::is_same<T, int>::value || std::is_same<T, double>::value
				|| std::is_same<T, std::string>::value) {
			*this=val;
		} else {
			ABORT("Unsupported type in setValue: "
				+std::string(typeid(T).name()))
		}
	}

	template<class T>
	const T &getValue() const
	{
		if (std::is_same<T, int>::value) {
			if (type_!=TYPE_INT) {
				ABORT("not an int: "+to_string());
			}
			return *static_cast<const T *>(ptr_);
		} else if (std::is_same<T, double>::value) {
			if (type_!=TYPE_REAL) {
				ABORT("not a real: "+to_string());
			}
			return *static_cast<const T *>(ptr_);
		} else {
			ABORT("getValue<"+std::string(typeid(T).name())+">() failed for "+to_string());
		}
	}

	void copy(const DF &);
};

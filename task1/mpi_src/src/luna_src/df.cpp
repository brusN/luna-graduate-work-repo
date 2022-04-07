#include "df.h"

#include <cstring>
#include <iomanip>
#include <sstream>

const std::vector<std::string> _STR_VT={
	"(unset)", "int", "real", "string", "value", "name"
};

DF::~DF()
{
	_unlink();
}

DF::DF()
	: ptr_(nullptr), size_(0), refs_(nullptr), type_(TYPE_UNDEFINED)
{
}

DF::DF(const DF &df)
	: ptr_(nullptr), size_(0), refs_(nullptr), type_(TYPE_UNDEFINED)
#ifdef ADD_DEBUG_INFO	
	, DBG_INFO(df.DBG_INFO)
#endif
{
	_link(df);
}

DF::DF(int val)
	: ptr_(nullptr), size_(0), refs_(nullptr), type_(TYPE_UNDEFINED)
{
	_copy(&val, sizeof(val));
	type_=TYPE_INT;
}

DF::DF(double val)
	: ptr_(nullptr), size_(0), refs_(nullptr), type_(TYPE_UNDEFINED)
{
	_copy(&val, sizeof(val));
	type_=TYPE_REAL;
}

DF::DF(const std::string &s)
	: ptr_(nullptr), size_(0), refs_(nullptr), type_(TYPE_UNDEFINED)
{
	_copy(s.c_str(), s.size()+1);
	type_=TYPE_STRING;
}

DF::DF(const void *data, size_t sz)
	: ptr_(nullptr), size_(0), refs_(nullptr), type_(TYPE_UNDEFINED)
{
	_copy(data, sz);
}

DF &DF::operator=(int val)
{
	_unlink();
	_copy(&val, sizeof(val));
	type_=TYPE_INT;
	return *this;
}

DF &DF::operator=(double val)
{
	_unlink();
	_copy(&val, sizeof(val));
	type_=TYPE_REAL;
	return *this;
}

DF &DF::operator=(const std::string &s)
{
	_unlink();
	_copy(s.c_str(), s.size()+1);
	type_=TYPE_STRING;
	return *this;
}

DF &DF::operator=(const DF &df)
{
	_unlink();
	_link(df);
#ifdef ADD_DEBUG_INFO	
	NAME=df.NAME;
	DBG_INFO=df.DBG_INFO;
#endif
	return *this;
}

#define DEFINE_DF_OPERATOR(op) \
DF DF::operator op (const DF &df) const \
{ \
	if (is_unset() or df.is_unset()) { \
		ABORT("Operand(s) unset"); \
	} \
	 \
	if (type_==TYPE_INT && df.type_==TYPE_INT) \
		return DF(get_int() op df.get_int()); \
	else if (type_==TYPE_INT && df.type_==TYPE_REAL) \
		return DF(get_int() op df.get_real()); \
	else if (type_==TYPE_REAL && df.type_==TYPE_INT) \
		return DF(get_real() op df.get_int()); \
	else if (type_==TYPE_REAL && df.type_==TYPE_REAL) \
		return DF(get_real() op df.get_real()); \
	else \
		ABORT("Unsupported operand types: " + _STR_VT[type_] + " " #op \
			" " + _STR_VT[df.type_]) \
}

DEFINE_DF_OPERATOR(+)
DEFINE_DF_OPERATOR(-)
DEFINE_DF_OPERATOR(*)
DEFINE_DF_OPERATOR(/)

DF DF::operator % (const DF &df) const
{
	if (type_==TYPE_INT && df.type_==TYPE_INT) {
		return DF(get_int() % df.get_int());
	} else {
		ABORT("Unsupported operand types: " + _STR_VT[type_] + " % "
			+ _STR_VT[df.type_]);
	}
}

#undef DEFINE_DF_OPERATOR

#define DEFINE_DF_OPERATOR(op) \
DF DF::operator op(const DF &df) const \
{ \
	if (is_unset() or df.is_unset()) { \
		ABORT("Operand(s) unset"); \
	} \
 \
	if (type_==TYPE_INT && df.type_==TYPE_INT) \
		return DF(get_int() op df.get_int()? 1: 0); \
	else if (type_==TYPE_INT && df.type_==TYPE_REAL) \
		return DF(get_int() op df.get_real()? 1: 0); \
	else if (type_==TYPE_REAL && df.type_==TYPE_INT) \
		return DF(get_real() op df.get_int()? 1: 0); \
	else if (type_==TYPE_REAL && df.type_==TYPE_REAL) \
		return DF(get_real() op df.get_real()? 1: 0); \
	else \
		ABORT("Unsupported operand types: " + _STR_VT[type_] + " " #op \
			" " + _STR_VT[df.type_]) \
}

DEFINE_DF_OPERATOR(<)
DEFINE_DF_OPERATOR(<=)
DEFINE_DF_OPERATOR(&&)
DEFINE_DF_OPERATOR(==)
DEFINE_DF_OPERATOR(!=)
DEFINE_DF_OPERATOR(>)
DEFINE_DF_OPERATOR(>=)

#undef DEFINE_DF_OPERATOR

bool DF::is_set() const noexcept { return type_!=TYPE_UNDEFINED; }
bool DF::is_unset() const noexcept { return !is_set(); }

int DF::get_int() const
{
	if (type_!=TYPE_INT) {
		ABORT("get_int failed for type " + _STR_VT[type_])
	}

	assert(size_==sizeof(int));

	return *static_cast<int *>(ptr_);
}

double DF::get_real() const
{
	if (type_!=TYPE_REAL) {
		ABORT("get_real failed for type " + _STR_VT[type_])
	}

	assert(size_==sizeof(double));

	return *static_cast<double *>(ptr_);
}

std::string DF::get_string() const
{
	if (type_!=TYPE_STRING) {
		ABORT("get_string failed for type " + _STR_VT[type_])
	}

	return std::string(static_cast<const char *>(ptr_));
}

size_t DF::get_size() const
{
	if (type_!=TYPE_VALUE) {
		ABORT("Cannot get_size of " + _STR_VT[type_])
	}

	return size_;
}

const void *DF::get_data() const
{
	if (type_!=TYPE_VALUE) {
		ABORT("Cannot get_data of " + _STR_VT[type_])
	}

	assert(ptr_);

	return ptr_;
}

void *DF::get_data()
{
	if (type_!=TYPE_VALUE) {
		ABORT("Cannot get_data of " + _STR_VT[type_])
	}

	assert(ptr_);

	return ptr_;
}

int DF::cast_int() const
{
	switch (type_) {
		case TYPE_INT:
			return get_int();
		case TYPE_REAL:
			return static_cast<int>(get_real());
		case TYPE_STRING: {
			size_t pos;
			std::string s(get_string());
			int val;
			try{
				val=std::stoi(s, &pos);
				if (pos!=s.size()) {
					ABORT("cast to int failed: string(" + s + ")")
				}
				return val;
			} catch (std::invalid_argument &x) {
				ABORT("cast to int failed: string(" + s + ")")
			} catch (std::out_of_range &x) {
				ABORT("cast to int failed: string(" + s + ")")
			}
		}
		default:
			ABORT("cast to int failed: " + to_string())
	}
}

double DF::cast_real() const
{
	switch (type_) {
		case TYPE_INT:
			return static_cast<double>(get_int());
		case TYPE_REAL:
			return get_int();
		case TYPE_STRING: {
			size_t pos;
			std::string s(get_string());
			double val;
			try{
				val=std::stod(s, &pos);
				if (pos!=s.size()) {
					ABORT("cast to real failed: string(" + s + ")")
				}
				return val;
			} catch (std::invalid_argument &x) {
				ABORT("cast to real failed: string(" + s + ")")
			} catch (std::out_of_range &x) {
				ABORT("cast to real failed: string(" + s + ")")
			}
		}
		default:
			ABORT("cast to real failed: " + to_string())
	}
}

std::string DF::cast_string() const
{
	switch (type_) {
		case TYPE_INT:
			return std::to_string(get_int());
		case TYPE_REAL:
			return std::to_string(get_real());
		case TYPE_STRING:
			return get_string();
		default:
			ABORT("cast to string failed: " + to_string())
	}
}

void DF::_unlink()
{
	if (type_==TYPE_UNDEFINED) {
		assert(!refs_ && !size_ && !ptr_);
		return;
	}

	int refs=--(*refs_);
	assert(refs>=0);

	if (refs==0) {
		operator delete(ptr_);
		delete refs_;
	}

	type_=TYPE_UNDEFINED;
	ptr_=nullptr;
	refs_=nullptr;
	size_=0;
}

void DF::_link(const DF &df)
{
	assert(type_==TYPE_UNDEFINED && !ptr_ && !size_ && !refs_);

	if (df.is_unset()) {
		return;
	}

	type_=df.type_;
	ptr_=df.ptr_;
	size_=df.size_;
	refs_=df.refs_;

	int refs=++(*refs_);
	assert(refs>1);
}

void DF::_grab(void *ptr, size_t size)
{
	assert(type_==TYPE_UNDEFINED && !ptr_ && !size_ && !refs_);
	
	type_=TYPE_VALUE;
	size_=size;
	ptr_=ptr;
	refs_=new std::atomic_int(1);
}

void DF::_copy(const void *ptr, size_t size)
{
	assert(type_==TYPE_UNDEFINED && !ptr_ && !size_ && !refs_);
	
	type_=TYPE_VALUE;
	size_=size;
	ptr_=operator new(size_);
	if (!ptr_) {
		ABORT("operator new(" + std::to_string(size_) + ") failed")
	}
	memcpy(ptr_, ptr, size_);
	refs_=new std::atomic_int(1);
}

void DF::_create(size_t size)
{
	assert(type_==TYPE_UNDEFINED && !ptr_ && !size_ && !refs_);
	
	type_=TYPE_VALUE;
	size_=size;
	ptr_=operator new(size_);
	if (!ptr_) {
		ABORT("operator new(" + std::to_string(size_) + ") failed")
	}
	refs_=new std::atomic_int(1);
}

std::string DF::to_string() const
{
	std::string res;

	switch (type_) {
		case TYPE_UNDEFINED:
			res="DF<unset>";
			break;
		case TYPE_INT:
			res="DF<int>(" + std::to_string(get_int()) + ")";
			break;
		case TYPE_REAL:
			res="DF<real>(" + std::to_string(get_real()) + ")";
			break;
		case TYPE_STRING:
			res="DF<string>(" + std::string(get_string()) + ")";
			break;
		case TYPE_VALUE: {
			char buf[1024];
			char cur=0;
			const char *s=static_cast<const char *>(ptr_);
			cur+=sprintf(buf, "DF<size=%d>(", (int)size_);
			for (auto i=0u; i<size_ && i<10u; i++) {
				cur+=sprintf(buf+cur, "%s%02X", (i>0? ", ": ""), s[i]);
			}
			cur+=sprintf(buf+cur, "%s)", size_>10? "...": "");
			assert(cur<=1024);
			res=buf;
			break;
		}
		case TYPE_NAME:
			NIMPL
		default:
			ABORT("Unexpected df type: " + std::to_string(type_))
	}
#ifdef ADD_DEBUG_INFO
	return res+"{name=" + NAME + ",DBI=" + DBG_INFO + "}";
#endif
	return res;

}

size_t DF::get_serialization_size() const
{
	return sizeof(size_t)+size_+sizeof(ValueType)
#ifdef ADD_DEBUG_INFO
	+::get_serialization_size(NAME)
	+::get_serialization_size(DBG_INFO)
#endif
	;
}

size_t DF::serialize(void *buf, size_t buf_size) const
{
	size_t orig_buf_size=buf_size;
	assert(buf_size>=get_serialization_size());

	put<size_t>(buf, buf_size, size_);

	memcpy(buf, ptr_, size_);

	shift(buf, buf_size, size_);

	put<ValueType>(buf, buf_size, type_);

#ifdef ADD_DEBUG_INFO
	shift(buf, buf_size, ::serialize(buf, buf_size, NAME));
	shift(buf, buf_size, ::serialize(buf, buf_size, DBG_INFO));
#endif

	return orig_buf_size-buf_size;
}

size_t DF::deserialize(const void *buf, size_t buf_size)
{
	size_t orig_buf_size=buf_size;

	_unlink();

	size_t size=get<size_t>(buf, buf_size);

	assert(0<=buf_size);
	assert(0<=size);
	assert(size<=buf_size);

	_copy(buf, size);

	shift(buf, buf_size, size_);

	type_=get<ValueType>(buf, buf_size);

#ifdef ADD_DEBUG_INFO
	shift(buf, buf_size, ::deserialize(NAME, buf, buf_size));
	shift(buf, buf_size, ::deserialize(DBG_INFO, buf, buf_size));
#endif

	return orig_buf_size-buf_size;
}

std::string DF::getName() const
{
#ifdef ADD_DEBUG_INFO
	return NAME;
#endif
	return "(df)";
}

size_t DF::getSize() const
{
	if (is_unset()) {
		ABORT("Cannot getSize(), df is unset");
	}

	return size_;
}

void *DF::create(size_t size)
{
	_unlink();
	_create(size);
	return ptr_;
}

void DF::copy(const DF &df)
{
	_unlink();
	_link(df);
}

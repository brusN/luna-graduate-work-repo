#include "cf.h"

#include "comm.h"
#include "common.h"
#include "rts.h"

std::atomic_uint CF::uid(0);

CF::CF(RTS *rts, const std::string &caption)
	: rts_(rts), caption_(caption), dest_rank_(-1), NextBlock(0)
{
	self_.reset(this);
	rts_->change_load(1);
	LOG("WORKLOAD: CF created");
}

CF::~CF()
{
	if (push_cfid_) {
		rts_->unexpect_pushes(push_cfid_);
	}
	rts_->change_load(-1);
	LOG("WORKLOAD: CF destroyed");
}

DF CF::argv(ValueType t, unsigned int n)
{
	const std::string &s=rts_->argv(n);
	switch (t) {
		case TYPE_INT:
			return DF(std::stoi(s));
		case TYPE_REAL: {
			double val;
			std::stringstream ss;
			ss << s;
			ss >> val;
			return DF(val);
		}
		case TYPE_STRING:
			return DF(s.c_str());
		default:
			ABORT("Conversion to type not supported: " + std::to_string(t))
	}
}

DF &CF::arg(unsigned int pos)
{
	std::lock_guard<std::mutex> lk(m_);

	if (pos>args_.size()) {
		ABORT("arg() index of range: " + std::to_string(pos)
			+ " >= " + std::to_string(args_.size()));
	}

	if (pos==args_.size()) {
		args_.push_back(DF());
	}

	return args_[pos];
}

Id &CF::id(unsigned int pos)
{
	std::lock_guard<std::mutex> lk(m_);

	if (pos>ids_.size()) {
		ABORT("id() index out of range: " + std::to_string(pos)
			+ " >= " + std::to_string(ids_.size()));
	}

	if (pos==ids_.size()) {
		ids_.push_back(Id());
	}

	return ids_[pos];
}

DF CF::wait(const Id &id)
{
	std::lock_guard<std::mutex> lk(m_);

	auto it=store_.find(id);

	if (it==store_.end()) {
		awaited_dfs_.insert(id);
		LOG("Wait " + id.to_string() + " by " + to_string());
		DF res;
#ifdef ADD_DEBUG_INFO
		res.DBG_INFO="wait_failed:" + id.to_string();
#endif
		return res;
	} else {
		LOG("Wait " + id.to_string() + " -> " + it->second.to_string()
			+ " by " + to_string());
		return it->second;
	}
}

Id CF::create_id()
{
	return Id({
		rts_->comm().rank(),
		static_cast<int>(CF::uid++)
	});
}

CF *CF::fork(int block, const std::string &caption)
{
	CF *cf=new CF(rts_, caption);
	cf->NextBlock=block;

	std::lock_guard<std::mutex> lk(m_);

	forked_cfs_.push_back(cf);

	return cf;
}

void CF::post(const Id &id, const DF &val, const Locator &loc,
	int req_count)
{
	rts_->post(id, val,loc, req_count);
}

void CF::request(const Id &id, const Locator &loc)
{
	LOG("requested	"+id.to_string()+" by "
		+to_string());
	CFPtr *cf_ptr=new CFPtr(self_);
	rts_->request(id, loc, [this, id, cf_ptr](const DF &df){
		std::unique_lock<std::mutex> lk(m_);

		LOG("responded "+id.to_string() + "=" + df.to_string() + " for "
			+to_string());

		auto it=store_.find(id);
		if (it!=store_.end()) {
			// Allow re-requesting (needed for while-next-iter)
			// TODO insert asssertion, that this CF is 'while'
			//	to ensure no other CFs request DFs more than once
			// ABORT("Duplicate id response: " + id.to_string()
			//	+ ", val=" + df.to_string());
		} else {
			store_[id]=df;
		}
		
		size_t removed=awaited_dfs_.erase(id);

		if (removed>0 && awaited_dfs_.empty()) {
			if (self_.get()) {
				lk.unlock();
				rts_->submit(this);
			} else {
				lk.unlock();
			}
		}
		delete cf_ptr;
	});
}

bool CF::migrate(const Locator &loc)
{
	std::lock_guard<std::mutex> lk(m_);
	
	dest_rank_=loc.get_next_rank(rts_->comm());

	if (dest_rank_==rts_->comm().rank()) {
		return false;
	}

	return true;
}

bool CF::check_steal()
{
    int steal_rank=rts_->get_steal_req();
    if( steal_rank!=-1) {
        dest_rank_=steal_rank;
        return true;
    }
    return false;
}

void CF::destroy(const Id &id, const Locator &loc)
{
	rts_->destroy(id, loc);
}

void CF::store(const Id &id, const DF &val)
{
	std::unique_lock<std::mutex> lk(m_);

	LOG("Store " + id.to_string() + " := " + val.to_string())

	if (store_.find(id)!=store_.end()) {
		ABORT("Duplicate id");
	}
	
	store_[id]=val;
}

void CF::push(const Id &dfid, const DF &val, const Id &cfid,
	const Locator &loc)
{
	rts_->push(dfid, val, cfid, loc);
}

void CF::expect_pushes(const Id &cfid)
{
	rts_->expect_pushes(cfid, [this](const Id &id, const DF &df){
		std::unique_lock<std::mutex> lk(m_);

		auto it=store_.find(id);
		if (it!=store_.end()) {
			ABORT("Duplicate id");
		}
		store_[id]=df;
		
		size_t removed=awaited_dfs_.erase(id);

		if (removed>0 && awaited_dfs_.empty()) {
			lk.unlock();
			rts_->submit(this);
		}
	});
}

std::string CF::to_string() const
{
	if (caption_.empty()) {
		return "CF " + std::to_string((unsigned long long)this) + " : "
			+ std::to_string(NextBlock)
#ifdef ADD_DEBUG_INFO
			+ " "
			+ DBG_INFO
#endif
			;
	} else {
		return "CF " + caption_ + " : " + std::to_string(NextBlock)
#ifdef ADD_DEBUG_INFO
		+ " " + DBG_INFO
#endif
		;
	}
}

size_t CF::get_serialization_size() const
{
	std::lock_guard<std::mutex> lk(m_);

	size_t size=
		sizeof(int) // NextBlock
		+sizeof(size_t) // args_ size
		+sizeof(size_t) // ids_ size
		+sizeof(size_t) // store_ size
		+::get_serialization_size(caption_)
	;

	for (auto i=0u; i<args_.size(); i++) {
		size+=args_[i].get_serialization_size();
	}

	for (auto i=0u; i<ids_.size(); i++) {
		size+=ids_[i].get_serialization_size();
	}

	for (auto kv : store_) {
		size+=kv.first.get_serialization_size()
			+kv.second.get_serialization_size();
	}

#ifdef ADD_DEBUG_INFO
	size+=::get_serialization_size(DBG_INFO);
#endif

	return size;
}

size_t CF::serialize(void *buf, size_t buf_size) const
{
	assert(buf_size>=get_serialization_size());

	std::lock_guard<std::mutex> lk(m_);

	size_t orig_buf_size=buf_size;

	put<int>(buf, buf_size, NextBlock);

	put<size_t>(buf, buf_size, args_.size());

	for (auto df : args_) {
		shift(buf, buf_size, df.serialize(buf, buf_size));
	}

	put<size_t>(buf, buf_size, ids_.size());

	for (auto id : ids_) {
		shift(buf, buf_size, id.serialize(buf, buf_size));
	}

	put<size_t>(buf, buf_size, store_.size());

	for (auto kv : store_) {
		shift(buf, buf_size, kv.first.serialize(buf, buf_size));
		shift(buf, buf_size, kv.second.serialize(buf, buf_size));
	}

	shift(buf, buf_size, ::serialize(buf, buf_size, caption_));

#ifdef ADD_DEBUG_INFO
	shift(buf, buf_size, ::serialize(buf, buf_size, DBG_INFO));
#endif

	return orig_buf_size-buf_size;
}

size_t CF::deserialize(const void *buf, size_t buf_size)
{
	std::lock_guard<std::mutex> lk(m_);

	assert(args_.empty() && ids_.empty() && store_.empty());

	size_t orig_buf_size=buf_size;

	NextBlock=get<int>(buf, buf_size);

	size_t args_size=get<size_t>(buf, buf_size);

	for (auto i=0u; i<args_size; i++) {
		DF df;
		shift(buf, buf_size, df.deserialize(buf, buf_size));
		args_.push_back(df);
	}

	size_t ids_size=get<size_t>(buf, buf_size);

	for (auto i=0u; i<ids_size; i++) {
		Id id;
		shift(buf, buf_size, id.deserialize(buf, buf_size));
		ids_.push_back(id);
	}

	size_t store_size=get<size_t>(buf, buf_size);

	for (auto i=0u; i<store_size; i++) {
		Id id;
		shift(buf, buf_size, id.deserialize(buf, buf_size));
		DF df;
		shift(buf, buf_size, df.deserialize(buf, buf_size));
		assert(store_.find(id)==store_.end());
		store_[id]=df;
	}

	shift(buf, buf_size, ::deserialize(caption_, buf, buf_size));

#ifdef ADD_DEBUG_INFO
	shift(buf, buf_size, ::deserialize(DBG_INFO, buf, buf_size));
#endif

	return orig_buf_size-buf_size;
}

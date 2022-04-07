#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <vector>
#include <memory>

#include "common.h"
#include "df.h"
#include "id.h"
#include "locator.h"
#include "rts.h"
#include "serializable.h"

class CF;
using CFPtr = std::shared_ptr<CF>;

class CF : public Serializable
{
	friend class RTS;

	RTS *rts_;
	std::string caption_;
	std::vector<Id> ids_;
	std::vector<DF> args_;
	std::map<Id, DF> store_;
	mutable std::mutex m_;

	// When wait() is invoked, id is added to awaited_dfs_ unless
	// present in store_. When returned with WAIT status CF waits
	// until all awaited_dfs_ are delivered, then it is resumed.
	std::set<Id> awaited_dfs_;

	// Forked cfs are submitted after parent cf returns from block
	std::vector<CF *> forked_cfs_;
	int dest_rank_;
	Id push_cfid_;

	CFPtr self_;

	CF(const CF &)=delete;
	void operator=(const CF &)=delete;

	CF(RTS *, const std::string &caption="");

public:
	unsigned int NextBlock;

	virtual ~CF();

	DF argv(ValueType, unsigned int num);

	DF &arg(unsigned int);
	Id &id(unsigned int);

	DF wait(const Id &id);

	Id create_id();

	CF *fork(int block, const std::string &caption="");

	void post(const Id &id, const DF &val, const Locator &,
		int req_count);

	void request(const Id &id, const Locator &);

	bool migrate(const Locator &loc);

	bool check_steal();

	void destroy(const Id &id, const Locator &);

	void store(const Id &id, const DF &val);

	void push(const Id &dfid, const DF &val, const Id &cfid,
		const Locator &);

	void expect_pushes(const Id &cfid);

	std::string to_string() const;

	// Serializable
	// Format: int next_block_idx, ids, store_
	virtual size_t get_serialization_size() const;
	virtual size_t serialize(void *buf, size_t buf_size) const;
	virtual size_t deserialize(const void *buf, size_t buf_size);

	static std::atomic_uint uid;

#ifdef ADD_DEBUG_INFO
	std::string DBG_INFO;
#endif
};

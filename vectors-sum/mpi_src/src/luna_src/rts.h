#pragma once

#include <condition_variable>
#include <map>
#include <mutex>
#include <set>

#include "config.h"
#include "df.h"
#include "id.h"
#include "idle_stopper.h"
#include "locator.h"
#include "thread_pool.h"

class Comm;
class Config;
class FP;
class CF;

class RTS
{
	Comm *comm_;
	const Config *conf_;
	const FP *fp_;
	IdleStopper<int> *stopper_;
	bool finished_flag_;
	ThreadPool pool_;
	std::mutex m_;
	std::condition_variable cv_;
	bool need_jobs_;
public:
	virtual ~RTS();

	RTS(Comm &, const Config &, const FP &);

	int run();

	Comm &comm();

	void submit(CF *);

	const std::string &argv(unsigned int n);

	void post(const Id &id, const DF &val, const Locator &loc,
		int req_count);

	void request(const Id &id, const Locator &,
		std::function<void (const DF &)>);

	void destroy(const Id &id, const Locator &);

	void push(const Id &dfid, const DF &val, const Id &cfid,
		const Locator &);

	void expect_pushes(const Id &cfid,
		std::function<void (const Id &, const DF &)>);

	void unexpect_pushes(const Id &cfid);

	int get_steal_req();

	void steal_req(int tag, unsigned int proc_count);

	void change_load(int delta) {
		stopper_->change_works(delta);
	}

	std::string get_status() const;
private:
	void on_recv(int src, int tag, void *buf, size_t);

	void _submit(CF *);

	std::map<Id, std::pair<int, DF> > posts_;
	std::map<Id, std::vector<std::function<void (const DF &)> > > requests_;
	std::set<Id> destroys_;

	std::map<Id, std::vector<std::pair<Id, DF> > > pushed_;
	std::map<Id, std::function<void (const Id &, const DF &)> > waiters_;

	std::set<int> steal_requests_;

	void _check_requests(const Id &id);

	void _post(const Id &id, const DF &, int req_count);
	void _request(const Id &id, std::function<void (const DF &)> cb);
	void _destroy(const Id &id);
	void _push(const Id &dfid, const DF &val, const Id &cfid);
};

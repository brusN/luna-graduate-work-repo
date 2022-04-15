#include "rts.h"

// TODO: make requests and pushes impact load

// BUG: there is a tiny chance, that while CF is migrating
// the message will be sent, but not yet received. If such condition
// lasts long enough, the stopper may consider the whole system idle.
// The probability is very small, almost negliable, but still.
// This can be fixed e.g. by awaiting response on delivery of CF

#include "cf.h"
#include "comm.h"
#include "common.h"
#include "config.h"
#include "fp.h"
#include "tags.h"

RTS::~RTS()
{
	delete stopper_;
}

RTS::RTS(Comm &comm, const Config &conf, const FP &fp)
	: comm_(&comm), conf_(&conf), fp_(&fp), need_jobs_(false)
{
	comm_->set_handler([this](int from, int tag, void *buf, size_t size) {
		on_recv(from, tag, buf, size);
	});

	comm_->barrier();
	stopper_=new IdleStopper<int>(comm_->rank(),
		[this](const int &marker){
			// send to next
			comm_->send(comm_->next_rank(), TAG_IDLE_MARKER, marker);
		},
		[this]() {
			// all idle
			comm_->bcast(TAG_STOP);
		}
	);

	if(conf.dynamic_balance()) {
        unsigned int proc_count = conf_->get_steal_proc_count();
        pool_.on_empty([this, proc_count]() {
            if (!need_jobs_) {
                need_jobs_ = true;
                steal_req(TAG_STEAL, proc_count);
            }
        });

        pool_.on_submit([this, proc_count]() {
            if (need_jobs_) {
                need_jobs_ = false;
                steal_req(TAG_STEAL_REVOKE, proc_count);
            }
        });
    }

    comm_->barrier();
}

int RTS::run()
{
	double start_time=wtime();
	comm_->barrier();
	std::unique_lock<std::mutex> lk(m_);
	pool_.start(conf_->get_worker_threads_count());

	finished_flag_=false;

	if (comm_->is_root()) {
		CF *cf=new CF(this, "_main");
		LOG("Create "+cf->to_string())
		_submit(cf);
		stopper_->start();
	}

	while (!finished_flag_) {
		cv_.wait(lk);
	}
	pool_.stop();

#ifdef ADD_DEBUG_INFO
	comm_->barrier();

	if (!posts_.empty()) {
		size_t garbage_size=0;

		for (auto x : posts_)
			garbage_size+=x.second.second.getSize();

		WARN("Garbage: " + std::to_string(posts_.size()) + " posts ("
			+ std::to_string(garbage_size/1024.0/1024.0) + " MB)");
	}
	if (!requests_.empty()) {
		WARN("Garbage: " + std::to_string(requests_.size()) + " requests");
	}
	if (!destroys_.empty()) {
		WARN("Garbage: " + std::to_string(destroys_.size()) + " destroys");
	}

	comm_->barrier();
#endif

	SHOW_EXEC_TIME
	if (comm_->is_root()) {
		printf(STYLE_BOLD "NORMAL SYSTEM STOP: %lf sec\n" STYLE_RESET,
			wtime()-start_time);
	}

	return 0;
}

Comm &RTS::comm() { return *comm_; }

void RTS::submit(CF *cf)
{
	std::lock_guard<std::mutex> lk(m_);

	_submit(cf);
}

const std::string &RTS::argv(unsigned int n)
{
	return conf_->argv(n);
}

void RTS::post(const Id &id, const DF &val, const Locator &loc,
	int req_count)
{
	int rank=loc.get_next_rank(*comm_);

	TRACE("POST\t" + id.to_string() + "\t" + std::to_string(rank));

	if (rank==comm_->rank()) {
		_post(id, val, req_count);
	} else {
		// MSG format: [id] [df] [count]
		size_t size=
			id.get_serialization_size()
			+val.get_serialization_size()
			+sizeof(int);
		void *buf=operator new(size);

		size_t s=size;
		void *b=buf;

		shift(b, s, id.serialize(b, s));
		shift(b, s, val.serialize(b, s));
		put<int>(b, s, req_count);
		assert(s==0);

		comm_->send(rank, TAG_POST, buf, size, [buf](){
			operator delete(buf);
		});
	}
}

void RTS::request(const Id &id, const Locator &loc,
	std::function<void (const DF &)> cb)
{
	int rank=loc.get_next_rank(*comm_);

	TRACE("REQUEST\t" + id.to_string() + "\t" + std::to_string(rank));

	if (rank==comm_->rank()) {
		_request(id, cb);
	} else {
		// Format: [id] [rCB]
		size_t size=id.get_serialization_size() + sizeof(void*);
		void *buf=operator new(size);

		void *b=buf; size_t s=size;

		shift(b, s, id.serialize(b, s));
		std::function<void (const DF &)> *fptr=nullptr;
		fptr=new std::function<void (const DF &)>(
				[cb, fptr](const DF &df){
			cb(df);
			delete fptr;
		});

		put<void*>(b, s, (void*)fptr);

		comm_->send(rank, TAG_REQUEST, buf, size, [buf](){
			operator delete(buf);
		});
	}
}

void RTS::destroy(const Id &id, const Locator &loc)
{
	int rank=loc.get_next_rank(*comm_);

	if (rank==comm_->rank()) {
		_destroy(id);
	} else {
		// Format: [id]
		size_t size=id.get_serialization_size();
		void *buf=operator new(size);
		size_t written=id.serialize(buf, size);
		assert(written==size);
		comm_->send(rank, TAG_DESTROY, buf, size, [buf](){
			operator delete(buf);
		});
	}
}

void RTS::_check_requests(const Id &id)
{
	std::unique_lock<std::mutex> lk(m_);

	auto pit=posts_.find(id);

	if (pit==posts_.end()) {
		return;
	}

	auto it=requests_.find(id);
	
	if (it==requests_.end()) {
		return;
	}

	std::vector<std::function<void (const DF &)> > reqs=it->second;

	int count=reqs.size();

	requests_.erase(it);

	DF val=pit->second.second;

	lk.unlock();

	for (auto f : reqs) {
		f(val);
	}

	lk.lock();

	auto it2=posts_.find(id);

	if (it2==posts_.end()) {
		return;
	}

	int &req_count=it2->second.first;

	if (req_count>=0 && count>req_count) {
		ABORT("Too many requests");
	}

	req_count-=count;

	if (req_count==0) {
		posts_.erase(it2);
	}
}

void RTS::push(const Id &dfid, const DF &val, const Id &cfid,
	const Locator &loc)
{
	int rank=loc.get_next_rank(*comm_);

	if (rank==comm_->rank()) {
		_push(dfid, val, cfid);
	} else {
		// Format dfid, val, cfid
		size_t size=dfid.get_serialization_size()
			+val.get_serialization_size()
			+cfid.get_serialization_size();
		void *buf=operator new(size);
		void *b=buf; size_t s=size;
		shift(b, s, dfid.serialize(b, s));
		shift(b, s, val.serialize(b, s));
		shift(b, s, cfid.serialize(b, s));
		assert(s==0);
		comm_->send(rank, TAG_PUSH, buf, size, [buf]() {
			operator delete(buf);
		});
	}
}

void RTS::expect_pushes(const Id &cfid,
	std::function<void (const Id &, const DF &)> cb)
{
	std::unique_lock<std::mutex> lk(m_);
	
	if (waiters_.find(cfid)!=waiters_.end()) {
		ABORT("Duplicate waiter");
	}

	waiters_[cfid]=cb;

	auto it=pushed_.find(cfid);

	if (it==pushed_.end()) {
		return;
	}

	auto pushed=it->second;
	pushed_.erase(it);

	lk.unlock();

	for (auto it : pushed) {
		cb(it.first, it.second);
	}
}

void RTS::unexpect_pushes(const Id &cfid)
{
	std::lock_guard<std::mutex> lk(m_);
	
	if (pushed_.find(cfid)!=pushed_.end()) {
		ABORT("Unexpecting pushes while pushed DFs present");
	}

	auto it=waiters_.find(cfid);

	if (it==waiters_.end()) {
		return;
	}

	waiters_.erase(it);
}

void RTS::steal_req(int tag, unsigned int proc_count)
{
    if(comm_->size() - 1 <= proc_count) {
        comm_->bcast(tag);
    } else {
        int rank = comm_->rank();
        for(int i = 0; i < proc_count; i++) {
            comm_->send((i+rank)%comm_->size(), tag, nullptr, 0);
        }
    }
}

int RTS::get_steal_req()
{
    std::lock_guard<std::mutex> lk(m_);
    if(pool_.has_jobs() && !steal_requests_.empty()) {
        int rank=*steal_requests_.begin();
        steal_requests_.erase(rank);
        return rank;
    }
    return -1;
}

std::string RTS::get_status() const
{
	std::ostringstream os;

	os << "IdleStopper: " << stopper_->to_string();

	return os.str();
}

void RTS::on_recv(int src, int tag, void *buf, size_t size)
{
	switch (tag) {
		case TAG_IDLE_MARKER:
			assert(size==sizeof(int));
			stopper_->receive(*static_cast<int*>(buf));
			operator delete(buf);
			break;
		case TAG_STOP: {
			std::lock_guard<std::mutex> lk(m_);
			assert(!finished_flag_);
			finished_flag_=true;
			cv_.notify_all();
			operator delete(buf);
			break;
		}
		case TAG_CF: {
			CF *cf=new CF(this);
			size_t read=cf->deserialize(buf, size);
			assert(read==size);
			LOG("Receive "+cf->to_string())
			submit(cf);
			operator delete(buf);
			break;
		}
		case TAG_POST: {
		// MSG format: [id] [df] [count]
			const void *b=buf; size_t s=size;
			Id id;
			shift(b, s, id.deserialize(b, s));

			DF val;
			shift(b, s, val.deserialize(b, s));

			int req_count=get<int>(b, s);

			assert(s==0);

			operator delete(buf);

			_post(id, val, req_count);
			break;
		}
		case TAG_REQUEST: {
		// Format: [id] [rCB]
			const void *b=buf; size_t s=size;
			Id id;
			shift(b, s, id.deserialize(b, s));

			void *rptr=get<void *>(b, s);

			operator delete(buf);

			_request(id, [rptr, src, this](const DF &df){
				// Format: [rCB] [df]
				size_t size=sizeof(void *)
					+df.get_serialization_size();
				void *buf=operator new(size);

				void *b=buf; size_t s=size;

				put<void *>(b, s, rptr);
				shift(b, s, df.serialize(b, s));
				assert(s==0);

				comm_->send(src, TAG_RESPONSE, buf, size, [buf](){
					operator delete(buf);
				});
			});

			break;
		}
		case TAG_RESPONSE: {
				// Format: [rCB] [df]
			const void *b=buf; size_t s=size;

			void *rptr=get<void *>(b, s);

			DF df;
			shift(b, s, df.deserialize(b, s));

			assert(s==0);

			operator delete(buf);

			std::function<void (const DF &)> *fptr=
				static_cast<std::function<void (const DF &)> *>(rptr);

			(*fptr)(df);

			break;
		}
		case TAG_DESTROY: {
			Id id;
			size_t read=id.deserialize(buf, size);
			assert(read==size);

			_destroy(id);
			operator delete(buf);
			break;
		}
		case TAG_PUSH: {
		// Format dfid, val, cfid
			Id dfid, cfid;
			DF val;
			const void *b=buf;
			size_t s=size;
			shift(b, s, dfid.deserialize(b, s));
			shift(b, s, val.deserialize(b, s));
			shift(b, s, cfid.deserialize(b, s));
			assert(s==0);
			_push(dfid, val, cfid);
			operator delete(buf);
			break;
		}
        case TAG_STEAL: {
            if(src==comm_->rank()){
                return;
            }
            std::lock_guard <std::mutex> lock(m_);
            LOG("Steal request from " + std::to_string(src))
            steal_requests_.insert(src);
            operator delete(buf);
            break;
        }
        case TAG_STEAL_REVOKE: {
            if(src==comm_->rank()){
                return;
            }
            std::lock_guard <std::mutex> lock(m_);
            LOG("Steal revoke request from " + std::to_string(src))
            steal_requests_.erase(src);
            operator delete(buf);
            break;
        }
		default:
			ABORT("Tag not implemented: " + std::to_string(tag));
	}
}

void RTS::_submit(CF *cf)
{
	pool_.submit([this, cf](){
		BlockRetStatus ret;
			do {
				LOG("Invoke " + cf->to_string())

				ret=(*fp_)[cf->NextBlock](*cf);

				// submit forked cfs:
				{
					std::lock_guard<std::mutex> lk(cf->m_);
					
					for (auto fcf : cf->forked_cfs_) {
						LOG("Create "+fcf->to_string())
						submit(fcf);
					}

					cf->forked_cfs_.clear();
				}

				switch (ret) {
					case EXIT:
						LOG("Finish "+cf->to_string())
						cf->self_.reset();
						break;
					case MIGRATE: {
						int dest_rank=cf->dest_rank_;
						LOG("Migrate "+cf->to_string()+" to "+std::to_string(dest_rank))
						assert(cf->dest_rank_!=comm_->rank());

						size_t size=cf->get_serialization_size();
						void *buf=operator new(size);
						size_t written=cf->serialize(buf, size);
						assert(written==size);
						cf->self_.reset();

						change_load(1);
						LOG("WORKLOAD SEND SENDING");
						comm_->send(dest_rank, TAG_CF, buf, size,
								[this, buf](){
							operator delete(buf);
							change_load(-1);
							LOG("WORKLOAD SEND SENT");
						});
						break;
					}
                    case STEAL: {
                        int dest_rank = cf->dest_rank_;
                        LOG("Stealing job " + cf->to_string() + " to " + std::to_string(dest_rank))
                        assert(dest_rank!=comm_->rank());

                        size_t size = cf->get_serialization_size();
                        void *buf = operator new(size);
                        size_t written = cf->serialize(buf, size);
                        assert(written == size);
                        cf->self_.reset();

                        change_load(1);
                        LOG("WORKLOAD SEND SENDING");
                        comm_->send(dest_rank, TAG_CF, buf, size,
                                    [this, buf]() {
                                        operator delete(buf);
                                        change_load(-1);
                                        LOG("WORKLOAD SEND SENT");
                                    });
                        break;
                    }
					case WAIT: {
						break;
					}
					case CONTINUE:
						break;
					default:
						ABORT("Unexpected block ret status: "
							+ std::to_string(ret));
				}
			} while (ret==CONTINUE);
	});
}

void RTS::_post(const Id &id, const DF &val, int req_count)
{
	std::unique_lock<std::mutex> lk(m_);

	LOG("POST " + id.to_string() + " := " + val.to_string() + " &"
		+ std::to_string(req_count))
	
	if (val.is_unset()) {
		ABORT("Posting an unset value, id: " + id.to_string());
	}

	if (posts_.find(id)!=posts_.end()) {
		ABORT("Duplicate id in post: " + id.to_string());
	}

	if (destroys_.find(id)!=destroys_.end()) {
		assert(requests_.find(id)==requests_.end());

		// do not actually post, just erase the destroy
		destroys_.erase(id);
		return;
	}

	posts_[id]=std::make_pair(req_count, val);

	lk.unlock();

	_check_requests(id);
}

void RTS::_request(const Id &id, std::function<void (const DF &)> cb)
{
	std::unique_lock<std::mutex> lk(m_);

	if (destroys_.find(id)!=destroys_.end()) {
		ABORT("Destroy present, but request appeared: " + id.to_string());
	}

	auto it=requests_.find(id);

	if (it==requests_.end()) {
		requests_[id]={};
		it=requests_.find(id);
	}

	it->second.push_back([cb](const DF &df){
		cb(df);
	});

	lk.unlock();

	_check_requests(id);
}

void RTS::_destroy(const Id &id)
{
	std::unique_lock<std::mutex> lk(m_);

	if (requests_.find(id)!=requests_.end()) {
		ABORT("Request(s) present at destroy");
	}

	auto it=posts_.find(id);

	if (it!=posts_.end()) {
		assert(it->second.first!=0);
		if (it->second.first>0) {
			ABORT("Attempting to destroy DF with positive requests count");
		}
		posts_.erase(it);
		LOG("Destroyed " + id.to_string());
	} else {
		if (destroys_.find(id)!=destroys_.end()) {
			ABORT("Duplicate destroy");
		}
		destroys_.insert(id);
	}
}

void RTS::_push(const Id &dfid, const DF &val, const Id &cfid)
{
	std::unique_lock<std::mutex> lk(m_);

	auto it=waiters_.find(cfid);
	if (it!=waiters_.end()) {
		lk.unlock();

		it->second(dfid, val);

		// BUG: there is a chance waiter will unexpect himself before
		// this callback will be invoked. To detect this we ensure that
		// after callback waiter is still there. But this is also wrong,
		// since a new waiter can apply himself before we check.
		// Although all this is unlikely to happen, this bug is better
		// to be fixed.
		lk.lock();
		assert(waiters_.find(cfid)!=waiters_.end());
	} else {
		auto it=pushed_.find(cfid);

		if (it==pushed_.end()) {
			pushed_.insert(std::make_pair(cfid,
				std::vector<std::pair<Id, DF> >()));
			it=pushed_.find(cfid);
		}

		it->second.push_back(std::make_pair(dfid, val));
	}
}

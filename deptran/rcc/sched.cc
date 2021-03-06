
#include "sched.h"
#include "../txn_chopper.h"
#include "commo.h"
#include "dtxn.h"
#include "frame.h"
#include "graph.h"
#include "txn-info.h"
#include "waitlist_checker.h"
#include <bench/tpcc/piece.h>
#include <server_worker.h>

namespace rococo {

map<txnid_t, int32_t> RccSched::__debug_xxx_s{};
std::recursive_mutex RccSched::__debug_mutex_s{};
void RccSched::__DebugCheckParentSetSize(txnid_t tid, int32_t sz) {
#ifdef DEBUG_CODE
    std::lock_guard<std::recursive_mutex> guard(__debug_mutex_s);
    if (__debug_xxx_s.count(tid) > 0) {
        auto s = __debug_xxx_s[tid];
        verify(s == sz);
    } else {
        __debug_xxx_s[tid] = sz;
    }
#endif
}

RccSched::RccSched() : Scheduler(), waitlist_(), mtx_() {
    RccGraph::sched_ = this;
    RccGraph::partition_id_ = Scheduler::partition_id_;
    RccGraph::managing_memory_ = false;
    waitlist_checker_ = new WaitlistChecker(this);
    epoch_enabled_ = true;
}

RccSched::~RccSched() {
    verify(waitlist_checker_);
    delete waitlist_checker_;
}

DTxn *RccSched::GetOrCreateDTxn(txnid_t tid, bool ro) {
    RccDTxn *dtxn = (RccDTxn *) Scheduler::GetOrCreateDTxn(tid, ro);
    dtxn->partition_.insert(Scheduler::partition_id_);
    verify(dtxn->id() == tid);
    return dtxn;
}

int RccSched::OnDispatch(const vector<SimpleCommand> &cmd,
                         int32_t *res,
                         TxnOutput *output,
                         RccGraph *graph,
                         const function<void()> &callback) {
    std::lock_guard<std::recursive_mutex> guard(mtx_);
    verify(graph != nullptr);
    txnid_t txn_id = cmd[0].root_id_;
    RccDTxn *dtxn = (RccDTxn *) GetOrCreateDTxn(txn_id);
    verify(dtxn->id() == txn_id);
    verify(RccGraph::partition_id_ == Scheduler::partition_id_);
    //  auto job = [&cmd, res, dtxn, callback, graph, output, this, txn_id] () {
    verify(cmd[0].partition_id_ == Scheduler::partition_id_);
    for (auto &c : cmd) {
        dtxn->DispatchExecute(c, res, &(*output)[c.inn_id()]);
    }
    dtxn->UpdateStatus(TXN_STD);
    int depth = 1;
    verify(cmd[0].root_id_ == txn_id);
    auto sz = MinItfrGraph(dtxn, graph, true, depth);
//#ifdef DEBUG_CODE
//    if (sz > 4) {
//      Log_fatal("something is wrong, graph size %d", sz);
//    }
//#endif
#ifdef DEBUG_CODE
    if (sz > 0) {
        RccDTxn &info1 = dep_graph_->vertex_index_.at(cmd[0].root_id_)->Get();
        RccDTxn &info2 = graph->vertex_index_.at(cmd[0].root_id_)->Get();
        verify(info1.partition_.find(cmd[0].partition_id_) != info1.partition_.end());
        verify(info2.partition_.find(cmd[0].partition_id_) != info2.partition_.end());
        verify(sz > 0);
        if (RandomGenerator::rand(1, 2000) <= 1)
            Log_debug("dispatch ret graph size: %d", graph->size());
    }
#endif
    callback();
    //  };
    //
    //  static bool do_record = Config::GetConfig()->do_logging();
    //  if (do_record) {
    //    Marshal m;
    //    m << cmd;
    //    recorder_->submit(m, job);
    //  } else {
    //    job();
    //  }
}

int RccSched::OnCommit(cmdid_t cmd_id,
                       const RccGraph &graph,
                       TxnOutput *output,
                       const function<void()> &callback) {
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    // union the graph into dep graph
    RccDTxn *dtxn = (RccDTxn *) GetDTxn(cmd_id);
    verify(dtxn != nullptr);
    verify(dtxn->ptr_output_repy_ == nullptr);
    dtxn->ptr_output_repy_ = output;
    dtxn->finish_reply_callback_ = [callback](int r) { callback(); };
    Aggregate(epoch_mgr_.curr_epoch_, const_cast<RccGraph &>(graph));
    TriggerCheckAfterAggregation(const_cast<RccGraph &>(graph));

    //  // fast path without check wait list?
    //  if (graph.size() == 1) {
    //    auto v = dep_graph_->FindV(cmd_id);
    //    if (v->incoming_.size() == 0);
    //    Execute(v->Get());
    //    return 0;
    //  } else {
    //    Log_debug("graph size on commit, %d", (int) graph.size());
    ////    verify(0);
    //  }
}

int RccSched::OnInquire(epoch_t epoch,
                        txnid_t txn_id,
                        RccGraph *graph,
                        const function<void()> &callback) {
    std::lock_guard<std::recursive_mutex> guard(mtx_);

    verify(0);
    RccDTxn *v = FindV(txn_id);
    verify(v != nullptr);
    RccDTxn &dtxn = *v;
    //register an event, triggered when the status >= COMMITTING;
    verify(dtxn.Involve(Scheduler::partition_id_));

    if (dtxn.status() >= TXN_CMT) {
        InquiredGraph(dtxn, graph);
        callback();
    } else {
        dtxn.graphs_for_inquire_.push_back(graph);
        dtxn.callbacks_for_inquire_.push_back(callback);
        verify(dtxn.graphs_for_inquire_.size() ==
               dtxn.callbacks_for_inquire_.size());
    }
}

void RccSched::InquiredGraph(RccDTxn &dtxn, RccGraph *graph) {
    verify(graph != nullptr);
    if (dtxn.IsDecided()) {
        // return scc is enough.
        auto &scc = FindSccPred(&dtxn);
        for (auto v : scc) {
            RccDTxn *vv = graph->CreateV(*v);
            verify(vv->parents_.size() == v->parents_.size());
            verify(vv->partition_.size() == v->partition_.size());
        }
    } else {
        MinItfrGraph(&dtxn, graph, false, 1);
    }
}

void RccSched::AnswerIfInquired(RccDTxn &dtxn) {
    // reply inquire requests if possible.
    auto sz1 = dtxn.graphs_for_inquire_.size();
    auto sz2 = dtxn.callbacks_for_inquire_.size();
    if (sz1 != sz2) {
        Log_fatal("graphs for inquire sz %d, callbacks sz %d",
                  (int) sz1, (int) sz2);
    }
    if (dtxn.status() >= TXN_CMT && dtxn.graphs_for_inquire_.size() > 0) {
        for (RccGraph *graph : dtxn.graphs_for_inquire_) {
            InquiredGraph(dtxn, graph);
        }
        for (auto &callback : dtxn.callbacks_for_inquire_) {
            verify(callback);
            callback();
        }
        dtxn.callbacks_for_inquire_.clear();
        dtxn.graphs_for_inquire_.clear();
    }
}

void RccSched::CheckWaitlist() {
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    Log_debug("%s called", __FUNCTION__);
    auto it = waitlist_.begin();

    int kunk = 0;
    while (it != waitlist_.end()) {
        RccDTxn *v = *it;
        verify(v != nullptr);
        RccDTxn &dtxn = *v;
        Log_debug("checking loop, checking txn %lu, status = %hhd, is executed? %d. involved = %d,  waitlist length: %d, commit status for TXN_DCD is %hhd",
                 dtxn.id(), dtxn.status(), dtxn.IsExecuted(), dtxn.Involve(Scheduler::partition_id_), (int) waitlist_.size(), TXN_DCD);
        InquireAboutIfNeeded(dtxn);// inquire about unknown transaction.
        if (dtxn.status() >= TXN_CMT &&
            !dtxn.IsExecuted()) {

            //debug code
            for (auto &p : dtxn.parents_) {
                Log_debug("txn %lu's parent %lu", dtxn.id(), p);
            }


            if (AllAncCmt(v)) {
                RccScc &scc = FindSccPred(v);
                Log_debug("Txn %lu's ancestor are all committed, scc size = %d", dtxn.id(), scc.size());
                verify(v->epoch_ > 0);
                Decide(scc);
                if (AllAncFns(scc)) {
                    if (FullyDispatched(scc)) {
                        if (!ExecuteScc(scc, kunk)){
                            kunk++;
                        }
                        for (auto v : scc) {
                            //              AddChildrenIntoWaitlist(v);
                        }
                    } else {
                        Log_debug("Txn %lu's scc are not funlly dispatched, scc size = %d", dtxn.id(), scc.size());
                    }
                    //        check_again = true;
                } else {
                    Log_debug("Txn %lu's ancestor not finished", dtxn.id());
                }// else do nothing.
            } else {
                Log_debug("Txn %lu's ancestor not committed", dtxn.id());
                // else do nothing
            }
            AnswerIfInquired(dtxn);
        } else {
            Log_debug("Txn %lu is not committed or alraedy executed", dtxn.id());
        }// else do nothing

        // Adjust the waitlist.
        if (dtxn.IsExecuted() ||
            dtxn.IsAborted() ||
            (
             !dtxn.Involve(Scheduler::partition_id_))) {
            // TODO? check for its descendants, perhaps
            // add redundant vertex here?
            //      AddChildrenIntoWaitlist(v);
            for (auto &v : dtxn.to_checks_) {
                waitlist_.insert(v);
            }
            dtxn.to_checks_.clear();
            it = waitlist_.erase(it);
            Log_debug("removing txn %lu", dtxn.id());
        } else {
            it++;
        }
        //        it = waitlist_.erase(it);
    }
}

void RccSched::AddChildrenIntoWaitlist(RccDTxn *v) {
    verify(0);
    //  RccDTxn& tinfo = *v;
    //  verify(tinfo.status() >= TXN_DCD && tinfo.IsExecuted());
    //
    //  for (auto& child_pair : v->outgoing_) {
    //    RccDTxn* child_v = child_pair.first;
    //    if (!child_v->IsExecuted() &&
    //        waitlist_.count(child_v) == 0) {
    //      waitlist_.insert(child_v);
    //      verify(child_v->epoch_ > 0);
    //    }
    //  }
    //  for (RccDTxn* child_v : v->removed_children_) {
    //    if (!child_v->IsExecuted() &&
    //        waitlist_.count(child_v) == 0) {
    //      waitlist_.insert(child_v);
    //      verify(child_v->epoch_ > 0);
    //    }
    //  }
    //#ifdef DEBUG_CODE
    //    fridge_.erase(v);
    //#endif
}

void RccSched::__DebugExamineGraphVerify(RccDTxn *v) {
#ifdef DEBUG_CODE
    for (auto &pair : v->incoming_) {
        verify(pair.first->outgoing_.count(v) > 0);
    }

    for (auto &pair : v->outgoing_) {
        verify(pair.first->incoming_.count(v) > 0);
    }
#endif
}

void RccSched::__DebugExamineFridge() {
#ifdef DEBUG_CODE
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    int in_ask = 0;
    int in_wait_self_cmt = 0;
    int in_wait_anc_exec = 0;
    int in_wait_anc_cmt = 0;
    int64_t id = 0;
    for (RccVertex *v : fridge_) {
        RccDTxn &tinfo = v->Get();
        if (tinfo.status() >= TXN_CMT) {
            verify(tinfo.graphs_for_inquire_.size() == 0);
        }
        if (tinfo.status() >= TXN_CMT) {
            if (AllAncCmt(v)) {
                //        verify(!tinfo.IsExecuted());
                //        verify(tinfo.IsDecided() || tinfo.IsAborted());
                if (!tinfo.IsExecuted()) {
                    in_wait_anc_exec++;
                } else {
                    // why is this still in fridge?
                    verify(0);
                }
            } else {
                //        RccScc& scc = dep_graph_->FindSCC(v);
                //        verify(!AllAncFns(scc));
                in_wait_anc_cmt++;
            }
        }
        //    Log_debug("my partition: %d", (int) partition_id_);
        if (tinfo.status() < TXN_CMT) {
            if (tinfo.Involve(partition_id_)) {
                in_wait_self_cmt++;
                id = tinfo.id();
            } else {
                verify(tinfo.during_asking);
                in_ask++;
            }
        }
    }
    int sz = (int) fridge_.size();
    Log_debug("examining fridge. fridge size: %d, in_ask: %d, in_wait_self_cmt: %d"
             " in_wait_anc_exec: %d, in wait anc cmt: %d, else %d",
             sz, in_ask, in_wait_self_cmt, in_wait_anc_exec, in_wait_anc_cmt,
             sz - in_ask - in_wait_anc_exec - in_wait_anc_cmt - in_wait_self_cmt);
//  Log_debug("wait for myself commit: %llx par: %d", id, (int)partition_id_);
#endif
}

void RccSched::InquireAboutIfNeeded(RccDTxn &dtxn) {
    //  Graph<RccDTxn> &txn_gra = dep_graph_->txn_gra_;
    if (dtxn.status() <= TXN_STD &&
        !dtxn.during_asking &&
        !dtxn.Involve(Scheduler::partition_id_)) {
        verify(!dtxn.Involve(Scheduler::partition_id_));
        verify(!dtxn.during_asking);
        parid_t par_id = *(dtxn.partition_.begin());
        dtxn.during_asking = true;
        commo()->SendInquire(par_id,
                             dtxn.epoch_,
                             dtxn.tid_,
                             std::bind(&RccSched::InquireAck,
                                       this,
                                       dtxn.id(),
                                       std::placeholders::_1));
    }
}


void RccSched::InquireAck(cmdid_t cmd_id, RccGraph &graph) {
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    auto v = FindV(cmd_id);
    verify(v != nullptr);
    RccDTxn &tinfo = *v;
    tinfo.inquire_acked_ = true;
    Aggregate(epoch_mgr_.curr_epoch_, const_cast<RccGraph &>(graph));
    TriggerCheckAfterAggregation(const_cast<RccGraph &>(graph));
    verify(tinfo.status() >= TXN_CMT);
}

RccDTxn *RccSched::__DebugFindAnOngoingAncestor(RccDTxn *vertex) {
    RccDTxn *ret = nullptr;
    set<RccDTxn *> walked;
    std::function<bool(RccDTxn *)> func = [&ret, vertex](RccDTxn *v) -> bool {
        RccDTxn &info = *v;
        if (info.status() >= TXN_CMT) {
            return true;
        } else {
            ret = v;
            return false;// stop traversing.
        }
    };
    TraversePred(vertex, -1, func, walked);
    return ret;
}

bool RccSched::AllAncCmt(RccDTxn *vertex) {
    bool all_anc_cmt = true;
    std::function<int(RccDTxn *)> func =
            [&all_anc_cmt, vertex](RccDTxn *v) -> int {
        RccDTxn &parent = *v;
        int r = 0;
        if (parent.IsExecuted() || parent.IsAborted()){
            r = RccGraph::SearchHint::Skip;
        } else if (parent.involve_flag_ != RccDTxn::INVOLVED){
            Log_debug("check ancestor %lu commietted", parent.id());
            if (parent.checkInquire() == true){
                r = RccGraph::SearchHint::Skip;
            }else{
                r = RccGraph::SearchHint::Exit;
                parent.to_checks_.insert(vertex);
                all_anc_cmt = false;
            }
        }
        else if (parent.status() >= TXN_CMT) {
            r = RccGraph::SearchHint::Ok;
        } else {
            Log_debug("ancestor %lu of %lu not committed", parent.id(), vertex->id());
            r = RccGraph::SearchHint::Exit;
            parent.to_checks_.insert(vertex);

            all_anc_cmt = false;
        }
        return r;
    };
    TraversePred(vertex, -1, func);
    return all_anc_cmt;
}

void RccSched::Decide(const RccScc &scc) {
    for (auto v : scc) {
        RccDTxn &info = *v;
        UpgradeStatus(v, TXN_DCD);
        //    Log_debug("txnid: %llx, parent size: %d", v->id(), v->parents_.size());
    }
}

bool RccSched::HasICycle(const RccScc &scc) {
    for (auto &vertex : scc) {
        set<RccDTxn *> walked;
        bool ret = false;
        std::function<bool(RccDTxn *)> func =
                [&ret, vertex](RccDTxn *v) -> bool {
            if (v == vertex) {
                ret = true;
                return false;
            }
            return true;
        };
        TraverseDescendant(vertex, -1, func, walked, EDGE_I);
        if (ret) return true;
    }
    return false;
};

void RccSched::TriggerCheckAfterAggregation(RccGraph &graph) {
    bool check = false;
    for (auto &pair : graph.vertex_index()) {
        // TODO optimize here.
        auto txnid = pair.first;
        RccDTxn *rhs_v = pair.second;
        auto v = FindV(txnid);
        verify(v != nullptr);
        check = true;
        //insert all deps into wait list
        waitlist_.insert(v);
        verify(v->epoch_ > 0);
    }
    if (check)
        CheckWaitlist();
}


bool RccSched::HasAbortedAncestor(const RccScc &scc) {
    verify(scc.size() > 0);
    bool has_aborted = false;
    std::function<int(RccDTxn *)> func =
            [&has_aborted](RccDTxn *v) -> int {
        RccDTxn &info = *v;
        if (info.IsExecuted()) {
            return RccGraph::SearchHint::Skip;
        }
        if (info.IsAborted()) {
            has_aborted = true;               // found aborted transaction.
            return RccGraph::SearchHint::Exit;// abort traverse
        }
        return RccGraph::SearchHint::Ok;
    };
    TraversePred(scc[0], -1, func);
    return has_aborted;
};

bool RccSched::FullyDispatched(const RccScc &scc) {
    bool ret = std::all_of(scc.begin(),
                           scc.end(),
                           [this](RccDTxn *v) {
                               bool r = true;
                               RccDTxn &tinfo = *v;
                               if (tinfo.Involve(Scheduler::partition_id_)) {
                                   r = tinfo.fully_dispatched;
                               }
                               return r;
                           });
    return ret;
}

bool RccSched::AllAncFns(const RccScc &scc) {
    verify(scc.size() > 0);
    set<RccDTxn *> scc_set;
    scc_set.insert(scc.begin(), scc.end());
    bool all_anc_fns = true;
    std::function<int(RccDTxn *)> func =
            [&all_anc_fns, &scc_set, &scc](RccDTxn *v) -> int {
        RccDTxn &info = *v;
        if (info.IsExecuted()) {
            info.executed_ = true;
            return RccGraph::SearchHint::Skip;
        } else if (info.involve_flag_ != RccDTxn::INVOLVED){
            Log_debug("%s checking not involved txn %lu, flag = %d, partition size = %lu" , __FUNCTION__ ,  info.id(), info.involve_flag_, info.partition_.size());
            if (info.checkInquire()){
                info.executed_ = true;
                Log_debug("%s not involved txn %lu, checked" , __FUNCTION__ ,  info.id());
                return RccGraph::SearchHint::Skip;
            }else{
                all_anc_fns = false;
                return RccGraph::SearchHint::Exit;// abort traverse
            }
        } else if (info.status() >= TXN_DCD) {
            return RccGraph::SearchHint::Ok;
        } else if (scc_set.find(v) != scc_set.end()) {
            return RccGraph::SearchHint::Ok;
        } else {
            Log_debug("%lu not finished", info.id());
            all_anc_fns = false;
            info.to_checks_.insert(scc[0]);
            return RccGraph::SearchHint::Exit;// abort traverse
        }
    };
    TraversePred(scc[0], -1, func);
    return all_anc_fns;
};

bool RccSched::ExecuteScc(const RccScc &scc, int kunk) {
    verify(scc.size() > 0);
    bool is_spec = false;

    for (auto v : scc) {
        RccDTxn &dtxn = *v;


        TryExecReadyPieces(dtxn, is_spec);
        if (dtxn.IsExecuted()) {
            verify(dtxn.n_executed_pieces == dtxn.n_local_pieces);

            if (dtxn.IsExecuted()) {
                auto it = std::find(key_not_known_txns_.begin(), key_not_known_txns_.end(), v);
                if (it != key_not_known_txns_.end()) {
                    key_not_known_txns_.erase(it);
                    Log_debug("removing txn %lu from key not knwon txns, size %lu", dtxn.id(), key_not_known_txns_.size());
                }
            }

            Log_debug("%s txn %lu execution finished", __FUNCTION__, dtxn.id());
        } else if (dtxn.involve_flag_ != RccDTxn::INVOLVED) {
            Log_debug("%s txn %lu execution not involved", __FUNCTION__, dtxn.id());
            dtxn.executed_ = true;
        } else {
            Log_debug("%s txn %lu execution not finished, breaking executing scc of size %d", __FUNCTION__, dtxn.id(), scc.size());
//            break;
            is_spec = true;
        }
    }
    return !is_spec;
}

void RccSched::TryExecReadyPieces(RccDTxn &dtxn, bool is_spec) {
    Log_debug("%s called for txn %lu, phase = %d, is_spec = %d", __PRETTY_FUNCTION__, dtxn.id(), dtxn.phase_, is_spec);
    verify(dtxn.IsDecided());
    verify(dtxn.epoch_ > 0);
    if (dtxn.Involve(Scheduler::partition_id_)) {
        auto output_to_send = dtxn.CommitExecAllRdy(Scheduler::partition_id_);


        for (auto &pair : output_to_send) {
            parid_t target_par = pair.first;
            if (target_par != Scheduler::partition_id_) {
                Log_debug("Sending output to partition %u, size = %d, phase = %d",
                         target_par,
                         pair.second.size(),
                         dtxn.phase_);
                ChronosSendOutputReq chr_req;
                chr_req.var_values = pair.second;
                chr_req.txn_id = dtxn.id();
                dynamic_cast<BrqCommo *>(commo())->SendOutput(target_par, chr_req);
            }
        }

        if (is_spec){
            return;
        }

        if (dtxn.n_local_pieces == dtxn.n_executed_pieces) {
            //only if all transactions are executed.
            Log_debug("Execution of txn %lu finishes", dtxn.id());
            dtxn.executed_ = true;
            dtxn.ReplyFinishOk();
            TrashExecutor(dtxn.tid_);
        } else {
            Log_debug("Execution of txn %lu does not finish, type %u, to_checks size = %d", dtxn.id(), dtxn.dreqs_[0].root_type_, dtxn.to_checks_.size());
        }
    }
}

void RccSched::Abort(const RccScc &scc) {
    verify(0);
    verify(scc.size() > 0);
    for (auto v : scc) {
        RccDTxn &info = *v;
        info.union_status(TXN_ABT);// FIXME, remove this.
        verify(info.IsAborted());
        RccDTxn *dtxn = (RccDTxn *) GetDTxn(info.id());
        if (dtxn == nullptr) continue;
        if (info.Involve(Scheduler::partition_id_)) {
            dtxn->Abort();
            dtxn->ReplyFinishOk();
        }
    }
}

RccCommo *RccSched::commo() {
    //  if (commo_ == nullptr) {
    //    verify(0);
    //  }
    auto commo = dynamic_cast<RccCommo *>(commo_);
    verify(commo != nullptr);
    return commo;
}

void RccSched::DestroyExecutor(txnid_t txn_id) {
    RccDTxn *dtxn = (RccDTxn *) GetOrCreateDTxn(txn_id);
    verify(dtxn->committed_ || dtxn->aborted_);
    if (epoch_enabled_) {
        //    Remove(txn_id);
        DestroyDTxn(txn_id);
    }
}

}// namespace rococo

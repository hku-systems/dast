//
// Created by tyycxs on 2020/5/31.
//
#include "auxiliary_raft.h"
#include "__dep__.h"
#include "benchmark_control_rpc.h"
#include "brq/sched.h"
#include "classic/sched.h"
#include "command.h"
#include "command_marshaler.h"
#include "config.h"
#include "rcc/dep_graph.h"
#include "rcc/sched.h"
#include "rcc_service.h"
#include "scheduler.h"
#include "tapir/sched.h"
#include "txn_chopper.h"

namespace rococo {

void AuxiliaryRaftImpl::SlogRaftSubmit(const std::map<uint32_t, std::vector<SimpleCommand>> &cmds_by_par,
                                       const siteid_t &handler_site,
                                       int32_t *res,
                                       rrr::DeferredReply *defer) {
    std::lock_guard<std::recursive_mutex> guard(mu_);
    Log_debug("%s called for txn %lu", __FUNCTION__, cmds_by_par.begin()->second.at(0).id_);

    logEntry *e = new logEntry();

    e->cmds_ = cmds_by_par;
    e->committed_ = false;
    e->coord_site_ = handler_site;
    e->start_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    log_.push_back(e);
    size_t index = log_.size() - 1;

    std::function<void(Future *)> cb =
            [=](Future *fu) {
                int ack_res;
                fu->get_reply() >> ack_res;
                RaftAppendAck(index);
            };
    rrr::FutureAttr fuattr;
    fuattr.callback = cb;

    for (int i = 1; i < raft_commo_->raft_proxies_.size(); i++) {
        Log_debug("raft leader sending to proxy %d", i);
        Future::safe_release(raft_commo_->raft_proxies_[i]->async_RaftAppendEntries(cmds_by_par, fuattr));
    }
    defer->reply();
    Log_debug("%s returned", __FUNCTION__);
}

void AuxiliaryRaftImpl::RaftAppendAck(size_t index){

    Log_debug("%s called on index %lu", __FUNCTION__, index);

    std::lock_guard<std::recursive_mutex> guard(mu_);
    verify(log_.size() > index);
    if (log_[index]->committed_ == false){
       log_[index]->committed_ = true;

       for (int i = next_send_index_; i < log_.size(); i++){
           if (log_[i]->committed_ == true){
               verify(log_[i]->sent == false);
               //send
               SendToAllPars(i);
               log_[i]->sent = true;
               next_send_index_++;
           }else{
               break;
           }
       }
    }
//    if (committed_indices_.count(index) == 0) {
//        //only one ack is enough
//        committed_indices_.insert(index);
//        Log_debug("index %lu committed, going to send to all par leaders", index);
//        std::vector<parid_t> touched_pars;
//        for (auto &pair : cmds_by_par) {
//            touched_pars.push_back(pair.first);
//        }
//    }
    Log_debug("%s returned", __FUNCTION__);
}

//void AuxiliaryRaftImpl::SendToAllLeaders (){
//
//}


void AuxiliaryRaftImpl::SendToAllPars(size_t log_index){
    std::lock_guard<std::recursive_mutex> guard(mu_);

    int64_t now =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    Log_debug("Raft leader sending txn %lu (index %lu of log) to all peers, elapsed time =%lu ms",  log_[log_index]->cmds_.begin()->second.at(0).id_, log_index,  now-log_[log_index]->start_time_);


    txnid_t txn_id = log_[log_index]->cmds_.begin()->second.at(0).id_;
    auto & cmds_by_par = log_[log_index]->cmds_;
    siteid_t coord_site = log_[log_index]->coord_site_;

    map<regionid_t, map<parid_t, vector<SimpleCommand>>> cmds_by_region;
    for (auto &pair: cmds_by_par) {
        parid_t par_id = pair.first;
        regionid_t rid = par_id / Config::GetConfig()->n_shard_per_region_;
        cmds_by_region[rid][par_id] = pair.second;
    }


    //by region instead of by par
    for (auto &pair : raft_commo_->slog_rpc_region_managers_){
        siteid_t target_site = pair.first;
        if (target_site % (Config::GetConfig()->n_shard_per_region_ * N_REP_PER_SHARD) == 0){
            //only to the leader
            regionid_t rid = target_site / (N_REP_PER_SHARD * Config::GetConfig()->n_shard_per_region_);
            if (cmds_by_region.count(rid) != 0){
                auto proxy = pair.second;
                Log_debug("sending pieces (map size = %d) to site %hu of region %u, index = %lu",
                         cmds_by_region.at(rid).size(),
                         target_site,
                         rid,
                         log_index);
                Future::safe_release(proxy->async_SlogSendOrderedCRT(cmds_by_region[rid], log_index, coord_site));
            }
            else{
                auto skip = map<parid_t, vector<SimpleCommand>>{};
                auto proxy = pair.second;
                Log_debug("sending skip message to site %hu of region %u, index = %lu",
                         target_site,
                         rid,
                         log_index);
                Future::safe_release(proxy->async_SlogSendOrderedCRT(skip, log_index, coord_site));
            }
        }
    }
};

void AuxiliaryRaftImpl::RaftAppendEntries(const std::map<uint32_t, std::vector<SimpleCommand>> &cmds_by_par,
                                          rrr::i32 *res,
                                          rrr::DeferredReply *defer) {
    std::lock_guard<std::recursive_mutex> guard(mu_);
    txnid_t txn_id = cmds_by_par.begin()->second[0].root_id_;
    Log_debug("%s called for id %lu", __FUNCTION__, txn_id);
    logEntry *e = new logEntry();
    e->cmds_ = cmds_by_par;
    e->committed_ = false;
    log_.push_back(e);

    *res = SUCCESS;
    defer->reply();
}
}// namespace rococo
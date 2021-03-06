//
// Created by micha on 2020/3/23.
//

#include "scheduler.h"
#include "commo.h"
#include "deptran/chronos/tx.h"
#include "deptran/frame.h"
#include <climits>
#include <limits>

//Thos XXX_LOG is defined in __dep__.h


using namespace rococo;

class Frame;

//chr_ts_t SchedulerChronos::CalculateAnticipatedTs(const chr_ts_t &src_ts, const chr_ts_t &recv_ts) {
//    std::lock_guard<std::recursive_mutex> guard(mtx_);
//    chr_ts_t ret;
//
//#ifdef USE_PHY_EST
//    auto now = std::chrono::system_clock::now();
//    int64_t
//            now_ts = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
//
//    int64_t estimated_rtt = (now_ts - src_ts.timestamp_) * 2;
//    Log_debug("logged rtt =%ld=, recv_ts = %s, src_ts = %s, now_ts = %ld", estimated_rtt, recv_ts.to_string().c_str(),
//              src_ts.to_string().c_str(), now_ts);
//    //  if  (estimated_rtt < 80 * 1000 || estimated_rtt > 150 * 1000){
//    //< 100ms, TC not added
//    estimated_rtt = 90 * 1000;
//    //  }
//    Log_debug("estimated rtt =%ld=, recv_ts = %s, src_ts = %s, now_ts = %ld", estimated_rtt,
//              recv_ts.to_string().c_str(), src_ts.to_string().c_str(), now_ts);
//    ret.timestamp_ =
//            now_ts + estimated_rtt;
////  ret.timestamp_ =
////      recv_ts.timestamp_ + 2 * (recv_ts.timestamp_ - src_ts.timestamp_) + 50 * 1000; //+100 because no TC added for now
//#else//USE_PHY_EST
//    int64_t estimated_rtt = (recv_ts.timestamp_ - src_ts.timestamp_) * 2;
//    Log_info("logged rtt =%ld=, recv_ts = %s, src_ts = %s, now_ts = %ld", estimated_rtt, recv_ts.to_string().c_str(), src_ts.to_string().c_str());
//    if (estimated_rtt < 80 * 1000 || estimated_rtt > 150 * 1000) {
//        //< 100ms, TC not added
//        estimated_rtt = 100 * 1000;
//    }
//    Log_info("estimated rtt =%ld=, recv_ts = %s, src_ts = %s, now_ts = %ld", estimated_rtt, recv_ts.to_string().c_str(), src_ts.to_string().c_str());
//    ret.timestamp_ =
//            recv_ts.timestamp_ + estimated_rtt;
//
//
//#endif//USE_PHY_EST
//
//    ret.stretch_counter_ = 0;
//    ret.site_id_ = recv_ts.site_id_;
//
//    if (!(ret > max_future_ts_)) {
//        ret = max_future_ts_;
//        ret.timestamp_++;
//    }
//    max_future_ts_ = ret;
//
//
//
//    return ret;
//}

SchedulerChronos::SchedulerChronos(Frame *frame) : Scheduler() {
    this->frame_ = frame;
    auto config = Config::GetConfig();
    for (auto &site : config->SitesByPartitionId(frame->site_info_->partition_id_)) {
        if (site.id != frame->site_info_->id) {
            local_replicas_.insert(site.id);
            //      local_replicas_ts_[site.id] = chr_ts_t();
            //      local_replicas_clear_ts_[site.id] = chr_ts_t();
            //      notified_txn_ts[site.id] = chr_ts_t();
            Log_info("created timestamp for local replica for partition %u,  site_id = %hu",
                     frame->site_info_->partition_id_,
                     site.id);
        }
    }

    for (auto &site : config->SitesInMyRegion(frame->site_info_->id)) {
        if (site.id != frame->site_info_->id) {
            ir_site_max_ts[site.id] = chr_ts_t();
            ir_synced[site.id] = false;
            Log_info("Set max_ts entry for intra-region site %hu, ts = %s",
                     site.id,
                     ir_site_max_ts[site.id].to_string().c_str());
        }
    }

    ir_sync_interval_ms_ = config->chronos_local_sync_interval_ms_;
    if (ir_sync_interval_ms_ != -1) {
        ir_sync_loop_thread_ = std::thread(&SchedulerChronos::IRSyncLoop, this);
    }

    // All shard have the same number of replicas
    verify(!config->replica_groups_.empty());
    n_replicas = config->replica_groups_[0].replicas.size();

    sites_per_region_ = Config::GetConfig()->n_shard_per_region_ * N_REP_PER_SHARD;

    Log_info("Created scheduler for site %hu, n_replicas in each site %u, region %u, sites_per_region = %u",
             site_id_,
             n_replicas,
             my_rid_,
             sites_per_region_);
}

void SchedulerChronos::IRSyncLoop() {
    while (true) {
        std::unique_lock<std::recursive_mutex> lk(mtx_);
        if (commo_ != nullptr) {
            SYNC_LOG("local_replicas_ts_size = %d", this->local_replicas_ts_.size());
            for (auto &s : this->ir_synced) {
                if (s.second == false) {
                    siteid_t target_site = s.first;
                    DastIRSyncReq req;
                    CollectNotifyTxns(target_site, target_site / N_REP_PER_SHARD, req.my_txns);
                    req.delivered_ts = this->ir_site_max_ts[target_site];
                    req.my_site = this->site_id_;
                    req.txn_ts = GenerateChrTs(true);
                    commo()->SendIRSync(target_site, req);
                }
                //set the flag back to false;
                s.second = false;
            }
        }
        lk.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds{ir_sync_interval_ms_});
    }
}

int SchedulerChronos::OnSubmitTxn(const map<uint32_t, vector<SimpleCommand>> &cmds_by_par,
                                  const ChronosSubmitReq &chr_req,
                                  const int32_t &is_irt,
                                  ChronosSubmitRes *chr_res,
                                  TxnOutput *output,
                                  const function<void()> &callback) {


    std::lock_guard<std::recursive_mutex> guard(mtx_);

    Log_debug("%s called, is_irt = %d", __FUNCTION__, is_irt);

    //Separate the two cases for readability.
    if (is_irt == 0) {
        //        HandleCRT(cmds_by_par, chr_req, chr_res, output, callback);
    } else {
        HandleIRT(cmds_by_par, chr_req, chr_res, output, callback);
    }
}


int SchedulerChronos::HandleIRT(const map<parid_t, vector<SimpleCommand>> &cmds_by_par,
                                const ChronosSubmitReq &chr_req,
                                ChronosSubmitRes *chr_res,
                                TxnOutput *output,
                                const function<void()> &reply_callback) {


    std::lock_guard<std::recursive_mutex> guard(mtx_);

    verify(!cmds_by_par.empty());
    verify(!cmds_by_par.begin()->second.empty());

    txnid_t txn_id = cmds_by_par.begin()->second.begin()->root_id_;//should have the same root_id
    Log_debug("++++ %s called for txn id = %lu", __FUNCTION__, txn_id);

    // For debug
//    for (auto &item : cmds_by_par) {
//        for (auto &c : item.second) {
//            Log_info("piece for par %u, piece type %d, input ready = %d, map size = %d",
//                     item.first,
//                     c.inn_id(),
//                     c.input.piece_input_ready_,
//                     c.waiting_var_par_map.size());
//
//            if (c.type_ == 205 &&
//                c.input.input_var_ready_.count(1007) != 0) {
//                Log_info("PAYMENT_CID READY = %d", c.input.input_var_ready_.at(1007));
//            }
//
//            for (auto &pair : c.waiting_var_par_map) {
//                for (auto &p : pair.second) {
//                    Log_info("%d is required by par %u", pair.first, p);
//                }
//            }
//        }
//    }

    /*
     * Step 1: generate timestamp ts for the IRT.
     * The ts should not be blocked by "not-ready" CRTs.
     * */
    chr_ts_t ts = GenerateChrTs(true);
    while (ready_txns_.count(ts) != 0) {
        /*
         * For handling a corner case.
         * Where the generated timestamp for this IRT is the same as a "ready" CRT
         * This may happen because only "not-ready" CRTs will affect the timestamp generation of IRTs.
         * not performance critical, very rare corner case
         * */
        verify(ready_txns_[ts] != nullptr);
        auto prev_txn = ready_txns_[ts];
        if (prev_txn->chr_phase_ != DIST_CAN_EXE) {
            Log_warn("here phase = %d, check your logic", prev_txn->chr_phase_);
            verify(0);
        }
        ts = GenerateChrTs(true);
    }


    /*
    * Step 2: create the txn data structure, and assign the execution callback.
     * The entry in ready_txns will be delted by the CheckExecutable funciton
     * Then add to the ready_txns queue (for execution) and my_txns (for notification)
    */
    auto dtxn = (TxChronos *) (CreateDTxn(txn_id));
    dtxn->chr_phase_ = LOCAL_NOT_STORED;
    dtxn->ts_ = ts;
    dtxn->handler_site_ = site_id_;
    dtxn->region_handler_ = site_id_;
    dtxn->pieces_ = cmds_by_par.at(this->partition_id_);
    dtxn->n_local_pieces_ = dtxn->pieces_.size();
    dtxn->n_executed_pieces = 0;
    dtxn->dist_output_ = output;
    dtxn->is_irt_ = true;

    ready_txns_[ts] = dtxn;
    my_txns_[ts] = dtxn;

#ifdef LAT_BREAKDOWN
    dtxn->handler_submit_ts_ =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
#endif//LAT_BREAKDOWN

    std::function<bool(std::set<innid_t> &)> execute_callback = [=](std::set<innid_t> &pending_pieces) {
        std::lock_guard<std::recursive_mutex> guard(mtx_);
        auto output_to_send = dtxn->RecursiveExecuteReadyPieces(this->partition_id_, pending_pieces);
        for (auto &pair : output_to_send) {
            parid_t target_par = pair.first;
            if (target_par != this->partition_id_) {
                ChronosSendOutputReq chr_req;
                chr_req.var_values = pair.second;
                chr_req.txn_id = txn_id;
                commo()->SendOutput(target_par, chr_req);
            }
        }
        bool finished = (dtxn->n_local_pieces_ == dtxn->n_executed_pieces);

        if (finished) {
#ifdef LAT_BREAKDOWN
            int64_t finish_ts =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
            Log_debug("id = %lu, break down local lat = %ld ms, local store = %ld ms, wait for execute = %ld ms",
                     txn_id,
                     finish_ts - dtxn->handler_submit_ts_,
                     dtxn->handler_local_prepared_ts - dtxn->handler_submit_ts_,
                     finish_ts - dtxn->handler_local_prepared_ts);
#endif//LAT_BREAKDOWN
            reply_callback();
        } else {
            for (auto &c : dtxn->pieces_) {
                if (!c.local_executed_) {
                }
            }
        }
        return finished;
    };
    dtxn->execute_callback_ = execute_callback;

    /*
     * Step 3: assign callback and call the store local rpc.
     */

    for (auto &item : cmds_by_par) {
        parid_t target_par = item.first;
        dtxn->dist_partition_acks_[target_par] = std::set<uint16_t>();


        for (auto &p : Config::GetConfig()->SitesByPartitionId(target_par)) {
            siteid_t target_site = p.id;
            if (target_site == site_id_) {
                //No need to send to myself
                continue;
            }

            ChronosStoreLocalReq req;
            req.txn_ts = ts;
            CollectNotifyTxns(target_site, target_par, req.piggy_my_txns);

            auto ack_callback = std::bind(&SchedulerChronos::PrepareIRTACK,
                                          this,
                                          txn_id,
                                          target_par,
                                          target_site,
                                          std::placeholders::_1);

            commo()->SendPrepareIRT(target_site, item.second, req, ack_callback);
        }
    }

    //    for (auto &replica: this->local_replicas_) {
    //        siteid_t target_site = replica;
    //        ChronosStoreLocalReq req;
    //        req.txn_ts = ts;
    //        auto ack_callback = std::bind(&SchedulerChronos::PrepareIRTACK,
    //                                      this,
    //                                      txn_id,
    //                                      target_site,
    //                                      std::placeholders::_1);
    //        commo()->SendPrepareIRT(target_site, cmd, req, ack_callback);
    //    }

    return 0;
}

int SchedulerChronos::OnSubmitCRT(const map<regionid_t, map<parid_t, vector<SimpleCommand>>> &cmds_by_region,
                                  const ChronosSubmitReq &chr_req,
                                  ChronosSubmitRes *chr_res,
                                  TxnOutput *output,
                                  const function<void()> &reply_callback) {

    std::lock_guard<std::recursive_mutex> guard(mtx_);
    Log_debug("++++ %s called", __FUNCTION__);
    //This is a distributed txn, and I am the coordinator.
    verify(!cmds_by_region.empty());
    verify(!cmds_by_region.begin()->second.empty());
    verify(!cmds_by_region.begin()->second.begin()->second.empty());
    txnid_t txn_id = cmds_by_region.begin()->second.begin()->second.begin()->root_id_;//should have the same root id

    /*
     * Step 1:
     * Initialize the status of each region to 0 (not_received) first.
     */
    verify(distributed_txns_by_me_.count(txn_id) == 0);

    distributed_txns_by_me_.insert(txn_id);
    auto dtxn = (TxChronos *) (GetOrCreateDTxn(txn_id));
    dtxn->handler_site_ = site_id_;
    dtxn->region_handler_ = site_id_;
    dtxn->is_irt_ = false;

    vector<parid_t> wait_input_shards{};
    map<parid_t, int32_t> par_input_ready;

    bool TPCC_wait_input = false;

    for (auto &r : cmds_by_region) {
        for (auto &item : r.second) {
            par_input_ready[item.first] = 1;
            for (auto &c : item.second) {
                Log_debug("piece for par %u, piece type %d, input ready = %d, map size = %d",
                          item.first,
                          c.inn_id(),
                          c.input.piece_input_ready_,
                          c.waiting_var_par_map.size());
                //Clean this up
                //This only works for tpcc
                if (c.inn_id() == 204 && c.input.piece_input_ready_ == 0) {
                    TPCC_wait_input = true;
                }
            }
        }
    }
    if (TPCC_wait_input) {
        dtxn->input_ready_ = 0;
        par_input_ready[partition_id_] = 0;
    }
//    for (auto &itr : par_input_ready) {
//        Log_info("txn %lu, input for par %u ready? %d", txn_id, itr.first, itr.second);
//    }


    /*
     * Step 2: Send the propose request to local replicas and the proxy of each touched region.
     */
#ifdef LAT_BREAKDOWN
    dtxn->handler_submit_ts_ =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
#endif//LAT_BREAKDOWN

    chr_ts_t src_ts = GenerateChrTs(true);

    //TODO: use now or use create ts
    //    auto now = std::chrono::system_clock::now();
    //    int64_t now_ts = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

    //    src_ts.timestamp_ = now_ts;


    for (auto &itr : cmds_by_region) {
        regionid_t region_id = itr.first;
        ChronosProposeRemoteReq prepare_remote_req;
        prepare_remote_req.par_input_ready = par_input_ready;
        prepare_remote_req.src_ts = src_ts;
        commo()->SendPrepareRemote(cmds_by_region.at(region_id), region_id, prepare_remote_req);
        //        }


        for (auto &pair : itr.second) {
            //Send to proxy
            parid_t par_id = pair.first;
            //pieces for remote (non-home) partitions
            //Send propose remote to *the proxy* of other replicas.
            auto callback = std::bind(&SchedulerChronos::ProposeRemoteAck,
                                      this,
                                      txn_id,
                                      par_id,
                                      std::placeholders::_1,
                                      std::placeholders::_2);
            //            if (region_id != my_rid_) {
            dtxn->dist_partition_status_[par_id] = P_REMOTE_SENT;
            dtxn->dist_partition_acks_[par_id] = std::set<uint16_t>();
            dtxn->dist_shard_mgr_acked_[par_id] = false;
        }
    }

    dtxn->chr_phase_ = DIST_REMOTE_SENT;


    //XS: no need to care the order of assigning callback and send RPC
    //The scheduler has global lock
    auto remote_prepared_callback = [=]() {
        std::lock_guard<std::recursive_mutex> guard(mtx_);
#ifdef LAT_BREAKDOWN
        dtxn->handler_remote_prepared_ts =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
#endif//LAT_BREAKDOWN
        if (cmds_by_region.count(my_rid_) != 0) {
            for (auto &itr : cmds_by_region.at(my_rid_)) {
                parid_t target_par = itr.first;
                verify(dtxn->dist_partition_status_[target_par] == P_LOCAL_NOT_SENT);

                auto par_sites = Config::GetConfig()->SitesByPartitionId(target_par);
                DastProposeLocalReq chr_req;
                chr_req.txn_ts = dtxn->ts_;
                chr_req.coord_site = site_id_;
                if (target_par == partition_id_) {
                    chr_req.shard_leader_site = site_id_;
                } else {
                    int r = RandomGenerator::rand(0, par_sites.size() - 1);
                    chr_req.shard_leader_site = par_sites[r].id;
                }
                for (auto &s : par_sites) {
                    siteid_t target_site = s.id;
                    if (target_site != site_id_) {
                        auto callback = std::bind(&SchedulerChronos::ProposeLocalACK,
                                                  this,
                                                  txn_id,
                                                  target_site,
                                                  std::placeholders::_1);
                        commo()->SendProposeLocal(target_site, itr.second, chr_req, callback);
                    }
                }
                dtxn->dist_partition_status_[target_par] = P_LOCAL_SENT;
            }
        } else {
            auto empty = std::vector<SimpleCommand>();
            for (auto &s : Config::GetConfig()->SitesByPartitionId(partition_id_)) {
                siteid_t target_site = s.id;
                DastProposeLocalReq chr_req;
                chr_req.txn_ts = dtxn->ts_;
                chr_req.coord_site = site_id_;
                chr_req.shard_leader_site = sites_per_region_;

                auto callback = std::bind(&SchedulerChronos::ProposeLocalACK,
                                          this,
                                          txn_id,
                                          target_site,
                                          std::placeholders::_1);
                commo()->SendProposeLocal(target_site, empty, chr_req, callback);
            }
            dtxn->dist_partition_status_[partition_id_] = P_LOCAL_SENT;
        }


        //      verify(dtxn->dist_partition_status_[partition_id_] == P_LOCAL_NOT_SENT);
        //      for (auto &replica : this->local_replicas_) {
        //            auto target_site = replica;
        //
        //            //      chr_req.piggy_clear_ts = my_clear_ts_;
        //            //      chr_req.piggy_my_ts = GenerateChrTs(true);
        //            //      CollectNotifyTxns(target_site, chr_req.piggy_my_pending_txns, chr_req.piggy_my_ts);
        //
        //            regionid_t rid = this->partition_id_ / (n_replicas * Config::GetConfig()->n_shard_per_region_);
        //            if (cmds_by_region.count(rid) != 0 && cmds_by_region.at(rid).count(this->partition_id_) != 0) {
        //            } else {
        //            }
        //        }
    };

    dtxn->remote_prepared_callback_ = remote_prepared_callback;
    /*
     * Step 3:
     * Assign the callback for execute
     */


    dtxn->send_output_client = reply_callback;

    regionid_t rid = this->partition_id_ / (n_replicas * Config::GetConfig()->n_shard_per_region_);
    if (cmds_by_region.count(rid) != 0 && cmds_by_region.at(rid).count(this->partition_id_) != 0) {
        dtxn->pieces_ = cmds_by_region.at(rid).at(this->partition_id_);
    } else {
        dtxn->pieces_ = std::vector<SimpleCommand>();
    }
    dtxn->n_local_pieces_ = dtxn->pieces_.size();
    dtxn->n_executed_pieces = 0;
    dtxn->dist_output_ = output;

    std::function<bool(std::set<innid_t> &)> execute_callback = [=](std::set<innid_t> &pending_pieces) {
        std::lock_guard<std::recursive_mutex> guard(mtx_);
        #ifdef LAT_BREAKDOWN
            int64_t ts_before, ts_after;
            ts_before =
                std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        #endif //LAT_BREAKDOWN

        if (dtxn->pieces_.size() != 0) {
            auto output_to_send = dtxn->RecursiveExecuteReadyPieces(this->partition_id_, pending_pieces);
            for (auto &pair : output_to_send) {
                parid_t target_par = pair.first;
                if (target_par != this->partition_id_) {
                    ChronosSendOutputReq chr_req;
                    chr_req.var_values = pair.second;
                    chr_req.txn_id = txn_id;
                    commo()->SendOutput(target_par, chr_req);
                }
            }
        }
        #ifdef LAT_BREAKDOWN
            ts_after =
                std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            Log_info("time for executing txn %lu, type = %u, is %ld us", txn_id, dtxn->root_type, ts_after - ts_before);
        #endif //LAT_BREAKDOWN

        bool finished = dtxn->n_local_pieces_ == dtxn->n_executed_pieces;
        if (finished) {
#ifdef LAT_BREAKDOWN
            dtxn->handler_local_exe_finish_ts =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
#endif//LAT_BREAKDOWN

            auto pos = std::find(distributed_txns_by_me_.begin(), distributed_txns_by_me_.end(), txn_id);
            verify(pos != distributed_txns_by_me_.end());
            distributed_txns_by_me_.erase(pos);
            dtxn->dist_partition_status_[partition_id_] = P_OUTPUT_RECEIVED;

            //            verify(dist_txn_tss_.count(dtxn->ts_) != 0);
            dist_txn_tss_.erase(dtxn->ts_);
            dtxn->CheckSendOutputToClient();
        }
        return finished;
    };

    dtxn->execute_callback_ = execute_callback;

    //Step 3: assign callback
    RPC_LOG("----- %s returned for txn_id = %lu", __FUNCTION__, txn_id);
}

void SchedulerChronos::ProposeRemoteAck(txnid_t txn_id,
                                        parid_t par_id,
                                        siteid_t site_id,
                                        ChronosProposeRemoteRes &chr_res) {
    std::lock_guard<std::recursive_mutex> guard(mtx_);
    Log_info("%s called for txn %lu from par_id %u: site id %hu", __FUNCTION__, txn_id, par_id, site_id);
    //This is the ack from the leader.
    auto dtxn = (TxChronos *) (GetOrCreateDTxn(txn_id));

    verify(dtxn->dist_partition_acks_.count(par_id) != 0);
    dtxn->dist_partition_acks_[par_id].insert(site_id);
    dtxn->anticipated_ts_[par_id / Config::GetConfig()->n_shard_per_region_] = chr_res.anticipated_ts;
    //  if (chr_res.anticipated_ts > dtxn->ts_){
    //    dtxn->ts_ = chr_res.anticipated_ts;
    //  }
    if (dtxn->dist_partition_status_[par_id] == P_REMOTE_SENT &&
        dtxn->dist_partition_acks_[par_id].size() >= (n_replicas + 1) / 2) {
        dtxn->dist_partition_status_[par_id] = P_REMOTE_PREPARED;
        CheckRemotePrepared(dtxn);
    }
}

void SchedulerChronos::ProposeLocalACK(txnid_t txn_id,
                                       siteid_t target_site,
                                       ChronosProposeLocalRes &chr_res) {
    std::lock_guard<std::recursive_mutex> guard(mtx_);
    Log_debug("ProposeLocalAck called for txn %lu from site %hu", txn_id, target_site);
    auto dtxn = (TxChronos *) (GetOrCreateDTxn(txn_id));
    verify(dtxn->dist_partition_acks_.count(partition_id_) != 0);
    siteid_t src_site = chr_res.my_site_id;
    dtxn->dist_partition_acks_[partition_id_].insert(src_site);

    //  if (this->notified_txn_ts[target_site] < told_ts){
    //    this->notified_txn_ts[target_site] = told_ts;
    //    SER_LOG("notified txn ts for site %hu changed to %s",
    //             target_site,
    //             told_ts.to_string().c_str());
    //  }

    if (dtxn->dist_partition_status_[partition_id_] == P_LOCAL_SENT &&
        dtxn->dist_partition_acks_[partition_id_].size() >= (n_replicas - 1) / 2) {
        verify(dtxn->dist_partition_status_[partition_id_] == P_LOCAL_SENT);
        dtxn->dist_partition_status_[partition_id_] = P_LOCAL_PREPARED;
        verify(dtxn->chr_phase_ == DIST_REMOTE_PREPARED);
        //    verify(dist_txn_tss_.count(dtxn->ts_) != 0);
        //    dist_txn_tss_.erase(dtxn->ts_);
    }
}

void SchedulerChronos::DistExeAck(txnid_t txn_id, parid_t par_id, TxnOutput &output, ChronosDistExeRes &chr_res) {
    if (chr_res.is_region_leader == 0) {
        return;
    }
    std::lock_guard<std::recursive_mutex> guard(mtx_);
    auto dtxn = (TxChronos *) (GetOrCreateDTxn(txn_id));
    for (auto &pair : output) {
        verify((*dtxn->dist_output_).count(pair.first) == 0);
        (*dtxn->dist_output_)[pair.first] = pair.second;
    }
    dtxn->dist_partition_status_[par_id] = P_OUTPUT_RECEIVED;
    dtxn->CheckSendOutputToClient();
}

void SchedulerChronos::PrepareIRTACK(txnid_t txn_id,
                                     parid_t target_par,
                                     siteid_t target_site,
                                     ChronosStoreLocalRes &chr_res) {
    //#ifdef LAT_BREAKDOWN
    //  int64_t ts_call, ts_return;
    //  ts_call =
    //      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    //#endif //LAT_BREAKDOWN


    std::lock_guard<std::recursive_mutex> guard(mtx_);
    auto dtxn = (TxChronos *) (GetDTxn(txn_id));
    verify(dtxn != nullptr);

    //Hanlde the piggy info before early return
    HandlePiggyInfo(target_site, chr_res.piggy_my_ts, chr_res.piggy_my_pending_txns, chr_res.piggy_delivered_ts);

    if (dtxn->chr_phase_ == LOCAL_CAN_EXE) {
        //already committed
        return;
    }
    dtxn->dist_partition_acks_[target_par].insert(target_site);
    bool all_par_majority_acks = true;
    for (auto &p : dtxn->dist_partition_acks_) {
        if (p.second.size() < (n_replicas - 1) / 2) {
            all_par_majority_acks = false;
        }
    }
    if (!all_par_majority_acks) {
        return;
    }
//    if (dtxn->n_local_store_acks > (this->n_replicas - 1) / 2) {
//        return;
//    }
//    dtxn->n_local_store_acks++;
//    if (dtxn->n_local_store_acks == (this->n_replicas - 1) / 2) {
#ifdef LAT_BREAKDOWN
    dtxn->handler_local_prepared_ts =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
#endif// LAT_BREAKDOWN
    dtxn->chr_phase_ = LOCAL_CAN_EXE;


    CheckExecutableTxns();

    //#ifdef LAT_BREAKDOWN
    //  ts_return =
    //      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    //  Log_info("time for call %s is %ld ms", __FUNCTION__, ts_return - ts_call);
    //#endif //LAT_BREAKDOWN

    RPC_LOG("--- %s returned", __FUNCTION__);
}

void SchedulerChronos::CheckExecutableTxns() {
    std::lock_guard<std::recursive_mutex> guard(mtx_);
    int found_pending_ = 0;
    std::set<innid_t> pending_pieces;
    for (auto itr = ready_txns_.begin(); itr != ready_txns_.end();) {
        auto dtxn = itr->second;
        verify(dtxn != nullptr);
        auto txn_id = dtxn->id();
        bool should_break =false;
        for (auto& c: ir_site_max_ts){
            if (! (c.second > dtxn->commit_ts_)){
                should_break = true;
            }
        }
        if (should_break){
            break;
        }
        if (dtxn->chr_phase_ < PHASE_CAN_EXE) {
            break;
        }
        if (dtxn->region_handler_ == this->site_id_) {
            bool finished = false;
            if (dtxn->exe_tried_ == false) {
                finished = dtxn->execute_callback_(pending_pieces);
            } else {
                if (new_input_) {
                    finished = dtxn->execute_callback_(pending_pieces);
                }
            }
            if (finished) {
                if (found_pending_ != 0) {
                }
                itr = ready_txns_.erase(itr);
                new_input_ = true;
            } else {
                found_pending_++;
                itr++;
                if (found_pending_ > max_pending_txns_) {
                    break;
                }
            }
        } else {
            dtxn->execute_callback_(pending_pieces);
            itr = ready_txns_.erase(itr);
        }
    }
    new_input_ = false;
}


void SchedulerChronos::CheckRemotePrepared(TxChronos *dtxn) {
    std::lock_guard<std::recursive_mutex> guard(mtx_);
    bool can_move_forward = true;
    for (auto &s : dtxn->dist_partition_status_) {
        if (s.second < P_REMOTE_PREPARED) {
            can_move_forward = false;
            break;
        }
    }
    if (can_move_forward) {
        chr_ts_t max_anticipated_ts;
        for (auto &pair : dtxn->anticipated_ts_) {
            if (pair.second > max_anticipated_ts) {
                max_anticipated_ts = pair.second;
            }
        }
        //        chr_ts_t my_anticipated;
        //        do {
        //            my_anticipated = GenerateChrTs(false);
        //
        //            if (my_anticipated > max_anticipated_ts) {
        //                max_anticipated_ts = my_anticipated;
        //            }
        //            Log_debug("My anticipated ts = %s",
        //                      my_anticipated.to_string().c_str());
        //        } while (dist_txn_tss_.count(max_anticipated_ts) != 0 || ready_txns_.count(max_anticipated_ts) != 0);

        /*
         * This guarantee that there is stretchable space between last and anticipated ts
         */

        dist_txn_tss_.erase(dtxn->ts_);


        dtxn->ts_ = max_anticipated_ts;
        //      dist_txn_tss_.insert(max_anticipated_ts);

        if (!dtxn->AllPiecesInputReady()) {
            dist_txn_tss_.insert(dtxn->ts_);
        }



        chr_ts_t cur_ts = GenerateChrTs(true);
        //        if (!(cur_ts < dtxn->ts_)) {
        //            SER_LOG("cut ts = %s, remote ts = %s",
        //                    cur_ts.to_string().c_str(),
        //                    dtxn->ts_.to_string().c_str());
        //        }
        //        verify(cur_ts < dtxn->ts_);
        verify(ready_txns_.count(dtxn->ts_) == 0);
        ready_txns_[dtxn->ts_] = dtxn;
        verify(dtxn->chr_phase_ == DIST_REMOTE_SENT);
        dtxn->chr_phase_ = DIST_REMOTE_PREPARED;


#ifdef LAT_BREAKDOWN
        dtxn->handler_local_prepared_ts =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
#endif//LAT_BREAKDOWN

        set<regionid_t> touched_regions;

        txnid_t txn_id = dtxn->id();
        ChronosDistExeReq chr_req;
        chr_req.txn_id = txn_id;
        chr_req.decision_ts = dtxn->ts_;
        for (auto &pair : dtxn->dist_partition_status_) {
            parid_t par_id = pair.first;
            touched_regions.insert(par_id / Config::GetConfig()->n_shard_per_region_);
            auto callback =
                    std::bind(&SchedulerChronos::DistExeAck,
                              this,
                              txn_id,
                              par_id,
                              std::placeholders::_1,
                              std::placeholders::_2);
            commo()->SendDistExe(par_id, chr_req, callback);
        }

        //Todo, notify the manager of all part regions.
        for (auto &r : touched_regions) {

            if (!dtxn->AllPiecesInputReady() && r == my_rid_) {
                //need to wait on new ts
                //Clean this up
                //This is hard coded.
                //Send block to manager,
                //IN TPC-C, this only happens to my region.
                //For other workloads, need to
                DastNotiManagerWaitReq noti;
                for (auto &s : Config::GetConfig()->SitesByPartitionId(partition_id_)) {
                    noti.touched_sites.insert(s.id);
                }
                noti.new_ts = max_anticipated_ts;
                noti.original_ts = dtxn->anticipated_ts_[r];
                noti.need_input_wait = 1;
                noti.txn_id = dtxn->id();
                commo()->SendNotiWait(r, noti);
            } else {
                DastNotiManagerWaitReq noti;
                noti.original_ts = dtxn->anticipated_ts_[r];
                noti.need_input_wait = 0;
                noti.txn_id = dtxn->id();
                commo()->SendNotiWait(r, noti);
            }
        }

        dtxn->chr_phase_ = DIST_CAN_EXE;
        CheckExecutableTxns();


        //Send propose local to **all** replicas (encapsulated in the commo send function)
    }
}

void SchedulerChronos::OnProposeRemote(const vector<SimpleCommand> &cmds,
                                       const ChronosProposeRemoteReq &req,
                                       ChronosProposeRemoteRes *chr_res,
                                       const function<void()> &reply_callback) {
    /*
     * Only the leader receives this
     * Step 1: calculate the anticipated ts, put into pending remote txns
     *
     */
    std::lock_guard<std::recursive_mutex> guard(mtx_);
    auto txn_id = cmds[0].root_id_;

    chr_ts_t src_ts = req.src_ts;
    chr_ts_t my_ts = GenerateChrTs(true);


    //I am the region leader of this txn
    //    verify(distributed_txns_by_me_.count(txn_id) == 0);
    distributed_txns_by_me_.insert(txn_id);

    chr_ts_t anticipated_ts;
    int tmp = 0;
    do {
        src_ts.timestamp_ += tmp++;
        //        anticipated_ts = CalculateAnticipatedTs(src_ts, my_ts);
    } while (dist_txn_tss_.count(anticipated_ts) != 0 || ready_txns_.count(anticipated_ts) != 0);

    chr_res->anticipated_ts = anticipated_ts;

    /*
     * There should be stretchable space, as the anticipated ts is a future for me
     */
    if (last_clock_ < anticipated_ts) {
        verify(dist_txn_tss_.count(anticipated_ts) == 0);
        dist_txn_tss_.insert(anticipated_ts);
    } else {
        verify(0);
    }

    verify(ready_txns_.count(anticipated_ts) == 0);


    /*
     * Step 2:
     * create the transaction and assign the callback after execution
     *
    */
    auto dtxn = (TxChronos *) (GetOrCreateDTxn(txn_id));
    dtxn->ts_ = anticipated_ts;
    dtxn->commit_ts_ = anticipated_ts;
    dtxn->handler_site_ = src_ts.site_id_;
    dtxn->region_handler_ = site_id_;
    dtxn->pieces_ = cmds;
    dtxn->n_local_pieces_ = cmds.size();
    dtxn->n_executed_pieces = 0;
    dtxn->dist_output_ = new TxnOutput;

    this->ready_txns_[anticipated_ts] = dtxn;

    std::function<bool(std::set<innid_t> &)> execute_callback = [=](std::set<innid_t> &pending_pieces) {
        std::lock_guard<std::recursive_mutex> guard(mtx_);
        auto output_to_send = dtxn->RecursiveExecuteReadyPieces(this->partition_id_, pending_pieces);
        for (auto &pair : output_to_send) {
            parid_t target_par = pair.first;
            if (target_par != this->partition_id_) {
                ChronosSendOutputReq chr_req;
                chr_req.var_values = pair.second;
                chr_req.txn_id = txn_id;
                commo()->SendOutput(target_par, chr_req);
            }
        }
        bool finished = (dtxn->n_local_pieces_ == dtxn->n_executed_pieces);
        if (finished) {
            dtxn->send_output_to_handler_();
            //            dist_txn_tss_.erase(anticipated_ts);
        }
        return finished;
    };
    dtxn->execute_callback_ = execute_callback;


    /*
     * Step3:
     * send to local replicas.
     */

    for (auto &replica : this->local_replicas_) {
        siteid_t target_site = replica;
        ChronosStoreRemoteReq store_remote_req;
        store_remote_req.handler_site = src_ts.site_id_;
        store_remote_req.region_leader = site_id_;
        //    store_remote_req.piggy_clear_ts = my_clear_ts_;
        //    store_remote_req.piggy_my_ts = GenerateChrTs(true);
        store_remote_req.anticipated_ts = anticipated_ts;
        //    CollectNotifyTxns(target_site, store_remote_req.piggy_my_pending_txns, store_remote_req.piggy_my_ts);


        commo()->SendStoreRemote(target_site, cmds, store_remote_req);
    }

    reply_callback();
    RPC_LOG("--On propose Remote returned");
}

void SchedulerChronos::OnProposeLocal(const vector<SimpleCommand> &cmds,
                                      const DastProposeLocalReq &req,
                                      ChronosProposeLocalRes *chr_res,
                                      const function<void()> &reply_callback) {
    std::lock_guard<std::recursive_mutex> guard(mtx_);


    verify(!cmds.empty());

    txnid_t txn_id = cmds.begin()->root_id_;
    Log_info("%s called for txn %lu on ts %s", __FUNCTION__, txn_id, req.txn_ts.to_string().c_str());

    chr_ts_t anticipated_ts = req.txn_ts;

    verify(dist_txn_tss_.count(anticipated_ts) == 0);
    dist_txn_tss_.insert(anticipated_ts);

    if (req.coord_site / N_REP_PER_SHARD != site_id_ / N_REP_PER_SHARD) {
        Log_info("test case passed, my site = %hu, coord = %hu", site_id_, req.coord_site);
    }

    auto dtxn = (TxChronos *) (GetOrCreateDTxn(txn_id));
    dtxn->ts_ = anticipated_ts;
    dtxn->commit_ts_ = anticipated_ts;
    dtxn->handler_site_ = req.coord_site;
    dtxn->region_handler_ = req.shard_leader_site;
    dtxn->pieces_ = cmds;
    dtxn->n_local_pieces_ = cmds.size();
    dtxn->n_executed_pieces = 0;
    dtxn->dist_output_ = new TxnOutput;
    dtxn->own_dist_output_ = true;

    verify(ready_txns_.count(anticipated_ts) == 0);
    ready_txns_[anticipated_ts] = dtxn;


    std::function<bool(std::set<innid_t> &)> execute_callback = [=](std::set<innid_t> &pending_pieces) {
        std::lock_guard<std::recursive_mutex> guard(mtx_);
        Log_info("executing CRT %lu", txn_id);
        auto output_to_send = dtxn->RecursiveExecuteReadyPieces(this->partition_id_, pending_pieces);


        for (auto &pair : output_to_send) {
            parid_t target_par = pair.first;
            if (target_par != this->partition_id_) {
                Log_info("Sending output to partition %u, size = %d",
                         target_par,
                         pair.second.size());
                ChronosSendOutputReq chr_req;
                chr_req.var_values = pair.second;
                chr_req.txn_id = txn_id;
                commo()->SendOutput(target_par, chr_req);
            }
        }

        bool finished = dtxn->n_local_pieces_ == dtxn->n_executed_pieces;
        if (finished) {
            dtxn->send_output_to_handler_();
            //            dist_txn_tss_.erase(anticipated_ts);
        }
        return finished;
    };
    dtxn->execute_callback_ = execute_callback;
    //  UpdateReplicaInfo(req.handler_site, req.piggy_my_ts, req.piggy_clear_ts, req.piggy_my_pending_txns);

    chr_res->my_site_id = site_id_;
    //  chr_res->piggy_clear_ts = my_clear_ts_;
    //  chr_res->piggy_my_ts = GenerateChrTs(true);
    //  siteid_t src_site = req.handler_site;
    //  CollectNotifyTxns(src_site, chr_res->piggy_my_pending_txns, anticipated_ts);

    reply_callback();
    RPC_LOG("%s returned", __FUNCTION__);
}

//Log_info stretchable timestamp implemented here
chr_ts_t SchedulerChronos::GenerateChrTs(bool for_local) {
    std::lock_guard<std::recursive_mutex> guard(mtx_);
    chr_ts_t ret;

    //Step 1: generate a monotonic ts first
    auto now = std::chrono::system_clock::now();

    int64_t ts = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() + this->clock_offset_;
    ret.timestamp_ = ts;
    ret.site_id_ = site_id_;
    ret.stretch_counter_ = ts;

    if (for_local == false) {
        if (ret < last_clock_) {
            ret.timestamp_ = last_clock_.timestamp_ + 1;
        }
        //!! Not setting last.
        //Leave a gap between last and ret;
        return ret;
    }


    //Step 2: Check whether need to stretch
    if (!dist_txn_tss_.empty()) {
        //will check stretch;
        chr_ts_t low_dist_ts = *dist_txn_tss_.begin();
        if (!(ret < low_dist_ts)) {
            //verify(last_clock_ < low_dist_ts);
            ret.timestamp_ = low_dist_ts.timestamp_ - 1;
        }
    }


    if (!(ret > last_clock_)) {
        //for monotocity
        ret = last_clock_;
        ret.stretch_counter_++;
    }
    //for monotocity
    last_clock_ = ret;
    return ret;
}

void SchedulerChronos::OnRemotePrepared(const DastRemotePreparedReq &req,
                                        DastRemotePreparedRes *chr_res,
                                        const function<void()> &callback) {

    std::lock_guard<std::recursive_mutex> lk(mtx_);

    parid_t par_id = req.my_partition_id;
    siteid_t site_id = req.my_site_id;
    txnid_t txn_id = req.txn_id;

    auto dtxn = (TxChronos *) (GetOrCreateDTxn(txn_id));
    verify(dtxn->dist_partition_acks_.count(par_id) != 0);
    dtxn->dist_partition_acks_[par_id].insert(site_id);
    dtxn->anticipated_ts_[par_id / Config::GetConfig()->n_shard_per_region_] = req.anticipated_ts;

    if (req.shard_manager == req.my_site_id) {
        dtxn->dist_shard_mgr_acked_[par_id] = true;
    }

    if (dtxn->dist_partition_status_[par_id] == P_REMOTE_SENT &&
        dtxn->dist_partition_acks_[par_id].size() >= (n_replicas + 1) / 2 &&
        dtxn->dist_shard_mgr_acked_[par_id]) {
        dtxn->dist_partition_status_[par_id] = P_REMOTE_PREPARED;
        CheckRemotePrepared(dtxn);
    }

    callback();
    RPC_LOG("%s returned, for txn id = %lu from site %hu", __FUNCTION__, req.txn_id, req.my_site_id);
}


void SchedulerChronos::OnPrepareIRT(const vector<SimpleCommand> &cmd,
                                    const ChronosStoreLocalReq &chr_req,
                                    ChronosStoreLocalRes *chr_res,
                                    const function<void()> &reply_callback) {

    //#ifdef LAT_BREAKDOWN
    //  int64_t ts_call, ts_return;
    //  ts_call =
    //      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    //#endif //LAT_BREAKDOWN

    //#ifndef BACKUP_TEST
    //  std::lock_guard<std::recursive_mutex> guard(mtx_);

    chr_ts_t ts = chr_req.txn_ts;
    siteid_t src_site = ts.site_id_;
    auto txn_id = cmd[0].root_id_;
    parid_t src_par = Config::GetConfig()->SiteById(src_site).partition_id_;


    std::lock_guard<std::recursive_mutex> guard(mtx_);

    if (this->ready_txns_.count(ts) != 0) {
        verify(ready_txns_[ts] != nullptr);
        verify(ready_txns_[ts]->id() == txn_id);
    }

    auto dtxn = (TxChronos *) (GetOrCreateDTxn(txn_id));
    dtxn->ts_ = ts;
    dtxn->handler_site_ = ts.site_id_;
    dtxn->region_handler_ = sites_for_same_par(site_id_, ts.site_id_) ? ts.site_id_ : site_id_;
    dtxn->chr_phase_ = LOCAL_CAN_EXE;
    dtxn->pieces_ = cmd;
    dtxn->n_local_pieces_ = cmd.size();
    dtxn->n_executed_pieces = 0;
    dtxn->dist_output_ = new TxnOutput;
    dtxn->own_dist_output_ = true;
    dtxn->is_irt_ = true;

    this->ready_txns_[ts] = dtxn;

    std::function<bool(std::set<innid_t> &)> execute_callback = [=](std::set<innid_t> &pending_pieces) {
        std::lock_guard<std::recursive_mutex> guard(mtx_);
        auto output_to_send = dtxn->RecursiveExecuteReadyPieces(this->partition_id_, pending_pieces);
        for (auto &pair : output_to_send) {
            parid_t target_par = pair.first;
            if (target_par != this->partition_id_) {
                ChronosSendOutputReq chr_req;
                chr_req.var_values = pair.second;
                chr_req.txn_id = txn_id;
                commo()->SendOutput(target_par, chr_req);
            }
        }

        bool finished = dtxn->n_local_pieces_ == dtxn->n_executed_pieces;

        return finished;
    };
    dtxn->execute_callback_ = execute_callback;


    HandlePiggyInfo(src_site, chr_req.txn_ts, chr_req.piggy_my_txns, chr_req.piggy_delivered_ts);
    CheckExecutableTxns();


    //fill in the reply info
    chr_res->piggy_my_ts = GenerateChrTs(true);
    chr_res->piggy_delivered_ts = chr_req.txn_ts;


    CollectNotifyTxns(src_site, src_par, chr_res->piggy_my_pending_txns);

    reply_callback();

    //#ifdef LAT_BREAKDOWN
    //  ts_return =
    //      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    //  Log_info("time for call %s is %ld ms", __FUNCTION__, ts_return - ts_call);
    //#endif //LAT_BREAKDOWN

    RPC_LOG("-- %s returned", __FUNCTION__);
}

void SchedulerChronos::OnDistExe(const ChronosDistExeReq &chr_req,
                                 ChronosDistExeRes *chr_res,
                                 TxnOutput *output,
                                 const function<void()> &reply_callback) {
    std::lock_guard<std::recursive_mutex> guard(mtx_);

    txnid_t txn_id = chr_req.txn_id;
    auto dtxn = (TxChronos *) (GetDTxn(txn_id));
    if (dtxn != nullptr) {
        //    if (this->local_replicas_.count(dtxn->handler_site_) == 0) {
        //      //This (previous line) ``if'' is not needed.
        //      //Keep for readbility.
        //      //The nodes in the same region as the handler did not add ts to dist_txn_tss
        //      //
        //      if (dist_txn_tss_.count(dtxn->ts_) != 0) {
        //        dist_txn_tss_.erase(dtxn->ts_);
        //        Log_debug("removing ts from dist_txn_tss %s",
        //                  dtxn->ts_.to_string().c_str());
        //      } else {
        //        Log_debug("not removing ts from dist_txn_tss %s, it was not added ",
        //                  dtxn->ts_.to_string().c_str());
        //      }
        //    }

        dist_txn_tss_.erase(dtxn->ts_);

        dtxn->chr_phase_ = DIST_CAN_EXE;

        chr_ts_t original_ts = dtxn->ts_;
        if (ready_txns_.count(original_ts) != 0) {
            //I am backup
            ready_txns_.erase(original_ts);
        }

        chr_ts_t new_ts = chr_req.decision_ts;
        verify(ready_txns_.count(new_ts) == 0);
        ready_txns_[new_ts] = dtxn;
        dtxn->ts_ = chr_req.decision_ts;

        if (!dtxn->AllPiecesInputReady()) {
            dist_txn_tss_.insert(dtxn->ts_);
        }

        if (dtxn->region_handler_ == site_id_) {
            chr_res->is_region_leader = 1;
            dtxn->send_output_to_handler_ = [=]() {
                std::lock_guard<std::recursive_mutex> guard(mtx_);
                *output = *dtxn->dist_output_;//Copy the output
                reply_callback();
            };
        } else {
            chr_res->is_region_leader = 0;
            reply_callback();
        }
        CheckExecutableTxns();
    } else {
        auto dtxn = (TxChronos *) (CreateDTxn(txn_id));
    }
}

void SchedulerChronos::OnSendOutput(const ChronosSendOutputReq &chr_req,
                                    ChronosSendOutputRes *chr_res,
                                    const function<void()> &reply_callback) {

    auto txn_id = chr_req.txn_id;
    std::lock_guard<std::recursive_mutex> guard(mtx_);
    auto dtxn = (TxChronos *) (GetDTxn(txn_id));

//    for (auto &out : chr_req.var_values) {
//        Log_info("received var %d", out.first);
//    }


    if (dtxn != nullptr) {
        if (dtxn->input_ready_ != 2) {
            dtxn->input_ready_ = 2;
        }

        if (dtxn->is_irt_ == false) {
            if (dtxn->AllPiecesInputReady() || dtxn->chr_phase_ > DIST_CAN_EXE) {
                dist_txn_tss_.erase(dtxn->ts_);
            }

            DastNotiManagerWaitReq noti;
            noti.original_ts = dtxn->ts_;
            noti.need_input_wait = 0;
            noti.txn_id = dtxn->id();
            commo()->SendNotiWait(my_rid_, noti);
        }

        new_input_ = true;
        bool more_to_run = dtxn->MergeCheckReadyPieces(chr_req.var_values);
        if (more_to_run) {
            if (dtxn->chr_phase_ >= LOCAL_CAN_EXE) {
                CheckExecutableTxns();
                //        bool finished = dtxn->execute_callback_();
                //        if (finished) {
                //          verify(this->pending_txns_.count(dtxn->ts_) != 0);
                //          verify (this->pending_txns_[dtxn->ts_] == txn_id);
                //          this->pending_txns_.erase(dtxn->ts_);
                //        } else {
                //          Log_info("n_executed = %d, n_total = %d", dtxn->n_executed_pieces, dtxn->n_local_pieces_);
                //        }
            } else {
            }
        } else {
        }
    }
    reply_callback();
}
void SchedulerChronos::CollectNotifyTxns(uint16_t target_site,
                                         parid_t target_par,
                                         std::vector<ChrTxnInfo> &into_vecotr) {
    std::lock_guard<std::recursive_mutex> guard(mtx_);

    auto itr = my_txns_.upper_bound(ir_site_notified_ts[target_site]);

    int count = 0;


    for (; itr != my_txns_.end(); itr++) {
        ChrTxnInfo info;
        TxChronos *txn = itr->second;
        verify(txn != nullptr);
        if (txn->is_irt_ && txn->dist_partition_acks_.count(target_par) != 0) {
            info.txn_id = itr->second->tid_;
            info.ts = itr->first;
            info.region_leader = site_id_;
            into_vecotr.push_back(info);
            count++;
        } else {
            //CRT
        }
    }

    ir_synced[target_site] = true;

    //    for (auto &item : my_txns_) {
    //        chr_ts_t ts = item.first;
    //        txnid_t tid = item.second;
    //
    //        if
    //
    //        verify(p_txn != nullptr);
    //        if (p_txn->ts_ < notified_txn_ts[site]) {
    //            continue;
    //        }
    //        if (p_txn->ts_ > up_to_ts) {
    //            verify(0);//this should not happen, a future txn?
    //            break;
    //        }
    //
    //        ChrTxnInfo info;
    //        info.txn_id = tid, info.ts = p_txn->ts_;
    //        info.region_leader = p_txn->region_handler_;
    //        into_vecotr.push_back(info);
    //        SER_LOG(
    //                "notifying site %hu the existence of txn %lu, with regional leader %hu, with ts %s",
    //                site,
    //                tid,
    //                p_txn->region_handler_,
    //                p_txn->ts_.to_string().c_str());
    //    }
}


//void SchedulerChronos::StoreRemoteACK(txnid_t txn_id,
//                                      siteid_t target_site,
//                                      ChronosStoreRemoteRes &chr_res) {
//  std::lock_guard<std::recursive_mutex> guard(mtx_);
//  ("%s called", __FUNCTION__);
//  auto dtxn = (TxChronos *) (GetOrCreateDTxn(txn_id));
//  //No need to do anything, except for update using piggy back information,
//
////  siteid_t src_site = chr_res.piggy_my_ts.site_id_;
//////  InsertNotifiedTxns(chr_res.piggy_my_pending_txns);
////  UpdateReplicaInfo(src_site, chr_res.piggy_my_ts, chr_res.piggy_clear_ts, chr_res.piggy_my_pending_txns);
//
////  if (this->notified_txn_ts[target_site] < told_ts){
////    this->notified_txn_ts[target_site] = told_ts;
////    SYNC_LOG("notified txn ts for site %hu changed to %s",
////             target_site,
////             told_ts.to_string().c_str());
////  }
//  RPC_LOG("--- %s returned", __FUNCTION__);
//}

//void SchedulerChronos::UpdateReplicaInfo(uint16_t for_site_id, const chr_ts_t &ts, const chr_ts_t &clear_ts,
//                                         const std::vector<ChrTxnInfo>& piggy_pending_txns) {
//  /*
//   * Important: Note that NOT necessarily site_id == ts.site_id_ (for distributed)
//  */
//  std::lock_guard<std::recursive_mutex> guard(mtx_);
//  bool need_check = false;
//  verify(local_replicas_clear_ts_.count(for_site_id) != 0);
//  verify(local_replicas_ts_.count(for_site_id) != 0);
//  if (local_replicas_ts_[for_site_id] < ts) {
//    local_replicas_ts_[for_site_id] = ts;
//    SYNC_LOG("time clock for site %hu changed to %s",
//              for_site_id,
//              ts.to_string().c_str());
//    need_check = true;
//  }
//
//  if (local_replicas_clear_ts_[for_site_id] < clear_ts) {
//    local_replicas_clear_ts_[for_site_id] = clear_ts;
//    SYNC_LOG("clear timestamp for site %hu changed to %s",
//              for_site_id,
//              clear_ts.to_string().c_str());
//    need_check = true;
//  }
//
//  for (auto &info: piggy_pending_txns) {
//    //pair <txnid, ts>
//    txnid_t tid = info.txn_id;
//    chr_ts_t tts = info.ts;
//    //use this to determine whether the transaction is known
//    //rather than using pending_txns (this txn may already been executed)
//    auto dtxn = (TxChronos *) (GetDTxn(tid));
//    if (dtxn != nullptr){
//      if (pending_txns_.count(tts) == 0) {
//        //Have already been removed from pending
//        verify(dtxn->chr_phase_ >= PHASE_CAN_EXE);
//      } else {
//        verify(pending_txns_[tts] == tid);
//        Log_debug("already know the existence of transaction id = %lu, with ts %s",
//                 tid,
//                 tts.to_string().c_str());
//      }
//    }else{
//      Log_debug("Notified with the existence of transaction id = %lu, with ts %s, region_leader = %hu",
//               tid,
//               tts.to_string().c_str(),
//               info.region_leader);
//      dtxn = (TxChronos* ) (CreateDTxn(tid));
//      pending_txns_[tts] = tid;
//      dtxn->region_handler_ = info.region_leader;
//    }
//  }
//
//  if (need_check) {
//    CheckExecutableTxns();
//  }
//}
//
//void SchedulerChronos::OnSync(const ChronosLocalSyncReq &req,
//                              ChronosLocalSyncRes *res,
//                              const function<void()> &reply_callback) {
//
//#ifdef LAT_BREAKDOWN
//  int64_t ts_call, ts_return;
//  ts_call =
//      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
//#endif //LAT_BREAKDOWN
//
//  std::lock_guard<std::recursive_mutex> guard(mtx_);
//  SYNC_LOG("***+++ %s called form site = %hu", __FUNCTION__, req.my_clock.site_id_);
//
////  UpdateReplicaInfo(req.my_clock.site_id_, req.my_clock, req.my_clear_ts, req.piggy_my_pending_txns);
//
//  reply_callback();
//
//
//#ifdef LAT_BREAKDOWN
//  ts_return =
//      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
//  Log_info("time for call %s is %ld ms", __FUNCTION__, ts_return - ts_call);
//#endif //LAT_BREAKDOWN
//  SYNC_LOG("***--- %s returned", __FUNCTION__);
//}

//void SchedulerChronos::SyncAck(siteid_t from_site,
//                               chr_ts_t told_ts,
//                               ChronosLocalSyncRes &res) {
//#ifdef LAT_BREAKDOWN
//  int64_t ts_call, ts_return;
//  ts_call =
//      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
//#endif //LAT_BREAKDOWN
//
//  std::lock_guard<std::recursive_mutex> guard(mtx_);
//  SYNC_LOG("++++++ %s called from site %hu", __FUNCTION__, from_site);
////  UpdateReplicaInfo(res.ret_clock.site_id_, res.ret_clock, res.ret_clear_ts);
////  if (this->notified_txn_ts[from_site] < told_ts){
////    this->notified_txn_ts[from_site] = told_ts;
////    SYNC_LOG("notified txn ts for site %hu changed to %s",
////        from_site,
////        told_ts.to_string().c_str());
////  }
//
//#ifdef LAT_BREAKDOWN
//  ts_return =
//      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
//  Log_info("time for call %s is %ld ms", __FUNCTION__, ts_return - ts_call);
//#endif //LAT_BREAKDOWN
//  SYNC_LOG("------ %s returned", __FUNCTION__);
//}
//
void SchedulerChronos::HandlePiggyInfo(siteid_t remote_site,
                                       const chr_ts_t &remote_ts,
                                       const vector<ChrTxnInfo> &noti_txns,
                                       const chr_ts_t &remote_delivered_ts) {


    std::lock_guard<std::recursive_mutex> guard(mtx_);
    for (auto &txn : noti_txns) {
        if (GetDTxn(txn.txn_id) != nullptr){
            //already know the txn
            continue;
        }
        auto dtxn = (TxChronos *) (GetOrCreateDTxn(txn.txn_id));
        dtxn->ts_ = txn.ts;
        dtxn->handler_site_ = remote_site;
        dtxn->region_handler_ = txn.region_leader;
        dtxn->chr_phase_ = LOCAL_CAN_EXE;
        this->ready_txns_[txn.ts] = dtxn;
    }
    /*
     * Update the three corresponding states
     */

    //update delivered ts
    if (ir_site_notified_ts[remote_site] < remote_delivered_ts) {
        ir_site_notified_ts[remote_site] = remote_delivered_ts;
    }
    //aware of txn_ts
    if (ir_site_max_ts[remote_site] < remote_ts) {
        //update clock offset
        chr_ts_t my_ts = GenerateChrTs(false);
        if (my_ts.timestamp_ < remote_ts.timestamp_) {
            this->clock_offset_ += (remote_ts.timestamp_ - my_ts.timestamp_);
        }

        ir_site_max_ts[remote_site] = remote_ts;

        for (auto it = this->ir_site_wait_ts_.begin(); it != this->ir_site_wait_ts_.end(); /* no increment*/) {
            if (remote_ts < it->first) {
                break;
            } else {
                it->second.erase(remote_site);
                if (it->second.empty()) {
                    dist_txn_tss_.erase(it->first);
                    //All sites are unblocked;
                    it = ir_site_wait_ts_.erase(it);
                } else {
                    it++;
                }
            }
        }
    }
}

void SchedulerChronos::OnIRSync(const DastIRSyncReq &req) {

    std::lock_guard<std::recursive_mutex> lk(mtx_);

    HandlePiggyInfo(req.my_site, req.txn_ts, req.my_txns, req.delivered_ts);

    CheckExecutableTxns();
}


void SchedulerChronos::OnNotiCRT(const DastNotiCRTReq &req) {
    txnid_t txn_id = req.txn_id;


    std::lock_guard<std::recursive_mutex> lk(mtx_);

    //I should be ir-relevant to this txn.
    verify(GetDTxn(txn_id) == nullptr);

    this->dist_txn_tss_.insert(req.anticipated_ts);

    this->ir_site_wait_ts_[req.anticipated_ts].insert(req.touched_sites.begin(), req.touched_sites.end());
}


void SchedulerChronos::OnNotiCommit(const DastNotiCommitReq &req) {

    std::lock_guard<std::recursive_mutex> lk(mtx_);

    this->dist_txn_tss_.erase(req.original_ts);
    this->ir_site_wait_ts_.erase(req.original_ts);

    if (req.need_input_wait == 1) {
        this->dist_txn_tss_.insert(req.new_ts);
        this->ir_site_wait_ts_[req.new_ts].insert(req.touched_sites.begin(), req.touched_sites.end());
    }
}


void SchedulerChronos::OnPrepareCRT(const vector<SimpleCommand> &cmds, const DastPrepareCRTReq &req) {
    verify(!cmds.empty());
    txnid_t txn_id = cmds.begin()->root_id_;

    std::lock_guard<std::recursive_mutex> lk(mtx_);

    chr_ts_t anticipated_ts = req.anticipated_ts;


    verify(dist_txn_tss_.count(anticipated_ts) == 0);
    verify(ready_txns_.count(anticipated_ts) == 0);

    auto test_dtxn = (TxChronos *) GetDTxn(txn_id);
    if (test_dtxn != nullptr) {
        //have already seen this dtxn
        //possibility 1. I am the coord
        //possibility 2. The onDistExe has already already arrived.
        if (test_dtxn->chr_phase_ == DIST_REMOTE_SENT) {
            //case 1: I am the coord.
            test_dtxn->dist_shard_mgr_acked_[this->partition_id_] = true;
            test_dtxn->dist_partition_acks_[this->partition_id_].insert(this->site_id_);
            test_dtxn->anticipated_ts_[this->partition_id_ / Config::GetConfig()->n_shard_per_region_] = req.anticipated_ts;

            test_dtxn->ts_ = req.anticipated_ts;
            dist_txn_tss_.insert(req.anticipated_ts);

            if (test_dtxn->dist_partition_status_[this->partition_id_] == P_REMOTE_SENT &&
                test_dtxn->dist_partition_acks_[this->partition_id_].size() >= (n_replicas + 1) / 2 &&
                test_dtxn->dist_shard_mgr_acked_[this->partition_id_]) {
                test_dtxn->dist_partition_status_[this->partition_id_] = P_REMOTE_PREPARED;
                CheckRemotePrepared(test_dtxn);
            }
        }
        //else: case 2, no need to do anything
        else {
            if (test_dtxn->input_ready_ == 1) {
                test_dtxn->input_ready_ = req.input_ready;
            }
        }
        return;
    }

    //Put it here to avoid the corner case where
    //DistExe arrives before prepareCRT.
    dist_txn_tss_.insert(anticipated_ts);


    auto dtxn = (TxChronos *) (GetOrCreateDTxn(txn_id));
    dtxn->ts_ = anticipated_ts;
    dtxn->handler_site_ = req.coord_site;
    dtxn->region_handler_ = req.shard_leader;
    dtxn->pieces_ = cmds;
    dtxn->n_local_pieces_ = cmds.size();
    dtxn->n_executed_pieces = 0;
    dtxn->dist_output_ = new TxnOutput;
    dtxn->input_ready_ = req.input_ready;
    dtxn->is_irt_ = false;

    this->ready_txns_[anticipated_ts] = dtxn;


    std::function<bool(std::set<innid_t> &)> execute_callback = [=](std::set<innid_t> &pending_pieces) {
        std::lock_guard<std::recursive_mutex> guard(mtx_);
        auto output_to_send = dtxn->RecursiveExecuteReadyPieces(this->partition_id_, pending_pieces);
        for (auto &pair : output_to_send) {
            parid_t target_par = pair.first;
            if (target_par != this->partition_id_) {
                ChronosSendOutputReq chr_req;
                chr_req.var_values = pair.second;
                chr_req.txn_id = txn_id;
                commo()->SendOutput(target_par, chr_req);
            }
        }
        bool finished = (dtxn->n_local_pieces_ == dtxn->n_executed_pieces);

        if (finished) {
            dtxn->send_output_to_handler_();
            //            dist_txn_tss_.erase(anticipated_ts);
        }
        return finished;
    };
    dtxn->execute_callback_ = execute_callback;

    DastRemotePreparedReq ack_to_handler;
    ack_to_handler.txn_id = txn_id;
    ack_to_handler.shard_manager = req.shard_leader;
    ack_to_handler.anticipated_ts = req.anticipated_ts;
    ack_to_handler.my_site_id = this->site_id_;
    ack_to_handler.my_partition_id = this->partition_id_;

    siteid_t coord_site = req.coord_site;

    commo()->SendRemotePrepared(coord_site, ack_to_handler);
}


ChronosCommo *SchedulerChronos::commo() {

    auto commo = dynamic_cast<ChronosCommo *>(commo_);
    verify(commo != nullptr);
    return commo;
}

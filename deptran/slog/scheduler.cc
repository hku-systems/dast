//
// Created by micha on 2020/3/23.
//


#include "deptran/slog/scheduler.h"
#include "deptran/frame.h"
#include "deptran/slog/commo.h"
#include "deptran/slog/tx.h"
#include <climits>
#include <limits>

//Thos XXX_LOG is defined in __dep__.h
using namespace rococo;
class Frame;

SchedulerSlog::SchedulerSlog(Frame *frame) : Scheduler() {
    this->frame_ = frame;
    auto config = Config::GetConfig();
    this->site_id_ = frame->site_info_->id;
    verify(!config->replica_groups_.empty());
    //TODO: currently assume the same number of replicas in each shard/replication
    n_replicas_ = config->replica_groups_[0].replicas.size();

    Log_debug("Created scheduler for site %hu, sync_interval_ms = %d, n_replicas in each site %d",
             site_id_,
             0,
             n_replicas_);


    for (auto site : config->SitesByPartitionId(frame->site_info_->partition_id_)) {
        if (site.id != this->site_id_) {
            //TODO: actually only the leader has follower sites.
            //For other sites, this should be empty
            follower_sites_.insert(site.id);
            Log_debug("added follower site %hu, my site id = %hu", site.id, this->site_id_);
        }
    }

    check_batch_loop_thread = std::thread(&SchedulerSlog::CheckBatchLoop, this);
    check_batch_loop_thread.detach();
}

int SchedulerSlog::OnSubmitTxn(const map<parid_t, vector<SimpleCommand>> &cmd_by_par,
                                int32_t  is_irt,
                                 TxnOutput *output,
                                 const function<void()> &reply_callback) {


#ifdef LAT_BREAKDOWN
    int64_t ts_call, ts_return;
    ts_call =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#endif//LAT_BREAKDOWN

    //In slog, the leader is always the leader
    std::lock_guard<std::recursive_mutex> guard(mtx_);
    verify(!cmd_by_par.empty());
    verify(cmd_by_par.begin()->second.size() > 0);
    txnid_t txn_id = cmd_by_par.begin()->second.at(0).root_id_;


    //For debug
    Log_debug("++++ %s called for txn_id = %lu, is_irt %d", __FUNCTION__, txn_id, is_irt);
    for (auto &pair : cmd_by_par) {
        for (auto &c : pair.second) {
            Log_debug("piece for par %u, type = %d, input ready = %d, map size = %d",
                     pair.first,
                     c.inn_id(),
                     c.input.piece_input_ready_,
                     c.waiting_var_par_map.size());
        }
    }


    //Push to local log
    auto dtxn = (TxSlog *) (GetOrCreateDTxn(txn_id));
    dtxn->is_local_ = true;
    txn_log_.push_back(dtxn);
    uint64_t local_index = txn_log_.size() - 1;

    Log_debug("local txn id = %lu has been inserted to the %lu th of the log",
             txn_id,
             local_index);

    //assign execute callback

    if (cmd_by_par.count(partition_id_) != 0) {
        dtxn->relevant_ = true;
        dtxn->pieces_ = cmd_by_par.at(partition_id_);
        dtxn->n_local_pieces_ = dtxn->pieces_.size();
    } else {
        dtxn->relevant_ = false;
    }
    dtxn->coord_site_ = site_id_;
    dtxn->n_executed_pieces = 0;
    dtxn->dist_output_ = output;
    for (auto &pair : cmd_by_par) {
        dtxn->par_output_received_[pair.first] = false;
    }
    //    id_to_index_map_[txn_id] = local_index;
    dtxn->reply_callback = [=]() {
        for (auto &pair : dtxn->par_output_received_) {
            if (pair.second == false) {
                Log_debug("output from par %u not received for txn %lu", pair.first, dtxn->id());
                return;
            }
        }
        Log_debug("all outputs received for txn %lu, send reply back to client", dtxn->id());
        reply_callback();
    };

    //    std::function<bool(std::set<innid_t> & pending_pieces)> execute_callback = [=](std::set<innid_t> &pending_pieces) {
    //        std::lock_guard<std::recursive_mutex> guard(mtx_);
    //        dtxn->RecursiveExecuteReadyPieces(this->partition_id_, pending_pieces);
    //        InsertBatch(dtxn);
    //
    //        //this is local txn.
    //        //Must can finish after one recursive phase
    //        bool finished = (dtxn->n_local_pieces_ == dtxn->n_executed_pieces);
    //
    //        if (finished) {
    //            Log_debug("finished executing transaction (I am leader) for id = %lu, will reply to client, output size = %d",
    //                      txn_id,
    //                      output->size());
    //            reply_callback();
    //        }
    //        return finished;
    //    };

    //    dtxn->execute_callback_ = execute_callback;
    /*
   * Step 3: assign callback and call the store local rpc.
   */

    //    verify(dtxn->id() == txn_id);
    //    verify(cmd[0].partition_id_ == Scheduler::partition_id_);
    //    verify(cmd[0].root_id_ == txn_id);

    if (is_irt){
        //send to the manager of my region
        regionid_t rid = partition_id_ / Config::GetConfig()->n_shard_per_region_;
        siteid_t manager_site = rid * Config::GetConfig()->n_shard_per_region_ * N_REP_PER_SHARD;
        commo()->SendToRegionManager(cmd_by_par, manager_site, site_id_);
    }else{
        //send to the global manager.
        commo()->SendToRaft(cmd_by_par, site_id_);
    }



    //Replicate the log

    //    auto ack_callback = std::bind(&SchedulerSlog::ReplicateLogLocalAck, this, local_index);
    //    for (auto siteid : follower_sites_) {
    //        commo()->SendReplicateLocal(siteid, cmd, txn_id, local_index, this->local_log_commited_index_, ack_callback);
    //    }

#ifdef LAT_BREAKDOWN
    ts_return =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    Log_debug("time for call %s is %ld ms", __FUNCTION__, ts_return - ts_call);
#endif//LAT_BREAKDOWN
    RPC_LOG("----- %s returned for txn_id = %lu", __FUNCTION__, txn_id);
    return 0;
}

void SchedulerSlog::ReplicateLogLocalAck(uint64_t local_index) {

#ifdef LAT_BREAKDOWN
    int64_t ts_call, ts_return;
    ts_call =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#endif//LAT_BREAKDOWN

    std::lock_guard<std::recursive_mutex> guard(mtx_);
    auto dtxn = this->txn_log_[local_index];
    dtxn->n_local_store_acks++;
    SER_LOG("n acks = %d, threshold = %d, id = %lu", dtxn->n_local_store_acks, this->n_replicas_ - 1, dtxn->id());
    if (dtxn->n_local_store_acks == this->n_replicas_ - 1) {
        if (dtxn->is_local_) {
            dtxn->slog_phase_ = TxSlog::SLOG_LOCAL_REPLICATED;
        } else {
            dtxn->slog_phase_ = TxSlog::SLOG_DIST_REPLICATED;
        }
    }
    //Check whether there are node to commit
    CheckExecutableTxns();
#ifdef LAT_BREAKDOWN
    ts_return =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    Log_debug("time for call %s is %ld ms", __FUNCTION__, ts_return - ts_call);
#endif//LAT_BREAKDOWN
    RPC_LOG("--- %s returned", __FUNCTION__);
}

int SchedulerSlog::OnInsertDistributed(const vector<SimpleCommand> &cmd,
                                       uint64_t global_index,
                                       const std::vector<parid_t> &touched_pars,
                                       siteid_t handler_site,
                                       uint64_t txn_id) {


#ifdef LAT_BREAKDOWN
    int64_t ts_call, ts_return;
    ts_call =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#endif//LAT_BREAKDOWN

    Log_debug("%s called, txn id = %lu", __FUNCTION__, txn_id);
    std::lock_guard<std::recursive_mutex> guard(mtx_);
    //This is a distributed txn, and I am the coordinator.
    auto g_log_len = global_log_.size();

    auto c = std::make_unique<std::vector<SimpleCommand>>();
    *c = cmd;

    if (global_index < g_log_len) {
        verify(global_log_[global_index].second == nullptr);
        global_log_[global_index].second = std::move(c);
        global_log_[global_index].first = txn_id;
    } else if (global_index == g_log_len) {
        global_log_.push_back(std::pair<uint64_t, std::unique_ptr<vector<SimpleCommand>>>(txn_id, std::move(c)));
    } else {
        while (global_log_.size() < global_index) {
            Log_debug("Global log skipping %lu, not received yet", global_log_.size() - 1);
            global_log_.push_back(std::pair<uint64_t, std::unique_ptr<vector<SimpleCommand>>>(0, nullptr));
        }
        global_log_.push_back(std::pair<uint64_t, std::unique_ptr<vector<SimpleCommand>>>(txn_id, std::move(c)));
    }


    while (global_log_next_index_ < global_log_.size()) {
        if (global_log_[global_log_next_index_].second == nullptr) {
            Log_debug("Not Processing index %lu of global log for now, not received yet", global_log_next_index_);
            break;
        }
        Log_debug("Processing index %lu of global log", global_log_next_index_);

        const vector<SimpleCommand> &cmds = *(global_log_[global_log_next_index_].second);
        auto txn_id = global_log_[global_log_next_index_].first;

        if (cmds.size() == 0 && GetDTxn(txn_id) == nullptr) {
            Log_debug("processing global log, skipping global log index %lu", global_index);
            global_log_next_index_++;
            continue;
        }
        Log_debug("processing global log txn id %lu at global index %lu",
                  txn_id,
                  global_index);


        if (GetDTxn(txn_id) == nullptr) {
            Log_debug("this is a new txn");
        }
        auto dtxn = (TxSlog *) (GetOrCreateDTxn(txn_id));
        dtxn->is_local_ = false;
        txn_log_.push_back(dtxn);
        uint64_t local_index = txn_log_.size() - 1;

        id_to_index_map_[txn_id] = local_index;
        Log_debug("here");
        dtxn->pieces_ = cmds;
        dtxn->n_local_pieces_ = dtxn->pieces_.size();
        dtxn->n_executed_pieces = 0;
        bool is_handler = true;
        if (dtxn->handler_site_ != this->site_id_) {
            Log_debug("I am not handler for txn id %lu", txn_id);
            is_handler = false;
            dtxn->dist_output_ = new TxnOutput;
            dtxn->send_output_client = []() {
                Log_debug("no need to send output to client");
            };
        }

        Log_debug("here");
        std::function<bool(std::set<innid_t> &)> execute_callback = [=](std::set<innid_t> &pending_pieces) {
            std::lock_guard<std::recursive_mutex> guard(mtx_);
            dtxn->RecursiveExecuteReadyPieces(this->partition_id_, pending_pieces);
            InsertBatch(dtxn);
            bool finished = (dtxn->n_executed_pieces == dtxn->n_local_pieces_);

            return finished;
        };

        dtxn->execute_callback_ = execute_callback;
        verify(dtxn->id() == txn_id);

        //Replicate the log
        Log_debug("here");
        auto ack_callback = std::bind(&SchedulerSlog::ReplicateLogLocalAck, this, local_index);
        for (auto siteid : follower_sites_) {
            commo()->SendReplicateLocal(siteid, cmds, txn_id, local_index, this->local_log_commited_index_, ack_callback);
        }

        RPC_LOG("----- %s returned for txn_id = %lu", __FUNCTION__, txn_id);
        global_log_next_index_++;
    }

#ifdef LAT_BREAKDOWN
    ts_return =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    Log_debug("time for call %s is %ld ms", __FUNCTION__, ts_return - ts_call);
#endif//LAT_BREAKDOWN

    return 0;
}



SlogCommo *SchedulerSlog::commo() {

    auto commo = dynamic_cast<SlogCommo *>(commo_);
    verify(commo != nullptr);
    return commo;
}


void SchedulerSlog::OnReplicateLocal(const vector<SimpleCommand> &cmd,
                                     txnid_t txn_id,
                                     uint64_t index,
                                     uint64_t commit_index,
                                     const function<void()> &callback) {

#ifdef LAT_BREAKDOWN
    int64_t ts_call, ts_return;
    ts_call =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#endif//LAT_BREAKDOWN


    std::lock_guard<std::recursive_mutex> guard(mtx_);


    //For debug
    Log_debug("++++ %s called for txn_id = %lu", __FUNCTION__, txn_id);
    for (auto &c : cmd) {
        Log_debug("piece is %d, input ready = %d, map size = %d",
                  c.inn_id(),
                  c.input.piece_input_ready_,
                  c.waiting_var_par_map.size());
    }


    //Push to local log
    auto dtxn = (TxSlog *) (GetOrCreateDTxn(txn_id));

    auto log_len = txn_log_.size();

    //This code can be simplified. But the current version is easier to understand
    if (index < log_len) {
        //This txn is late
        verify(txn_log_[index] == nullptr);
        txn_log_[index] = dtxn;
    } else if (index == log_len) {
        txn_log_.push_back(dtxn);
    } else {
        //Skip the middle
        while (txn_log_.size() < index) {
            txn_log_.push_back(nullptr);
        }
        txn_log_.push_back(dtxn);
    }

    id_to_index_map_[txn_id] = index;


    Log_debug("local txn id = %lu has been inserted to the %lu th of log by leader, commit index = %lu",
              txn_id,
              index,
              commit_index);

    //assign execute callback

    dtxn->pieces_ = cmd;
    dtxn->n_local_pieces_ = cmd.size();
    dtxn->n_executed_pieces = 0;
    dtxn->dist_output_ = new TxnOutput;

    std::function<bool(std::set<innid_t> & pending_pieces)> execute_callback = [=](std::set<innid_t> &pending_pieces) {
        std::lock_guard<std::recursive_mutex> guard(mtx_);
        dtxn->RecursiveExecuteReadyPieces(this->partition_id_, pending_pieces);

        bool finished = (dtxn->n_local_pieces_ == dtxn->n_executed_pieces);

        if (finished) {

            Log_debug("finished executing transaction index = %lu (I am not leader) for id = %lu, will reply to client",
                      index,
                      txn_id);
        }
        return finished;
    };

    dtxn->execute_callback_ = execute_callback;

    if (commit_index > local_log_commited_index_ || local_log_commited_index_ == -1) {
        local_log_commited_index_ = commit_index;
        Log_debug("commit index updated to %lu", commit_index);
    }
    for (uint64_t cur = backup_executed_index_ + 1; cur < txn_log_.size(); cur++) {
        if (cur > local_log_commited_index_) {
            break;
        }
        if (txn_log_[cur] != nullptr) {
            //backup is not criticial for performance
            //XSTODO: make this robust
            std::set<innid_t> fake_pending;
            txn_log_[cur]->execute_callback_(fake_pending);
            Log_debug("[First] Going to execute txn (I am not leader) for txn id = %lu", txn_log_[cur]->id());
            backup_executed_index_ = cur;
        }
    }


    verify(dtxn->id() == txn_id);
    //  verify(cmd[0].partition_id_ == Scheduler::partition_id_);
    //  verify(cmd[0].root_id_ == txn_id);

    //Replicate the log

    RPC_LOG("----- %s returned for txn_id = %lu", __FUNCTION__, txn_id);
    callback();
#ifdef LAT_BREAKDOWN
    ts_return =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    Log_debug("time for call %s is %ld ms", __FUNCTION__, ts_return - ts_call);
#endif//LAT_BREAKDOWN
}

void SchedulerSlog::OnSubmitDistributed(const map<parid_t, vector<SimpleCommand>> &cmds_by_par,
                                        TxnOutput *output,
                                        const function<void()> &reply_callback) {
#ifdef LAT_BREAKDOWN
    int64_t ts_call, ts_return;
    ts_call =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#endif//LAT_BREAKDOWN


    //Create the txn meta data
    std::lock_guard<std::recursive_mutex> guard(mtx_);

    verify(cmds_by_par.size() > 0);
    verify(cmds_by_par.begin()->second.size() > 0);
    txnid_t txn_id = cmds_by_par.begin()->second.begin()->root_id_;//should have the same root_id

    Log_debug("%s called for txn %lu", __FUNCTION__, txn_id);

    auto dtxn = (TxSlog *) (GetOrCreateDTxn(txn_id));

    dtxn->dist_output_ = output;
    dtxn->send_output_client = [=]() {
        Log_debug("Sending output to client");
        reply_callback();
    };
    dtxn->handler_site_ = this->site_id_;
    for (auto &pair : cmds_by_par) {
        dtxn->par_output_received_[pair.first] = false;
    }
    //Submit to raft;
    commo()->SendToRaft(cmds_by_par, this->site_id_);
#ifdef LAT_BREAKDOWN
    ts_return =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    Log_debug("time for call %s is %ld ms", __FUNCTION__, ts_return - ts_call);
#endif//LAT_BREAKDOWN
}
void SchedulerSlog::OnSendBatchRemote(uint32_t src_par, const std::vector<std::pair<txnid_t, TxnOutput>> &batch) {


#ifdef LAT_BREAKDOWN
    int64_t ts_call, ts_return;
    ts_call =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#endif//LAT_BREAKDOWN

    std::lock_guard<std::recursive_mutex> guard(mtx_);

    Log_debug("%s called from src_par %u", __FUNCTION__, src_par);

    for (auto &out : batch) {
        auto txnid = out.first;
        for (auto &pair : out.second) {
            auto vars = pair.second;
            for (auto &var : vars) {
                Log_debug("[recv] txn %lu, piece %u, var %d", out.first, pair.first, var.first);
            }
        }
    }

    bool need_check = false;

    for (auto itr = recved_output_buffer_.begin(); itr != recved_output_buffer_.end();) {
        auto txn_id = itr->first;

        auto dtxn = (TxSlog *) GetDTxn(txn_id);
        if (dtxn == nullptr) {
            verify(id_to_index_map_.count(txn_id) == 0);
            Log_debug("Received buffer has for txn id %lu, not related for me", txn_id);
            verify(0);
        } else {
            //This should be a distributed txn, and I am part of it.
            if (id_to_index_map_.count(txn_id) != 0) {
                auto index = id_to_index_map_[txn_id];
                Log_debug("Received log for txn id %lu, at index %lu", txn_id, index);
            } else {
                Log_debug("Received log for txn id %lu, raft not inserted yet", txn_id);
                itr++;
                continue;
            }

            dtxn->dist_output_->insert(itr->second.begin(), itr->second.end());
            Log_debug("output for txn %lu received, status for par %hu changed to true, phase = %d", dtxn->id(), src_par, dtxn->slog_phase_);
            dtxn->par_output_received_[src_par] = true;
            if (dtxn->slog_phase_ == TxSlog::SLOG_DIST_EXECUTED) {
                //Local execution finished
                dtxn->CheckSendOutputToClient();
            } else {
                bool more_to_run = dtxn->MergeCheckReadyPieces(itr->second);
                if (more_to_run) {
                    need_check = true;
                    Log_debug("more to run for txn id = %lu ", dtxn->id());
                }
            }
        }
        itr = recved_output_buffer_.erase(itr);
    }


    for (auto &pair : batch) {
        auto txn_id = pair.first;

        auto dtxn = (TxSlog *) GetDTxn(txn_id);
        if (dtxn == nullptr) {
            verify(id_to_index_map_.count(txn_id) == 0);
            Log_debug("Received log for txn id %lu, not related for me", txn_id);
            continue;
        } else {
            //This should be a distributed txn, and I am part of it.
            if (id_to_index_map_.count(txn_id) != 0) {
                auto index = id_to_index_map_[txn_id];
                Log_debug("Received log for txn id %lu, at index %lu", txn_id, index);
            } else {
                recved_output_buffer_.push_back(pair);
                Log_debug("Received log for txn id %lu, raft not inserted yet", txn_id);
                continue;
            }
            Log_debug("dtxn phase %d", dtxn->slog_phase_);
            if (dtxn->slog_phase_ == TxSlog::SLOG_OUTPUT_SENT) {
                continue;
            }
            dtxn->dist_output_->insert(pair.second.begin(), pair.second.end());
            Log_debug("output for txn %lu received, status for par %hu changed to true, phase = %d, size = %d",
                     dtxn->id(),
                     src_par,
                     dtxn->slog_phase_,
                     pair.second.size());
            dtxn->par_output_received_[src_par] = true;
            if (dtxn->slog_phase_ == TxSlog::SLOG_DIST_EXECUTED) {
                //Local execution finished
                dtxn->CheckSendOutputToClient();
            } else {
                bool more_to_run = dtxn->MergeCheckReadyPieces(pair.second);
                if (more_to_run) {
                    need_check = true;
                    Log_debug("more to run for txn id = %lu ", dtxn->id());
                }
            }
        }
    }
    if (need_check) {
        CheckExecutableTxns();
    }
    Log_debug("%s returned", __FUNCTION__);

#ifdef LAT_BREAKDOWN
    ts_return =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    Log_debug("time for call %s is %ld ms", __FUNCTION__, ts_return - ts_call);
#endif//LAT_BREAKDOWN
}
void SchedulerSlog::InsertBatch(TxSlog *dtxn) {

#ifdef LAT_BREAKDOWN
    int64_t ts_call, ts_return;
    ts_call =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#endif//LAT_BREAKDOWN

    std::lock_guard<std::recursive_mutex> guard(mtx_);
    verify(dtxn != nullptr);

    auto now = std::chrono::system_clock::now();
    uint64_t now_ts = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

    Log_debug("Inserting dtxn %lu", dtxn->id());
    output_batches_.emplace_back(std::pair<txnid_t, TxnOutput>(dtxn->id(), *(dtxn->dist_output_)));
    if (now_ts - batch_start_time_ > batch_interval_) {
        commo()->SendBatchOutput(output_batches_, this->partition_id_);
        output_batches_.clear();

        auto now = std::chrono::system_clock::now();
        uint64_t now_ts = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
        batch_start_time_ = now_ts;
    } else {
        Log_debug("batch time = %d ms", (now_ts - batch_start_time_) / 1000);
    }


#ifdef LAT_BREAKDOWN
    ts_return =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    Log_debug("time for call %s is %ld ms", __FUNCTION__, ts_return - ts_call);
#endif//LAT_BREAKDOWN
}
void SchedulerSlog::CheckBatchLoop() {
    while (true) {
        std::unique_lock<std::recursive_mutex> lk(mtx_);
        if (commo_ != nullptr) {
            auto now = std::chrono::system_clock::now();
            uint64_t now_ts = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
            if (!output_batches_.empty() && now_ts - batch_start_time_ > batch_interval_) {
                commo()->SendBatchOutput(output_batches_, this->partition_id_);
                output_batches_.clear();

                auto now = std::chrono::system_clock::now();
                uint64_t now_ts = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
                batch_start_time_ = now_ts;
            }
        }
        lk.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(batch_interval_));
    }
}
void SchedulerSlog::CheckExecutableTxns() {

#ifdef LAT_BREAKDOWN
    int64_t ts_call, ts_return;
    ts_call =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#endif//LAT_BREAKDOWN
    int found_pending = 0;

    std::set<innid_t> pending_pieces;


    std::lock_guard<std::recursive_mutex> guard(mtx_);
    Log_debug("%s called, local committed index =%lu", __FUNCTION__, local_log_commited_index_);
    auto itr = txn_log_.begin();
    itr = itr + local_log_commited_index_ + 1;

    bool cont_check = true;

    while (itr != txn_log_.end() && cont_check) {
        auto txn_to_exe = *itr;
        Log_debug("Trying to commit index %lu, phase = %d", itr - txn_log_.begin(), txn_to_exe->slog_phase_);

        switch (txn_to_exe->slog_phase_) {
            case TxSlog::SLOG_EMPTY: {
                break;
            }
            case TxSlog::SLOG_LOCAL_REPLICATED: {
                bool finished = (*itr)->execute_callback_(pending_pieces);
                if (finished) {
                    Log_debug("finish executing transaction, I am the leader, id = %lu, found_pending = %d",
                             (*itr)->id(),
                             found_pending);
                    if (found_pending == 0) {
                        Log_debug("committed index = %lu", local_log_commited_index_);
                        local_log_commited_index_++;
                        verify(local_log_commited_index_ == itr - txn_log_.begin());
                    } else {
                        //not updating local_log_committed
                    }
                    txn_to_exe->slog_phase_ = TxSlog::SLOG_LOCAL_OUTPUT_READY;
                } else {
                    Log_debug("local txn not finished, transaction, I am the leader, id = %lu, found_pending = %d",
                             (*itr)->id(),
                             found_pending);
                    found_pending++;
                    if (found_pending > max_pending_txns_) {
                        cont_check = false;
                    }
                }
                break;
            }
            case TxSlog::SLOG_LOCAL_OUTPUT_READY: {
                if (found_pending == 0) {
                    Log_debug("committed index = %lu", local_log_commited_index_);
                    local_log_commited_index_++;
                }
                break;
            }
            case TxSlog::SLOG_DIST_REPLICATED://3
            case TxSlog::SLOG_DIST_CAN_EXE: { //4
                Log_debug("Trying to commit index %lu (distributed), id =%lu, phase =%d",
                         itr - txn_log_.begin(),
                         (*itr)->id(),
                         (*itr)->slog_phase_);
                txn_to_exe->slog_phase_ = TxSlog::SLOG_DIST_CAN_EXE;
                Log_debug("[First] Going to execute transaction, I am the leader, id = %lu, found_pending = %d",
                         (*itr)->id(),
                         found_pending);
                bool finished = txn_to_exe->execute_callback_(pending_pieces);
                if (finished) {
                    Log_debug("txn id = %lu execution finished", txn_to_exe->id());
                    txn_to_exe->par_output_received_[partition_id_] = true;
                    txn_to_exe->slog_phase_ = TxSlog::SLOG_DIST_EXECUTED;
                    txn_to_exe->CheckSendOutputToClient();
                    if (found_pending == 0) {
                        Log_debug("committed index = %lu", local_log_commited_index_);
                        local_log_commited_index_++;
                        verify(local_log_commited_index_ == itr - txn_log_.begin());
                    }
                } else {
                    found_pending++;
                    Log_debug("execution of txn %lu not finished, found pending = %d", (*itr)->id(), found_pending);
                    if (found_pending > max_pending_txns_) {
                        cont_check = false;
                    }
                }
                break;
            }
            case TxSlog::SLOG_DIST_EXECUTED://5
            case TxSlog::SLOG_DIST_OUTPUT_READY:
                txn_to_exe->CheckSendOutputToClient();
            case TxSlog::SLOG_OUTPUT_SENT: {
                if (found_pending == 0) {
                    Log_debug("committed index = %lu", local_log_commited_index_);
                    local_log_commited_index_++;
                }
                break;
            }
            default:
                verify(0);
        }
        itr++;
    }

#ifdef LAT_BREAKDOWN
    ts_return =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    Log_debug("time for call %s is %ld ms", __FUNCTION__, ts_return - ts_call);
#endif//LAT_BREAKDOWN
}

void SchedulerSlog::OnSendBatch(const uint64_t &start_index, const std::vector<std::pair<txnid_t, std::vector<SimpleCommand>>> &cmds, const std::vector<std::pair<txnid_t, siteid_t>> &coord_sites) {
    Log_debug("%s called, %lu txns, starting from index %d", __FUNCTION__, cmds.size(), start_index);
    std::lock_guard<std::recursive_mutex> guard(mtx_);

    for (auto i = txn_queue_.size(); i < start_index + cmds.size(); i++) {
        txn_queue_.push_back(nullptr);
    }


    for (int i = 0; i < cmds.size(); i++) {
        txnid_t txn_id = cmds[i].first;
        Log_debug("trying to insert txn %lu to queue, coord_site= %hu,  queue size = %lu", txn_id, coord_sites[i].second, txn_queue_.size());
        if (GetDTxn(txn_id) != nullptr) {
            // I am the coord
            verify(coord_sites[i].second == site_id_);
            auto dtxn = (TxSlog *) GetDTxn(txn_id);
            verify(dtxn->coord_site_ == site_id_);
            verify(txn_queue_.size() > start_index + i);
            verify(txn_queue_[start_index + i] == nullptr);
            txn_queue_[start_index + i] = dtxn;
        } else {
            auto dtxn = (TxSlog *) CreateDTxn(txn_id);
            dtxn->relevant_ = true;
            dtxn->pieces_ = cmds[i].second;
            dtxn->n_local_pieces_ = dtxn->pieces_.size();
            dtxn->n_executed_pieces = 0;
            dtxn->coord_site_ = coord_sites[i].second;
            dtxn->dist_output_ = new TxnOutput;
            dtxn->own_dist_output_ = true;

            if (dtxn->coord_site_ / N_REP_PER_SHARD == site_id_ / N_REP_PER_SHARD) {
                //I am a replica of the coord
                dtxn->reply_callback = [=]() {
                    Log_debug("I am a replica of the coord, no need to send output");
                };
            } else {
                dtxn->reply_callback = [=]() {
                    //send output to the coord
                    if (dtxn->n_local_pieces_ != 0 && dtxn->dist_output_->size() != 0)
                        commo()->SendTxnOutput(dtxn->coord_site_, partition_id_, dtxn->id(), dtxn->dist_output_);
                };
            }
            verify(txn_queue_.size() > start_index + i);
            verify(txn_queue_[start_index + i] == nullptr);
            txn_queue_[start_index + i] = dtxn;
        }
    }
    Log_debug("after push, txn queue size become %lu", txn_queue_.size());
    ExecTxnQueue();

    CheckPendingDeps();
}

void SchedulerSlog::ExecTxnQueue() {

    int found_pending = 0;
    std::set<innid_t> pending_pieces;

    std::lock_guard<std::recursive_mutex> guard(mtx_);
    Log_debug("%s called, next index to check = %lu", __FUNCTION__, next_execute_index_);
    for (auto i = next_execute_index_; i < txn_queue_.size(); i++) {
        if (txn_queue_[i] == nullptr) {
            Log_debug("txn queue index %lu not received", i);
            break;
        }
        TxSlog *txn = txn_queue_[i];
        Log_debug("checking txn %lu at index %lu", txn->id(), i);
        if (!txn->executed_) {
            std::map<parid_t, std::map<varid_t, Value>> dependent_vars;
            bool finished = txn->TryExecute(pending_pieces, partition_id_, dependent_vars);
            for (auto &pair : dependent_vars) {
                parid_t target_par = pair.first;
                if (target_par != this->partition_id_) {
                    Log_debug("Sending dependent value to partition %u, size = %d",
                             target_par,
                             pair.second.size());
                    ChronosSendOutputReq chr_req;
                    chr_req.var_values = pair.second;
                    chr_req.txn_id = txn->id();
                    commo()->SendDependentValues(target_par, chr_req);
                }
            }
            if (!finished) {
                found_pending++;
                Log_debug("Txn execution not finished, found pending ++ => %d", found_pending);
                if (found_pending > max_pending_txns_) {
                    break;
                }
            }
            if (finished && found_pending == 0){
                next_execute_index_++;
                Log_debug("exection finished and no pending before, next execut++ => %lu", next_execute_index_);
            }
        }else{
            //txn has executed
            if (found_pending == 0) {
                next_execute_index_++;
                Log_debug("exection already executed and no pending before, next execut++ => %lu", next_execute_index_);
            }
        }
    }
    Log_debug("%s returned", __FUNCTION__);
}

void SchedulerSlog::OnSendTxnOutput(const uint64_t &txn_id, const parid_t &par_id, const TxnOutput &output) {
    Log_debug("%s called for Txn %lu from par %u", __FUNCTION__, txn_id, par_id);
    std::lock_guard<std::recursive_mutex> guard(mtx_);

    verify(GetDTxn(txn_id) != nullptr);

    auto dtxn = (TxSlog *) GetDTxn(txn_id);
    verify(dtxn->par_output_received_.count(par_id) > 0);

    if (dtxn->par_output_received_[par_id] == false) {
        dtxn->par_output_received_[par_id] = true;
        dtxn->reply_callback();
    }
}

void SchedulerSlog::OnSendDepValues(const ChronosSendOutputReq &chr_req, ChronosSendOutputRes *chr_res) {

    std::lock_guard<std::recursive_mutex> guard(mtx_);
    Log_debug("%s called, received output for txn %lu", __FUNCTION__, chr_req.txn_id);
    auto txn_id = chr_req.txn_id;
    if (GetDTxn(txn_id) == nullptr) {
        //txn content not arrived yet.
        pending_dep_values_.push_back(chr_req);
    } else {
        auto dtxn = (TxSlog *) (GetDTxn(txn_id));

        bool more_to_run = dtxn->MergeCheckReadyPieces(chr_req.var_values);

        if (more_to_run) {
            ExecTxnQueue();
        }
    }
}

void SchedulerSlog::CheckPendingDeps() {
    Log_debug("%s called", __FUNCTION__ );
    std::lock_guard<std::recursive_mutex> guard(mtx_);
    bool need_check = false;
    for (auto itr = pending_dep_values_.begin(); itr != pending_dep_values_.end();) {
        txnid_t txnid = itr->txn_id;
        if (GetDTxn(txnid) != nullptr) {
            Log_debug("merging pending output for txn %lu", txnid);
            auto dtxn = (TxSlog*)GetDTxn(txnid);
            need_check =
                    dtxn->MergeCheckReadyPieces(itr->var_values) || need_check;
            itr = pending_dep_values_.erase(itr);
        } else {
            itr++;
        }
    }
    if (need_check){
        ExecTxnQueue();
    }
    Log_debug("%s returned", __FUNCTION__ );
}
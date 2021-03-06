//
// Created by micha on 2020/3/23.
//


#pragma once
#include "deptran/brq/sched.h"
#include "deptran/rcc_rpc.h"
#include "deptran/slog/tx.h"
namespace rococo {

class TxSlog;
class SlogCommo;

class SchedulerSlog : public Scheduler {
public:
    SchedulerSlog(Frame *frame);


    int OnSubmitTxn(const map<parid_t, vector<SimpleCommand>> &cmd,
                    int32_t  is_irt,
                      TxnOutput *output,
                      const function<void()> &callback);


    int OnInsertDistributed(const vector<SimpleCommand> &cmds,
                            uint64_t index,
                            const std::vector<parid_t> &touched_pars,
                            siteid_t handler_site,
                            uint64_t txn_id);

    void OnSubmitDistributed(const map<parid_t, vector<SimpleCommand>> &cmds_by_par,
                             TxnOutput *output,
                             const function<void()> &reply_callback);


    void ReplicateLogLocalAck(uint64_t index);

    void OnSendBatch(const uint64_t &start_index, const std::vector<std::pair<txnid_t, std::vector<SimpleCommand>>> &cmds, const std::vector<std::pair<txnid_t, siteid_t>> &coord_sites);


    void OnReplicateLocal(const vector<SimpleCommand> &cmd,
                          uint64_t txn_id,
                          uint64_t index,
                          uint64_t commit_index,
                          const function<void()> &callback);



    void OnSendBatchRemote(parid_t src_par,
                           const vector<pair<uint64_t, TxnOutput>> &batch);

    void InsertBatch(TxSlog *dtxn);

    void CheckBatchLoop();

    void CheckExecutableTxns();

    //for those not ready.
    std::vector<pair<txnid_t, TxnOutput>> recved_output_buffer_;


    std::thread check_batch_loop_thread;

    std::vector<TxSlog *> txn_log_ = {};

    std::vector<std::pair<txnid_t, std::unique_ptr<vector<SimpleCommand>>>> global_log_ = {};
    uint64_t global_log_next_index_ = 0;

    SlogCommo *commo();


    std::vector<std::pair<txnid_t, TxnOutput>> output_batches_ = {};
    int batch_size_ = 1;
    int batch_interval_ = 5;
    uint64_t batch_start_time_ = 0;

    map<txnid_t, uint64_t> id_to_index_map_ = {};


    int max_pending_txns_ = 5;

    //  uint64_t received_global_index_ = -1;
    int n_replicas_;
    uint64_t local_log_commited_index_ = -1;
    uint64_t backup_executed_index_ = -1;

    std::set<siteid_t> follower_sites_;


    //refactor
    std::vector<TxSlog *> txn_queue_;
    size_t next_execute_index_;

    std::vector<ChronosSendOutputReq> pending_dep_values_;//txn content not arrived yet.

    void OnSendTxnOutput(const txnid_t &txn_id, const parid_t &par_id, const TxnOutput &output);

    void ExecTxnQueue();

    void OnSendDepValues(const ChronosSendOutputReq &chr_req,
                         ChronosSendOutputRes *chr_res);

    void CheckPendingDeps();

};
}// namespace rococo

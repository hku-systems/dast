//
// Created by micha on 2020/3/23.
//


#pragma once
#include "deptran/brq/sched.h"
#include "deptran/rcc_rpc.h"
#include "deptran/chronos/tx.h"
namespace rococo {

class RccGraph;
class ChronosCommo;
class TxChronos;



class SchedulerChronos : public Scheduler {
 public:


  SchedulerChronos(Frame* frame);



  int max_pending_txns_ = 5;
  void CheckExecutableTxns();

//  void UpdateReplicaInfo(siteid_t site_id, const chr_ts_t& ts, const chr_ts_t& clear_ts, const std::vector<ChrTxnInfo>& piggy_pending_txns);

  int HandleIRT(const map<parid_t, vector<SimpleCommand>>& cmds_by_par,
                  const ChronosSubmitReq &chr_req,
                  ChronosSubmitRes *chr_res,
                  TxnOutput* output,
                  const function<void()> &callback);

//  int HandleCRT(const map<parid_t, vector<SimpleCommand>>& cmds_by_par,
//                  const ChronosSubmitReq &chr_req,
//                  ChronosSubmitRes *chr_res,
//                  TxnOutput* output,
//                  const function<void()> &callback);

  int OnSubmitTxn(const map<parid_t, vector<SimpleCommand>>& cmds_by_par,
                          const ChronosSubmitReq &chr_req,
                          const int32_t &is_local,
                          ChronosSubmitRes *chr_res,
                          TxnOutput* output,
                          const function<void()> &callback);

    int OnSubmitCRT(const map<regionid_t, map<parid_t, vector<SimpleCommand>>>& cmds_by_region,
                    const ChronosSubmitReq &chr_req,
                    ChronosSubmitRes *chr_res,
                    TxnOutput* output,
                    const function<void()> &callback);

    void ProposeRemoteAck(txnid_t txn_id,
                        parid_t partition_id,
                        siteid_t site_id,
                        ChronosProposeRemoteRes &chr_res);

  void ProposeLocalACK(txnid_t txn_id,
                        siteid_t target_site,
                        ChronosProposeLocalRes &chr_res);


  void PrepareIRTACK(txnid_t txn_id,
                     parid_t target_par,
                     siteid_t target_site,
                     ChronosStoreLocalRes &chr_res);

//  void SyncAck(siteid_t from_site,
//               chr_ts_t told_ts,
//               ChronosLocalSyncRes &res);

  void OnPrepareIRT(const vector<SimpleCommand> &cmd,
               const ChronosStoreLocalReq &chr_req,
               ChronosStoreLocalRes *chr_res,
               const function<void()> &callback);

//  void OnStoreRemote(const vector<SimpleCommand> &cmd,
//                    const ChronosStoreRemoteReq &chr_req,
//                    ChronosStoreRemoteRes *chr_res,
//                    const function<void()> &callback);

//  void OnSync (const ChronosLocalSyncReq &req,
//               ChronosLocalSyncRes *res,
//               const function<void()> &callback);

  void OnProposeRemote(const vector<SimpleCommand>& cmds,
                        const ChronosProposeRemoteReq &req,
                        ChronosProposeRemoteRes *chr_res,
                        const function<void()> &callback);

  void OnProposeLocal(const vector<SimpleCommand>& cmds,
                       const DastProposeLocalReq &req,
                       ChronosProposeLocalRes *chr_res,
                       const function<void()> &callback);

  void OnRemotePrepared(const DastRemotePreparedReq &req,
                      DastRemotePreparedRes *chr_res,
                      const function<void()> &callback);

  void OnDistExe (const ChronosDistExeReq &chr_req,
                    ChronosDistExeRes* chr_res,
                    TxnOutput* output,
                    const function<void()>& callback);

  void OnSendOutput(const ChronosSendOutputReq &chr_req,
                    ChronosSendOutputRes *chr_res,
                    const function<void()>& callback);

  void DistExeAck(txnid_t txn_id, parid_t par_id, TxnOutput &output, ChronosDistExeRes &chr_res);

  void OnNotiCRT(const DastNotiCRTReq& );

  void OnNotiCommit(const DastNotiCommitReq&);

  void OnPrepareCRT(const vector<SimpleCommand>& cmds,
                        const DastPrepareCRTReq& req);
//                    const function<void()> &callback);

//  void StoreRemoteACK(txnid_t txn_id,
//                      siteid_t target_site,
//                      ChronosStoreRemoteRes& chr_res);

  ChronosCommo* commo();

//  chr_ts_t CalculateAnticipatedTs (const chr_ts_t &src_ts, const chr_ts_t &recv_ts);
  chr_ts_t GenerateChrTs(bool for_local);

  void HandlePiggyInfo(siteid_t remote_site,
                       const chr_ts_t& remote_ts,
                       const vector<ChrTxnInfo>& noti_txns,
                       const chr_ts_t& remote_delivered_ts);

  void OnIRSync(const DastIRSyncReq &req);

  void CollectNotifyTxns(siteid_t target_site, parid_t target_par, std::vector<ChrTxnInfo>& into_vector);
//  void InsertNotifiedTxns(const std::vector<ChrTxnInfo>& piggy_pending_txns);

//  std::map<chr_ts_t, txnid_t> pending_local_txns_ {}; //transactions in my region
//  std::map<chr_ts_t, txnid_t> pending_remote_txns_ {}; //transactions not in my region

  std::map<chr_ts_t, TxChronos*> ready_txns_{};

//  std::set<txnid_t> unassigned_distributed_txns_ {};
//  std::vector<txnid_t> local_txns_by_me_ {};


  std::set<txnid_t> distributed_txns_by_me_ {};

  std::set<chr_ts_t> dist_txn_tss_ {};

//  std::map<siteid_t,  chr_ts_t> local_replicas_ts_;

   std::map<siteid_t, chr_ts_t> ir_site_max_ts;
   std::map<chr_ts_t, TxChronos*> my_txns_{}; //
   std::map<siteid_t, chr_ts_t> ir_site_notified_ts;


   std::map<chr_ts_t, std::set<siteid_t>> ir_site_wait_ts_;

   std::set<siteid_t> local_replicas_;

//  std::map<siteid_t,  chr_ts_t> local_replicas_clear_ts_;

//  std::map<siteid_t, txnid_t> notified_txn_ids; //received submitted transaction are not in id order
//  std::map<siteid_t, chr_ts_t> notified_txn_ts; //but should be in ts order

//  chr_ts_t my_clock_;

//  chr_ts_t my_clear_ts_;
  bool new_input_ = false;


  void CheckRemotePrepared(TxChronos* dtxn);

  chr_ts_t last_clock_; //This is for ensuring the monotonically of the generated clock.

  //match the naming in paper
  int64_t clock_offset_;


  void IRSyncLoop();
  std::map<siteid_t, bool> ir_synced;
  int ir_sync_interval_ms_;
  std::thread ir_sync_loop_thread_;

  int n_replicas;

  uint32_t  sites_per_region_;


  chr_ts_t min_future_ts_;
  chr_ts_t max_future_ts_;

};
} // namespace janus

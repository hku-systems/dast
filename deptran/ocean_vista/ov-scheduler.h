//
// Created by micha on 2020/3/23.
//


#pragma once
#include "deptran/brq/sched.h"
#include "deptran/rcc_rpc.h"
#include "ov-txn_mgr.h"
#include <memory>
#include "ov-frame.h"
#include "ov-tx.h"
#include "ov-gossiper.h"
namespace rococo {

class OVCommo;
class OVFrame;

class SchedulerOV : public BrqSched {
 public:
  Config::SiteInfo *site_info_;
  Config *config_;
  OVGossiper *gossiper_;

  bool gossiper_inited_;

  SchedulerOV(Config::SiteInfo *site_info);

  void SetFrame(Frame* frame){
    verify(frame != nullptr);
    this->frame_ = frame;
  }

  int OnDispatch(const vector<SimpleCommand> &cmd,
                 rrr::i32 *res,
                 TxnOutput *output);

  void OnStore(txnid_t txnid,
               const vector<SimpleCommand> &cmds,
               const OVStoreReq &ov_req,
               int32_t *res,
               OVStoreRes *ov_res);

  void OnCreateTs(txnid_t txnid,
                  int64_t *timestamp,
                  int16_t *server_id);

  void OnStoredRemoveTs(txnid_t txnid,
                        int64_t timestamp,
                        int16_t server_id,
                        int32_t *res);

  void OnExecute(txnid_t txn_id,
                 const OVExecuteReq &req,
                 int32_t *res,
                 OVExecuteRes *ov_res,
                 TxnOutput *output,
                 const function<void()> &callback);

  void OnPublish(int64_t dc_ts, int16_t dc_id, int64_t *ret_ts, int16_t *ret_id);

  void OnExchange(const std::string &dcname, int64_t dvw_ts, int16_t dvw_id,
                  int64_t *ret_ts, int16_t *ret_id);

  OVCommo *commo();

  std::unique_ptr<TidMgr> tid_mgr_;

  std::map<txnid_t, TxOV *> stored_txns_by_id_;

  ov_ts_t vwatermark_;
};
} // namespace janus

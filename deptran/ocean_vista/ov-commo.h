//

#pragma once
#include "brq/commo.h"

namespace rococo {
class ov_ts_t;
class OVCommo : public BrqCommo {
 public:
  using BrqCommo::BrqCommo;

  void SendCreateTs(txnid_t txn_id,
                    const function<void(int64_t ts_raw, siteid_t server_id)> &);

  void SendStoredRemoveTs(txnid_t txn_id, int64_t timestamp, int16_t site_id,
                          const function<void(int res)> &);

  void BroadcastStore(parid_t par_id,
                      txnid_t txn_id,
                      vector<SimpleCommand> &cmds,
                      OVStoreReq &req,
                      const function<void(int, OVStoreRes &)> &callback);

  void SendDispatch(vector<SimpleCommand> &cmd,
                    const function<void(int res,
                                        TxnOutput &output)> &);

  void BroadcastExecute(
      parid_t par_id,
      txnid_t cmd_id,
      OVExecuteReq &chr_req,
      const function<void(int32_t, OVExecuteRes &, TxnOutput &)> &callback);

  void SendPublish(siteid_t siteid,
                   const ov_ts_t &dc_vwm,
                   const function<void(const ov_ts_t &)> &);

  void SendExchange(siteid_t target_siteid,
                    const std::string &my_dcname,
                    const ov_ts_t &my_dvw,
                    const function<void(const ov_ts_t &)> &);

};

} // namespace


//
// Created by micha on 2020/4/7.
//

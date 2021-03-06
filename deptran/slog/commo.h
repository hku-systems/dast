//
// Created by micha on 2020/3/23.
//

#pragma once
#include "brq/commo.h"

namespace rococo {

class SlogCommo : public BrqCommo {
 public:
  using BrqCommo::BrqCommo;

  void SendHandoutRo(SimpleCommand& cmd,
                     const function<void(int res,
                                         SimpleCommand& cmd,
                                         map<int, mdb::version_t>& vers)>&)
  override;

  //xs's code
  void SubmitTxn(map<parid_t, vector<SimpleCommand>>& cmd,
                      bool is_irt,
                      parid_t home_par,
                    const function<void(TxnOutput& output)>&)  ;

  void SendToRegionManager(const map<parid_t, vector<SimpleCommand>>& cmd,
                           siteid_t target_site,
                           siteid_t hander_site);

  void SubmitDistributedReq(parid_t home_par,
                      map<parid_t, vector<SimpleCommand>>& cmd,
                      const function<void(TxnOutput& output)>&)  ;

  void SendReplicateLocal(siteid_t target_site,
                    const vector<SimpleCommand>& cmd,
                    txnid_t txn_id,
                    uint64_t index,
                    uint64_t commit_index,
                    const function<void()>&)  ;


  void SendDependentValues(parid_t target_partition,
                  const ChronosSendOutputReq &req);

  void SendBatchOutput(const std::vector<std::pair<txnid_t, TxnOutput>>& batch, parid_t my_par_id);

  void SendToRaft(const map<parid_t, vector<SimpleCommand>>& cmd,
                            siteid_t handler_site);

  void SendTxnOutput(siteid_t target_site, parid_t my_parid, txnid_t txnid, TxnOutput* output);
};

} // namespace



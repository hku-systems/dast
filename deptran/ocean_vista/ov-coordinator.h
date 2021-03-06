//
// Created by micha on 2020/3/23.
//
#pragma once

#include "../__dep__.h"
#include "../constants.h"
#include "../command.h"
#include "../brq/coord.h"
#include "ov-txn_mgr.h"




namespace rococo {
class OVCommo;
class CoordinatorOV : public BrqCoord {
 public:
//  enum Phase {CHR_INIT=0, CHR_DISPATCH=1, CHR_FAST=2, CHR_FALLBACK=3, CHR_COMMIT=4};
  enum Phase {OV_INIT =0, OV_CREATED_TS = 1, OV_DISPATHED = 2, OV_STORED = 3, OV_EXECUTE = 4, OV_END = 5};

  using BrqCoord::BrqCoord;


  virtual ~CoordinatorOV() {}

  OVCommo *commo();
  // Dispatch inherits from RccCoord;



  bool TxnStored();

  void Reset() override;

  void OVStore();
  void OVStoreACK(phase_t phase,
                  parid_t par_id,
                  int res,
                  OVStoreRes& ov_res);



  void CreateTs();
  void CreateTsAck(phase_t phase,
                    int64_t ts_raw,
                    siteid_t server_id);


  void StoredRemoveTs();
  void StoredRemoveTsAck(phase_t phase, int res);

  void Dispatch();
  void DispatchAck(phase_t phase,
                   int res,
                   TxnOutput& cmd);



  void Execute();
  void ExecuteAck(phase_t phase,
                 parid_t par_id,
                 int32_t res,
                 OVExecuteRes &chr_res,
                 TxnOutput &output);


  void GotoNextPhase() override;

  map<parid_t, int> n_ov_execute_acks_ = {};

  map<parid_t, int> n_store_acks_{};

  ov_ts_t my_ovts_;

};
} // namespace janus


//
// Created by micha on 2020/3/23.
//

#include "../__dep__.h"
#include "txn_chopper.h"
#include "ov-frame.h"
#include "ov-commo.h"
#include "ov-coordinator.h"

namespace rococo {

OVCommo *CoordinatorOV::commo() {
  if (commo_ == nullptr) {
    commo_ = frame_->CreateCommo();
    commo_->loc_id_ = loc_id_;
  }
  verify(commo_ != nullptr);
  return dynamic_cast<OVCommo *>(commo_);
}

void CoordinatorOV::OVStore() {
  std::lock_guard<std::recursive_mutex> guard(mtx_);
  for (auto par_id : cmd_->GetPartitionIds()) {
    auto cmds = txn().GetCmdsByPartition(par_id);
    OVStoreReq req;
    req.ts = my_ovts_.timestamp_;
    req.site_id = my_ovts_.site_id_;
    Log_debug("[coord %u], %s called, txn_id = %lu, ts = %ld.%d, par_id = %u",
              this->coo_id_,
              __FUNCTION__,
              cmd_->id_,
              req.ts,
              req.site_id,
              par_id);

    commo()->BroadcastStore(par_id,
                            cmd_->id_,
                            cmds,
                            req,
                            std::bind(&CoordinatorOV::OVStoreACK,
                                      this,
                                      phase_,
                                      par_id,
                                      std::placeholders::_1,
                                      std::placeholders::_2));
  }
}

void CoordinatorOV::OVStoreACK(phase_t phase, parid_t par_id, int res, OVStoreRes &ov_res) {
  std::lock_guard<std::recursive_mutex> guard(mtx_);
  if (phase != phase_) {
    //Log_info("already in next phase, return. Current phase = %d, suppose to be %d", phase_, phase);
    return;
  }

  if (res == SUCCESS) {
    n_store_acks_[par_id]++;
    Log_debug("Received store ok for par_id: %d", par_id);
  } else if (res == REJECT) {
    Log_debug("Reject not handled yet");
    verify(0);
  } else {
    verify(0);
  }

  if (TxnStored()) {
    //Received quorum ACK from all participating partitions.
    Log_debug("[Txn id = %d] store ok", this->txn().id_);
    GotoNextPhase();
  }
}

bool CoordinatorOV::TxnStored() {
  auto pars = txn().GetPartitionIds();
  for (auto &par_id : pars) {
    if (n_store_acks_.count(par_id) < 1) {
      //xstodo: temporarily chenged to 1 for code refinement
      return false;
    }
  }
  return true;
}

void CoordinatorOV::StoredRemoveTs() {
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  auto txn = (TxnCommand *) cmd_;
  verify(txn->root_id_ == txn->id_);
  auto callback = std::bind(&CoordinatorOV::StoredRemoveTsAck,
                            this,
                            phase_,
                            std::placeholders::_1);
  commo()->SendStoredRemoveTs(txn->id_, my_ovts_.timestamp_, my_ovts_.site_id_, callback);

}

void CoordinatorOV::StoredRemoveTsAck(phase_t phase, int res) {
  std::lock_guard<std::recursive_mutex> lock(this->mtx_);
  verify(phase == phase_); // cannot proceed without all acks.
  verify(txn().root_id_ == txn().id_);
  verify(res == SUCCESS);
  Log_debug("[txn %d] stored, removed ts from manager", cmd_->id_);
  GotoNextPhase();
}

void CoordinatorOV::CreateTs() {
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  auto txn = (TxnCommand *) cmd_;
  verify(txn->root_id_ == txn->id_);

  auto callback = std::bind(&CoordinatorOV::CreateTsAck,
                            this,
                            phase_,
                            std::placeholders::_1,
                            std::placeholders::_2);
  commo()->SendCreateTs(txn->id_, callback);
}

void CoordinatorOV::CreateTsAck(phase_t phase, int64_t ts_raw, siteid_t site_id) {
  std::lock_guard<std::recursive_mutex> lock(this->mtx_);
  verify(phase == phase_); // cannot proceed without all acks.
  verify(txn().root_id_ == txn().id_);

  Log_debug("%s called, ts_raw = %ld, site_id = %d", __FUNCTION__, ts_raw, site_id);

  my_ovts_.timestamp_ = ts_raw;
  my_ovts_.site_id_ = site_id;
  GotoNextPhase();
}

void CoordinatorOV::Execute() {
  Log_debug("%s called", __FUNCTION__);
  std::lock_guard<std::recursive_mutex> guard(mtx_);
  OVExecuteReq req;
  for (auto par_id : cmd_->GetPartitionIds()) {
    commo()->BroadcastExecute(par_id,
                              cmd_->id_,
                              req,
                              std::bind(&CoordinatorOV::ExecuteAck,
                                        this,
                                        phase_,
                                        par_id,
                                        std::placeholders::_1,
                                        std::placeholders::_2,
                                        std::placeholders::_3));
  }
}

void CoordinatorOV::ExecuteAck(uint32_t phase,
                               uint32_t par_id,
                               int32_t res,
                               OVExecuteRes &chr_res,
                               TxnOutput &output) {

  std::lock_guard<std::recursive_mutex> guard(mtx_);

  if (phase != phase_) return;
  if (res == SUCCESS) {
    committed_ = true;
  } else if (res == REJECT) {
    verify(0);
  } else {
    verify(0);
  }
  n_ov_execute_acks_[par_id]++;
  if (n_commit_oks_[par_id] > 1) {
    //Redundant
    return;
  }
  txn().Merge(output);
  // if collect enough results.
  // if there are still more results to collect.
  bool all_acked = txn().OutputReady();
  if (all_acked) {
    GotoNextPhase();
  }
}

void CoordinatorOV::Dispatch() {

  verify(ro_state_ == BEGIN);
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  auto txn = (TxnCommand *) cmd_;
  verify(txn->root_id_ == txn->id_);
  int cnt = 0;
//  Log_info("%s called", __PRETTY_FUNCTION__);

  map<parid_t, vector<SimpleCommand *>> cmds_by_par = txn->GetReadyCmds();
  Log_debug("transaction (id %d) has %d ready pieces", txn->id_, cmds_by_par.size());

  int index = 0;
  for (auto &pair: cmds_by_par) {

    const parid_t &par_id = pair.first;
    auto &cmds = pair.second;
    n_dispatch_ += cmds.size();
    cnt += cmds.size();
    vector<SimpleCommand> cc;
    for (auto c: cmds) {
      c->id_ = next_pie_id(); //next_piece_id
      dispatch_acks_[c->inn_id_] = false;
      cc.push_back(*c);
    }

    auto callback = std::bind(&CoordinatorOV::DispatchAck,
                              this,
                              phase_,
                              std::placeholders::_1,
                              std::placeholders::_2);

    commo()->SendDispatch(cc, callback);
  }
}

void CoordinatorOV::DispatchAck(phase_t phase,
                                int res,
                                TxnOutput &output) {

  std::lock_guard<std::recursive_mutex> lock(this->mtx_);
  verify(phase == phase_); // cannot proceed without all acks.
  verify(txn().root_id_ == txn().id_);

  for (auto &pair : output) {
    n_dispatch_ack_++;
    verify(dispatch_acks_[pair.first] == false);
    dispatch_acks_[pair.first] = true;
    txn().Merge(pair.first, pair.second); //For those txn that need the read value for other command.
    Log_debug("get start ack %ld/%ld for cmd_id: %lx, inn_id: %d",
              n_dispatch_ack_, n_dispatch_, txn().id_, pair.first);
  }

  if (txn().HasMoreSubCmdReadyNotOut()) {
    Log_debug("command has more sub-cmd, cmd_id: %lx,"
              " n_started_: %d, n_pieces: %d",
              txn().id_,
              txn().n_pieces_dispatched_, txn().GetNPieceAll());
    Dispatch();
  } else if (AllDispatchAcked()) {
    //xs: this is for OCC + Paxos based method.
    Log_debug("receive all start acks, txn_id: %llx; START PREPARE", cmd_->id_);
    verify(!txn().do_early_return());
    GotoNextPhase();
  }
}

void CoordinatorOV::GotoNextPhase() {

  int n_phase = 6;
  int current_phase = phase_++ % n_phase; // for debug
  Log_debug("--------------------------------------------------");
  Log_debug("%s: phase = %d", __FUNCTION__, current_phase);

  switch (current_phase) {
    case Phase::OV_INIT:

      CreateTs();
      verify(phase_ % n_phase == OV_CREATED_TS);
      break;
    case Phase::OV_CREATED_TS: //1

      Dispatch();
      verify(phase_ % n_phase == Phase::OV_DISPATHED);
      break;

    case Phase::OV_DISPATHED: //2

      OVStore();
      verify(phase_ % n_phase == Phase::OV_STORED);
      break;

    case Phase::OV_STORED: //3
      StoredRemoveTs();
      verify(phase_ % n_phase == Phase::OV_EXECUTE);
      break;
    case Phase::OV_EXECUTE: // 4

      Execute();
      verify(phase_ % n_phase == Phase::OV_END);
      break;
    case Phase::OV_END: //5
      verify(phase_ % n_phase == Phase::OV_INIT); //overflow

      Log_debug("[txn %d],type = %d all acked, end", txn().txn_id_, txn().type_);
      End();
      break;

    default:verify(0);
  }

}

void CoordinatorOV::Reset() {
  RccCoord::Reset();

  committed_ = false;
  n_ov_execute_acks_.clear();
  n_store_acks_.clear();
}

} // namespace janus

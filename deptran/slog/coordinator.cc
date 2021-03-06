//
// Created by micha on 2020/3/23.
//

#include "../__dep__.h"
#include "txn_chopper.h"
#include "frame.h"
#include "commo.h"
#include "coordinator.h"



namespace rococo {


SlogCommo *CoordinatorSlog::commo() {
  if (commo_ == nullptr) {
    commo_ = frame_->CreateCommo();
    commo_->loc_id_ = loc_id_;
  }
  verify(commo_ != nullptr);
  return dynamic_cast<SlogCommo *>(commo_);
}

void CoordinatorSlog::launch_recovery(cmdid_t cmd_id) {
  // TODO
  prepare();
}



void CoordinatorSlog::SubmitReq() {

  slog_submit_ts_ =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

  verify(ro_state_ == BEGIN);
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  auto txn = (TxnCommand *) cmd_;
  verify(txn->root_id_ == txn->id_);
  int cnt = 0;
  txn->PrepareAllCmds();
  map<parid_t, vector<SimpleCommand*>> cmds_by_par = txn->GetAllCmds();
  Log_debug("transaction (id %d) has %d ready pieces", txn->id_, cmds_by_par.size());

  bool is_local = my_txn_is_loacal_;
  siteid_t home_region = home_par_id_;

  std::set<parid_t> all_regions;
  for (auto &pair: cmds_by_par){
    all_regions.insert(pair.first);
  }
  //int index = 0;


//  if (is_local){
      map<parid_t, vector<SimpleCommand>> cmds_to_send;
      for (auto &pair: cmds_by_par) {
          parid_t par_id = pair.first;
          auto &cmds = pair.second;
          n_dispatch_ += cmds.size();
          cnt += cmds.size();
          verify(cmds_to_send.count(par_id) == 0);
          cmds_to_send[par_id] = vector<SimpleCommand>();
          for (auto c: cmds) {
              c->id_ = next_pie_id(); //next_piece_id
              dispatch_acks_[c->inn_id_] = false;
              cmds_to_send[par_id].push_back(*c);
          }
      }



    for (auto& pair: cmds_to_send){
        for (auto &c: pair.second){
            Log_debug("herehere, id = %d, input ready = %d, input values size = %d", c.inn_id(), c.input.piece_input_ready_, c.input.values_->size());
        }
    }

    Log_debug("[coord %u] submit txn %lu", this->coo_id_, txn->id_);

    auto callback = std::bind(&CoordinatorSlog::SubmitAck,
                              this,
                              phase_,
                              std::placeholders::_1);
    //Currently nothing in the feild
    commo()->SubmitTxn(cmds_to_send,
                       is_local,
                       home_par_id_,
                            callback);

//  }else{
//    //Submit remote requests;
//    Log_debug("[coo_id_ = %u] submit distributed txn %lu", this->coo_id_, txn->id_);
//    map<parid_t, vector<SimpleCommand>> cmds_to_send; //xs: cmd_by_par has pointer to SimpleCommand, not sure why doing so
//    for (auto &pair: cmds_by_par) {
//      parid_t par_id = pair.first;
//      auto &cmds = pair.second;
//      n_dispatch_ += cmds.size();
//      cnt += cmds.size();
//      verify(cmds_to_send.count(par_id) == 0);
//      cmds_to_send[par_id] = vector<SimpleCommand>();
//      for (auto c: cmds) {
//        c->id_ = next_pie_id(); //next_piece_id
//        dispatch_acks_[c->inn_id_] = false;
//        cmds_to_send[par_id].push_back(*c);
//      }
//    }
//    auto callback = std::bind(&CoordinatorSlog::SubmitAck,
//                              this,
//                              phase_,
//                              std::placeholders::_1);
//
//    commo()->SubmitDistributedReq(home_region, cmds_to_send, callback);
//  }
//  Log_debug("%s returned", __FUNCTION__);
}

void CoordinatorSlog::SubmitAck(phase_t phase,
                                     TxnOutput &output) {

  std::lock_guard<std::recursive_mutex> lock(this->mtx_);
  verify(phase == phase_); // cannot proceed without all acks.
  verify(txn().root_id_ == txn().id_);
  Log_debug("[coord %u] %s called for txn %lu", coo_id_, __FUNCTION__, txn().id_);
  committed_ = true;
  uint32_t type = txn().type();
  int64_t now =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();


  if (my_txn_is_loacal_){
    Log_info("txn %lu, Local Latency =%ld= ms, type %u", txn().root_id_, now-slog_submit_ts_, type);
  }
  else{
    Log_info("txn %lu, Distributed Latency =%ld= ms, type %u", txn().root_id_, now-slog_submit_ts_, type);
  }

  GotoNextPhase();

}

void CoordinatorSlog::GotoNextPhase() {

  int n_phase = 2;
  int current_phase = phase_++ % n_phase; // for debug

  switch (current_phase) {
    case Phase::CHR_INIT:
      /*
       * Collect the local-DC timestamp.
       * Try to make my clock as up-to-date as possible.
       */
      SubmitReq();
      verify(phase_ % n_phase == Phase::CHR_COMMIT);
      break;

    case Phase::CHR_COMMIT: //4

      verify(phase_ % n_phase == Phase::CHR_INIT); //overflow
      if (committed_) {
        End();
      } else if (aborted_) {
        Restart();
      } else {
        verify(0);
      }
      break;

    default:verify(0);
  }

}

void CoordinatorSlog::Reset() {
  RccCoord::Reset();
  fast_path_ = false;
  fast_commit_ = false;
  n_fast_accept_graphs_.clear();
  n_fast_accept_oks_.clear();
  n_accept_oks_.clear();
  fast_accept_graph_check_caches_.clear();
  n_commit_oks_.clear();
  //xstodo: think about how to forward the clock
}



} // namespace janus

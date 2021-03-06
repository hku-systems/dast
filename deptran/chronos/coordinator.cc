//
// Created by micha on 2020/3/23.
//

#include "../__dep__.h"
#include "txn_chopper.h"
#include "frame.h"
#include "commo.h"
#include "coordinator.h"


namespace rococo {


ChronosCommo *CoordinatorChronos::commo() {
  if (commo_ == nullptr) {
    commo_ = frame_->CreateCommo();
    commo_->loc_id_ = loc_id_;
  }
  verify(commo_ != nullptr);
  return dynamic_cast<ChronosCommo *>(commo_);
}

void CoordinatorChronos::launch_recovery(cmdid_t cmd_id) {
  // TODO
  prepare();
}



void CoordinatorChronos::SubmitReq() {


  verify(ro_state_ == BEGIN);
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  auto txn = (TxnCommand *) cmd_;
  verify(txn->root_id_ == txn->id_);
  int cnt = 0;
  txn->PrepareAllCmds();
  map<parid_t, vector<SimpleCommand*>> cmds_by_par = txn->GetAllCmds();

  bool is_irt = my_txn_is_loacal_;
  siteid_t home_region = home_par_id_;

  string touched_par_ids = "";
  for (auto &itr: cmds_by_par){
      touched_par_ids = touched_par_ids + std::to_string(itr.first) + " ";
  }

  Log_info("Submitting request: (id = %lu); ready pieces = %d; is_local = %d; home_par = %u,  touched pars = %s",
           txn->id_,
           cmds_by_par.size(),
           is_irt,
           home_par_id_,
           touched_par_ids.c_str());
//  std::set<parid_t> all_regions;
//  for (auto &pair: cmds_by_par){
//    all_regions.insert(pair.first);
//  }




//  if (is_irt){
//    auto pair = cmds_by_par.begin();
//    const parid_t &par_id = pair->first;
//    auto &cmds = pair->second;
//    n_dispatch_ += cmds.size();
//    cnt += cmds.size();
//    vector<SimpleCommand> cc;
//    for (auto c: cmds) {
//      Log_info("here, id = %d, input ready = %d, input values size = %d, home_region = %u", c->inn_id(), c->input.piece_input_ready_, c->input.values_->size(), home_region);
//      c->id_ = next_pie_id(); //next_piece_id
//      dispatch_acks_[c->inn_id_] = false;
//      cc.push_back(*c);
//    }
//
//    auto callback = std::bind(&CoordinatorChronos::SubmitAck,
//                              this,
//                              phase_,
//                              std::placeholders::_1,
//                              std::placeholders::_2);
//    ChronosSubmitReq req;
//    //Currently nothing in the feild
//
//    commo()->SubmitLocalReq(cc, req, callback);
//    chr_submit_ts =
//        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
//  }else{
    if (is_irt) {
        map<parid_t, vector<SimpleCommand>> cmds_to_send; //xs: cmd_by_par has pointer to SimpleCommand, not sure why doing so
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
        auto callback = std::bind(&CoordinatorChronos::SubmitAck,
                                  this,
                                  phase_,
                                  std::placeholders::_1,
                                  std::placeholders::_2);
        ChronosSubmitReq req;
        if (cmds_by_par.count(home_par_id_) != 0){
            commo()->SubmitTxn(cmds_to_send, home_par_id_, is_irt, req, callback);
        }
        else{
            auto r = RandomGenerator::rand(0, cmds_by_par.size()-1);
            auto target = cmds_by_par.begin();
            std::advance(target, r);

            commo()->SubmitTxn(cmds_to_send, target->first, is_irt, req, callback);
        }

        Log_info("[coo_id_ = %u] submitting IRT %lu", this->coo_id_, txn->id_);
    }else {
        uint sites_per_region = Config::GetConfig()->n_shard_per_region_ * Config::GetConfig()->GetPartitionSize(0);


        map<regionid_t, map<parid_t, vector<SimpleCommand>>> cmds_by_region;
        for (auto &pair: cmds_by_par) {
            parid_t par_id = pair.first;
            auto &cmds = pair.second;
            n_dispatch_ += cmds.size();
            cnt += cmds.size();
            regionid_t rid = par_id / Config::GetConfig()->n_shard_per_region_;

            cmds_by_region[rid][par_id] = vector<SimpleCommand>();
            for (auto c: cmds) {
                c->id_ = next_pie_id(); //next_piece_id
                dispatch_acks_[c->inn_id_] = false;
                cmds_by_region[rid][par_id].push_back(*c);
            }
        }

        auto callback = std::bind(&CoordinatorChronos::SubmitAck,
                                  this,
                                  phase_,
                                  std::placeholders::_1,
                                  std::placeholders::_2);
        ChronosSubmitReq req;
        if (cmds_by_par.count(home_par_id_) != 0){
            commo()->SubmitCRT(cmds_by_region, home_par_id_,  req, callback);
        }
        else{
            regionid_t rid = home_par_id_ / Config::GetConfig()->n_shard_per_region_;
            auto target = cmds_by_region[rid].begin();
            auto r = RandomGenerator::rand(0, cmds_by_region[rid].size()-1);
            std::advance(target, r);
            commo()->SubmitCRT(cmds_by_region, target->first,  req, callback);
        }



        Log_info("[coo_id_ = %u] submitting CRT %lu", this->coo_id_, txn->id_);
    }

    chr_submit_ts =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

}

void CoordinatorChronos::SubmitAck(phase_t phase,
                                     TxnOutput &output,
                                     ChronosSubmitRes &chr_res) {

  std::lock_guard<std::recursive_mutex> lock(this->mtx_);
  verify(phase == phase_); // cannot proceed without all acks.
  verify(txn().root_id_ == txn().id_);
  Log_info("%s called, Txn %lu received reply", __FUNCTION__ , txn().id_);
  committed_ = true;

  uint32_t type = txn().type();
  int64_t now =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

  if (my_txn_is_loacal_){
    Log_info("txn %lu, Local Latency =%ld= ms, type %u", txn().root_id_, now-chr_submit_ts, type);
  }
  else{
    Log_info("txn %lu, Distributed Latency =%ld= ms, type %u", txn().root_id_, now-chr_submit_ts, type);
  }

  GotoNextPhase();
//  for (auto &pair : output) {
//    n_dispatch_ack_++;
//    verify(dispatch_acks_[pair.first] == false);
//    dispatch_acks_[pair.first] = true;
//    txn().Merge(pair.first, pair.second); //For those txn that need the read value for other command.
////    Log_info("get start ack %ld/%ld for cmd_id: %lx, inn_id: %d",
////             n_dispatch_ack_, n_dispatch_, txn().id_, pair.first);
//  }
//
//
//  if (txn().HasMoreSubCmdReadyNotOut()) {
//    Log_info("command has more sub-cmd, cmd_id: %lx,"
//             " n_started_: %d, n_pieces: %d",
//             txn().id_,
//             txn().n_pieces_dispatched_, txn().GetNPieceAll());
//    SubmitReq();
//  } else if (AllDispatchAcked()) {
//    //xs: this is for OCC + Paxos based method.
//
//    verify(!txn().do_early_return());
//    Log_info("[coo_id_ = %u] receive output for txn id = %lu", this->coo_id_, txn().id_);
//    committed_ = true;
//    GotoNextPhase();
//  }
//  else{
//    Log_info("here");
//  }
}

void CoordinatorChronos::GotoNextPhase() {

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

    case Phase::CHR_COMMIT://4

      verify(phase_ % n_phase == Phase::CHR_INIT); //overflow
      if (committed_) {
        Log_debug("txn %lu commited", txn().id_);
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

void CoordinatorChronos::Reset() {
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

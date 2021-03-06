//
// Created by micha on 2020/3/23.
//


#include "deptran/slog/tx.h"
#include "memdb/row_mv.h"

#include "bench/tpcc/piece.h"

namespace rococo {


static char * defer_str[] = {"defer_real", "defer_no", "defer_fake"};
static char * hint_str[] = {"n/a", "bypass", "instant", "n/a",  "deferred"};
//SimpleCommand is a typedef of TxnPieceData
//add a simpleCommand to the local Tx's dreq
void TxSlog::DispatchExecute(const SimpleCommand &cmd,
                                int32_t *res,
                                map<int32_t, Value> *output) {

  //xs: for debug;
  this->root_type = cmd.root_type_;

  //xs: Step 1: skip this simpleCommand if it is already in the dreqs.
  for (auto& c: dreqs_) {
    if (c.inn_id() == cmd.inn_id()) // already handled?
      return;
  }


  verify(txn_reg_);
  // execute the IR actions.
  auto pair = txn_reg_->get(cmd);
  // To tolerate deprecated codes

  //Log_debug("%s called, defer= %s, txn_id = %hu, root_type = %d, type = %d" , __FUNCTION__, defer_str[pair.defer], cmd.root_id_, cmd.root_type_, cmd.type_);
  int xxx, *yyy;
  if (pair.defer == DF_REAL) {
    yyy = &xxx;
    dreqs_.push_back(cmd);
  } else if (pair.defer == DF_NO) {
    yyy = &xxx;
  } else if (pair.defer == DF_FAKE) {
    //Log_debug("%s returned, defer= %s, txn_id = %hu, root_type = %d, type = %d" , __FUNCTION__, defer_str[pair.defer], cmd.root_id_, cmd.root_type_, cmd.type_);
    dreqs_.push_back(cmd);
    return;
//    verify(0);
  } else {
    verify(0);
  }
  pair.txn_handler(nullptr,
                   this,
                   const_cast<SimpleCommand&>(cmd),
                   yyy,
                   *output);
  *res = pair.defer;

  //Log_debug("%s returned, defer= %s, txn_id = %hu, root_type = %d, type = %d" , __FUNCTION__, defer_str[pair.defer], cmd.root_id_, cmd.root_type_, cmd.type_);
}
std::map<parid_t, std::map<varid_t, Value>>
TxSlog::RecursiveExecuteReadyPieces(parid_t my_par_id, std::set<innid_t>& pending_pieces) {
  if (this->exe_tried_ == false) {
    this->exe_tried_ = true;

#ifdef LAT_BREAKDOWN
    handler_local_start_exe_ts =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#endif //LAT_BREAKDOWN
  }


  recur_times_++;

  std::map<parid_t, std::map<varid_t, Value>> ret;
  int32_t exe_res;
  bool need_check;
  do {
    need_check = false;
    for (auto &c : pieces_) {
      bool no_conflict = can_exe_wo_conflict(c.inn_id(), pending_pieces);
      if (c.input.piece_input_ready_ && c.local_executed_ == false && no_conflict) {
        Log_debug("executing piece %d of txn %lu", c.inn_id(), id());
//        pending_piecesc.inn_id()].erase(id());
//        Log_debug("pending pieces removed piece %u of txn %lu, count = %d", c.inn_id(), id(), pending_pieces[c.inn_id()].size());

        map<int32_t, Value> piece_output;
        DispatchExecute(const_cast<SimpleCommand &>(c),
                        &exe_res, &piece_output);
        verify(c.local_executed_ == false);
        c.local_executed_ = true;
        n_executed_pieces++;
        auto targets = GetOutputsTargets(&piece_output);
        for (auto& tar: targets){
          ret[tar].insert(piece_output.begin(), piece_output.end());
        }
        if (targets.count(my_par_id) != 0) {
          //The output is also required by my partition
          bool has_more_ready = MergeCheckReadyPieces(piece_output);
          DEP_LOG("has more ready = %d", has_more_ready);
          need_check = need_check || has_more_ready;
        }
        (*dist_output_)[c.inn_id()] = piece_output;
      } else {
        Log_debug("Not executing piece %d of txn %lu, for now, input_ready = %d, executed = %d",
                  c.inn_id(),
                  id(),
                  c.input.piece_input_ready_,
                  c.local_executed_);
//        if (c.local_executed_ == false){
//           for (auto & pair: c.input.input_var_ready_){
//             if (pair.second == 0){
//               Log_debug("Key %d not ready", pair.first);
//             }
//           }
//        }
      }
    }
  } while (need_check == true);

  for (auto &c: pieces_) {
    if (c.local_executed_ == false) {
      pending_pieces.insert(c.inn_id());
      Log_debug("pending pieces add piece %u of txn %lu, count = %d", c.inn_id(), id(), pending_pieces.size());
    }
  }

  return ret;




//  std::map<parid_t, std::map<varid_t, Value>> ret;
//  int32_t exe_res;
//  bool need_check;
//  do {
//    need_check = false;
//    for (auto &c : pieces_) {
//      if (c.input.piece_input_ready_ && c.local_executed_ == false) {
//        Log_debug("executing piece %d of txn %lu", c.inn_id(), id());
//        map<int32_t, Value> piece_output;
//        DispatchExecute(const_cast<SimpleCommand &>(c),
//                              &exe_res, &piece_output);
//        verify(c.local_executed_ == false);
//        c.local_executed_ = true;
//        n_executed_pieces++;
//        Log_debug("output size = %d", piece_output.size());
//        for (auto &o: piece_output){
//          Log_debug("output has var %d", o.first);
//        }
//        auto targets = GetOutputsTargets(&piece_output);
//        for (auto& tar: targets){
//           ret[tar].insert(piece_output.begin(), piece_output.end());
//        }
//        if (targets.count(my_par_id) != 0) {
//          //The output is also required by my partition
//          bool has_more_ready = MergeCheckReadyPieces(piece_output);
//          DEP_LOG("has more ready = %d", has_more_ready);
//          need_check = need_check || has_more_ready;
//        }
//        (*dist_output_)[c.inn_id()] = piece_output;
//      } else {
//        Log_debug("Not executing piece %d of txn %lu, for now, input_ready = %d, executed = %d",
//                 c.inn_id(),
//                 id(),
//                 c.input.piece_input_ready_,
//                 c.local_executed_);
//        for (auto &p: c.input.input_var_ready_){
//          Log_debug("piece %u, var %d, ready %d", c.inn_id(), p.first, p.second);
//        }
//      }
//    }
//  } while (need_check == true);
//  return ret;
}
bool TxSlog::check_piece_conflict(uint32_t pending_pie_id, uint32_t type_to_check) {
//  return true;
  if (Config::GetConfig()->benchmark_ == TPCC){
    std::set<std::pair<innid_t, innid_t>>& conf = TpccPiece::conflicts_;
    if (conf.count(std::pair<innid_t, innid_t>(pending_pie_id, type_to_check)) != 0){
//      Log_debug("%u and %u conflict", pending_pie_id, type_to_check);
      return true;
    }
    else{
//      Log_debug("%u and %u do not conflict", pending_pie_id, type_to_check);
      return false;
    }
  }
  else{
    return false;
  }
}
bool TxSlog::can_exe_wo_conflict(uint32_t to_check, const set<uint32_t> &pending_pieces) {
  for (auto &p: pending_pieces){
    if (check_piece_conflict(p, to_check)){
      Log_debug("Piece %u of txn %lu cannot run, because it conflicts with %u", to_check, id(), p);
      return false;
    }
  }
  Log_debug("Piece %u of txn %lu can run, not conflicts", to_check, id());
  return true;
}

std::set<parid_t> TxSlog::GetOutputsTargets(std::map<int32_t, Value> *output){
  std::set<parid_t> ret;
  for (auto& pair: *output){
     varid_t varid = pair.first;
     auto& var_par_map = this->pieces_[0].waiting_var_par_map;
     if (var_par_map.count(varid) != 0){
         for (auto& target: var_par_map[varid]){
           ret.insert(target);
           Log_debug("var %d is required by partition %u", varid, target);
         }
     }
  }
  return ret;
}


bool TxSlog::MergeCheckReadyPieces(const TxnOutput& output){
  bool ret = false;
  for (auto& pair: output){
    Log_debug("chekcing piece %u", pair.first);
    ret =  MergeCheckReadyPieces(pair.second) || ret; 
  }
  return ret;
}


bool TxSlog::MergeCheckReadyPieces(const std::map<int32_t, Value> &output) {
  bool ret = false;
  for (auto& pair: output){
    varid_t varid = pair.first;
    Value value = pair.second;
    Log_debug("Recevied var %d check if necessary", varid);
    for (auto& p: this->pieces_){
      if (p.input.piece_input_ready_ == 0){
        //This piece not ready
        if (p.input.keys_.count(varid) != 0){
           //This piece need this var
           if (p.input.input_var_ready_[varid] == 0){
             //This var is not ready yet
             (*p.input.values_)[varid] = value;
             p.input.input_var_ready_[varid] = 1;
             bool ready = true;
             for (auto &re: p.input.input_var_ready_){
               if (re.second == 0){
                 DEP_LOG("%s, var %d is not ready yet for piece %u",
                     __FUNCTION__ ,
                     varid,
                     p.inn_id());
                 ready = false;
               }
             }
             if (ready){
               DEP_LOG("%s, piece %u is ready for execution",
                   __FUNCTION__ ,
                   p.inn_id());
               p.input.piece_input_ready_ = 1;
               ret = true;
             }
           }
           else{
             //This var is already ready.
             Log_warn("%s, Var %d is arleady ready for piece %u",
                 __FUNCTION__ ,
                 varid,
                 p.inn_id());
              //do nothing
           }
        }
      }
    }
  }
  return ret;
}

void TxSlog::PreAcceptExecute(const SimpleCommand &cmd, int *res, map<int32_t, Value> *output) {


  //xs: Step 1: skip this simpleCommand if it is already in the dreqs.

  bool already_dispatched = false;
  for (auto& c: dreqs_) {
    if (c.inn_id() == cmd.inn_id()) {
      // already received this piece, no need to push_back again.
      already_dispatched = true;
    }
  }
  verify(txn_reg_);
  // execute the IR actions.
  auto pair = txn_reg_->get(cmd);
  // To tolerate deprecated codes

  Log_debug("%s called, defer= %s, txn_id = %d, root_type = %d, type = %d" , __FUNCTION__, defer_str[pair.defer], cmd.root_id_, cmd.root_type_, cmd.type_);
  int xxx, *yyy;
  if (pair.defer == DF_REAL) {
    yyy = &xxx;
    if (!already_dispatched){
      dreqs_.push_back(cmd);
    }
  } else if (pair.defer == DF_NO) {
    yyy = &xxx;
  } else if (pair.defer == DF_FAKE) {
    if (!already_dispatched){
      dreqs_.push_back(cmd);
    }
    /*
     * xs: don't know the meaning of DF_FAKE
     * seems cannot run pieces with DF_FAKE now, other wise it will cause segfault when retrieving input values
     */
    return;
  } else {
    verify(0);
  }
  pair.txn_handler(nullptr,
                   this,
                   const_cast<SimpleCommand&>(cmd),
                   yyy,
                   *output);
  *res = pair.defer;
}

/*
 * XS notes;
 * Bypass Read: read immutable columns (e.g., name, addr), ``not'' need to be serialized
 * Bypass Write: never used
 *
 * Instant:  Not used in RW and TPCC.
 *
 * Deferred: read/writes pieces that need to be serialized.
 *  DF_REAL: that can be executed
 *  DF_FAKE: old implementations that cannot run properly with janus's read/write mechanism,
 *           these pieces should be treated as deferred, by will cause segfault.
 *           So, these pieces will be executed in the commit phase.
 *           After all other pieces has been executed?
 */
bool TxSlog::ReadColumn(mdb::Row *row,
                           mdb::column_id_t col_id,
                           Value *value,
                           int hint_flag) {
  //Seems no instant through out the process
  //Log_debug("Rtti = %d", row->rtti());
  auto r = dynamic_cast<ChronosRow*>(row);
  verify(r->rtti() == symbol_t::ROW_CHRONOS);

  verify(!read_only_);
  if (true) {
    if (hint_flag == TXN_BYPASS || hint_flag == TXN_INSTANT) {
      //xs notes 1: seems there is no instant read in TPCC and RW benchmark.


      /*
       * xs notes 2: assumes that workload has already been re-writen to one-shot transactions,
       * s.t., this read values are immutable.
       * In such a scenario, the dispatch is only used to calibrate the timestamps.
       * Takeaways:
       * 1. No need to lock
       * 2. No need to track the timestamps for these locks.
       * (3) that's why they are named bypass?
      */
//      r ->rlock_row_by(this->tid_);
//      locked_rows_.insert(r);
      auto c = r->get_column(col_id);
      row->ref_copy();
      *value = c;


      int64_t t_pw  = r->max_prepared_wver(col_id);
      int64_t t_cw  = r->wver_[col_id];



      //xs notes 2, cont'd  no need to save prepared for this
      //prepared_read_ranges_[r][col_id] = pair<int64_t, int64_t>(t_left, received_prepared_ts_right_);
      return true;
    }
    if (hint_flag == TXN_DEFERRED) {
      //xs: seems no need to read for a deferred
      return true;
    }
  }else if (false) {
    //In the pre-accept phase
    if (hint_flag == TXN_BYPASS || hint_flag == TXN_INSTANT) {
      /*
       * xs notes
       *
       *
       */
      r->rlock_row_by(this->tid_);
      locked_rows_.insert(r);
      auto c = r->get_column(col_id);
      row->ref_copy();
      *value = c;

      int64_t t_pw = r->max_prepared_wver(col_id);
      int64_t t_cw = r->wver_[col_id];
      int64_t t_low = received_prepared_ts_left_;

      int64_t t_left = t_pw > t_cw ? t_pw : t_cw;
      t_left = t_left > t_low ? t_left : t_low;
      Log_debug(
          "[txn %d] ReadColumn, pre-accept phase1: table = %s, col_id = %d,  hint_flag = %s, tyep = %d, t_pw = %d, t_cw = %d, t_low = %d, t_left = %d, t_right = %d",
          this->id(),
          row->get_table()->Name().c_str(),
          col_id,
          hint_str[hint_flag],
          root_type,
          t_pw,
          t_cw,
          t_low,
          t_left,
          received_prepared_ts_right_);
      prepared_read_ranges_[r][col_id] = pair<int64_t, int64_t>(t_left, received_prepared_ts_right_);

      return true;
    }
    if (hint_flag == TXN_DEFERRED) {
      r->rlock_row_by(this->tid_);
      locked_rows_.insert(r);
      auto c = r->get_column(col_id);
      row->ref_copy();
      *value = c;

      int64_t t_pw = r->max_prepared_wver(col_id);
      int64_t t_cw = r->wver_[col_id];
      int64_t t_low = received_prepared_ts_left_;

      int64_t t_left = t_pw > t_cw ? t_pw : t_cw;
      t_left = t_left > t_low ? t_left : t_low;

      Log_debug(
          "[txn %d] ReadColumn, pre-accept phase2: table = %s, col_id = %d,  hint_flag = %s, tyep = %d, t_pw = %d, t_cw = %d, t_low = %d, t_left = %d, t_right = %d",
          this->id(),
          row->get_table()->Name().c_str(),
          col_id,
          hint_str[hint_flag],
          this->root_type,
          t_pw,
          t_cw,
          t_low,
          t_left,
          received_prepared_ts_right_);
      prepared_read_ranges_[r][col_id] = pair<int64_t, int64_t>(t_left, received_prepared_ts_right_);
      return true;
    }
  }
  else if (false) {
    if(r->rver_[col_id] < commit_ts_ ){
      r->rver_[col_id] = commit_ts_;
    }
    if (hint_flag == TXN_BYPASS || hint_flag == TXN_DEFERRED) {
      //For fast path_, seems no need to read.
      Log_debug("[txn %d] ReadColumn, commit phase: table = %s, col_id = %d,  hint_flag = %s, commit_ts = %d,",
                id(),
          row->get_table()->Name().c_str(),
               col_id,
               hint_str[hint_flag],
               commit_ts_);

      auto c = r->get_column(col_id);
      *value = c;
    } else {
      verify(0);
    }
  } else {
    verify(0);
  }
  return true;
}


bool TxSlog::WriteColumn(Row *row,
                            column_id_t col_id,
                            const Value &value,
                            int hint_flag) {

  auto r = dynamic_cast<ChronosRow*>(row);
  verify(r->rtti() == symbol_t::ROW_CHRONOS);

  verify(!read_only_);
  if (true) {
    if (hint_flag == TXN_BYPASS || hint_flag == TXN_INSTANT) {
      r->wlock_row_by(this->tid_);
      locked_rows_.insert(r);
      int64_t t_pr = r->max_prepared_rver(col_id);
      int64_t t_cr = r->rver_[col_id];
      int64_t t_low = received_prepared_ts_left_;

      int64_t t_left = t_pr > t_cr ? t_pr : t_cr;
      t_left = t_left > t_low ? t_left : t_low;

      Log_debug("[txn %d] Write Column, Prepare phase1: table = %s, col_id = %d,  hint_flag = %s, t_pr = %d, t_cr = %d, t_low = %d, t_left = %d, t_right = %d",
                id(),
          row->get_table()->Name().c_str(),
               col_id,
               hint_str[hint_flag],
               t_pr,
               t_cr,
               t_low,
               t_left,
               received_prepared_ts_right_);
      prepared_read_ranges_[r][col_id] = pair<int64_t, int64_t>(t_left, received_prepared_ts_right_);
      //xs notes: If readlly there ``are'' instant writes, then we should apply it here.
      mdb_txn()->write_column(row, col_id, value);

    }
    if (hint_flag == TXN_INSTANT || hint_flag == TXN_DEFERRED) {
      r->wlock_row_by(this->tid_);
      locked_rows_.insert(r);
      int64_t t_pr = r->max_prepared_rver(col_id);
      int64_t t_cr = r->rver_[col_id];
      int64_t t_low = received_prepared_ts_left_;

      int64_t t_left = t_pr > t_cr ? t_pr : t_cr;
      t_left = t_left > t_low ? t_left : t_low;

      Log_debug("[txn %d] Write Column, Prepare phase2: table = %s, col_id = %d,  hint_flag = %s, t_pr = %d, t_cr = %d, t_low = %d, t_left = %d, t_right = %d",
              id(),
          row->get_table()->Name().c_str(),
               col_id,
               hint_str[hint_flag],
               t_pr,
               t_cr,
               t_low,
               t_left,
               received_prepared_ts_right_);
      //xs: add to prepared_write_ranges
      prepared_write_ranges_[r][col_id] = pair<int64_t, int64_t>(t_left, received_prepared_ts_right_);
    }
  } else if (false) {
    if (r->wver_[col_id] < commit_ts_){
      r->wver_[col_id] = commit_ts_;
    };
    if (hint_flag == TXN_BYPASS || hint_flag == TXN_DEFERRED) {
      Log_debug("[txn %d] Write Column, commit phase: table = %s, col_id = %d,  hint_flag = %s, commit_ts = %d,",
              id(),
          row->get_table()->Name().c_str(),
               col_id,
               hint_str[hint_flag],
               commit_ts_);
      //TODO: remove prepared ts for GC
      mdb_txn()->write_column(row, col_id, value);
    } else {
      verify(0);
    }
  } else {
    verify(0);
  }
  return true;
}



void TxSlog::CommitExecute() {
//  verify(phase_ == PHASE_RCC_START);
  Log_debug("%s called", __FUNCTION__);
  TxnWorkspace ws;
  for (auto &cmd: dreqs_) {
    auto pair = txn_reg_->get(cmd);
    int tmp;
    cmd.input.Aggregate(ws);
    auto& m = output_[cmd.inn_id_];
    pair.txn_handler(nullptr, this, cmd, &tmp, m);
    ws.insert(m);
  }
  Log_debug("%s returned", __FUNCTION__);
  committed_ = true;
}

void TxSlog::CheckSendOutputToClient() {


  Log_debug("%s called for txn id = %lu", __FUNCTION__, id() );
  if (this->slog_phase_ < SLOG_DIST_EXECUTED){
    Log_debug("Execution not finished");
    return;
  }
  if (this->slog_phase_ == SLOG_OUTPUT_SENT){
    Log_debug("already sent");
    return;
  }
  bool all_output_received = true;
  for (auto& pair : par_output_received_){
    if (pair.second == false){
      Log_debug("output for partition %hu not received", pair.first);
      all_output_received = false;
      break;
    }else{
      Log_debug("output for partition %hu received", pair.first);
    }
  }
  if (all_output_received){
    Log_debug("going to send outputs to client txn id = %lu", this->id());
#ifdef LAT_BREAKDOWN
    this->handler_output_ready_ts = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    Log_debug("txn %lu, lat breakdown in ms, remote prepare, local prepare, wait for exe local, exe (incl. wait for input),  wait for output,%ld,%ld,%ld,%ld,%ld",
        id(),
        handler_remote_prepared_ts-handler_submit_ts_,
        handler_local_prepared_ts-handler_remote_prepared_ts,
        handler_local_start_exe_ts-handler_local_prepared_ts,
        handler_local_exe_finish_ts - handler_local_start_exe_ts,
        handler_output_ready_ts-handler_local_exe_finish_ts);
#endif //LAT_BREAKDOWN
    Log_debug("here");
    send_output_client();
    this->slog_phase_ = SLOG_OUTPUT_SENT;
  }
}

bool TxSlog::TryExecute(std::set<innid_t> & pending_pieces, parid_t my_par_id,
                        std::map<parid_t, std::map<varid_t, Value>> &dependent_vars){

    dependent_vars = RecursiveExecuteReadyPieces(my_par_id, pending_pieces);

    bool finished = (n_local_pieces_ == n_executed_pieces);

    if (finished){
        executed_ = true;
        Log_debug("finished executing transaction for id = %lu, output size = %d",
                 id(),
                 dist_output_->size());
        if (dist_output_->size() != 0){
            par_output_received_[my_par_id] = true;
        }
        reply_callback();
    }else{
        Log_debug("execution not finished %d vs %d", n_local_pieces_, n_executed_pieces);
    }

    return finished;
}


} // namespace janus

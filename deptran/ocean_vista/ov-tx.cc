//
// Created by micha on 2020/4/7.
//


#include "ov-tx.h"
#include "memdb/row_mv.h"
namespace rococo {
//
//
//Already defined
static char * defer_str[] = {"defer_real", "defer_no", "defer_fake"};
static char * hint_str[] = {"n/a", "bypass", "instant", "n/a",  "deferred"};
//SimpleCommand is a typedef of TxnPieceData
//add a simpleCommand to the local Tx's dreq
void TxOV::DispatchExecute(const SimpleCommand &cmd,
                                int32_t *res,
                                map<int32_t, Value> *output) {

  phase_ = PHASE_CHRONOS_DISPATCH;

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

//  Log_info("%s called, defer= %s, txn_id = %d, root_type = %d, type = %d" , __FUNCTION__, defer_str[pair.defer], cmd.root_id_, cmd.root_type_, cmd.type_);
  int xxx, *yyy;
  if (pair.defer == DF_REAL) {
    yyy = &xxx;
    dreqs_.push_back(cmd);
  } else if (pair.defer == DF_NO) {
    yyy = &xxx;
  } else if (pair.defer == DF_FAKE) {
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
}


void TxOV::PreAcceptExecute(const SimpleCommand &cmd, int *res, map<int32_t, Value> *output) {

  phase_ = PHASE_CHRONOS_PRE_ACCEPT;

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

//  Log_info("%s called, defer= %s, txn_id = %d, root_type = %d, type = %d" , __FUNCTION__, defer_str[pair.defer], cmd.root_id_, cmd.root_type_, cmd.type_);
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
bool TxOV::ReadColumn(mdb::Row *row,
                           mdb::column_id_t col_id,
                           Value *value,
                           int hint_flag) {
  //Seems no instant through out the process
  //Log_info("Rtti = %d", row->rtti());
  auto r = dynamic_cast<ChronosRow*>(row);
  verify(r->rtti() == symbol_t::ROW_CHRONOS);

  verify(!read_only_);
  if (phase_ == PHASE_CHRONOS_DISPATCH) {
    if (hint_flag == TXN_BYPASS || hint_flag == TXN_INSTANT) {
      auto c = r->get_column(col_id);
      row->ref_copy();
      *value = c;
      return true;
    }
    if (hint_flag == TXN_DEFERRED) {
      //xs: seems no need to read for a deferred
      return true;
    }
  }else if (phase_ == PHASE_CHRONOS_PRE_ACCEPT) {
    //In the pre-accept phase
    if (hint_flag == TXN_BYPASS || hint_flag == TXN_INSTANT) {
      /*
       * xs notes
       *
       *
       */
      auto c = r->get_column(col_id);
      row->ref_copy();
      *value = c;
      return true;
    }
    if (hint_flag == TXN_DEFERRED) {
      auto c = r->get_column(col_id);
      row->ref_copy();
      *value = c;
      return true;
    }
  }
  else if (phase_ == PHASE_CHRONOS_COMMIT) {
    if (hint_flag == TXN_BYPASS || hint_flag == TXN_DEFERRED) {
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


bool TxOV::WriteColumn(Row *row,
                            column_id_t col_id,
                            const Value &value,
                            int hint_flag) {

  auto r = dynamic_cast<ChronosRow*>(row);
  verify(r->rtti() == symbol_t::ROW_CHRONOS);

  verify(!read_only_);
  if (phase_ == PHASE_CHRONOS_DISPATCH || phase_ == PHASE_CHRONOS_PRE_ACCEPT) {
    if (hint_flag == TXN_BYPASS || hint_flag == TXN_INSTANT) {

      mdb_txn()->write_column(row, col_id, value);

    }
    if (hint_flag == TXN_INSTANT || hint_flag == TXN_DEFERRED) {
    }
  } else if (phase_ == PHASE_CHRONOS_COMMIT) {
    if (hint_flag == TXN_BYPASS || hint_flag == TXN_DEFERRED) {
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



void TxOV::CommitExecute() {
//  verify(phase_ == PHASE_RCC_START);
  phase_ = PHASE_CHRONOS_COMMIT;
//  Log_info("%s called", __FUNCTION__);
  TxnWorkspace ws;
  for (auto &cmd: dreqs_) {
    auto pair = txn_reg_->get(cmd);
    int tmp;
    cmd.input.Aggregate(ws);
    auto& m = output_[cmd.inn_id_];
    pair.txn_handler(nullptr, this, cmd, &tmp, m);
    ws.insert(m);
  }
//  Log_info("%s returned", __FUNCTION__);
  committed_ = true;
}



} // namespace janus

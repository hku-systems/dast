#include "../__dep__.h"
#include "../txn_chopper.h"
#include "exec.h"
#include "dtxn.h"

namespace rococo {

set<Row*> TapirExecutor::locked_rows_s = {};

void TapirExecutor::FastAccept(const vector<SimpleCommand>& txn_cmds,
                               int32_t* res) {
  // validate read versions
  if (txn_cmds.size() > 0){
    Log_debug("[%s]: id = %d, type = %d, size of txn_cmds = %d", __FUNCTION__, txn_cmds[0].root_id_, txn_cmds[0].root_type_, txn_cmds.size());
  }else{
    Log_debug("[%s]: size = 0", __FUNCTION__);
  }
  *res = SUCCESS;

  map<varid_t, Value> output_m;
  for (auto& c: txn_cmds) {
    output_m.insert(c.output.begin(), c.output.end()); //xs: add all cmds' output during the dispatch phase
  }

  phase_ = tapir_fast; // added by xs for debug

  //map<innid_t, map<int32_t, Value>> TxnOutput;
  TxnOutput output;
  Execute(txn_cmds, &output);
  for (auto& pair : output) {
    for (auto &ppair: pair.second) {
      auto &idx = ppair.first; //var_id
      Value &value = ppair.second;
      if (output_m.count(idx) == 0 ||
          value.ver_ != output_m.at(idx).ver_) {
        *res = REJECT;
        return;
      }
    }
  }//the output value is different from the dispatch phase

  // TODO lock read items.
  auto& read_vers = dtxn()->read_vers_;
  for (auto& pair1 : read_vers) {
    Row* r = pair1.first;
    if (r->rtti() != symbol_t::ROW_VERSIONED) {
      verify(0);
    };
    auto row = dynamic_cast<mdb::VersionedRow*>(r);
    verify(row != nullptr);
    for (auto& pair2: pair1.second) {
      auto col_id = pair2.first;
      auto ver_read = pair2.second;
      auto ver_now = row->get_column_ver(col_id);
      if (ver_now > ver_read) {
        // value has been updated. abort transaction
        verify(0);
//        *res = REJECT;
        return;
      } else {
        // grab read lock.
        if (!row->rlock_row_by(this->cmd_id_)) {
          *res = REJECT;
        } else {
          // remember locks.
          locked_rows_.insert(row);
        }
      };
    }
  }
  // in debug
//  return;
  // grab write locks.
  for (auto& pair1 : dtxn()->write_bufs_) {
    auto row = (mdb::VersionedRow*)pair1.first;
    if (!row->wlock_row_by(cmd_id_)) {
//        verify(0);
      *res = REJECT;
//      Log_debug("lock row %llx failed", row);
    } else {
      // remember locks.
//     Log_debug("lock row %llx succeed", row);
      locked_rows_.insert(row);
    }
  }
  verify(*res == SUCCESS || *res == REJECT);
}

void TapirExecutor::Commit() {
  // merge write buffers into database.
  for (auto& pair1 : dtxn()->write_bufs_) {
    auto row = (mdb::VersionedRow*)pair1.first;
    for (auto& pair2: pair1.second) {
      auto& col_id = pair2.first;
      auto& value = pair2.second;
      row->incr_column_ver(col_id);
      value.ver_ = row->get_column_ver(col_id);
      row->update(col_id, value);
    }
  }
  // release all the locks.
  for (auto row : locked_rows_) {
    // Log_debug("unlock row %llx succeed", row);
    auto r = row->unlock_row_by(cmd_id_);
    verify(r);
  }
  dtxn()->read_vers_.clear();
  dtxn()->write_bufs_.clear();
}

void TapirExecutor::Abort() {
  for (auto row : locked_rows_) {
    // Log_debug("unlock row %llx succeed", row);
    auto r = row->unlock_row_by(cmd_id_);
    verify(r);
  }
  dtxn()->read_vers_.clear();
  dtxn()->write_bufs_.clear();
}



TapirDTxn* TapirExecutor::dtxn() {
  verify(dtxn_ != nullptr);
  auto d = dynamic_cast<TapirDTxn*>(dtxn_);
  verify(d);
  return d;
}


void TapirExecutor::Execute(const vector<SimpleCommand>& cmds,
                       TxnOutput* output) {
  Log_debug("[%s]: id = %d, type = %d, phase = %d, size of txn_cmds = %d", __FUNCTION__, cmds[0].root_id_, cmds[0].type_, phase_ , cmds.size());
  TxnWorkspace ws;
  for (const SimpleCommand& c: cmds) {
    auto& cmd = const_cast<SimpleCommand&>(c);
    const auto &handler = txn_reg_->get(c).txn_handler;
    auto& m = (*output)[c.inn_id_];
    int res;
    cmd.input.Aggregate(ws);
    handler(this,
            dtxn_,
            cmd,
            &res,
            m);
    ws.insert(m);
#ifdef DEBUG_CODE
    for (auto &pair: output) {
      verify(pair.second.get_kind() != Value::UNKNOWN);
    }
#endif
    for (auto &pair: m){
      Log_debug("[id = %d, type = %d, inn_id = %d] output key = %d", c.root_id_, c.root_type_, c.inn_id_, pair.first);
    }
  }
}


} // namespace rococo

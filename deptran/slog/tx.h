//
// Created by micha on 2020/3/23.
//
#pragma once

#include "deptran/rcc/dtxn.h"
#include "../command.h"
#include "memdb/row_mv.h"
#include "deptran/chronos/scheduler.h"

#define LAT_BREAKDOWN

namespace rococo {





//const int PHASE_CAN_EXE = 4;  // >= PHASE_CAN_EXE can be executed

class TxSlog : public RccDTxn {
 public:
  using RccDTxn::RccDTxn;

  //xs TODO: This can be simplified
  enum SlogPhase {
    SLOG_EMPTY = 0,
    SLOG_LOCAL_REPLICATED = 1,
    SLOG_LOCAL_OUTPUT_READY = 2,
    SLOG_DIST_REPLICATED = 3,
    SLOG_DIST_CAN_EXE = 4,
    SLOG_DIST_EXECUTED = 5, //exe finishes
    SLOG_DIST_OUTPUT_READY = 6,
    SLOG_OUTPUT_SENT = 7
  };

  bool is_local_ = true;

  virtual mdb::Row *CreateRow(
      const mdb::Schema *schema,
      const std::vector<mdb::Value> &values) override {
    Log_debug("[[%s]] called", __PRETTY_FUNCTION__);
    return ChronosRow::create(schema, values);
  }

  ~TxSlog(){
    if (own_dist_output_){
      delete dist_output_;
    }

  }

  void DispatchExecute(const SimpleCommand &cmd,
                       int *res,
                       map<int32_t, Value> *output) override;

  void PreAcceptExecute(const SimpleCommand &cmd,
                        int *res,
                        map<int32_t, Value> *output);

  void CommitExecute() override;

  bool ReadColumn(mdb::Row *row,
                  mdb::column_id_t col_id,
                  Value *value,
                  int hint_flag = TXN_INSTANT) override;

  bool WriteColumn(Row *row,
                   column_id_t col_id,
                   const Value &value,
                   int hint_flag = TXN_INSTANT) override;




  //return whether the txn has finished
  std::function<bool(std::set<innid_t>&)> execute_callback_ = [this](std::set<innid_t>& pending_pieces){
    Log_fatal("execution call back not assigned, id = %lu", this->tid_);
    verify(0);
    return false;
  };


  int n_local_store_acks = 0;

  bool relevant_ = false;
//  bool local_stored_ = false;


  std::set<ChronosRow *> locked_rows_ = {};


  int64_t received_prepared_ts_left_ = 0;
  int64_t received_prepared_ts_right_ = 0;

  map<ChronosRow*, map<column_id_t, pair<mdb::version_t, mdb::version_t>>> prepared_read_ranges_ = {};
  map<ChronosRow*, map<column_id_t, pair<mdb::version_t, mdb::version_t>>> prepared_write_ranges_ = {};




  int64_t commit_ts_;

  cmdtype_t root_type = 0;



  std::map<parid_t, bool> par_output_received_ = {};

  std::function<void()> send_output_client = [this](){
    Log_fatal("send to client callback not assigned, id = %lu", this->tid_);
    verify(0);
  };

  std::map<parid_t, std::map<varid_t, Value>> RecursiveExecuteReadyPieces(parid_t my_par_id, std::set<innid_t>& pending_pieces);
  void CheckSendOutputToClient();
  std::set<parid_t> GetOutputsTargets(std::map<int32_t, Value> *output);
  bool MergeCheckReadyPieces(const std::map<int32_t, Value>& output);
  bool MergeCheckReadyPieces(const TxnOutput& output);

  std::vector<SimpleCommand> pieces_;
  int n_local_pieces_;
  int n_executed_pieces = 0;

  SlogPhase slog_phase_ = SLOG_EMPTY;
  int n_touched_partitions = -1;
  siteid_t handler_site_ = -1;

  TxnOutput* dist_output_;
  bool own_dist_output_ = false;  //xsTodo: use smart pointer

  bool exe_tried_ = false;

#ifdef LAT_BREAKDOWN
  int64_t handler_submit_ts_;
  int64_t handler_remote_prepared_ts;
  int64_t handler_local_prepared_ts;
  int64_t handler_local_start_exe_ts;
  int64_t handler_local_exe_finish_ts;
  int64_t handler_output_ready_ts;
#endif //LAT_BREAKDOWN

  bool check_piece_conflict (innid_t pending_pie_id, innid_t type_to_check);

  bool can_exe_wo_conflict (innid_t to_check, const std::set<innid_t>& pending_pieces);

  int recur_times_ = 0;

  bool TryExecute(std::set<innid_t> & pending_pieces, parid_t my_par_id,
                  std::map<parid_t, std::map<varid_t, Value>> &dependent_vars);


  std::function<void()> reply_callback = [this](){
      verify(0);
  };

  siteid_t coord_site_;

};

} // namespace janus



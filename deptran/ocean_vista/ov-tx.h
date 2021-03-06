//
// Created by micha on 2020/3/23.
//
#pragma once

#include "deptran/rcc/dtxn.h"
#include "../command.h"
#include "memdb/row_mv.h"
#include "ov-txn_mgr.h"
namespace rococo {

#define PHASE_CHRONOS_DISPATCH (1)
#define PHASE_CHRONOS_PRE_ACCEPT (2)
#define PHASE_CHRONOS_COMMIT (3)


class TxOV : public RccDTxn {
 public:
  using RccDTxn::RccDTxn;


  virtual mdb::Row *CreateRow(
      const mdb::Schema *schema,
      const std::vector<mdb::Value> &values) override {
    return ChronosRow::create(schema, values);
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

  ov_ts_t ovts_;
  std::function<void()> executed_callback = [this](){
    Log_fatal("call back not assigned, id = %lu", this->tid_);
    verify(0);
  };

  cmdtype_t root_type = 0;

  enum OV_txn_status {
    INIT = 0,
    STORED = 1,
    CAN_EXECUTE = 2,
    EXECUTED = 3
  };

  OV_txn_status ov_status_ = OV_txn_status::INIT;
};

} // namespace janus




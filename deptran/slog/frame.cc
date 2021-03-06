//
// Created by micha on 2020/3/23.
//

#include "frame.h"

#include "../__dep__.h"
#include "../command.h"
#include "../command_marshaler.h"
#include "../communicator.h"
#include "../rcc/rcc_row.h"
#include "commo.h"
#include "frame.h"
#include "coordinator.h"
#include "scheduler.h"
#include "tx.h"
#include "memdb/row_mv.h"

namespace rococo {

static Frame *slog_frame_s = Frame::RegFrame(MODE_SLOG,
                                                {"SLOG", "slog", "Slog"},
                                                []() -> Frame * {
                                                  return new SlogFrame();
                                                });

Coordinator *SlogFrame::CreateCoord(cooid_t coo_id,
                                             Config *config,
                                             int benchmark,
                                             ClientControlServiceImpl *ccsi,
                                             uint32_t id,
                                             TxnRegistry *txn_reg) {

  if (site_info_ != nullptr){
    Log_info("[site %d] created slog coordinator", site_info_->id);
  }else{
    Log_info("[site null] created slog coordinator");
  }


  verify(config != nullptr);
  CoordinatorSlog *coord = new CoordinatorSlog(coo_id,
                                                     benchmark,
                                                     ccsi,
                                                     id);
  coord->txn_reg_ = txn_reg;
  coord->frame_ = this;
  return coord;
}

Executor *SlogFrame::CreateExecutor(uint64_t, Scheduler *sched) {
  if (site_info_ != nullptr){
    Log_info("[site %d] created slog executor", site_info_->id);
  }else{
    Log_info("[site null] created slog executor");
  }
  verify(0);
  return nullptr;
}

Scheduler *SlogFrame::CreateScheduler() {
  if (site_info_ != nullptr){
    Log_info("[site %d] created slog scheduler", site_info_->id);
  }else{
    Log_info("[site null] created slog scheduler");
  }

  Scheduler *sched = new SchedulerSlog(this);
  return sched;
}

////XS: seems no need to override. Use the base funciton is ok.
////for now, only debug print is slightly different
//vector<rrr::Service *>
//ChronosFrame::CreateRpcServices(uint32_t site_id,
//                              Scheduler *sched,
//                              rrr::PollMgr *poll_mgr,
//                              ServerControlServiceImpl *scsi) {
//
//
//
//  if (site_info_ != nullptr){
//    Log_info("[site %d] created rpc services", site_info_->id);
//  }else{
//    Log_info("[site null] created rpc services");
//  }
//
//  return Frame::CreateRpcServices(site_id, sched, poll_mgr, scsi);
//}

//mdb::Row *ChronosFrame::CreateRow(const mdb::Schema *schema,
//                                vector<Value> &row_data) {
//  if (site_info_ != nullptr){
//    Log_info("[site %d] [Chronos] created row", site_info_->id);
//  }else{
//    Log_info("[site null] [Chrnonos] created row");
//  }
//
//
//
//  mdb::Row *r = RCCRow::create(schema, row_data);
//  return r;
//}
//
DTxn* SlogFrame::CreateDTxn(uint32_t epoch, uint64_t tid, bool ro, Scheduler *mgr) {

  auto dtxn = new TxSlog(epoch, tid, mgr, ro);
  return dtxn;
}


Communicator *SlogFrame::CreateCommo(PollMgr *poll) {
  Communicator* commo_ = new SlogCommo(poll);
  if (site_info_ != NULL){
    commo_ ->dcname_ = site_info_->dcname;
    commo_ ->site_info_ = site_info_;
    Log_info("[site %d] Creating chronos communicator, at dc [%s]", site_info_->id, commo_->dcname_.c_str());
  }
  else{
    Log_info("[site null] Creating chronos communicator, I think it should be the client");
  }
  return commo_;
}


mdb::Row *SlogFrame::CreateRow(const mdb::Schema *schema,
                                  vector<Value> &row_data) {

  mdb::Row *r = ChronosRow::create(schema, row_data);
  return r;
}

} // namespace janus

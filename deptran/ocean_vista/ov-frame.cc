//
// Created by micha on 2020/3/23.
//

#include "ov-frame.h"

#include "../__dep__.h"
#include "../command.h"
#include "../command_marshaler.h"
#include "../communicator.h"
#include "../rcc/rcc_row.h"
#include "ov-commo.h"
#include "ov-frame.h"
#include "ov-coordinator.h"
#include "ov-scheduler.h"
#include "ov-tx.h"
#include "memdb/row_mv.h"

namespace rococo {

static Frame *OV_frame_s = Frame::RegFrame(MODE_OV,
                                                {"ov", "ocean-vista", "ocean_vista"},
                                                []() -> Frame * {
                                                  return new OVFrame();
                                                });

Coordinator *OVFrame::CreateCoord(cooid_t coo_id,
                                             Config *config,
                                             int benchmark,
                                             ClientControlServiceImpl *ccsi,
                                             uint32_t id,
                                             TxnRegistry *txn_reg) {

  if (site_info_ != nullptr){
    Log_info("[site %d] creating ov coordinator", site_info_->id);
  }else{
    Log_debug("[site null] creating ov coordinator, coo_id = %u", coo_id);
  }


  verify(config != nullptr);
  CoordinatorOV *coord = new CoordinatorOV(coo_id,
                                                     benchmark,
                                                     ccsi,
                                                     id);

//  Log_info("Created coord, coo_id = %u", coord->coo_id_);
  coord->txn_reg_ = txn_reg;
  coord->frame_ = this;
  return coord;
}

Executor *OVFrame::CreateExecutor(uint64_t, Scheduler *sched) {
  if (site_info_ != nullptr){
    Log_info("[site %d] created ov executor", site_info_->id);
  }else{
    Log_info("[site null] created ov executor");
  }
  verify(0);
  return nullptr;
}

Scheduler *OVFrame::CreateScheduler() {
  if (site_info_ != nullptr){
    Log_info("[site %d] created ov scheduler, locale_id = %u, name = %s, proc_name = %s host = %s, port = %u, parititon id = %u",
        site_info_->id,
        site_info_->locale_id,
        site_info_->name.c_str(),
        site_info_->proc_name.c_str(),
        site_info_->host.c_str(),
        site_info_->port,
        site_info_->partition_id_);
  }else{
    Log_info("[site null] created ov scheduler");

  }


  SchedulerOV *sched = new SchedulerOV(this->site_info_);
  sched->SetFrame(this);
  return (Scheduler*) sched;
}

DTxn* OVFrame::CreateDTxn(uint32_t epoch, uint64_t tid, bool ro, Scheduler *mgr) {

  auto dtxn = new TxOV(epoch, tid, mgr, ro);
  return dtxn;
}


Communicator *OVFrame::CreateCommo(PollMgr *poll) {
  Communicator* commo_ = new OVCommo(poll);


  if (site_info_ != NULL){
    commo_->dcname_ = site_info_->dcname;
    Log_info("[site %d] Creating OV communicator, at dc [%s]", site_info_->id, commo_->dcname_.c_str());
  }
  else{
    Log_info("[site null] Creating OV communicator, I think it should be the client");
  }
  return commo_;
}


mdb::Row *OVFrame::CreateRow(const mdb::Schema *schema,
                                  vector<Value> &row_data) {

  mdb::Row *r = ChronosRow::create(schema, row_data);
  return r;
}

} // namespace janus

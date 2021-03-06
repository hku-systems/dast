//
// Created by micha on 2020/3/23.
//

#ifndef ROCOCO_DEPTRAN_CHRONOS_FRAME_H_
#define ROCOCO_DEPTRAN_CHRONOS_FRAME_H_

#include "../frame.h"
#include "deptran/brq/frame.h"
namespace rococo{

class ChronosFrame : public BrqFrame {
 public:
  ChronosFrame(int mode = MODE_CHRONOS) : BrqFrame(mode) {}

  //xs: always null ptr, whats that for?
  Executor *CreateExecutor(cmdid_t, Scheduler *sched) override;

  //xs: transaction coordinator,
  //broadcastAccept to communicators.
  //call acceptrACK to do after receive ACK.
  //created by the client process/thread.
  Coordinator *CreateCoord(cooid_t coo_id,
                                 Config *config,
                                 int benchmark,
                                 ClientControlServiceImpl *ccsi,
                                 uint32_t id,
                                 TxnRegistry *txn_reg) override;

  //xs: What a participant do on receiving various requests.
  //created by each
  Scheduler *CreateScheduler() override;

//  //xs: whats that for
//  //created by each server, but not client.
//  vector<rrr::Service *> CreateRpcServices(uint32_t site_id,
//                                           Scheduler *dtxn_sched,
//                                           rrr::PollMgr *poll_mgr,
//                                           ServerControlServiceImpl *scsi)
//  override;

//  //xs: called during initialization
  mdb::Row *CreateRow(const mdb::Schema *schema,
                      vector<Value> &row_data) override;
//
//
////  //xs: called in each site.
  DTxn* CreateDTxn(epoch_t epoch, txnid_t tid,
                          bool ro, Scheduler *mgr) override;

  //created by each client **and** each server
  Communicator *CreateCommo(PollMgr *poll = nullptr) override;
};


};

#endif //ROCOCO_DEPTRAN_CHRONOS_FRAME_H_

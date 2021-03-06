//
// Created by micha on 2020/3/23.
//
#pragma once

#include "../__dep__.h"
#include "../constants.h"
#include "../command.h"
#include "../brq/coord.h"




namespace rococo {
class ChronosCommo;
class CoordinatorChronos : public BrqCoord {
 public:
  enum Phase {CHR_INIT=0, CHR_COMMIT=1};
  enum Decision {CHR_UNK=0, CHR_COMMI=1, CHR_ABORT=2 };
  using BrqCoord::BrqCoord;


  int64_t chr_submit_ts;


  virtual ~CoordinatorChronos() {}

  ChronosCommo *commo();
  // Dispatch inherits from RccCoord;

  void launch_recovery(cmdid_t cmd_id);

  ballot_t magic_ballot() {
    ballot_t ret = 0;
    ret = (ret << 32) | coo_id_;
    return ret;
  }

  cmdid_t next_cmd_id() {
    cmdid_t ret = cmdid_prefix_c_++;
    ret = (ret << 32 | coo_id_);
    return ret;
  }
  void Reset() override;



  //xs's code start here
  std::atomic<uint64_t> logical_clock {0};
//  int32_t GetQuorumSize(parid_t par_id);


  void SubmitReq();
  void SubmitAck(phase_t phase,
                   TxnOutput& cmd,
                   ChronosSubmitRes &chr_res);


  void GotoNextPhase() override;
};
} // namespace janus


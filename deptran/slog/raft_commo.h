//
// Created by tyycxs on 2020/5/30.
//

#ifndef ROCOCO_DEPTRAN_SLOG_RAFT_COMMO_H
#define ROCOCO_DEPTRAN_SLOG_RAFT_COMMO_H


#include "__dep__.h"
#include "constants.h"
#include "rcc/graph.h"
#include "rcc/graph_marshaler.h"
#include "command.h"
#include "command_marshaler.h"
#include "txn_chopper.h"
#include "rcc_rpc.h"
#include "msg.h"
#include "txn_chopper.h"
#include "communicator.h"


namespace rococo {

class RaftCommo : public Communicator {
public:
  using Communicator::Communicator;
};
} // namespace rococo
#endif //ROCOCO_DEPTRAN_SLOG_RAFT_COMMO_H

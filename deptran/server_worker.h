#pragma once

#include "__dep__.h"
#include "command.h"
#include "command_marshaler.h"
#include "deptran/chronos/manager.h"
#include "deptran/slog/manager.h"
#include "deptran/slog/auxiliary_raft.h"
#include "dtxn.h"
#include "marshal-value.h"
#include "piece.h"
#include "rcc/graph.h"
#include "rcc/graph_marshaler.h"
#include "rcc_rpc.h"
#include "rcc_service.h"
#include "sharding.h"
#include "txn_chopper.h"

namespace rococo {

class Communicator;
class Frame;
class ServerWorker {
public:
    rrr::PollMgr *svr_poll_mgr_ = nullptr;
    vector<rrr::Service *> services_ = {};
    rrr::Server *rpc_server_ = nullptr;
    base::ThreadPool *thread_pool_g = nullptr;

    rrr::PollMgr *svr_hb_poll_mgr_g = nullptr;
    ServerControlServiceImpl *scsi_ = nullptr;
    rrr::Server *hb_rpc_server_ = nullptr;
    base::ThreadPool *hb_thread_pool_g = nullptr;

    Frame *dtxn_frame_ = nullptr;
    Frame *rep_frame_ = nullptr;
    Config::SiteInfo *site_info_ = nullptr;
    Sharding *sharding_ = nullptr;
    Scheduler *dtxn_sched_ = nullptr;
    Scheduler *rep_sched_ = nullptr;
    TxnRegistry *txn_reg_ = nullptr;

    Communicator *dtxn_commo_ = nullptr;
    Communicator *rep_commo_ = nullptr;


    void SetupGlobalRaftService();//the
    rrr::PollMgr *svr_raft_poll_mgr_ = nullptr;
    AuxiliaryRaftImpl *raft_service_ = nullptr;
    base::ThreadPool *raft_thread_pool_ = nullptr;
    rrr::Server *raft_rpc_server_ = nullptr;

    void SetupSlogRegionManagerService(Frame *f);
    rrr::PollMgr *slog_manager_poll_mgr_ = nullptr;
    SlogRegionManager *slog_manager_service_ = nullptr;
    base::ThreadPool *slog_manager_thread_pool_ = nullptr;
    rrr::Server *slog_manager_rpc_server_ = nullptr;

    void SetupDastManagerService(Frame *f);
    rrr::PollMgr *dast_manager_poll_mgr_ = nullptr;
    DastManager *dast_manager_service_ = nullptr;
    base::ThreadPool *dast_manager_thread_pool_ = nullptr;
    rrr::Server *dast_manager_rpc_server_ = nullptr;


    void SetupHeartbeat();
    void PopTable();
    void SetupBase();
    void SetupService();
    void SetupCommo();
    void RegPiece();
    void ShutDown();


    static const uint32_t CtrlPortDelta = 10000;
    void WaitForShutdown();
};


}// namespace rococo

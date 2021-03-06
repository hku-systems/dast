//
// Created by micha on 2021/1/28.
//
#ifndef ROCOCO_DAST_MANAGER_H
#define ROCOCO_DAST_MANAGER_H

#include "deptran/__dep__.h"
#include "deptran/chronos/scheduler.h"
#include "deptran/rcc_rpc.h"
#include "deptran/chronos/commo.h"


namespace rococo {
class DastManager : public DastManagerService, public SchedulerChronos {

public:
    DastManager(Frame *f);


    void PrepareCRT(const std::map<uint32_t, std::vector<SimpleCommand>> &cmds_by_par,
                    const ChronosProposeRemoteReq &req,
                    ChronosProposeRemoteRes *chr_res,
                    rrr::DeferredReply *defer) override;


    void NotiWait(const DastNotiManagerWaitReq& req, rrr::i32* res, rrr::DeferredReply* defer) override;

    ChronosCommo* commo();


    chr_ts_t CalculateAnticipatedTs(const chr_ts_t &src_ts, const chr_ts_t &recv_ts);


    regionid_t mgr_rid_;
    siteid_t mgr_siteid_;


};



}
#endif//ROCOCO_DAST_MANAGER_H

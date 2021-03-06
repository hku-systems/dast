//
// Created by micha on 2021/2/19.
//

#ifndef ROCOCO_SLOG_MANAGER_H
#define ROCOCO_SLOG_MANAGER_H


#include "deptran/__dep__.h"
#include "deptran/slog/scheduler.h"
#include "deptran/rcc_rpc.h"
#include "deptran/slog/commo.h"


namespace rococo{

class SlogRegionManager: public SlogRegionMangerService, public SchedulerSlog{
public:
    SlogRegionManager(Frame *f);

    void OnSubmitIRT(const std::map<parid_t, std::vector<SimpleCommand>> &);


    void SlogSendOrderedCRT(const map<parid_t, vector<SimpleCommand>>& cmd_by_par, const uint64_t& index, const siteid_t& handler_site, rrr::i32* res, rrr::DeferredReply* defer) override;

    void SendRmIRT(const std::map<uint32_t, std::vector<SimpleCommand>>& cmds_by_par, const siteid_t& handler_site, rrr::i32* res, rrr::DeferredReply* defer) override;

    void InsertLocalLog(const std::map<uint32_t, std::vector<SimpleCommand>>& cmds_by_par, const siteid_t& handler_site);

    void RmReplicate(const std::map<uint32_t, std::vector<SimpleCommand>>& cmds_by_par, const siteid_t& handler_site, rrr::i32* res, rrr::DeferredReply* defer) override;

    void ReplicateACK(size_t index);

    void CheckAndSendBacth();
    std::thread send_batch_thread_;

    struct logEntry{
        std::map<parid_t, std::vector<SimpleCommand>> cmds_;
        siteid_t coord_site_;
        bool committed_ = false;
    };

    std::vector<logEntry* > log_;
    size_t next_commit_index_ = 0;
    size_t next_send_index_ = 0;


    std::vector<logEntry*> global_log_;

    size_t global_next_insert_index_ = 0;

};


}


#endif//ROCOCO_SLOG_MANAGER_H

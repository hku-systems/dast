//
// Created by micha on 2021/1/28.
//

#include "deptran/chronos/manager.h"
#include "deptran/frame.h"


namespace rococo {


DastManager::DastManager(Frame *f) : DastManagerService(), SchedulerChronos(f) {
    /*
     * change my site id to be different from the process I am residing in
     * This is because we start the manager within the same process as an normal node's site
     */


    this->mgr_rid_ = f->site_info_->id / sites_per_region_;
    int total_num_sites = Config::GetConfig()->NumSites();
    this->mgr_siteid_ = total_num_sites + mgr_rid_;

}


void DastManager::NotiWait(const DastNotiManagerWaitReq &req, rrr::i32 *res, rrr::DeferredReply *defer) {


    parid_t region_start_par = mgr_rid_ * Config::GetConfig()->n_shard_per_region_;
    parid_t end_par = (mgr_rid_ + 1) * Config::GetConfig()->n_shard_per_region_;

    for (parid_t target_par = region_start_par; target_par < end_par; target_par++) {
        DastNotiCommitReq noti;
        noti.original_ts = req.original_ts;
        noti.need_input_wait = req.need_input_wait;
        noti.txn_id = req.txn_id;
        noti.new_ts = req.new_ts;
        noti.touched_sites.insert(req.touched_sites.begin(), req.touched_sites.end());
//
        auto par_sites = Config::GetConfig()->SitesByPartitionId(target_par);

        for (auto &site : par_sites) {
            siteid_t target_site = site.id;
            if (req.touched_sites.count(target_site) == 0){
                commo()->SendNotiCommit(target_site, noti);
            }
        }
    }

    std::lock_guard<std::recursive_mutex> guard(mtx_);
    dist_txn_tss_.erase(req.original_ts);
    if (req.need_input_wait){
        dist_txn_tss_.insert(req.new_ts);
    }

    defer->reply();
}

void DastManager::PrepareCRT(const std::map<uint32_t, std::vector<SimpleCommand>> &cmds_by_par,
                             const ChronosProposeRemoteReq &req,
                             ChronosProposeRemoteRes *chr_res,
                             rrr::DeferredReply *defer) {


    chr_ts_t src_ts = req.src_ts;
    verify(!cmds_by_par.empty());
    verify(!cmds_by_par.begin()->second.empty());
    txnid_t txn_id = cmds_by_par.begin()->second.begin()->root_id_;


    chr_ts_t anticipated_ts;
    chr_ts_t my_ts = GenerateChrTs(true);
    anticipated_ts = CalculateAnticipatedTs(src_ts, my_ts);

//    for (auto &p : req.par_input_ready) {
//        Log_info("[Dast Manager], input for par %u is ready? %d", p.first, p.second);
//    }


    std::set<siteid_t> touched_sites;

    for (auto &itr : cmds_by_par) {
        parid_t target_par = itr.first;
        auto par_sites = Config::GetConfig()->SitesByPartitionId(target_par);
        verify(target_par / Config::GetConfig()->n_shard_per_region_ == mgr_rid_);


        //Randomly select one as the leader (who sends outputs).
        //
        //
        siteid_t coord_site = req.src_ts.site_id_;
        DastPrepareCRTReq prep_req;
        prep_req.anticipated_ts = anticipated_ts;
        prep_req.coord_site = coord_site;
        prep_req.input_ready = req.par_input_ready.at(target_par);

        if (coord_site / N_REP_PER_SHARD == target_par) {
            //Coordinator is this shard
            //No need to assign a shard leader
            prep_req.shard_leader = coord_site;
        } else {
            int r = RandomGenerator::rand(0, par_sites.size() - 1);
            prep_req.shard_leader = par_sites[r].id;
        }

        for (auto &site : par_sites) {
            siteid_t target_site = site.id;
            touched_sites.insert(target_site);
            //            if (target_site != prep_req.coord_site){
            commo()->SendPrepareCRT(itr.second, target_par, target_site, prep_req);
            //            }
        }
    }


    //Notify other sites
    parid_t region_start_par = mgr_rid_ * Config::GetConfig()->n_shard_per_region_;
    parid_t end_par = (mgr_rid_ + 1) * Config::GetConfig()->n_shard_per_region_;

    for (parid_t target_par = region_start_par; target_par < end_par; target_par++) {
        if (cmds_by_par.count(target_par) == 0) {
            DastNotiCRTReq req;
            req.anticipated_ts = anticipated_ts;
            req.txn_id = txn_id;
            req.touched_sites = touched_sites;


            auto par_sites = Config::GetConfig()->SitesByPartitionId(target_par);

            for (auto &site : par_sites) {
                siteid_t target_site = site.id;
                commo()->SendNotiCRT(target_site, req);
            }
        }
    }

    std::lock_guard<std::recursive_mutex> guard(mtx_);
    dist_txn_tss_.insert(anticipated_ts);

    defer->reply();
}

ChronosCommo *DastManager::commo() {
    auto commo = dynamic_cast<ChronosCommo *>(commo_);
    verify(commo != nullptr);
    return commo;
}


chr_ts_t DastManager::CalculateAnticipatedTs(const chr_ts_t &src_ts, const chr_ts_t &recv_ts) {
    std::lock_guard<std::recursive_mutex> guard(mtx_);
    chr_ts_t ret;

    int64_t estimated_rtt = 90 * 1000;

    ret.timestamp_ = std::max(src_ts.timestamp_, recv_ts.timestamp_) + estimated_rtt;

    ret.stretch_counter_ = 0;
    ret.site_id_ = this->mgr_siteid_;

    if (!(ret > max_future_ts_)) {
        ret = max_future_ts_;
        ret.timestamp_++;
    }
    max_future_ts_ = ret;

    return ret;
}

}// namespace rococo

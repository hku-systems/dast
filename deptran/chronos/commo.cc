//
// Created by micha on 2020/3/23.
//

#include "commo.h"
#include "../rcc/graph_marshaler.h"
#include "deptran/rcc/dtxn.h"
#include "marshallable.h"
#include "txn_chopper.h"

namespace rococo {

//void ChronosCommo::SubmitLocalReq(vector<TxPieceData> &cmd,
//                                  const ChronosSubmitReq &chr_req,
//                                  const function<void(TxnOutput &cmd,
//                                                      ChronosSubmitRes &chr_res)> &callback) {
//
//    rrr::FutureAttr fuattr;
//    auto tid = cmd[0].root_id_;
//    auto par_id = cmd[0].partition_id_;
//    std::function<void(Future *)> cb =
//            [callback, tid, par_id](Future *fu) {
//                TxnOutput output;
//                ChronosSubmitRes chr_res;
//                fu->get_reply() >> chr_res >> output;
//                callback(output, chr_res);
//            };
//    fuattr.callback = cb;
//    auto proxy_info = EdgeServerForPartition(cmd[0].PartitionId());
//    //xs: seems to dispatch only the nearst replica fo the shard
//
//
//    auto proxy = proxy_info.second;
//    //XS: proxy is the rpc client side handler.
//    Log_debug("dispatch local transaction %lu to partition %u, proxy (site) = %hu",
//              cmd[0].root_id_,
//              cmd[0].PartitionId(),
//              proxy_info.first);
//
//    Future::safe_release(proxy->async_ChronosSubmitLocal(cmd, chr_req, fuattr));
//
//    Log_debug("-- SubmitReq returned");
//}

void ChronosCommo::SubmitTxn(map<parid_t, vector<SimpleCommand>> &cmds_by_par,
                             parid_t home_partition,
                             bool is_local,
                             const ChronosSubmitReq &chr_req,
                             const function<void(TxnOutput &output, ChronosSubmitRes &chr_res)> &callback) {
    rrr::FutureAttr fuattr;
    verify(cmds_by_par.size() > 0);
    verify(cmds_by_par.begin()->second.size() > 0);
    txnid_t txn_id = cmds_by_par.begin()->second.begin()->root_id_;
    std::function<void(Future *)> cb =
            [callback](Future *fu) {
                TxnOutput output;
                ChronosSubmitRes chr_res;
                fu->get_reply() >> chr_res >> output;
                callback(output, chr_res);
            };
    fuattr.callback = cb;
    auto proxy_info = EdgeServerForPartition(home_partition);
    //xs: seems to dispatch only the nearst replica fo the shard

    auto proxy = proxy_info.second;
    //XS: proxy is the rpc client side handler.
    Log_debug("Submit transaction id= %lu to home partition %u, proxy (site) = %hu, is_local =%d",
             txn_id,
             home_partition,
             proxy_info.first,
             is_local);

    Future::safe_release(proxy->async_ChronosSubmitTxn(cmds_by_par, chr_req, (int32_t) is_local, fuattr));

    //  Log_info("-- SubmitReq returned");
}

void ChronosCommo::SubmitCRT(map<regionid_t, map<parid_t, vector<SimpleCommand>>> &cmds_by_region,
                             parid_t home_partition,
                             const ChronosSubmitReq &chr_req,
                             const function<void(TxnOutput &output, ChronosSubmitRes &chr_res)> &callback) {
    rrr::FutureAttr fuattr;
    verify(!cmds_by_region.empty());
    verify(!cmds_by_region.begin()->second.empty());
    verify(!cmds_by_region.begin()->second.begin()->second.empty());
    txnid_t txn_id = cmds_by_region.begin()->second.begin()->second.begin()->root_id_;//should have the same root id

    std::function<void(Future *)> cb =
            [callback](Future *fu) {
                TxnOutput output;
                ChronosSubmitRes chr_res;
                fu->get_reply() >> chr_res >> output;
                callback(output, chr_res);
            };
    fuattr.callback = cb;
    auto proxy_info = EdgeServerForPartition(home_partition);
    //xs: seems to dispatch only the nearst replica fo the shard

    auto proxy = proxy_info.second;
    //XS: proxy is the rpc client side handler.
    Log_debug("Submit CRT id= %lu to home partition %u, proxy (site) = %hu",
             txn_id,
             home_partition,
             proxy_info.first);

    Future::safe_release(proxy->async_ChronosSubmitCRT(cmds_by_region, chr_req, fuattr));

    //  Log_info("-- SubmitReq returned");
}

void ChronosCommo::SendPrepareIRT(siteid_t target_site,
                                  const vector<SimpleCommand> &cmd,
                                  const ChronosStoreLocalReq &req,
                                  const function<void(ChronosStoreLocalRes &)> &callback) {


    auto proxy = rpc_proxies_[target_site];
    rrr::FutureAttr fuattr;
    std::function<void(Future *)> cb =
            [callback](Future *fu) {
                ChronosStoreLocalRes chr_res;
                fu->get_reply() >> chr_res;
                callback(chr_res);
            };
    fuattr.callback = cb;

    Log_debug("sending PrepareIRT to site %hu, my site id = %hu, my partition id = %u, ts = %lu:%lu:%hu",
              target_site,
              site_info_->id,
              site_info_->partition_id_,
              req.txn_ts.timestamp_,
              req.txn_ts.stretch_counter_,
              req.txn_ts.site_id_);

    Future::safe_release(proxy->async_ChronosStoreLocal(cmd, req, fuattr));
}

void ChronosCommo::SendNotiWait(uint32_t target_region, const DastNotiManagerWaitReq &req) {
    Log_debug("%s called, sending notify manager wait req for txn %lu changed from on %s to on %s to region %u, touching %d sites from %u",
             __FUNCTION__,
             req.txn_id,
             req.original_ts.to_string().c_str(),
             req.new_ts.to_string().c_str(),
             target_region,
             req.touched_sites.size(),
             *req.touched_sites.begin());

    auto proxy = dast_rpc_region_managers_[target_region];

    //no need for callback
    Future::safe_release(proxy->async_NotiWait(req));
}

void ChronosCommo::SendPrepareRemote(const map<parid_t, vector<SimpleCommand>> &cmd,
                                     regionid_t target_region,
                                     const ChronosProposeRemoteReq &req) {
    //                    const function<void(uint16_t target_site, ChronosProposeRemoteRes &chr_res)> &){
    txnid_t txn_id = cmd.begin()->second.begin()->root_id_;
    Log_debug("%s called, sending txn %lu to the manager of region %u", __FUNCTION__, txn_id, target_region);

    auto proxy = dast_rpc_region_managers_[target_region];

    //no need for callback
    Future::safe_release(proxy->async_PrepareCRT(cmd, req));
}


void ChronosCommo::SendIRSync(siteid_t target_site, const DastIRSyncReq &req) {
    auto proxy = rpc_proxies_[target_site];

    Future::safe_release(proxy->async_DastIRSync(req));
}

void ChronosCommo::SendNotiCRT(siteid_t target_site, const DastNotiCRTReq &req) {
    Log_debug("%s called, sending notiCRT to site %hu for txn %lu", __FUNCTION__, target_site, req.txn_id);
    auto proxy = rpc_proxies_[target_site];

    Future::safe_release(proxy->async_DastNotiCRT(req));
}

void ChronosCommo::SendNotiCommit(siteid_t target_site, const DastNotiCommitReq &req) {
    Log_debug("%s called, sending NotiCommit to site %hu for txn %lu", __FUNCTION__, target_site, req.txn_id);
    auto proxy = rpc_proxies_[target_site];

    Future::safe_release(proxy->async_DastNotiCommit(req));
}
void ChronosCommo::SendPrepareCRT(const vector<SimpleCommand> &cmds,
                                  parid_t target_par,
                                  siteid_t target_site,
                                  const DastPrepareCRTReq &req) {

    verify(!cmds.empty());
    txnid_t txn_id = cmds.begin()->root_id_;
    Log_debug("%s called, sending txn %lu to site %hu of par %u, par_leader = %hu",
             __FUNCTION__,
             txn_id,
             target_site,
             target_par,
             req.shard_leader);

    auto proxy = rpc_proxies_[target_site];

    Future::safe_release(proxy->async_DastPrepareCRT(cmds, req));
}
void ChronosCommo::SendProposeRemote(const vector<SimpleCommand> &cmd,
                                     const ChronosProposeRemoteReq &req,
                                     const function<void(uint16_t, ChronosProposeRemoteRes &)> &callback) {
    auto par_id = cmd[0].partition_id_;
    auto remote_region_proxies = ProxiesInPartition(par_id);
    auto leader = *(remote_region_proxies.begin());
    auto target_site = leader.first;
    rrr::FutureAttr fuattr;
    std::function<void(Future *)> cb =
            [callback, target_site](Future *fu) {
                //        Log_debug("callback for send propose remote called");
                ChronosProposeRemoteRes chr_res;
                fu->get_reply() >> chr_res;
                callback(target_site, chr_res);
            };
    fuattr.callback = cb;

    Log_debug("%s called, sending txn = %lu, to partition id %lu, site_id = %u ",
              __FUNCTION__,
              cmd[0].root_id_,
              par_id,
              leader.first);

    auto proxy = leader.second;
    //XS: proxy is the rpc client side handler.

    Future::safe_release(proxy->async_ChronosProposeRemote(cmd, req, fuattr));
}

void ChronosCommo::SendStoreRemote(siteid_t target_site,
                                   const vector<SimpleCommand> &cmd,
                                   const ChronosStoreRemoteReq &req) {

    Log_debug("%s called", __FUNCTION__);

    verify(this->site_info_ != nullptr);
    verify(cmd[0].PartitionId() == this->site_info_->partition_id_);

    auto proxy = rpc_proxies_[target_site];


    Log_debug("sending Store Remote to site %hu, my site id = %hu, my partition id = %u, ts = %lu:%lu:%hu",
              target_site,
              site_info_->id,
              site_info_->partition_id_,
              req.anticipated_ts.timestamp_,
              req.anticipated_ts.stretch_counter_,
              req.anticipated_ts.site_id_);
    Future::safe_release(proxy->async_ChronosStoreRemote(cmd, req));
}

void ChronosCommo::SendRemotePrepared(siteid_t target_site,
                                      const DastRemotePreparedReq &req) {

    Log_debug("%s called", __FUNCTION__);

    verify(this->site_info_ != nullptr);
    auto target_proxy = rpc_proxies_[target_site];

    verify(req.my_site_id == site_info_->id);

    Log_debug("sending Remote stored Ack to site %hu, my site id = %hu, my partition id = %u",
             target_site,
             site_info_->id,
             site_info_->partition_id_);
    Future::safe_release(target_proxy->async_DastRemotePrepared(req));
}

void ChronosCommo::SendOutput(parid_t target_partition,
                              const ChronosSendOutputReq &req) {
    Log_debug("%s called, sending to partition %u", __FUNCTION__, target_partition);

    auto proxies = rpc_par_proxies_[target_partition];
    for (auto &proxy : proxies) {
        Log_debug("sending output of txn %lu to site = %hu", req.txn_id, proxy.first);
        Future::safe_release(proxy.second->async_ChronosSendOutput(req));
    }
}


void ChronosCommo::SendDistExe(parid_t par_id, const ChronosDistExeReq &chr_req,
                               const function<void(TxnOutput &output, ChronosDistExeRes &chr_res)> &callback) {
    Log_debug("%s called", __FUNCTION__);
    rrr::FutureAttr fuattr;

    auto region_proxies = ProxiesInPartition(par_id);
    std::function<void(Future *)> cb =
            [callback](Future *fu) {
                //        Log_debug("callback for send propose remote called");
                TxnOutput output;
                ChronosDistExeRes chr_res;
                fu->get_reply() >> chr_res >> output;
                callback(output, chr_res);
            };
    fuattr.callback = cb;

    for (auto &proxy : region_proxies) {
        siteid_t siteid = proxy.first;
        if (siteid != this->site_info_->id) {
            Future::safe_release(proxy.second->async_ChronosDistExe(chr_req, fuattr));
            Log_debug("sending DistExe to site %hu, for txn id = %lu",
                     siteid,
                     chr_req.txn_id);
        }
    }
}

void ChronosCommo::SendProposeLocal(siteid_t target_site,
                                    const vector<SimpleCommand> &cmd,
                                    const DastProposeLocalReq &req,
                                    const function<void(ChronosProposeLocalRes &)> &callback) {

    rrr::FutureAttr fuattr;

    verify(this->site_info_ != nullptr);

    //  verify(cmd[0].PartitionId() == this->site_info_->partition_id_);


    std::function<void(Future *)> cb =
            [callback](Future *fu) {
                ChronosProposeLocalRes chr_res;
                fu->get_reply() >> chr_res;
                callback(chr_res);
            };
    fuattr.callback = cb;

    auto proxy = rpc_proxies_[target_site];
    Log_debug("sending Propose Local to site %hu, coord_site = %hu, shard_leader = %hu, ts = %s",
             target_site,
             req.coord_site,
             req.shard_leader_site,
             req.txn_ts.to_string().c_str());
    Future::safe_release(proxy->async_ChronosProposeLocal(cmd, req, fuattr));
}

//void ChronosCommo::SendLocalSync(siteid_t target_site, const ChronosLocalSyncReq &req,
//                                    const function<void(ChronosLocalSyncRes &)> &callback) {
//  rrr::FutureAttr fuattr;
//  std::function<void(Future *)> cb =
//      [callback](Future *fu) {
//        ChronosLocalSyncRes res;
//        fu->get_reply() >> res;
//        callback(res);
//      };
//  fuattr.callback = cb;
//  auto proxy = rpc_proxies_[target_site];
//  Log_debug("sending sync to site %hu", target_site);
//  Future::safe_release(proxy->async_ChronosLocalSync(req, fuattr));
//}

void ChronosCommo::SendHandoutRo(SimpleCommand &cmd,
                                 const function<void(int res,
                                                     SimpleCommand &cmd,
                                                     map<int,
                                                         mdb::version_t> &vers)> &) {
    verify(0);
}

}// namespace rococo

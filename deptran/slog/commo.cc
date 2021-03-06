//
// Created by micha on 2020/3/23.
//

#include "commo.h"
#include "../rcc/graph_marshaler.h"
#include "deptran/rcc/dtxn.h"
#include "marshallable.h"
#include "txn_chopper.h"

namespace rococo {

void SlogCommo::SubmitTxn(map<parid_t, vector<TxPieceData>> &cmd_by_par,
                               bool is_irt,
                               parid_t home_par,
                               const function<void(TxnOutput &cmd)> &callback) {

    rrr::FutureAttr fuattr;
    verify(cmd_by_par.size() > 0);
    verify(cmd_by_par.begin()->second.size() > 0);
    auto tid = cmd_by_par.begin()->second.at(0).root_id_;

    std::function<void(Future *)> cb =
            [callback, tid](Future *fu) {
                TxnOutput output;
                fu->get_reply() >> output;
                callback(output);
            };
    fuattr.callback = cb;

    auto proxy_info = EdgeServerForPartition(home_par);
    auto proxy = proxy_info.second;

    Log_debug("submit txn %lu to site %hu", tid, proxy_info.first);

    Future::safe_release(proxy->async_SlogSubmitTxn(cmd_by_par, (int32_t)is_irt, fuattr));

}

void SlogCommo::SubmitDistributedReq(parid_t home_par,
                                     map<parid_t, vector<SimpleCommand>> &cmds_by_par,
                                     const function<void(TxnOutput &output)> &callback) {


    //Test code


    rrr::FutureAttr fuattr;
    verify(cmds_by_par.size() > 0);
    verify(cmds_by_par.begin()->second.size() > 0);
    txnid_t txn_id = cmds_by_par.begin()->second.begin()->root_id_;
    std::function<void(Future *)> cb =
            [callback](Future *fu) {
                TxnOutput output;
                fu->get_reply() >> output;
                callback(output);
            };
    fuattr.callback = cb;
    //XS: proxy is the rpc client side handler.

    auto leader = rpc_par_proxies_[home_par][0];

    Log_debug("Submit distributed transaction id= %lu to partition %u, proxy (site) = %hu",
              txn_id,
              home_par,
              leader.first);

    Future::safe_release(leader.second->async_SlogSubmitDistributed(cmds_by_par, fuattr));

    //  Log_debug("-- SubmitReq returned");
}


void SlogCommo::SendDependentValues(parid_t target_partition,
                                    const ChronosSendOutputReq &req) {
    Log_debug("%s called, sending to partition %u", __FUNCTION__, target_partition);

    auto proxies = rpc_par_proxies_[target_partition];
    for (auto &proxy : proxies) {
        Log_debug("sending dependent values of txn %lu to site = %hu", req.txn_id, proxy.first);
        Future::safe_release(proxy.second->async_SlogSendDepValues(req));
    }
}


void SlogCommo::SendHandoutRo(SimpleCommand &cmd,
                              const function<void(int res,
                                                  SimpleCommand &cmd,
                                                  map<int,
                                                      mdb::version_t> &vers)> &) {
    verify(0);
}
void SlogCommo::SendReplicateLocal(uint16_t target_site,
                                   const vector<SimpleCommand> &cmd,
                                   txnid_t txn_id,
                                   uint64_t index,
                                   uint64_t commit_index,
                                   const function<void()> &callback) {
    Log_debug("%s called, sending to partition %hu, index = %lu, commit_index = %lu", __FUNCTION__, target_site, index, commit_index);

    rrr::FutureAttr fuattr;
    std::function<void(Future *)> cb =
            [callback](Future *fu) {
                fu->get_reply();
                callback();
            };
    fuattr.callback = cb;
    //XS: proxy is the rpc client side handler.
    auto proxy = rpc_proxies_[target_site];

    Future::safe_release(proxy->async_SlogReplicateLogLocal(cmd, txn_id, index, commit_index, fuattr));
}
void SlogCommo::SendToRaft(const map<uint32_t, vector<SimpleCommand>> &cmds_by_par, uint16_t handler_site) {
    verify(raft_proxies_.size() > 0);
    auto leader = raft_proxies_[0];
    Future::safe_release(leader->async_SlogRaftSubmit(cmds_by_par, handler_site));
}

void SlogCommo::SendToRegionManager(const map<uint32_t, vector<SimpleCommand>> &cmd,
                                    siteid_t target_site,
                                    uint16_t handler_site) {
    verify(slog_rpc_region_managers_.size() > 0);
    auto proxy = slog_rpc_region_managers_[target_site];
    Log_debug("send to region manager on site %d of txn %lu", target_site, cmd.begin()->second.at(0).id_);
    Future::safe_release(proxy->async_SendRmIRT(cmd, handler_site));
}

void SlogCommo::SendBatchOutput(const std::vector<std::pair<uint64_t, TxnOutput>> &batch, uint32_t my_par_id) {
    Log_debug("%s called", __FUNCTION__);

    Log_debug("Sending batch consitsting of %d output, first id = %lu ",
              batch.size(),
              batch[0].first);

    for (auto &out : batch) {
        auto txnid = out.first;
        for (auto &pair : out.second) {
            auto vars = pair.second;
            for (auto &var : vars) {
                Log_debug("txn %lu, piece %u, var %d", out.first, pair.first, var.first);
            }
        }
    }


    for (auto &proxy : rpc_proxies_) {
        if (proxy.first != this->site_info_->id) {
            proxy.second->async_SlogSendBatchRemote(batch, my_par_id);
        }
    }
}

void SlogCommo::SendTxnOutput(siteid_t target_site, parid_t my_parid, txnid_t txnid, TxnOutput *output) {
    Log_debug("%s called, send output of txn %lu to site %hu, output size = %d", __FUNCTION__, txnid, target_site, output->size());
    rpc_proxies_[target_site]->async_SlogSendTxnOutput(txnid, my_parid, *output);
}

}// namespace rococo

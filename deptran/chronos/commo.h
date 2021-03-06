//
// Created by micha on 2020/3/23.
//

#pragma once
#include "brq/commo.h"

namespace rococo {

class ChronosCommo : public BrqCommo {
public:
    using BrqCommo::BrqCommo;

    void SendHandoutRo(SimpleCommand &cmd,
                       const function<void(int res,
                                           SimpleCommand &cmd,
                                           map<int, mdb::version_t> &vers)> &)
            override;


    //  //xs's code
    //  void SubmitLocalReq(vector<SimpleCommand>& cmd,
    //                    const ChronosSubmitReq& req,
    //                    const function<void(TxnOutput& output,
    //                                        ChronosSubmitRes &chr_res)>&)  ;

    void SubmitTxn(map<parid_t, vector<SimpleCommand>> &cmd,
                   parid_t home_partition,
                   bool is_local,
                   const ChronosSubmitReq &req,
                   const function<void(TxnOutput &output,
                                       ChronosSubmitRes &chr_res)> &);


    void SendPrepareRemote(const map<parid_t, vector<SimpleCommand>> &cmd,
                        regionid_t target_region,
                        const ChronosProposeRemoteReq &req);

    void SendNotiWait(regionid_t target_region, const DastNotiManagerWaitReq& req);
    //                        const function<void(uint16_t target_site, ChronosProposeRemoteRes &chr_res)> &);

    //  void BroadcastLocalSync(const ChronosLocalSyncReq& req,
    //                          const function<void(ChronosLocalSyncRes& res)>& callback);

    void SendIRSync(siteid_t target_site,
                              const DastIRSyncReq& req);

    void SendNotiCRT(siteid_t target_site,
                    const DastNotiCRTReq& req);

    void SendNotiCommit(siteid_t target_site,
                     const DastNotiCommitReq& req);

    void SendPrepareCRT(const vector<SimpleCommand>& cmds,
                        parid_t target_par,
                        siteid_t target_site,
                        const DastPrepareCRTReq& req);



    void SubmitCRT(map<regionid_t, map<parid_t, vector<SimpleCommand>>> &cmds_by_region,
                   parid_t home_partition,
                   const ChronosSubmitReq &chr_req,
                   const function<void(TxnOutput &output, ChronosSubmitRes &chr_res)> &callback);


    void SendPrepareIRT(siteid_t target_site,
                        const vector<SimpleCommand> &cmd,
                        const ChronosStoreLocalReq &req,
                        const function<void(ChronosStoreLocalRes &chr_res)> &callback);

    void SendProposeRemote(const vector<SimpleCommand> &cmd,
                           const ChronosProposeRemoteReq &req,
                           const function<void(uint16_t target_site, ChronosProposeRemoteRes &chr_res)> &);


    void SendProposeLocal(siteid_t target_site,
                          const vector<SimpleCommand> &cmd,
                          const DastProposeLocalReq &req,
                          const function<void(ChronosProposeLocalRes &chr_res)> &);

    void SendStoreRemote(siteid_t target_site,
                         const vector<SimpleCommand> &cmd,
                         const ChronosStoreRemoteReq &req);


    void SendRemotePrepared(siteid_t target_site,
                            const DastRemotePreparedReq &req);

    void SendOutput(parid_t target_partition,
                    const ChronosSendOutputReq &req);


    void SendDistExe(parid_t par_id, const ChronosDistExeReq &chr_req,
                     const function<void(TxnOutput &output, ChronosDistExeRes &chr_res)> &);
};

}// namespace rococo

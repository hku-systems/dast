#pragma once

#include "__dep__.h"
#include "rcc_rpc.h"

#define DepTranServiceImpl ClassicServiceImpl

namespace rococo {

class ServerControlServiceImpl;
class Scheduler;
class SimpleCommand;
class ClassicSched;

class ClassicServiceImpl : public ClassicService {

public:
    AvgStat stat_sz_gra_start_;
    AvgStat stat_sz_gra_commit_;
    AvgStat stat_sz_gra_ask_;
    AvgStat stat_sz_scc_;
    AvgStat stat_n_ask_;
    AvgStat stat_ro6_sz_vector_;
    uint64_t n_asking_ = 0;

    std::mutex mtx_;
    Recorder *recorder_ = NULL;
    ServerControlServiceImpl *scsi_;// for statistics;
    Scheduler *dtxn_sched_;

    ClassicSched *dtxn_sched() {
        return (ClassicSched *) dtxn_sched_;
    }

    void rpc_null(DeferredReply *defer);

    void Dispatch(const vector<SimpleCommand> &cmd,
                  int32_t *res,
                  TxnOutput *output,
                  DeferredReply *defer_reply) override;

    void Prepare(const i64 &tid,
                 const std::vector<i32> &sids,
                 i32 *res,
                 DeferredReply *defer) override;

    void Commit(const i64 &tid,
                i32 *res,
                DeferredReply *defer) override;

    void Abort(const i64 &tid,
               i32 *res,
               DeferredReply *defer) override;

    void UpgradeEpoch(const uint32_t &curr_epoch,
                      int32_t *res,
                      DeferredReply *defer) override;

    void TruncateEpoch(const uint32_t &old_epoch,
                       DeferredReply *defer) override;

    void TapirAccept(const cmdid_t &cmd_id,
                     const ballot_t &ballot,
                     const int32_t &decision,
                     rrr::DeferredReply *defer) override;
    void TapirFastAccept(const cmdid_t &cmd_id,
                         const vector<SimpleCommand> &txn_cmds,
                         rrr::i32 *res,
                         rrr::DeferredReply *defer) override;
    void TapirDecide(const cmdid_t &cmd_id,
                     const rrr::i32 &decision,
                     rrr::DeferredReply *defer) override;

#ifdef PIECE_COUNT
    typedef struct piece_count_key_t {
        i32 t_type;
        i32 p_type;
        bool operator<(const piece_count_key_t &rhs) const {
            if (t_type < rhs.t_type)
                return true;
            else if (t_type == rhs.t_type && p_type < rhs.p_type)
                return true;
            return false;
        }
    } piece_count_key_t;

    std::map<piece_count_key_t, uint64_t> piece_count_;

    std::unordered_set<i64> piece_count_tid_;

    uint64_t piece_count_prepare_fail_, piece_count_prepare_success_;

    base::Timer piece_count_timer_;
#endif

public:
    ClassicServiceImpl() = delete;

    ClassicServiceImpl(Scheduler *sched,
                       rrr::PollMgr *poll_mgr,
                       ServerControlServiceImpl *scsi = NULL);

    void RccDispatch(const vector<SimpleCommand> &cmd,
                     int32_t *res,
                     TxnOutput *output,
                     RccGraph *graph,
                     DeferredReply *defer) override;

    void RccFinish(const cmdid_t &cmd_id,
                   const RccGraph &graph,
                   TxnOutput *output,
                   DeferredReply *defer) override;

    void RccInquire(const epoch_t &epoch,
                    const cmdid_t &tid,
                    RccGraph *graph,
                    DeferredReply *) override;

    void RccDispatchRo(const SimpleCommand &cmd,
                       map<int32_t, Value> *output,
                       DeferredReply *reply);

    void BrqDispatch(const vector<SimpleCommand> &cmd,
                     int32_t *res,
                     TxnOutput *output,
                     Marshallable *res_graph,
                     DeferredReply *defer) override;

    void BrqCommit(const cmdid_t &cmd_id,
                   const Marshallable &graph,
                   int32_t *res,
                   TxnOutput *output,
                   DeferredReply *defer) override;

    void BrqCommitWoGraph(const cmdid_t &cmd_id,
                          int32_t *res,
                          TxnOutput *output,
                          DeferredReply *defer) override;

    void BrqInquire(const epoch_t &epoch,
                    const cmdid_t &tid,
                    Marshallable *graph,
                    DeferredReply *) override;

    void BrqPreAccept(const cmdid_t &txnid,
                      const vector<SimpleCommand> &cmd,
                      const Marshallable &graph,
                      int32_t *res,
                      Marshallable *res_graph,
                      DeferredReply *defer) override;

    void BrqPreAcceptWoGraph(const cmdid_t &txnid,
                             const vector<SimpleCommand> &cmd,
                             int32_t *res,
                             Marshallable *res_graph,
                             DeferredReply *defer) override;

    void BrqAccept(const cmdid_t &txnid,
                   const ballot_t &ballot,
                   const Marshallable &graph,
                   int32_t *res,
                   DeferredReply *defer) override;


    void BrqSendOutput(const ChronosSendOutputReq &chr_req, ChronosSendOutputRes *chr_res, rrr::DeferredReply *defer) override;

    void ChronosSubmitLocal(const vector<SimpleCommand> &cmd,
                            const ChronosSubmitReq &req,
                            ChronosSubmitRes *chr_res,
                            TxnOutput *p_output,
                            DeferredReply *p_defer) override;

    void ChronosSubmitTxn(const std::map<parid_t, std::vector<SimpleCommand>> &cmds_by_par,
                          const ChronosSubmitReq &req,
                          const int32_t &is_local,
                          ChronosSubmitRes *chr_res,
                          TxnOutput *output,
                          rrr::DeferredReply *defer) override;

    void DastPrepareCRT(const std::vector<SimpleCommand> &cmds,
                        const DastPrepareCRTReq &req,
                        rrr::i32 *res, rrr::DeferredReply *defer) override;


    void ChronosStoreLocal(const vector<SimpleCommand> &cmd,
                           const ChronosStoreLocalReq &req,
                           ChronosStoreLocalRes *chr_res,
                           DeferredReply *p_defer) override;

    //  void ChronosLocalSync(const ChronosLocalSyncReq &req, ChronosLocalSyncRes *res, rrr::DeferredReply *defer) override;

    void ChronosProposeRemote(const std::vector<SimpleCommand> &cmd,
                              const ChronosProposeRemoteReq &req,
                              ChronosProposeRemoteRes *chr_res,
                              rrr::DeferredReply *defer) override;

    void ChronosDistExe(const ChronosDistExeReq &chr_req,
                        ChronosDistExeRes *chr_res,
                        TxnOutput *output,
                        rrr::DeferredReply *defer) override;

    void ChronosProposeLocal(const std::vector<SimpleCommand> &cmd,
                             const DastProposeLocalReq &req,
                             ChronosProposeLocalRes *chr_res,
                             rrr::DeferredReply *defer) override;

    void ChronosStoreRemote(const std::vector<SimpleCommand> &cmd,
                            const ChronosStoreRemoteReq &req,
                            ChronosStoreRemoteRes *chr_res,
                            rrr::DeferredReply *defer) override;


    void DastNotiCommit(const DastNotiCommitReq &req, rrr::i32 *res, rrr::DeferredReply *defer) override;

    void DastNotiCRT(const DastNotiCRTReq &req, rrr::i32 *res, rrr::DeferredReply *defer) override;

    void DastRemotePrepared(const DastRemotePreparedReq &req,
                            DastRemotePreparedRes *chr_res,
                            rrr::DeferredReply *defer) override;

    void ChronosSendOutput(const ChronosSendOutputReq &chr_req,
                           ChronosSendOutputRes *chr_res,
                           rrr::DeferredReply *defer) override;


    void ChronosSubmitCRT(const std::map<uint32_t, std::map<uint32_t, std::vector<SimpleCommand>>> &cmds_by_region,
                          const ChronosSubmitReq &req,
                          ChronosSubmitRes *chr_res,
                          TxnOutput *output,
                          rrr::DeferredReply *defer) override;

    void DastIRSync(const DastIRSyncReq &req, rrr::i32 *res, rrr::DeferredReply *defer) override;


    void OVStore(const cmdid_t &txn_id,
                 const std::vector<SimpleCommand> &cmd,
                 const OVStoreReq &ov_req,
                 rrr::i32 *res,
                 OVStoreRes *ov_res,
                 rrr::DeferredReply *defer) override;

    void OVCreateTs(const cmdid_t &txn_id,
                    rrr::i64 *timestamp,
                    rrr::i16 *site_id,
                    rrr::DeferredReply *defer) override;

    void OVExecute(const cmdid_t &id,
                   const OVExecuteReq &req,
                   int32_t *res,
                   OVExecuteRes *chr_res,
                   TxnOutput *output,
                   rrr::DeferredReply *defer) override;

    void OVStoredRemoveTs(const cmdid_t &txn_id,
                          const rrr::i64 &timestamp,
                          const rrr::i16 &site_id,
                          int32_t *res,
                          rrr::DeferredReply *defer) override;

    void OVPublish(const rrr::i64 &dc_timestamp,
                   const rrr::i16 &dc_site_id,
                   rrr::i64 *ret_timestamp,
                   rrr::i16 *ret_site_id,
                   rrr::DeferredReply *defer) override;

    void OVExchange(const std::string &source_dcname,
                    const rrr::i64 &dvw_timestamp,
                    const rrr::i16 &dvw_site_id,
                    rrr::i64 *ret_timestamp,
                    rrr::i16 *ret_site_id,
                    rrr::DeferredReply *defer) override;

    void OVDispatch(const vector<SimpleCommand> &cmd,
                    int32_t *p_res,
                    TxnOutput *p_output,
                    DeferredReply *p_defer) override;

    void SlogInsertDistributed(const std::vector<SimpleCommand> &cmd,
                               const uint64_t &index,
                               const std::vector<parid_t> &touched_pars,
                               const siteid_t &handler_site,
                               const uint64_t &txn_id,
                               int32_t *res,
                               rrr::DeferredReply *defer) override;

    void SlogSubmitTxn(const std::map<parid_t, std::vector<SimpleCommand>> &cmd,
                       const int32_t &is_irt,
                         TxnOutput *output,
                         rrr::DeferredReply *defer) override;

    void SlogReplicateLogLocal(const std::vector<SimpleCommand> &cmd,
                               const uint64_t &txn_id,
                               const uint64_t &index,
                               const uint64_t &commit_index,
                               rrr::i32 *res,
                               rrr::DeferredReply *defer) override;

    void SlogSubmitDistributed(const std::map<uint32_t, std::vector<SimpleCommand>> &cmds_by_par,
                               TxnOutput *output,
                               rrr::DeferredReply *defer) override;

    void SlogSendBatch(const uint64_t& start_index, const std::vector<std::pair<txnid_t, std::vector<SimpleCommand>>>& cmds, const std::vector<std::pair<txnid_t, siteid_t>>& coord_sites, rrr::i32* res, rrr::DeferredReply* defer) override;

    void SlogSendBatchRemote(const std::vector<std::pair<txnid_t, TxnOutput>> &batch,
                             const parid_t &my_par,
                             rrr::i32 *res, rrr::DeferredReply *defer) override;

   void SlogSendTxnOutput(const txnid_t& txn_id, const parid_t &parid,  const TxnOutput& output, rrr::i32* res, rrr::DeferredReply* defer) override;

   void SlogSendDepValues(const ChronosSendOutputReq& chr_req, ChronosSendOutputRes* chr_res, rrr::DeferredReply* defer) override;



protected:
    void RegisterStats();
};

}// namespace rococo

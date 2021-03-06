//
// Created by micha on 2020/3/23.
//


#include "ov-scheduler.h"
#include "ov-commo.h"
#include "ov-tx.h"
#include <climits>
#include "deptran/frame.h"
#include <limits>

using namespace rococo;
class Frame;

SchedulerOV::SchedulerOV(Config::SiteInfo *site_info) : BrqSched() {
  tid_mgr_ = std::make_unique<TidMgr>(site_info->id);
  site_info_ = site_info;
  config_ = Config::GetConfig();
  verify(site_info_ != nullptr);
  verify(config_ != nullptr);
  gossiper_inited_ = false;

//    vwatermark_ = ov_ts_t(std::numeric_limits<int64_t>::max(), 0);
  vwatermark_ = ov_ts_t(0, 0);
  std::set<std::string> sites_in_my_dc;

  for (auto &s: config_->sites_) {
    std::string site = s.name;
    std::string proc = config_->site_proc_map_[site];
    std::string host = config_->proc_host_map_[proc];
    std::string dc = config_->host_dc_map_[host];
    if (dc == site_info_->dcname) {
      Log_debug("stte [%s] is in the same dc [%s] as me (%s)", site.c_str(), dc.c_str(), site_info->name.c_str());
      sites_in_my_dc.insert(site);
    }
  }

  if (site_info_->name == *(sites_in_my_dc.begin())) {
    Log_info("I [%s] am the first proc in my dc [%s], creating gossiper",
             site_info->name.c_str(),
             site_info->dcname.c_str());
    gossiper_ = new OVGossiper(config_, site_info);
  }
}

int SchedulerOV::OnDispatch(const vector<SimpleCommand>& cmd,
                                 int32_t* res,
                                 TxnOutput* output) {
  std::lock_guard<std::recursive_mutex> guard(mtx_);

  txnid_t txn_id = cmd[0].root_id_; //should have the same root_id
  Log_debug("[Scheduler %d] On Dispatch, txn_id = %d, gossiper_inited_", this->frame_->site_info_->id, txn_id, gossiper_inited_);

  auto dtxn = (TxOV* )(GetOrCreateDTxn(txn_id)); //type is shared_pointer
  verify(dtxn->id() == txn_id);
  verify(cmd[0].partition_id_ == Scheduler::partition_id_);



  for (auto& c : cmd) {
    dtxn->DispatchExecute(const_cast<SimpleCommand&>(c),
                          res, &(*output)[c.inn_id()]);
  }


  dtxn->UpdateStatus(TXN_STD); //started
  verify(cmd[0].root_id_ == txn_id);


  return 0;
}



void SchedulerOV::OnStore(const txnid_t txn_id,
                                   const vector<SimpleCommand> &cmds,
                                   const OVStoreReq &ov_req,
                                   int32_t *res,
                                   OVStoreRes *ov_res) {

  std::lock_guard<std::recursive_mutex> lock(mtx_);
  Log_debug("asdfgh %s called on site %hu, txnid = %lu", __FUNCTION__, this->site_id_, txn_id);
  verify(txn_id > 0);
  verify(cmds[0].root_id_ == txn_id);

  if (gossiper_inited_ == false){
    gossiper_inited_ = true;
    if (gossiper_ != nullptr){
      Log_debug("setting frame and commo for %s", __FUNCTION__ , frame_->site_info_->name.c_str());
      gossiper_->frame_ = frame_;
      gossiper_->commo_ = commo();
      gossiper_->StartLoop();
    }
  }



  auto dtxn = (TxOV*)(GetOrCreateDTxn(txn_id));

  dtxn->UpdateStatus(TXN_PAC);

  dtxn->involve_flag_ = RccDTxn::INVOLVED;
  TxOV &tinfo = *dtxn;

    //Normal case
  if (dtxn->status() < TXN_CMT) {
    if (dtxn->phase_ < PHASE_CHRONOS_PRE_ACCEPT && tinfo.status() < TXN_CMT) {
      for (auto &c: cmds) {
        map<int32_t, Value> output;
        dtxn->PreAcceptExecute(const_cast<SimpleCommand &>(c), res, &output);
      }
    }
  } else {
    if (dtxn->dreqs_.size() == 0) {
      for (auto &c: cmds) {
        dtxn->dreqs_.push_back(c);
      }
    }
  }
  verify(!tinfo.fully_dispatched);

  tinfo.fully_dispatched = true;

  dtxn->ovts_.timestamp_ = ov_req.ts;
  dtxn->ovts_.site_id_ = ov_req.site_id;

  TxOV* txovptr = dynamic_cast<TxOV*>(dtxn);

  txovptr->ov_status_ = TxOV::OV_txn_status::STORED;

  stored_txns_by_id_[txn_id] = txovptr;

  *res = SUCCESS;
}


void SchedulerOV::OnCreateTs (txnid_t txnid,
                 int64_t *timestamp,
                 int16_t *server_id){


  std::lock_guard<std::recursive_mutex> lock(mtx_);
  ov_ts_t ovts = tid_mgr_->CreateTs(txnid);


  *timestamp = ovts.timestamp_;
  *server_id = siteid_t(ovts.site_id_);

  return;
}

void SchedulerOV::OnStoredRemoveTs(uint64_t txnid, int64_t timestamp, int16_t server_id, int32_t *res) {
  std::lock_guard<std::recursive_mutex> lock(mtx_);

  Log_debug("asdfgh %s called on site %hu, txnid = %lu", __FUNCTION__, this->site_id_, txnid);
  tid_mgr_->StoredTs(txnid, timestamp, server_id);
  *res = SUCCESS;

  return;
}

void SchedulerOV::OnPublish(int64_t dc_ts,
                            int16_t dc_id,
                            int64_t *ret_ts,
                            int16_t *ret_id) {
  std::lock_guard<std::recursive_mutex> lock(mtx_);

  ov_ts_t dc_ovts(dc_ts, dc_id);

  if (dc_ovts > this->vwatermark_) {
    this->vwatermark_ = dc_ovts;
    Log_debug("Scheduler [%d]'s vwatermark has been set to %ld.%d, will execute pending reqs",
             this->site_id_,
             vwatermark_.timestamp_,
             vwatermark_.site_id_);
    for (auto it = stored_txns_by_id_.cbegin(); it != stored_txns_by_id_.cend(); ) {
      TxOV *dtxn = it->second;
      txnid_t txn_id = it->first;
      if (dtxn->ovts_ < this->vwatermark_ && dtxn->ov_status_ == TxOV::OV_txn_status::CAN_EXECUTE){
        Log_debug("[Scheduler %hu] Executing txn %lu with timestamp %ld.%d < vwatermark %ld.%d",
                 this->site_id_,
                 txn_id,
                 dtxn->ovts_.timestamp_,
                 dtxn->ovts_.site_id_,
                 vwatermark_.timestamp_,
                 vwatermark_.site_id_);
        verify(!dtxn->IsExecuted());
        dtxn->CommitExecute();
        it = stored_txns_by_id_.erase(it);
        dtxn->executed_callback();
      }else{
        ++it;
      }
    }
  }

  ov_ts_t svw = tid_mgr_->GetServerVWatermark();
  *ret_ts = svw.timestamp_;
  *ret_id = svw.site_id_;

  return;
}

void SchedulerOV::OnExchange(const std::string &source_dcname,
                             int64_t dvw_ts,
                             int16_t dvw_id,
                             int64_t *ret_ts,
                             int16_t *ret_id) {
  verify(this->gossiper_ != nullptr);

  ov_ts_t dvw_ovts(dvw_ts,  dvw_id);

  ov_ts_t ret_ovts;
  gossiper_->OnExchange(source_dcname, dvw_ovts, ret_ovts);
  *ret_ts = ret_ovts.timestamp_;
  *ret_id = ret_ovts.site_id_;
  return;

}

void SchedulerOV::OnExecute(uint64_t txn_id,
                            const OVExecuteReq &req,
                            int32_t *res,
                            OVExecuteRes *ov_res,
                            TxnOutput *output,
                            const function<void()> &callback) {
  std::lock_guard<std::recursive_mutex> lock(mtx_);

  *res = SUCCESS;

  auto dtxn = (TxOV *) (GetOrCreateDTxn(txn_id));
  verify(dtxn->ptr_output_repy_ == nullptr);
  dtxn->ptr_output_repy_ = output;
  dtxn->ov_status_ = TxOV::OV_txn_status::CAN_EXECUTE;

  if (dtxn->ovts_> vwatermark_){
    Log_debug("scheduler %hu,  txn %lu vwatermark not ready, postpone execution, vwatermark = %ld, ts = %ld", site_id_, txn_id, vwatermark_.timestamp_, dtxn->ovts_.timestamp_);
    dtxn->executed_callback = callback;
  }
  else{
    verify(!dtxn->IsExecuted());
    dtxn->CommitExecute();
    stored_txns_by_id_.erase(txn_id);
    Log_debug("scheuler %hu, txn %lu executed, vwatermark = %ld, ts = %ld", site_id_, txn_id,  vwatermark_.timestamp_, dtxn->ovts_.timestamp_);
    dtxn->ov_status_ = TxOV::OV_txn_status::EXECUTED;
    callback();
  }

  return;
}

OVCommo *SchedulerOV::commo() {

  auto commo = dynamic_cast<OVCommo *>(commo_);
  verify(commo != nullptr);
  return commo;
}



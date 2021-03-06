#include "ov-txn_mgr.h"
#include <chrono>


namespace rococo {


void TidMgr::GetMonotTimestamp(ov_ts_t &ovts) {
  auto now = std::chrono::system_clock::now();

  int64_t ts = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() + this->time_drift_ms_;

  if (ts <= last_clock_){
    ts = ++last_clock_;
  }else{
    last_clock_ = ts;
  }

  ovts.timestamp_ = ts;
  ovts.site_id_ = site_id_;
  return;
}

ov_ts_t TidMgr::CreateTs(mdb::txn_id_t txn_id) {
//  std::unique_lock<std::mutex> lk(mu);
  ov_ts_t ovts;

  GetMonotTimestamp(ovts);
  s_phase_txns_[ovts] = txn_id;

  Log_debug("TidMgr %hu created Ts for txn %lu, ovts = %ld.%d ", this->site_id_, txn_id, ovts.timestamp_, ovts.site_id_);
  return ovts;
}

void TidMgr::StoredTs(mdb::txn_id_t txn_id, int64_t timestamp, int16_t server_id) {
 ov_ts_t ovts;
 ovts.timestamp_ = timestamp;
 ovts.site_id_ = server_id;

 Log_debug("TidMgr %hu stored (to remove) Ts for txn %lu, ts = %ld, ovts.ts = %ld ", this->site_id_, txn_id, timestamp, ovts.timestamp_);

 verify(s_phase_txns_.count(ovts) != 0);

 if (s_phase_txns_[ovts] != txn_id){
   Log_debug("asdf looking for txn_id %lu, but foun %lu", txn_id, s_phase_txns_[ovts]);
 }

 verify(s_phase_txns_[ovts] == txn_id);

 s_phase_txns_.erase(ovts);
}


ov_ts_t TidMgr::GetServerVWatermark() {
  ov_ts_t ovts;

  if (s_phase_txns_.size() == 0){
    GetMonotTimestamp(ovts);
  }else{
    ovts = s_phase_txns_.begin()->first;
  }
  return ovts;
}






}//namespace rococo
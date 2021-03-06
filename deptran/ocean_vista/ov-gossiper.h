//
// Created by micha on 2020/4/7.
//

#ifndef ROCOCO_DEPTRAN_OCEAN_VISTA_OV_GOSSIPER_H_
#define ROCOCO_DEPTRAN_OCEAN_VISTA_OV_GOSSIPER_H_
#include "deptran/config.h"
#include "ov-txn_mgr.h"
#include <memory>


namespace rococo {
class ClassicProxy;
class OVCommo;
class OVGossiper {
 public:
  OVGossiper(Config *config, Config::SiteInfo *info);


  Config *config_;
  Config::SiteInfo* site_info_;
  std::string my_dcname_;
  Frame *frame_;
  OVCommo *commo_;

  int gossip_interval_ms_;
  int aggregate_interval_ms_;

  std::thread gossip_thread_;
  std::thread aggregrate_thread_;

  std::recursive_mutex mu_;


  std::map<std::string, std::string> peer_gossiper_dc_site_map_;
  std::map<std::string, siteid_t> peer_gossiper_dc_siteid_map_;

  std::map<std::string, ov_ts_t> dc_watermarks_; //watermark of each dc.
  std::map<siteid_t, ov_ts_t> site_watermarks_;  //sitewater mark of sites in my dc.

  ov_ts_t my_vwatermark_;

  void Aggregate();
  void PublishAck(siteid_t site_id, const ov_ts_t& ret_ovts);
  std::set<siteid_t> publish_ack_received_;

  void Gossip();
  void ExchangeAck(siteid_t site_id, const std::string &dcname, const ov_ts_t& ret_ovts);
  std::set<siteid_t> exchange_ack_received_;

  void OnExchange(const std::string dcname, const ov_ts_t& dvw_ovts, ov_ts_t& ret_ovts);

  void GossipLoop();
  void AggregateLoop();

  std::condition_variable gossip_cond_;
  std::mutex gossip_cond_mu_;
  bool gossip_flag_;


  std::condition_variable aggregate_cond_;
  std::mutex aggregate_cond_mu_;
  bool aggregate_flag_;

  void StartLoop();
};
}
#endif //ROCOCO_DEPTRAN_OCEAN_VISTA_OV_GOSSIPER_H_

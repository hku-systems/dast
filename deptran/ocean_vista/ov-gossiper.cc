//
// Created by micha on 2020/4/7.
//

#include "ov-gossiper.h"
#include "ov-commo.h"
#include "unistd.h"

namespace rococo{

OVGossiper::OVGossiper(rococo::Config *config, rococo::Config::SiteInfo *info) {
  config_ = config;
  site_info_ = info;

  this->aggregate_interval_ms_ = config_->ov_aggregate_interval_ms_;
  this->gossip_interval_ms_ = config_->ov_gossip_interval_ms_;

  Log_info("OVGossiper Created, aggregate_interval_ms = %d, gossip_interval_ms = %d", this->aggregate_interval_ms_, this->gossip_interval_ms_);


  //Get the proxy to peers;
  std::map<std::string, std::set<std::string>> sites_by_dc_;
  for (auto &s: config_->sites_){
    std::string site = s.name;
    std::string proc = config_->site_proc_map_[site];
    std::string host = config_->proc_host_map_[proc];
    std::string dc = config_->host_dc_map_[host];
    if (sites_by_dc_.count(dc) == 0){
      std::set<std::string> sites;
      sites.insert(site);
      sites_by_dc_[dc] = sites;
    }
    else{
      sites_by_dc_[dc].insert(site);
    }
  }
  /*
   * xs TODO: why don't I change the mapping between site and proc to use site_id instead of name
   * not emergent, as these happens during init time.
  */
  for (auto &dc_s: sites_by_dc_){
    ov_ts_t ovts;
    dc_watermarks_[dc_s.first] = ovts;
    Log_debug("Init dc watermark for dc [%s] to zero", dc_s.first.c_str());
    my_dcname_ = info->dcname;

    if (dc_s.first != info->dcname){
      std::string gossiper_site_name = *(dc_s.second.begin());
      peer_gossiper_dc_site_map_[dc_s.first] = gossiper_site_name;
      int index;
      for (index = 0; index < config_->sites_.size(); index++){
        if (config->sites_[index].name == gossiper_site_name){
          break;
        }
      }
      verify(index != config_->sites_.size());
      peer_gossiper_dc_siteid_map_[dc_s.first] = (siteid_t) index;
      Log_debug("Gossiper found peer [%s, id = %d] at dc [%s]", gossiper_site_name.c_str(), index, dc_s.first.c_str());
    }
    else{
      //sites of my dc;
      for (auto& site : dc_s.second){
        int index;
        for (index = 0; index < config_->sites_.size(); index++){
          if (config->sites_[index].name == site){
            break;
          }
        }
        ov_ts_t ovts;
        site_watermarks_[(siteid_t)index] = ovts;
        Log_debug("Init site water mark for site_id %d to zero", index);
      }
    }
  }
}


void OVGossiper::OnExchange(const std::string dcname, const ov_ts_t &dvw_ovts, ov_ts_t &ret_ovts) {
  std::lock_guard<std::recursive_mutex> lk(mu_);
  if (dvw_ovts > dc_watermarks_[dcname]){
    dc_watermarks_[dcname] = dvw_ovts;
  }
  ret_ovts = dc_watermarks_[my_dcname_];
  Log_debug("Received Exchange Request from %s, his ovts is set to %ld.%d, returning my ovts %ld.%d",
           dcname.c_str(),
           dc_watermarks_[dcname].timestamp_,
           dc_watermarks_[dcname].site_id_,
           dc_watermarks_[my_dcname_].timestamp_,
           dc_watermarks_[my_dcname_].site_id_);
  return;
}

void OVGossiper::Aggregate(){
  std::lock_guard<std::recursive_mutex> lk(mu_);
  publish_ack_received_.clear();
  for (auto &pair: site_watermarks_){
    siteid_t siteid = pair.first;
    auto callback = std::bind(&OVGossiper::PublishAck,
                            this,
                            siteid,
                            std::placeholders::_1);
    verify(commo_ != nullptr);
    commo_->SendPublish(siteid, my_vwatermark_, callback);
  }
}

void OVGossiper::PublishAck(uint16_t site_id, const ov_ts_t &ret_ovts) {
  std::lock_guard<std::recursive_mutex> lk(mu_);
  //Note: this does not handle failure or straggler, as in the paper
  publish_ack_received_.insert(site_id); //for checking each round
  verify(site_watermarks_.count(site_id) != 0);
  if(ret_ovts > site_watermarks_[site_id]){
    site_watermarks_[site_id] = ret_ovts;
  }
  Log_debug("Received Publish result from site %d, timestamp = %ld.%d, total_size = %d, received size = %d", site_id, ret_ovts.timestamp_, ret_ovts.site_id_, site_watermarks_.size(), publish_ack_received_.size());
  if (publish_ack_received_.size() == site_watermarks_.size()){
     ov_ts_t min_ovts = site_watermarks_.begin()->second;
     for (auto &r: site_watermarks_){
       if (r.second < min_ovts){
           min_ovts = r.second;
       }
     }
     this->dc_watermarks_[my_dcname_] = min_ovts;
     Log_debug("my dc_watermark (dw[%s]) has been set to %ld.%d", my_dcname_.c_str(), min_ovts.timestamp_, min_ovts.site_id_);
     std::unique_lock<std::mutex> lk(aggregate_cond_mu_);
     aggregate_flag_ = true;
     aggregate_cond_.notify_all();
     lk.unlock();
  }
}

void OVGossiper::Gossip() {
  std::lock_guard<std::recursive_mutex> lk(mu_);
  exchange_ack_received_.clear();
  for (auto &pair: peer_gossiper_dc_siteid_map_){
    std::string dcname = pair.first;
    siteid_t siteid = pair.second;
    auto callback = std::bind(&OVGossiper::ExchangeAck,
                              this,
                              siteid,
                              dcname,
                              std::placeholders::_1);
    verify(commo_ != nullptr);
    commo_->SendExchange(siteid, my_dcname_, dc_watermarks_[my_dcname_], callback);
  }
}

void OVGossiper::ExchangeAck(uint16_t site_id, const std::string &dcname, const ov_ts_t &ret_ovts) {
  std::lock_guard<std::recursive_mutex> lk(mu_);
  //Note: this does not handle failure or straggler, as in the paper
  exchange_ack_received_.insert(site_id);
  verify(dc_watermarks_.count(dcname) != 0);
  if (ret_ovts > dc_watermarks_[dcname]){
    dc_watermarks_[dcname] = ret_ovts;
  }
  Log_debug("Received Exchange result from site [id = %d] at dc [%s], timestamp = %ld.%d", site_id, dcname.c_str(), ret_ovts.timestamp_, ret_ovts.site_id_);
  if (exchange_ack_received_.size() == peer_gossiper_dc_siteid_map_.size()){
    ov_ts_t min_ovts = dc_watermarks_.begin()->second;
    for (auto &r: dc_watermarks_){
      if (r.second < min_ovts){
        min_ovts = r.second;
      }
    }
    this->my_vwatermark_ = min_ovts;
    Log_debug("My vwatermark has been set to %ld.%d", min_ovts.timestamp_, min_ovts.site_id_);
    std::unique_lock<std::mutex> lk(gossip_cond_mu_);
    gossip_flag_ = true;
    gossip_cond_.notify_all();
    lk.unlock();
  }
}

void OVGossiper::GossipLoop() {
  Gossip(); // The first call
  while(true){
    std::unique_lock<std::mutex> lk(gossip_cond_mu_);
    gossip_cond_.wait(lk, [this]{return gossip_flag_;});
    gossip_flag_ = false;
    lk.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds{gossip_interval_ms_});
    Gossip();
  }
}

void OVGossiper::AggregateLoop() {
  Aggregate();
  while(true){
    std::unique_lock<std::mutex> lk(aggregate_cond_mu_);
    aggregate_cond_.wait(lk, [this]{return aggregate_flag_;});
    aggregate_flag_ = false;
    lk.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds{aggregate_interval_ms_});
    Aggregate();
  }
}

void OVGossiper::StartLoop() {
  gossip_thread_ = std::thread(&OVGossiper::GossipLoop, this);
  gossip_thread_.detach();

  aggregrate_thread_ = std::thread(&OVGossiper::AggregateLoop, this);
  aggregrate_thread_.detach();
}
}//namespace rococo

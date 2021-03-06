
#include "communicator.h"
#include "command.h"
#include "command_marshaler.h"
#include "rcc/graph.h"
#include "rcc/graph_marshaler.h"
#include "rcc_rpc.h"
#include "txn_chopper.h"


namespace rococo {

using namespace std::chrono;

Communicator::Communicator(PollMgr *poll_mgr) {
    vector<string> addrs;
    if (poll_mgr == nullptr)
        rpc_poll_ = new PollMgr(1);
    else
        rpc_poll_ = poll_mgr;
    auto config = Config::GetConfig();
    vector<parid_t> partitions = config->GetAllPartitionIds();
    for (auto &par_id : partitions) {
        auto site_infos = config->SitesByPartitionId(par_id);
        vector<std::pair<siteid_t, ClassicProxy *>> proxies;
        for (auto &si : site_infos) {
            auto result = ConnectToSite(si, milliseconds(CONNECT_TIMEOUT_MS));
            verify(result.first == SUCCESS);
            proxies.push_back(std::make_pair(si.id, result.second));
            //save the according to dc
            if (rpc_dc_proxies_.count(si.dcname) == 0) {
                vector<std::pair<siteid_t, ClassicProxy *>> dcproxies;
                dcproxies.push_back(std::make_pair(si.id, result.second));
                rpc_dc_proxies_.insert(std::make_pair(si.dcname, dcproxies));
            } else {
                rpc_dc_proxies_[si.dcname].push_back(std::make_pair(si.id, result.second));
            }
        }

        rpc_par_proxies_.insert(std::make_pair(par_id, proxies));
    }
    //xs print for understanding.


      for (auto &par_item: rpc_par_proxies_){
        for (auto &pair : par_item.second) {
          Log_info("[rpc_par_proxies]: par_id = %d, site = %d", par_item.first, pair.first);
        }
      }
    if (Config::GetConfig()->get_mode() == MODE_SLOG) {
        ConnectToRaft();
        ConnectToSlogManager();
    }

    if (Config::GetConfig()->get_mode() == MODE_CHRONOS) {
        ConnectToDastManager();
    }


    client_leaders_connected_.store(false);
    if (config->forwarding_enabled_) {
        threads.push_back(std::thread(&Communicator::ConnectClientLeaders, this));
    } else {
        client_leaders_connected_.store(true);
    }
}

void Communicator::ConnectClientLeaders() {
    auto config = Config::GetConfig();
    if (config->forwarding_enabled_) {
        Log_debug("%s: connect to client sites", __FUNCTION__);
        auto client_leaders = config->SitesByLocaleId(0, Config::CLIENT);
        for (Config::SiteInfo leader_site_info : client_leaders) {
            verify(leader_site_info.locale_id == 0);
            Log_debug("client @ leader %d", leader_site_info.id);
            auto result = ConnectToClientSite(leader_site_info,
                                              milliseconds(CONNECT_TIMEOUT_MS));
            verify(result.first == SUCCESS);
            verify(result.second != nullptr);
            Log_debug("connected to client leader site: %d, %d, %p",
                      leader_site_info.id,
                      leader_site_info.locale_id,
                      result.second);
            client_leaders_.push_back(std::make_pair(leader_site_info.id,
                                                     result.second));
        }
    }
    client_leaders_connected_.store(true);
}

void Communicator::WaitConnectClientLeaders() {
    bool connected;
    do {
        connected = client_leaders_connected_.load();
    } while (!connected);
    Log_info("Done waiting to connect to client leaders.");
}

Communicator::~Communicator() {
    verify(rpc_clients_.size() > 0);
    for (auto &pair : rpc_clients_) {
        rrr::Client *rpc_cli = pair.second;
        rpc_cli->close_and_release();
    }
    rpc_clients_.clear();
}

std::pair<siteid_t, ClassicProxy *>
Communicator::RandomProxyForPartition(parid_t par_id) const {
    auto it = rpc_par_proxies_.find(par_id);
    verify(it != rpc_par_proxies_.end());
    auto &par_proxies = it->second;
    int index = rrr::RandomGenerator::rand(0, par_proxies.size() - 1);
    return par_proxies[index];
}

std::pair<siteid_t, ClassicProxy *>
Communicator::LeaderProxyForPartition(parid_t par_id) const {
    auto leader_cache = const_cast<map<parid_t, SiteProxyPair> &>(this->leader_cache_);
    auto leader_it = leader_cache.find(par_id);
    if (leader_it != leader_cache.end()) {
        return leader_it->second;
    } else {
        auto it = rpc_par_proxies_.find(par_id);
        verify(it != rpc_par_proxies_.end());
        auto &partition_proxies = it->second;
        auto config = Config::GetConfig();
        auto proxy_it = std::find_if(partition_proxies.begin(),
                                     partition_proxies.end(),
                                     [config](const std::pair<siteid_t, ClassicProxy *> &p) {
                                         auto &site = config->SiteById(p.first);
                                         return site.locale_id == 0;
                                     });
        if (proxy_it == partition_proxies.end()) {
            Log_fatal("could not find leader for partition %d", par_id);
        } else {
            leader_cache[par_id] = *proxy_it;
            Log_debug("leader site for parition %d is %d", par_id, proxy_it->first);
        }
        return *proxy_it;
    }
}

ClientSiteProxyPair
Communicator::ConnectToClientSite(Config::SiteInfo &site,
                                  milliseconds timeout) {
    auto config = Config::GetConfig();
    char addr[1024];
    snprintf(addr, sizeof(addr), "%s:%d", site.host.c_str(), site.port);

    auto start = steady_clock::now();
    rrr::Client *rpc_cli = new rrr::Client(rpc_poll_);
    double elapsed;
    int attempt = 0;
    do {
        Log_debug("connect to client site: %s (attempt %d)", addr, attempt++);
        auto connect_result = rpc_cli->connect(addr);
        if (connect_result == SUCCESS) {
            ClientControlProxy *rpc_proxy = new ClientControlProxy(rpc_cli);
            rpc_clients_.insert(std::make_pair(site.id, rpc_cli));
            Log_debug("connect to client site: %s success!", addr);
            return std::make_pair(SUCCESS, rpc_proxy);
        } else {
            std::this_thread::sleep_for(milliseconds(CONNECT_SLEEP_MS));
        }
        auto end = steady_clock::now();
        elapsed = duration_cast<milliseconds>(end - start).count();
    } while (elapsed < timeout.count());
    Log_debug("timeout connecting to client %s", addr);
    rpc_cli->close_and_release();
    return std::make_pair(FAILURE, nullptr);
}

void Communicator::ConnectToDastManager() {
    auto global_cfg = Config::GetConfig();

    Log_info("%s called, mode %d", __FUNCTION__, global_cfg->get_mode());
    dast_manager_poll_ = new PollMgr(1);
    int timeout_ms = CONNECT_TIMEOUT_MS;

    //Currently assume all pars have the same number of replicas.
    //Use the number for partition 0.
    //There must be par 0.
    int sites_per_region = Config::GetConfig()->n_shard_per_region_ * global_cfg->GetPartitionSize(0);

    for (auto &s: global_cfg->sites_){
        if (s.id % sites_per_region == 0){
            regionid_t rid = s.id / sites_per_region;
            uint32_t port = 9000 + rid;
            string addr = global_cfg->proc_host_map_[s.proc_name] + ":" +std::to_string(port);

            Log_info("[Dast Manager] Connecting to manager of region %u at site %hu, addr = %s", rid, s.id, addr.c_str());
            auto start = steady_clock::now();
            double elapsed;
            int attempt = 0;
            do {
                Log_info("connect to site: %s (attempt %d)", addr.c_str(), attempt++);
                auto c = new rrr::Client(dast_manager_poll_);
                auto connect_result = c->connect(addr.c_str());
                if (connect_result == SUCCESS) {
                    dast_manager_clients_[rid] = c;
                    auto p = new DastManagerProxy(c);
                    dast_rpc_region_managers_[rid] = p;
                    Log_info("connect to dast manager: %s success!", addr.c_str());
                    break;
                } else {
                    std::this_thread::sleep_for(milliseconds(CONNECT_SLEEP_MS));
                }
                auto end = steady_clock::now();
                elapsed = duration_cast<milliseconds>(end - start).count();
            } while (elapsed < timeout_ms);
            if (elapsed >= timeout_ms) {
                Log_fatal("timeout connecting to %s", addr.c_str());
            }

        }
    }

}


void Communicator::ConnectToSlogManager() {
    auto global_cfg = Config::GetConfig();

    Log_info("%s called, mode %d", __FUNCTION__, global_cfg->get_mode());
    slog_manager_poll_ = new PollMgr(1);
    int timeout_ms = CONNECT_TIMEOUT_MS;

    //Currently assume all pars have the same number of replicas.
    //Use the number for partition 0.
    //There must be par 0.
    int sites_per_region = Config::GetConfig()->n_shard_per_region_ * N_REP_PER_SHARD;

    for (auto &s: global_cfg->sites_){
        if (s.id % sites_per_region == 0 || s.id % sites_per_region == 1){
            regionid_t rid = s.id / sites_per_region;
            uint32_t port = 9000 + s.id;
            string addr = global_cfg->proc_host_map_[s.proc_name] + ":" +std::to_string(port);

            Log_info("[slog Manager] Connecting to manager of region %u at site %hu, addr = %s", rid, s.id, addr.c_str());
            auto start = steady_clock::now();
            double elapsed;
            int attempt = 0;
            do {
                Log_info("connect to site: %s (attempt %d)", addr.c_str(), attempt++);
                auto c = new rrr::Client(slog_manager_poll_);
                auto connect_result = c->connect(addr.c_str());
                if (connect_result == SUCCESS) {
                    slog_manager_clients_[rid] = c;
                    auto p = new SlogRegionMangerProxy(c);
                    slog_rpc_region_managers_[s.id] = p;
                    Log_info("connect to slog manager: %s success!", addr.c_str());
                    break;
                } else {
                    std::this_thread::sleep_for(milliseconds(CONNECT_SLEEP_MS));
                }
                auto end = steady_clock::now();
                elapsed = duration_cast<milliseconds>(end - start).count();
            } while (elapsed < timeout_ms);
            if (elapsed >= timeout_ms) {
                Log_fatal("timeout connecting to %s", addr.c_str());
            }

        }
    }

}

void Communicator::ConnectToRaft() {
    auto glbal_config = Config::GetConfig();

    Log_info("%s called, mode %d", __FUNCTION__, glbal_config->get_mode());
    raft_rpc_poll_ = new PollMgr(1);
    int timeout_ms = CONNECT_TIMEOUT_MS;
    auto global_cfg = Config::GetConfig();
    verify(global_cfg->raft_port_map_.size() > 0);

    for (auto &pair : global_cfg->raft_port_map_) {
        std::string proc = pair.first;
        int port = pair.second;
        auto addr = global_cfg->proc_host_map_[proc] + ":" + std::to_string(port);
        auto start = steady_clock::now();
        double elapsed;
        int attempt = 0;
        do {
            Log_info("connect to site: %s (attempt %d)", addr.c_str(), attempt++);
            auto c = new rrr::Client(raft_rpc_poll_);
            auto connect_result = c->connect(addr.c_str());
            if (connect_result == SUCCESS) {
                raft_clients_.push_back(c);
                auto p = new SlogRaftProxy(c);
                raft_proxies_.push_back(p);
                Log_info("connect to raft leader: %s success!", addr.c_str());
                break;
            } else {
                std::this_thread::sleep_for(milliseconds(CONNECT_SLEEP_MS));
            }
            auto end = steady_clock::now();
            elapsed = duration_cast<milliseconds>(end - start).count();
        } while (elapsed < timeout_ms);
        if (elapsed >= timeout_ms) {
            Log_fatal("timeout connecting to %s", addr.c_str());
        }
    }
}

std::pair<int, ClassicProxy *>
Communicator::ConnectToSite(Config::SiteInfo &site,
                            milliseconds timeout) {
    string addr = site.GetHostAddr();
    auto start = steady_clock::now();
    rrr::Client *rpc_cli = new rrr::Client(rpc_poll_);
    double elapsed;
    int attempt = 0;
    do {
        Log_debug("connect to site: %s (attempt %d)", addr.c_str(), attempt++);
        auto connect_result = rpc_cli->connect(addr.c_str());
        if (connect_result == SUCCESS) {
            ClassicProxy *rpc_proxy = new ClassicProxy(rpc_cli);
            rpc_clients_.insert(std::make_pair(site.id, rpc_cli));
            rpc_proxies_.insert(std::make_pair(site.id, rpc_proxy));
            Log_debug("connect to site: %s success!", addr.c_str());
            return std::make_pair(SUCCESS, rpc_proxy);
        } else {
            std::this_thread::sleep_for(milliseconds(CONNECT_SLEEP_MS));
        }
        auto end = steady_clock::now();
        elapsed = duration_cast<milliseconds>(end - start).count();
    } while (elapsed < timeout.count());
    Log_debug("timeout connecting to %s", addr.c_str());
    rpc_cli->close_and_release();
    return std::make_pair(FAILURE, nullptr);
}

std::pair<siteid_t, ClassicProxy *>
Communicator::NearestProxyForPartition(parid_t par_id) const {
    // TODO Fix me.
    Log_debug("%s called, loc_id_ = %d", __FUNCTION__ , loc_id_);
    auto it = rpc_par_proxies_.find(par_id);
    verify(it != rpc_par_proxies_.end());
    auto &partition_proxies = it->second;
    verify(partition_proxies.size() > loc_id_);
    int index = loc_id_;
    return partition_proxies[index];
};


std::pair<siteid_t, ClassicProxy *>
Communicator::EdgeServerForPartition(parid_t par_id) {
    // TODO Fix me.
    auto it = rpc_par_proxies_.find(par_id);
    //  Log_info("Sending to par %u", par_id);
    verify(it != rpc_par_proxies_.end());
    auto &partition_proxies = it->second;

    int index = last_edge_index++ % partition_proxies.size();
    //  verify(partition_proxies.size() > 0);
    return partition_proxies[index];
};

std::pair<siteid_t, ClassicProxy *>
Communicator::NearestProxyForAnyPartition(const std::vector<parid_t> &par_ids) const {
    // TODO Fix me.
    //xs todo, currently, the code assumes full partition.
    //The way it find nearest proxy is to find the partition of the same location
    verify(!par_ids.empty());
    auto par_id = par_ids[0];
    auto it = rpc_par_proxies_.find(par_id);
    verify(it != rpc_par_proxies_.end());
    auto &partition_proxies = it->second;
    verify(partition_proxies.size() > loc_id_);
    int index = loc_id_;
    return partition_proxies[index];
};


std::pair<siteid_t, ClassicProxy *>
Communicator::NearestRandomProxy() {
    // TODO Fix me.
    //xs todo, currently, the code assumes full partition.
    //The way it find nearest proxy is to find the partition of the same location
    if (rpc_dc_proxies_.count(this->dcname_) != 0) {
        auto &dc_proxies = rpc_dc_proxies_[this->dcname_];
        int rand_proxy = rand() % dc_proxies.size();
        Log_debug("my dc [%s] has %d proxies, using the %d-th as the nearest rand proxy", dcname_.c_str(), dc_proxies.size(), rand_proxy);
        return dc_proxies[rand_proxy];
    } else {
        int rand_dc = rand() % rpc_dc_proxies_.size();
        auto dc_itr = rpc_par_proxies_.begin();
        std::advance(dc_itr, rand_dc);

        auto &dc_proxies = dc_itr->second;
        int rand_proxy = rand() % dc_proxies.size();
        Log_debug("my dc has no proxy, using dc [%s] has %d proxies, using the %d-th as the nearest rand proxy", dc_itr->first, dc_proxies.size(), rand_proxy);
        return dc_proxies[rand_proxy];
    }
};

std::vector<std::pair<siteid_t, ClassicProxy *>>
Communicator::ProxiesInPartition(uint32_t par_id) {
    return rpc_par_proxies_[par_id];
}


}// namespace rococo
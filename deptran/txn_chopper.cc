#include "txn_chopper.h"
#include "__dep__.h"
#include "benchmark_control_rpc.h"
#include "coordinator.h"
#include "marshal-value.h"

// deprecated below
//#include "rcc/rcc_srpc.h"
// deprecate above

namespace rococo {

TxnWorkspace::TxnWorkspace() {
    values_ = std::make_shared<map<int32_t, Value>>();
}

TxnWorkspace::~TxnWorkspace() {
    //  delete values_;
}

TxnWorkspace::TxnWorkspace(const TxnWorkspace &rhs)
    : keys_(rhs.keys_),
      values_{rhs.values_},
      input_var_ready_{rhs.input_var_ready_},
      piece_input_ready_(rhs.piece_input_ready_) {
}

void TxnWorkspace::Aggregate(const TxnWorkspace &rhs) {
    keys_.insert(rhs.keys_.begin(), rhs.keys_.end());
    if (values_ != rhs.values_) {
        values_->insert(rhs.values_->begin(), rhs.values_->end());
    }
}

TxnWorkspace &TxnWorkspace::operator=(const TxnWorkspace &rhs) {
    keys_ = rhs.keys_;
    values_ = rhs.values_;
    input_var_ready_ = rhs.input_var_ready_;
    piece_input_ready_ = rhs.piece_input_ready_;
    return *this;
}

TxnWorkspace &TxnWorkspace::operator=(const map<int32_t, Value> &rhs) {
    //  Log_info("%s called",  __FUNCTION__);
    keys_.clear();
    for (const auto &pair : rhs) {
        keys_.insert(pair.first);
    }
    *values_ = rhs;
    return *this;
}

Value &TxnWorkspace::operator[](size_t idx) {
    //Log_info("%s called",  __FUNCTION__);
    keys_.insert(idx);
    return (*values_)[idx];
}

TxnCommand::TxnCommand() {
    clock_gettime(&start_time_);
    read_only_failed_ = false;
    pre_time_ = timespec2ms(start_time_);
    early_return_ = Config::GetConfig()->do_early_return();
}

Marshal &operator<<(Marshal &m, const TxnWorkspace &ws) {
    //  m << (ws.keys_);
    if (Config::GetConfig()->cc_mode_ == MODE_TAPIR){
        uint64_t sz = ws.keys_.size();
        m << sz;
        for (int32_t k : ws.keys_) {
            auto it = (*ws.values_).find(k);
            verify(it != (*ws.values_).end());
            m << k << it->second;
        }
        return m;
    }

    //else; handle dep at server side
    uint64_t sz = ws.keys_.size();
    m << sz;
    m << ws.piece_input_ready_;
    m << ws.input_var_ready_;
    for (int32_t k : ws.keys_) {
        if (ws.piece_input_ready_ == 0) {
            if (ws.input_var_ready_.at(k) == 1) {
                auto it = (*ws.values_).find(k);
                verify(it != (*ws.values_).end());
                m << k << it->second;
            } else {
                //xs: put an EMPTY_VALUE here
                int32_t empty_magic_value = EMPTY_VALUE;
                Value v_tmp(empty_magic_value);
                m << k << v_tmp;
            }
        } else {
            //input ready, as normal
            auto it = (*ws.values_).find(k);
            verify(it != (*ws.values_).end());
            m << k << it->second;
        }
    }
    return m;
}

Marshal &operator>>(Marshal &m, TxnWorkspace &ws) {

    if (Config::GetConfig()->cc_mode_ == MODE_TAPIR){
        uint64_t sz;
        m >> sz;
        for (uint64_t i = 0; i < sz; i++) {
            int32_t k;
            Value v;
            m >> k >> v;
            ws.keys_.insert(k);
            (*ws.values_)[k] = v;
        }
        return m;
    }


    //else, handle dep at server side
    uint64_t sz;
    m >> sz;
    m >> ws.piece_input_ready_;
    m >> ws.input_var_ready_;

    Value v_empty(int32_t(EMPTY_VALUE));
    for (uint64_t i = 0; i < sz; i++) {
        int32_t k;
        Value v;


        m >> k >> v;
        ws.keys_.insert(k);
        (*ws.values_)[k] = v;

        if (ws.piece_input_ready_ == 0) {
            //Input not ready
            //For debug only
            if (ws.input_var_ready_[k] == 0) {
                Log_debug("key = %d, ws_input_var_ready size = %d, not ready", k, ws.input_var_ready_.size());
                verify((*ws.values_)[k] == v_empty);
            } else {
                Log_debug("key = %d, ws_input_var_ready size = %d, ready", k, ws.input_var_ready_.size());
            }
        }
    }


    return m;
}

Marshal &operator<<(Marshal &m, const TxnReply &reply) {
    m << reply.res_;
    m << reply.output_;
    m << reply.n_try_;
    // TODO -- currently this is only used when marshalling
    // replies from forwarded requests so the source
    // (non-leader) site correctly populates this field when
    // reporting.
    // m << reply.start_time_;
    m << reply.time_;
    m << reply.txn_type_;
    return m;
}

Marshal &operator>>(Marshal &m, TxnReply &reply) {
    m >> reply.res_;
    m >> reply.output_;
    m >> reply.n_try_;
    memset(&reply.start_time_, sizeof(reply.start_time_), 0);
    m >> reply.time_;
    m >> reply.txn_type_;
    return m;
}

set<parid_t> TxnChopper::GetPartitionIds() {
    return partition_ids_;
}

bool TxnCommand::IsOneRound() {
    return false;
}

vector<SimpleCommand> TxnCommand::GetCmdsByPartition(parid_t par_id) {
    vector<SimpleCommand> cmds;
    for (auto &pair : cmds_) {
        SimpleCommand *cmd = dynamic_cast<SimpleCommand *>(pair.second);
        verify(cmd);
        if (cmd->partition_id_ == par_id) {
            cmds.push_back(*cmd);
        }
    }
    return cmds;
}

map<parid_t, vector<SimpleCommand *>> TxnCommand::GetAllCmds(int32_t max) {
    verify(n_pieces_dispatched_ < n_pieces_dispatchable_);
    verify(n_pieces_dispatched_ < n_pieces_all_);
    map<parid_t, vector<SimpleCommand *>> cmds;

    std::map<varid_t, std::vector<parid_t>> waiting_var_par_map;
    //  int n_debug = 0;
    int n_not_used_pi = 0;
    auto sz = status_.size();//xs: status_ is the status of each piece of the transaction
    for (auto &kv : status_) {
        Log_debug("piece %d status %d",kv.first, kv.second);
        auto pi = kv.first;
        auto &status = kv.second;
        //xs: status dispatchable > all input ready
        //xs: status waiting > some inputs are not ready
        //    if (status == DISPATCHABLE) {
        //      status = INIT;
        if (status == OUTPUT_READY) {
            //Not needed;
            Log_debug("piece %d's status is output ready", pi);
            n_not_used_pi++;
            continue;
        }
        SimpleCommand *cmd = new SimpleCommand();
        cmd->inn_id_ = pi;//pi is inn_id, actually inner id
        cmd->partition_id_ = GetPiecePartitionId(pi);
        cmd->type_ = pi;
        cmd->root_id_ = id_;
        cmd->root_type_ = type_;
        cmd->input = inputs_[pi];
        //Check input ready;
        if (status == DISPATCHABLE) {
            cmd->input.piece_input_ready_ = 1;
        } else {
            if (status != WAITING) {
                Log_info("tid = %lu, piece =%u, status is not waiting, it's %d", this->id_, pi, status);
                verify(status == WAITING);
            }
            cmd->input.piece_input_ready_ = 0;
            for (auto &k : cmd->input.keys_) {
                if (cmd->input.values_->count(k) != 0) {
                    cmd->input.input_var_ready_[k] = 1;
                } else {
                    cmd->input.input_var_ready_[k] = 0;
                    waiting_var_par_map[k].push_back(cmd->partition_id_);
                }
            }

            //For debug, pure print
            //        DEP_LOG("waiting piece %d, input values size = %d, input keys size = %d, output_size = %d, output_.size = %d",
            //                 pi,
            //                 inputs_[pi].values_->size(),
            //                 inputs_[pi].keys_.size(),
            //                 output_size_[pi], outputs_.size());
            //        for (auto& pair:  *(inputs_[pi].values_)){
            //          DEP_LOG("waiting piece %d, var id = %d, value =%s",
            //                   pi,
            //                   pair.first,
            //                   to_string(pair.second).c_str());
            //        }
            //        for (auto &k: inputs_[pi].keys_){
            //          DEP_LOG("waiting piece %d, key = %d",
            //                   pi,
            //                   k);
            //        }
            //        //Debug ends here
        }


        cmd->output_size = output_size_[pi];
        cmd->root_ = this;
        cmd->timestamp_ = timestamp_;
        cmds_[pi] = cmd;
        Log_debug("pushed piece %d", pi);
        cmds[cmd->partition_id_].push_back(cmd);
        partition_ids_.insert(cmd->partition_id_);

        Log_debug("getting subcmd i: %d, thread id: %x",
                  pi, std::this_thread::get_id());
        //verify(status_[pi] == INIT);
        status_[pi] = DISPATCHED;

        verify(type_ == type());
        verify(cmd->root_type_ == type());
        verify(cmd->root_type_ > 0);
        n_pieces_dispatched_++;


        //xs: Dont use it
        //max--;
        //if (max == 0) break;
    }
    if (cmds_.size() + n_not_used_pi != status_.size()) {
        Log_info("cmds_.szie = %d vs status.size = %d, not used = %d", cmds_.size(), status_.size(), n_not_used_pi);
        verify(0);
    }

    for (auto &pair : cmds) {
        for (auto &c : pair.second) {
            c->waiting_var_par_map = waiting_var_par_map;
        }
    }

    for (auto &pair : waiting_var_par_map) {
        for (auto &p : pair.second) {
            Log_debug("txn %lu, type %u, var %d is required by par %u", this->txn_id_, this->type_, pair.first, p);
        }
    }
    verify(cmds.size() > 0);
    return cmds;
}


map<parid_t, vector<SimpleCommand *>> TxnCommand::GetReadyCmds(int32_t max) {

    Log_info("n_pieces_dispatched = %d, n_pieces_dispatchable = %d, n_pieces_all = %d", n_pieces_dispatched_,  n_pieces_dispatchable_, n_pieces_all_);

    verify(n_pieces_dispatched_ < n_pieces_dispatchable_);
    verify(n_pieces_dispatched_ < n_pieces_all_);
    map<parid_t, vector<SimpleCommand *>> cmds;

    //  int n_debug = 0;
    auto sz = status_.size();//xs: status_ is the status of each piece of the transaction
    for (auto &kv : status_) {
        auto pi = kv.first;
        auto &status = kv.second;
        if (status == DISPATCHABLE) {
            status = INIT;
            SimpleCommand *cmd = new SimpleCommand();
            cmd->inn_id_ = pi;//pi is inn_id, actually inner id
            cmd->partition_id_ = GetPiecePartitionId(pi);
            cmd->type_ = pi;
            cmd->root_id_ = id_;
            cmd->root_type_ = type_;
            cmd->input = inputs_[pi];
            cmd->output_size = output_size_[pi];
            cmd->root_ = this;
            cmd->timestamp_ = timestamp_;

            cmds_[pi] = cmd;
            cmds[cmd->partition_id_].push_back(cmd);
            partition_ids_.insert(cmd->partition_id_);

            Log_debug("getting subcmd i: %d, thread id: %x",
                      pi, std::this_thread::get_id());
            verify(status_[pi] == INIT);
            status_[pi] = DISPATCHED;

            verify(type_ == type());
            verify(cmd->root_type_ == type());
            verify(cmd->root_type_ > 0);
            n_pieces_dispatched_++;

            max--;
            if (max == 0) break;
        } else if (status == WAITING) {
            //xs; for understanding the code, remember to remove it later
            //      Log_info("waiting piece %d, input values size = %d, input keys size = %d, output size = %d",
            //          pi,
            //          inputs_[pi].values_->size(),
            //          inputs_[pi].keys_.size(),
            //          output_size_[pi]);
            //      for (auto& pair:  *(inputs_[pi].values_)){
            //        Log_info("waiting piece %d, var id = %d, value =%s",
            //            pi,
            //            pair.first,
            //            to_string(pair.second).c_str());
            //      }
            //      for (auto &k: inputs_[pi].keys_){
            //        Log_info("waiting piece %d, key = %d",
            //            pi,
            //            k);
            //      }
        }
    }

    verify(cmds.size() > 0);
    return cmds;
}

ContainerCommand *TxnCommand::GetNextReadySubCmd() {
    verify(0);
    verify(n_pieces_dispatched_ < n_pieces_dispatchable_);
    verify(n_pieces_dispatched_ < n_pieces_all_);
    SimpleCommand *cmd = nullptr;

    auto sz = status_.size();
    for (auto &kv : status_) {
        auto pi = kv.first;
        auto &status = kv.second;
        if (status == DISPATCHABLE) {
            status = INIT;
            cmd = new SimpleCommand();
            cmd->inn_id_ = pi;
            cmd->partition_id_ = GetPiecePartitionId(pi);
            cmd->type_ = pi;
            cmd->root_id_ = id_;
            cmd->root_type_ = type_;
            cmd->input = inputs_[pi];
            cmd->output_size = output_size_[pi];
            cmd->root_ = this;
            cmd->timestamp_ = timestamp_;
            cmds_[pi] = cmd;
            partition_ids_.insert(cmd->partition_id_);

            Log_debug("getting subcmd i: %d, thread id: %x",
                      pi, std::this_thread::get_id());
            verify(status_[pi] == INIT);
            status_[pi] = DISPATCHED;

            verify(type_ == type());
            verify(cmd->root_type_ == type());
            verify(cmd->root_type_ > 0);
            n_pieces_dispatched_++;
            return cmd;
        }
    }

    verify(cmd);
    return cmd;
}

bool TxnCommand::OutputReady() {
    if (n_pieces_all_ == n_pieces_dispatch_acked_) {
        return true;
    } else {
        return false;
    }
}

void TxnCommand::Merge(TxnOutput &output) {
    //  Log_info("%s called", __FUNCTION__ );
    for (auto &pair : output) {
        Merge(pair.first, pair.second);
    }
}

void TxnCommand::Merge(innid_t inn_id, map<int32_t, Value> &output) {
    //  Log_info("%s called", __FUNCTION__ );
    verify(outputs_.find(inn_id) == outputs_.end());
    n_pieces_dispatch_acked_++;
    verify(n_pieces_all_ >= n_pieces_dispatchable_);
    verify(n_pieces_dispatchable_ >= n_pieces_dispatched_);
    verify(n_pieces_dispatched_ >= n_pieces_dispatch_acked_);

    //  Log_info("Log before merging, len output %d", outputs_.size());
    outputs_[inn_id] = output;
    //  Log_info("Log after merging, len output %d", outputs_.size());

    //  for (auto& pair: cmds_){
    //    innid_t piece_type = pair.first;
    //    Log_info("Before merge, total piece %d, piece %u, status %d, input values_ size %d, keys size = %d",
    //             cmds_.size(),
    //             piece_type,
    //             status_[piece_type],
    //             pair.second->input.values_->size(),
    //             pair.second->input.keys_.size());
    //    for (auto& pp: *(pair.second->input.values_)){
    //      Log_info("piece = %d, var id = %d, value = %s", piece_type, pp.first, to_string(pp.second).c_str());
    //    }
    //    for (auto& k: pair.second->input.keys_){
    //      Log_info("piece = %d, key %d", piece_type,  k);
    //    }
    //  }

    cmds_[inn_id]->output = output;
    this->start_callback(inn_id, SUCCESS, output);

    ////  for (auto& pair: cmds_){
    ////    innid_t piece_type = pair.first;
    ////    Log_info("After merge, total piece %d, piece %u, status %d, input values_ size %d, keys size = %d",
    ////             cmds_.size(),
    ////             piece_type,
    ////             status_[piece_type],
    ////             pair.second->input.values_->size(),
    ////             pair.second->input.keys_.size());
    ////    for (auto& pp: *(pair.second->input.values_)){
    ////      Log_info("piece = %d, var id = %d, value = %s", piece_type, pp.first, to_string(pp.second).c_str());
    ////    }
    ////    for (auto& k: pair.second->input.keys_) {
    ////      Log_info("piece = %d, key %d", piece_type, k);
    ////    }
    ////  }
    //  Log_info("%s returned", __FUNCTION__ );
}

void TxnCommand::Merge(ContainerCommand &cmd) {
    auto simple_cmd = (SimpleCommand *) &cmd;
    auto pi = cmd.inn_id();
    auto &output = simple_cmd->output;
    Merge(pi, output);
}

bool TxnCommand::HasMoreSubCmdReadyNotOut() {
    verify(n_pieces_all_ >= n_pieces_dispatchable_);
    verify(n_pieces_dispatchable_ >= n_pieces_dispatched_);
    verify(n_pieces_dispatched_ >= n_pieces_dispatch_acked_);
    if (n_pieces_dispatchable_ == n_pieces_dispatched_) {
        verify(n_pieces_all_ == n_pieces_dispatched_ ||
               n_pieces_dispatch_acked_ < n_pieces_dispatched_);
        return false;
    } else {
        verify(n_pieces_dispatchable_ > n_pieces_dispatched_);
        return true;
    }
}

void TxnCommand::Reset() {
    n_pieces_dispatchable_ = 0;
    n_pieces_dispatch_acked_ = 0;
    n_pieces_dispatched_ = 0;
    outputs_.clear();
}

void TxnCommand::read_only_reset() {
    read_only_failed_ = false;
    Reset();
}

bool TxnCommand::read_only_start_callback(int pi,
                                          int *res,
                                          map<int32_t, mdb::Value> &output) {
    verify(pi < GetNPieceAll());
    if (res == NULL) {// phase one, store outputs only
        outputs_[pi] = output;
    } else {
        // phase two, check if this try not failed yet
        // and outputs is consistent with previous stored one
        // store current outputs
        if (read_only_failed_) {
            *res = REJECT;
        } else if (pi >= outputs_.size()) {
            *res = REJECT;
            read_only_failed_ = true;
        } else if (is_consistent(outputs_[pi], output))
            *res = SUCCESS;
        else {
            *res = REJECT;
            read_only_failed_ = true;
        }
        outputs_[pi] = output;
    }
    return start_callback(pi, SUCCESS, output);
}

double TxnCommand::last_attempt_latency() {
    double tmp = pre_time_;
    struct timespec t_buf;
    clock_gettime(&t_buf);
    pre_time_ = timespec2ms(t_buf);
    return pre_time_ - tmp;
}

TxnReply &TxnCommand::get_reply() {
    reply_.start_time_ = start_time_;
    reply_.n_try_ = n_try_;
    struct timespec t_buf;
    clock_gettime(&t_buf);
    reply_.time_ = timespec2ms(t_buf) - timespec2ms(start_time_);
    reply_.txn_type_ = (int32_t) type_;
    return reply_;
}

void TxnRequest::get_log(i64 tid, std::string &log) {
    uint32_t len = 0;
    len += sizeof(tid);
    len += sizeof(txn_type_);
    uint32_t input_size = input_.size();
    len += sizeof(input_size);
    log.resize(len);
    memcpy((void *) log.data(), (void *) &tid, sizeof(tid));
    memcpy((void *) log.data(), (void *) &txn_type_, sizeof(txn_type_));
    memcpy((void *) (log.data() + sizeof(txn_type_)), (void *) &input_size, sizeof(input_size));
    for (int i = 0; i < input_size; i++) {
        log.append(mdb::to_string(input_[i]));
    }
}

Marshal &TxnCommand::ToMarshal(Marshal &m) const {
    m << ws_;
    m << ws_init_;
    m << inputs_;
    m << output_size_;
    m << p_types_;
    m << sharding_;
    m << status_;
    // FIXME
    //  m << cmd_;
    m << partition_ids_;
    m << n_pieces_all_;
    m << n_pieces_dispatchable_;
    m << n_pieces_dispatch_acked_;
    m << n_pieces_dispatched_;
    m << n_finished_;
    m << max_try_;
    m << n_try_;
    return m;
}

Marshal &TxnCommand::FromMarshal(Marshal &m) {
    m >> ws_;
    m >> ws_init_;
    m >> inputs_;
    m >> output_size_;
    m >> p_types_;
    m >> sharding_;
    m >> status_;
    // FIXME
    //  m >> cmd_;
    m >> partition_ids_;
    m >> n_pieces_all_;
    m >> n_pieces_dispatchable_;
    m >> n_pieces_dispatch_acked_;
    m >> n_pieces_dispatched_;
    m >> n_finished_;
    m >> max_try_;
    m >> n_try_;
    return m;
}


}// namespace rococo

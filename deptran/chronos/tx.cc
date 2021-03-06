//
// Created by micha on 2020/3/23.
//


#include "tx.h"
#include "bench/tpcc/piece.h"
#include "memdb/row_mv.h"
namespace rococo {

//
//static char * defer_str[] = {"defer_real", "defer_no", "defer_fake"};
//static char * hint_str[] = {"n/a", "bypass", "instant", "n/a",  "deferred"};
////SimpleCommand is a typedef of TxnPieceData
//add a simpleCommand to the local Tx's dreq
void TxChronos::DispatchExecute(const SimpleCommand &cmd,
                                int32_t *res,
                                map<int32_t, Value> *output) {

    //xs: for debug;
    this->root_type = cmd.root_type_;

    //xs: Step 1: skip this simpleCommand if it is already in the dreqs.
    //  for (auto& c: dreqs_) {
    //    if (c.inn_id() == cmd.inn_id()){ // already handled?{
    //    Log_debug("here");
    //    return;
    //    }
    //  }


    verify(txn_reg_);
    // execute the IR actions.
    auto pair = txn_reg_->get(cmd);
    // To tolerate deprecated codes

    //Log_debug("%s called, defer= %s, txn_id = %hu, root_type = %d, type = %d" , __FUNCTION__, defer_str[pair.defer], cmd.root_id_, cmd.root_type_, cmd.type_);
    int xxx, *yyy;
    //  if (pair.defer == DF_REAL) {
    yyy = &xxx;
    //    dreqs_.push_back(cmd);
    //  } else if (pair.defer == DF_NO) {
    //    yyy = &xxx;
    //  } else
    if (pair.defer == DF_FAKE) {
        //    Log_debug("herehere");
        //    dreqs_.push_back(cmd);
        return;
    }
    //    verify(0);
    //  } else {
    //    verify(0);
    //  }
    pair.txn_handler(nullptr,
                     this,
                     const_cast<SimpleCommand &>(cmd),
                     yyy,
                     *output);
    *res = pair.defer;

    //Log_debug("%s returned, defer= %s, txn_id = %hu, root_type = %d, type = %d" , __FUNCTION__, defer_str[pair.defer], cmd.root_id_, cmd.root_type_, cmd.type_);
}
std::map<parid_t, std::map<varid_t, Value>>
TxChronos::RecursiveExecuteReadyPieces(parid_t my_par_id, std::set<innid_t> &pending_pieces) {
    if (this->exe_tried_ == false) {
        this->exe_tried_ = true;

#ifdef LAT_BREAKDOWN
        handler_local_start_exe_ts =
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#endif//LAT_BREAKDOWN
    }

    recur_times_++;

    std::map<parid_t, std::map<varid_t, Value>> ret;
    int32_t exe_res;
    bool need_check;
    do {
        need_check = false;
        for (auto &c : pieces_) {
            bool no_conflict = can_exe_wo_conflict(c.inn_id(), pending_pieces);
            if (c.input.piece_input_ready_ && c.local_executed_ == false && no_conflict) {
                Log_debug("executing piece %d of txn %lu", c.inn_id(), id());
                //        pending_piecesc.inn_id()].erase(id());
                //        Log_debug("pending pieces removed piece %u of txn %lu, count = %d", c.inn_id(), id(), pending_pieces[c.inn_id()].size());

                map<int32_t, Value> piece_output;
                DispatchExecute(const_cast<SimpleCommand &>(c),
                                &exe_res, &piece_output);
                verify(c.local_executed_ == false);
                c.local_executed_ = true;
                n_executed_pieces++;
                Log_debug("output size = %d", piece_output.size());
                auto targets = GetOutputsTargets(&piece_output);
                for (auto &tar : targets) {
                    Log_debug("traget par %u", tar);
                    ret[tar].insert(piece_output.begin(), piece_output.end());
                }
                if (targets.count(my_par_id) != 0) {
                    //The output is also required by my partition
                    bool has_more_ready = MergeCheckReadyPieces(piece_output);
                    DEP_LOG("has more ready = %d", has_more_ready);
                    need_check = need_check || has_more_ready;
                }
                (*dist_output_)[c.inn_id()] = piece_output;
            } else {
                Log_debug("Not executing piece %d of txn %lu, for now, input_ready = %d, executed = %d",
                          c.inn_id(),
                          id(),
                          c.input.piece_input_ready_,
                          c.local_executed_);
                //        if (c.local_executed_ == false){
                //           for (auto & pair: c.input.input_var_ready_){
                //             if (pair.second == 0){
                //               Log_debug("Key %d not ready", pair.first);
                //             }
                //           }
                //        }
            }
        }
    } while (need_check == true);

    for (auto &c : pieces_) {
        if (c.local_executed_ == false) {
            pending_pieces.insert(c.inn_id());
            Log_debug("pending pieces add piece %u of txn %lu, count = %d", c.inn_id(), id(), pending_pieces.size());
        }
    }

    return ret;
}


std::set<parid_t> TxChronos::GetOutputsTargets(std::map<int32_t, Value> *output) {
    std::set<parid_t> ret;
    for (auto &pair : *output) {
        varid_t varid = pair.first;
        auto &var_par_map = this->pieces_[0].waiting_var_par_map;
        if (var_par_map.count(varid) != 0) {
            for (auto &target : var_par_map[varid]) {
                ret.insert(target);
                Log_debug("var %d is required by partition %u", varid, target);
            }
        }
    }
    return ret;
}


//bool TxChronos::MergeCheckReadyPieces(const TxnOutput& output){
//  bool ret = false;
//  for (auto& pair: output){
//    ret = ret || MergeCheckReadyPieces(pair.second);
//  }
//  return ret;
//}


bool TxChronos::MergeCheckReadyPieces(const std::map<int32_t, Value> &output) {
    bool ret = false;
    for (auto &pair : output) {
        varid_t varid = pair.first;
        Value value = pair.second;
        for (auto &p : this->pieces_) {
            if (p.input.piece_input_ready_ == 0) {
                //This piece not ready
                if (p.input.keys_.count(varid) != 0) {
                    //This piece need this var
                    if (p.input.input_var_ready_[varid] == 0) {
                        //This var is not ready yet
                        (*p.input.values_)[varid] = value;
                        p.input.input_var_ready_[varid] = 1;
                        bool ready = true;
                        for (auto &re : p.input.input_var_ready_) {
                            if (re.second == 0) {
                                Log_debug("%s, var %d is not ready yet for piece %u",
                                         __FUNCTION__,
                                         varid,
                                         p.inn_id());
                                ready = false;
                            }
                        }
                        if (ready) {
                            Log_debug("%s, piece %u is ready for execution",
                                     __FUNCTION__,
                                     p.inn_id());
                            p.input.piece_input_ready_ = 1;
                            ret = true;
                        }
                    } else {
                        //This var is already ready.
//                        Log_debug("%s, Var %d is arleady ready for piece %u",
//                                 __FUNCTION__,
//                                 varid,
//                                 p.inn_id());
                        //do nothing
                    }
                }
            }
        }
    }
    return ret;
}

void TxChronos::PreAcceptExecute(const SimpleCommand &cmd, int *res, map<int32_t, Value> *output) {


    //xs: Step 1: skip this simpleCommand if it is already in the dreqs.
    Log_debug("%s called", __FUNCTION__);
    bool already_dispatched = false;
    for (auto &c : dreqs_) {
        if (c.inn_id() == cmd.inn_id()) {
            // already received this piece, no need to push_back again.
            already_dispatched = true;
        }
    }
    verify(txn_reg_);
    // execute the IR actions.
    auto pair = txn_reg_->get(cmd);
    // To tolerate deprecated codes

    int xxx, *yyy;
    if (pair.defer == DF_REAL) {
        yyy = &xxx;
        if (!already_dispatched) {
            dreqs_.push_back(cmd);
        }
    } else if (pair.defer == DF_NO) {
        yyy = &xxx;
    } else if (pair.defer == DF_FAKE) {
        if (!already_dispatched) {
            dreqs_.push_back(cmd);
        }
        /*
     * xs: don't know the meaning of DF_FAKE
     * seems cannot run pieces with DF_FAKE now, other wise it will cause segfault when retrieving input values
     */
        return;
    } else {
        verify(0);
    }
    pair.txn_handler(nullptr,
                     this,
                     const_cast<SimpleCommand &>(cmd),
                     yyy,
                     *output);
    *res = pair.defer;
}

bool TxChronos::ReadColumn(mdb::Row *row,
                           mdb::column_id_t col_id,
                           Value *value,
                           int hint_flag) {
    //Seems no instant through out the process
    //Log_debug("Rtti = %d", row->rtti());
    auto r = dynamic_cast<ChronosRow *>(row);
    verify(r->rtti() == symbol_t::ROW_CHRONOS);

    verify(!read_only_);
    if (hint_flag == TXN_INSTANT || hint_flag == TXN_BYPASS) {
        //TODO: this works, but why
        //No seg fault
        auto c = r->get_column(col_id);
        row->ref_copy();
        *value = c;
    }
    return true;
}


bool TxChronos::WriteColumn(Row *row,
                            column_id_t col_id,
                            const Value &value,
                            int hint_flag) {
    if (hint_flag == TXN_INSTANT) {
        //TODO: this works, but why
        //No seg fault
        auto r = dynamic_cast<ChronosRow *>(row);
        verify(r->rtti() == symbol_t::ROW_CHRONOS);
        mdb_txn()->write_column(row, col_id, value);
    }
    return true;
}


void TxChronos::CommitExecute() {
    //  verify(phase_ == PHASE_RCC_START);
    Log_debug("%s called", __FUNCTION__);
    TxnWorkspace ws;
    for (auto &cmd : dreqs_) {
        auto pair = txn_reg_->get(cmd);
        int tmp;
        cmd.input.Aggregate(ws);
        auto &m = output_[cmd.inn_id_];
        pair.txn_handler(nullptr, this, cmd, &tmp, m);
        ws.insert(m);
    }
    Log_debug("%s returned", __FUNCTION__);
    committed_ = true;
}

void TxChronos::CheckSendOutputToClient() {


    Log_debug("%s called for txn id = %lu", __FUNCTION__, id());
    if (this->chr_phase_ == OUTPUT_SENT) {
        Log_debug("already sent");
        return;
    }
    bool all_output_received = true;
    for (auto &pair : dist_partition_status_) {
        if (pair.second < P_OUTPUT_RECEIVED) {
            Log_debug("output for par %u not received", pair.first);
            all_output_received = false;
            break;
        }
    }
    if (all_output_received) {
        Log_debug("going to send outputs to client txn id = %lu", this->id());
#ifdef LAT_BREAKDOWN
        this->handler_output_ready_ts = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        Log_debug("txn %lu, type = %u, lat %d ms breakdown in ms, remote prepare, local prepare, wait for exe local, exe (incl. wait for input),  wait for output,%ld,%ld,%ld,%ld,%ld. Recursive times = %d",
                 id(),
                 root_type,
                 handler_output_ready_ts - handler_submit_ts_,
                 handler_remote_prepared_ts - handler_submit_ts_,
                 handler_local_prepared_ts - handler_remote_prepared_ts,
                 handler_local_start_exe_ts - handler_local_prepared_ts,
                 handler_local_exe_finish_ts - handler_local_start_exe_ts,
                 handler_output_ready_ts - handler_local_exe_finish_ts,
                 recur_times_);
#endif//LAT_BREAKDOWN
        send_output_client();
        this->chr_phase_ = OUTPUT_SENT;
    }
}
bool TxChronos::can_exe_wo_conflict(uint32_t to_check, const std::set<innid_t> &pending_pieces) {
    for (auto &p : pending_pieces) {
        if (check_piece_conflict(p, to_check)) {
            Log_debug("Piece %u of txn %lu cannot run, because it conflicts with %u", to_check, id(), p);
            return false;
        }
    }
    Log_debug("Piece %u of txn %lu can run, not conflicts", to_check, id());
    return true;
}
bool TxChronos::check_piece_conflict(uint32_t pending_pie_id, uint32_t type_to_check) {
    if (Config::GetConfig()->benchmark_ == TPCC) {
        std::set<std::pair<innid_t, innid_t>> &conf = TpccPiece::conflicts_;
        if (conf.count(std::pair<innid_t, innid_t>(pending_pie_id, type_to_check)) != 0) {
            //      Log_debug("%u and %u conflict", pending_pie_id, type_to_check);
            return true;
        } else {
            //      Log_debug("%u and %u do not conflict", pending_pie_id, type_to_check);
            return false;
        }
    } else {
        return false;
    }
}

}// namespace rococo

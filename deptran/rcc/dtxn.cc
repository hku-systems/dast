#include "dtxn.h"
#include "../__dep__.h"
#include "../scheduler.h"
#include "bench/tpcc/piece.h"
#include "dep_graph.h"
#include "rcc_rpc.h"
#include "txn_chopper.h"
#include <deptran/txn_reg.h>

namespace rococo {

RccDTxn::RccDTxn(epoch_t epoch,
                 txnid_t tid,
                 Scheduler *mgr,
                 bool ro) : DTxn(epoch, tid, mgr) {
    read_only_ = ro;
    mdb_txn_ = mgr->GetOrCreateMTxn(tid_);
    verify(id() == tid);
}

RccDTxn::RccDTxn(txnid_t id) : DTxn(0, id, nullptr) {
    // alert!! after this a lot of stuff need to be set manually.
    tid_ = id;
}

RccDTxn::RccDTxn(RccDTxn &rhs_dtxn) : Vertex<RccDTxn>(rhs_dtxn),
                                      DTxn(rhs_dtxn.epoch_, rhs_dtxn.tid_, nullptr),
                                      partition_(rhs_dtxn.partition_),
                                      status_(rhs_dtxn.status_) {
}


std::map<parid_t, std::map<varid_t, Value>> RccDTxn::CommitExecAllRdy(parid_t my_parid) {
    Log_debug("%s called", __FUNCTION__);
    phase_ = PHASE_RCC_COMMIT;
    for (auto &c : this->dreqs_) {
        c.local_executed_ = false;
    }
    this->n_executed_pieces = 0;

    auto output_to_send = RecursiveExecuteReadyPieces(my_parid);


    committed_ = true;
    return output_to_send;
}

void RccDTxn::ExecSimpleCmd(const SimpleCommand &cmd, int *res, map<int32_t, Value> *output) {

    verify(txn_reg_);
    // execute the IR actions.
    auto pair = txn_reg_->get(cmd);

    int xxx, *yyy;
    //  if (pair.defer == DF_REAL) {
    yyy = &xxx;
    //    dreqs_.push_back(cmd);
    //  } else if (pair.defer == DF_NO) {
    //    yyy = &xxx;
    //  } else
    if (pair.defer == DF_FAKE) {
        //        Log_debug("herehere");
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
}

std::map<parid_t, std::map<varid_t, Value>>
RccDTxn::RecursiveExecuteReadyPieces(parid_t my_par_id) {
    std::map<parid_t, std::map<varid_t, Value>> ret;
    int32_t exe_res;
    bool need_check;
    do {
        need_check = false;
        for (auto &c : dreqs_) {
            if (c.input.piece_input_ready_ && c.local_executed_ == false) {
                Log_debug("executing piece %d of txn %lu", c.inn_id(), id());
                //        pending_piecesc.inn_id()].erase(id());
                //        Log_debug("pending pieces removed piece %u of txn %lu, count = %d", c.inn_id(), id(), pending_pieces[c.inn_id()].size());

                map<int32_t, Value> piece_output;
                ExecSimpleCmd(const_cast<SimpleCommand &>(c),
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
                output_[c.inn_id()] = piece_output;
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

    //    for (auto &c: dreqs_) {
    //        if (c.local_executed_ == false) {
    //            pending_pieces.insert(c.inn_id());
    //            Log_debug("pending pieces add piece %u of txn %lu, count = %d", c.inn_id(), id(), pending_pieces.size());
    //        }
    //    }
    return ret;
}


bool RccDTxn::MergeCheckReadyPieces(const std::map<int32_t, Value> &output) {
    bool ret = false;
    for (auto &pair : output) {
        varid_t varid = pair.first;
        Value value = pair.second;
        for (auto &p : this->dreqs_) {
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
//                        Log_warn("%s, Var %d is arleady ready for piece %u",
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


void RccDTxn::DispatchExecute(const SimpleCommand &cmd,
                              int32_t *res,
                              map<int32_t, Value> *output) {
    for (auto &c : dreqs_) {
        if (c.inn_id() == cmd.inn_id())
            return;
    }
    verify(phase_ <= PHASE_RCC_DISPATCH);
    phase_ = PHASE_RCC_DISPATCH;

    // execute the IR actions.
    verify(txn_reg_);
    auto pair = txn_reg_->get(cmd);
    // To tolerate deprecated codes
    int xxx, *yyy;


    //Those whose input are not ready must be df_real or df_fake

    if (pair.defer == DF_REAL) {
        yyy = &xxx;
        dreqs_.push_back(cmd);
    } else if (pair.defer == DF_NO) {
        yyy = &xxx;
    } else if (pair.defer == DF_FAKE) {
        dreqs_.push_back(cmd);
        return; //xs: does not need to execute here
                //    verify(0);
    } else {
        verify(0);
    }


    Log_debug("%s going to execute piece %u, defer = %d, input ready = %d", __FUNCTION__, cmd.type_, pair.defer, cmd.input.piece_input_ready_);
    pair.txn_handler(nullptr,
                     this,
                     const_cast<SimpleCommand &>(cmd),
                     yyy,
                     *output);
    *res = pair.defer;
}


void RccDTxn::Abort() {
    aborted_ = true;
    // TODO. nullify changes in the staging area.
}


bool RccDTxn::DispatchExec(parid_t my_parid) {
    //  verify(phase_ == PHASE_RCC_START);
    Log_debug("%s called", __FUNCTION__);
    phase_ = PHASE_RCC_DISPATCH;

    RecursiveExecuteReadyPieces(my_parid);

    bool all_pie_checked = true;
    for (auto &cmd : dreqs_) {
        if (!cmd.local_executed_) {
            Log_debug("checking dep for input not ready piece %u", cmd.type_);
            int tmp;
            map<int32_t, Value> tmp_output;
            auto pair = txn_reg_->get(cmd);
            pair.txn_handler(nullptr, this, cmd, &tmp, tmp_output);

            //FIXME: currently only works for TPCC (and workloads w/o value dep)

            verify(cmd.type_ == TPCC_PAYMENT_5);
            all_pie_checked = false;
        }
    }
    return all_pie_checked;
    //    for (auto &cmd : dreqs_) {
    //        auto pair = txn_reg_->get(cmd);
    //        int tmp;
    //        cmd.input.Aggregate(ws);
    //        auto &m = output_[cmd.inn_id_];
    //        pair.txn_handler(nullptr, this, cmd, &tmp, m);
    //        ws.insert(m);
    //    }
}


void RccDTxn::CommitExecute() {
    //  verify(phase_ == PHASE_RCC_START);
    Log_debug("%s called", __FUNCTION__);
    phase_ = PHASE_RCC_COMMIT;


    TxnWorkspace ws;


    for (auto &cmd : dreqs_) {
        auto pair = txn_reg_->get(cmd);
        int tmp;
        cmd.input.Aggregate(ws);
        auto &m = output_[cmd.inn_id_];
        pair.txn_handler(nullptr, this, cmd, &tmp, m);
        ws.insert(m);
    }
    committed_ = true;
}

void RccDTxn::ReplyFinishOk() {
    //  verify(committed != aborted);
    int r = committed_ ? SUCCESS : REJECT;
    if (commit_request_received_) {
        if (__debug_replied)
            return;
        verify(!__debug_replied);// FIXME
        __debug_replied = true;
        verify(ptr_output_repy_);
        finish_reply_callback_(r);
    }
}


std::set<parid_t> RccDTxn::GetOutputsTargets(std::map<int32_t, Value> *output) {
    std::set<parid_t> ret;
    for (auto &pair : *output) {
        varid_t varid = pair.first;
        Log_debug("%s checking var %d", __FUNCTION__, varid);
        auto &var_par_map = this->dreqs_[0].waiting_var_par_map;
        if (var_par_map.count(varid) != 0) {
            for (auto &target : var_par_map[varid]) {
                ret.insert(target);
                Log_debug("var %d is required by partition %u", varid, target);
            }
        }
    }
    return ret;
}


bool RccDTxn::start_exe_itfr(defer_t defer_type,
                             TxnHandler &handler,
                             const SimpleCommand &cmd,
                             map<int32_t, Value> *output) {
    verify(0);
    //  bool deferred;
    //  switch (defer_type) {
    //    case DF_NO: { // immediate
    //      int res;
    //      // TODO fix
    //      handler(nullptr,
    //              this,
    //              const_cast<SimpleCommand&>(cmd),
    //              &res,
    //              *output);
    //      deferred = false;
    //      break;
    //    }
    //    case DF_REAL: { // defer
    //      dreqs_.push_back(cmd);
    //      map<int32_t, Value> no_use;
    //      handler(nullptr,
    //              this,
    //              const_cast<SimpleCommand&>(cmd),
    //              NULL,
    //              no_use);
    //      deferred = true;
    //      break;
    //    }
    //    case DF_FAKE: //TODO
    //    {
    //      dreqs_.push_back(cmd);
    //      int output_size = 300; //XXX
    //      int res;
    //      handler(nullptr,
    //              this,
    //              const_cast<SimpleCommand&>(cmd),
    //              &res,
    //              *output);
    //      deferred = false;
    //      break;
    //    }
    //    default:
    //      verify(0);
    //  }
    //  return deferred;
}

//void RccDTxn::start(
//    const RequestHeader &header,
//    const std::vector<mdb::Value> &input,
//    bool *deferred,
//    ChopStartResponse *res
//) {
//  // TODO Remove
//  verify(0);
//}

void RccDTxn::start_ro(const SimpleCommand &cmd,
                       map<int32_t, Value> &output,
                       DeferredReply *defer) {
    verify(0);
}

//void RccDTxn::commit(const ChopFinishRequest &req,
//                     ChopFinishResponse *res,
//                     rrr::DeferredReply *defer) {
//  verify(0);
//}

//void RccDTxn::commit_anc_finish(
//    Vertex<TxnInfo> *v,
//    rrr::DeferredReply *defer
//) {
//  std::function<void(void)> scc_anc_commit_cb = [v, defer, this]() {
//    this->commit_scc_anc_commit(v, defer);
//  };
//  Log::debug("all ancestors have finished for txn id: %llx", v->data_->id());
//  // after all ancestors become COMMITTING
//  // wait for all ancestors of scc become DECIDED
//  std::set<Vertex<TxnInfo> *> scc_anc;
//  RccDTxn::dep_s->find_txn_scc_nearest_anc(v, scc_anc);
//
//  DragonBall *wait_commit_ball = new DragonBall(scc_anc.size() + 1,
//                                                scc_anc_commit_cb);
//  for (auto &sav: scc_anc) {
//    //Log::debug("\t ancestor id: %llx", sav->data_.id());
//    // what if the ancestor is not related and not committed or not finished?
//    sav->data_->register_event(TXN_DCD, wait_commit_ball);
//    send_ask_req(sav);
//  }
//  wait_commit_ball->trigger();
//}

//void RccDTxn::commit_scc_anc_commit(
//    Vertex<TxnInfo> *v,
//    rrr::DeferredReply *defer
//) {
//  Graph<TxnInfo> &txn_gra = RccDTxn::dep_s->txn_gra_;
//  uint64_t txn_id = v->data_->id();
//  Log::debug("all scc ancestors have committed for txn id: %llx", txn_id);
//  // after all scc ancestors become DECIDED
//  // sort, and commit.
//  TxnInfo &tinfo = *v->data_;
//  if (tinfo.is_commit()) { ;
//  } else {
//    std::vector<Vertex<TxnInfo> *> sscc;
//    txn_gra.FindSortedSCC(v, &sscc);
//    //static int64_t sample = 0;
//    //if (RandomGenerator::rand(1, 100)==1) {
//    //    scsi_->do_statistics(S_RES_KEY_N_SCC, sscc.size());
//    //}
//    //this->stat_sz_scc_.sample(sscc.size());
//    for (auto &vv: sscc) {
//      bool commit_by_other = vv->data_->get_status() & TXN_DCD;
//      if (!commit_by_other) {
//        // apply changes.
//
//        // this res may not be mine !!!!
//        if (vv->data_->res != nullptr) {
//          auto txn = (RccDTxn *) sched_->GetDTxn(vv->data_->id());
//          txn->exe_deferred(vv->data_->res->outputs);
//          sched_->DestroyDTxn(vv->data_->id());
//        }
//
//        Log::debug("txn commit. tid:%llx", vv->data_->id());
//        // delay return back to clients.
//        //
//        verify(vv->data_->committed_ == false);
//        vv->data_->committed_ = true;
//        vv->data_->union_status(TXN_DCD, false);
//      }
//    }
//    for (auto &vv: sscc) {
//      vv->data_->trigger();
//    }
//  }
//
//  if (defer != nullptr) {
//    //if (commit_by_other) {
//    //    Log::debug("reply finish request of txn: %llx, it's committed by other txn", vv->data_.id());
//    //} else {
//    Log::debug("reply finish request of txn: %llx", txn_id);
//    //}
//    defer->reply();
//  }
//}
//
//
//void RccDTxn::exe_deferred(vector<std::pair<RequestHeader,
//                                            map<int32_t, Value> > > &outputs) {
//
//  verify(phase_ == 1);
//  phase_ = 2;
//  if (dreqs_.size() == 0) {
//    // this tid does not need deferred execution.
//    //verify(0);
//  } else {
//    // delayed execution
//    outputs.clear(); // FIXME does this help? seems it does, why?
//    for (auto &cmd: dreqs_) {
//      auto txn_handler_pair = txn_reg_->get(cmd.root_type_, cmd.type_);
////      verify(header.tid == tid_);
//
//      map<int32_t, Value> output;
//      int output_size = 0;
//      int res;
//      txn_handler_pair.txn_handler(nullptr,
//                                   this,
//                                   cmd,
//                                   &res,
//                                   output);
//      if (cmd.type_ == TPCC_PAYMENT_4 && cmd.root_type_ == TPCC_PAYMENT)
//        verify(output_size == 15);
//
//      // FIXME. what the fuck happens here?
//      // XXX FIXME
//      verify(0);
////      RequestHeader header;
////      auto pp = std::make_pair(header, output);
////      outputs.push_back(pp);
//    }
//  }
//}
//

void RccDTxn::kiss(mdb::Row *r, int col, bool immediate) {
    verify(0);
}

bool RccDTxn::checkInquire() {
    if (inquiretimer_ == 0){
        inquiretimer_ = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        return false;
    }else{
        int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (now - inquiretimer_ > 100){
            return true;
        }else{
            return false;
        }
    }
}

bool RccDTxn::ReadColumn(mdb::Row *row,
                         mdb::column_id_t col_id,
                         Value *value,
                         int hint_flag) {
    verify(!read_only_);
    if (phase_ == PHASE_RCC_DISPATCH) {
        int8_t edge_type;
        //        Log_debug("%s row empty? %d, col id = %u", __FUNCTION__, row == nullptr, col_id);
        if (hint_flag == TXN_BYPASS || hint_flag == TXN_INSTANT) {
            mdb_txn()->read_column(row, col_id, value);
//            TraceDep(row, col_id, hint_flag);
        }

        if (hint_flag == TXN_INSTANT || hint_flag == TXN_DEFERRED) {
            //      entry_t *entry = ((RCCRow *) row)->get_dep_entry(col_id);
            TraceDep(row, col_id, hint_flag);
        }
    } else if (phase_ == PHASE_RCC_COMMIT) {
            mdb_txn()->read_column(row, col_id, value);
    } else {
        verify(0);
    }
    return true;
}


bool RccDTxn::WriteColumn(Row *row,
                          column_id_t col_id,
                          const Value &value,
                          int hint_flag) {
    verify(!read_only_);
    if (phase_ == PHASE_RCC_DISPATCH) {
        //        Log_debug("%s row empty? %d, col id = %u", __FUNCTION__, row == nullptr, col_id);
        if (hint_flag == TXN_BYPASS || hint_flag == TXN_INSTANT) {
            mdb_txn()->write_column(row, col_id, value);
            TraceDep(row, col_id, hint_flag);
        }
        if (hint_flag == TXN_INSTANT || hint_flag == TXN_DEFERRED) {
            //      entry_t *entry = ((RCCRow *) row)->get_dep_entry(col_id);
            TraceDep(row, col_id, hint_flag);
        }
    } else if (phase_ == PHASE_RCC_COMMIT) {
        if (hint_flag == TXN_BYPASS || hint_flag == TXN_DEFERRED) {
            mdb_txn()->write_column(row, col_id, value);
        } else {
            verify(0);
        }
    } else {
        verify(0);
    }
    return true;
}

void RccDTxn::AddParentEdge(RccDTxn *other, int8_t weight) {
    Vertex::AddParentEdge(other, weight);
    if (sched_) {
        verify(other->epoch_ > 0);
        sched_->epoch_mgr_.IncrementRef(other->epoch_);
    }
}

void RccDTxn::TraceDepUnkKey(vector<RccDTxn *> &unk_key_txns_) {
    // TODO remove pointers in outdated epoch ???
    Log_debug("%s called for txn %lu", __FUNCTION__, this->id());

    //xs TODO: check this
    int8_t edge_type = EDGE_D;

    if (unk_key_txns_.empty()) {
        Log_debug("Unk_key_txns empty");
        return;
    }

    RccDTxn *parent_dtxn = unk_key_txns_.back();

    verify(parent_dtxn != nullptr);
    Log_debug("[%s] adding txn %lu as a parent of txn %lu, edge type = %hhu", __FUNCTION__, parent_dtxn->id(), this->id(), edge_type);

    if (parent_dtxn == this) {
        // skip
    } else {
        if (parent_dtxn->IsExecuted()) {
            ;
        } else {
            this->AddParentEdge(parent_dtxn, edge_type);
        }
    }
}
void RccDTxn::TraceDep(Row *row, column_id_t col_id, int hint_flag) {
    // TODO remove pointers in outdated epoch ???
    auto r = dynamic_cast<RCCRow *>(row);
    Log_debug("%s called for row %p col %d", __FUNCTION__, r, col_id);
    verify(r != nullptr);
    entry_t *entry = r->get_dep_entry(col_id);


    //xs TODO: check this
    int8_t edge_type = (hint_flag == TXN_INSTANT) ? EDGE_I : EDGE_D;
    RccDTxn *parent_dtxn = (RccDTxn *) (entry->last_);

    if (parent_dtxn != nullptr) {
        Log_debug("[%s] adding txn %lu as a parent of txn %lu, edge type = %hhu", __FUNCTION__, parent_dtxn->id(), this->id(), edge_type);
    }

    if (parent_dtxn == this) {
        // skip
    } else {
        if (parent_dtxn != nullptr) {
            if (parent_dtxn->IsExecuted()) {
                ;
            } else {
                if (sched_->epoch_enabled_) {
                    auto epoch1 = parent_dtxn->epoch_;
                    auto epoch2 = sched_->epoch_mgr_.oldest_active_;
                    // adding a parent from an inactive epoch is
                    // dangerous.
                    // verify(epoch1 >= epoch2);
                }
                this->AddParentEdge(parent_dtxn, edge_type);
            }
            //      parent_dtxn->external_ref_ = nullptr;
        }
        this->external_refs_.push_back(&(entry->last_));
        entry->last_ = this;
    }
#ifdef DEBUG_CODE
    verify(graph_);
    TxnInfo &tinfo = tv_->Get();
    auto s = tinfo.status();
    verify(s < TXN_CMT);
//  RccScc& scc = graph_->FindSCC(tv_);
//  if (scc.size() > 1 && graph_->HasICycle(scc)) {
//    verify(0);
//  }
#endif
}

}// namespace rococo

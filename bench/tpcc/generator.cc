
#include "generator.h"
#include "deptran/txn_chopper.h"
#include "piece.h"

namespace rococo {

TpccTxnGenerator::TpccTxnGenerator(Config *config, parid_t partition_id) : TxnGenerator(config) {
    std::map<std::string, uint64_t> table_num_rows;
    sharding_->get_number_rows(table_num_rows);
    this->partition_id_ = partition_id;

    std::vector<unsigned int> partitions;
    sharding_->GetTablePartitions(TPCC_TB_WAREHOUSE, partitions);
    uint64_t tb_w_rows = table_num_rows[std::string(TPCC_TB_WAREHOUSE)];
    tpcc_para_.n_w_id_ = (int) tb_w_rows * partitions.size();

    tpcc_para_.const_home_w_id_ = RandomGenerator::rand(0, tpcc_para_.n_w_id_ - 1);
    uint64_t tb_d_rows = table_num_rows[std::string(TPCC_TB_DISTRICT)];
    tpcc_para_.n_d_id_ = (int) tb_d_rows;
    uint64_t tb_c_rows = table_num_rows[std::string(TPCC_TB_CUSTOMER)];
    uint64_t tb_r_rows = table_num_rows[std::string(TPCC_TB_REGION)];
    tpcc_para_.n_r_id_ = (int) tb_r_rows;
    tpcc_para_.n_c_id_ = (int) tb_c_rows / tb_d_rows;

    tpcc_para_.n_i_id_ = (int) table_num_rows[std::string(TPCC_TB_ITEM)];
    tpcc_para_.delivery_d_id_ = RandomGenerator::rand(0, tpcc_para_.n_d_id_ - 1);
    switch (single_server_) {
        case Config::SS_DISABLED:
            fix_id_ = -1;
            break;
        case Config::SS_THREAD_SINGLE:
        case Config::SS_PROCESS_SINGLE: {
            fix_id_ = Config::GetConfig()->get_client_id() % tpcc_para_.n_d_id_;
            tpcc_para_.const_home_w_id_ =
                    Config::GetConfig()->get_client_id() / tpcc_para_.n_d_id_;
            break;
        }
        default:
            verify(0);
    }
}


void TpccTxnGenerator::GetNewOrderTxnReq(TxnRequest *req,
                                         uint32_t cid) const {
    req->is_local_txn_ = true;
    req->txn_type_ = TPCC_NEW_ORDER;
    //int home_w_id = RandomGenerator::rand(0, tpcc_para_.n_w_id_ - 1);
    int home_w_id = cid % tpcc_para_.n_w_id_;
    Value w_id((i32) home_w_id);
    req->home_region_id = home_w_id;
    //Value d_id((i32)RandomGenerator::rand(0, tpcc_para_.n_d_id_ - 1));
    Value d_id((i32)(cid / tpcc_para_.n_w_id_) % tpcc_para_.n_d_id_);
    Value c_id((i32) RandomGenerator::nu_rand(1022, 0, tpcc_para_.n_c_id_ - 1));
    int ol_cnt = RandomGenerator::rand(6, 15);
    //  int ol_cnt = 0;

    rrr::i32 i_id_buf[ol_cnt];

    req->input_[TPCC_VAR_W_ID] = w_id;
    req->input_[TPCC_VAR_D_ID] = d_id;
    req->input_[TPCC_VAR_C_ID] = c_id;
    req->input_[TPCC_VAR_OL_CNT] = Value((i32) ol_cnt);
    req->input_[TPCC_VAR_O_CARRIER_ID] = Value((int32_t) 0);
    bool all_local = true;
    for (int i = 0; i < ol_cnt; i++) {
        //req->input_[4 + 3 * i] = Value((i32)RandomGenerator::nu_rand(8191, 0, tpcc_para_.n_i_id_ - 1)); XXX nurand is the standard
        rrr::i32 tmp_i_id;
        while (true) {
            tmp_i_id = (i32) RandomGenerator::rand(0, tpcc_para_.n_i_id_ - 1 - i);
            if ((int) tmp_i_id % tpcc_para_.n_w_id_ == home_w_id) {
                break;
            }
        }
        //int pre_n_less = 0, n_less = 0;
        //while (true) {
        //  n_less = 0;
        //  for (int j = 0; j < i; j++)
        //    if (i_id_buf[j] <= tmp_i_id)
        //      n_less++;
        //  if (n_less == pre_n_less)
        //    break;
        //  tmp_i_id += (n_less - pre_n_less);
        //  pre_n_less = n_less;
        //}

        i_id_buf[i] = tmp_i_id;
        req->input_[TPCC_VAR_I_ID(i)] = Value(tmp_i_id);
        req->input_[TPCC_VAR_OL_NUMBER(i)] = i;
        req->input_[TPCC_VAR_OL_DELIVER_D(i)] = std::string();

        //auto config = Config::GetConfig();
        //int new_order_remote_rate = config->tpcc_payment_crt_rate_;
        //Log_info("new_order_remote_rate %d", new_order_remote_rate);
        if (tpcc_para_.n_w_id_ > 1 &&             // warehouse more than one, can do remote
            RandomGenerator::percentage_true(1)) {//XXX 1% REMOTE_RATIO
            int remote_w_id = RandomGenerator::rand(0, tpcc_para_.n_w_id_ - 2);
            remote_w_id = remote_w_id >= home_w_id ? remote_w_id + 1 : remote_w_id;
            if (!partitions_in_same_region((parid_t) remote_w_id, (parid_t) home_w_id)) {
                req->is_local_txn_ = false;
            }

            req->input_[TPCC_VAR_S_W_ID(i)] = Value((i32) remote_w_id);
            req->input_[TPCC_VAR_S_REMOTE_CNT(i)] = Value((i32) 1);
            //      verify(req->input_[TPCC_VAR_S_W_ID(i)].get_i32() < 3);
            all_local = false;
        } else {
            req->input_[TPCC_VAR_S_W_ID(i)] = req->input_[TPCC_VAR_W_ID];
            req->input_[TPCC_VAR_S_REMOTE_CNT(i)] = Value((i32) 0);
            //      verify(req->input_[TPCC_VAR_S_W_ID(i)].get_i32() < 3);
        }
        req->input_[TPCC_VAR_OL_QUANTITY(i)] = Value((i32) RandomGenerator::rand(0, 10));
    }
    req->input_[TPCC_VAR_O_ALL_LOCAL] = all_local ? Value((i32) 1) : Value((i32) 0);
}


void TpccTxnGenerator::get_tpcc_payment_txn_req(
        TxnRequest *req, uint32_t cid) const {
    req->is_local_txn_ = true;
    req->txn_type_ = TPCC_PAYMENT;
    //DAST's Edge semantics
    int home_w_id = (int) cid % tpcc_para_.n_w_id_;
    req->home_region_id = home_w_id;
    Value c_w_id, c_d_id;
    Value w_id((i32) home_w_id);
    Value d_id((i32)(cid / tpcc_para_.n_w_id_) % tpcc_para_.n_d_id_);

    bool by_last_name = false;
    if (RandomGenerator::percentage_true(40)) {
        //This will cause cross-region dependencies.
        by_last_name = true;
        req->input_[TPCC_VAR_C_LAST] =
                Value(RandomGenerator::int2str_n(RandomGenerator::nu_rand(255, 0, 999), 3));
    } else {
        req->input_[TPCC_VAR_C_ID] =
                Value((i32) RandomGenerator::nu_rand(1022, 0, tpcc_para_.n_c_id_ - 1));
    }
    auto config = Config::GetConfig();
    int crt_ratio = config->tpcc_payment_crt_rate_;

    //NO need to work for N_W_ID = 1;
    verify(tpcc_para_.n_w_id_ > 1);

    Log_info("crt ratio %d", crt_ratio);

    if (crt_ratio >= 0) {
        // Testing different ratios of CRTs

        if (RandomGenerator::percentage_true(crt_ratio)) {
            // A CRT
            // Random select a shard that is not in my region.
            int c_w_id_int;
            do {
                c_w_id_int = RandomGenerator::rand(0, tpcc_para_.n_w_id_ - 2);
                c_w_id_int = c_w_id_int >= home_w_id ? c_w_id_int + 1 : c_w_id_int;
                req->is_local_txn_ = partitions_in_same_region((parid_t) c_w_id_int, (parid_t) home_w_id);
                Log_info("[CRT ratio test] c_w_id_int = %d, home_id = %d, is_local = %d, by_last_name = %d", c_w_id_int, home_w_id,
                         req->is_local_txn_, by_last_name);
            } while (req->is_local_txn_);//this should be a distributed txn

            c_w_id = Value((i32) c_w_id_int);
            c_d_id = Value((i32) RandomGenerator::rand(0, tpcc_para_.n_d_id_ - 1));
        } else {
            // An IRT.
            // By default 15% will go to another shard within my region.
            // 85% will be single-shard transaction.
            req->is_local_txn_ = true;

            if (RandomGenerator::percentage_true(15)) {
                int region_id = home_w_id / Config::GetConfig()->n_shard_per_region_;
                int c_w_id_int = region_id * Config::GetConfig()->n_shard_per_region_ + RandomGenerator::rand(0, Config::GetConfig()->n_shard_per_region_ - 2);
                c_w_id_int = c_w_id_int >= home_w_id ? c_w_id_int + 1 : c_w_id_int;
                Log_info("[CRT ratio test] c_w_id_int = %d, home_id = %d, is_local = %d, by_last_name = %d", c_w_id_int, home_w_id,
                         req->is_local_txn_, by_last_name);
                c_w_id = Value((i32) c_w_id_int);
                c_d_id = Value((i32) RandomGenerator::rand(0, tpcc_para_.n_d_id_ - 1));
            } else {
                Log_info("[CRT ratio test], single-shard payment, home_id = %d", w_id.get_i32());
                c_w_id = w_id;
                c_d_id = d_id;
            }
        }
    } else if (crt_ratio == -1) {
        //Normal test, not setting specific CRT ratios

        if (RandomGenerator::percentage_true(15)) {
            //with 15\% rate, randomly select a remote shard (warehouse) from all warehouses.
            int c_w_id_int = RandomGenerator::rand(0, tpcc_para_.n_w_id_ - 2);
            c_w_id_int = c_w_id_int >= home_w_id ? c_w_id_int + 1 : c_w_id_int;
            req->is_local_txn_ = partitions_in_same_region((parid_t) c_w_id_int, (parid_t) home_w_id);
            Log_info("[Not in CRT ratio test] c_w_id_int = %d, home_id = %d, is_local = %d, by_last_name = %d", c_w_id_int, home_w_id,
                     req->is_local_txn_, by_last_name);
            c_w_id = Value((i32) c_w_id_int);
            c_d_id = Value((i32) RandomGenerator::rand(0, tpcc_para_.n_d_id_ - 1));
        } else {
            //with 85\% rate, access the same shard (warehouse) as me.
            Log_info("[Not in CRT ratio test], single-shard payment, home_id = %d", home_w_id);
            c_w_id = w_id;
            c_d_id = d_id;
        }
    } else {
        Log_error("CRT ratio should be set a non-negative number for testing different CRT ratios, and \"-1\" for other tests");
        verify(0);
    }
    Value h_amount(RandomGenerator::rand_double(1.00, 5000.00));
    //  req->input_.resize(7);
    req->input_[TPCC_VAR_W_ID] = w_id;
    req->input_[TPCC_VAR_D_ID] = d_id;
    req->input_[TPCC_VAR_C_W_ID] = c_w_id;
    req->input_[TPCC_VAR_C_D_ID] = c_d_id;
    req->input_[TPCC_VAR_H_AMOUNT] = h_amount;
    rrr::i32 tmp_h_key = home_w_id;
    req->input_[TPCC_VAR_H_KEY] = Value(tmp_h_key);// h_key
                                                   //  req->input_[TPCC_VAR_W_NAME] = Value();
                                                   //  req->input_[TPCC_VAR_D_NAME] = Value();
}

void TpccTxnGenerator::get_tpcc_stock_level_txn_req(
        TxnRequest *req, uint32_t cid) const {
    req->is_local_txn_ = true;
    req->txn_type_ = TPCC_STOCK_LEVEL;
    req->input_[TPCC_VAR_W_ID] = Value((i32)(cid % tpcc_para_.n_w_id_));
    req->home_region_id = (i16)(cid % tpcc_para_.n_w_id_);
    req->input_[TPCC_VAR_D_ID] = Value((i32)(cid / tpcc_para_.n_w_id_) % tpcc_para_.n_d_id_);
    req->input_[TPCC_VAR_THRESHOLD] = Value((i32) RandomGenerator::rand(10, 20));
}

void TpccTxnGenerator::get_tpcc_delivery_txn_req(
        TxnRequest *req, uint32_t cid) const {
    req->is_local_txn_ = true;
    req->txn_type_ = TPCC_DELIVERY;
    req->input_[TPCC_VAR_W_ID] = Value((i32)(cid % tpcc_para_.n_w_id_));
    req->home_region_id = (i16)(cid % tpcc_para_.n_w_id_);
    req->input_[TPCC_VAR_O_CARRIER_ID] = Value((i32) RandomGenerator::rand(1, 10));
    req->input_[TPCC_VAR_D_ID] = Value((i32)(cid / tpcc_para_.n_w_id_) % tpcc_para_.n_d_id_);
}


void TpccTxnGenerator::get_tpcc_order_status_txn_req(
        TxnRequest *req, uint32_t cid) const {
    req->is_local_txn_ = true;
    req->txn_type_ = TPCC_ORDER_STATUS;
    if (RandomGenerator::percentage_true(60)) {//XXX 60% by c_last
        req->input_[TPCC_VAR_C_LAST] =
                Value(RandomGenerator::int2str_n(RandomGenerator::nu_rand(255, 0, 999),
                                                 3));
    } else {
        req->input_[TPCC_VAR_C_ID] =
                Value((i32) RandomGenerator::nu_rand(1022, 0, tpcc_para_.n_c_id_ - 1));
    }
    req->input_[TPCC_VAR_W_ID] = Value((i32)(cid % tpcc_para_.n_w_id_));
    req->home_region_id = (i16)(cid % tpcc_para_.n_w_id_);
    req->input_[TPCC_VAR_D_ID] = Value((i32)((cid / tpcc_para_.n_w_id_) % tpcc_para_.n_d_id_));
}

void TpccTxnGenerator::get_tpcc_query2_txn_req(
        TxnRequest *req, uint32_t cid) const {
    req->is_local_txn_ = false;
    req->txn_type_ = TPCC_QUERY2;
    req->home_region_id = (i16)(cid % tpcc_para_.n_w_id_);
    int home_w_id = cid % tpcc_para_.n_w_id_;
    Value w_id((i32) home_w_id);
    req->input_[TPCC_VAR_W_ID] = w_id;
    rrr::i32 tmp_r_id = (i32) RandomGenerator::rand(0, tpcc_para_.n_r_id_ - 1);
    req->input_[TPCC_VAR_R_ID] = Value(tmp_r_id);
    for (int i = 0; i < tpcc_para_.n_w_id_; i++) {
        req->input_[TPCC_VAR_RS_W_ID(i)] = Value(i);
    }
    req->input_[TPCC_VAR_NUM_N_ID] = Value(10);
    req->input_[TPCC_VAR_NUM_W_ID] = Value(tpcc_para_.n_w_id_);
    req->input_[TPCC_VAR_NUM_I_ID] = Value(tpcc_para_.n_i_id_);
}

void TpccTxnGenerator::GetTxnReq(TxnRequest *req, uint32_t cid) {
    req->n_try_ = n_try_;
    if (txn_weight_.size() != 6) {
        verify(0);
        GetNewOrderTxnReq(req, cid);
    } else {
        switch (RandomGenerator::weighted_select(txn_weight_)) {
            case 0:
                GetNewOrderTxnReq(req, cid);
                break;
            case 1:
                get_tpcc_payment_txn_req(req, cid);
                break;
            case 2:
                get_tpcc_order_status_txn_req(req, cid);
                break;
            case 3:
                get_tpcc_delivery_txn_req(req, cid);
                break;
            case 4:
                get_tpcc_stock_level_txn_req(req, cid);
                break;
            case 5:
                get_tpcc_query2_txn_req(req, cid);
                break;
            default:
                verify(0);
        }
    }
}

}// namespace rococo

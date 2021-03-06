#include "chopper.h"
#include "piece.h"
#include "generator.h"
#define N_N_ID 10
#define SUPPLIER_FRACTION 10 //max 100
#define N_W_ID (Config::GetConfig()->GetNumPartition())
#define N_I_ID 100000 

namespace rococo {

static uint32_t TXN_TYPE = TPCC_QUERY2;

void TpccTxn::Query2Init(TxnRequest &req) {
  Query2Retry();
}

void TpccTxn::Query2Retry() {
  status_[TPCC_QUERY2_0] = WAITING;
  //can be a fraction of supplier table
  int32_t supplier_cnt = SUPPLIER_FRACTION;
  int32_t n_id_cnt = ws_[TPCC_VAR_NUM_N_ID].get_i32();
  int32_t w_id_cnt = ws_[TPCC_VAR_NUM_W_ID].get_i32();
  int32_t i_id_cnt = ws_[TPCC_VAR_NUM_I_ID].get_i32();

  n_pieces_all_ = 1 + N_N_ID + supplier_cnt + supplier_cnt * N_W_ID ;
 // n_pieces_all_ = 1 + N_N_ID + N_W_ID * SUPPLIER_FRACTION;
  n_pieces_dispatchable_ = 0;
  n_pieces_dispatch_acked_ = 0;
  n_pieces_dispatched_ = 0;
  for (int i = 0; i < supplier_cnt; i++){
    for(int j = 0; j< N_W_ID; j++){
      status_[TPCC_QUERY2_RS(i*N_W_ID+j)] = WAITING;
    }
  }
  for (int i = 0; i < supplier_cnt; i++) {
    status_[TPCC_QUERY2_RSU(i)] = WAITING;
    status_[TPCC_QUERY2_WS(i)] = WAITING;
  }
  CheckReady();
}

void TpccPiece::RegQuery2() {
  // read and filter nation locally
  INPUT_PIE(TPCC_QUERY2, TPCC_QUERY2_0,
            TPCC_VAR_W_ID, TPCC_VAR_R_ID)
  SHARD_PIE(TPCC_QUERY2, TPCC_QUERY2_0,
            TPCC_TB_NATION, TPCC_VAR_W_ID);
  BEGIN_PIE(TPCC_QUERY2, TPCC_QUERY2_0,
            DF_NO) {
    verify(cmd.input.size() >= 2);
    verify(cmd.input[TPCC_VAR_W_ID].get_i32() >= 0);
    verify(cmd.input[TPCC_VAR_R_ID].get_i32() >= 0);
    for(int i = 0; i< N_N_ID ;i++){
      mdb::MultiBlob mb(3);
      mb[1] = cmd.input[TPCC_VAR_W_ID].get_blob();
      mb[2] = cmd.input[TPCC_VAR_R_ID].get_blob();
      auto nation_id =  Value((int32_t)i);
      mb[0] = nation_id.get_blob();
      Log_debug("query2 w_id: %x r_id: %x",
              cmd.input[TPCC_VAR_W_ID].get_i32(),
              cmd.input[TPCC_VAR_R_ID].get_i32());
      mdb::Row *row_nation = dtxn->Query(dtxn->GetTable(TPCC_TB_NATION),
                              mb,
                              ROW_NATION);
      dtxn->ReadColumn(row_nation,
                     TPCC_COL_NATION_N_NATION_KEY,
                     &output[TPCC_VAR_N_KEY(i)],
                     TXN_BYPASS);
    }
    *res = SUCCESS;
    return;
  } END_PIE

  for (int i = (0); i < (1000); i++) {
    // 1000 is a magical number?
    SHARD_PIE(TPCC_QUERY2, TPCC_QUERY2_RSU(i),
              TPCC_TB_SUPPLIER, TPCC_VAR_W_ID)
    INPUT_PIE(TPCC_QUERY2, TPCC_QUERY2_RSU(i),
              TPCC_VAR_W_ID, TPCC_VAR_N_KEY(i))
  }

  BEGIN_LOOP_PIE(TPCC_QUERY2, TPCC_QUERY2_RSU(0), 1000, DF_NO)
    verify(cmd.input.size() >= 2);
    Log_debug("TPCC_QUERY2, piece: %d", TPCC_QUERY2_RSU(I));
    for(int j = 0; j< SUPPLIER_FRACTION; j++){
      mdb::MultiBlob mb(3);
      mb[1] = cmd.input[TPCC_VAR_W_ID].get_blob();
      mb[2] = cmd.input[TPCC_VAR_N_KEY(I)].get_blob();
      auto supplier_id =  Value((int32_t)j);
      mb[0] = supplier_id.get_blob();
      Log_debug("query2 w_id: %x n_key: %x",
              cmd.input[TPCC_VAR_W_ID].get_i32(),
              cmd.input[TPCC_VAR_N_KEY(I)].get_i32());
      mdb::Row *row_supplier = dtxn->Query(dtxn->GetTable(TPCC_TB_SUPPLIER),
                              mb,
                              ROW_SUPPLIER);
      dtxn->ReadColumn(row_supplier,
                     TPCC_COL_SUPPLIER_S_SUPPLIERKEY,
                     &output[TPCC_VAR_SU_KEY(I*SUPPLIER_FRACTION+j)],
                     TXN_BYPASS);
      output[TPCC_VAR_OUT_NATION_KEY(I*SUPPLIER_FRACTION+j)] = cmd.input[TPCC_VAR_N_KEY(I)];
      
    }    
    *res = SUCCESS;
    return;
  END_LOOP_PIE

for (int i = (0); i < SUPPLIER_FRACTION; i++) {
    // 1000 is a magical number?
    for (int j = (0); j < N_W_ID; j++){
    SHARD_PIE(TPCC_QUERY2, TPCC_QUERY2_RS(i*N_W_ID+j),
              TPCC_TB_STOCK, TPCC_VAR_RS_W_ID(j))
    INPUT_PIE(TPCC_QUERY2, TPCC_QUERY2_RS(i*N_W_ID+j),
              TPCC_VAR_SU_KEY(i),TPCC_VAR_RS_W_ID(j))
  }
}
  BEGIN_LOOP_PIE(TPCC_QUERY2, TPCC_QUERY2_RS(0), N_W_ID * SUPPLIER_FRACTION, DF_NO)
    verify(cmd.input.size() >= 2);
    Log_debug("TPCC_QUERY2, piece: %d", TPCC_QUERY2_RS(I));
    Value min_quan(0); 
    Value min_item(0);
    for(int j = 0; j< N_W_ID ; j++){
      int warehouse_index = I % N_W_ID;
      int supplier_index =  I / N_W_ID;
     //Log_info("1");
      if((j*(int)cmd.input[TPCC_VAR_RS_W_ID(warehouse_index)].get_i32()) % 10000 != (int)cmd.input[TPCC_VAR_SU_KEY(supplier_index)].get_i32()){
        continue;
      }
     // Log_info("2");
      mdb::MultiBlob mb(2);
      mb[1] = cmd.input[TPCC_VAR_RS_W_ID(warehouse_index)].get_blob();
      auto item_id =  Value((int32_t)j);
    //  Log_info("3");
      mb[0] = item_id.get_blob();
      mdb::Row *row_stock = dtxn->Query(dtxn->GetTable(TPCC_TB_STOCK),
                              mb,
                              ROW_STOCK);
      if(row_stock){
        Value tmp(0);
        dtxn->ReadColumn(row_stock,
                     TPCC_COL_STOCK_S_QUANTITY,
                     &tmp,
                     TXN_BYPASS);
        if(((i32)(tmp.get_i32())) <  ((i32)(min_quan.get_i32()))){
          min_quan.set_i32((i32)tmp.get_i32());
          dtxn->ReadColumn(row_stock,
                     TPCC_COL_STOCK_S_I_ID,
                     &min_item,
                     TXN_BYPASS);
        }
      }
    }  
    output[TPCC_VAR_MIN_ITEM(I)] = min_item;
    output[TPCC_VAR_MIN_QUAN(I)] = min_quan;

    *res = SUCCESS;
    return;
  END_LOOP_PIE

for (int i = (0); i < (1000); i++) {
    // 1000 is a magical number?
    SHARD_PIE(TPCC_QUERY2, TPCC_QUERY2_WS(i),
              TPCC_TB_SUPPLIER, TPCC_VAR_W_ID)
    INPUT_PIE(TPCC_QUERY2, TPCC_QUERY2_WS(i),
              TPCC_VAR_W_ID,
              TPCC_VAR_SU_KEY(i), TPCC_VAR_OUT_NATION_KEY(i))
    for(int j = 0;j<N_W_ID ;j++){
      txn_reg_->input_vars_[TPCC_QUERY2][TPCC_QUERY2_WS(i)].insert(TPCC_VAR_MIN_ITEM(i * N_W_ID + j));
      txn_reg_->input_vars_[TPCC_QUERY2][TPCC_QUERY2_WS(i)].insert(TPCC_VAR_MIN_QUAN(i * N_W_ID + j));
   }
  }

  BEGIN_LOOP_PIE(TPCC_QUERY2, TPCC_QUERY2_WS(0), 1000, DF_REAL)
    verify(cmd.input.size() >= 3);
    Log_debug("TPCC_QUERY2, piece: %d", TPCC_QUERY2_WS(I));
    //choose the min item id from all warehouse
    Value min_quan(0); 
    Value min_item(0);
    min_quan.set_i32((i32)cmd.input[TPCC_VAR_MIN_QUAN(I * N_W_ID + 0)].get_i32());
    min_item.set_i32((i32)cmd.input[TPCC_VAR_MIN_ITEM(I * N_W_ID + 0)].get_i32());
    int min_warehouse = 0;
    for(int j = 0; j< N_W_ID; j++){
      if(((i32)(cmd.input[TPCC_VAR_MIN_QUAN(I * N_W_ID + j)].get_i32())) <  ((i32)(min_quan.get_i32())))
      {
        min_quan.set_i32((i32)cmd.input[TPCC_VAR_MIN_QUAN(I * N_W_ID + j)].get_i32());
        min_item.set_i32((i32)cmd.input[TPCC_VAR_MIN_ITEM(I * N_W_ID + j)].get_i32());
        min_warehouse = j;
      }
    }
      mdb::MultiBlob mb(3);
      mb[1] = cmd.input[TPCC_VAR_W_ID].get_blob();
      mb[2] = cmd.input[TPCC_VAR_OUT_NATION_KEY(I)].get_blob();
//      Log_info("1");
      mb[0] = cmd.input[TPCC_VAR_SU_KEY(I)].get_blob();
//      Log_info("2");
      mdb::Row *row_supplier = dtxn->Query(dtxn->GetTable(TPCC_TB_SUPPLIER),
                              mb,
                              ROW_SUPPLIER);
    
    Value buf(0);
    buf.set_i32((i32)min_warehouse);
    dtxn->WriteColumn(row_supplier,
                      TPCC_COL_SUPPLIER_S_MIN_WAREHOUSE,
                      buf,
                      TXN_DEFERRED);
    dtxn->WriteColumn(row_supplier,
                      TPCC_COL_SUPPLIER_S_MIN_ITEM,
                      min_item,
                      TXN_DEFERRED);
    dtxn->WriteColumn(row_supplier,
                      TPCC_COL_SUPPLIER_S_MIN_QUAN,
                      min_quan,
                      TXN_DEFERRED);
    *res = SUCCESS;
    return;
  END_LOOP_PIE

}
} // namespace rococo

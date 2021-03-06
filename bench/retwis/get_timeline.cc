#include "chopper.h"
#include "piece.h"
#include "generator.h"
#include "../tpcc/piece.h"
namespace rococo {

static uint32_t TXN_TYPE = RETWIS_GET_TIMELINE;

void RetwisTxn::GetTimelineInit(rococo::TxnRequest &req){
    int32_t timeline_piece_cnt = ws_[RETWIS_VAR_GET_TIMELINE_CNT].get_i32();
    for (int i = 0; i < timeline_piece_cnt; i++) {
        sss_->GetPartition(RETWIS_TB, req.input_[RETWIS_VAR_GET_TIMELINE(i)],
                           sharding_[RETWIS_GET_TIMELINE_P(i)]);
    }
  GetTimelineRetry();
}

void RetwisTxn::GetTimelineRetry(){
    int32_t timeline_piece_cnt = ws_[RETWIS_VAR_GET_TIMELINE_CNT].get_i32();
    for (int i = 0; i < timeline_piece_cnt; i++) {
        status_[RETWIS_GET_TIMELINE_P(i)] = DISPATCHABLE;
        GetWorkspace(RETWIS_GET_TIMELINE_P(i)).keys_ = {
                RETWIS_VAR_GET_TIMELINE(i)
        };
    }
    output_size_ = {{0,0}};
    n_pieces_all_ = timeline_piece_cnt;
    n_pieces_dispatchable_ =  timeline_piece_cnt;
    n_pieces_dispatch_acked_ = 0;
    n_pieces_dispatched_ = 0;
}

void RetwisPiece::RegGetTimeline(){
for (int i = (0); i < (10); i++) {
    // 10 is a magical number?
    SHARD_PIE(RETWIS_GET_TIMELINE, RETWIS_GET_TIMELINE_P(i),
              RETWIS_TB, TPCC_VAR_H_KEY)
    INPUT_PIE(RETWIS_GET_TIMELINE, RETWIS_GET_TIMELINE_P(i), RETWIS_VAR_GET_TIMELINE(i))
  }
  
  BEGIN_LOOP_PIE(RETWIS_GET_TIMELINE, RETWIS_GET_TIMELINE_P(0), 10, DF_NO)
    verify(cmd.input.size() >= 1);
    mdb::MultiBlob buf(1);
      Value result(0);
      buf[0] = cmd.input[RETWIS_VAR_GET_TIMELINE(I)].get_blob();
      auto tbl = dtxn->GetTable(RETWIS_TB);
      auto row = dtxn->Query(tbl, buf);
      dtxn->ReadColumn(row, 1, &result, TXN_BYPASS);
      *res = SUCCESS;
      return;
  END_LOOP_PIE

}
} // namespace rococo

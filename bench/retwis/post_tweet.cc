#include "chopper.h"
#include "piece.h"
#include "generator.h"
#include "../tpcc/piece.h"
namespace rococo {

static uint32_t TXN_TYPE = RETWIS_POST_TWEET;

void RetwisTxn::PostTweetInit(TxnRequest &req) {
    sss_->GetPartition(RETWIS_TB, req.input_[RETWIS_VAR_POST_TWEET_1],
                       sharding_[RETWIS_POST_TWEET_0]);
    sss_->GetPartition(RETWIS_TB, req.input_[RETWIS_VAR_POST_TWEET_2],
                       sharding_[RETWIS_POST_TWEET_1]);
    sss_->GetPartition(RETWIS_TB, req.input_[RETWIS_VAR_POST_TWEET_3],
                       sharding_[RETWIS_POST_TWEET_2]);
    sss_->GetPartition(RETWIS_TB, req.input_[RETWIS_VAR_POST_TWEET_4],
                       sharding_[RETWIS_POST_TWEET_3]);
    sss_->GetPartition(RETWIS_TB, req.input_[RETWIS_VAR_POST_TWEET_5],
                       sharding_[RETWIS_POST_TWEET_4]);
  PostTweetRetry();
}

void RetwisTxn::PostTweetRetry() {
    GetWorkspace(RETWIS_POST_TWEET_0).keys_ = {
            RETWIS_VAR_POST_TWEET_1
    };
    GetWorkspace(RETWIS_POST_TWEET_1).keys_ = {
            RETWIS_VAR_POST_TWEET_2
    };
    GetWorkspace(RETWIS_POST_TWEET_2).keys_ = {
            RETWIS_VAR_POST_TWEET_3
    };
    GetWorkspace(RETWIS_POST_TWEET_3).keys_ = {
            RETWIS_VAR_POST_TWEET_4
    };
    GetWorkspace(RETWIS_POST_TWEET_4).keys_ = {
            RETWIS_VAR_POST_TWEET_5
    };
    output_size_ = {{0,5}};
  status_[RETWIS_POST_TWEET_0] = DISPATCHABLE;
  status_[RETWIS_POST_TWEET_1] = DISPATCHABLE;
  status_[RETWIS_POST_TWEET_2] = DISPATCHABLE;
  status_[RETWIS_POST_TWEET_3] = DISPATCHABLE;
  status_[RETWIS_POST_TWEET_4] = DISPATCHABLE;
  n_pieces_all_ = 5;
  n_pieces_dispatchable_ =  5;
  n_pieces_dispatch_acked_ = 0;
  n_pieces_dispatched_ = 0;
}

void RetwisPiece::RegPostTweet(){
    INPUT_PIE(RETWIS_POST_TWEET, RETWIS_POST_TWEET_0,
              RETWIS_VAR_POST_TWEET_1)
    SHARD_PIE(RETWIS_POST_TWEET, RETWIS_POST_TWEET_0,
              RETWIS_TB, TPCC_VAR_H_KEY)
    BEGIN_PIE(RETWIS_POST_TWEET, RETWIS_POST_TWEET_0, DF_REAL) {
      verify(cmd.input.size() >= 1);
      mdb::MultiBlob buf(1);
      Value result(0);
      buf[0] = cmd.input[RETWIS_VAR_POST_TWEET_1].get_blob();
      auto tbl = dtxn->GetTable(RETWIS_TB);
      auto row = dtxn->Query(tbl, buf);
      dtxn->ReadColumn(row, 1, &result, TXN_BYPASS);
      output[0] = result;
      result.set_i32(result.get_i32()+1);
      dtxn->WriteColumn(row, 1, result, TXN_DEFERRED);
      *res = SUCCESS;
      return;
    } END_PIE

    INPUT_PIE(RETWIS_POST_TWEET, RETWIS_POST_TWEET_1,
              RETWIS_VAR_POST_TWEET_2)
    SHARD_PIE(RETWIS_POST_TWEET, RETWIS_POST_TWEET_1,
              RETWIS_TB, TPCC_VAR_H_KEY)
    BEGIN_PIE(RETWIS_POST_TWEET, RETWIS_POST_TWEET_1, DF_REAL) {
      verify(cmd.input.size() >= 1);
      mdb::MultiBlob buf(1);
      Value result(0);
      buf[0] = cmd.input[RETWIS_VAR_POST_TWEET_2].get_blob();
      auto tbl = dtxn->GetTable(RETWIS_TB);
      auto row = dtxn->Query(tbl, buf);
      dtxn->ReadColumn(row, 1, &result, TXN_BYPASS);
      output[1] = result;
      result.set_i32(result.get_i32()+1);
      dtxn->WriteColumn(row, 1, result, TXN_DEFERRED);
      *res = SUCCESS;
      return;
    } END_PIE

    INPUT_PIE(RETWIS_POST_TWEET, RETWIS_POST_TWEET_2,
              RETWIS_VAR_POST_TWEET_3)
    SHARD_PIE(RETWIS_POST_TWEET, RETWIS_POST_TWEET_2,
              RETWIS_TB, TPCC_VAR_H_KEY)
    BEGIN_PIE(RETWIS_POST_TWEET, RETWIS_POST_TWEET_2, DF_REAL) {
      verify(cmd.input.size() >= 1);
      mdb::MultiBlob buf(1);
      Value result(0);
      buf[0] = cmd.input[RETWIS_VAR_POST_TWEET_3].get_blob();
      auto tbl = dtxn->GetTable(RETWIS_TB);
      auto row = dtxn->Query(tbl, buf);
      dtxn->ReadColumn(row, 1, &result, TXN_BYPASS);
      output[2] = result;
      result.set_i32(result.get_i32()+1);
      dtxn->WriteColumn(row, 1, result, TXN_DEFERRED);
      *res = SUCCESS;
      return;
    } END_PIE

    INPUT_PIE(RETWIS_POST_TWEET, RETWIS_POST_TWEET_3,
              RETWIS_VAR_POST_TWEET_4)
    SHARD_PIE(RETWIS_POST_TWEET, RETWIS_POST_TWEET_3,
              RETWIS_TB, TPCC_VAR_H_KEY)
    BEGIN_PIE(RETWIS_POST_TWEET, RETWIS_POST_TWEET_3, DF_NO) {
      verify(cmd.input.size() >= 1);
      mdb::MultiBlob buf(1);
      Value result(0);
      buf[0] = cmd.input[RETWIS_VAR_POST_TWEET_4].get_blob();
      auto tbl = dtxn->GetTable(RETWIS_TB);
      auto row = dtxn->Query(tbl, buf);
      dtxn->ReadColumn(row, 1, &result, TXN_BYPASS);
      output[3] = result;
      *res = SUCCESS;
      return;
    } END_PIE

    INPUT_PIE(RETWIS_POST_TWEET, RETWIS_POST_TWEET_4,
              RETWIS_VAR_POST_TWEET_5)
    SHARD_PIE(RETWIS_POST_TWEET, RETWIS_POST_TWEET_4,
              RETWIS_TB, TPCC_VAR_H_KEY)
    BEGIN_PIE(RETWIS_POST_TWEET, RETWIS_POST_TWEET_4, DF_NO) {
      verify(cmd.input.size() >= 1);
      mdb::MultiBlob buf(1);
      Value result(0);
      buf[0] = cmd.input[RETWIS_VAR_POST_TWEET_5].get_blob();
      auto tbl = dtxn->GetTable(RETWIS_TB);
      auto row = dtxn->Query(tbl, buf);
      dtxn->ReadColumn(row, 1, &result, TXN_BYPASS);
      output[4] = result;
      *res = SUCCESS;
      return;
    } END_PIE

}
} // namespace rococo

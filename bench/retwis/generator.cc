//
// Created by lamont on 5/6/16.
//

#include <deptran/txn_chopper.h>
#include "generator.h"
#include "piece.h"

namespace rococo {

RetwisTxnGenerator::RetwisTxnGenerator(rococo::Config *config) : TxnGenerator(config) {
}

void RetwisTxnGenerator::GetTxnReq(TxnRequest *req, uint32_t cid) {
    req->n_try_ = n_try_;
    if (txn_weight_.size() != 4) {
        verify(0);
    }
    else {
        switch (RandomGenerator::weighted_select(txn_weight_)) {
            case 0:
                GenerateAddUsersRequest(req,cid);
                break;
            case 1:
                GenerateFollowRequest(req,cid);
                break;
            case 2:
                GeneratePostTweetRequest(req,cid);
                break;
            case 3:
                GenerateGetTimelineRequest(req,cid);
                break;

            default:
                verify(0);
        }
    }
}

void RetwisTxnGenerator::GenerateAddUsersRequest(rococo::TxnRequest *req, uint32_t cid){
  auto var1 = this->GetId(cid);
  auto var2 = this->GetId(cid);
  auto var3 = this->GetId(cid);
  req->txn_type_ = RETWIS_ADD_USERS;
  req->input_ = {
      {RETWIS_VAR_ADD_USERS_1, Value((i32) var1)},
      {RETWIS_VAR_ADD_USERS_2, Value((i32) var2)},
      {RETWIS_VAR_ADD_USERS_3, Value((i32) var3)}
  };
}

void RetwisTxnGenerator::GenerateFollowRequest(rococo::TxnRequest *req, uint32_t cid){
    auto var1 = this->GetId(cid);
    auto var2 = this->GetId(cid);
    req->txn_type_ = RETWIS_FOLLOW;
    req->input_ = {
            {RETWIS_VAR_FOLLOW_1, Value((i32) var1)},
            {RETWIS_VAR_FOLLOW_2, Value((i32) var2)}
    };
}


void RetwisTxnGenerator::GeneratePostTweetRequest(rococo::TxnRequest *req, uint32_t cid){
    auto var1 = this->GetId(cid);
    auto var2 = this->GetId(cid);
    auto var3 = this->GetId(cid);
    auto var4 = this->GetId(cid);
    auto var5 = this->GetId(cid);
    req->txn_type_ = RETWIS_POST_TWEET;
    req->input_ = {
            {RETWIS_VAR_POST_TWEET_1, Value((i32) var1)},
            {RETWIS_VAR_POST_TWEET_2, Value((i32) var2)},
            {RETWIS_VAR_POST_TWEET_3, Value((i32) var3)},
            {RETWIS_VAR_POST_TWEET_4, Value((i32) var4)},
            {RETWIS_VAR_POST_TWEET_5, Value((i32) var5)}
    };
}


void RetwisTxnGenerator::GenerateGetTimelineRequest(rococo::TxnRequest *req, uint32_t cid){
    int timeline_cnt = RandomGenerator::rand(1, 10);
    req->input_[RETWIS_VAR_GET_TIMELINE_CNT] = Value((i32) timeline_cnt);
    req->txn_type_ = RETWIS_GET_TIMELINE;
    for (int i = 0; i < timeline_cnt; i++) {
        auto var = this->GetId(cid);
        req->input_[RETWIS_VAR_GET_TIMELINE(i)] = Value((i32) var);
    }
}



int32_t RetwisTxnGenerator::GetId(uint32_t cid) {
  int32_t id;
  if (fix_id_ == -1) {
    id = RandomGenerator::rand(0, retwis_para_.n_table_ - 1);
  } else {
    auto it = this->key_ids_.find(cid);
    if (it == key_ids_.end()) {
      id = fix_id_;
      id += (cid & 0xFFFFFFFF);
      id = (id<0) ? -1*id : id;
      id %= retwis_para_.n_table_;
      key_ids_[cid] = id;
      Log_info("coordinator %d using fixed id of %d", cid, id);
    } else {
      id = it->second;
    }
  }
  return id;
}
}

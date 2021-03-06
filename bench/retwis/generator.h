//
// Created by lamont on 5/6/16.
//
#pragma once

#include "deptran/__dep__.h"
#include "deptran/txn_req_factory.h"
namespace rococo {
  class RetwisTxnGenerator : public TxnGenerator {
  public:
    map<cooid_t, int32_t> key_ids_ = {};
    RetwisTxnGenerator(Config *config);
    virtual void GetTxnReq(TxnRequest *req, uint32_t cid) override;

  protected:
    int32_t GetId(uint32_t cid);
    void GenerateAddUsersRequest(TxnRequest *req, uint32_t cid);
    void GenerateFollowRequest(TxnRequest *req, uint32_t cid);
    void GeneratePostTweetRequest(TxnRequest *req, uint32_t cid);
    void GenerateGetTimelineRequest(TxnRequest *req, uint32_t cid);
  };
}

#pragma once

#include "deptran/__dep__.h"
#include "deptran/txn_chopper.h"

namespace deptran {

class Coordinator;

class RetwisTxn : public TxnCommand {
private:
    void AddUsersInit(TxnRequest &req);
    void FollowInit(TxnRequest &req);
    void PostTweetInit(TxnRequest &req);
    void GetTimelineInit(TxnRequest &req);
    void AddUsersRetry();
    void FollowRetry();
    void PostTweetRetry();
    void GetTimelineRetry();

public:
    RetwisTxn();
  

    virtual void Init(TxnRequest &req);

    virtual bool start_callback(const std::vector<int> &pi,
                                int res,
                                BatchStartArgsHelper &bsah);

    virtual bool start_callback(int pi,
                                int res,
                                map<int32_t, Value> &output);

    virtual bool IsReadOnly();

    virtual int GetNPieceAll();

    virtual void Reset();

    virtual ~RetwisTxn();

};

}

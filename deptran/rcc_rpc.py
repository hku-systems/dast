import os
from simplerpc.marshal import Marshal
from simplerpc.future import Future

ValueTimesPair = Marshal.reg_type('ValueTimesPair', [('value', 'rrr::i64'), ('times', 'rrr::i64')])

TxnInfoRes = Marshal.reg_type('TxnInfoRes', [('start_txn', 'rrr::i32'), ('total_txn', 'rrr::i32'), ('total_try', 'rrr::i32'), ('commit_txn', 'rrr::i32'), ('num_exhausted', 'rrr::i32'), ('this_irt_latency', 'std::vector<double>'), ('this_crt_latency', 'std::vector<double>'), ('last_irt_latency', 'std::vector<double>'), ('last_crt_latency', 'std::vector<double>'), ('attempt_latency', 'std::vector<double>'), ('interval_irt_latency', 'std::vector<double>'), ('interval_crt_latency', 'std::vector<double>'), ('all_interval_latency', 'std::vector<double>'), ('num_try', 'std::vector<rrr::i32>')])

ServerResponse = Marshal.reg_type('ServerResponse', [('statistics', 'std::map<std::string, ValueTimesPair>'), ('cpu_util', 'double'), ('r_cnt_sum', 'rrr::i64'), ('r_cnt_num', 'rrr::i64'), ('r_sz_sum', 'rrr::i64'), ('r_sz_num', 'rrr::i64')])

ClientResponse = Marshal.reg_type('ClientResponse', [('txn_info', 'std::map<rrr::i32, TxnInfoRes>'), ('run_sec', 'rrr::i64'), ('run_nsec', 'rrr::i64'), ('period_sec', 'rrr::i64'), ('period_nsec', 'rrr::i64'), ('is_finish', 'rrr::i32'), ('n_asking', 'rrr::i64')])

TxnDispatchRequest = Marshal.reg_type('TxnDispatchRequest', [('id', 'rrr::i32'), ('txn_type', 'rrr::i32'), ('input', 'std::vector<Value>')])

TxnDispatchResponse = Marshal.reg_type('TxnDispatchResponse', [])

ChrTxnInfo = Marshal.reg_type('ChrTxnInfo', [('txn_id', 'rrr::i64'), ('ts', 'chr_ts_t'), ('region_leader', 'uint16_t')])

ChronosSubmitReq = Marshal.reg_type('ChronosSubmitReq', [])

DastPrepareCRTReq = Marshal.reg_type('DastPrepareCRTReq', [('anticipated_ts', 'chr_ts_t'), ('shard_leader', 'uint16_t'), ('coord_site', 'uint16_t'), ('input_ready', 'rrr::i32')])

DastNotiCRTReq = Marshal.reg_type('DastNotiCRTReq', [('txn_id', 'txnid_t'), ('anticipated_ts', 'chr_ts_t'), ('touched_sites', 'std::set<siteid_t>')])

DastNotiCommitReq = Marshal.reg_type('DastNotiCommitReq', [('txn_id', 'txnid_t'), ('original_ts', 'chr_ts_t'), ('need_input_wait', 'rrr::i32'), ('new_ts', 'chr_ts_t'), ('touched_sites', 'std::set<siteid_t>')])

DastNotiManagerWaitReq = Marshal.reg_type('DastNotiManagerWaitReq', [('txn_id', 'txnid_t'), ('original_ts', 'chr_ts_t'), ('need_input_wait', 'rrr::i32'), ('new_ts', 'chr_ts_t'), ('touched_sites', 'std::set<siteid_t>')])

DastIRSyncReq = Marshal.reg_type('DastIRSyncReq', [('my_site', 'uint16_t'), ('txn_ts', 'chr_ts_t'), ('my_txns', 'std::vector<ChrTxnInfo>'), ('delivered_ts', 'chr_ts_t')])

ChronosSubmitRes = Marshal.reg_type('ChronosSubmitRes', [])

ChronosDistExeReq = Marshal.reg_type('ChronosDistExeReq', [('txn_id', 'uint64_t'), ('decision_ts', 'chr_ts_t')])

ChronosDistExeRes = Marshal.reg_type('ChronosDistExeRes', [('is_region_leader', 'rrr::i32')])

ChronosStoreLocalReq = Marshal.reg_type('ChronosStoreLocalReq', [('txn_ts', 'chr_ts_t'), ('piggy_my_txns', 'std::vector<ChrTxnInfo>'), ('piggy_delivered_ts', 'chr_ts_t')])

ChronosStoreLocalRes = Marshal.reg_type('ChronosStoreLocalRes', [('piggy_my_ts', 'chr_ts_t'), ('piggy_my_pending_txns', 'std::vector<ChrTxnInfo>'), ('piggy_delivered_ts', 'chr_ts_t')])

ChronosProposeRemoteReq = Marshal.reg_type('ChronosProposeRemoteReq', [('src_ts', 'chr_ts_t'), ('par_input_ready', 'std::map<parid_t, rrr::i32>')])

ChronosProposeRemoteRes = Marshal.reg_type('ChronosProposeRemoteRes', [('anticipated_ts', 'chr_ts_t')])

ChronosStoreRemoteReq = Marshal.reg_type('ChronosStoreRemoteReq', [('handler_site', 'uint16_t'), ('region_leader', 'uint16_t'), ('anticipated_ts', 'chr_ts_t')])

ChronosStoreRemoteRes = Marshal.reg_type('ChronosStoreRemoteRes', [])

DastRemotePreparedReq = Marshal.reg_type('DastRemotePreparedReq', [('txn_id', 'uint64_t'), ('my_site_id', 'uint16_t'), ('my_partition_id', 'uint32_t'), ('shard_manager', 'uint16_t'), ('anticipated_ts', 'chr_ts_t')])

DastRemotePreparedRes = Marshal.reg_type('DastRemotePreparedRes', [])

DastProposeLocalReq = Marshal.reg_type('DastProposeLocalReq', [('coord_site', 'uint16_t'), ('shard_leader_site', 'uint16_t'), ('txn_ts', 'chr_ts_t')])

ChronosProposeLocalReq = Marshal.reg_type('ChronosProposeLocalReq', [('txn_anticipated_ts', 'chr_ts_t')])

ChronosProposeLocalRes = Marshal.reg_type('ChronosProposeLocalRes', [('my_site_id', 'uint16_t')])

ChronosSendOutputReq = Marshal.reg_type('ChronosSendOutputReq', [('txn_id', 'txnid_t'), ('var_values', 'std::map<varid_t, Value>')])

ChronosSendOutputRes = Marshal.reg_type('ChronosSendOutputRes', [])

OVStoreReq = Marshal.reg_type('OVStoreReq', [('ts', 'rrr::i64'), ('site_id', 'rrr::i16')])

OVStoreRes = Marshal.reg_type('OVStoreRes', [('ts', 'rrr::i64')])

OVExecuteReq = Marshal.reg_type('OVExecuteReq', [])

OVExecuteRes = Marshal.reg_type('OVExecuteRes', [])

class MultiPaxosService(object):
    FORWARD = 0x5c7a9836
    PREPARE = 0x4d76ced0
    ACCEPT = 0x6441a852
    DECIDE = 0x1b56859a

    __input_type_info__ = {
        'Forward': ['ContainerCommand'],
        'Prepare': ['uint64_t','uint64_t'],
        'Accept': ['uint64_t','uint64_t','ContainerCommand'],
        'Decide': ['uint64_t','uint64_t','ContainerCommand'],
    }

    __output_type_info__ = {
        'Forward': [],
        'Prepare': ['uint64_t'],
        'Accept': ['uint64_t'],
        'Decide': [],
    }

    def __bind_helper__(self, func):
        def f(*args):
            return getattr(self, func.__name__)(*args)
        return f

    def __reg_to__(self, server):
        server.__reg_func__(MultiPaxosService.FORWARD, self.__bind_helper__(self.Forward), ['ContainerCommand'], [])
        server.__reg_func__(MultiPaxosService.PREPARE, self.__bind_helper__(self.Prepare), ['uint64_t','uint64_t'], ['uint64_t'])
        server.__reg_func__(MultiPaxosService.ACCEPT, self.__bind_helper__(self.Accept), ['uint64_t','uint64_t','ContainerCommand'], ['uint64_t'])
        server.__reg_func__(MultiPaxosService.DECIDE, self.__bind_helper__(self.Decide), ['uint64_t','uint64_t','ContainerCommand'], [])

    def Forward(__self__, cmd):
        raise NotImplementedError('subclass MultiPaxosService and implement your own Forward function')

    def Prepare(__self__, slot, ballot):
        raise NotImplementedError('subclass MultiPaxosService and implement your own Prepare function')

    def Accept(__self__, slot, ballot, cmd):
        raise NotImplementedError('subclass MultiPaxosService and implement your own Accept function')

    def Decide(__self__, slot, ballot, cmd):
        raise NotImplementedError('subclass MultiPaxosService and implement your own Decide function')

class MultiPaxosProxy(object):
    def __init__(self, clnt):
        self.__clnt__ = clnt

    def async_Forward(__self__, cmd):
        return __self__.__clnt__.async_call(MultiPaxosService.FORWARD, [cmd], MultiPaxosService.__input_type_info__['Forward'], MultiPaxosService.__output_type_info__['Forward'])

    def async_Prepare(__self__, slot, ballot):
        return __self__.__clnt__.async_call(MultiPaxosService.PREPARE, [slot, ballot], MultiPaxosService.__input_type_info__['Prepare'], MultiPaxosService.__output_type_info__['Prepare'])

    def async_Accept(__self__, slot, ballot, cmd):
        return __self__.__clnt__.async_call(MultiPaxosService.ACCEPT, [slot, ballot, cmd], MultiPaxosService.__input_type_info__['Accept'], MultiPaxosService.__output_type_info__['Accept'])

    def async_Decide(__self__, slot, ballot, cmd):
        return __self__.__clnt__.async_call(MultiPaxosService.DECIDE, [slot, ballot, cmd], MultiPaxosService.__input_type_info__['Decide'], MultiPaxosService.__output_type_info__['Decide'])

    def sync_Forward(__self__, cmd):
        __result__ = __self__.__clnt__.sync_call(MultiPaxosService.FORWARD, [cmd], MultiPaxosService.__input_type_info__['Forward'], MultiPaxosService.__output_type_info__['Forward'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_Prepare(__self__, slot, ballot):
        __result__ = __self__.__clnt__.sync_call(MultiPaxosService.PREPARE, [slot, ballot], MultiPaxosService.__input_type_info__['Prepare'], MultiPaxosService.__output_type_info__['Prepare'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_Accept(__self__, slot, ballot, cmd):
        __result__ = __self__.__clnt__.sync_call(MultiPaxosService.ACCEPT, [slot, ballot, cmd], MultiPaxosService.__input_type_info__['Accept'], MultiPaxosService.__output_type_info__['Accept'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_Decide(__self__, slot, ballot, cmd):
        __result__ = __self__.__clnt__.sync_call(MultiPaxosService.DECIDE, [slot, ballot, cmd], MultiPaxosService.__input_type_info__['Decide'], MultiPaxosService.__output_type_info__['Decide'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

class ClassicService(object):
    DISPATCH = 0x41719b95
    PREPARE = 0x22858a2f
    COMMIT = 0x5422082b
    ABORT = 0x30ec3ed1
    UPGRADEEPOCH = 0x3d086233
    TRUNCATEEPOCH = 0x14b93f6b
    RPC_NULL = 0x26c29787
    TAPIRACCEPT = 0x128a380d
    TAPIRFASTACCEPT = 0x458b2429
    TAPIRDECIDE = 0x5e00fe9c
    RCCDISPATCH = 0x5aabd5eb
    RCCFINISH = 0x1fde66cf
    RCCINQUIRE = 0x23a13184
    RCCDISPATCHRO = 0x15ab17b5
    BRQDISPATCH = 0x3f169bf4
    BRQCOMMIT = 0x22fbf8e7
    BRQCOMMITWOGRAPH = 0x6a9dd6ff
    BRQINQUIRE = 0x641258e7
    BRQPREACCEPT = 0x6c9b5304
    BRQPREACCEPTWOGRAPH = 0x3e10653f
    BRQACCEPT = 0x14ab0158
    BRQSENDOUTPUT = 0x11dabbe3
    CHRONOSSUBMITLOCAL = 0x506174ab
    CHRONOSSUBMITTXN = 0x5992637a
    DASTPREPARECRT = 0x5ce253b1
    CHRONOSSUBMITCRT = 0x519146df
    CHRONOSSTORELOCAL = 0x13850b88
    CHRONOSPROPOSEREMOTE = 0x4f8aba2a
    CHRONOSSTOREREMOTE = 0x551e60c1
    CHRONOSPROPOSELOCAL = 0x1fded8f8
    DASTREMOTEPREPARED = 0x126132fe
    DASTNOTICOMMIT = 0x1dfa2705
    DASTIRSYNC = 0x398e3e31
    DASTNOTICRT = 0x202ea86f
    CHRONOSDISTEXE = 0x2a0497e5
    CHRONOSSENDOUTPUT = 0x411f0403
    OVSTORE = 0x50da473e
    OVCREATETS = 0x4c0f9fee
    OVSTOREDREMOVETS = 0x53773f2d
    OVEXECUTE = 0x16399aa7
    OVPUBLISH = 0x2a0e8960
    OVEXCHANGE = 0x5c638d39
    OVDISPATCH = 0x3c0a7b5f
    SLOGINSERTDISTRIBUTED = 0x5290db66
    SLOGREPLICATELOGLOCAL = 0x274f7a23
    SLOGSUBMITDISTRIBUTED = 0x2c8c78c5
    SLOGSENDBATCHREMOTE = 0x31d6cf23
    SLOGSENDBATCH = 0x332e608b
    SLOGSENDTXNOUTPUT = 0x60ba1746
    SLOGSENDDEPVALUES = 0x41afcc65
    SLOGSUBMITTXN = 0x17453aa5

    __input_type_info__ = {
        'Dispatch': ['std::vector<SimpleCommand>'],
        'Prepare': ['rrr::i64','std::vector<rrr::i32>'],
        'Commit': ['rrr::i64'],
        'Abort': ['rrr::i64'],
        'UpgradeEpoch': ['uint32_t'],
        'TruncateEpoch': ['uint32_t'],
        'rpc_null': [],
        'TapirAccept': ['uint64_t','uint64_t','int32_t'],
        'TapirFastAccept': ['uint64_t','std::vector<SimpleCommand>'],
        'TapirDecide': ['uint64_t','rrr::i32'],
        'RccDispatch': ['std::vector<SimpleCommand>'],
        'RccFinish': ['cmdid_t','RccGraph'],
        'RccInquire': ['epoch_t','txnid_t'],
        'RccDispatchRo': ['SimpleCommand'],
        'BrqDispatch': ['std::vector<SimpleCommand>'],
        'BrqCommit': ['cmdid_t','Marshallable'],
        'BrqCommitWoGraph': ['cmdid_t'],
        'BrqInquire': ['epoch_t','txnid_t'],
        'BrqPreAccept': ['cmdid_t','std::vector<SimpleCommand>','Marshallable'],
        'BrqPreAcceptWoGraph': ['cmdid_t','std::vector<SimpleCommand>'],
        'BrqAccept': ['cmdid_t','ballot_t','Marshallable'],
        'BrqSendOutput': ['ChronosSendOutputReq'],
        'ChronosSubmitLocal': ['std::vector<SimpleCommand>','ChronosSubmitReq'],
        'ChronosSubmitTxn': ['std::map<uint32_t, std::vector<SimpleCommand>>','ChronosSubmitReq','rrr::i32'],
        'DastPrepareCRT': ['std::vector<SimpleCommand>','DastPrepareCRTReq'],
        'ChronosSubmitCRT': ['std::map<uint32_t, std::map<uint32_t, std::vector<SimpleCommand>>>','ChronosSubmitReq'],
        'ChronosStoreLocal': ['std::vector<SimpleCommand>','ChronosStoreLocalReq'],
        'ChronosProposeRemote': ['std::vector<SimpleCommand>','ChronosProposeRemoteReq'],
        'ChronosStoreRemote': ['std::vector<SimpleCommand>','ChronosStoreRemoteReq'],
        'ChronosProposeLocal': ['std::vector<SimpleCommand>','DastProposeLocalReq'],
        'DastRemotePrepared': ['DastRemotePreparedReq'],
        'DastNotiCommit': ['DastNotiCommitReq'],
        'DastIRSync': ['DastIRSyncReq'],
        'DastNotiCRT': ['DastNotiCRTReq'],
        'ChronosDistExe': ['ChronosDistExeReq'],
        'ChronosSendOutput': ['ChronosSendOutputReq'],
        'OVStore': ['cmdid_t','std::vector<SimpleCommand>','OVStoreReq'],
        'OVCreateTs': ['cmdid_t'],
        'OVStoredRemoveTs': ['cmdid_t','rrr::i64','rrr::i16'],
        'OVExecute': ['cmdid_t','OVExecuteReq'],
        'OVPublish': ['rrr::i64','rrr::i16'],
        'OVExchange': ['std::string','rrr::i64','rrr::i16'],
        'OVDispatch': ['std::vector<SimpleCommand>'],
        'SlogInsertDistributed': ['std::vector<SimpleCommand>','uint64_t','std::vector<parid_t>','siteid_t','uint64_t'],
        'SlogReplicateLogLocal': ['std::vector<SimpleCommand>','uint64_t','uint64_t','uint64_t'],
        'SlogSubmitDistributed': ['std::map<uint32_t, std::vector<SimpleCommand>>'],
        'SlogSendBatchRemote': ['std::vector<std::pair<txnid_t, TxnOutput>>','parid_t'],
        'SlogSendBatch': ['uint64_t','std::vector<std::pair<txnid_t, std::vector<SimpleCommand>>>','std::vector<std::pair<txnid_t, siteid_t>>'],
        'SlogSendTxnOutput': ['txnid_t','parid_t','TxnOutput'],
        'SlogSendDepValues': ['ChronosSendOutputReq'],
        'SlogSubmitTxn': ['std::map<parid_t, std::vector<SimpleCommand>>','rrr::i32'],
    }

    __output_type_info__ = {
        'Dispatch': ['rrr::i32','TxnOutput'],
        'Prepare': ['rrr::i32'],
        'Commit': ['rrr::i32'],
        'Abort': ['rrr::i32'],
        'UpgradeEpoch': ['int32_t'],
        'TruncateEpoch': [],
        'rpc_null': [],
        'TapirAccept': [],
        'TapirFastAccept': ['rrr::i32'],
        'TapirDecide': [],
        'RccDispatch': ['rrr::i32','TxnOutput','RccGraph'],
        'RccFinish': ['std::map<uint32_t, std::map<int32_t, Value>>'],
        'RccInquire': ['RccGraph'],
        'RccDispatchRo': ['std::map<rrr::i32, Value>'],
        'BrqDispatch': ['rrr::i32','TxnOutput','Marshallable'],
        'BrqCommit': ['int32_t','TxnOutput'],
        'BrqCommitWoGraph': ['int32_t','TxnOutput'],
        'BrqInquire': ['Marshallable'],
        'BrqPreAccept': ['rrr::i32','Marshallable'],
        'BrqPreAcceptWoGraph': ['rrr::i32','Marshallable'],
        'BrqAccept': ['rrr::i32'],
        'BrqSendOutput': ['ChronosSendOutputRes'],
        'ChronosSubmitLocal': ['ChronosSubmitRes','TxnOutput'],
        'ChronosSubmitTxn': ['ChronosSubmitRes','TxnOutput'],
        'DastPrepareCRT': ['rrr::i32'],
        'ChronosSubmitCRT': ['ChronosSubmitRes','TxnOutput'],
        'ChronosStoreLocal': ['ChronosStoreLocalRes'],
        'ChronosProposeRemote': ['ChronosProposeRemoteRes'],
        'ChronosStoreRemote': ['ChronosStoreRemoteRes'],
        'ChronosProposeLocal': ['ChronosProposeLocalRes'],
        'DastRemotePrepared': ['DastRemotePreparedRes'],
        'DastNotiCommit': ['rrr::i32'],
        'DastIRSync': ['rrr::i32'],
        'DastNotiCRT': ['rrr::i32'],
        'ChronosDistExe': ['ChronosDistExeRes','TxnOutput'],
        'ChronosSendOutput': ['ChronosSendOutputRes'],
        'OVStore': ['rrr::i32','OVStoreRes'],
        'OVCreateTs': ['rrr::i64','rrr::i16'],
        'OVStoredRemoveTs': ['int32_t'],
        'OVExecute': ['int32_t','OVExecuteRes','TxnOutput'],
        'OVPublish': ['rrr::i64','rrr::i16'],
        'OVExchange': ['rrr::i64','rrr::i16'],
        'OVDispatch': ['rrr::i32','TxnOutput'],
        'SlogInsertDistributed': ['rrr::i32'],
        'SlogReplicateLogLocal': ['rrr::i32'],
        'SlogSubmitDistributed': ['TxnOutput'],
        'SlogSendBatchRemote': ['rrr::i32'],
        'SlogSendBatch': ['rrr::i32'],
        'SlogSendTxnOutput': ['rrr::i32'],
        'SlogSendDepValues': ['ChronosSendOutputRes'],
        'SlogSubmitTxn': ['TxnOutput'],
    }

    def __bind_helper__(self, func):
        def f(*args):
            return getattr(self, func.__name__)(*args)
        return f

    def __reg_to__(self, server):
        server.__reg_func__(ClassicService.DISPATCH, self.__bind_helper__(self.Dispatch), ['std::vector<SimpleCommand>'], ['rrr::i32','TxnOutput'])
        server.__reg_func__(ClassicService.PREPARE, self.__bind_helper__(self.Prepare), ['rrr::i64','std::vector<rrr::i32>'], ['rrr::i32'])
        server.__reg_func__(ClassicService.COMMIT, self.__bind_helper__(self.Commit), ['rrr::i64'], ['rrr::i32'])
        server.__reg_func__(ClassicService.ABORT, self.__bind_helper__(self.Abort), ['rrr::i64'], ['rrr::i32'])
        server.__reg_func__(ClassicService.UPGRADEEPOCH, self.__bind_helper__(self.UpgradeEpoch), ['uint32_t'], ['int32_t'])
        server.__reg_func__(ClassicService.TRUNCATEEPOCH, self.__bind_helper__(self.TruncateEpoch), ['uint32_t'], [])
        server.__reg_func__(ClassicService.RPC_NULL, self.__bind_helper__(self.rpc_null), [], [])
        server.__reg_func__(ClassicService.TAPIRACCEPT, self.__bind_helper__(self.TapirAccept), ['uint64_t','uint64_t','int32_t'], [])
        server.__reg_func__(ClassicService.TAPIRFASTACCEPT, self.__bind_helper__(self.TapirFastAccept), ['uint64_t','std::vector<SimpleCommand>'], ['rrr::i32'])
        server.__reg_func__(ClassicService.TAPIRDECIDE, self.__bind_helper__(self.TapirDecide), ['uint64_t','rrr::i32'], [])
        server.__reg_func__(ClassicService.RCCDISPATCH, self.__bind_helper__(self.RccDispatch), ['std::vector<SimpleCommand>'], ['rrr::i32','TxnOutput','RccGraph'])
        server.__reg_func__(ClassicService.RCCFINISH, self.__bind_helper__(self.RccFinish), ['cmdid_t','RccGraph'], ['std::map<uint32_t, std::map<int32_t, Value>>'])
        server.__reg_func__(ClassicService.RCCINQUIRE, self.__bind_helper__(self.RccInquire), ['epoch_t','txnid_t'], ['RccGraph'])
        server.__reg_func__(ClassicService.RCCDISPATCHRO, self.__bind_helper__(self.RccDispatchRo), ['SimpleCommand'], ['std::map<rrr::i32, Value>'])
        server.__reg_func__(ClassicService.BRQDISPATCH, self.__bind_helper__(self.BrqDispatch), ['std::vector<SimpleCommand>'], ['rrr::i32','TxnOutput','Marshallable'])
        server.__reg_func__(ClassicService.BRQCOMMIT, self.__bind_helper__(self.BrqCommit), ['cmdid_t','Marshallable'], ['int32_t','TxnOutput'])
        server.__reg_func__(ClassicService.BRQCOMMITWOGRAPH, self.__bind_helper__(self.BrqCommitWoGraph), ['cmdid_t'], ['int32_t','TxnOutput'])
        server.__reg_func__(ClassicService.BRQINQUIRE, self.__bind_helper__(self.BrqInquire), ['epoch_t','txnid_t'], ['Marshallable'])
        server.__reg_func__(ClassicService.BRQPREACCEPT, self.__bind_helper__(self.BrqPreAccept), ['cmdid_t','std::vector<SimpleCommand>','Marshallable'], ['rrr::i32','Marshallable'])
        server.__reg_func__(ClassicService.BRQPREACCEPTWOGRAPH, self.__bind_helper__(self.BrqPreAcceptWoGraph), ['cmdid_t','std::vector<SimpleCommand>'], ['rrr::i32','Marshallable'])
        server.__reg_func__(ClassicService.BRQACCEPT, self.__bind_helper__(self.BrqAccept), ['cmdid_t','ballot_t','Marshallable'], ['rrr::i32'])
        server.__reg_func__(ClassicService.BRQSENDOUTPUT, self.__bind_helper__(self.BrqSendOutput), ['ChronosSendOutputReq'], ['ChronosSendOutputRes'])
        server.__reg_func__(ClassicService.CHRONOSSUBMITLOCAL, self.__bind_helper__(self.ChronosSubmitLocal), ['std::vector<SimpleCommand>','ChronosSubmitReq'], ['ChronosSubmitRes','TxnOutput'])
        server.__reg_func__(ClassicService.CHRONOSSUBMITTXN, self.__bind_helper__(self.ChronosSubmitTxn), ['std::map<uint32_t, std::vector<SimpleCommand>>','ChronosSubmitReq','rrr::i32'], ['ChronosSubmitRes','TxnOutput'])
        server.__reg_func__(ClassicService.DASTPREPARECRT, self.__bind_helper__(self.DastPrepareCRT), ['std::vector<SimpleCommand>','DastPrepareCRTReq'], ['rrr::i32'])
        server.__reg_func__(ClassicService.CHRONOSSUBMITCRT, self.__bind_helper__(self.ChronosSubmitCRT), ['std::map<uint32_t, std::map<uint32_t, std::vector<SimpleCommand>>>','ChronosSubmitReq'], ['ChronosSubmitRes','TxnOutput'])
        server.__reg_func__(ClassicService.CHRONOSSTORELOCAL, self.__bind_helper__(self.ChronosStoreLocal), ['std::vector<SimpleCommand>','ChronosStoreLocalReq'], ['ChronosStoreLocalRes'])
        server.__reg_func__(ClassicService.CHRONOSPROPOSEREMOTE, self.__bind_helper__(self.ChronosProposeRemote), ['std::vector<SimpleCommand>','ChronosProposeRemoteReq'], ['ChronosProposeRemoteRes'])
        server.__reg_func__(ClassicService.CHRONOSSTOREREMOTE, self.__bind_helper__(self.ChronosStoreRemote), ['std::vector<SimpleCommand>','ChronosStoreRemoteReq'], ['ChronosStoreRemoteRes'])
        server.__reg_func__(ClassicService.CHRONOSPROPOSELOCAL, self.__bind_helper__(self.ChronosProposeLocal), ['std::vector<SimpleCommand>','DastProposeLocalReq'], ['ChronosProposeLocalRes'])
        server.__reg_func__(ClassicService.DASTREMOTEPREPARED, self.__bind_helper__(self.DastRemotePrepared), ['DastRemotePreparedReq'], ['DastRemotePreparedRes'])
        server.__reg_func__(ClassicService.DASTNOTICOMMIT, self.__bind_helper__(self.DastNotiCommit), ['DastNotiCommitReq'], ['rrr::i32'])
        server.__reg_func__(ClassicService.DASTIRSYNC, self.__bind_helper__(self.DastIRSync), ['DastIRSyncReq'], ['rrr::i32'])
        server.__reg_func__(ClassicService.DASTNOTICRT, self.__bind_helper__(self.DastNotiCRT), ['DastNotiCRTReq'], ['rrr::i32'])
        server.__reg_func__(ClassicService.CHRONOSDISTEXE, self.__bind_helper__(self.ChronosDistExe), ['ChronosDistExeReq'], ['ChronosDistExeRes','TxnOutput'])
        server.__reg_func__(ClassicService.CHRONOSSENDOUTPUT, self.__bind_helper__(self.ChronosSendOutput), ['ChronosSendOutputReq'], ['ChronosSendOutputRes'])
        server.__reg_func__(ClassicService.OVSTORE, self.__bind_helper__(self.OVStore), ['cmdid_t','std::vector<SimpleCommand>','OVStoreReq'], ['rrr::i32','OVStoreRes'])
        server.__reg_func__(ClassicService.OVCREATETS, self.__bind_helper__(self.OVCreateTs), ['cmdid_t'], ['rrr::i64','rrr::i16'])
        server.__reg_func__(ClassicService.OVSTOREDREMOVETS, self.__bind_helper__(self.OVStoredRemoveTs), ['cmdid_t','rrr::i64','rrr::i16'], ['int32_t'])
        server.__reg_func__(ClassicService.OVEXECUTE, self.__bind_helper__(self.OVExecute), ['cmdid_t','OVExecuteReq'], ['int32_t','OVExecuteRes','TxnOutput'])
        server.__reg_func__(ClassicService.OVPUBLISH, self.__bind_helper__(self.OVPublish), ['rrr::i64','rrr::i16'], ['rrr::i64','rrr::i16'])
        server.__reg_func__(ClassicService.OVEXCHANGE, self.__bind_helper__(self.OVExchange), ['std::string','rrr::i64','rrr::i16'], ['rrr::i64','rrr::i16'])
        server.__reg_func__(ClassicService.OVDISPATCH, self.__bind_helper__(self.OVDispatch), ['std::vector<SimpleCommand>'], ['rrr::i32','TxnOutput'])
        server.__reg_func__(ClassicService.SLOGINSERTDISTRIBUTED, self.__bind_helper__(self.SlogInsertDistributed), ['std::vector<SimpleCommand>','uint64_t','std::vector<parid_t>','siteid_t','uint64_t'], ['rrr::i32'])
        server.__reg_func__(ClassicService.SLOGREPLICATELOGLOCAL, self.__bind_helper__(self.SlogReplicateLogLocal), ['std::vector<SimpleCommand>','uint64_t','uint64_t','uint64_t'], ['rrr::i32'])
        server.__reg_func__(ClassicService.SLOGSUBMITDISTRIBUTED, self.__bind_helper__(self.SlogSubmitDistributed), ['std::map<uint32_t, std::vector<SimpleCommand>>'], ['TxnOutput'])
        server.__reg_func__(ClassicService.SLOGSENDBATCHREMOTE, self.__bind_helper__(self.SlogSendBatchRemote), ['std::vector<std::pair<txnid_t, TxnOutput>>','parid_t'], ['rrr::i32'])
        server.__reg_func__(ClassicService.SLOGSENDBATCH, self.__bind_helper__(self.SlogSendBatch), ['uint64_t','std::vector<std::pair<txnid_t, std::vector<SimpleCommand>>>','std::vector<std::pair<txnid_t, siteid_t>>'], ['rrr::i32'])
        server.__reg_func__(ClassicService.SLOGSENDTXNOUTPUT, self.__bind_helper__(self.SlogSendTxnOutput), ['txnid_t','parid_t','TxnOutput'], ['rrr::i32'])
        server.__reg_func__(ClassicService.SLOGSENDDEPVALUES, self.__bind_helper__(self.SlogSendDepValues), ['ChronosSendOutputReq'], ['ChronosSendOutputRes'])
        server.__reg_func__(ClassicService.SLOGSUBMITTXN, self.__bind_helper__(self.SlogSubmitTxn), ['std::map<parid_t, std::vector<SimpleCommand>>','rrr::i32'], ['TxnOutput'])

    def Dispatch(__self__, cmd):
        raise NotImplementedError('subclass ClassicService and implement your own Dispatch function')

    def Prepare(__self__, tid, sids):
        raise NotImplementedError('subclass ClassicService and implement your own Prepare function')

    def Commit(__self__, tid):
        raise NotImplementedError('subclass ClassicService and implement your own Commit function')

    def Abort(__self__, tid):
        raise NotImplementedError('subclass ClassicService and implement your own Abort function')

    def UpgradeEpoch(__self__, curr_epoch):
        raise NotImplementedError('subclass ClassicService and implement your own UpgradeEpoch function')

    def TruncateEpoch(__self__, old_epoch):
        raise NotImplementedError('subclass ClassicService and implement your own TruncateEpoch function')

    def rpc_null(__self__):
        raise NotImplementedError('subclass ClassicService and implement your own rpc_null function')

    def TapirAccept(__self__, cmd_id, ballot, decision):
        raise NotImplementedError('subclass ClassicService and implement your own TapirAccept function')

    def TapirFastAccept(__self__, cmd_id, txn_cmds):
        raise NotImplementedError('subclass ClassicService and implement your own TapirFastAccept function')

    def TapirDecide(__self__, cmd_id, commit):
        raise NotImplementedError('subclass ClassicService and implement your own TapirDecide function')

    def RccDispatch(__self__, cmd):
        raise NotImplementedError('subclass ClassicService and implement your own RccDispatch function')

    def RccFinish(__self__, id, in1):
        raise NotImplementedError('subclass ClassicService and implement your own RccFinish function')

    def RccInquire(__self__, epoch, txn_id):
        raise NotImplementedError('subclass ClassicService and implement your own RccInquire function')

    def RccDispatchRo(__self__, cmd):
        raise NotImplementedError('subclass ClassicService and implement your own RccDispatchRo function')

    def BrqDispatch(__self__, cmd):
        raise NotImplementedError('subclass ClassicService and implement your own BrqDispatch function')

    def BrqCommit(__self__, id, graph):
        raise NotImplementedError('subclass ClassicService and implement your own BrqCommit function')

    def BrqCommitWoGraph(__self__, id):
        raise NotImplementedError('subclass ClassicService and implement your own BrqCommitWoGraph function')

    def BrqInquire(__self__, epoch, txn_id):
        raise NotImplementedError('subclass ClassicService and implement your own BrqInquire function')

    def BrqPreAccept(__self__, txn_id, cmd, graph):
        raise NotImplementedError('subclass ClassicService and implement your own BrqPreAccept function')

    def BrqPreAcceptWoGraph(__self__, txn_id, cmd):
        raise NotImplementedError('subclass ClassicService and implement your own BrqPreAcceptWoGraph function')

    def BrqAccept(__self__, txn_id, ballot, graph):
        raise NotImplementedError('subclass ClassicService and implement your own BrqAccept function')

    def BrqSendOutput(__self__, chr_req):
        raise NotImplementedError('subclass ClassicService and implement your own BrqSendOutput function')

    def ChronosSubmitLocal(__self__, cmd, req):
        raise NotImplementedError('subclass ClassicService and implement your own ChronosSubmitLocal function')

    def ChronosSubmitTxn(__self__, cmds_by_par, req, is_local):
        raise NotImplementedError('subclass ClassicService and implement your own ChronosSubmitTxn function')

    def DastPrepareCRT(__self__, cmds, req):
        raise NotImplementedError('subclass ClassicService and implement your own DastPrepareCRT function')

    def ChronosSubmitCRT(__self__, cmds_by_region, req):
        raise NotImplementedError('subclass ClassicService and implement your own ChronosSubmitCRT function')

    def ChronosStoreLocal(__self__, cmd, req):
        raise NotImplementedError('subclass ClassicService and implement your own ChronosStoreLocal function')

    def ChronosProposeRemote(__self__, cmd, req):
        raise NotImplementedError('subclass ClassicService and implement your own ChronosProposeRemote function')

    def ChronosStoreRemote(__self__, cmd, req):
        raise NotImplementedError('subclass ClassicService and implement your own ChronosStoreRemote function')

    def ChronosProposeLocal(__self__, cmd, req):
        raise NotImplementedError('subclass ClassicService and implement your own ChronosProposeLocal function')

    def DastRemotePrepared(__self__, req):
        raise NotImplementedError('subclass ClassicService and implement your own DastRemotePrepared function')

    def DastNotiCommit(__self__, req):
        raise NotImplementedError('subclass ClassicService and implement your own DastNotiCommit function')

    def DastIRSync(__self__, req):
        raise NotImplementedError('subclass ClassicService and implement your own DastIRSync function')

    def DastNotiCRT(__self__, req):
        raise NotImplementedError('subclass ClassicService and implement your own DastNotiCRT function')

    def ChronosDistExe(__self__, chr_req):
        raise NotImplementedError('subclass ClassicService and implement your own ChronosDistExe function')

    def ChronosSendOutput(__self__, chr_req):
        raise NotImplementedError('subclass ClassicService and implement your own ChronosSendOutput function')

    def OVStore(__self__, txn_id, cmd, ov_req):
        raise NotImplementedError('subclass ClassicService and implement your own OVStore function')

    def OVCreateTs(__self__, txn_id):
        raise NotImplementedError('subclass ClassicService and implement your own OVCreateTs function')

    def OVStoredRemoveTs(__self__, txn_id, timestamp, site_id):
        raise NotImplementedError('subclass ClassicService and implement your own OVStoredRemoveTs function')

    def OVExecute(__self__, id, req):
        raise NotImplementedError('subclass ClassicService and implement your own OVExecute function')

    def OVPublish(__self__, dc_timestamp, dc_site_id):
        raise NotImplementedError('subclass ClassicService and implement your own OVPublish function')

    def OVExchange(__self__, dcname, dvw_timestamp, dvw_site_id):
        raise NotImplementedError('subclass ClassicService and implement your own OVExchange function')

    def OVDispatch(__self__, cmd):
        raise NotImplementedError('subclass ClassicService and implement your own OVDispatch function')

    def SlogInsertDistributed(__self__, cmd, index, touched_pars, handler_site, txn_id):
        raise NotImplementedError('subclass ClassicService and implement your own SlogInsertDistributed function')

    def SlogReplicateLogLocal(__self__, cmd, txn_id, index, commit_index):
        raise NotImplementedError('subclass ClassicService and implement your own SlogReplicateLogLocal function')

    def SlogSubmitDistributed(__self__, cmds_by_par):
        raise NotImplementedError('subclass ClassicService and implement your own SlogSubmitDistributed function')

    def SlogSendBatchRemote(__self__, batch, my_par_id):
        raise NotImplementedError('subclass ClassicService and implement your own SlogSendBatchRemote function')

    def SlogSendBatch(__self__, start_index, cmds, coord_sites):
        raise NotImplementedError('subclass ClassicService and implement your own SlogSendBatch function')

    def SlogSendTxnOutput(__self__, txn_id, par_id, output):
        raise NotImplementedError('subclass ClassicService and implement your own SlogSendTxnOutput function')

    def SlogSendDepValues(__self__, chr_req):
        raise NotImplementedError('subclass ClassicService and implement your own SlogSendDepValues function')

    def SlogSubmitTxn(__self__, cmds_by_par, is_irt):
        raise NotImplementedError('subclass ClassicService and implement your own SlogSubmitTxn function')

class ClassicProxy(object):
    def __init__(self, clnt):
        self.__clnt__ = clnt

    def async_Dispatch(__self__, cmd):
        return __self__.__clnt__.async_call(ClassicService.DISPATCH, [cmd], ClassicService.__input_type_info__['Dispatch'], ClassicService.__output_type_info__['Dispatch'])

    def async_Prepare(__self__, tid, sids):
        return __self__.__clnt__.async_call(ClassicService.PREPARE, [tid, sids], ClassicService.__input_type_info__['Prepare'], ClassicService.__output_type_info__['Prepare'])

    def async_Commit(__self__, tid):
        return __self__.__clnt__.async_call(ClassicService.COMMIT, [tid], ClassicService.__input_type_info__['Commit'], ClassicService.__output_type_info__['Commit'])

    def async_Abort(__self__, tid):
        return __self__.__clnt__.async_call(ClassicService.ABORT, [tid], ClassicService.__input_type_info__['Abort'], ClassicService.__output_type_info__['Abort'])

    def async_UpgradeEpoch(__self__, curr_epoch):
        return __self__.__clnt__.async_call(ClassicService.UPGRADEEPOCH, [curr_epoch], ClassicService.__input_type_info__['UpgradeEpoch'], ClassicService.__output_type_info__['UpgradeEpoch'])

    def async_TruncateEpoch(__self__, old_epoch):
        return __self__.__clnt__.async_call(ClassicService.TRUNCATEEPOCH, [old_epoch], ClassicService.__input_type_info__['TruncateEpoch'], ClassicService.__output_type_info__['TruncateEpoch'])

    def async_rpc_null(__self__):
        return __self__.__clnt__.async_call(ClassicService.RPC_NULL, [], ClassicService.__input_type_info__['rpc_null'], ClassicService.__output_type_info__['rpc_null'])

    def async_TapirAccept(__self__, cmd_id, ballot, decision):
        return __self__.__clnt__.async_call(ClassicService.TAPIRACCEPT, [cmd_id, ballot, decision], ClassicService.__input_type_info__['TapirAccept'], ClassicService.__output_type_info__['TapirAccept'])

    def async_TapirFastAccept(__self__, cmd_id, txn_cmds):
        return __self__.__clnt__.async_call(ClassicService.TAPIRFASTACCEPT, [cmd_id, txn_cmds], ClassicService.__input_type_info__['TapirFastAccept'], ClassicService.__output_type_info__['TapirFastAccept'])

    def async_TapirDecide(__self__, cmd_id, commit):
        return __self__.__clnt__.async_call(ClassicService.TAPIRDECIDE, [cmd_id, commit], ClassicService.__input_type_info__['TapirDecide'], ClassicService.__output_type_info__['TapirDecide'])

    def async_RccDispatch(__self__, cmd):
        return __self__.__clnt__.async_call(ClassicService.RCCDISPATCH, [cmd], ClassicService.__input_type_info__['RccDispatch'], ClassicService.__output_type_info__['RccDispatch'])

    def async_RccFinish(__self__, id, in1):
        return __self__.__clnt__.async_call(ClassicService.RCCFINISH, [id, in1], ClassicService.__input_type_info__['RccFinish'], ClassicService.__output_type_info__['RccFinish'])

    def async_RccInquire(__self__, epoch, txn_id):
        return __self__.__clnt__.async_call(ClassicService.RCCINQUIRE, [epoch, txn_id], ClassicService.__input_type_info__['RccInquire'], ClassicService.__output_type_info__['RccInquire'])

    def async_RccDispatchRo(__self__, cmd):
        return __self__.__clnt__.async_call(ClassicService.RCCDISPATCHRO, [cmd], ClassicService.__input_type_info__['RccDispatchRo'], ClassicService.__output_type_info__['RccDispatchRo'])

    def async_BrqDispatch(__self__, cmd):
        return __self__.__clnt__.async_call(ClassicService.BRQDISPATCH, [cmd], ClassicService.__input_type_info__['BrqDispatch'], ClassicService.__output_type_info__['BrqDispatch'])

    def async_BrqCommit(__self__, id, graph):
        return __self__.__clnt__.async_call(ClassicService.BRQCOMMIT, [id, graph], ClassicService.__input_type_info__['BrqCommit'], ClassicService.__output_type_info__['BrqCommit'])

    def async_BrqCommitWoGraph(__self__, id):
        return __self__.__clnt__.async_call(ClassicService.BRQCOMMITWOGRAPH, [id], ClassicService.__input_type_info__['BrqCommitWoGraph'], ClassicService.__output_type_info__['BrqCommitWoGraph'])

    def async_BrqInquire(__self__, epoch, txn_id):
        return __self__.__clnt__.async_call(ClassicService.BRQINQUIRE, [epoch, txn_id], ClassicService.__input_type_info__['BrqInquire'], ClassicService.__output_type_info__['BrqInquire'])

    def async_BrqPreAccept(__self__, txn_id, cmd, graph):
        return __self__.__clnt__.async_call(ClassicService.BRQPREACCEPT, [txn_id, cmd, graph], ClassicService.__input_type_info__['BrqPreAccept'], ClassicService.__output_type_info__['BrqPreAccept'])

    def async_BrqPreAcceptWoGraph(__self__, txn_id, cmd):
        return __self__.__clnt__.async_call(ClassicService.BRQPREACCEPTWOGRAPH, [txn_id, cmd], ClassicService.__input_type_info__['BrqPreAcceptWoGraph'], ClassicService.__output_type_info__['BrqPreAcceptWoGraph'])

    def async_BrqAccept(__self__, txn_id, ballot, graph):
        return __self__.__clnt__.async_call(ClassicService.BRQACCEPT, [txn_id, ballot, graph], ClassicService.__input_type_info__['BrqAccept'], ClassicService.__output_type_info__['BrqAccept'])

    def async_BrqSendOutput(__self__, chr_req):
        return __self__.__clnt__.async_call(ClassicService.BRQSENDOUTPUT, [chr_req], ClassicService.__input_type_info__['BrqSendOutput'], ClassicService.__output_type_info__['BrqSendOutput'])

    def async_ChronosSubmitLocal(__self__, cmd, req):
        return __self__.__clnt__.async_call(ClassicService.CHRONOSSUBMITLOCAL, [cmd, req], ClassicService.__input_type_info__['ChronosSubmitLocal'], ClassicService.__output_type_info__['ChronosSubmitLocal'])

    def async_ChronosSubmitTxn(__self__, cmds_by_par, req, is_local):
        return __self__.__clnt__.async_call(ClassicService.CHRONOSSUBMITTXN, [cmds_by_par, req, is_local], ClassicService.__input_type_info__['ChronosSubmitTxn'], ClassicService.__output_type_info__['ChronosSubmitTxn'])

    def async_DastPrepareCRT(__self__, cmds, req):
        return __self__.__clnt__.async_call(ClassicService.DASTPREPARECRT, [cmds, req], ClassicService.__input_type_info__['DastPrepareCRT'], ClassicService.__output_type_info__['DastPrepareCRT'])

    def async_ChronosSubmitCRT(__self__, cmds_by_region, req):
        return __self__.__clnt__.async_call(ClassicService.CHRONOSSUBMITCRT, [cmds_by_region, req], ClassicService.__input_type_info__['ChronosSubmitCRT'], ClassicService.__output_type_info__['ChronosSubmitCRT'])

    def async_ChronosStoreLocal(__self__, cmd, req):
        return __self__.__clnt__.async_call(ClassicService.CHRONOSSTORELOCAL, [cmd, req], ClassicService.__input_type_info__['ChronosStoreLocal'], ClassicService.__output_type_info__['ChronosStoreLocal'])

    def async_ChronosProposeRemote(__self__, cmd, req):
        return __self__.__clnt__.async_call(ClassicService.CHRONOSPROPOSEREMOTE, [cmd, req], ClassicService.__input_type_info__['ChronosProposeRemote'], ClassicService.__output_type_info__['ChronosProposeRemote'])

    def async_ChronosStoreRemote(__self__, cmd, req):
        return __self__.__clnt__.async_call(ClassicService.CHRONOSSTOREREMOTE, [cmd, req], ClassicService.__input_type_info__['ChronosStoreRemote'], ClassicService.__output_type_info__['ChronosStoreRemote'])

    def async_ChronosProposeLocal(__self__, cmd, req):
        return __self__.__clnt__.async_call(ClassicService.CHRONOSPROPOSELOCAL, [cmd, req], ClassicService.__input_type_info__['ChronosProposeLocal'], ClassicService.__output_type_info__['ChronosProposeLocal'])

    def async_DastRemotePrepared(__self__, req):
        return __self__.__clnt__.async_call(ClassicService.DASTREMOTEPREPARED, [req], ClassicService.__input_type_info__['DastRemotePrepared'], ClassicService.__output_type_info__['DastRemotePrepared'])

    def async_DastNotiCommit(__self__, req):
        return __self__.__clnt__.async_call(ClassicService.DASTNOTICOMMIT, [req], ClassicService.__input_type_info__['DastNotiCommit'], ClassicService.__output_type_info__['DastNotiCommit'])

    def async_DastIRSync(__self__, req):
        return __self__.__clnt__.async_call(ClassicService.DASTIRSYNC, [req], ClassicService.__input_type_info__['DastIRSync'], ClassicService.__output_type_info__['DastIRSync'])

    def async_DastNotiCRT(__self__, req):
        return __self__.__clnt__.async_call(ClassicService.DASTNOTICRT, [req], ClassicService.__input_type_info__['DastNotiCRT'], ClassicService.__output_type_info__['DastNotiCRT'])

    def async_ChronosDistExe(__self__, chr_req):
        return __self__.__clnt__.async_call(ClassicService.CHRONOSDISTEXE, [chr_req], ClassicService.__input_type_info__['ChronosDistExe'], ClassicService.__output_type_info__['ChronosDistExe'])

    def async_ChronosSendOutput(__self__, chr_req):
        return __self__.__clnt__.async_call(ClassicService.CHRONOSSENDOUTPUT, [chr_req], ClassicService.__input_type_info__['ChronosSendOutput'], ClassicService.__output_type_info__['ChronosSendOutput'])

    def async_OVStore(__self__, txn_id, cmd, ov_req):
        return __self__.__clnt__.async_call(ClassicService.OVSTORE, [txn_id, cmd, ov_req], ClassicService.__input_type_info__['OVStore'], ClassicService.__output_type_info__['OVStore'])

    def async_OVCreateTs(__self__, txn_id):
        return __self__.__clnt__.async_call(ClassicService.OVCREATETS, [txn_id], ClassicService.__input_type_info__['OVCreateTs'], ClassicService.__output_type_info__['OVCreateTs'])

    def async_OVStoredRemoveTs(__self__, txn_id, timestamp, site_id):
        return __self__.__clnt__.async_call(ClassicService.OVSTOREDREMOVETS, [txn_id, timestamp, site_id], ClassicService.__input_type_info__['OVStoredRemoveTs'], ClassicService.__output_type_info__['OVStoredRemoveTs'])

    def async_OVExecute(__self__, id, req):
        return __self__.__clnt__.async_call(ClassicService.OVEXECUTE, [id, req], ClassicService.__input_type_info__['OVExecute'], ClassicService.__output_type_info__['OVExecute'])

    def async_OVPublish(__self__, dc_timestamp, dc_site_id):
        return __self__.__clnt__.async_call(ClassicService.OVPUBLISH, [dc_timestamp, dc_site_id], ClassicService.__input_type_info__['OVPublish'], ClassicService.__output_type_info__['OVPublish'])

    def async_OVExchange(__self__, dcname, dvw_timestamp, dvw_site_id):
        return __self__.__clnt__.async_call(ClassicService.OVEXCHANGE, [dcname, dvw_timestamp, dvw_site_id], ClassicService.__input_type_info__['OVExchange'], ClassicService.__output_type_info__['OVExchange'])

    def async_OVDispatch(__self__, cmd):
        return __self__.__clnt__.async_call(ClassicService.OVDISPATCH, [cmd], ClassicService.__input_type_info__['OVDispatch'], ClassicService.__output_type_info__['OVDispatch'])

    def async_SlogInsertDistributed(__self__, cmd, index, touched_pars, handler_site, txn_id):
        return __self__.__clnt__.async_call(ClassicService.SLOGINSERTDISTRIBUTED, [cmd, index, touched_pars, handler_site, txn_id], ClassicService.__input_type_info__['SlogInsertDistributed'], ClassicService.__output_type_info__['SlogInsertDistributed'])

    def async_SlogReplicateLogLocal(__self__, cmd, txn_id, index, commit_index):
        return __self__.__clnt__.async_call(ClassicService.SLOGREPLICATELOGLOCAL, [cmd, txn_id, index, commit_index], ClassicService.__input_type_info__['SlogReplicateLogLocal'], ClassicService.__output_type_info__['SlogReplicateLogLocal'])

    def async_SlogSubmitDistributed(__self__, cmds_by_par):
        return __self__.__clnt__.async_call(ClassicService.SLOGSUBMITDISTRIBUTED, [cmds_by_par], ClassicService.__input_type_info__['SlogSubmitDistributed'], ClassicService.__output_type_info__['SlogSubmitDistributed'])

    def async_SlogSendBatchRemote(__self__, batch, my_par_id):
        return __self__.__clnt__.async_call(ClassicService.SLOGSENDBATCHREMOTE, [batch, my_par_id], ClassicService.__input_type_info__['SlogSendBatchRemote'], ClassicService.__output_type_info__['SlogSendBatchRemote'])

    def async_SlogSendBatch(__self__, start_index, cmds, coord_sites):
        return __self__.__clnt__.async_call(ClassicService.SLOGSENDBATCH, [start_index, cmds, coord_sites], ClassicService.__input_type_info__['SlogSendBatch'], ClassicService.__output_type_info__['SlogSendBatch'])

    def async_SlogSendTxnOutput(__self__, txn_id, par_id, output):
        return __self__.__clnt__.async_call(ClassicService.SLOGSENDTXNOUTPUT, [txn_id, par_id, output], ClassicService.__input_type_info__['SlogSendTxnOutput'], ClassicService.__output_type_info__['SlogSendTxnOutput'])

    def async_SlogSendDepValues(__self__, chr_req):
        return __self__.__clnt__.async_call(ClassicService.SLOGSENDDEPVALUES, [chr_req], ClassicService.__input_type_info__['SlogSendDepValues'], ClassicService.__output_type_info__['SlogSendDepValues'])

    def async_SlogSubmitTxn(__self__, cmds_by_par, is_irt):
        return __self__.__clnt__.async_call(ClassicService.SLOGSUBMITTXN, [cmds_by_par, is_irt], ClassicService.__input_type_info__['SlogSubmitTxn'], ClassicService.__output_type_info__['SlogSubmitTxn'])

    def sync_Dispatch(__self__, cmd):
        __result__ = __self__.__clnt__.sync_call(ClassicService.DISPATCH, [cmd], ClassicService.__input_type_info__['Dispatch'], ClassicService.__output_type_info__['Dispatch'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_Prepare(__self__, tid, sids):
        __result__ = __self__.__clnt__.sync_call(ClassicService.PREPARE, [tid, sids], ClassicService.__input_type_info__['Prepare'], ClassicService.__output_type_info__['Prepare'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_Commit(__self__, tid):
        __result__ = __self__.__clnt__.sync_call(ClassicService.COMMIT, [tid], ClassicService.__input_type_info__['Commit'], ClassicService.__output_type_info__['Commit'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_Abort(__self__, tid):
        __result__ = __self__.__clnt__.sync_call(ClassicService.ABORT, [tid], ClassicService.__input_type_info__['Abort'], ClassicService.__output_type_info__['Abort'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_UpgradeEpoch(__self__, curr_epoch):
        __result__ = __self__.__clnt__.sync_call(ClassicService.UPGRADEEPOCH, [curr_epoch], ClassicService.__input_type_info__['UpgradeEpoch'], ClassicService.__output_type_info__['UpgradeEpoch'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_TruncateEpoch(__self__, old_epoch):
        __result__ = __self__.__clnt__.sync_call(ClassicService.TRUNCATEEPOCH, [old_epoch], ClassicService.__input_type_info__['TruncateEpoch'], ClassicService.__output_type_info__['TruncateEpoch'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_rpc_null(__self__):
        __result__ = __self__.__clnt__.sync_call(ClassicService.RPC_NULL, [], ClassicService.__input_type_info__['rpc_null'], ClassicService.__output_type_info__['rpc_null'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_TapirAccept(__self__, cmd_id, ballot, decision):
        __result__ = __self__.__clnt__.sync_call(ClassicService.TAPIRACCEPT, [cmd_id, ballot, decision], ClassicService.__input_type_info__['TapirAccept'], ClassicService.__output_type_info__['TapirAccept'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_TapirFastAccept(__self__, cmd_id, txn_cmds):
        __result__ = __self__.__clnt__.sync_call(ClassicService.TAPIRFASTACCEPT, [cmd_id, txn_cmds], ClassicService.__input_type_info__['TapirFastAccept'], ClassicService.__output_type_info__['TapirFastAccept'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_TapirDecide(__self__, cmd_id, commit):
        __result__ = __self__.__clnt__.sync_call(ClassicService.TAPIRDECIDE, [cmd_id, commit], ClassicService.__input_type_info__['TapirDecide'], ClassicService.__output_type_info__['TapirDecide'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_RccDispatch(__self__, cmd):
        __result__ = __self__.__clnt__.sync_call(ClassicService.RCCDISPATCH, [cmd], ClassicService.__input_type_info__['RccDispatch'], ClassicService.__output_type_info__['RccDispatch'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_RccFinish(__self__, id, in1):
        __result__ = __self__.__clnt__.sync_call(ClassicService.RCCFINISH, [id, in1], ClassicService.__input_type_info__['RccFinish'], ClassicService.__output_type_info__['RccFinish'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_RccInquire(__self__, epoch, txn_id):
        __result__ = __self__.__clnt__.sync_call(ClassicService.RCCINQUIRE, [epoch, txn_id], ClassicService.__input_type_info__['RccInquire'], ClassicService.__output_type_info__['RccInquire'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_RccDispatchRo(__self__, cmd):
        __result__ = __self__.__clnt__.sync_call(ClassicService.RCCDISPATCHRO, [cmd], ClassicService.__input_type_info__['RccDispatchRo'], ClassicService.__output_type_info__['RccDispatchRo'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_BrqDispatch(__self__, cmd):
        __result__ = __self__.__clnt__.sync_call(ClassicService.BRQDISPATCH, [cmd], ClassicService.__input_type_info__['BrqDispatch'], ClassicService.__output_type_info__['BrqDispatch'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_BrqCommit(__self__, id, graph):
        __result__ = __self__.__clnt__.sync_call(ClassicService.BRQCOMMIT, [id, graph], ClassicService.__input_type_info__['BrqCommit'], ClassicService.__output_type_info__['BrqCommit'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_BrqCommitWoGraph(__self__, id):
        __result__ = __self__.__clnt__.sync_call(ClassicService.BRQCOMMITWOGRAPH, [id], ClassicService.__input_type_info__['BrqCommitWoGraph'], ClassicService.__output_type_info__['BrqCommitWoGraph'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_BrqInquire(__self__, epoch, txn_id):
        __result__ = __self__.__clnt__.sync_call(ClassicService.BRQINQUIRE, [epoch, txn_id], ClassicService.__input_type_info__['BrqInquire'], ClassicService.__output_type_info__['BrqInquire'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_BrqPreAccept(__self__, txn_id, cmd, graph):
        __result__ = __self__.__clnt__.sync_call(ClassicService.BRQPREACCEPT, [txn_id, cmd, graph], ClassicService.__input_type_info__['BrqPreAccept'], ClassicService.__output_type_info__['BrqPreAccept'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_BrqPreAcceptWoGraph(__self__, txn_id, cmd):
        __result__ = __self__.__clnt__.sync_call(ClassicService.BRQPREACCEPTWOGRAPH, [txn_id, cmd], ClassicService.__input_type_info__['BrqPreAcceptWoGraph'], ClassicService.__output_type_info__['BrqPreAcceptWoGraph'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_BrqAccept(__self__, txn_id, ballot, graph):
        __result__ = __self__.__clnt__.sync_call(ClassicService.BRQACCEPT, [txn_id, ballot, graph], ClassicService.__input_type_info__['BrqAccept'], ClassicService.__output_type_info__['BrqAccept'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_BrqSendOutput(__self__, chr_req):
        __result__ = __self__.__clnt__.sync_call(ClassicService.BRQSENDOUTPUT, [chr_req], ClassicService.__input_type_info__['BrqSendOutput'], ClassicService.__output_type_info__['BrqSendOutput'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_ChronosSubmitLocal(__self__, cmd, req):
        __result__ = __self__.__clnt__.sync_call(ClassicService.CHRONOSSUBMITLOCAL, [cmd, req], ClassicService.__input_type_info__['ChronosSubmitLocal'], ClassicService.__output_type_info__['ChronosSubmitLocal'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_ChronosSubmitTxn(__self__, cmds_by_par, req, is_local):
        __result__ = __self__.__clnt__.sync_call(ClassicService.CHRONOSSUBMITTXN, [cmds_by_par, req, is_local], ClassicService.__input_type_info__['ChronosSubmitTxn'], ClassicService.__output_type_info__['ChronosSubmitTxn'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_DastPrepareCRT(__self__, cmds, req):
        __result__ = __self__.__clnt__.sync_call(ClassicService.DASTPREPARECRT, [cmds, req], ClassicService.__input_type_info__['DastPrepareCRT'], ClassicService.__output_type_info__['DastPrepareCRT'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_ChronosSubmitCRT(__self__, cmds_by_region, req):
        __result__ = __self__.__clnt__.sync_call(ClassicService.CHRONOSSUBMITCRT, [cmds_by_region, req], ClassicService.__input_type_info__['ChronosSubmitCRT'], ClassicService.__output_type_info__['ChronosSubmitCRT'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_ChronosStoreLocal(__self__, cmd, req):
        __result__ = __self__.__clnt__.sync_call(ClassicService.CHRONOSSTORELOCAL, [cmd, req], ClassicService.__input_type_info__['ChronosStoreLocal'], ClassicService.__output_type_info__['ChronosStoreLocal'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_ChronosProposeRemote(__self__, cmd, req):
        __result__ = __self__.__clnt__.sync_call(ClassicService.CHRONOSPROPOSEREMOTE, [cmd, req], ClassicService.__input_type_info__['ChronosProposeRemote'], ClassicService.__output_type_info__['ChronosProposeRemote'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_ChronosStoreRemote(__self__, cmd, req):
        __result__ = __self__.__clnt__.sync_call(ClassicService.CHRONOSSTOREREMOTE, [cmd, req], ClassicService.__input_type_info__['ChronosStoreRemote'], ClassicService.__output_type_info__['ChronosStoreRemote'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_ChronosProposeLocal(__self__, cmd, req):
        __result__ = __self__.__clnt__.sync_call(ClassicService.CHRONOSPROPOSELOCAL, [cmd, req], ClassicService.__input_type_info__['ChronosProposeLocal'], ClassicService.__output_type_info__['ChronosProposeLocal'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_DastRemotePrepared(__self__, req):
        __result__ = __self__.__clnt__.sync_call(ClassicService.DASTREMOTEPREPARED, [req], ClassicService.__input_type_info__['DastRemotePrepared'], ClassicService.__output_type_info__['DastRemotePrepared'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_DastNotiCommit(__self__, req):
        __result__ = __self__.__clnt__.sync_call(ClassicService.DASTNOTICOMMIT, [req], ClassicService.__input_type_info__['DastNotiCommit'], ClassicService.__output_type_info__['DastNotiCommit'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_DastIRSync(__self__, req):
        __result__ = __self__.__clnt__.sync_call(ClassicService.DASTIRSYNC, [req], ClassicService.__input_type_info__['DastIRSync'], ClassicService.__output_type_info__['DastIRSync'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_DastNotiCRT(__self__, req):
        __result__ = __self__.__clnt__.sync_call(ClassicService.DASTNOTICRT, [req], ClassicService.__input_type_info__['DastNotiCRT'], ClassicService.__output_type_info__['DastNotiCRT'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_ChronosDistExe(__self__, chr_req):
        __result__ = __self__.__clnt__.sync_call(ClassicService.CHRONOSDISTEXE, [chr_req], ClassicService.__input_type_info__['ChronosDistExe'], ClassicService.__output_type_info__['ChronosDistExe'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_ChronosSendOutput(__self__, chr_req):
        __result__ = __self__.__clnt__.sync_call(ClassicService.CHRONOSSENDOUTPUT, [chr_req], ClassicService.__input_type_info__['ChronosSendOutput'], ClassicService.__output_type_info__['ChronosSendOutput'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_OVStore(__self__, txn_id, cmd, ov_req):
        __result__ = __self__.__clnt__.sync_call(ClassicService.OVSTORE, [txn_id, cmd, ov_req], ClassicService.__input_type_info__['OVStore'], ClassicService.__output_type_info__['OVStore'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_OVCreateTs(__self__, txn_id):
        __result__ = __self__.__clnt__.sync_call(ClassicService.OVCREATETS, [txn_id], ClassicService.__input_type_info__['OVCreateTs'], ClassicService.__output_type_info__['OVCreateTs'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_OVStoredRemoveTs(__self__, txn_id, timestamp, site_id):
        __result__ = __self__.__clnt__.sync_call(ClassicService.OVSTOREDREMOVETS, [txn_id, timestamp, site_id], ClassicService.__input_type_info__['OVStoredRemoveTs'], ClassicService.__output_type_info__['OVStoredRemoveTs'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_OVExecute(__self__, id, req):
        __result__ = __self__.__clnt__.sync_call(ClassicService.OVEXECUTE, [id, req], ClassicService.__input_type_info__['OVExecute'], ClassicService.__output_type_info__['OVExecute'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_OVPublish(__self__, dc_timestamp, dc_site_id):
        __result__ = __self__.__clnt__.sync_call(ClassicService.OVPUBLISH, [dc_timestamp, dc_site_id], ClassicService.__input_type_info__['OVPublish'], ClassicService.__output_type_info__['OVPublish'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_OVExchange(__self__, dcname, dvw_timestamp, dvw_site_id):
        __result__ = __self__.__clnt__.sync_call(ClassicService.OVEXCHANGE, [dcname, dvw_timestamp, dvw_site_id], ClassicService.__input_type_info__['OVExchange'], ClassicService.__output_type_info__['OVExchange'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_OVDispatch(__self__, cmd):
        __result__ = __self__.__clnt__.sync_call(ClassicService.OVDISPATCH, [cmd], ClassicService.__input_type_info__['OVDispatch'], ClassicService.__output_type_info__['OVDispatch'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_SlogInsertDistributed(__self__, cmd, index, touched_pars, handler_site, txn_id):
        __result__ = __self__.__clnt__.sync_call(ClassicService.SLOGINSERTDISTRIBUTED, [cmd, index, touched_pars, handler_site, txn_id], ClassicService.__input_type_info__['SlogInsertDistributed'], ClassicService.__output_type_info__['SlogInsertDistributed'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_SlogReplicateLogLocal(__self__, cmd, txn_id, index, commit_index):
        __result__ = __self__.__clnt__.sync_call(ClassicService.SLOGREPLICATELOGLOCAL, [cmd, txn_id, index, commit_index], ClassicService.__input_type_info__['SlogReplicateLogLocal'], ClassicService.__output_type_info__['SlogReplicateLogLocal'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_SlogSubmitDistributed(__self__, cmds_by_par):
        __result__ = __self__.__clnt__.sync_call(ClassicService.SLOGSUBMITDISTRIBUTED, [cmds_by_par], ClassicService.__input_type_info__['SlogSubmitDistributed'], ClassicService.__output_type_info__['SlogSubmitDistributed'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_SlogSendBatchRemote(__self__, batch, my_par_id):
        __result__ = __self__.__clnt__.sync_call(ClassicService.SLOGSENDBATCHREMOTE, [batch, my_par_id], ClassicService.__input_type_info__['SlogSendBatchRemote'], ClassicService.__output_type_info__['SlogSendBatchRemote'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_SlogSendBatch(__self__, start_index, cmds, coord_sites):
        __result__ = __self__.__clnt__.sync_call(ClassicService.SLOGSENDBATCH, [start_index, cmds, coord_sites], ClassicService.__input_type_info__['SlogSendBatch'], ClassicService.__output_type_info__['SlogSendBatch'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_SlogSendTxnOutput(__self__, txn_id, par_id, output):
        __result__ = __self__.__clnt__.sync_call(ClassicService.SLOGSENDTXNOUTPUT, [txn_id, par_id, output], ClassicService.__input_type_info__['SlogSendTxnOutput'], ClassicService.__output_type_info__['SlogSendTxnOutput'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_SlogSendDepValues(__self__, chr_req):
        __result__ = __self__.__clnt__.sync_call(ClassicService.SLOGSENDDEPVALUES, [chr_req], ClassicService.__input_type_info__['SlogSendDepValues'], ClassicService.__output_type_info__['SlogSendDepValues'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_SlogSubmitTxn(__self__, cmds_by_par, is_irt):
        __result__ = __self__.__clnt__.sync_call(ClassicService.SLOGSUBMITTXN, [cmds_by_par, is_irt], ClassicService.__input_type_info__['SlogSubmitTxn'], ClassicService.__output_type_info__['SlogSubmitTxn'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

class DastManagerService(object):
    PREPARECRT = 0x105a9a14
    NOTIWAIT = 0x4f135010

    __input_type_info__ = {
        'PrepareCRT': ['std::map<uint32_t, std::vector<SimpleCommand>>','ChronosProposeRemoteReq'],
        'NotiWait': ['DastNotiManagerWaitReq'],
    }

    __output_type_info__ = {
        'PrepareCRT': ['ChronosProposeRemoteRes'],
        'NotiWait': ['rrr::i32'],
    }

    def __bind_helper__(self, func):
        def f(*args):
            return getattr(self, func.__name__)(*args)
        return f

    def __reg_to__(self, server):
        server.__reg_func__(DastManagerService.PREPARECRT, self.__bind_helper__(self.PrepareCRT), ['std::map<uint32_t, std::vector<SimpleCommand>>','ChronosProposeRemoteReq'], ['ChronosProposeRemoteRes'])
        server.__reg_func__(DastManagerService.NOTIWAIT, self.__bind_helper__(self.NotiWait), ['DastNotiManagerWaitReq'], ['rrr::i32'])

    def PrepareCRT(__self__, cmds_by_par, req):
        raise NotImplementedError('subclass DastManagerService and implement your own PrepareCRT function')

    def NotiWait(__self__, req):
        raise NotImplementedError('subclass DastManagerService and implement your own NotiWait function')

class DastManagerProxy(object):
    def __init__(self, clnt):
        self.__clnt__ = clnt

    def async_PrepareCRT(__self__, cmds_by_par, req):
        return __self__.__clnt__.async_call(DastManagerService.PREPARECRT, [cmds_by_par, req], DastManagerService.__input_type_info__['PrepareCRT'], DastManagerService.__output_type_info__['PrepareCRT'])

    def async_NotiWait(__self__, req):
        return __self__.__clnt__.async_call(DastManagerService.NOTIWAIT, [req], DastManagerService.__input_type_info__['NotiWait'], DastManagerService.__output_type_info__['NotiWait'])

    def sync_PrepareCRT(__self__, cmds_by_par, req):
        __result__ = __self__.__clnt__.sync_call(DastManagerService.PREPARECRT, [cmds_by_par, req], DastManagerService.__input_type_info__['PrepareCRT'], DastManagerService.__output_type_info__['PrepareCRT'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_NotiWait(__self__, req):
        __result__ = __self__.__clnt__.sync_call(DastManagerService.NOTIWAIT, [req], DastManagerService.__input_type_info__['NotiWait'], DastManagerService.__output_type_info__['NotiWait'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

class SlogRaftService(object):
    SLOGRAFTSUBMIT = 0x1e962b9a
    RAFTAPPENDENTRIES = 0x46b8e443

    __input_type_info__ = {
        'SlogRaftSubmit': ['std::map<uint32_t, std::vector<SimpleCommand>>','siteid_t'],
        'RaftAppendEntries': ['std::map<uint32_t, std::vector<SimpleCommand>>'],
    }

    __output_type_info__ = {
        'SlogRaftSubmit': ['rrr::i32'],
        'RaftAppendEntries': ['rrr::i32'],
    }

    def __bind_helper__(self, func):
        def f(*args):
            return getattr(self, func.__name__)(*args)
        return f

    def __reg_to__(self, server):
        server.__reg_func__(SlogRaftService.SLOGRAFTSUBMIT, self.__bind_helper__(self.SlogRaftSubmit), ['std::map<uint32_t, std::vector<SimpleCommand>>','siteid_t'], ['rrr::i32'])
        server.__reg_func__(SlogRaftService.RAFTAPPENDENTRIES, self.__bind_helper__(self.RaftAppendEntries), ['std::map<uint32_t, std::vector<SimpleCommand>>'], ['rrr::i32'])

    def SlogRaftSubmit(__self__, cmds_by_par, handler_site):
        raise NotImplementedError('subclass SlogRaftService and implement your own SlogRaftSubmit function')

    def RaftAppendEntries(__self__, cmds_by_par):
        raise NotImplementedError('subclass SlogRaftService and implement your own RaftAppendEntries function')

class SlogRaftProxy(object):
    def __init__(self, clnt):
        self.__clnt__ = clnt

    def async_SlogRaftSubmit(__self__, cmds_by_par, handler_site):
        return __self__.__clnt__.async_call(SlogRaftService.SLOGRAFTSUBMIT, [cmds_by_par, handler_site], SlogRaftService.__input_type_info__['SlogRaftSubmit'], SlogRaftService.__output_type_info__['SlogRaftSubmit'])

    def async_RaftAppendEntries(__self__, cmds_by_par):
        return __self__.__clnt__.async_call(SlogRaftService.RAFTAPPENDENTRIES, [cmds_by_par], SlogRaftService.__input_type_info__['RaftAppendEntries'], SlogRaftService.__output_type_info__['RaftAppendEntries'])

    def sync_SlogRaftSubmit(__self__, cmds_by_par, handler_site):
        __result__ = __self__.__clnt__.sync_call(SlogRaftService.SLOGRAFTSUBMIT, [cmds_by_par, handler_site], SlogRaftService.__input_type_info__['SlogRaftSubmit'], SlogRaftService.__output_type_info__['SlogRaftSubmit'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_RaftAppendEntries(__self__, cmds_by_par):
        __result__ = __self__.__clnt__.sync_call(SlogRaftService.RAFTAPPENDENTRIES, [cmds_by_par], SlogRaftService.__input_type_info__['RaftAppendEntries'], SlogRaftService.__output_type_info__['RaftAppendEntries'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

class SlogRegionMangerService(object):
    SENDRMIRT = 0x1782dac1
    RMREPLICATE = 0x45770a0e
    SLOGSENDORDEREDCRT = 0x524b6d86

    __input_type_info__ = {
        'SendRmIRT': ['std::map<uint32_t, std::vector<SimpleCommand>>','siteid_t'],
        'RmReplicate': ['std::map<uint32_t, std::vector<SimpleCommand>>','siteid_t'],
        'SlogSendOrderedCRT': ['std::map<parid_t, std::vector<SimpleCommand>>','uint64_t','siteid_t'],
    }

    __output_type_info__ = {
        'SendRmIRT': ['rrr::i32'],
        'RmReplicate': ['rrr::i32'],
        'SlogSendOrderedCRT': ['rrr::i32'],
    }

    def __bind_helper__(self, func):
        def f(*args):
            return getattr(self, func.__name__)(*args)
        return f

    def __reg_to__(self, server):
        server.__reg_func__(SlogRegionMangerService.SENDRMIRT, self.__bind_helper__(self.SendRmIRT), ['std::map<uint32_t, std::vector<SimpleCommand>>','siteid_t'], ['rrr::i32'])
        server.__reg_func__(SlogRegionMangerService.RMREPLICATE, self.__bind_helper__(self.RmReplicate), ['std::map<uint32_t, std::vector<SimpleCommand>>','siteid_t'], ['rrr::i32'])
        server.__reg_func__(SlogRegionMangerService.SLOGSENDORDEREDCRT, self.__bind_helper__(self.SlogSendOrderedCRT), ['std::map<parid_t, std::vector<SimpleCommand>>','uint64_t','siteid_t'], ['rrr::i32'])

    def SendRmIRT(__self__, cmds_by_par, handler_site):
        raise NotImplementedError('subclass SlogRegionMangerService and implement your own SendRmIRT function')

    def RmReplicate(__self__, cmds_by_par, hander_site):
        raise NotImplementedError('subclass SlogRegionMangerService and implement your own RmReplicate function')

    def SlogSendOrderedCRT(__self__, cmds_by_par, index, handler_site):
        raise NotImplementedError('subclass SlogRegionMangerService and implement your own SlogSendOrderedCRT function')

class SlogRegionMangerProxy(object):
    def __init__(self, clnt):
        self.__clnt__ = clnt

    def async_SendRmIRT(__self__, cmds_by_par, handler_site):
        return __self__.__clnt__.async_call(SlogRegionMangerService.SENDRMIRT, [cmds_by_par, handler_site], SlogRegionMangerService.__input_type_info__['SendRmIRT'], SlogRegionMangerService.__output_type_info__['SendRmIRT'])

    def async_RmReplicate(__self__, cmds_by_par, hander_site):
        return __self__.__clnt__.async_call(SlogRegionMangerService.RMREPLICATE, [cmds_by_par, hander_site], SlogRegionMangerService.__input_type_info__['RmReplicate'], SlogRegionMangerService.__output_type_info__['RmReplicate'])

    def async_SlogSendOrderedCRT(__self__, cmds_by_par, index, handler_site):
        return __self__.__clnt__.async_call(SlogRegionMangerService.SLOGSENDORDEREDCRT, [cmds_by_par, index, handler_site], SlogRegionMangerService.__input_type_info__['SlogSendOrderedCRT'], SlogRegionMangerService.__output_type_info__['SlogSendOrderedCRT'])

    def sync_SendRmIRT(__self__, cmds_by_par, handler_site):
        __result__ = __self__.__clnt__.sync_call(SlogRegionMangerService.SENDRMIRT, [cmds_by_par, handler_site], SlogRegionMangerService.__input_type_info__['SendRmIRT'], SlogRegionMangerService.__output_type_info__['SendRmIRT'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_RmReplicate(__self__, cmds_by_par, hander_site):
        __result__ = __self__.__clnt__.sync_call(SlogRegionMangerService.RMREPLICATE, [cmds_by_par, hander_site], SlogRegionMangerService.__input_type_info__['RmReplicate'], SlogRegionMangerService.__output_type_info__['RmReplicate'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_SlogSendOrderedCRT(__self__, cmds_by_par, index, handler_site):
        __result__ = __self__.__clnt__.sync_call(SlogRegionMangerService.SLOGSENDORDEREDCRT, [cmds_by_par, index, handler_site], SlogRegionMangerService.__input_type_info__['SlogSendOrderedCRT'], SlogRegionMangerService.__output_type_info__['SlogSendOrderedCRT'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

class ServerControlService(object):
    SERVER_SHUTDOWN = 0x2ea3e183
    SERVER_READY = 0x68433c6b
    SERVER_HEART_BEAT_WITH_DATA = 0x2900dc25
    SERVER_HEART_BEAT = 0x62c565da

    __input_type_info__ = {
        'server_shutdown': [],
        'server_ready': [],
        'server_heart_beat_with_data': [],
        'server_heart_beat': [],
    }

    __output_type_info__ = {
        'server_shutdown': [],
        'server_ready': ['rrr::i32'],
        'server_heart_beat_with_data': ['ServerResponse'],
        'server_heart_beat': [],
    }

    def __bind_helper__(self, func):
        def f(*args):
            return getattr(self, func.__name__)(*args)
        return f

    def __reg_to__(self, server):
        server.__reg_func__(ServerControlService.SERVER_SHUTDOWN, self.__bind_helper__(self.server_shutdown), [], [])
        server.__reg_func__(ServerControlService.SERVER_READY, self.__bind_helper__(self.server_ready), [], ['rrr::i32'])
        server.__reg_func__(ServerControlService.SERVER_HEART_BEAT_WITH_DATA, self.__bind_helper__(self.server_heart_beat_with_data), [], ['ServerResponse'])
        server.__reg_func__(ServerControlService.SERVER_HEART_BEAT, self.__bind_helper__(self.server_heart_beat), [], [])

    def server_shutdown(__self__):
        raise NotImplementedError('subclass ServerControlService and implement your own server_shutdown function')

    def server_ready(__self__):
        raise NotImplementedError('subclass ServerControlService and implement your own server_ready function')

    def server_heart_beat_with_data(__self__):
        raise NotImplementedError('subclass ServerControlService and implement your own server_heart_beat_with_data function')

    def server_heart_beat(__self__):
        raise NotImplementedError('subclass ServerControlService and implement your own server_heart_beat function')

class ServerControlProxy(object):
    def __init__(self, clnt):
        self.__clnt__ = clnt

    def async_server_shutdown(__self__):
        return __self__.__clnt__.async_call(ServerControlService.SERVER_SHUTDOWN, [], ServerControlService.__input_type_info__['server_shutdown'], ServerControlService.__output_type_info__['server_shutdown'])

    def async_server_ready(__self__):
        return __self__.__clnt__.async_call(ServerControlService.SERVER_READY, [], ServerControlService.__input_type_info__['server_ready'], ServerControlService.__output_type_info__['server_ready'])

    def async_server_heart_beat_with_data(__self__):
        return __self__.__clnt__.async_call(ServerControlService.SERVER_HEART_BEAT_WITH_DATA, [], ServerControlService.__input_type_info__['server_heart_beat_with_data'], ServerControlService.__output_type_info__['server_heart_beat_with_data'])

    def async_server_heart_beat(__self__):
        return __self__.__clnt__.async_call(ServerControlService.SERVER_HEART_BEAT, [], ServerControlService.__input_type_info__['server_heart_beat'], ServerControlService.__output_type_info__['server_heart_beat'])

    def sync_server_shutdown(__self__):
        __result__ = __self__.__clnt__.sync_call(ServerControlService.SERVER_SHUTDOWN, [], ServerControlService.__input_type_info__['server_shutdown'], ServerControlService.__output_type_info__['server_shutdown'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_server_ready(__self__):
        __result__ = __self__.__clnt__.sync_call(ServerControlService.SERVER_READY, [], ServerControlService.__input_type_info__['server_ready'], ServerControlService.__output_type_info__['server_ready'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_server_heart_beat_with_data(__self__):
        __result__ = __self__.__clnt__.sync_call(ServerControlService.SERVER_HEART_BEAT_WITH_DATA, [], ServerControlService.__input_type_info__['server_heart_beat_with_data'], ServerControlService.__output_type_info__['server_heart_beat_with_data'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_server_heart_beat(__self__):
        __result__ = __self__.__clnt__.sync_call(ServerControlService.SERVER_HEART_BEAT, [], ServerControlService.__input_type_info__['server_heart_beat'], ServerControlService.__output_type_info__['server_heart_beat'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

class ClientControlService(object):
    CLIENT_GET_TXN_NAMES = 0x42b12ebf
    CLIENT_SHUTDOWN = 0x30b5d0cd
    CLIENT_FORCE_STOP = 0x618c36f7
    CLIENT_RESPONSE = 0x209a0657
    CLIENT_READY = 0x65576788
    CLIENT_READY_BLOCK = 0x61f1fe20
    CLIENT_START = 0x40ad6717
    DISPATCHTXN = 0x2d284f56

    __input_type_info__ = {
        'client_get_txn_names': [],
        'client_shutdown': [],
        'client_force_stop': [],
        'client_response': [],
        'client_ready': [],
        'client_ready_block': [],
        'client_start': [],
        'DispatchTxn': ['TxnDispatchRequest'],
    }

    __output_type_info__ = {
        'client_get_txn_names': ['std::map<rrr::i32, std::string>'],
        'client_shutdown': [],
        'client_force_stop': [],
        'client_response': ['ClientResponse'],
        'client_ready': ['rrr::i32'],
        'client_ready_block': ['rrr::i32'],
        'client_start': [],
        'DispatchTxn': ['TxnReply'],
    }

    def __bind_helper__(self, func):
        def f(*args):
            return getattr(self, func.__name__)(*args)
        return f

    def __reg_to__(self, server):
        server.__reg_func__(ClientControlService.CLIENT_GET_TXN_NAMES, self.__bind_helper__(self.client_get_txn_names), [], ['std::map<rrr::i32, std::string>'])
        server.__reg_func__(ClientControlService.CLIENT_SHUTDOWN, self.__bind_helper__(self.client_shutdown), [], [])
        server.__reg_func__(ClientControlService.CLIENT_FORCE_STOP, self.__bind_helper__(self.client_force_stop), [], [])
        server.__reg_func__(ClientControlService.CLIENT_RESPONSE, self.__bind_helper__(self.client_response), [], ['ClientResponse'])
        server.__reg_func__(ClientControlService.CLIENT_READY, self.__bind_helper__(self.client_ready), [], ['rrr::i32'])
        server.__reg_func__(ClientControlService.CLIENT_READY_BLOCK, self.__bind_helper__(self.client_ready_block), [], ['rrr::i32'])
        server.__reg_func__(ClientControlService.CLIENT_START, self.__bind_helper__(self.client_start), [], [])
        server.__reg_func__(ClientControlService.DISPATCHTXN, self.__bind_helper__(self.DispatchTxn), ['TxnDispatchRequest'], ['TxnReply'])

    def client_get_txn_names(__self__):
        raise NotImplementedError('subclass ClientControlService and implement your own client_get_txn_names function')

    def client_shutdown(__self__):
        raise NotImplementedError('subclass ClientControlService and implement your own client_shutdown function')

    def client_force_stop(__self__):
        raise NotImplementedError('subclass ClientControlService and implement your own client_force_stop function')

    def client_response(__self__):
        raise NotImplementedError('subclass ClientControlService and implement your own client_response function')

    def client_ready(__self__):
        raise NotImplementedError('subclass ClientControlService and implement your own client_ready function')

    def client_ready_block(__self__):
        raise NotImplementedError('subclass ClientControlService and implement your own client_ready_block function')

    def client_start(__self__):
        raise NotImplementedError('subclass ClientControlService and implement your own client_start function')

    def DispatchTxn(__self__, req):
        raise NotImplementedError('subclass ClientControlService and implement your own DispatchTxn function')

class ClientControlProxy(object):
    def __init__(self, clnt):
        self.__clnt__ = clnt

    def async_client_get_txn_names(__self__):
        return __self__.__clnt__.async_call(ClientControlService.CLIENT_GET_TXN_NAMES, [], ClientControlService.__input_type_info__['client_get_txn_names'], ClientControlService.__output_type_info__['client_get_txn_names'])

    def async_client_shutdown(__self__):
        return __self__.__clnt__.async_call(ClientControlService.CLIENT_SHUTDOWN, [], ClientControlService.__input_type_info__['client_shutdown'], ClientControlService.__output_type_info__['client_shutdown'])

    def async_client_force_stop(__self__):
        return __self__.__clnt__.async_call(ClientControlService.CLIENT_FORCE_STOP, [], ClientControlService.__input_type_info__['client_force_stop'], ClientControlService.__output_type_info__['client_force_stop'])

    def async_client_response(__self__):
        return __self__.__clnt__.async_call(ClientControlService.CLIENT_RESPONSE, [], ClientControlService.__input_type_info__['client_response'], ClientControlService.__output_type_info__['client_response'])

    def async_client_ready(__self__):
        return __self__.__clnt__.async_call(ClientControlService.CLIENT_READY, [], ClientControlService.__input_type_info__['client_ready'], ClientControlService.__output_type_info__['client_ready'])

    def async_client_ready_block(__self__):
        return __self__.__clnt__.async_call(ClientControlService.CLIENT_READY_BLOCK, [], ClientControlService.__input_type_info__['client_ready_block'], ClientControlService.__output_type_info__['client_ready_block'])

    def async_client_start(__self__):
        return __self__.__clnt__.async_call(ClientControlService.CLIENT_START, [], ClientControlService.__input_type_info__['client_start'], ClientControlService.__output_type_info__['client_start'])

    def async_DispatchTxn(__self__, req):
        return __self__.__clnt__.async_call(ClientControlService.DISPATCHTXN, [req], ClientControlService.__input_type_info__['DispatchTxn'], ClientControlService.__output_type_info__['DispatchTxn'])

    def sync_client_get_txn_names(__self__):
        __result__ = __self__.__clnt__.sync_call(ClientControlService.CLIENT_GET_TXN_NAMES, [], ClientControlService.__input_type_info__['client_get_txn_names'], ClientControlService.__output_type_info__['client_get_txn_names'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_client_shutdown(__self__):
        __result__ = __self__.__clnt__.sync_call(ClientControlService.CLIENT_SHUTDOWN, [], ClientControlService.__input_type_info__['client_shutdown'], ClientControlService.__output_type_info__['client_shutdown'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_client_force_stop(__self__):
        __result__ = __self__.__clnt__.sync_call(ClientControlService.CLIENT_FORCE_STOP, [], ClientControlService.__input_type_info__['client_force_stop'], ClientControlService.__output_type_info__['client_force_stop'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_client_response(__self__):
        __result__ = __self__.__clnt__.sync_call(ClientControlService.CLIENT_RESPONSE, [], ClientControlService.__input_type_info__['client_response'], ClientControlService.__output_type_info__['client_response'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_client_ready(__self__):
        __result__ = __self__.__clnt__.sync_call(ClientControlService.CLIENT_READY, [], ClientControlService.__input_type_info__['client_ready'], ClientControlService.__output_type_info__['client_ready'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_client_ready_block(__self__):
        __result__ = __self__.__clnt__.sync_call(ClientControlService.CLIENT_READY_BLOCK, [], ClientControlService.__input_type_info__['client_ready_block'], ClientControlService.__output_type_info__['client_ready_block'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_client_start(__self__):
        __result__ = __self__.__clnt__.sync_call(ClientControlService.CLIENT_START, [], ClientControlService.__input_type_info__['client_start'], ClientControlService.__output_type_info__['client_start'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_DispatchTxn(__self__, req):
        __result__ = __self__.__clnt__.sync_call(ClientControlService.DISPATCHTXN, [req], ClientControlService.__input_type_info__['DispatchTxn'], ClientControlService.__output_type_info__['DispatchTxn'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]


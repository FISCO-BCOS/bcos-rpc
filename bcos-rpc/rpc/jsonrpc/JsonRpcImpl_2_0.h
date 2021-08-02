/**
 *  Copyright (C) 2021 FISCO BCOS.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @brief: implement for the RPC
 * @file: JsonRpcImpl_2_0.h
 * @author: octopus
 * @date: 2021-07-09
 */

#pragma once
#include <bcos-framework/interfaces/consensus/ConsensusInterface.h>
#include <bcos-framework/interfaces/executor/ExecutorInterface.h>
#include <bcos-framework/interfaces/gateway/GatewayInterface.h>
#include <bcos-framework/interfaces/ledger/LedgerInterface.h>
#include <bcos-framework/interfaces/sync/BlockSyncInterface.h>
#include <bcos-framework/interfaces/txpool/TxPoolInterface.h>
#include <bcos-rpc/rpc/jsonrpc/JsonRpcInterface.h>
#include <json/json.h>
#include <boost/core/ignore_unused.hpp>
#include <unordered_map>

namespace bcos
{
namespace rpc
{
class JsonRpcImpl_2_0 : public JsonRpcInterface,
                        public std::enable_shared_from_this<JsonRpcImpl_2_0>
{
public:
    using Ptr = std::shared_ptr<JsonRpcImpl_2_0>;

public:
    JsonRpcImpl_2_0() { initMethod(); }
    ~JsonRpcImpl_2_0() {}

    void initMethod();

public:
    static std::string encodeData(bcos::bytesConstRef _data);
    static std::shared_ptr<bcos::bytes> decodeData(const std::string& _data);
    static void parseRpcRequestJson(const std::string& _requestBody, JsonRequest& _jsonRequest);
    static void parseRpcResponseJson(const std::string& _responseBody, JsonResponse& _jsonResponse);
    static Json::Value toJsonResponse(const JsonResponse& _jsonResponse);
    static std::string toStringResponse(const JsonResponse& _jsonResponse);
    static void toJsonResp(
        Json::Value& jResp, bcos::protocol::Transaction::ConstPtr _transactionPtr);

    static void toJsonResp(Json::Value& jResp, bcos::protocol::BlockHeader::Ptr _blockHeaderPtr);
    static void toJsonResp(
        Json::Value& jResp, bcos::protocol::Block::Ptr _blockPtr, bool _onlyTxHash);
    static void toJsonResp(Json::Value& jResp, const std::string& _txHash,
        bcos::protocol::TransactionReceipt::ConstPtr _transactionReceiptPtr);
    static void addProofToResponse(
        Json::Value& jResp, std::string const& _key, ledger::MerkleProofPtr _merkleProofPtr);

    virtual void onRPCRequest(const std::string& _requestBody, Sender _sender) override;

public:
    virtual void call(
        const std::string& _to, const std::string& _data, RespFunc _respFunc) override;

    virtual void sendTransaction(
        const std::string& _data, bool _requireProof, RespFunc _respFunc) override;

    virtual void getTransaction(
        const std::string& _txHash, bool _requireProof, RespFunc _respFunc) override;

    virtual void getTransactionReceipt(
        const std::string& _txHash, bool _requireProof, RespFunc _respFunc) override;

    virtual void getBlockByHash(const std::string& _blockHash, bool _onlyHeader, bool _onlyTxHash,
        RespFunc _respFunc) override;

    virtual void getBlockByNumber(
        int64_t _blockNumber, bool _onlyHeader, bool _onlyTxHash, RespFunc _respFunc) override;

    virtual void getBlockHashByNumber(int64_t _blockNumber, RespFunc _respFunc) override;

    virtual void getBlockNumber(RespFunc _respFunc) override;

    virtual void getCode(const std::string _contractAddress, RespFunc _respFunc) override;

    virtual void getSealerList(RespFunc _respFunc) override;

    virtual void getObserverList(RespFunc _respFunc) override;

    virtual void getPbftView(RespFunc _respFunc) override;

    virtual void getPendingTxSize(RespFunc _respFunc) override;

    virtual void getSyncStatus(RespFunc _respFunc) override;

    virtual void getSystemConfigByKey(const std::string& _keyValue, RespFunc _respFunc) override;

    virtual void getTotalTransactionCount(RespFunc _respFunc) override;

    virtual void getPeers(RespFunc _respFunc) override;

    virtual void getNodeInfo(RespFunc _respFunc) override;

public:
    void callI(const Json::Value& req, RespFunc _respFunc)
    {
        call(req[0u].asString(), req[1u].asString(), _respFunc);
    }

    void sendTransactionI(const Json::Value& req, RespFunc _respFunc)
    {
        sendTransaction(req[0u].asString(), req[1u].asBool(), _respFunc);
    }

    void getTransactionI(const Json::Value& req, RespFunc _respFunc)
    {
        getTransaction(req[0u].asString(), req[1u].asBool(), _respFunc);
    }

    void getTransactionReceiptI(const Json::Value& req, RespFunc _respFunc)
    {
        getTransactionReceipt(req[0u].asString(), req[1u].asBool(), _respFunc);
    }

    void getBlockByHashI(const Json::Value& req, RespFunc _respFunc)
    {
        getBlockByHash(req[0u].asString(), (req.size() > 1 ? req[1u].asBool() : true),
            (req.size() > 2 ? req[2u].asBool() : true), _respFunc);
    }

    void getBlockByNumberI(const Json::Value& req, RespFunc _respFunc)
    {
        getBlockByNumber(req[0u].asInt64(), (req.size() > 1 ? req[1u].asBool() : true),
            (req.size() > 2 ? req[2u].asBool() : true), _respFunc);
    }

    void getBlockHashByNumberI(const Json::Value& req, RespFunc _respFunc)
    {
        getBlockHashByNumber(req[0u].asInt64(), _respFunc);
    }

    void getBlockNumberI(const Json::Value& req, RespFunc _respFunc)
    {
        boost::ignore_unused(req);
        getBlockNumber(_respFunc);
    }

    void getCodeI(const Json::Value& req, RespFunc _respFunc)
    {
        getCode(req[0u].asString(), _respFunc);
    }

    void getSealerListI(const Json::Value& req, RespFunc _respFunc)
    {
        boost::ignore_unused(req);
        getSealerList(_respFunc);
    }

    void getObserverListI(const Json::Value& req, RespFunc _respFunc)
    {
        boost::ignore_unused(req);
        getObserverList(_respFunc);
    }

    void getPbftViewI(const Json::Value& req, RespFunc _respFunc)
    {
        boost::ignore_unused(req);
        getPbftView(_respFunc);
    }

    void getPendingTxSizeI(const Json::Value& req, RespFunc _respFunc)
    {
        boost::ignore_unused(req);
        getPendingTxSize(_respFunc);
    }

    void getSyncStatusI(const Json::Value& req, RespFunc _respFunc)
    {
        boost::ignore_unused(req);
        getSyncStatus(_respFunc);
    }

    void getSystemConfigByKeyI(const Json::Value& req, RespFunc _respFunc)
    {
        getSystemConfigByKey(req[0u].asString(), _respFunc);
    }

    void getTotalTransactionCountI(const Json::Value& req, RespFunc _respFunc)
    {
        boost::ignore_unused(req);
        getTotalTransactionCount(_respFunc);
    }

    void getPeersI(const Json::Value& req, RespFunc _respFunc)
    {
        boost::ignore_unused(req);
        getPeers(_respFunc);
    }

    void getNodeInfoI(const Json::Value& req, RespFunc _respFunc)
    {
        boost::ignore_unused(req);
        getNodeInfo(_respFunc);
    }

public:
    const std::unordered_map<std::string, std::function<void(Json::Value, RespFunc _respFunc)>>&
    methodToFunc() const
    {
        return m_methodToFunc;
    }

    void registerMethod(
        const std::string& _method, std::function<void(Json::Value, RespFunc _respFunc)> _callback)
    {
        m_methodToFunc[_method] = _callback;
    }

    bcos::ledger::LedgerInterface::Ptr ledger() const { return m_ledgerInterface; }
    void setLedger(bcos::ledger::LedgerInterface::Ptr _ledgerInterface)
    {
        m_ledgerInterface = _ledgerInterface;
    }

    std::shared_ptr<bcos::executor::ExecutorInterface> executorInterface() const
    {
        return m_executorInterface;
    }
    void setExecutorInterface(std::shared_ptr<bcos::executor::ExecutorInterface> _executorInterface)
    {
        m_executorInterface = _executorInterface;
    }

    bcos::txpool::TxPoolInterface::Ptr txPoolInterface() const { return m_txPoolInterface; }
    void setTxPoolInterface(bcos::txpool::TxPoolInterface::Ptr _txPoolInterface)
    {
        m_txPoolInterface = _txPoolInterface;
    }

    bcos::consensus::ConsensusInterface::Ptr consensusInterface() const
    {
        return m_consensusInterface;
    }
    void setConsensusInterface(bcos::consensus::ConsensusInterface::Ptr _consensusInterface)
    {
        m_consensusInterface = _consensusInterface;
    }

    bcos::sync::BlockSyncInterface::Ptr blockSyncInterface() const { return m_blockSyncInterface; }
    void setBlockSyncInterface(bcos::sync::BlockSyncInterface::Ptr _blockSyncInterface)
    {
        m_blockSyncInterface = _blockSyncInterface;
    }

    bcos::gateway::GatewayInterface::Ptr gatewayInterface() const { return m_gatewayInterface; }
    void setGatewayInterface(bcos::gateway::GatewayInterface::Ptr _gatewayInterface)
    {
        m_gatewayInterface = _gatewayInterface;
    }

    bcos::protocol::TransactionFactory::Ptr transactionFactory() const
    {
        return m_transactionFactory;
    }

    void setTransactionFactory(bcos::protocol::TransactionFactory::Ptr _transactionFactory)
    {
        m_transactionFactory = _transactionFactory;
    }

    void setNodeInfo(const NodeInfo& _nodeInfo) { m_nodeInfo = _nodeInfo; }
    NodeInfo nodeInfo() const { return m_nodeInfo; }

private:
    std::unordered_map<std::string, std::function<void(Json::Value, RespFunc _respFunc)>>
        m_methodToFunc;

    bcos::ledger::LedgerInterface::Ptr m_ledgerInterface;
    std::shared_ptr<bcos::executor::ExecutorInterface> m_executorInterface;
    bcos::txpool::TxPoolInterface::Ptr m_txPoolInterface;
    bcos::consensus::ConsensusInterface::Ptr m_consensusInterface;
    bcos::sync::BlockSyncInterface::Ptr m_blockSyncInterface;
    bcos::gateway::GatewayInterface::Ptr m_gatewayInterface;
    bcos::protocol::TransactionFactory::Ptr m_transactionFactory;
    NodeInfo m_nodeInfo;
};

}  // namespace rpc
}  // namespace bcos
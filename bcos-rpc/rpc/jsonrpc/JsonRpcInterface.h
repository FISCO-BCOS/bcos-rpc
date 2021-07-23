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
 * @brief interface for JsonRPC
 * @file JsonRpcInterface.h
 * @author: octopus
 * @date 2021-07-09
 */

#pragma once

#include <bcos-framework/interfaces/protocol/CommonError.h>
#include <bcos-framework/libutilities/Error.h>
#include <bcos-rpc/rpc/jsonrpc/Common.h>
#include <json/json.h>
#include <functional>

namespace bcos
{
namespace rpc
{
using Sender = std::function<void(const std::string&)>;
using RespFunc = std::function<void(bcos::Error::Ptr, Json::Value&)>;

class JsonRpcInterface
{
public:
    using Ptr = std::shared_ptr<JsonRpcInterface>;
    virtual ~JsonRpcInterface() {}

public:
    virtual void onRPCRequest(const std::string& _requestBody, Sender _sender) = 0;

public:
    virtual void call(const std::string& _to, const std::string& _data, RespFunc _respFunc) = 0;

    virtual void sendTransaction(
        const std::string& _data, bool _requireProof, RespFunc _respFunc) = 0;

    virtual void getTransaction(
        const std::string& _txHash, bool _requireProof, RespFunc _respFunc) = 0;

    virtual void getTransactionReceipt(
        const std::string& _txHash, bool _requireProof, RespFunc _respFunc) = 0;

    virtual void getBlockByHash(
        const std::string& _blockHash, bool _onlyHeader, bool _onlyTxHash, RespFunc _respFunc) = 0;

    virtual void getBlockByNumber(
        int64_t _blockNumber, bool _onlyHeader, bool _onlyTxHash, RespFunc _respFunc) = 0;

    virtual void getBlockHashByNumber(int64_t _blockNumber, RespFunc _respFunc) = 0;

    virtual void getBlockNumber(RespFunc _respFunc) = 0;

    virtual void getCode(const std::string _contractAddress, RespFunc _respFunc) = 0;

    virtual void getSealerList(RespFunc _respFunc) = 0;

    virtual void getObserverList(RespFunc _respFunc) = 0;

    virtual void getPbftView(RespFunc _respFunc) = 0;

    virtual void getPendingTxSize(RespFunc _respFunc) = 0;

    virtual void getSyncStatus(RespFunc _respFunc) = 0;

    virtual void getSystemConfigByKey(const std::string& _keyValue, RespFunc _respFunc) = 0;

    virtual void getTotalTransactionCount(RespFunc _respFunc) = 0;

    virtual void getPeers(RespFunc _respFunc) = 0;

    virtual void getNodeInfo(RespFunc _respFunc) = 0;
};

}  // namespace rpc
}  // namespace bcos

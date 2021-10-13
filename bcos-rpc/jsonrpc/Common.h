/*
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
 * @file Common.h
 * @author: octopus
 * @date 2021-07-02
 */
#pragma once

#include <bcos-framework/interfaces/multigroup/GroupInfo.h>
#include <json/json.h>
#include <exception>

#define RPC_IMPL_LOG(LEVEL) BCOS_LOG(LEVEL) << "[RPC][JSONRPC]"

namespace bcos
{
namespace rpc
{
enum MessageType
{
    HANDESHAKE = 0x100,    // 256
    BLOCK_NOTIFY = 0x101,  // 257
    RPC_REQUEST = 0x102,   // 258
    GROUP_NOTIFY = 0x103,  // 259
};
struct NodeInfo
{
    std::string version;
    std::string supportedVersion;
    std::string nodeID;
    std::string chainID;
    std::string groupID;
    std::string agency;
    std::string buildTime;
    std::string gitCommitHash;
    bool isWasm;
    bool isSM;
};

class JsonRpcException : public std::exception
{
public:
    JsonRpcException(int32_t _code, std::string const& _msg) : m_code(_code), m_msg(_msg) {}
    virtual const char* what() const noexcept override { return m_msg.c_str(); }

public:
    int32_t code() const noexcept { return m_code; }
    std::string msg() const noexcept { return m_msg; }

private:
    int32_t m_code;
    std::string m_msg;
};

enum JsonRpcError : int32_t
{
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    // -32000 to -32099: Server error	Reserved for implementation-defined server-errors.
    NodeNotExistOrNotStarted = -32000,
    GroupAlreadyExists = -32001,
    NodeAlreadyExists = -32002,
    OperationNotAllowed = -32003,
};

struct JsonRequest
{
    std::string jsonrpc;
    std::string method;
    int64_t id;
    Json::Value params;
};

struct JsonResponse
{
    struct Error
    {
        int32_t code{0};
        std::string message{"success"};

        std::string toString() const
        {
            return "{\"code\":" + std::to_string(code) + "\"message\":" + message + "}";
        }
    };
    std::string jsonrpc;
    int64_t id;
    Error error;
    Json::Value result;
};

inline Json::Value generateResponse(Error::Ptr _error)
{
    Json::Value response;
    response["code"] = _error ? _error->errorCode() : 0;
    response["msg"] = _error ? _error->errorMessage() : "success";
    return response;
}

inline void nodeInfoToJson(Json::Value& _response, bcos::group::ChainNodeInfo::Ptr _nodeInfo)
{
    _response["name"] = _nodeInfo->nodeName();
    _response["type"] = _nodeInfo->nodeType();
    _response["status"] = (int32_t)_nodeInfo->status();
    _response["iniConfig"] = _nodeInfo->iniConfig();
    // set deployInfo
    _response["deployInfo"] = Json::Value(Json::arrayValue);
    auto const& infos = _nodeInfo->deployInfo();
    for (auto const& it : infos)
    {
        Json::Value item;
        item["service"] = it.first;
        item["ip"] = it.second;
        _response["deployInfo"].append(item);
    }
}

inline void groupInfoToJson(Json::Value& _response, bcos::group::GroupInfo::Ptr _groupInfo)
{
    _response["chainID"] = _groupInfo->chainID();
    _response["groupID"] = _groupInfo->groupID();
    _response["gensisConfig"] = _groupInfo->genesisConfig();
    _response["iniConfig"] = _groupInfo->iniConfig();
    _response["status"] = (int32_t)_groupInfo->status();
    _response["nodeList"] = Json::Value(Json::arrayValue);
    auto nodeInfos = _groupInfo->nodeInfos();
    for (auto const& it : nodeInfos)
    {
        Json::Value nodeInfoResponse;
        nodeInfoToJson(nodeInfoResponse, it.second);
        _response["nodeList"].append(nodeInfoResponse);
    }
}
}  // namespace rpc
}  // namespace bcos
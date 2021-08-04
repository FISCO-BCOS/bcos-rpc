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
 * @brief interface for RPC
 * @file Rpc.cpp
 * @author: octopus
 * @date 2021-07-15
 */

#include "libutilities/Log.h"
#include <bcos-rpc/rpc/Rpc.h>
using namespace bcos;
using namespace bcos::rpc;

void Rpc::start()
{
    // start amop
    m_AMOP->start();
    // start websocket service
    m_wsService->start();
    // start jsonhttp service
    m_httpServer->startListen();
    RPC_LOG(INFO) << LOG_BADGE("start");
}

void Rpc::stop()
{
    if (m_httpServer)
    {
        m_httpServer->stop();
    }
    if (m_wsService)
    {
        m_wsService->stop();
    }
    if (m_AMOP)
    {
        m_AMOP->stop();
    }
    RPC_LOG(INFO) << LOG_BADGE("stop");
}

/**
 * @brief: notify blockNumber to rpc
 * @param _blockNumber: blockNumber
 * @param _callback: resp callback
 * @return void
 */
void Rpc::asyncNotifyBlockNumber(
    bcos::protocol::BlockNumber _blockNumber, std::function<void(Error::Ptr)> _callback)
{
    m_wsService->notifyBlockNumberToClient(_blockNumber);
    if (_callback)
    {
        _callback(nullptr);
    }
}
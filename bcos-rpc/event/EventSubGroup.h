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
 * @file EvenPushGroup.h
 * @author: octopus
 * @date 2021-09-07
 */

#pragma once

#include <bcos-framework/interfaces/ledger/LedgerInterface.h>
#include <bcos-framework/interfaces/protocol/ProtocolTypeDef.h>
#include <bcos-framework/libutilities/Worker.h>
#include <bcos-rpc/event/EventSubTask.h>
#include <atomic>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace bcos
{
namespace event
{
class EventSubMatcher;
class EventSubGroup : bcos::Worker, public std::enable_shared_from_this<EventSubGroup>
{
public:
    using Ptr = std::shared_ptr<EventSubGroup>;
    using ConstPtr = std::shared_ptr<const EventSubGroup>;
    EventSubGroup(const std::string& _group) : bcos::Worker("t_event_" + _group) {}
    virtual ~EventSubGroup() { stop(); }

public:
    virtual void start();
    virtual void stop();

    virtual void executeWorker() override;

public:
    void reportTasks();
    void executeAddTasks();
    void executeCancelTasks();
    void executeEventSubTasks();

public:
    void subEventSubTask(EventSubTask::Ptr _task);
    void unsubEventSubTask(const std::string& _id);
    int64_t executeEventSubTask(EventSubTask::Ptr _task);

public:
    void onTaskComplete(EventSubTask::Ptr _task);
    bool checkConnAvailable(EventSubTask::Ptr _task);
    void processNextBlock(int64_t _blockNumber, EventSubTask::Ptr _task,
        std::function<void(Error::Ptr _error)> _callback);

public:
    std::string group() const { return m_group; }
    void setGroup(const std::string& _group) { m_group = _group; }

    std::shared_ptr<EventSubMatcher> matcher() const { return m_matcher; }
    void setMatcher(std::shared_ptr<EventSubMatcher> _matcher) { m_matcher = _matcher; }

    bcos::protocol::BlockNumber latestBlockNumber() { return m_latestBlockNumber.load(); }
    void setLatestBlockNumber(bcos::protocol::BlockNumber _latestBlockNumber)
    {
        m_latestBlockNumber.store(_latestBlockNumber);
    }

    void setIoc(std::shared_ptr<boost::asio::io_context> _ioc) { m_ioc = _ioc; }
    std::shared_ptr<boost::asio::io_context> ioc() const { return m_ioc; }

    bcos::ledger::LedgerInterface::Ptr ledger() const { return m_ledgerInterface; }
    void setLedger(bcos::ledger::LedgerInterface::Ptr _ledgerInterface)
    {
        m_ledgerInterface = _ledgerInterface;
    }

    int64_t maxBlockProcessPerLoop() const { return m_maxBlockProcessPerLoop; }
    void setMaxBlockProcessPerLoop(int64_t _maxBlockProcessPerLoop)
    {
        m_maxBlockProcessPerLoop = _maxBlockProcessPerLoop;
    }

private:
    std::atomic<bool> m_running{false};

    std::string m_group;
    // Note: the latest blocknumber of the group, the m_currentBlockNumber should keep update with
    // the latest block number
    std::atomic<bcos::protocol::BlockNumber> m_latestBlockNumber{-1};

    std::shared_ptr<EventSubMatcher> m_matcher;

    // lock for m_addTasks
    mutable std::shared_mutex x_addTasks;
    // tasks to be add
    std::vector<EventSubTask::Ptr> m_addTasks;
    // the number of tasks to be add
    std::atomic<uint32_t> m_addTaskCount{0};

    // lock for m_cancelTasks
    mutable std::shared_mutex x_cancelTasks;
    // tasks to be cancel
    std::vector<std::string> m_cancelTasks;
    // the number of tasks to be cancel
    std::atomic<uint32_t> m_cancelTaskCount{0};

    // all subscribe event sub tasks
    std::unordered_map<std::string, EventSubTask::Ptr> m_tasks;

    // ledger interfaces, for get tx && tx receipt of block
    bcos::ledger::LedgerInterface::Ptr m_ledgerInterface;

    // timer
    std::shared_ptr<boost::asio::deadline_timer> m_timer;
    // io context
    std::shared_ptr<boost::asio::io_context> m_ioc;

    //
    int64_t m_maxBlockProcessPerLoop = 10;
};

}  // namespace event
}  // namespace bcos
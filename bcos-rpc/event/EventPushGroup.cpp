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
 * @file EvenPushGroup.cpp
 * @author: octopus
 * @date 2021-09-07
 */

#include <bcos-framework/interfaces/protocol/CommonError.h>
#include <bcos-framework/interfaces/protocol/ProtocolTypeDef.h>
#include <bcos-framework/libutilities/Log.h>
#include <bcos-rpc/event/EventPushGroup.h>
#include <bcos-rpc/event/EventPushMatcher.h>
#include <chrono>
#include <memory>
#include <thread>

#define MAX_BLOCK_PROCESS_PER_LOOP (10)

using namespace bcos;
using namespace bcos::event;

void EventPushGroup::start()
{
    if (m_running.load())
    {
        EVENT_GROUP(INFO) << LOG_BADGE("start") << LOG_DESC("event push group is running")
                          << LOG_KV("group", m_group);
        return;
    }
    m_running.store(true);
    startWorking();

    if (1)
    {
        auto group = m_group;
        auto self = std::weak_ptr<EventPushGroup>(shared_from_this());
        // latest block number
        m_ledgerInterface->asyncGetBlockNumber(
            [group, self](Error::Ptr _error, protocol::BlockNumber _blockNumber) {
                if (_error && _error->errorCode() != bcos::protocol::CommonError::SUCCESS)
                {
                    EVENT_GROUP(ERROR)
                        << LOG_BADGE("start") << LOG_DESC("asyncGetBlockNumber")
                        << LOG_KV("group", group) << LOG_KV("errorCode", _error->errorCode())
                        << LOG_KV("errorMessage", _error->errorMessage());
                    return;
                }

                auto epGroup = self.lock();
                if (!epGroup)
                {
                    return;
                }

                epGroup->setLatestBlockNumber(_blockNumber);
                EVENT_GROUP(INFO) << LOG_BADGE("start") << LOG_DESC("asyncGetBlockNumber")
                                  << LOG_KV("group", group) << LOG_KV("blockNumber", _blockNumber);
            });
    }

    EVENT_GROUP(INFO) << LOG_BADGE("start") << LOG_DESC("start event push group successfully")
                      << LOG_KV("group", m_group);
}

void EventPushGroup::stop()
{
    if (!m_running.load())
    {
        EVENT_GROUP(INFO) << LOG_BADGE("stop") << LOG_DESC("event push group is not running")
                          << LOG_KV("group", m_group);
        return;
    }
    m_running.store(false);
    finishWorker();
    stopWorking();
    // will not restart worker, so terminate it
    terminate();

    EVENT_GROUP(INFO) << LOG_BADGE("stop") << LOG_DESC("stop event push group successfully")
                      << LOG_KV("group", m_group);
}

void EventPushGroup::subEventPushTask(EventPushTask::Ptr _task)
{
    EVENT_GROUP(INFO) << LOG_BADGE("subEventPushTask") << LOG_KV("id", _task->id());
    std::unique_lock lock(x_addTasks);
    m_addTasks.push_back(_task);
    m_addTaskCount++;
}

void EventPushGroup::unsubEventPushTask(const std::string& _id)
{
    EVENT_GROUP(INFO) << LOG_BADGE("unsubEventPushTask") << LOG_KV("id", _id);
    std::unique_lock lock(x_cancelTasks);
    m_cancelTasks.push_back(_id);
    m_cancelTaskCount++;
}

void EventPushGroup::executeWorker()
{
    executeCancelTasks();
    executeAddTasks();
    executeEventPushTasks();
}

void EventPushGroup::executeAddTasks()
{
    if (m_addTaskCount.load() == 0)
    {
        return;
    }

    std::unique_lock lock(x_addTasks);
    for (auto& task : m_addTasks)
    {
        auto id = task->id();
        if (m_tasks.find(id) == m_tasks.end())
        {
            m_tasks[id] = task;
            EVENT_GROUP(INFO) << LOG_BADGE("executeAddTasks") << LOG_KV("id", task->id());
        }
        else
        {
            EVENT_GROUP(ERROR) << LOG_BADGE("executeAddTasks")
                               << LOG_DESC("event push task already exist")
                               << LOG_KV("id", task->id());
        }
    }
    m_addTaskCount.store(0);
    m_addTasks.clear();
}

void EventPushGroup::executeCancelTasks()
{
    if (m_cancelTaskCount.load() == 0)
    {
        return;
    }

    std::unique_lock lock(x_cancelTasks);
    for (const auto& id : m_cancelTasks)
    {
        auto r = m_tasks.erase(id);
        if (r)
        {
            EVENT_GROUP(INFO) << LOG_BADGE("executeAddTasks") << LOG_KV("id", id)
                              << LOG_KV("result", r);
        }
        else
        {
            EVENT_GROUP(WARNING) << LOG_BADGE("executeAddTasks")
                                 << LOG_DESC("event push task not exist") << LOG_KV("id", id)
                                 << LOG_KV("result", r);
        }
    }
    m_cancelTaskCount.store(0);
    m_cancelTasks.clear();
}

bool EventPushGroup::checkConnAvailable(EventPushTask::Ptr _task)
{
    Json::Value jResp(Json::arrayValue);
    return _task->callback()(_task->id(), jResp);
}

int64_t EventPushGroup::executeEventPushTask(EventPushTask::Ptr _task)
{
    // tests whether the connection of the session is available first
    auto connAvailable = checkConnAvailable(_task);
    if (!connAvailable)
    {
        unsubEventPushTask(_task->id());
        return -1;
    }

    // task is working, waiting for the end
    if (_task->work())
    {
        return 0;
    }

    if (_task->isCompleted())
    {
        // TODO: check if the task is completed
        return 0;
    }

    bcos::protocol::BlockNumber blockNumber = latestBlockNumber();
    bcos::protocol::BlockNumber nextBlockNumber = _task->state()->currentBlockNumber() + 1;
    if (blockNumber < nextBlockNumber)
    {
        // wait for block to be sealed
        return 0;
    }

    _task->setWork(true);

    int64_t blockCanProcess = blockNumber - nextBlockNumber + 1;
    int64_t maxBlockProcessPerLoop = MAX_BLOCK_PROCESS_PER_LOOP;
    blockCanProcess =
        (blockCanProcess > maxBlockProcessPerLoop ? maxBlockProcessPerLoop : blockCanProcess);

    class BlockProcess : public std::enable_shared_from_this<BlockProcess>
    {
    public:
        void process(int64_t _blockNumber)
        {
            if (_blockNumber > m_endBlockNumber)
            {  // all block has been proccessed
                m_task->state()->setCurrentBlockNumber(m_endBlockNumber);
                m_task->setWork(false);
                return;
            }

            auto group = m_group;
            auto task = m_task;
            auto pro = shared_from_this();
            m_group->processBlock(
                _blockNumber, task, [_blockNumber, group, pro](Error::Ptr _error) {
                    (void)_error;
                    pro->process(_blockNumber + 1);
                });
        }

    public:
        bcos::protocol::BlockNumber m_endBlockNumber;
        std::shared_ptr<EventPushGroup> m_group;
        EventPushTask::Ptr m_task;
    };

    auto p = std::make_shared<BlockProcess>();

    p->m_endBlockNumber = nextBlockNumber + blockCanProcess - 1;
    p->m_group = shared_from_this();
    p->m_task = _task;
    p->process(nextBlockNumber);

    return blockCanProcess;
}

void EventPushGroup::processBlock(int64_t _blockNumber, EventPushTask::Ptr _task,
    std::function<void(Error::Ptr _error)> _callback)
{
    auto self = std::weak_ptr<EventPushGroup>(shared_from_this());
    auto matcher = m_matcher;
    m_ledgerInterface->asyncGetBlockDataByNumber(_blockNumber,
        bcos::ledger::RECEIPTS | bcos::ledger::TRANSACTIONS,
        [matcher, _task, _blockNumber, _callback, self](
            Error::Ptr _error, protocol::Block::Ptr _block) {
            if (_error && _error->errorCode() != bcos::protocol::CommonError::SUCCESS)
            {
                EVENT_GROUP(ERROR)
                    << LOG_BADGE("processBlock") << LOG_DESC("asyncGetBlockDataByNumber")
                    << LOG_KV("id", _task->id()) << LOG_KV("blockNumber", _blockNumber)
                    << LOG_KV("errorCode", _error->errorCode())
                    << LOG_KV("errorMessage", _error->errorMessage());
                _callback(_error);
                return;
            }

            Json::Value jResp(Json::arrayValue);
            auto count = matcher->matches(_task->params(), _block, jResp);
            if (!count)
            {
                _callback(nullptr);
                return;
            }

            EVENT_GROUP(TRACE) << LOG_BADGE("processBlock") << LOG_DESC("asyncGetBlockDataByNumber")
                               << LOG_KV("blockNumber", _blockNumber) << LOG_KV("id", _task->id())
                               << LOG_KV("count", count);

            _task->callback()(_task->id(), jResp);
            _callback(nullptr);
        });
}

void EventPushGroup::executeEventPushTasks()
{
    for (const auto& taskEntry : m_tasks)
    {
        executeEventPushTask(taskEntry.second);
    }

    // limiting speed
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

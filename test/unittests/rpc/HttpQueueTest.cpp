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
 * @brief test for HttpQueue
 * @file HttpQueueTest.cpp
 * @author: octopus
 * @date 2021-07-12
 */

#include <bcos-framework/testutils/TestPromptFixture.h>
#include <bcos-rpc/http/HttpQueue.h>
#include <boost/test/unit_test.hpp>

using namespace bcos;
using namespace bcos::http;
using namespace bcos::test;

BOOST_FIXTURE_TEST_SUITE(HttpQueueTest, TestPromptFixture)

BOOST_AUTO_TEST_CASE(test_Queuelimit)
{
    auto queue = std::make_shared<Queue>();
    BOOST_CHECK_EQUAL(queue->limit(), 16);
    BOOST_CHECK(!queue->isFull());
    queue->setLimit(11111);
    BOOST_CHECK_EQUAL(queue->limit(), 11111);
    BOOST_CHECK(!queue->isFull());
}


BOOST_AUTO_TEST_CASE(test_QueueEnqueue)
{
    auto queue = std::make_shared<Queue>(20);
    BOOST_CHECK_EQUAL(queue->limit(), 20);
    BOOST_CHECK(!queue->isFull());
    bool flag = false;
    queue->setSender([&flag](HttpResponsePtr _req) {
        (void)_req;
        flag = true;
    });
    queue->enqueue(HttpResponsePtr());

    BOOST_CHECK(flag);
}

BOOST_AUTO_TEST_SUITE_END()
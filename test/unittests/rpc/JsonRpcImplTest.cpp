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
 * @brief test for JsonRpcImpl
 * @file JsonRpcImplTest.cpp
 * @author: octopus
 * @date 2021-07-12
 */

#include <bcos-framework/testutils/TestPromptFixture.h>
#include <bcos-rpc/jsonrpc/JsonRpcImpl_2_0.h>
#include <boost/test/unit_test.hpp>

using namespace bcos;
using namespace bcos::rpc;
using namespace bcos::test;


BOOST_FIXTURE_TEST_SUITE(JsonRpcImplTest, TestPromptFixture)

BOOST_AUTO_TEST_CASE(test_initMethod)
{
    auto jsonRpcInterface = std::make_shared<JsonRpcImpl_2_0>();
    auto f = jsonRpcInterface->methodToFunc();
    BOOST_CHECK_EQUAL(f.size(), 18);

    bool flag = false;
    jsonRpcInterface->registerMethod("testFunc", [&flag](Json::Value _jValue, RespFunc _respFunc) {
        flag = true;
        _respFunc(nullptr, _jValue);
    });

    std::string json = R"({"jsonrpc":"2.0","method":"testFunc","params":[1,2,3],"id":123})";
    jsonRpcInterface->onRPCRequest(json, [](const std::string& _str) {
        (void)_str;
        // BOOST_CHECK_EQUAL(_str, R"({"id":123,"jsonrpc":"2.0","result":[[1,2,3]]})");
    });
    BOOST_CHECK(flag);
}

BOOST_AUTO_TEST_CASE(test_parseRpcRequestJson)
{
    auto jsonRpcInterface = std::make_shared<JsonRpcImpl_2_0>();
    {
        std::string json = R"({"jsonrpc":"2.0","method":"getNodeInfo","params":[],"id":123})";
        JsonRequest req;
        jsonRpcInterface->parseRpcRequestJson(json, req);
        BOOST_CHECK_EQUAL(req.id, 123);
        BOOST_CHECK_EQUAL(req.jsonrpc, "2.0");
        BOOST_CHECK_EQUAL(req.method, "getNodeInfo");
        BOOST_CHECK_EQUAL(req.params.size(), 0);
    }

    {
        JsonRequest req;
        std::string json =
            R"({"jsonrpc":"3.0","method":"testmethod","params":[1,2,3,"dsff"],"id":321})";
        jsonRpcInterface->parseRpcRequestJson(json, req);
        BOOST_CHECK_EQUAL(req.id, 321);
        BOOST_CHECK_EQUAL(req.jsonrpc, "3.0");
        BOOST_CHECK_EQUAL(req.method, "testmethod");
        BOOST_CHECK_EQUAL(req.params.size(), 4);
    }

    {
        JsonRequest req;
        // miss jsonrpc
        std::string json = R"({"jsonrpcExt":"2.0","method":"getNodeInfo","params":[],"id":123})";
        BOOST_CHECK_THROW(jsonRpcInterface->parseRpcRequestJson(json, req), JsonRpcException);
    }

    {
        JsonRequest req;
        // miss method
        std::string json = R"({"jsonrpc":"2.0","method_ext":"getNodeInfo","params":[],"id":123})";
        BOOST_CHECK_THROW(jsonRpcInterface->parseRpcRequestJson(json, req), JsonRpcException);
    }

    {
        JsonRequest req;
        // miss params
        std::string json = R"({"jsonrpc":"2.0","method":"getNodeInfo","paramsExt":[],"id":123})";
        BOOST_CHECK_THROW(jsonRpcInterface->parseRpcRequestJson(json, req), JsonRpcException);
    }

    {
        JsonRequest req;
        // params not array
        std::string json = R"({"jsonrpc":"2.0","method":"getNodeInfo","params":1111,"id":123})";
        BOOST_CHECK_THROW(jsonRpcInterface->parseRpcRequestJson(json, req), JsonRpcException);
    }

    {
        JsonRequest req;
        // invalid json string
        std::string json = R"(adfsafsfdfsf)";
        BOOST_CHECK_THROW(jsonRpcInterface->parseRpcRequestJson(json, req), JsonRpcException);
    }
}

BOOST_AUTO_TEST_CASE(test_parseRpcResponseJson)
{
    auto jsonRpcInterface = std::make_shared<JsonRpcImpl_2_0>();
    {
        std::string json = R"({"jsonrpc":"2.0","result":[],"id":123})";
        JsonResponse resp;
        jsonRpcInterface->parseRpcResponseJson(json, resp);
        BOOST_CHECK_EQUAL(resp.id, 123);
        BOOST_CHECK_EQUAL(resp.jsonrpc, "2.0");
        BOOST_CHECK_EQUAL(resp.result.size(), 0);
    }

    {
        std::string json = R"({"jsonrpc":"3.0","result":[1,2,3,4],"id":321})";
        JsonResponse resp;
        jsonRpcInterface->parseRpcResponseJson(json, resp);
        BOOST_CHECK_EQUAL(resp.id, 321);
        BOOST_CHECK_EQUAL(resp.jsonrpc, "3.0");
        BOOST_CHECK_EQUAL(resp.result.size(), 4);
    }

    {
        std::string json = R"({"jsonrpc":"3.0","error":{"code":0,"message":"success"},"id":321})";
        JsonResponse resp;
        jsonRpcInterface->parseRpcResponseJson(json, resp);
        BOOST_CHECK_EQUAL(resp.id, 321);
        BOOST_CHECK_EQUAL(resp.jsonrpc, "3.0");
        BOOST_CHECK_EQUAL(resp.error.code, 0);
        BOOST_CHECK_EQUAL(resp.error.message, "success");
    }

    {
        // miss jsonrpc
        std::string json =
            R"({"jsonrpcext":"3.0","error":{"code":0,"message":"success"},"id":321})";
        JsonResponse resp;
        BOOST_CHECK_THROW(jsonRpcInterface->parseRpcResponseJson(json, resp), JsonRpcException);
    }

    {
        // miss jsonrpc
        std::string json =
            R"({"jsonrpc":"3.0","error":{"code":0,"message":"success"},"idext":321})";
        JsonResponse resp;
        BOOST_CHECK_THROW(jsonRpcInterface->parseRpcResponseJson(json, resp), JsonRpcException);
    }

    {
        // invalid json
        std::string json = "aaa";
        JsonResponse resp;
        BOOST_CHECK_THROW(jsonRpcInterface->parseRpcResponseJson(json, resp), JsonRpcException);
    }
}

BOOST_AUTO_TEST_CASE(test_toStringResponse)
{
    auto jsonRpcInterface = std::make_shared<JsonRpcImpl_2_0>();
    {
        JsonResponse resp;
        resp.jsonrpc = "jsonrpc";
        resp.id = 123;
        resp.error.code = 1234;
        resp.error.message = "success";

        auto json = jsonRpcInterface->toStringResponse(resp);

        JsonResponse respDecode;
        jsonRpcInterface->parseRpcResponseJson(json, respDecode);
        BOOST_CHECK_EQUAL(respDecode.jsonrpc, resp.jsonrpc);
        BOOST_CHECK_EQUAL(respDecode.id, resp.id);
        BOOST_CHECK_EQUAL(respDecode.error.code, resp.error.code);
        BOOST_CHECK_EQUAL(respDecode.error.message, resp.error.message);
    }

    {
        JsonResponse resp;
        resp.jsonrpc = "jsonrpc";
        resp.id = 321;
        resp.result = "FISCO-BCOS 3.0";

        auto json = jsonRpcInterface->toStringResponse(resp);

        JsonResponse respDecode;
        jsonRpcInterface->parseRpcResponseJson(json, respDecode);
        BOOST_CHECK_EQUAL(respDecode.jsonrpc, resp.jsonrpc);
        BOOST_CHECK_EQUAL(respDecode.id, resp.id);
        BOOST_CHECK_EQUAL(respDecode.result.asString(), resp.result.asString());
    }
}

BOOST_AUTO_TEST_SUITE_END()

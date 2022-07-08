/**
 * @CopyRight:
 * FISCO-BCOS is free software: you can redistribute it and/or modify
 * it under the terms 32 the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FISCO-BCOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FISCO-BCOS.  If not, see <http://www.gnu.org/licenses/>
 * (c) 2016-2018 fisco-dev contributors.
 *
 * @file RPCTest.cpp
 * @author: lucasli
 * @date 2021-11-6
 */
#include "FakeModule.h"

#include <bcos-framework/testutils/TestPromptFixture.h>
#include <bcos-rpc/jsonrpc/JsonRpcImpl_2_0.h>
#include <bcos-rpc/jsonrpc/groupmgr/GroupManager.h>
#include <boost/test/unit_test.hpp>
#include <future>

using namespace bcos;
using namespace bcos::rpc;
using namespace bcos::group;
using namespace bcos::gateway;

namespace bcos
{
namespace test
{
class RpcTestFixure
{
public:
    JsonRpcImpl_2_0::Ptr createJsonRpc(std::string const& _groupID, std::string const& _nodeName)
    {
        auto fakeGroupMgr = std::make_shared<FakeGroupManager>(_groupID, _nodeName);
        auto fakeGateway = std::make_shared<bcos::test::FakeGateway>();

        return std::make_shared<JsonRpcImpl_2_0>(fakeGroupMgr, fakeGateway);
    }
};

BOOST_FIXTURE_TEST_SUITE(RPCTest, RpcTestFixure)
BOOST_AUTO_TEST_CASE(testGetBlockNumber)
{
    auto jsonRpc = createJsonRpc("getBlockNumberGroupID", "nodegetBlockNumberNodeNameName");

    std::promise<void> getBlockNumPromise;
    jsonRpc->getBlockNumber("getBlockNumberGroupID", "nodegetBlockNumberNodeNameName",
        [&](bcos::Error::Ptr _error, Json::Value& _blockNumber) {
            BOOST_CHECK(!_error);
            BOOST_CHECK(_blockNumber == 4);

            getBlockNumPromise.set_value();
        });
    getBlockNumPromise.get_future().get();
}

BOOST_AUTO_TEST_CASE(testGetPendingTxSize)
{
    auto jsonRpc = createJsonRpc("getPendingTxSizeGroupID", "getPendingTxSizeNodeName");

    std::promise<void> getPendingTxSizePromise;
    jsonRpc->getPendingTxSize("getPendingTxSizeGroupID", "getPendingTxSizeNodeName",
        [&](bcos::Error::Ptr _error, Json::Value& _pendingTxSize) {
            BOOST_CHECK(!_error);
            BOOST_CHECK(_pendingTxSize == 5);

            getPendingTxSizePromise.set_value();
        });
    getPendingTxSizePromise.get_future().get();
}

BOOST_AUTO_TEST_CASE(testGetSystemConfigByKey)
{
    std::string keyValue = "keyForGetSystemConfigByKey";
    auto jsonRpc = createJsonRpc("getSystemConfigByKeyGroupID", "getSystemConfigByKeyNodeName");

    std::promise<void> getPendingTxSizePromise;
    jsonRpc->getSystemConfigByKey("getSystemConfigByKeyGroupID", "getSystemConfigByKeyNodeName",
        keyValue, [&](bcos::Error::Ptr _error, Json::Value& _SystemConf) {
            BOOST_CHECK(!_error);
            BOOST_CHECK(_SystemConf["value"] == "mockSysConfBykey");
            BOOST_CHECK(_SystemConf["blockNumber"] == 3);

            getPendingTxSizePromise.set_value();
        });
    getPendingTxSizePromise.get_future().get();
}

BOOST_AUTO_TEST_CASE(testGetTotalTransactionCount)
{
    auto jsonRpc = createJsonRpc("totalTransCntGroupID", "getTotalTransCntNodeName");

    std::promise<void> totalTransCntPromise;
    jsonRpc->getTotalTransactionCount("totalTransCntGroupID", "getTotalTransCntNodeName",
        [&](bcos::Error::Ptr _error, Json::Value& _TransCnt) {
            BOOST_CHECK(!_error);
            BOOST_CHECK(_TransCnt["transactionCount"] == 5);
            BOOST_CHECK(_TransCnt["failedTransactionCount"] == 6);
            BOOST_CHECK(_TransCnt["blockNumber"] == 7);

            totalTransCntPromise.set_value();
        });
    totalTransCntPromise.get_future().get();
}

BOOST_AUTO_TEST_CASE(testGetGroupInfo)
{
    auto jsonRpc = createJsonRpc("getGroupInfoGroupID", "getGroupInfoNodeName");

    std::promise<void> getGroupInfoPromise;
    jsonRpc->getGroupInfo(
        "getGroupInfoGroupID", [&](bcos::Error::Ptr _error, Json::Value& _groupInfo) {
            BOOST_CHECK(!_error);
            BOOST_CHECK(_groupInfo["chainID"] == "");
            BOOST_CHECK(_groupInfo["groupID"] == "getGroupInfoGroupID");
            BOOST_CHECK(_groupInfo["gensisConfig"] == "fakeGenesisConf");
            BOOST_CHECK(_groupInfo["iniConfig"] == "fakeIniConf");
            BOOST_CHECK(_groupInfo["nodeList"][0]["name"] == "getGroupInfoNodeName");
            BOOST_CHECK(_groupInfo["nodeList"][0]["type"] == (bcos::group::NodeType)0);

            getGroupInfoPromise.set_value();
        });
    getGroupInfoPromise.get_future().get();
}

BOOST_AUTO_TEST_CASE(testGetGroupNodeInfo)
{
    auto jsonRpc = createJsonRpc("getGroupNodeInfoGroupID", "getGroupNodeInfoNodeName");

    std::promise<void> getGroupNodeInfoPromise;
    jsonRpc->getGroupNodeInfo("getGroupNodeInfoGroupID", "getGroupNodeInfoNodeName",
        [&](bcos::Error::Ptr _error, Json::Value& _groupNodeInfo) {
            BOOST_CHECK(!_error);
            BOOST_CHECK(_groupNodeInfo["name"] == "getGroupNodeInfoNodeName");
            BOOST_CHECK(_groupNodeInfo["type"] == (bcos::group::NodeType)0);

            getGroupNodeInfoPromise.set_value();
        });
    getGroupNodeInfoPromise.get_future().get();
}

BOOST_AUTO_TEST_CASE(testGetGroupList)
{
    auto jsonRpc = createJsonRpc("getGroupListGroupID", "getGroupListNodeName");

    std::promise<void> getGroupListPromise;
    jsonRpc->getGroupList([&](bcos::Error::Ptr _error, Json::Value& _groupList) {
        BOOST_CHECK(!_error);
        BOOST_CHECK(_groupList["msg"] == "success");
        BOOST_CHECK(_groupList["groupList"][0] == "getGroupListGroupID");
        getGroupListPromise.set_value();
    });
    getGroupListPromise.get_future().get();
}

BOOST_AUTO_TEST_CASE(testGetGroupInfoList)
{
    auto jsonRpc = createJsonRpc("getGroupInfoListGroupID", "getGroupInfoListNodeName");

    std::promise<void> getGroupInfoListPromise;
    jsonRpc->getGroupInfoList([&](bcos::Error::Ptr _error, Json::Value& _groupInfoList) {
        BOOST_CHECK(!_error);
        BOOST_CHECK(_groupInfoList[0]["chainID"] == "");
        BOOST_CHECK(_groupInfoList[0]["groupID"] == "getGroupInfoListGroupID");
        BOOST_CHECK(_groupInfoList[0]["gensisConfig"] == "fakeGenesisConf");
        BOOST_CHECK(_groupInfoList[0]["iniConfig"] == "fakeIniConf");
        BOOST_CHECK(_groupInfoList[0]["nodeList"][0]["name"] == "getGroupInfoListNodeName");

        getGroupInfoListPromise.set_value();
    });
    getGroupInfoListPromise.get_future().get();
}

BOOST_AUTO_TEST_CASE(testGetCode)
{
    auto jsonRpc = createJsonRpc("getCodeGroupID", "getCodeNodeName");

    std::promise<void> getCodePromise;
    jsonRpc->getCode("getCodeGroupID", "getCodeNodeName", "mockContractAddr",
        [&](bcos::Error::Ptr _error, Json::Value& _mockResponse) {
            BOOST_CHECK(!_error);
            BOOST_CHECK(_mockResponse["msg"] == "unimplemented method getConde");

            getCodePromise.set_value();
        });
    getCodePromise.get_future().get();
}

BOOST_AUTO_TEST_CASE(testGetPeers)
{
    auto jsonRpc = createJsonRpc("getPeersGroupID", "getPeersNodeName");

    std::promise<void> getPeersPromise;
    jsonRpc->getPeers([&](bcos::Error::Ptr _error, Json::Value& _peersInfo) {
        BOOST_CHECK(!_error);
        BOOST_CHECK(_peersInfo == "mockPeersStringInfo");

        getPeersPromise.set_value();
    });
    getPeersPromise.get_future().get();
}

BOOST_AUTO_TEST_CASE(testGetPbftView)
{
    auto jsonRpc = createJsonRpc("getPbftViewGroupID", "getPbftViewNodeName");

    std::promise<void> getPbftViewPromise;
    jsonRpc->getPbftView("getPbftViewGroupID", "getPbftViewNodeName",
        [&](bcos::Error::Ptr _error, Json::Value& _PbftView) {
            BOOST_CHECK(!_error);
            BOOST_CHECK(_PbftView.asInt() == 113);

            getPbftViewPromise.set_value();
        });
    getPbftViewPromise.get_future().get();
}

BOOST_AUTO_TEST_CASE(testGetSyncStatus)
{
    auto jsonRpc = createJsonRpc("getSyncStatusGroupID", "getSyncStatusNodeName");

    std::promise<void> getSyncStatusPromise;
    jsonRpc->getSyncStatus("getSyncStatusGroupID", "getSyncStatusNodeName",
        [&](bcos::Error::Ptr _error, Json::Value& _syncInfo) {
            BOOST_CHECK(!_error);
            BOOST_CHECK(_syncInfo == "mockSyncInfo");

            getSyncStatusPromise.set_value();
        });
    getSyncStatusPromise.get_future().get();
}

BOOST_AUTO_TEST_CASE(testGetSealerList)
{
    auto jsonRpc = createJsonRpc("GetSealerListGroupID", "GetSealerListNodeName");

    std::promise<void> getSealerListPromise;
    jsonRpc->getSealerList("GetSealerListGroupID", "GetSealerListNodeName",
        [&](bcos::Error::Ptr _error, Json::Value& _sealerList) {
            std::string testNum = "11123455";
            auto testNumByte = bytes(testNum.begin(), testNum.end());
            auto hexNumPtr = toHexString(testNumByte.begin(), testNumByte.end());

            BOOST_CHECK(!_error);
            BOOST_CHECK(_sealerList[0]["nodeID"].asString() == *hexNumPtr);
            BOOST_CHECK(_sealerList[0]["weight"].asInt() == 55);

            getSealerListPromise.set_value();
        });
    getSealerListPromise.get_future().get();
}

BOOST_AUTO_TEST_CASE(testGetObserverList)
{
    auto jsonRpc = createJsonRpc("getObserverListGroupID", "getObserverListNodeName");

    std::promise<void> getObserverListPromise;
    jsonRpc->getObserverList("getObserverListGroupID", "getObserverListNodeName",
        [&](bcos::Error::Ptr _error, Json::Value& _observerList) {
            std::string testNum = "11123455";
            auto testNumByte = bytes(testNum.begin(), testNum.end());
            auto hexNumPtr = toHexString(testNumByte.begin(), testNumByte.end());

            BOOST_CHECK(!_error);
            BOOST_CHECK(_observerList[0] == *hexNumPtr);

            getObserverListPromise.set_value();
        });
    getObserverListPromise.get_future().get();
}

BOOST_AUTO_TEST_CASE(testGetBlockByNumber)
{
    int64_t blockNumber = 5;
    auto jsonRpc = createJsonRpc("getBlockByNumberGroupID", "getBlockByNumberNodeName");

    std::promise<void> getBlockByNumberPromise;
    jsonRpc->getBlockByNumber("getBlockByNumberGroupID", "getBlockByNumberNodeName", blockNumber,
        false, true, [&](bcos::Error::Ptr _error, Json::Value& _blockValue) {
            Json::Value jResp;
            BOOST_CHECK(!_error);
            BOOST_CHECK(_blockValue["hash"] == jResp);

            getBlockByNumberPromise.set_value();
        });
    getBlockByNumberPromise.get_future().get();
}

BOOST_AUTO_TEST_CASE(testGetBlockByHash)
{
    std::string blockHash = "0x067150c07dab4facb7160e075548007e067150c07dab4facb7160e075548007e";
    auto jsonRpc = createJsonRpc("getBlockByHashGroupID", "getBlockByHashGroupIDNodeName");

    std::promise<void> getBlockByHashPromise;
    jsonRpc->getBlockByHash("getBlockByHashGroupID", "getBlockByHashGroupIDNodeName", blockHash,
        false, true, [&](bcos::Error::Ptr _error, Json::Value& _blockData) {
            BOOST_CHECK(!_error);
            BOOST_CHECK(_blockData["hash"] ==
                        "0x675d526d46f37a0b646c4b2e5fd1aaa343d2158b64a5d6b9782bda2bdf4461b3");

            getBlockByHashPromise.set_value();
        });
    getBlockByHashPromise.get_future().get();
}

BOOST_AUTO_TEST_CASE(testGetBlockHashByNumber)
{
    int64_t blockNumber = 5;
    auto jsonRpc = createJsonRpc("getBlockHashByNumberGroupID", "getBlockHashByNumberNodeName");

    std::promise<void> getBlockHashByNumberPromise;
    jsonRpc->getBlockHashByNumber("getBlockHashByNumberGroupID", "getBlockHashByNumberNodeName",
        blockNumber, [&](bcos::Error::Ptr _error, Json::Value& _blockHash) {
            BOOST_CHECK(!_error);
            BOOST_CHECK(
                _blockHash == "0x067150c07dab4facb7160e075548007e067150c07dab4facb7160e075548007e");

            getBlockHashByNumberPromise.set_value();
        });
    getBlockHashByNumberPromise.get_future().get();
}

// BOOST_AUTO_TEST_CASE(testSendTransaction)
// {
//     std::string mockData = "mockDataForSendTransaction";
//     auto jsonRpc = createJsonRpc("sendTransactionGroupID", "sendTransactionNodeName");

//     std::promise<void> sendTransactionPromise;
//     jsonRpc->sendTransaction("sendTransactionGroupID", "sendTransactionNodeName", mockData,
//     false,
//         [&](bcos::Error::Ptr _error, Json::Value& _transactionSubmitResult) {
//             Json::Value jResp;
//             RPC_IMPL_LOG(TRACE) << LOG_BADGE("sendTransaction")
//                                 << LOG_KV("_transactionSubmitResult", _transactionSubmitResult);

//             BOOST_CHECK(!_error);
//             BOOST_CHECK(_transactionSubmitResult == mockData);

//             sendTransactionPromise.set_value();
//         });
//     sendTransactionPromise.get_future().get();
// }

// BOOST_AUTO_TEST_CASE(testGetTransaction)
// {
//     Json::Value response = rpc->getTransaction(groupId, nodeName,
//         "447adc7975a23443135577c2272417d8682e709bf5868981f4d301f1cee561c8929f4a8ef604365a45025bde01"
//         "c1e67875f398fb564d5c73ffaa65b42c7f4586",
//         false, respFunc);

//     BOOST_CHECK(response ==
//                 "447adc7975a23443135577c2272417d8682e709bf5868981f4d301f1cee561c8929f4a8ef604365a45"
//                 "025bde01c1e67875f398fb564d5c73ffaa65b42c7f4586");
//     // BOOST_CHECK_THROW(rpc->getTransaction(invalidGroup, nodeName, "FAILEDdata", false,
//     respFunc),
//     // JsonRpcException);
// }

// BOOST_AUTO_TEST_CASE(testGetTransactionReceipt)
// {
//     Json::Value response = rpc->getTransactionReceipt(groupId, nodeName,
//         "447adc7975a23443135577c2272417d8682e709bf5868981f4d301f1cee561c8929f4a8ef604365a45025bde01"
//         "c1e67875f398fb564d5c73ffaa65b42c7f4586",
//         false, respFunc);

//     BOOST_CHECK(response ==
//                 "447adc7975a23443135577c2272417d8682e709bf5868981f4d301f1cee561c8929f4a8ef604365a45"
//                 "025bde01c1e67875f398fb564d5c73ffaa65b42c7f4586");
//     // BOOST_CHECK_THROW(rpc->getTransactionReceipt(invalidGroup, nodeName, "FAILEDdata", false,
//     // respFunc), JsonRpcException);
// }

// BOOST_AUTO_TEST_CASE(testCall)
// {
//     RespFunc respFunc;
//     int64_t blockNumber = 0;
//     std::string to = "";
//     std::string data = "testData";
//     rpc->call(groupId, nodeName, to, data, respFunc);
//     // std::cout << "1111111111111111111111111111" << endl;
//     // // std::cout << type(respFunc) << endl;
//     // std::cout << respFunc << endl;

//     BOOST_CHECK(respFunc["blockNumber"] ==
//                 "447adc7975a23443135577c2272417d8682e709bf5868981f4d301f1cee561c8929f4a8ef604365a45"
//                 "025bde01c1e67875f398fb564d5c73ffaa65b42c7f4586");
//     // BOOST_CHECK_THROW(rpc->call(invalidGroup, nodeName, blockNumber, true, true, respFunc),
//     // JsonRpcException);
// }

BOOST_AUTO_TEST_SUITE_END()
}  // namespace test
}  // namespace bcos

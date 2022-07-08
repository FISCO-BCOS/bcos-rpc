/**
 * @CopyRight:
 * FISCO-BCOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
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
 * @file FakeModule.h
 * @author: lucasli
 * @date 2021-11-6
 */
#pragma once

#include "bcos-tars-protocol/protocol/BlockFactoryImpl.h"
#include "bcos-tars-protocol/protocol/BlockHeaderFactoryImpl.h"
#include "bcos-tars-protocol/protocol/TransactionFactoryImpl.h"
#include "bcos-tars-protocol/protocol/TransactionReceiptFactoryImpl.h"
#include <bcos-framework/interfaces/consensus/ConsensusInterface.h>
#include <bcos-framework/interfaces/crypto/CryptoSuite.h>
#include <bcos-framework/interfaces/gateway/GatewayInterface.h>
#include <bcos-framework/interfaces/ledger/LedgerConfig.h>
#include <bcos-framework/interfaces/multigroup/GroupInfo.h>
#include <bcos-framework/interfaces/protocol/Block.h>
#include <bcos-framework/interfaces/protocol/BlockFactory.h>
#include <bcos-framework/interfaces/protocol/BlockHeaderFactory.h>
#include <bcos-framework/interfaces/protocol/TransactionFactory.h>
#include <bcos-framework/interfaces/protocol/TransactionMetaData.h>
#include <bcos-framework/interfaces/protocol/TransactionReceipt.h>
#include <bcos-framework/interfaces/protocol/TransactionReceiptFactory.h>
#include <bcos-framework/interfaces/sync/BlockSyncInterface.h>
#include <bcos-framework/libprotocol/TransactionSubmitResultImpl.h>
#include <bcos-framework/testutils/faker/FakeScheduler.h>
#include <bcos-framework/testutils/faker/FakeTxPool.h>
#include <bcos-rpc/jsonrpc/JsonRpcImpl_2_0.h>
#include <bcos-rpc/jsonrpc/groupmgr/GroupManager.h>
#include <bcos-rpc/jsonrpc/groupmgr/NodeService.h>
#include <boost/test/unit_test.hpp>

using namespace bcos;
using namespace bcos::rpc;
using namespace bcos::sync;
using namespace bcos::consensus;
using namespace bcos::group;
using namespace bcos::gateway;
using namespace bcos::protocol;
using namespace bcostars;

namespace bcos
{
namespace test
{
class FakeConsensusNode : public ConsensusNodeInterface
{
public:
    using Ptr = std::shared_ptr<FakeConsensusNode>;
    FakeConsensusNode(bcos::crypto::PublicPtr const& _nodeID, uint64_t const& _weight)
      : m_nodeID(_nodeID), m_weight(_weight)
    {}
    virtual ~FakeConsensusNode() {}

    // the nodeID of the consensus node
    bcos::crypto::PublicPtr nodeID() const override { return m_nodeID; }

    uint64_t weight() const override { return m_weight; }

private:
    bcos::crypto::PublicPtr m_nodeID;
    uint64_t m_weight;
};

class RPCFakeLedger : public LedgerInterface, public std::enable_shared_from_this<RPCFakeLedger>
{
public:
    using Ptr = std::shared_ptr<RPCFakeLedger>;
    RPCFakeLedger() = default;
    RPCFakeLedger(bcostars::protocol::BlockFactoryImpl::Ptr _blockFactory, size_t, size_t, size_t)
      : m_blockFactory(_blockFactory),
        m_ledgerConfig(std::make_shared<bcos::ledger::LedgerConfig>())
    {
        // init(_blockNumber, _txsSize, _receiptsSize);
        // m_worker = std::make_shared<ThreadPool>("worker", 1);
    }
    virtual ~RPCFakeLedger() {}

    // void init(size_t _blockNumber, size_t _txsSize, int64_t _timestamp = utcTime())
    // {
    //     auto genesisBlock = init(nullptr, true, 0, 0, 0);
    //     m_ledger.push_back(genesisBlock);

    //     auto parentHeader = genesisBlock->blockHeader();
    //     for (size_t i = 1; i < _blockNumber; i++)
    //     {
    //         auto block = init(parentHeader, true, i, _txsSize, _timestamp);
    //         parentHeader = block->blockHeader();
    //         m_ledger.push_back(block);
    //     }
    //     auto latestBlock = m_ledger[m_ledger.size() - 1];
    //     updateLedgerConfig(latestBlock->blockHeader());
    // }

    void asyncPrewriteBlock(bcos::storage::StorageInterface::Ptr, bcos::protocol::Block::ConstPtr,
        std::function<void(Error::Ptr&&)>) override
    {}

    void asyncStoreTransactions(std::shared_ptr<std::vector<bytesConstPtr>>, crypto::HashListPtr,
        std::function<void(Error::Ptr)>) override
    {}

    bcostars::protocol::BlockImpl::Ptr createFakeBlock()
    {
        auto cryptoSuite =
            std::make_shared<bcos::crypto::CryptoSuite>(std::make_shared<bcos::test::Sm3Hash>(),
                std::make_shared<bcos::test::SM2SignatureImpl>(), nullptr);
        auto transactionFactory =
            std::make_shared<bcostars::protocol::TransactionFactoryImpl>(cryptoSuite);
        auto transactionReceiptFactory =
            std::make_shared<bcostars::protocol::TransactionReceiptFactoryImpl>(cryptoSuite);

        return std::make_shared<bcostars::protocol::BlockImpl>(
            transactionFactory, transactionReceiptFactory);
    }

    void asyncGetBlockDataByNumber(protocol::BlockNumber _blockNumber, int32_t,
        std::function<void(Error::Ptr, protocol::Block::Ptr)> _onGetBlock) override
    {
        if (_blockNumber == 6)
        {
            auto mockBlockPtr = createFakeBlock();
            return _onGetBlock(nullptr, mockBlockPtr);
        }
        return _onGetBlock(nullptr, nullptr);
    }

    void asyncGetBlockNumber(
        std::function<void(Error::Ptr, protocol::BlockNumber)> _onGetBlockNumber) override
    {
        _onGetBlockNumber(nullptr, 4);
    }

    void asyncGetBlockHashByNumber(protocol::BlockNumber _blockNumber,
        std::function<void(Error::Ptr, crypto::HashType const&)> _onGetBlockHash) override
    {
        if (_blockNumber == 5)
            return _onGetBlockHash(
                nullptr, h256("067150c07dab4facb7160e075548007e067150c07dab4facb7160e075548007e"));
        return _onGetBlockHash(
            nullptr, h256("0x0000000000000000000000000000000000000000000000000000000000000000"));
    }

    void asyncGetBlockNumberByHash(crypto::HashType const& _blockHash,
        std::function<void(Error::Ptr, protocol::BlockNumber)> _onGetBlockNumber) override
    {
        if (_blockHash ==
            h256("0x067150c07dab4facb7160e075548007e067150c07dab4facb7160e075548007e"))
            return _onGetBlockNumber(nullptr, 6);
        return _onGetBlockNumber(nullptr, -1);
    }

    void asyncGetBatchTxsByHashList(crypto::HashListPtr, bool,
        std::function<void(Error::Ptr, bcos::protocol::TransactionsPtr,
            std::shared_ptr<std::map<std::string, MerkleProofPtr>>)>) override
    {}

    void asyncGetTransactionReceiptByHash(crypto::HashType const&, bool,
        std::function<void(Error::Ptr, protocol::TransactionReceipt::ConstPtr, MerkleProofPtr)>)
        override
    {}

    void asyncGetTotalTransactionCount(std::function<void(Error::Ptr, int64_t _totalTxCount,
            int64_t _failedTxCount, protocol::BlockNumber _latestBlockNumber)>
            _onGetTransCnt) override
    {
        return _onGetTransCnt(nullptr, 5, 6, 7);
    }

    void asyncGetSystemConfigByKey(std::string const&,
        std::function<void(Error::Ptr, std::string, protocol::BlockNumber)> _onGetSysConf) override
    {
        return _onGetSysConf(nullptr, "mockSysConfBykey", 3);
    }

    void asyncGetNodeListByType(std::string const& _consensusType,
        std::function<void(Error::Ptr, consensus::ConsensusNodeListPtr)> _onGetNodeList) override
    {
        if (_consensusType == bcos::ledger::CONSENSUS_SEALER ||
            _consensusType == bcos::ledger::CONSENSUS_OBSERVER)
        {
            std::string node = "11123455";
            auto nodeId = std::make_shared<CommonKeyImpl>(bytes(node.begin(), node.end()));
            auto mockConsensusNodePtr = std::make_shared<FakeConsensusNode>(nodeId, 55);
            bcos::consensus::ConsensusNodeList consensusNodeList;
            consensusNodeList.emplace_back(mockConsensusNodePtr);
            auto respConsensusNodeListPtr =
                std::make_shared<consensus::ConsensusNodeList>(std::move(consensusNodeList));
            return _onGetNodeList(nullptr, respConsensusNodeListPtr);
        }
    }

    void asyncGetNonceList(protocol::BlockNumber, int64_t,
        std::function<void(Error::Ptr,
            std::shared_ptr<std::map<protocol::BlockNumber, protocol::NonceListPtr>>)>) override
    {}

private:
    bcostars::protocol::BlockFactoryImpl::Ptr m_blockFactory;
    // std::vector<KeyPairInterface::Ptr> m_keyPairVec;
    bcos::ledger::LedgerConfig::Ptr m_ledgerConfig;

    // size_t m_totalTxCount;
    // bool m_statusNormal = true;

    // std::vector<Block::Ptr> m_ledger;
    // std::map<HashType, BlockNumber> m_hash2Block;
    // SharedMutex x_ledger;

    // std::map<HashType, bytesConstPtr> m_txsHashToData;
    // SharedMutex x_txsHashToData;

    // std::vector<bytes> m_sealerList;
    // std::shared_ptr<ThreadPool> m_worker = nullptr;
};

class FakeGateway : public GatewayInterface
{
public:
    using Ptr = std::shared_ptr<FakeGateway>;
    FakeGateway() = default;
    virtual ~FakeGateway(){};

    void start() override {}
    void stop() override {}

public:
    void asyncGetNodeIDs(const std::string&, GetNodeIDsFunc) override {}
    void asyncSendMessageByNodeID(const std::string&, bcos::crypto::NodeIDPtr,
        bcos::crypto::NodeIDPtr, bytesConstRef, ErrorRespFunc) override
    {}
    void asyncSendMessageByNodeIDs(const std::string&, bcos::crypto::NodeIDPtr,
        const bcos::crypto::NodeIDs&, bytesConstRef) override
    {}
    void asyncSendBroadcastMessage(
        const std::string&, bcos::crypto::NodeIDPtr, bytesConstRef) override
    {}
    void asyncNotifyGroupInfo(
        bcos::group::GroupInfo::Ptr, std::function<void(Error::Ptr&&)>) override
    {}
    void asyncSendMessageByTopic(const std::string&, bcos::bytesConstRef,
        std::function<void(bcos::Error::Ptr&&, int16_t, bytesPointer)>) override
    {}
    void asyncSendBroadbastMessageByTopic(const std::string&, bcos::bytesConstRef) override {}
    void asyncSubscribeTopic(
        std::string const&, std::string const&, std::function<void(Error::Ptr&&)>) override
    {}
    void asyncRemoveTopic(std::string const&, std::vector<std::string> const&,
        std::function<void(Error::Ptr&&)>) override
    {}

    void asyncGetPeers(PeerRespFunc _onGetPeers) override
    {
        return _onGetPeers(nullptr, "mockPeersStringInfo");
    }
};

// class FakeTransactionReceipt : public bcos::protocol::TransactionReceipt
// {
// public:
//     FakeTransactionReceipt() = delete;
//     using Ptr = std::shared_ptr<FakeTransactionReceipt>;
//     using ConstPtr = std::shared_ptr<const FakeTransactionReceipt>;
//     FakeTransactionReceipt(bcos::crypto::CryptoSuite::Ptr _cryptoSuite) :
//     TransactionReceipt(_cryptoSuite)
//     {}

//     virtual ~FakeTransactionReceipt() {};

//     void decode(bytesConstRef) override {};
//     void encode(bytes&) const override {};
//     bytesConstRef encode(bool) const override;

//     // virtual bcos::crypto::HashType const& hash() const
//     // {
//     //     UpgradableGuard l(x_hash);
//     //     if (m_hash != bcos::crypto::HashType())
//     //     {
//     //         return m_hash;
//     //     }
//     //     UpgradeGuard ul(l);
//     //     auto hashFields = encode(true);
//     //     m_hash = m_cryptoSuite->hash(hashFields);
//     //     return m_hash;
//     // }

//     int32_t version() const override {}
//     u256 const& gasUsed() const override {}// TODO: remove from hash
//     std::string_view contractAddress() const override {}
//     int32_t status() const override {}
//     bytesConstRef output() const override {}
//     gsl::span<const bcos::protocol::LogEntry> logEntries() const override {}
//     BlockNumber blockNumber() const override {}

// // private:
// //         bcos::crypto::CryptoSuite::Ptr m_fakeCryptoSuite;
// };

class RpcFakeTxPool : public FakeTxPool
{
public:
    using Ptr = std::shared_ptr<RpcFakeTxPool>;
    RpcFakeTxPool() {}
    ~RpcFakeTxPool() override {}

    void start() override {}
    void stop() override {}

    void asyncGetPendingTransactionSize(
        std::function<void(Error::Ptr, size_t)> _onGetPendingTransSize) override
    {
        _onGetPendingTransSize(nullptr, 5);
    }

    void asyncSubmit(bytesPointer, TxSubmitCallback) override
    {
        // auto cryptoSuite =
        //     std::make_shared<bcos::crypto::CryptoSuite>(std::make_shared<bcos::test::Sm3Hash>(),
        //         std::make_shared<bcos::test::SM2SignatureImpl>(), nullptr);
        // auto txReceipt = std::make_shared<FakeTransactionReceipt>(cryptoSuite);
        // auto txSubmitResultPtr = std::make_shared<bcos::protocol::TransactionSubmitResultImpl>();
        // txSubmitResultPtr->setTransactionReceipt(txReceipt);

        // return _onSubmitTx(nullptr, txSubmitResultPtr);

        return;
    }
};

class FakeConsensus : public bcos::consensus::ConsensusInterface
{
public:
    using Ptr = std::shared_ptr<FakeConsensus>;
    FakeConsensus() = default;
    virtual ~FakeConsensus(){};

    void start() override {}
    void stop() override {}

public:
    void asyncSubmitProposal(bool, bytesConstRef, bcos::protocol::BlockNumber,
        bcos::crypto::HashType const&, std::function<void(Error::Ptr)>) override{};

    void asyncCheckBlock(
        bcos::protocol::Block::Ptr, std::function<void(Error::Ptr, bool)>) override{};
    void asyncNotifyNewBlock(
        bcos::ledger::LedgerConfig::Ptr, std::function<void(Error::Ptr)>) override{};

    void asyncNotifyConsensusMessage(bcos::Error::Ptr, std::string const&, bcos::crypto::NodeIDPtr,
        bytesConstRef, std::function<void(Error::Ptr _error)>) override{};

    void notifyHighestSyncingNumber(bcos::protocol::BlockNumber) override{};

    void asyncNoteUnSealedTxsSize(size_t, std::function<void(Error::Ptr)>) override{};

    ConsensusNodeList consensusNodeList() const override { return ConsensusNodeList(); }

    void asyncGetPBFTView(
        std::function<void(Error::Ptr, bcos::consensus::ViewType)> _onGetPBFTView) override
    {
        return _onGetPBFTView(nullptr, 113);
    };
};

class FakeBlockSync : public bcos::sync::BlockSyncInterface
{
public:
    using Ptr = std::shared_ptr<FakeBlockSync>;
    FakeBlockSync() = default;
    virtual ~FakeBlockSync(){};

    void start() override {}
    void stop() override {}

public:
    // called by the consensus module when commit a new block
    void asyncNotifyNewBlock(
        bcos::ledger::LedgerConfig::Ptr, std::function<void(Error::Ptr)>) override{};

    // called by the frontService to dispatch message
    void asyncNotifyBlockSyncMessage(Error::Ptr, std::string const&, bcos::crypto::NodeIDPtr,
        bytesConstRef, std::function<void(Error::Ptr _error)>) override{};

    void asyncNotifyCommittedIndex(
        bcos::protocol::BlockNumber, std::function<void(Error::Ptr _error)>) override{};

    void notifyConnectedNodes(
        bcos::crypto::NodeIDSet const&, std::function<void(Error::Ptr)>) override{};

    // called by the RPC to get the sync status
    void asyncGetSyncInfo(std::function<void(Error::Ptr, std::string)> _onGetSyncInfo) override
    {
        return _onGetSyncInfo(nullptr, "mockSyncInfo");
    };
};

// class FakeBlockFactory : public BlockFactory
// {
// public:
//     using Ptr = std::shared_ptr<FakeBlockFactory>;
//     FakeBlockFactory() = default;
//     virtual ~FakeBlockFactory() {};

//     Block::Ptr createBlock() override { return std::make_shared<bcos::protocol::Block>(); }
//     Block::Ptr createBlock(
//         bytes const&, bool, bool) noexcept override { return
//         std::make_shared<bcos::protocol::Block>(); };
//     Block::Ptr createBlock(
//         bytesConstRef, bool, bool) noexcept override { return
//         std::make_shared<bcos::protocol::Block>(); };

//     TransactionMetaData::Ptr createTransactionMetaData() noexcept override { return
//     std::make_shared<bcos::protocol::TransactionMetaData>(); }; TransactionMetaData::Ptr
//     createTransactionMetaData(
//         bcos::crypto::HashType const, std::string const&) noexcept override { return
//         std::make_shared<bcos::protocol::TransactionMetaData>(); };

//     bcos::crypto::CryptoSuite::Ptr cryptoSuite() noexcept override { return
//     std::make_shared<bcos::crypto::CryptoSuite>(); }; BlockHeaderFactory::Ptr
//     blockHeaderFactory() noexcept override { return
//     std::make_shared<bcos::protocol::BlockHeaderFactory>(); }; TransactionFactory::Ptr
//     transactionFactory() override
//     {
//         return std::make_shared<bcostars::protocol::TransactionFactoryImpl>();
//     };
//     TransactionReceiptFactory::Ptr receiptFactory() noexcept override { return
//     std::make_shared<bcos::protocol::TransactionReceiptFactory>(); };
// };

class FakeGroupManager : public GroupManager
{
public:
    using Ptr = std::shared_ptr<FakeGroupManager>;
    FakeGroupManager(std::string const& _groupID, std::string const& _nodeName)
      : GroupManager("", nullptr)
    {
        auto m_fakeBlockFactory = createFakeBlockFactory();
        auto fakeLeder = std::make_shared<RPCFakeLedger>(m_fakeBlockFactory, 4, 8, 9);
        // auto fakeSchedule = std::make_shared<FakeScheduler>(fakeLeder, m_fakeBlockFactory);
        auto fakeTxpool = std::make_shared<RpcFakeTxPool>();
        auto fakeConsensus = std::make_shared<FakeConsensus>();
        auto fakeBlockSync = std::make_shared<FakeBlockSync>();

        // m_nodeService = std::make_shared<bcos::rpc::NodeService>(fakeLeder, fakeSchedule,
        // fakeTxpool, fakeConsensus, fakeBlockSync, m_fakeBlockFactory);
        m_nodeService = std::make_shared<bcos::rpc::NodeService>(
            fakeLeder, nullptr, fakeTxpool, fakeConsensus, fakeBlockSync, nullptr);
        m_groupID = _groupID;
        m_nodeName = _nodeName;

        initGroupInfos(_groupID, _nodeName);
    }

    bcos::rpc::NodeService::Ptr getNodeService(
        std::string const& _groupID, std::string const&) const override
    {
        if (_groupID == m_groupID)
        {
            return m_nodeService;
        }
        return nullptr;
    }

    bcostars::protocol::BlockFactoryImpl::Ptr createFakeBlockFactory()
    {
        auto cryptoSuite =
            std::make_shared<bcos::crypto::CryptoSuite>(std::make_shared<bcos::test::Sm3Hash>(),
                std::make_shared<bcos::test::SM2SignatureImpl>(), nullptr);
        auto blockHeaderFactory =
            std::make_shared<bcostars::protocol::BlockHeaderFactoryImpl>(cryptoSuite);
        auto transactionFactory =
            std::make_shared<bcostars::protocol::TransactionFactoryImpl>(cryptoSuite);
        auto transactionReceiptFactory =
            std::make_shared<bcostars::protocol::TransactionReceiptFactoryImpl>(cryptoSuite);

        return std::make_shared<bcostars::protocol::BlockFactoryImpl>(
            cryptoSuite, blockHeaderFactory, transactionFactory, transactionReceiptFactory);
    }

    void initGroupInfos(std::string const& _groupID, std::string const& _nodeName)
    {
        auto fakeNodeInfo = std::make_shared<bcos::group::ChainNodeInfo>(_nodeName, 0);
        auto fakeGroupInfo = std::make_shared<bcos::group::GroupInfo>("", _groupID);

        fakeGroupInfo->setIniConfig("fakeIniConf");
        fakeGroupInfo->setGenesisConfig("fakeGenesisConf");
        fakeGroupInfo->updateNodeInfo(fakeNodeInfo);
        appendGroupInfo(fakeGroupInfo);
    }

    bcos::group::GroupInfo::Ptr getGroupInfo(std::string const& _groupID) override
    {
        if (_groupID == m_groupID)
            return m_groupInfos[_groupID];
        return nullptr;
    }

    bool appendGroupInfo(bcos::group::GroupInfo::Ptr _groupInfo)
    {
        if (_groupInfo)
        {
            UpgradableGuard l(x_groupInfos);
            auto const& groupID = _groupInfo->groupID();
            if (m_groupInfos.count(groupID))
            {
                return false;
            }
            UpgradeGuard ul(l);
            m_groupInfos[groupID] = _groupInfo;
            return true;
        }
        return false;
    }

private:
    bcos::rpc::NodeService::Ptr m_nodeService;
    std::string m_groupID;
    std::string m_nodeName;

    mutable SharedMutex x_groupInfos;
};

}  // namespace test
}  // namespace bcos
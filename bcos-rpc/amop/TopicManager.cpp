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
 * @file TopicManager.cpp
 * @author: octopus
 * @date 2021-06-18
 */

#include <bcos-rpc/amop/Common.h>
#include <bcos-rpc/amop/TopicManager.h>
#include <json/json.h>

using namespace bcos;
using namespace bcos::amop;

/**
 * @brief: parse client sub topics json
 * @param _topicItems: return value, topics
 * @param _json: json
 * @return void
 */
bool TopicManager::parseSubTopicsJson(const std::string& _json, TopicItems& _topicItems)
{
    Json::Value root;
    Json::Reader jsonReader;

    try
    {
        if (!jsonReader.parse(_json, root))
        {
            TOPIC_LOG(ERROR) << "parseSubTopicsJson unable to parse json" << LOG_KV("json:", _json);
            return false;
        }

        TopicItems topicItems;

        auto topicItemsSize = root["topics"].size();

        for (unsigned int i = 0; i < topicItemsSize; i++)
        {
            std::string topic = root["topics"][i].asString();
            topicItems.insert(TopicItem(topic));
        }

        _topicItems = topicItems;

        TOPIC_LOG(INFO) << LOG_DESC("parseSubTopicsJson")
                        << LOG_KV("topicItems size", topicItems.size()) << LOG_KV("json", _json);
        return true;
    }
    catch (const std::exception& e)
    {
        TOPIC_LOG(ERROR) << LOG_DESC("parseSubTopicsJson: " + boost::diagnostic_information(e))
                         << LOG_KV("json:", _json);
        return false;
    }
}

/**
 * @brief: client subscribe topic
 * @param _clientID: client identify, to be defined
 * @param _topicJson: topics subscribe by client
 * @return void
 */
void TopicManager::subTopic(const std::string& _client, const std::string& _topicJson)
{
    TopicItems topicItems;
    // parser topic json
    if (parseSubTopicsJson(_topicJson, topicItems))
    {
        subTopic(_client, topicItems);
    };
}

/**
 * @brief: client subscribe topic
 * @param _clientID: client identify, to be defined
 * @param _topicItems: topics subscribe by client
 * @return void
 */
void TopicManager::subTopic(const std::string& _client, const TopicItems& _topicItems)
{
    {
        std::unique_lock lock(x_clientTopics);
        m_client2TopicItems[_client] = _topicItems;  // Override the previous value
        incTopicSeq();
    }

    TOPIC_LOG(INFO) << LOG_DESC("subTopic") << LOG_KV("client", _client)
                    << LOG_KV("topicSeq", topicSeq())
                    << LOG_KV("topicItems size", _topicItems.size());
}

/**
 * @brief: query topics sub by client
 * @param _clientID: client identify, to be defined
 * @param _topicItems: topics subscribe by client
 * @return void
 */
bool TopicManager::queryTopicItemsByClient(const std::string& _client, TopicItems& _topicItems)
{
    bool result = false;
    {
        std::shared_lock lock(x_clientTopics);
        auto it = m_client2TopicItems.find(_client);
        if (it != m_client2TopicItems.end())
        {
            _topicItems = it->second;
            result = true;
        }
    }

    TOPIC_LOG(INFO) << LOG_DESC("queryTopicItemsByClient") << LOG_KV("client", _client)
                    << LOG_KV("result", result) << LOG_KV("topicItems size", _topicItems.size());
    return result;
}

/**
 * @brief: clear all topics subscribe by client
 * @param _clientID: client identify, to be defined
 * @return void
 */
void TopicManager::removeTopicsByClient(const std::string& _client)
{
    {
        std::unique_lock lock(x_clientTopics);
        m_client2TopicItems.erase(_client);
        incTopicSeq();
    }

    TOPIC_LOG(INFO) << LOG_DESC("removeTopicsByClient") << LOG_KV("client", _client)
                    << LOG_KV("topicSeq", topicSeq());
}

/**
 * @brief: query topics subscribe by all connected clients
 * @return result in json format
 */
std::string TopicManager::queryTopicsSubByClient()
{
    try
    {
        uint32_t seq;
        TopicItems topicItems;
        {
            std::shared_lock lock(x_clientTopics);
            seq = topicSeq();
            for (const auto& m : m_client2TopicItems)
            {
                topicItems.insert(m.second.begin(), m.second.end());
            }
        }

        Json::Value jTopics = Json::Value(Json::arrayValue);
        for (const auto& topicItem : topicItems)
        {
            jTopics.append(topicItem.topicName());
        }

        Json::Value jResp;
        jResp["topicSeq"] = seq;
        jResp["topicItems"] = jTopics;

        Json::FastWriter writer;
        std::string topicJson = writer.write(jResp);

        TOPIC_LOG(DEBUG) << LOG_DESC("queryTopicsSubByClient") << LOG_KV("topicSeq", seq)
                         << LOG_KV("topicJson", topicJson);
        return topicJson;
    }
    catch (const std::exception& e)
    {
        TOPIC_LOG(ERROR) << LOG_DESC("queryTopicsSubByClient")
                         << LOG_KV("error", boost::diagnostic_information(e));
        return "";
    }
}

/**
 * @brief: parse json to fetch topicSeq and topicItems
 * @param _topicSeq: topicSeq
 * @param _topicItems: topics
 * @param _json: json
 * @return void
 */
bool TopicManager::parseTopicItemsJson(
    uint32_t& _topicSeq, TopicItems& _topicItems, const std::string& _json)
{
    Json::Value root;
    Json::Reader jsonReader;

    try
    {
        if (!jsonReader.parse(_json, root))
        {
            TOPIC_LOG(ERROR) << "parseTopicItemsJson unable to parse json"
                             << LOG_KV("json:", _json);
            return false;
        }

        uint32_t topicSeq;
        TopicItems topicItems;

        topicSeq = root["topicSeq"].asUInt();
        auto topicItemsSize = root["topicItems"].size();

        for (unsigned int i = 0; i < topicItemsSize; i++)
        {
            std::string topic = root["topicItems"][i].asString();
            topicItems.insert(TopicItem(topic));
        }

        _topicSeq = topicSeq;
        _topicItems = topicItems;

        TOPIC_LOG(INFO) << LOG_DESC("parseTopicItemsJson") << LOG_KV("topicSeq", topicSeq)
                        << LOG_KV("topicItems size", topicItems.size()) << LOG_KV("json", _json);
        return true;
    }
    catch (const std::exception& e)
    {
        TOPIC_LOG(ERROR) << LOG_DESC("parseTopicItemsJson: " + boost::diagnostic_information(e))
                         << LOG_KV("json:", _json);
        return false;
    }
}

/**
 * @brief: check if the topicSeq of nodeID changed
 * @param _nodeID: the peer nodeID
 * @param _topicSeq: the topicSeq of the nodeID
 * @return bool: if the nodeID has been changed
 */
bool TopicManager::checkTopicSeq(bcos::crypto::NodeIDPtr _nodeID, uint32_t _topicSeq)
{
    std::shared_lock lock(x_topics);
    auto it = m_nodeID2TopicSeq.find(_nodeID);
    if (it != m_nodeID2TopicSeq.end() && it->second == _topicSeq)
    {
        return false;
    }
    return true;
}

/**
 * @brief: update online nodeIDs, clean up the offline nodeIDs state
 * @param _nodeIDs: the online nodeIDs
 * @return void
 */
void TopicManager::notifyNodeIDs(const bcos::crypto::NodeIDs& _nodeIDs)
{
    int removeCount = 0;
    {
        std::unique_lock lock(x_topics);
        for (auto it = m_nodeID2TopicSeq.begin(); it != m_nodeID2TopicSeq.end();)
        {
            if (std::find_if(_nodeIDs.begin(), _nodeIDs.end(),
                    [&it](bcos::crypto::NodeIDPtr _nodeID) -> bool {
                        return it->first->hex() == _nodeID->hex();
                    }) == _nodeIDs.end())
            {  // nodeID is offline, remove the nodeID's state
                it = m_nodeID2TopicSeq.erase(it);
                m_nodeID2TopicItems.erase(it->first);
                removeCount++;
            }
            else
            {
                ++it;
            }
        }
    }

    TOPIC_LOG(INFO) << LOG_DESC("notifyNodeIDs") << LOG_KV("removeCount", removeCount);
}

/**
 * @brief: update the topicSeq and topicItems of the nodeID's
 * @param _nodeID: nodeID
 * @param _topicSeq: topicSeq
 * @param _topicItems: topicItems
 * @return void
 */
void TopicManager::updateSeqAndTopicsByNodeID(
    bcos::crypto::NodeIDPtr _nodeID, uint32_t _topicSeq, const TopicItems& _topicItems)
{
    {
        std::unique_lock lock(x_topics);
        m_nodeID2TopicSeq[_nodeID] = _topicSeq;
        m_nodeID2TopicItems[_nodeID] = _topicItems;
    }

    TOPIC_LOG(INFO) << LOG_DESC("updateSeqAndTopicsByNodeID") << LOG_KV("nodeID", _nodeID->hex())
                    << LOG_KV("topicSeq", _topicSeq)
                    << LOG_KV("topicItems size", _topicItems.size());
}

/**
 * @brief: find the nodeIDs by topic
 * @param _topic: topic
 * @param _nodeIDs: nodeIDs
 * @return void
 */
void TopicManager::queryNodeIDsByTopic(const std::string& _topic, bcos::crypto::NodeIDs& _nodeIDs)
{
    std::shared_lock lock(x_topics);
    for (auto it = m_nodeID2TopicItems.begin(); it != m_nodeID2TopicItems.end(); ++it)
    {
        auto findIt = std::find_if(it->second.begin(), it->second.end(),
            [_topic](const TopicItem& _topicItem) { return _topic == _topicItem.topicName(); });
        if (findIt != it->second.end())
        {
            _nodeIDs.push_back(it->first);
        }
    }
    return;
}
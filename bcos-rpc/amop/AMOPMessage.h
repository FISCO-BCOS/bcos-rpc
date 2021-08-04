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
 * @file AMOPMessage.h
 * @author: octopus
 * @date 2021-06-17
 */
#pragma once
#include <bcos-framework/libutilities/Common.h>

namespace bcos
{
namespace amop
{
enum AMOPMessageType : uint16_t
{
    TopicSeq = 0x1,
    RequestTopic = 0x2,
    ResponseTopic = 0x3,
    AMOPRequest = 0x4,
    AMOPBroadcast = 0x5
};

class AMOPMessage
{
public:
    using Ptr = std::shared_ptr<AMOPMessage>;

    /// type(2) + topic length(2) + topic + data
    const static size_t HEADER_LENGTH = 4;
    /// the max length of topic(65535)
    const static size_t MAX_TOPIC_LENGTH = 0xffff;

public:
    AMOPMessage() { m_data = bytesConstRef(); }

public:
    uint16_t type() const { return m_type; }
    void setType(uint16_t _type) { m_type = _type; }

    std::string topic() const { return m_topic; }
    void setTopic(const std::string& _topic) { m_topic = _topic; }

    bytesConstRef data() const { return m_data; }
    void setData(bcos::bytesConstRef _data) { m_data = _data; }

public:
    bool encode(bytes& _buffer);
    ssize_t decode(bytesConstRef _buffer);

private:
    uint16_t m_type{0};
    std::string m_topic;
    bcos::bytesConstRef m_data;
};
class MessageFactory
{
public:
    using Ptr = std::shared_ptr<MessageFactory>;

public:
    AMOPMessage::Ptr buildMessage() { return std::make_shared<AMOPMessage>(); }
};
}  // namespace amop
}  // namespace bcos

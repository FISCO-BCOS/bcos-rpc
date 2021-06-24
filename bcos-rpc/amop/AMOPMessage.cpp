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
 * @file AMOPMessage.cpp
 * @author: octopus
 * @date 2021-06-21
 */

#include <bcos-rpc/amop/AMOPMessage.h>
#include <boost/asio.hpp>

using namespace bcos;
using namespace bcos::amop;

void AMOPMessage::encode(bcos::bytes& _buffer)
{
    if (m_topic.size() > MAX_TOPIC_LENGTH)
    {
        throw std::length_error(
            "the topic length longer than the maximum allowed(65535), topic size:" +
            std::to_string(m_topic.size()));
    }

    uint16_t type = boost::asio::detail::socket_ops::host_to_network_short(m_type);
    uint16_t length =
        boost::asio::detail::socket_ops::host_to_network_short((uint16_t)m_topic.size());
    _buffer.insert(_buffer.end(), (byte*)&type, (byte*)&type + 2);
    _buffer.insert(_buffer.end(), (byte*)&length, (byte*)&length + 2);
    if (!m_topic.empty())
    {
        _buffer.insert(_buffer.end(), m_topic.begin(), m_topic.end());
    }
    _buffer.insert(_buffer.end(), m_data.begin(), m_data.end());
}

ssize_t AMOPMessage::decode(bcos::bytesConstRef _buffer)
{
    if (_buffer.size() < HEADER_LENGTH)
    {
        return -1;
    }

    std::size_t offset = 0;
    m_type = boost::asio::detail::socket_ops::network_to_host_short(
        *((uint16_t*)(_buffer.data() + offset)));
    offset += 2;
    uint16_t length = boost::asio::detail::socket_ops::network_to_host_short(
        *((uint16_t*)(_buffer.data() + offset)));
    offset += 2;
    if (length > 0)
    {
        m_topic = std::string(_buffer.data() + offset, _buffer.data() + offset + length);
        offset += length;
    }
    m_data = _buffer.getCroppedData(offset);

    return _buffer.size();
}
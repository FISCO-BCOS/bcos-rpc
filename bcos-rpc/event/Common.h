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
 * along with FISCO-BCOS.  If not, see <http:www.gnu.org/licenses/>
 * (c) 2019-2021 fisco-dev contributors.
 *
 * @file Common.h
 * @author: octopuswang
 * @date 2019-08-13
 */

#pragma once

#include <bcos-framework/libutilities/Log.h>

// The largest number of topic in one event log
#define EVENT_LOG_TOPICS_MAX_INDEX (4)

#define EVENT_REQUEST(LEVEL) BCOS_LOG(LEVEL) << "[EVENT][REQUEST]"
#define EVENT_RESPONSE(LEVEL) BCOS_LOG(LEVEL) << "[EVENT][RESPONSE]"
#define EVENT_TASK(LEVEL) BCOS_LOG(LEVEL) << "[EVENT][TASK]"
#define EVENT_GROUP(LEVEL) BCOS_LOG(LEVEL) << "[EVENT][GROUP]"
#define EVENT_PUSH(LEVEL) BCOS_LOG(LEVEL) << "[EVENT][PUSH]"


namespace bcos
{
namespace event
{
enum EP_STATUS_CODE
{
    SUCCESS = 0,
    PUSH_COMPLETED = 1,
    INVALID_PARAMS = -41000,
    INVALID_REQUEST = -41001,
    GROUP_NOT_EXIST = -41002,
    INVALID_REQUEST_RANGE = -41003,
    INVALID_RESPONSE = -41004,
    REQUST_TIMEOUT = -41005,
    SDK_PERMISSION_DENIED = -41006,
    NONEXISTENT_EVENT = -41007,
};

}  // namespace event
}  // namespace bcos

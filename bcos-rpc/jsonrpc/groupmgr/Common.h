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
 * @brief Common.h
 * @file Common.h
 * @author: yujiechen
 * @date 2021-10-11
 */
#pragma once
#include <bcos-crypto/encrypt/AESCrypto.h>
#include <bcos-crypto/encrypt/SM4Crypto.h>
#include <bcos-crypto/hash/Keccak256.h>
#include <bcos-crypto/hash/SM3.h>
#include <bcos-crypto/signature/secp256k1/Secp256k1Crypto.h>
#include <bcos-crypto/signature/sm2/SM2Crypto.h>
namespace bcos
{
namespace rpc
{
inline bcos::crypto::CryptoSuite::Ptr createCryptoSuite()
{
    auto hashImpl = std::make_shared<bcos::crypto::Keccak256>();
    auto signatureImpl = std::make_shared<bcos::crypto::Secp256k1Crypto>();
    auto encryptImpl = std::make_shared<bcos::crypto::AESCrypto>();
    return std::make_shared<bcos::crypto::CryptoSuite>(hashImpl, signatureImpl, encryptImpl);
}

inline bcos::crypto::CryptoSuite::Ptr createSMCryptoSuite()
{
    auto hashImpl = std::make_shared<bcos::crypto::SM3>();
    auto signatureImpl = std::make_shared<bcos::crypto::SM2Crypto>();
    auto encryptImpl = std::make_shared<bcos::crypto::SM4Crypto>();
    return std::make_shared<bcos::crypto::CryptoSuite>(hashImpl, signatureImpl, encryptImpl);
}
}  // namespace rpc
}  // namespace bcos
#------------------------------------------------------------------------------
# Top-level CMake file for bcos-rpc
# ------------------------------------------------------------------------------
# Copyright (C) 2021 bcos-rpc
# SPDX-License-Identifier: Apache-2.0
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.10)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.3" CACHE STRING "Minimum OS X deployment version")
include(FetchContent)
if (NOT DEFINED URL_BASE)
    # set(URL_BASE "github.com")
    set(URL_BASE "github.com.cnpmjs.org")
endif()
set(BCOS_CMAKE_SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bcos-cmake-scripts)
FetchContent_Declare(bcos-cmake-scripts
    GIT_REPOSITORY https://${URL_BASE}/FISCO-BCOS/bcos-cmake-scripts.git
    GIT_TAG 9b2ac5b979cbc97f640b2285feb2c74a18cfb935
    SOURCE_DIR ${BCOS_CMAKE_SCRIPTS_DIR}
)
FetchContent_MakeAvailable(bcos-cmake-scripts)

# set(HUNTER_PACKAGE_LOG_BUILD ON CACHE STRING "Hunter output build info to log")
# set(HUNTER_PACKAGE_LOG_INSTALL ON CACHE STRING "Hunter output install info to log")
# set(HUNTER_STATUS_DEBUG OFF CACHE STRING "Hunter debug output")
# set(HUNTER_USE_CACHE_SERVERS NO CACHE STRING "Download cache from cache server" FORCE)
# set(HUNTER_STATUS_PRINT OFF CACHE STRING "Hunter status output")
# set(HUNTER_CONFIGURATION_TYPES Release CACHE STRING "Build type of Hunter packages")

set(BCOS_RPC_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake" CACHE PATH "The cmake path for the library")
list(APPEND CMAKE_MODULE_PATH ${BCOS_CMAKE_SCRIPTS_DIR})
list(APPEND CMAKE_MODULE_PATH ${BCOS_RPC_CMAKE_DIR})
#init hunter
include(HunterGate)
HunterGate(
    URL "https://${URL_BASE}/FISCO-BCOS/hunter/archive/203039fdd0826330facd9c3841082dbf35d0c808.tar.gz"
    SHA1 "26ca1840eb26da410913469f2f278569c8d383ae"
    FILEPATH "${CMAKE_SOURCE_DIR}/cmake/config.cmake"
)
project(bcos-rpc VERSION "3.0.0")
set(VERSION_SUFFIX "")

# install dependencies
include(InstallBcosFrameworkDependencies)
include(InstallBcosTarsProtocolDependencies)
include(InstallBcosCryptoDependencies)
include(InstallBoostSSL)

hunter_add_package(jsoncpp)
find_package(jsoncpp CONFIG REQUIRED)

# basic settings
include(Options)
configure_project()
include(CompilerSettings)
include(InstallConfig)
# install the include files for hash
include(InstallInclude)

include_directories(${CMAKE_INSTALL_INCLUDEDIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(RPC_TARGET "rpc")
add_subdirectory(bcos-rpc)

if (TESTS)
    enable_testing()
    set(CTEST_OUTPUT_ON_FAILURE TRUE)
    add_subdirectory(test)
endif()

# install bcos rpc target
install(
    TARGETS ${RPC_TARGET}
    EXPORT "${TARGETS_EXPORT_NAME}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

# for doxygen
include(BuildDocs)

# for code coverage
if (COVERAGE)
    include(Coverage)
    config_coverage("rpc-coverage" "'/usr*' '${CMAKE_CURRENT_SOURCE_DIR}/bcos-cmake-scripts*' '${CMAKE_SOURCE_DIR}/test/mock**' '${CMAKE_SOURCE_DIR}/test/main**'")
endif ()

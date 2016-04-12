# Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

############################# Check minimum CMake version #####################

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT("wrt-service")

############################# cmake packages ##################################

INCLUDE(FindPkgConfig)

############################# build type ######################################

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "Release")
ENDIF(NOT CMAKE_BUILD_TYPE)

############################# compilation defines #############################

IF(DEVICE_PROFILE STREQUAL "WEARABLE")
    MESSAGE(STATUS "Device Profile: WEARABLE")
    ADD_DEFINITIONS("-DDEVICE_PROFILE_WEARABLE")
ELSEIF(DEVICE_PROFILE STREQUAL "MOBILE")
    MESSAGE(STATUS "Device Profile: MOBILE")
    ADD_DEFINITIONS("-DDEVICE_PROFILE_MOBILE")
ELSEIF(DEVICE_PROFILE STREQUAL "TV")
    MESSAGE(STATUS "Device Profile: TV")
    ADD_DEFINITIONS("-DDEVICE_PROFILE_TV")
ENDIF()

############################# device arch ##################################

IF(DEVICE_ARCH STREQUAL "AARCH64")
    MESSAGE(STATUS "Device Arch: 64bit")
    ADD_DEFINITIONS("-DDEVICE_ARCH_64_BIT")
ENDIF(DEVICE_ARCH)

############################# compiler flags ##################################

SET(CMAKE_C_FLAGS_PROFILING    "-O2")
SET(CMAKE_CXX_FLAGS_PROFILING  "-O2 -std=c++0x")
SET(CMAKE_C_FLAGS_DEBUG        "-O0 -g")
SET(CMAKE_CXX_FLAGS_DEBUG      "-O0 -std=c++0x -g")
SET(CMAKE_C_FLAGS_RELEASE      "-O2 -g")
SET(CMAKE_CXX_FLAGS_RELEASE    "-O2 -std=c++0x -g -fvisibility-inlines-hidden")
SET(CMAKE_CXX_FLAGS_CCOV       "-O0 -std=c++0x -g --coverage")

ADD_DEFINITIONS("-fPIC")

# Set compiler warning flags
#ADD_DEFINITIONS("-Werror")              # Make all warnings into errors.
ADD_DEFINITIONS("-Wall")                # Generate all warnings
ADD_DEFINITIONS("-Wextra")              # Generate even more extra warnings
ADD_DEFINITIONS("-Wno-variadic-macros") # Inhibit variadic macros warnings (needed for ORM)
ADD_DEFINITIONS("-Wno-deprecated")      # No warnings about deprecated features
ADD_DEFINITIONS("-std=c++0x")           # accept C++11x standard

############################# target names ####################################

SET(TARGET_SERVICE "wrt-service")
SET(TARGET_NODE_NATIVE "native")
SET(TARGET_NODE_GCONTEXT "gcontext")
SET(TARGET_NODE_APPFW "appfw")
SET(TARGET_NODE_SERVICEUTIL "serviceutil")
SET(TARGET_NODE_DLOG "nodedlog")

############################# subdirectories ##################################
ADD_SUBDIRECTORY(src)


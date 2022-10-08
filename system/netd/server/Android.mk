# Copyright (C) 2014 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
        $(call include-path-for, libhardware_legacy)/hardware_legacy \
        bionic/libc/dns/include \
        external/libcxx/include \
        external/mdnsresponder/mDNSShared \
        external/openssl/include \
        system/netd/include \

LOCAL_CLANG := true
LOCAL_CPPFLAGS := -std=c++11 -Wall -Werror
LOCAL_MODULE := netd

LOCAL_SHARED_LIBRARIES := \
        libcrypto \
        libcutils \
        libdl \
        libhardware_legacy \
        liblog \
        liblogwrap \
        libmdnssd \
        libnetutils \
        libsysutils \

LOCAL_SRC_FILES := \
        BandwidthController.cpp \
        ClatdController.cpp \
        CommandListener.cpp \
        DnsProxyListener.cpp \
        FirewallController.cpp \
        FwmarkServer.cpp \
        IdletimerController.cpp \
        InterfaceController.cpp \
        LocalNetwork.cpp \
        MDnsSdListener.cpp \
        NatController.cpp \
        NetdCommand.cpp \
        NetdConstants.cpp \
        NetlinkHandler.cpp \
        NetlinkManager.cpp \
        Network.cpp \
        NetworkController.cpp \
        PhysicalNetwork.cpp \
        PppController.cpp \
        ResolverController.cpp \
        RouteController.cpp \
        SoftapController.cpp \
        TetherController.cpp \
        UidRanges.cpp \
        VirtualNetwork.cpp \
        main.cpp \
        oem_iptables_hook.cpp \

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_CFLAGS := -Wall -Werror
LOCAL_CLANG := true
LOCAL_MODULE := ndc
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_SRC_FILES := ndc.c

include $(BUILD_EXECUTABLE)

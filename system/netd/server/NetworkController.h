/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NETD_SERVER_NETWORK_CONTROLLER_H
#define NETD_SERVER_NETWORK_CONTROLLER_H

#include "NetdConstants.h"
#include "Permission.h"

#include "utils/RWLock.h"

#include <list>
#include <map>
#include <set>
#include <sys/types.h>
#include <vector>

class Network;
class UidRanges;
class VirtualNetwork;

/*
 * Keeps track of default, per-pid, and per-uid-range network selection, as
 * well as the mark associated with each network. Networks are identified
 * by netid. In all set* commands netid == 0 means "unspecified" and is
 * equivalent to clearing the mapping.
 */
class NetworkController {
public:
    static const unsigned MIN_OEM_ID;
    static const unsigned MAX_OEM_ID;
    static const unsigned LOCAL_NET_ID;

    NetworkController();

    unsigned getDefaultNetwork() const;
    int setDefaultNetwork(unsigned netId) WARN_UNUSED_RESULT;

    // Sets |*netId| to an appropriate NetId to use for DNS for the given user. Call with |*netId|
    // set to a non-NETID_UNSET value if the user already has indicated a preference. Returns the
    // fwmark value to set on the socket when performing the DNS request.
    uint32_t getNetworkForDns(unsigned* netId, uid_t uid) const;
    unsigned getNetworkForUser(uid_t uid) const;
    unsigned getNetworkForConnect(uid_t uid) const;
    unsigned getNetworkForInterface(const char* interface) const;
    bool isVirtualNetwork(unsigned netId) const;

    int createPhysicalNetwork(unsigned netId, Permission permission) WARN_UNUSED_RESULT;
    int createVirtualNetwork(unsigned netId, bool hasDns, bool secure) WARN_UNUSED_RESULT;
    int destroyNetwork(unsigned netId) WARN_UNUSED_RESULT;

    int addInterfaceToNetwork(unsigned netId, const char* interface) WARN_UNUSED_RESULT;
    int removeInterfaceFromNetwork(unsigned netId, const char* interface) WARN_UNUSED_RESULT;

    Permission getPermissionForUser(uid_t uid) const;
    void setPermissionForUsers(Permission permission, const std::vector<uid_t>& uids);
    int checkUserNetworkAccess(uid_t uid, unsigned netId) const;
    int setPermissionForNetworks(Permission permission,
                                 const std::vector<unsigned>& netIds) WARN_UNUSED_RESULT;

    int addUsersToNetwork(unsigned netId, const UidRanges& uidRanges) WARN_UNUSED_RESULT;
    int removeUsersFromNetwork(unsigned netId, const UidRanges& uidRanges) WARN_UNUSED_RESULT;

    // |nexthop| can be NULL (to indicate a directly-connected route), "unreachable" (to indicate a
    // route that's blocked), "throw" (to indicate the lack of a match), or a regular IP address.
    //
    // Routes are added to tables determined by the interface, so only |interface| is actually used.
    // |netId| is given only to sanity check that the interface has the correct netId.
    int addRoute(unsigned netId, const char* interface, const char* destination,
                 const char* nexthop, bool legacy, uid_t uid) WARN_UNUSED_RESULT;
    int removeRoute(unsigned netId, const char* interface, const char* destination,
                    const char* nexthop, bool legacy, uid_t uid) WARN_UNUSED_RESULT;

    bool canProtect(uid_t uid) const;
    void allowProtect(const std::vector<uid_t>& uids);
    void denyProtect(const std::vector<uid_t>& uids);

private:
    bool isValidNetwork(unsigned netId) const;
    Network* getNetworkLocked(unsigned netId) const;
    VirtualNetwork* getVirtualNetworkForUserLocked(uid_t uid) const;
    Permission getPermissionForUserLocked(uid_t uid) const;
    int checkUserNetworkAccessLocked(uid_t uid, unsigned netId) const;

    int modifyRoute(unsigned netId, const char* interface, const char* destination,
                    const char* nexthop, bool add, bool legacy, uid_t uid) WARN_UNUSED_RESULT;
    int modifyFallthroughLocked(unsigned vpnNetId, bool add) WARN_UNUSED_RESULT;

    class DelegateImpl;
    DelegateImpl* const mDelegateImpl;

    // mRWLock guards all accesses to mDefaultNetId, mNetworks, mUsers and mProtectableUsers.
    mutable android::RWLock mRWLock;
    unsigned mDefaultNetId;
    std::map<unsigned, Network*> mNetworks;  // Map keys are NetIds.
    std::map<uid_t, Permission> mUsers;
    std::set<uid_t> mProtectableUsers;
};

#endif  // NETD_SERVER_NETWORK_CONTROLLER_H

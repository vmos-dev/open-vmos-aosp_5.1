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

// THREAD-SAFETY
// -------------
// The methods in this file are called from multiple threads (from CommandListener, FwmarkServer
// and DnsProxyListener). So, all accesses to shared state are guarded by a lock.
//
// In some cases, a single non-const method acquires and releases the lock several times, like so:
//     if (isValidNetwork(...)) {  // isValidNetwork() acquires and releases the lock.
//        setDefaultNetwork(...);  // setDefaultNetwork() also acquires and releases the lock.
//
// It might seem that this allows races where the state changes between the two statements, but in
// fact there are no races because:
//     1. This pattern only occurs in non-const methods (i.e., those that mutate state).
//     2. Only CommandListener calls these non-const methods. The others call only const methods.
//     3. CommandListener only processes one command at a time. I.e., it's serialized.
// Thus, no other mutation can occur in between the two statements above.

#include "NetworkController.h"

#include "Fwmark.h"
#include "LocalNetwork.h"
#include "PhysicalNetwork.h"
#include "RouteController.h"
#include "VirtualNetwork.h"

#include "cutils/misc.h"
#define LOG_TAG "Netd"
#include "log/log.h"
#include "resolv_netid.h"

namespace {

// Keep these in sync with ConnectivityService.java.
const unsigned MIN_NET_ID = 100;
const unsigned MAX_NET_ID = 65535;

}  // namespace

const unsigned NetworkController::MIN_OEM_ID   =  1;
const unsigned NetworkController::MAX_OEM_ID   = 50;
// NetIds 51..98 are reserved for future use.
const unsigned NetworkController::LOCAL_NET_ID = 99;

// All calls to methods here are made while holding a write lock on mRWLock.
class NetworkController::DelegateImpl : public PhysicalNetwork::Delegate {
public:
    explicit DelegateImpl(NetworkController* networkController);
    virtual ~DelegateImpl();

    int modifyFallthrough(unsigned vpnNetId, const std::string& physicalInterface,
                          Permission permission, bool add) WARN_UNUSED_RESULT;

private:
    int addFallthrough(const std::string& physicalInterface,
                       Permission permission) override WARN_UNUSED_RESULT;
    int removeFallthrough(const std::string& physicalInterface,
                          Permission permission) override WARN_UNUSED_RESULT;

    int modifyFallthrough(const std::string& physicalInterface, Permission permission,
                          bool add) WARN_UNUSED_RESULT;

    NetworkController* const mNetworkController;
};

NetworkController::DelegateImpl::DelegateImpl(NetworkController* networkController) :
        mNetworkController(networkController) {
}

NetworkController::DelegateImpl::~DelegateImpl() {
}

int NetworkController::DelegateImpl::modifyFallthrough(unsigned vpnNetId,
                                                       const std::string& physicalInterface,
                                                       Permission permission, bool add) {
    if (add) {
        if (int ret = RouteController::addVirtualNetworkFallthrough(vpnNetId,
                                                                    physicalInterface.c_str(),
                                                                    permission)) {
            ALOGE("failed to add fallthrough to %s for VPN netId %u", physicalInterface.c_str(),
                  vpnNetId);
            return ret;
        }
    } else {
        if (int ret = RouteController::removeVirtualNetworkFallthrough(vpnNetId,
                                                                       physicalInterface.c_str(),
                                                                       permission)) {
            ALOGE("failed to remove fallthrough to %s for VPN netId %u", physicalInterface.c_str(),
                  vpnNetId);
            return ret;
        }
    }
    return 0;
}

int NetworkController::DelegateImpl::addFallthrough(const std::string& physicalInterface,
                                                    Permission permission) {
    return modifyFallthrough(physicalInterface, permission, true);
}

int NetworkController::DelegateImpl::removeFallthrough(const std::string& physicalInterface,
                                                       Permission permission) {
    return modifyFallthrough(physicalInterface, permission, false);
}

int NetworkController::DelegateImpl::modifyFallthrough(const std::string& physicalInterface,
                                                       Permission permission, bool add) {
    for (const auto& entry : mNetworkController->mNetworks) {
        if (entry.second->getType() == Network::VIRTUAL) {
            if (int ret = modifyFallthrough(entry.first, physicalInterface, permission, add)) {
                return ret;
            }
        }
    }
    return 0;
}

NetworkController::NetworkController() :
        mDelegateImpl(new NetworkController::DelegateImpl(this)), mDefaultNetId(NETID_UNSET) {
    mNetworks[LOCAL_NET_ID] = new LocalNetwork(LOCAL_NET_ID);
}

unsigned NetworkController::getDefaultNetwork() const {
    android::RWLock::AutoRLock lock(mRWLock);
    return mDefaultNetId;
}

int NetworkController::setDefaultNetwork(unsigned netId) {
    android::RWLock::AutoWLock lock(mRWLock);

    if (netId == mDefaultNetId) {
        return 0;
    }

#if 0
    if (netId != NETID_UNSET) {
        Network* network = getNetworkLocked(netId);
        if (!network) {
            ALOGE("no such netId %u", netId);
            return -ENONET;
        }
        if (network->getType() != Network::PHYSICAL) {
            ALOGE("cannot set default to non-physical network with netId %u", netId);
            return -EINVAL;
        }
        if (int ret = static_cast<PhysicalNetwork*>(network)->addAsDefault()) {
            return ret;
        }
    }

    if (mDefaultNetId != NETID_UNSET) {
        Network* network = getNetworkLocked(mDefaultNetId);
        if (!network || network->getType() != Network::PHYSICAL) {
            ALOGE("cannot find previously set default network with netId %u", mDefaultNetId);
            return -ESRCH;
        }
        if (int ret = static_cast<PhysicalNetwork*>(network)->removeAsDefault()) {
            return ret;
        }
    }
#endif
    mDefaultNetId = netId;
    return 0;
}

uint32_t NetworkController::getNetworkForDns(unsigned* netId, uid_t uid) const {
    android::RWLock::AutoRLock lock(mRWLock);
    Fwmark fwmark;
    fwmark.protectedFromVpn = true;
    fwmark.permission = PERMISSION_SYSTEM;
#if 1
    (void)uid;
    *netId = mDefaultNetId;
#else
    if (checkUserNetworkAccessLocked(uid, *netId) == 0) {
        // If a non-zero NetId was explicitly specified, and the user has permission for that
        // network, use that network's DNS servers. Do not fall through to the default network even
        // if the explicitly selected network is a split tunnel VPN or a VPN without DNS servers.
        fwmark.explicitlySelected = true;
    } else {
        // If the user is subject to a VPN and the VPN provides DNS servers, use those servers
        // (possibly falling through to the default network if the VPN doesn't provide a route to
        // them). Otherwise, use the default network's DNS servers.
        VirtualNetwork* virtualNetwork = getVirtualNetworkForUserLocked(uid);
        if (virtualNetwork && virtualNetwork->getHasDns()) {
            *netId = virtualNetwork->getNetId();
        } else {
            *netId = mDefaultNetId;
        }
    }
#endif
    fwmark.netId = *netId;
    return fwmark.intValue;
}

// Returns the NetId that a given UID would use if no network is explicitly selected. Specifically,
// the VPN that applies to the UID if any; otherwise, the default network.
unsigned NetworkController::getNetworkForUser(uid_t uid) const {
    android::RWLock::AutoRLock lock(mRWLock);
    if (VirtualNetwork* virtualNetwork = getVirtualNetworkForUserLocked(uid)) {
        return virtualNetwork->getNetId();
    }
    return mDefaultNetId;
}

// Returns the NetId that will be set when a socket connect()s. This is the bypassable VPN that
// applies to the user if any; otherwise, the default network.
//
// In general, we prefer to always set the default network's NetId in connect(), so that if the VPN
// is a split-tunnel and disappears later, the socket continues working (since the default network's
// NetId is still valid). Secure VPNs will correctly grab the socket's traffic since they have a
// high-priority routing rule that doesn't care what NetId the socket has.
//
// But bypassable VPNs have a very low priority rule, so we need to mark the socket with the
// bypassable VPN's NetId if we expect it to get any traffic at all. If the bypassable VPN is a
// split-tunnel, that's okay, because we have fallthrough rules that will direct the fallthrough
// traffic to the default network. But it does mean that if the bypassable VPN goes away (and thus
// the fallthrough rules also go away), the socket that used to fallthrough to the default network
// will stop working.
unsigned NetworkController::getNetworkForConnect(uid_t uid) const {
    android::RWLock::AutoRLock lock(mRWLock);
    VirtualNetwork* virtualNetwork = getVirtualNetworkForUserLocked(uid);
    if (virtualNetwork && !virtualNetwork->isSecure()) {
        return virtualNetwork->getNetId();
    }
    return mDefaultNetId;
}

unsigned NetworkController::getNetworkForInterface(const char* interface) const {
    android::RWLock::AutoRLock lock(mRWLock);
    for (const auto& entry : mNetworks) {
        if (entry.second->hasInterface(interface)) {
            return entry.first;
        }
    }
    return NETID_UNSET;
}

bool NetworkController::isVirtualNetwork(unsigned netId) const {
    android::RWLock::AutoRLock lock(mRWLock);
    Network* network = getNetworkLocked(netId);
    return network && network->getType() == Network::VIRTUAL;
}

int NetworkController::createPhysicalNetwork(unsigned netId, Permission permission) {
    if (!((MIN_NET_ID <= netId && netId <= MAX_NET_ID) ||
          (MIN_OEM_ID <= netId && netId <= MAX_OEM_ID))) {
        ALOGE("invalid netId %u", netId);
        return -EINVAL;
    }

    if (isValidNetwork(netId)) {
        ALOGE("duplicate netId %u", netId);
        return -EEXIST;
    }

    PhysicalNetwork* physicalNetwork = new PhysicalNetwork(netId, mDelegateImpl);
    if (int ret = physicalNetwork->setPermission(permission)) {
        ALOGE("inconceivable! setPermission cannot fail on an empty network");
        delete physicalNetwork;
        return ret;
    }

    android::RWLock::AutoWLock lock(mRWLock);
    mNetworks[netId] = physicalNetwork;
    return 0;
}

int NetworkController::createVirtualNetwork(unsigned netId, bool hasDns, bool secure) {
    if (!(MIN_NET_ID <= netId && netId <= MAX_NET_ID)) {
        ALOGE("invalid netId %u", netId);
        return -EINVAL;
    }

    if (isValidNetwork(netId)) {
        ALOGE("duplicate netId %u", netId);
        return -EEXIST;
    }

    android::RWLock::AutoWLock lock(mRWLock);
    if (int ret = modifyFallthroughLocked(netId, true)) {
        return ret;
    }
    mNetworks[netId] = new VirtualNetwork(netId, hasDns, secure);
    return 0;
}

int NetworkController::destroyNetwork(unsigned netId) {
    if (netId == LOCAL_NET_ID) {
        ALOGE("cannot destroy local network");
        return -EINVAL;
    }
    if (!isValidNetwork(netId)) {
        ALOGE("no such netId %u", netId);
        return -ENONET;
    }

    // TODO: ioctl(SIOCKILLADDR, ...) to kill all sockets on the old network.

    android::RWLock::AutoWLock lock(mRWLock);
    Network* network = getNetworkLocked(netId);

    // If we fail to destroy a network, things will get stuck badly. Therefore, unlike most of the
    // other network code, ignore failures and attempt to clear out as much state as possible, even
    // if we hit an error on the way. Return the first error that we see.
    int ret = network->clearInterfaces();

    if (mDefaultNetId == netId) {
#if 0
        if (int err = static_cast<PhysicalNetwork*>(network)->removeAsDefault()) {
            ALOGE("inconceivable! removeAsDefault cannot fail on an empty network");
            if (!ret) {
                ret = err;
            }
        }
#endif
        mDefaultNetId = NETID_UNSET;
    } else if (network->getType() == Network::VIRTUAL) {
        if (int err = modifyFallthroughLocked(netId, false)) {
            if (!ret) {
                ret = err;
            }
        }
    }
    mNetworks.erase(netId);
    delete network;
    _resolv_delete_cache_for_net(netId);
    return ret;
}

int NetworkController::addInterfaceToNetwork(unsigned netId, const char* interface) {
    if (!isValidNetwork(netId)) {
        ALOGE("no such netId %u", netId);
        return -ENONET;
    }

    unsigned existingNetId = getNetworkForInterface(interface);
    if (existingNetId != NETID_UNSET && existingNetId != netId) {
        ALOGE("interface %s already assigned to netId %u", interface, existingNetId);
        return -EBUSY;
    }

    android::RWLock::AutoWLock lock(mRWLock);
    return getNetworkLocked(netId)->addInterface(interface);
}

int NetworkController::removeInterfaceFromNetwork(unsigned netId, const char* interface) {
    if (!isValidNetwork(netId)) {
        ALOGE("no such netId %u", netId);
        return -ENONET;
    }

    android::RWLock::AutoWLock lock(mRWLock);
    return getNetworkLocked(netId)->removeInterface(interface);
}

Permission NetworkController::getPermissionForUser(uid_t uid) const {
    android::RWLock::AutoRLock lock(mRWLock);
    return getPermissionForUserLocked(uid);
}

void NetworkController::setPermissionForUsers(Permission permission,
                                              const std::vector<uid_t>& uids) {
    android::RWLock::AutoWLock lock(mRWLock);
    for (uid_t uid : uids) {
        mUsers[uid] = permission;
    }
}

int NetworkController::checkUserNetworkAccess(uid_t uid, unsigned netId) const {
    android::RWLock::AutoRLock lock(mRWLock);
    return checkUserNetworkAccessLocked(uid, netId);
}

int NetworkController::setPermissionForNetworks(Permission permission,
                                                const std::vector<unsigned>& netIds) {
    android::RWLock::AutoWLock lock(mRWLock);
    for (unsigned netId : netIds) {
        Network* network = getNetworkLocked(netId);
        if (!network) {
            ALOGE("no such netId %u", netId);
            return -ENONET;
        }
        if (network->getType() != Network::PHYSICAL) {
            ALOGE("cannot set permissions on non-physical network with netId %u", netId);
            return -EINVAL;
        }

        // TODO: ioctl(SIOCKILLADDR, ...) to kill socets on the network that don't have permission.

        if (int ret = static_cast<PhysicalNetwork*>(network)->setPermission(permission)) {
            return ret;
        }
    }
    return 0;
}

int NetworkController::addUsersToNetwork(unsigned netId, const UidRanges& uidRanges) {
    android::RWLock::AutoWLock lock(mRWLock);
    Network* network = getNetworkLocked(netId);
    if (!network) {
        ALOGE("no such netId %u", netId);
        return -ENONET;
    }
    if (network->getType() != Network::VIRTUAL) {
        ALOGE("cannot add users to non-virtual network with netId %u", netId);
        return -EINVAL;
    }
    if (int ret = static_cast<VirtualNetwork*>(network)->addUsers(uidRanges)) {
        return ret;
    }
    return 0;
}

int NetworkController::removeUsersFromNetwork(unsigned netId, const UidRanges& uidRanges) {
    android::RWLock::AutoWLock lock(mRWLock);
    Network* network = getNetworkLocked(netId);
    if (!network) {
        ALOGE("no such netId %u", netId);
        return -ENONET;
    }
    if (network->getType() != Network::VIRTUAL) {
        ALOGE("cannot remove users from non-virtual network with netId %u", netId);
        return -EINVAL;
    }
    if (int ret = static_cast<VirtualNetwork*>(network)->removeUsers(uidRanges)) {
        return ret;
    }
    return 0;
}

int NetworkController::addRoute(unsigned netId, const char* interface, const char* destination,
                                const char* nexthop, bool legacy, uid_t uid) {
    return modifyRoute(netId, interface, destination, nexthop, true, legacy, uid);
}

int NetworkController::removeRoute(unsigned netId, const char* interface, const char* destination,
                                   const char* nexthop, bool legacy, uid_t uid) {
    return modifyRoute(netId, interface, destination, nexthop, false, legacy, uid);
}

bool NetworkController::canProtect(uid_t uid) const {
    android::RWLock::AutoRLock lock(mRWLock);
    return ((getPermissionForUserLocked(uid) & PERMISSION_SYSTEM) == PERMISSION_SYSTEM) ||
           mProtectableUsers.find(uid) != mProtectableUsers.end();
}

void NetworkController::allowProtect(const std::vector<uid_t>& uids) {
    android::RWLock::AutoWLock lock(mRWLock);
    mProtectableUsers.insert(uids.begin(), uids.end());
}

void NetworkController::denyProtect(const std::vector<uid_t>& uids) {
    android::RWLock::AutoWLock lock(mRWLock);
    for (uid_t uid : uids) {
        mProtectableUsers.erase(uid);
    }
}

bool NetworkController::isValidNetwork(unsigned netId) const {
    android::RWLock::AutoRLock lock(mRWLock);
    return getNetworkLocked(netId);
}

Network* NetworkController::getNetworkLocked(unsigned netId) const {
    auto iter = mNetworks.find(netId);
    return iter == mNetworks.end() ? NULL : iter->second;
}

VirtualNetwork* NetworkController::getVirtualNetworkForUserLocked(uid_t uid) const {
    for (const auto& entry : mNetworks) {
        if (entry.second->getType() == Network::VIRTUAL) {
            VirtualNetwork* virtualNetwork = static_cast<VirtualNetwork*>(entry.second);
            if (virtualNetwork->appliesToUser(uid)) {
                return virtualNetwork;
            }
        }
    }
    return NULL;
}

Permission NetworkController::getPermissionForUserLocked(uid_t uid) const {
    auto iter = mUsers.find(uid);
    if (iter != mUsers.end()) {
        return iter->second;
    }
    return PERMISSION_SYSTEM;//uid < FIRST_APPLICATION_UID ? PERMISSION_SYSTEM : PERMISSION_NONE;
}

int NetworkController::checkUserNetworkAccessLocked(uid_t uid, unsigned netId) const {
    Network* network = getNetworkLocked(netId);
    if (!network) {
        return -ENONET;
    }

    // If uid is INVALID_UID, this likely means that we were unable to retrieve the UID of the peer
    // (using SO_PEERCRED). Be safe and deny access to the network, even if it's valid.
    if (uid == INVALID_UID) {
        return -EREMOTEIO;
    }
    Permission userPermission = getPermissionForUserLocked(uid);
    if ((userPermission & PERMISSION_SYSTEM) == PERMISSION_SYSTEM) {
        return 0;
    }
    if (network->getType() == Network::VIRTUAL) {
        return static_cast<VirtualNetwork*>(network)->appliesToUser(uid) ? 0 : -EPERM;
    }
    VirtualNetwork* virtualNetwork = getVirtualNetworkForUserLocked(uid);
    if (virtualNetwork && virtualNetwork->isSecure() &&
            mProtectableUsers.find(uid) == mProtectableUsers.end()) {
        return -EPERM;
    }
    Permission networkPermission = static_cast<PhysicalNetwork*>(network)->getPermission();
    return ((userPermission & networkPermission) == networkPermission) ? 0 : -EACCES;
}

int NetworkController::modifyRoute(unsigned netId, const char* interface, const char* destination,
                                   const char* nexthop, bool add, bool legacy, uid_t uid) {
    if (!isValidNetwork(netId)) {
        ALOGE("no such netId %u", netId);
        return -ENONET;
    }
    unsigned existingNetId = getNetworkForInterface(interface);
    if (existingNetId == NETID_UNSET) {
        ALOGE("interface %s not assigned to any netId", interface);
        return -ENODEV;
    }
    if (existingNetId != netId) {
        ALOGE("interface %s assigned to netId %u, not %u", interface, existingNetId, netId);
        return -ENOENT;
    }

    RouteController::TableType tableType;
    if (netId == LOCAL_NET_ID) {
        tableType = RouteController::LOCAL_NETWORK;
    } else if (legacy) {
        if ((getPermissionForUser(uid) & PERMISSION_SYSTEM) == PERMISSION_SYSTEM) {
            tableType = RouteController::LEGACY_SYSTEM;
        } else {
            tableType = RouteController::LEGACY_NETWORK;
        }
    } else {
        tableType = RouteController::INTERFACE;
    }

    return add ? RouteController::addRoute(interface, destination, nexthop, tableType) :
                 RouteController::removeRoute(interface, destination, nexthop, tableType);
}

int NetworkController::modifyFallthroughLocked(unsigned vpnNetId, bool add) {
    if (mDefaultNetId == NETID_UNSET) {
        return 0;
    }
    Network* network = getNetworkLocked(mDefaultNetId);
    if (!network) {
        ALOGE("cannot find previously set default network with netId %u", mDefaultNetId);
        return -ESRCH;
    }
    if (network->getType() != Network::PHYSICAL) {
        ALOGE("inconceivable! default network must be a physical network");
        return -EINVAL;
    }
    Permission permission = static_cast<PhysicalNetwork*>(network)->getPermission();
    for (const auto& physicalInterface : network->getInterfaces()) {
        if (int ret = mDelegateImpl->modifyFallthrough(vpnNetId, physicalInterface, permission,
                                                       add)) {
            return ret;
        }
    }
    return 0;
}

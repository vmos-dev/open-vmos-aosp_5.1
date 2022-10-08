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

#ifndef NETD_INCLUDE_PERMISSION_H
#define NETD_INCLUDE_PERMISSION_H

// This enum represents the permissions we care about for networking. When applied to an app, it's
// the permission the app (UID) has been granted. When applied to a network, it's the permission an
// app must hold to be allowed to use the network. PERMISSION_NONE means "no special permission is
// held by the app" or "no special permission is required to use the network".
//
// Permissions are flags that can be OR'ed together to represent combinations of permissions.
//
// PERMISSION_NONE is used for regular networks and apps, such as those that hold the
// android.permission.INTERNET framework permission.
//
// PERMISSION_NETWORK is used for privileged networks and apps that can manipulate or access them,
// such as those that hold the android.permission.CHANGE_NETWORK_STATE framework permission.
//
// PERMISSION_SYSTEM is used for system apps, such as those that are installed on the system
// partition, those that hold the android.permission.CONNECTIVITY_INTERNAL framework permission and
// those whose UID is less than FIRST_APPLICATION_UID.
enum Permission {
    PERMISSION_NONE    = 0x0,
    PERMISSION_NETWORK = 0x1,
    PERMISSION_SYSTEM  = 0x3,  // Includes PERMISSION_NETWORK.
};

#endif  // NETD_INCLUDE_PERMISSION_H

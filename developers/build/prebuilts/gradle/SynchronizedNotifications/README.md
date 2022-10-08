Android SynchronizedNotifications Sample
===================================

A basic sample showing how to use simple or synchronized notifications.
This allows users to dismiss events from either their phone or wearable device simultaneously.

Introduction
------------

The [DataAPI][1] exposes an API for components to read or write data items and assets between
the handhelds and wearables. A [DataItem][2] is synchronized across all devices in an Android Wear network.
It is possible to set data items while not connected to any nodes. Those data items will be synchronized
when the nodes eventually come online.

This example presents three buttons that would trigger three different combinations of
notifications on the handset and the watch:

1. The first button builds a simple local-only notification on the handset.
2. The second one creates a wearable-only notification by putting a data item in the shared data
store and having a [com.google.android.gms.wearable.WearableListenerService][3] listen for
that on the wearable.
3. The third one creates a local notification and a wearable notification by combining the above
two. It, however, demonstrates how one can set things up so that the dismissal of one
notification results in the dismissal of the other one.

In the #2 and #3 items, the following code is used to synchronize the data between the handheld
and the wearable devices using DataAPI.

```java
PutDataMapRequest putDataMapRequest = PutDataMapRequest.create(path);
putDataMapRequest.getDataMap().putString(Constants.KEY_CONTENT, content);
putDataMapRequest.getDataMap().putString(Constants.KEY_TITLE, title);
PutDataRequest request = putDataMapRequest.asPutDataRequest();
Wearable.DataApi.putDataItem(mGoogleApiClient, request)
        .setResultCallback(new ResultCallback<DataApi.DataItemResult>() {
            @Override
            public void onResult(DataApi.DataItemResult dataItemResult) {
                if (!dataItemResult.getStatus().isSuccess()) {
                    Log.e(TAG, "buildWatchOnlyNotification(): Failed to set the data, "
                            + "status: " + dataItemResult.getStatus().getStatusCode());
                }
            }
        });
```

[1]: http://developer.android.com/reference/com/google/android/gms/wearable/DataApi.html#putDataItem(com.google.android.gms.common.api.GoogleApiClient%2C%20com.google.android.gms.wearable.PutDataRequest)
[2]: http://developer.android.com/reference/com/google/android/gms/wearable/DataItem.html
[3]: https://developer.android.com/reference/com/google/android/gms/wearable/WearableListenerService.html

Pre-requisites
--------------

- Android SDK v21
- Android Build Tools v21.1.1
- Android Support Repository

Screenshots
-------------

<img src="screenshots/different_notifications_phone.png" height="400" alt="Screenshot"/> <img src="screenshots/different_notifications_wearable.png" height="400" alt="Screenshot"/> <img src="screenshots/notification_options.png" height="400" alt="Screenshot"/> <img src="screenshots/watch_only_notification.png" height="400" alt="Screenshot"/> 

Getting Started
---------------

This sample uses the Gradle build system. To build this project, use the
"gradlew build" command or use "Import Project" in Android Studio.

Support
-------

- Google+ Community: https://plus.google.com/communities/105153134372062985968
- Stack Overflow: http://stackoverflow.com/questions/tagged/android

If you've found an error in this sample, please file an issue:
https://github.com/googlesamples/android-SynchronizedNotifications

Patches are encouraged, and may be submitted by forking this project and
submitting a pull request through GitHub. Please see CONTRIBUTING.md for more details.

License
-------

Copyright 2014 The Android Open Source Project, Inc.

Licensed to the Apache Software Foundation (ASF) under one or more contributor
license agreements.  See the NOTICE file distributed with this work for
additional information regarding copyright ownership.  The ASF licenses this
file to you under the Apache License, Version 2.0 (the "License"); you may not
use this file except in compliance with the License.  You may obtain a copy of
the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
License for the specific language governing permissions and limitations under
the License.

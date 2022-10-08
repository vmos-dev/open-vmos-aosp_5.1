Android Wearable Notifications Sample
===================================

This sample showcases the available notification styles on a device and wearable.

Introduction
------------

This sample app enables the user to choose a notification type (the selection is done on the phone),
which it then displays on the phone and wearable. The notification is built using
[NotificationCompat.Builder][1] and [NotificationCompat.WearableExtender][2].

See [Creating a Notification][3] for all the details on how to create a notification with wearable features.

On the wearable side, the sample also shows how to create a custom layout using [WearableListView][4].

[1]: http://developer.android.com/reference/android/support/v4/app/NotificationCompat.Builder.html
[2]: https://developer.android.com/reference/android/support/v4/app/NotificationCompat.WearableExtender.html
[3]: https://developer.android.com/training/wearables/notifications/creating.html
[4]: https://developer.android.com/training/wearables/apps/layouts.html#UiLibrary

Pre-requisites
--------------

- Android SDK v21
- Android Build Tools v21.1.1
- Android Support Repository

Screenshots
-------------

<img src="screenshots/companion-content-action.png" height="400" alt="Screenshot"/> <img src="screenshots/content-action" height="400" alt="Screenshot"/> <img src="screenshots/content-icon-menu" height="400" alt="Screenshot"/> 

Getting Started
---------------

This sample uses the Gradle build system. To build this project, use the
"gradlew build" command or use "Import Project" in Android Studio.

Support
-------

- Google+ Community: https://plus.google.com/communities/105153134372062985968
- Stack Overflow: http://stackoverflow.com/questions/tagged/android

If you've found an error in this sample, please file an issue:
https://github.com/googlesamples/android-Wearable Notifications

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

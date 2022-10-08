Android AgendaData Sample
===================================

Sample demonstrating sync of calendar events to a wearable by the press of a button.

Introduction
------------

Using the Wearable [DataApi][1] allows to transmit data such as event time,
description, and background image.

The sent [DataItems][2] can be deleted individually via an action on the event notifications,
or all at once via a button on the companion.

When deleted using the notification action, a ConfirmationActivity is used to indicate
success or failure. The sample shows implementations for both the success as well failure case.

[1]: https://developer.android.com/reference/com/google/android/gms/wearable/DataApi.html
[2]: https://developer.android.com/reference/com/google/android/gms/wearable/DataItem.html

Pre-requisites
--------------

- Android SDK v21
- Android Build Tools v21.1.1
- Android Support Repository

Screenshots
-------------

<img src="screenshots/companion_agenda_data.png" height="400" alt="Screenshot"/> <img src="screenshots/dummy_calendar_event.png" height="400" alt="Screenshot"/> 

Getting Started
---------------

This sample uses the Gradle build system. To build this project, use the
"gradlew build" command or use "Import Project" in Android Studio.

Support
-------

- Google+ Community: https://plus.google.com/communities/105153134372062985968
- Stack Overflow: http://stackoverflow.com/questions/tagged/android

If you've found an error in this sample, please file an issue:
https://github.com/googlesamples/android-AgendaData

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
